// PS/2 수신 엔진 (장치 -> 호스트 전용). 송신(호스트 -> 장치)은 만들지 않았다.
// 프레임: 시작 0, 데이터 8비트 LSB 먼저, 홀수 패리티, 정지 1 (Digilent Pmod PS2 문서).
// 데이터는 클럭 하강 에지에서 유효 (같은 문서). 입력 두 선 모두 2단 동기 + 안정 필터.
module ps2_rx_engine #(
    parameter CLK_MHZ     = 100,
    parameter FILTER_CLKS = 8,      // 설계안: 80 ns 안정이어야 반영 (Tck 최소 30 us 보다 훨씬 짧음)
    parameter TIMEOUT_US  = 2000    // 설계안: 비트 사이 이 시간 넘게 클럭이 없으면 프레임 폐기 (Tck 최대 50 us)
) (
    input  wire       clk,
    input  wire       rstn,
    input  wire       ps2c_i,
    input  wire       ps2d_i,
    output reg        byte_valid,   // 1클럭 펄스
    output reg  [7:0] byte_out,
    output reg        err_parity,   // 1클럭 펄스
    output reg        err_frame,    // 시작/정지 비트 오류
    output reg        err_timeout
);
    localparam [23:0] TIMEOUT_CLKS = TIMEOUT_US * CLK_MHZ;

    reg [1:0] c_sync, d_sync;
    reg       c_f, d_f, c_prev;
    reg [7:0] c_cnt, d_cnt;

    always @(posedge clk) begin
        if (!rstn) begin
            c_sync <= 2'b11; d_sync <= 2'b11; c_f <= 1'b1; d_f <= 1'b1; c_prev <= 1'b1;
            c_cnt <= 8'd0; d_cnt <= 8'd0;
        end else begin
            c_sync <= {c_sync[0], ps2c_i};
            d_sync <= {d_sync[0], ps2d_i};
            c_prev <= c_f;
            if (c_sync[1] != c_f) begin
                if (c_cnt == FILTER_CLKS - 1) begin c_f <= c_sync[1]; c_cnt <= 8'd0; end
                else c_cnt <= c_cnt + 8'd1;
            end else c_cnt <= 8'd0;
            if (d_sync[1] != d_f) begin
                if (d_cnt == FILTER_CLKS - 1) begin d_f <= d_sync[1]; d_cnt <= 8'd0; end
                else d_cnt <= d_cnt + 8'd1;
            end else d_cnt <= 8'd0;
        end
    end

    wire fall = c_prev & ~c_f;

    reg [3:0]  bitcnt;     // 0 = 시작 비트 대기, 1..8 데이터, 9 패리티, 10 정지
    reg [7:0]  sh;
    reg        par_bit;
    reg [23:0] idle;

    always @(posedge clk) begin
        byte_valid <= 1'b0; err_parity <= 1'b0; err_frame <= 1'b0; err_timeout <= 1'b0;
        if (!rstn) begin
            bitcnt <= 4'd0; sh <= 8'd0; par_bit <= 1'b0; idle <= 24'd0; byte_out <= 8'd0;
        end else if (fall) begin
            idle <= 24'd0;
            case (bitcnt)
                4'd0: if (d_f == 1'b0) bitcnt <= 4'd1; else err_frame <= 1'b1;   // 시작 비트는 0
                4'd9: begin par_bit <= d_f; bitcnt <= 4'd10; end
                4'd10: begin
                    bitcnt <= 4'd0;
                    if (d_f != 1'b1)                err_frame  <= 1'b1;          // 정지 비트는 1
                    else if (((^sh) ^ par_bit) != 1'b1) err_parity <= 1'b1;        // 홀수 패리티: 데이터+패리티의 1 개수가 홀수
                    else begin byte_valid <= 1'b1; byte_out <= sh; end
                end
                default: begin sh <= {d_f, sh[7:1]}; bitcnt <= bitcnt + 4'd1; end  // 1..8
            endcase
        end else if (bitcnt != 4'd0) begin
            if (idle >= TIMEOUT_CLKS) begin bitcnt <= 4'd0; idle <= 24'd0; err_timeout <= 1'b1; end
            else idle <= idle + 24'd1;
        end
    end
endmodule

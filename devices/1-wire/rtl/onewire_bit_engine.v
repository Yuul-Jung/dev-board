// 1-Wire 비트 엔진: 리셋/쓰기/읽기 한 동작을 µs 단위로 실행한다.
// 타이밍(µs)은 DS18B20 데이터시트 AC 표 범위 안에서 고른 값이다.
module onewire_bit_engine #(
    parameter CLK_MHZ = 100
) (
    input  wire       clk,
    input  wire       rstn,
    input  wire       start,      // 1클럭 펄스. busy 중에는 무시
    input  wire [1:0] op,         // 0=리셋, 1=쓰기, 2=읽기
    input  wire       wbit,       // 쓰기 비트
    input  wire       ow_i,       // 버스 입력(비동기)
    output reg        ow_low,     // 1이면 버스를 Low로 당김
    output wire       busy,
    output reg        done_pulse, // 동작 종료 시 1클럭
    output reg        presence,   // 리셋 후 소자 응답 있음
    output reg        rbit        // 읽은 비트
);
    // 모두 µs. 괄호는 데이터시트 허용 범위
    localparam T_RST_LOW = 500;  // tRSTL >= 480
    localparam T_PRES    = 570;  // 해제 후 70 us. Presence Low는 해제 후 60~75 us 구간을 항상 포함
    localparam T_RST_END = 1000; // 해제 후 500 us >= tRSTH 480
    localparam T_W0_LOW  = 65;   // tLOW0 60..120
    localparam T_W1_LOW  = 6;    // tLOW1 1..15
    localparam T_W_END   = 80;   // tSLOT 60..120, 회복 시간 tREC >= 1 충족
    localparam T_R_LOW   = 2;    // 읽기 시작 Low > 1 us
    localparam T_R_SMP   = 12;   // tINIT+tRC+tSAMPLE < 15 us 이내
    localparam T_R_END   = 75;   // 슬롯 >= 60 us

    reg        run;
    reg [7:0]  c;     // 클럭 카운터 (1 us 분주)
    reg [10:0] t;     // 경과 us
    reg [1:0]  op_r;
    reg        wbit_r;
    reg        s1, s2;

    assign busy = run;

    reg low_cond;
    always @(*) begin
        case (op_r)
            2'd0:    low_cond = run && (t < T_RST_LOW);
            2'd1:    low_cond = run && (t < (wbit_r ? T_W1_LOW : T_W0_LOW));
            2'd2:    low_cond = run && (t < T_R_LOW);
            default: low_cond = 1'b0;
        endcase
    end

    always @(posedge clk) begin
        s1 <= ow_i;
        s2 <= s1;
        ow_low <= low_cond;
        done_pulse <= 1'b0;
        if (!rstn) begin
            run <= 1'b0; c <= 8'd0; t <= 11'd0;
            op_r <= 2'd0; wbit_r <= 1'b0;
            presence <= 1'b0; rbit <= 1'b0;
        end else if (!run) begin
            if (start) begin
                run <= 1'b1; c <= 8'd0; t <= 11'd0;
                op_r <= op; wbit_r <= wbit;
            end
        end else begin
            if (c == CLK_MHZ - 1) begin
                c <= 8'd0;
                t <= t + 11'd1;
            end else begin
                c <= c + 8'd1;
            end
            if (c == 8'd0) begin
                case (op_r)
                    2'd0: begin
                        if (t == T_PRES) presence <= ~s2;
                        if (t == T_RST_END) begin run <= 1'b0; done_pulse <= 1'b1; end
                    end
                    2'd1: begin
                        if (t == T_W_END) begin run <= 1'b0; done_pulse <= 1'b1; end
                    end
                    2'd2: begin
                        if (t == T_R_SMP) rbit <= s2;
                        if (t == T_R_END) begin run <= 1'b0; done_pulse <= 1'b1; end
                    end
                    default: begin run <= 1'b0; done_pulse <= 1'b1; end
                endcase
            end
        end
    end
endmodule

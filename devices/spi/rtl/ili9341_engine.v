// ILI9341 4선 SPI 바이트 송신 엔진 (SPI 모드 0, MSB 먼저).
// 초기화 순서와 대기 시간은 이 모듈이 정하지 않는다. 소프트웨어가 정한다
// (데이터시트에서 해당 값을 확인하지 못했기 때문).
// 기능: (1) 1바이트 송신 + D/CX 레벨, (2) 같은 16비트 색을 N픽셀 연속 송신(fill).
// SCL 주기 = 2*(div+1) 클럭. 모드 0: SCL 대기 Low, 상승 에지에서 슬레이브가 읽고 하강 에지에서 SDA 변경.
module ili9341_engine (
    input  wire        clk,
    input  wire        rstn,
    input  wire [15:0] div,
    input  wire        tx_start,      // 1클럭 펄스, busy가 0일 때만
    input  wire [7:0]  tx_byte,
    input  wire        tx_dc,         // 0 명령, 1 데이터
    input  wire        fill_start,    // 1클럭 펄스
    input  wire [15:0] fill_color,
    input  wire [19:0] fill_cnt,      // 픽셀 수 (240*320 = 76800 < 2^20)
    output wire        busy,
    output reg         scl,
    output reg         sda,
    output reg         dcx
);
    reg        active;
    reg        fill_active;
    reg        hi_phase;
    reg [7:0]  sh;
    reg [2:0]  bits;
    reg [15:0] cnt;
    reg [19:0] fill_left;
    reg [15:0] color_r;

    assign busy = active;

    always @(posedge clk) begin
        if (!rstn) begin
            active <= 1'b0; fill_active <= 1'b0; hi_phase <= 1'b0;
            scl <= 1'b0; sda <= 1'b0; dcx <= 1'b0;
            sh <= 8'd0; bits <= 3'd0; cnt <= 16'd0;
            fill_left <= 20'd0; color_r <= 16'd0;
        end else if (!active) begin
            if (tx_start) begin
                sh <= tx_byte; sda <= tx_byte[7]; dcx <= tx_dc;
                bits <= 3'd0; cnt <= 16'd0; scl <= 1'b0;
                fill_active <= 1'b0; active <= 1'b1;
            end else if (fill_start && fill_cnt != 20'd0) begin
                color_r <= fill_color; fill_left <= fill_cnt; hi_phase <= 1'b1;
                sh <= fill_color[15:8]; sda <= fill_color[15]; dcx <= 1'b1;
                bits <= 3'd0; cnt <= 16'd0; scl <= 1'b0;
                fill_active <= 1'b1; active <= 1'b1;
            end
        end else begin
            if (cnt == div) begin
                cnt <= 16'd0;
                if (!scl) begin
                    scl <= 1'b1;                     // 상승 에지
                end else begin
                    scl <= 1'b0;                     // 하강 에지
                    if (bits == 3'd7) begin          // 바이트 끝
                        bits <= 3'd0;
                        if (!fill_active) begin
                            active <= 1'b0;
                        end else if (hi_phase) begin
                            hi_phase <= 1'b0;
                            sh <= color_r[7:0]; sda <= color_r[7];
                        end else if (fill_left == 20'd1) begin
                            fill_left <= 20'd0; fill_active <= 1'b0; active <= 1'b0;
                        end else begin
                            fill_left <= fill_left - 20'd1; hi_phase <= 1'b1;
                            sh <= color_r[15:8]; sda <= color_r[15];
                        end
                    end else begin
                        bits <= bits + 3'd1;
                        sh <= {sh[6:0], 1'b0};
                        sda <= sh[6];
                    end
                end
            end else begin
                cnt <= cnt + 16'd1;
            end
        end
    end
endmodule

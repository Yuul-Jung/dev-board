// 서보 PWM 엔진: 주기 period_us, 펄스 폭 width_us (마이크로초 단위).
// 폭은 [min_us, max_us] 로 제한된다. 한계값은 소프트웨어가 정한다(SG90 자료마다 범위가 달라서).
module servo_engine #(
    parameter CLK_MHZ = 100
) (
    input  wire        clk,
    input  wire        rstn,
    input  wire        enable,
    input  wire [15:0] period_us,
    input  wire [15:0] width_us,
    input  wire [15:0] min_us,
    input  wire [15:0] max_us,
    output reg         pwm,
    output wire [15:0] applied_us
);
    reg [7:0]  tick_cnt;
    reg [15:0] us_cnt;
    wire tick = (tick_cnt == CLK_MHZ - 1);

    wire [15:0] lim_lo = (min_us > max_us) ? max_us : min_us;
    wire [15:0] lim_hi = (min_us > max_us) ? min_us : max_us;
    wire [15:0] w_clamped = (width_us < lim_lo) ? lim_lo : (width_us > lim_hi) ? lim_hi : width_us;
    // (추가) AXI 직접 쓰기가 C 드라이버의 범위 검사를 우회할 때 출력 차단.
    // 0 폭, 0 주기, 역전된 범위, 주기 이상 펄스는 유효한 서보 명령이 아니다.
    wire config_valid = (period_us != 16'd0) && (min_us != 16'd0) &&
                        (min_us <= max_us) && (max_us < period_us);
    assign applied_us = config_valid ? w_clamped : 16'd0;

    always @(posedge clk) begin
        if (!rstn || !enable || !config_valid) begin
            tick_cnt <= 8'd0; us_cnt <= 16'd0; pwm <= 1'b0;
        end else begin
            tick_cnt <= tick ? 8'd0 : tick_cnt + 8'd1;
            if (tick) begin
                us_cnt <= (us_cnt + 16'd1 >= period_us) ? 16'd0 : us_cnt + 16'd1;
                pwm <= ((us_cnt + 16'd1 >= period_us) ? 16'd0 : us_cnt + 16'd1) < applied_us;
            end
        end
    end
endmodule

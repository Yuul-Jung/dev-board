// 8-bit PWM phase + 9-bit duty(0..256), 0%/100% 명시 지원.
// duty_in은 매 PWM 주기 경계에서만 duty_reg로 샘플링한다.
module pwm_core #(
    parameter integer CLK_DIV = 390
)(
    input  wire       clk,
    input  wire       rst,
    input  wire [8:0] duty_in,
    output wire       pwm_out
);
    localparam integer DIV_W = (CLK_DIV > 1) ? $clog2(CLK_DIV) : 1;
    reg [DIV_W-1:0] div_count;
    reg [7:0] phase;
    reg [8:0] duty_reg;

    // PWM 비교는 조합논리. 9비트 duty=256이면 항상 High.
    assign pwm_out = ({1'b0, phase} < duty_reg);

    always @(posedge clk) begin
        if (rst) begin
            div_count <= {DIV_W{1'b0}};
            phase     <= 8'd0;
            duty_reg  <= 9'd0;
        end else if (div_count == CLK_DIV-1) begin
            div_count <= {DIV_W{1'b0}};
            phase <= phase + 1'b1;
            if (phase == 8'hFF) begin
                // 새 PWM 주기의 첫 번째 phase=0부터 새 Duty 적용
                duty_reg <= (duty_in > 9'd256) ? 9'd256 : duty_in;
            end
        end else begin
            div_count <= div_count + 1'b1;
        end
    end
endmodule

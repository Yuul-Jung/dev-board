// Shell/Wrapper: 보드 핀, wire, 모듈 연결만 담당한다.
// 실제 Basys3 CLK/Reset/LED/SW XDC 및 LED 출력 극성을 확인한다.
module basys3_pwm_led_top(
    input wire clk,
    input wire rst,
    input wire [7:0] sw,
    output wire led_pwm
);
    wire [8:0] duty;
    switch_duty_controller u_duty(
        .clk(clk), .rst(rst), .sw(sw), .duty(duty)
    );
    pwm_core #(.CLK_DIV(390)) u_pwm(
        .clk(clk), .rst(rst), .duty_in(duty), .pwm_out(led_pwm)
    );
endmodule

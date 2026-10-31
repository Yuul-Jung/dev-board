// Top: 외부 포트와 모듈 인스턴스 연결만 수행.
// Basys3 버튼은 Active-High; RGB LED 출력은 Active-High 가정.
// 실제 XDC에서 선택한 버튼/LED 핀을 연결한다.
module basys3_rgb_buttons_top(
    input wire clk,
    input wire rst,
    input wire btn_a,
    input wire btn_b,
    output wire led_r,
    output wire led_g,
    output wire led_b
);
    wire pressed_a, pressed_b;
    button_level u_button_a(
        .clk(clk),.rst(rst),.button_in(btn_a),.pressed(pressed_a));
    button_level u_button_b(
        .clk(clk),.rst(rst),.button_in(btn_b),.pressed(pressed_b));
    rgb_button_controller u_rgb(
        .pressed_a(pressed_a),.pressed_b(pressed_b),
        .led_r(led_r),.led_g(led_g),.led_b(led_b));
endmodule

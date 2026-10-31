// Shell/Wrapper: 보드 핀과 재사용 모듈 연결만 한다.
// rst는 active-high 동기 리셋. 실제 보드의 버튼 극성 및 XDC 확인 필요.
module basys3_button_led_top (
    input  wire clk,
    input  wire rst,
    input  wire btn,
    output wire led
);
    wire press_pulse;
    button_event #(.CLK_HZ(100_000_000), .DEBOUNCE_MS(20), .ACTIVE_LOW(1))
        u_button_event (.clk(clk), .rst(rst), .button_in(btn), .press_pulse(press_pulse));
    led_toggle_controller u_led_toggle_controller (
        .clk(clk), .rst(rst), .press_pulse(press_pulse), .led(led));
endmodule

// 두 버튼 입력은 button_event에서 동기화/디바운스한 상태를 받는다.
// 버튼 상태 조합: 00 OFF, 10 RED, 01 GREEN, 11 BLUE.
// 조합논리이므로 버튼 안정 상태가 바뀌면 LED 출력도 변한다.
module rgb_button_controller(
    input wire pressed_a,
    input wire pressed_b,
    output reg led_r,
    output reg led_g,
    output reg led_b
);
    always @(*) begin
        led_r = pressed_a & ~pressed_b;
        led_g = ~pressed_a & pressed_b;
        led_b = pressed_a & pressed_b;
    end
endmodule

// tick이 High인 posedge에서만 LED 상태를 반전.
// enabled가 Low면 LED는 꺼지고, 다시 활성화하면 다음 tick부터 시작.
module timer_led_controller(
    input wire clk,
    input wire rst,
    input wire enabled,
    input wire tick,
    output reg led
);
    always @(posedge clk) begin
        if (rst)
            led <= 1'b0;
        else if (!enabled)
            led <= 1'b0;
        else if (tick)
            led <= ~led;
    end
endmodule

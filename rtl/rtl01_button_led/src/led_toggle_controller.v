// 버튼 이벤트마다 1비트 LED 레지스터 상태를 반전한다.
module led_toggle_controller(
    input  wire clk,
    input  wire rst,
    input  wire press_pulse,
    output reg  led
);
    always @(posedge clk) begin
        if (rst) led <= 1'b0;
        else if (press_pulse) led <= ~led;
    end
endmodule

// UART Byte Stream 수신 명령으로 LED 상태 레지스터 제어.
// '1'(0x31): ON, '0'(0x30): OFF, 기타: 무시.
// rx_valid && rx_ready가 posedge clk에서 참일 때만 수신한다.
// rx_ready는 항상 1: 매 클럭 최대 1바이트 처리.
module uart_led_controller (
    input wire clk,
    input wire rst,
    input wire [7:0] rx_data,
    input wire rx_valid,
    output wire rx_ready,
    output reg led
);
    assign rx_ready = 1'b1;

    always @(posedge clk) begin
        if (rst) begin
            led <= 1'b0;
        end else if (rx_valid && rx_ready) begin
            case (rx_data)
                8'h31: led <= 1'b1; // ASCII '1'
                8'h30: led <= 1'b0; // ASCII '0'
                default: led <= led; // 그 외 입력은 기존 상태 유지
            endcase
        end
    end
endmodule

// 연결 전용 Shell: Byte Stream 수신 포트와 기능 Controller만 연결.
// UART PHY/RX FIFO는 기존 uart_core 또는 호환 코어에 외부 연결한다.
// 보드의 uart_rx 핀을 직접 연결하는 Top이 아니다.
module uart_led_stream_top (
    input wire clk,
    input wire rst,
    input wire [7:0] rx_data,
    input wire rx_valid,
    output wire rx_ready,
    output wire led
);
    uart_led_controller u_uart_led_controller (
        .clk(clk),
        .rst(rst),
        .rx_data(rx_data),
        .rx_valid(rx_valid),
        .rx_ready(rx_ready),
        .led(led)
    );
endmodule

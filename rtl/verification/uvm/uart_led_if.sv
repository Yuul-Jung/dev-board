interface uart_led_if(input logic clk);
  logic rst;
  logic [7:0] rx_data;
  logic rx_valid;
  logic rx_ready;
  logic led;
endinterface

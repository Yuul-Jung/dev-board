`timescale 1ns/1ps
module tb_top;
 import uvm_pkg::*;
 import uart_led_uvm_pkg::*;
 logic clk=0;
 always #5 clk=~clk;
 uart_led_if intf(clk);
 uart_led_controller dut(
  .clk(clk),.rst(intf.rst),.rx_data(intf.rx_data),
  .rx_valid(intf.rx_valid),.rx_ready(intf.rx_ready),.led(intf.led)
 );
 initial begin
   intf.rst=1;
   intf.rx_valid=0;
   intf.rx_data=0;
   repeat(3) @(negedge clk);
   intf.rst=0;
 end
 initial begin
   uvm_config_db#(virtual uart_led_if)::set(null,"*","vif",intf);
   run_test("led_test");
 end
 initial begin
   #100000;
   $fatal(1,"UVM timeout");
 end
endmodule

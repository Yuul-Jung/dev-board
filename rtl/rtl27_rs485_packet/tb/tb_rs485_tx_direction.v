`timescale 1ns/1ps
module tb_rs485_tx_direction;
reg clk=0,rst=1,start=0,uart_ready=0,uart_done=0;wire tx_valid,de,busy,done;
rs485_tx_direction dut(.clk(clk),.rst(rst),.start(start),.uart_ready(uart_ready),.uart_done(uart_done),.tx_valid(tx_valid),.de(de),.busy(busy),.done(done));
initial begin repeat(2)@(negedge clk);rst=0;start=1;@(negedge clk);start=0;if(!de||!tx_valid)$fatal(1,"begin tx");
uart_ready=1;@(negedge clk);uart_ready=0;if(!de||tx_valid)$fatal(1,"wait wire");
uart_done=1;@(negedge clk);if(de||busy||!done)$fatal(1,"complete tx");
$display("PASS rs485_tx_direction");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

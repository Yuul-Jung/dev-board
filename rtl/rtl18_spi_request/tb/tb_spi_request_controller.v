`timescale 1ns/1ps
module tb_spi_request_controller;
reg clk=0,rst=1,start=0,ready=0,done_in=0;wire request,busy,done;
spi_request_controller dut(.clk(clk),.rst(rst),.start(start),.ready(ready),.done_in(done_in),.request(request),.busy(busy),.done(done));
initial begin repeat(2)@(negedge clk);rst=0;start=1;@(negedge clk);start=0;
if(!request||!busy)$fatal(1,"start");
ready=1;@(negedge clk);ready=0;
if(request||!busy)$fatal(1,"accepted");
done_in=1;@(negedge clk);
if(busy||!done)$fatal(1,"done");
$display("PASS spi_request_controller");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

`timescale 1ns/1ps
module tb_i2c_scan_controller;
reg clk=0,rst=1,start=0,cmd_ready=0,reply_valid=0,reply_ack=0;wire cmd_valid,found,done,busy;wire [6:0]address;
i2c_scan_controller dut(.clk(clk),.rst(rst),.start(start),.cmd_ready(cmd_ready),.reply_valid(reply_valid),.reply_ack(reply_ack),.cmd_valid(cmd_valid),.address(address),.found(found),.done(done),.busy(busy));
initial begin repeat(2)@(negedge clk);rst=0;start=1;@(negedge clk);start=0;
if(!cmd_valid||address!=7'h08)$fatal(1,"scan start");
cmd_ready=1;@(negedge clk);cmd_ready=0;reply_ack=1;reply_valid=1;@(negedge clk);reply_valid=0;
if(address!=7'h09||!found)$fatal(1,"scan advance");
$display("PASS i2c_scan_controller");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

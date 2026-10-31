`timescale 1ns/1ps
module tb_dht22_limit_controller;
reg clk=0,rst=1,sample_valid=0,checksum_ok=1;reg signed [15:0]temp_x10=0;reg [15:0]hum_x10=0;wire alarm,sample_error;
dht22_limit_controller dut(.clk(clk),.rst(rst),.temp_x10(temp_x10),.hum_x10(hum_x10),.sample_valid(sample_valid),.checksum_ok(checksum_ok),.alarm(alarm),.sample_error(sample_error));
initial begin repeat(2)@(negedge clk);rst=0;temp_x10=310;sample_valid=1;@(negedge clk);
if(!alarm)$fatal(1,"temperature threshold");
checksum_ok=0;temp_x10=0;@(negedge clk);
if(!sample_error||!alarm)$fatal(1,"checksum must retain previous alarm");
$display("PASS dht22_limit_controller");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

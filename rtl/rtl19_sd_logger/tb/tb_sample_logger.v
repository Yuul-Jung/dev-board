`timescale 1ns/1ps
module tb_sample_logger;
reg clk=0,rst=1,sensor_valid=0,write_ready=0;reg [15:0]sensor_data=0;wire [15:0]write_data;wire write_valid,overflow;
sample_logger dut(.clk(clk),.rst(rst),.sensor_data(sensor_data),.sensor_valid(sensor_valid),.write_ready(write_ready),.write_data(write_data),.write_valid(write_valid),.overflow(overflow));
initial begin repeat(2)@(negedge clk);rst=0;sensor_data=16'h1234;sensor_valid=1;@(negedge clk);
if(!write_valid||write_data!=16'h1234)$fatal(1,"captured");
sensor_data=16'h5678;@(negedge clk);if(!overflow)$fatal(1,"overflow");
$display("PASS sample_logger");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

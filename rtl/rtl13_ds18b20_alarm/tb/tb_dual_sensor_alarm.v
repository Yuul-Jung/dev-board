`timescale 1ns/1ps
module tb_dual_sensor_alarm;
reg clk=0,rst=1,temp_valid=0,distance_valid=0;reg signed [15:0]temp_x16=0;reg [15:0]distance_cm=0;wire alarm,hot,near;
dual_sensor_alarm dut(.clk(clk),.rst(rst),.temp_x16(temp_x16),.temp_valid(temp_valid),.distance_cm(distance_cm),.distance_valid(distance_valid),.alarm(alarm),.temp_hot(hot),.dist_near(near));
initial begin repeat(2)@(negedge clk);rst=0;temp_x16=500;temp_valid=1;@(negedge clk);
if(!hot||!alarm)$fatal(1,"hot temperature");
temp_x16=100;distance_cm=10;distance_valid=1;@(negedge clk);
if(hot||!near||!alarm)$fatal(1,"distance alarm");
distance_cm=30;@(negedge clk);if(alarm)$fatal(1,"clear");
$display("PASS dual_sensor_alarm");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

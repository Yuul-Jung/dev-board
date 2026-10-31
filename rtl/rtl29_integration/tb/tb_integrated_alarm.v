`timescale 1ns/1ps
module tb_integrated_alarm;
reg clk=0,rst=1,valid_temp=0,valid_distance=0;reg signed [15:0]temp_x16=0;reg [15:0]distance_cm=0;wire alert,hot,near;
integrated_alarm dut(.clk(clk),.rst(rst),.valid_temp(valid_temp),.valid_distance(valid_distance),.temp_x16(temp_x16),.distance_cm(distance_cm),.alert(alert),.hot(hot),.near(near));
initial begin repeat(2)@(negedge clk);rst=0;valid_temp=1;temp_x16=500;@(negedge clk);if(!alert||!hot)$fatal(1,"hot");
temp_x16=0;valid_distance=1;distance_cm=10;@(negedge clk);if(!near||!alert)$fatal(1,"near");
$display("PASS integrated_alarm");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

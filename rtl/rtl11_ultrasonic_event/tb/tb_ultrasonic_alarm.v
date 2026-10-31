`timescale 1ns/1ps
module tb_ultrasonic_alarm;
reg clk=0,rst=1;reg [15:0]distance_cm=0;reg distance_valid=0;wire alarm,update_pulse;
ultrasonic_alarm #(.LIMIT_CM(20)) dut(.clk(clk),.rst(rst),.distance_cm(distance_cm),.distance_valid(distance_valid),.alarm(alarm),.update_pulse(update_pulse));
initial begin repeat(2)@(negedge clk);rst=0;distance_cm=10;distance_valid=1;@(negedge clk);
if(!alarm||!update_pulse)$fatal(1,"near alarm");
distance_valid=0;@(negedge clk);if(update_pulse!==0)$fatal(1,"pulse width");
distance_cm=50;distance_valid=1;@(negedge clk);if(alarm)$fatal(1,"far alarm");
$display("PASS ultrasonic_alarm");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

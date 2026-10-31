`timescale 1ns/1ps
module tb_ultrasonic_alarm;
reg clk=0,rst=1,distance_valid=0;reg[15:0]distance_cm=0;
wire alarm,update_pulse;reg expected_alarm=0;integer i;
ultrasonic_alarm #(.LIMIT_CM(20))dut(.clk(clk),.rst(rst),.distance_cm(distance_cm),.distance_valid(distance_valid),.alarm(alarm),.update_pulse(update_pulse));
always #5 clk=~clk;
task step(input bit reset_next,input bit v,input [15:0]d);
reg pulse_expected;
begin
 @(negedge clk);rst=reset_next;distance_valid=v;distance_cm=d;
 if(reset_next)expected_alarm=0;else if(v)expected_alarm=(d<20);
 pulse_expected=!reset_next&&v;
 @(posedge clk);#1;
 if(alarm!==expected_alarm||update_pulse!==pulse_expected)
  $fatal(1,"ultrasonic expected alarm=%b pulse=%b got=%b/%b",expected_alarm,pulse_expected,alarm,update_pulse);
end endtask
initial begin
 step(1,0,0);step(0,0,5);
 step(0,1,19);step(0,0,100);step(0,1,20);
 step(0,1,0);step(0,1,65535);step(1,1,1);
 for(i=0;i<100;i=i+1)step(0,($random&1),$random);
 $display("PASS ultrasonic_alarm boundary/valid/reset");$finish;
end
endmodule

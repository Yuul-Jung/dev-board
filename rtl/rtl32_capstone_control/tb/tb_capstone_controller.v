`timescale 1ns/1ps
module tb_capstone_controller;
reg clk=0,rst=1,valid=0;reg [15:0]distance_cm=0;reg signed [15:0]temp_x16=0;wire motor_enable,led_alarm,log_event;
capstone_controller dut(.clk(clk),.rst(rst),.valid(valid),.distance_cm(distance_cm),.temp_x16(temp_x16),.motor_enable(motor_enable),.led_alarm(led_alarm),.log_event(log_event));
initial begin repeat(2)@(negedge clk);rst=0;valid=1;distance_cm=50;temp_x16=100;@(negedge clk);
if(!motor_enable||led_alarm||!log_event)$fatal(1,"safe");
distance_cm=10;@(negedge clk);if(motor_enable||!led_alarm)$fatal(1,"unsafe");
$display("PASS capstone_controller");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

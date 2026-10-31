`timescale 1ns/1ps
module tb_distance_motor_guard;
reg clk=0,rst=1,valid=0;reg [15:0]distance_cm=0;wire motor_allowed;
distance_motor_guard dut(.clk(clk),.rst(rst),.valid(valid),.distance_cm(distance_cm),.motor_allowed(motor_allowed));
initial begin repeat(2)@(negedge clk);rst=0;valid=1;distance_cm=30;@(negedge clk);if(!motor_allowed)$fatal(1,"far");
distance_cm=10;@(negedge clk);if(motor_allowed)$fatal(1,"near");
$display("PASS distance_motor_guard");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

`timescale 1ns/1ps
module tb_motor_dir_controller;
reg clk=0,rst=1,enable=0,forward=0,reverse=0,pwm_in=1;wire in1,in2,pwm_gate;
motor_dir_controller dut(.clk(clk),.rst(rst),.enable(enable),.forward(forward),.reverse(reverse),.pwm_in(pwm_in),.in1(in1),.in2(in2),.pwm_gate(pwm_gate));
initial begin repeat(2)@(negedge clk);rst=0;enable=1;forward=1;@(negedge clk);if(!in1||in2||!pwm_gate)$fatal(1,"forward");
reverse=1;@(negedge clk);if(in1||in2)$fatal(1,"conflict");
forward=0;@(negedge clk);if(in1||!in2)$fatal(1,"reverse");
$display("PASS motor_dir_controller");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

`timescale 1ns/1ps
module tb_stepper_phase;
reg clk=0,rst=1,step=0,reverse=0;wire [3:0]coils;
stepper_phase dut(.clk(clk),.rst(rst),.step(step),.reverse(reverse),.coils(coils));
initial begin repeat(2)@(negedge clk);rst=0;if(coils!=4'b1000)$fatal(1,"reset phase");
step=1;@(negedge clk);if(coils!=4'b1100)$fatal(1,"step1");
reverse=1;@(negedge clk);if(coils!=4'b1000)$fatal(1,"reverse step");
$display("PASS stepper_phase");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

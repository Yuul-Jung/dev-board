`timescale 1ns/1ps
module tb_servo_target;
reg clk=0,rst=1,valid=0;reg [7:0]angle=0;wire [15:0]pulse_us;
servo_target dut(.clk(clk),.rst(rst),.angle(angle),.valid(valid),.pulse_us(pulse_us));
initial begin repeat(2)@(negedge clk);if(pulse_us!=1500)$fatal(1,"reset");
rst=0;valid=1;angle=0;@(negedge clk);if(pulse_us!=1000)$fatal(1,"zero");
angle=180;@(negedge clk);if(pulse_us!=2000)$fatal(1,"full");
$display("PASS servo_target");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

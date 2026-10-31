`timescale 1ns/1ps
module tb_bt_command_controller;
reg clk=0,rst=1,valid=0;reg [7:0]data=0;wire ready,led,servo_enable;
bt_command_controller dut(.clk(clk),.rst(rst),.data(data),.valid(valid),.ready(ready),.led(led),.servo_enable(servo_enable));
initial begin repeat(2)@(negedge clk);rst=0;valid=1;data=8'h31;@(negedge clk);if(!led)$fatal(1,"LED ON");
data=8'h53;@(negedge clk);if(!servo_enable)$fatal(1,"servo on");
data=8'h30;@(negedge clk);if(led)$fatal(1,"LED OFF");
$display("PASS bt_command_controller");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

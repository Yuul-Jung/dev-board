`timescale 1ns/1ps
module tb_distance_motor_guard;
reg clk=0,rst=1,valid=0;reg[15:0]distance_cm=0;
wire motor_allowed;reg expected=0;integer i;
distance_motor_guard #(.STOP_CM(20))dut(.clk(clk),.rst(rst),.valid(valid),.distance_cm(distance_cm),.motor_allowed(motor_allowed));
always #5 clk=~clk;
task step(input bit reset_next,input bit v,input[15:0]d);
begin
 @(negedge clk);rst=reset_next;valid=v;distance_cm=d;
 if(reset_next)expected=0;else if(v)expected=(d>=20);
 @(posedge clk);#1;
 if(motor_allowed!==expected)
  $fatal(1,"motor guard expected=%b got=%b at d=%0d valid=%b",expected,motor_allowed,d,v);
end endtask
initial begin
 step(1,0,0);step(0,0,65535);step(0,1,20);
 step(0,0,0);step(0,1,19);step(0,1,21);
 step(0,1,0);step(0,1,65535);step(1,1,65535);
 for(i=0;i<100;i=i+1)step(0,($random&1),$random);
 $display("PASS distance_motor_guard boundary/hold/reset");$finish;
end
endmodule

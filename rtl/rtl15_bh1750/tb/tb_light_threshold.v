`timescale 1ns/1ps
module tb_light_threshold;
reg clk=0,rst=1,valid=0;reg[15:0]lux=0;
wire dark,updated;reg expect_dark=0;integer i;
light_threshold #(.LIMIT(100)) dut(.clk(clk),.rst(rst),.lux(lux),.valid(valid),.dark(dark),.updated(updated));
always #5 clk=~clk;
task step(input bit reset_next,input bit v,input[15:0]d);
begin
 @(negedge clk);rst=reset_next;valid=v;lux=d;
 if(reset_next)expect_dark=0;else if(v)expect_dark=(d<100);
 @(posedge clk);#1;
 if(dark!==expect_dark||updated!==(!reset_next&&v))
  $fatal(1,"light_threshold expected=%b/%b got=%b/%b",expect_dark,(!reset_next&&v),dark,updated);
end endtask
initial begin
 step(1,0,0);step(0,0,99);step(0,1,99);
 step(0,0,100);step(0,1,100);step(0,1,101);
 step(0,1,0);step(0,1,65535);step(1,1,0);
 for(i=0;i<100;i=i+1)step(0,($random&1),$random);
 $display("PASS light_threshold boundary/hold/valid");$finish;
end
endmodule

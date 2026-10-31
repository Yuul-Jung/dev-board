`timescale 1ns/1ps
module tb_pir_event;
reg clk=0,rst=1,pir_in=0;wire motion,rose,fall;
pir_event dut(.clk(clk),.rst(rst),.pir_in(pir_in),.motion(motion),.rose(rose),.fall(fall));
initial begin repeat(3)@(negedge clk);rst=0;pir_in=1;repeat(5)@(negedge clk);
if(!motion) $fatal(1,"PIR high missed");
pir_in=0;repeat(5)@(negedge clk);if(motion) $fatal(1,"PIR low missed");
$display("PASS pir_event");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

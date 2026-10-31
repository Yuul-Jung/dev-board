`timescale 1ns/1ps
module tb_pir_event;
reg clk=0,rst=1,pir_in=0;
wire motion,rose,fall;
reg m=0,s=0,l=0,exp_motion=0,exp_rose=0,exp_fall=0;
integer i;
pir_event dut(.clk(clk),.rst(rst),.pir_in(pir_in),.motion(motion),.rose(rose),.fall(fall));
always #5 clk=~clk;
task step(input bit reset_next,input bit input_next);
reg nm,ns,nl;
begin
 @(negedge clk);rst=reset_next;pir_in=input_next;
 nm=reset_next?0:input_next;
 ns=reset_next?0:m;
 nl=reset_next?0:s;
 exp_motion=reset_next?0:s;
 exp_rose=reset_next?0:(s&~l);
 exp_fall=reset_next?0:(~s&l);
 @(posedge clk);#1;
 if({motion,rose,fall}!=={exp_motion,exp_rose,exp_fall})
   $fatal(1,"PIR expected=%b actual=%b",{exp_motion,exp_rose,exp_fall},{motion,rose,fall});
 m=nm;s=ns;l=nl;
end endtask
initial begin
 step(1,0);step(0,0);
 for(i=0;i<4;i=i+1)step(0,1);
 for(i=0;i<4;i=i+1)step(0,0);
 for(i=0;i<100;i=i+1)step(0,($random&1));
 step(1,1);
 for(i=0;i<4;i=i+1)step(0,1);
 $display("PASS pir_event reference pipeline 112+ samples");$finish;
end
endmodule

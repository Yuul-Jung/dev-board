`timescale 1ns/1ps
module tb_max7219_tx;
reg clk=0,rst=1,start=0;reg [15:0]frame=16'h0A0F;wire din,sclk,cs_n,busy,done;integer edges=0;reg prev=0;
max7219_tx #(.DIV(2)) dut(.clk(clk),.rst(rst),.start(start),.frame(frame),.din(din),.sclk(sclk),.cs_n(cs_n),.busy(busy),.done(done));
always @(negedge clk)begin if(sclk&&!prev)edges=edges+1;prev=sclk;end
initial begin repeat(3)@(negedge clk);rst=0;start=1;@(negedge clk);start=0;wait(done);#1;
if(edges!=16||!cs_n) $fatal(1,"MAX7219 edges %0d",edges);
$display("PASS max7219_tx");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

`timescale 1ns/1ps
module tb_max7219_tx;
reg clk=0,rst=1,start=0;reg [15:0]frame=16'h0A0F;wire din,sclk,cs_n,busy,done;
integer edges=0,cyc=0,last_edge=0,bad_gap=0,cs_bad=0;reg prev=0;reg [15:0] cap=0;
max7219_tx #(.DIV(2)) dut(.clk(clk),.rst(rst),.start(start),.frame(frame),.din(din),.sclk(sclk),.cs_n(cs_n),.busy(busy),.done(done));
// sclk 상승 때 din 표본(MSB 먼저), 간격 3*DIV=6 클럭, 전송 중(busy)에는 cs_n=0 이어야 함
always @(negedge clk)begin
 cyc=cyc+1;
 if(sclk&&!prev)begin edges=edges+1;cap={cap[14:0],din};if((edges%16)!=1&&cyc-last_edge!=6)bad_gap=bad_gap+1;last_edge=cyc;end
 if(sclk&&cs_n)cs_bad=cs_bad+1;
 prev=sclk;
end
initial begin repeat(3)@(negedge clk);rst=0;start=1;@(negedge clk);start=0;wait(done);#1;
if(edges!=16||!cs_n) $fatal(1,"MAX7219 edges %0d",edges);
if(cap!==16'h0A0F) $fatal(1,"MAX7219 data got %h want 0a0f",cap);
if(bad_gap!==0||cs_bad!==0) $fatal(1,"MAX7219 gap=%0d cs_bad=%0d",bad_gap,cs_bad);
repeat(3)@(negedge clk);frame=16'h9C35;start=1;@(negedge clk);start=0;wait(done);#1;
if(edges!=32||!cs_n||busy) $fatal(1,"MAX7219 2nd edges %0d",edges);
if(cap!==16'h9C35) $fatal(1,"MAX7219 2nd data got %h want 9c35",cap);
if(bad_gap!==0||cs_bad!==0) $fatal(1,"MAX7219 2nd gap=%0d cs_bad=%0d",bad_gap,cs_bad);
$display("PASS max7219_tx");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

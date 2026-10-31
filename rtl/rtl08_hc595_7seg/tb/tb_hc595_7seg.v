`timescale 1ns/1ps
module tb_hc595_7seg;
reg clk=0,rst=1,start=0;reg [7:0]segments=8'hA5;wire data,shift_clk,latch_clk,busy,done;
hc595_7seg #(.DIV(2)) dut(.clk(clk),.rst(rst),.start(start),.segments(segments),.data(data),.shift_clk(shift_clk),.latch_clk(latch_clk),.busy(busy),.done(done));
integer edge_count=0,latch_count=0,cyc=0,last_edge=0,bad_gap=0;reg prev_sclk=0,prev_latch=0;reg [7:0] cap=0;
// 시프트 클럭 상승 때 data 를 표본(MSB 먼저), 간격은 3*DIV=6 클럭, 래치는 8번째 에지 뒤에 shift_clk=0 일 때만
always @(negedge clk)begin
 cyc=cyc+1;
 if(shift_clk&&!prev_sclk)begin edge_count=edge_count+1;cap={cap[6:0],data};if((edge_count%8)!=1&&cyc-last_edge!=6)bad_gap=bad_gap+1;last_edge=cyc;end
 if(latch_clk&&!prev_latch)begin latch_count=latch_count+1;if(edge_count!=8*latch_count||shift_clk) $fatal(1,"HC595 latch edges=%0d sclk=%b",edge_count,shift_clk);end
 prev_sclk=shift_clk;prev_latch=latch_clk;
end
initial begin repeat(3)@(negedge clk);rst=0;start=1;@(negedge clk);start=0;
wait(done);#1;if(edge_count!=8||busy) $fatal(1,"HC595 bits=%0d",edge_count);
if(cap!==8'hA5) $fatal(1,"HC595 data got %h want a5",cap);
if(latch_count!==1) $fatal(1,"HC595 latch_count=%0d",latch_count);
if(bad_gap!==0) $fatal(1,"HC595 shift period wrong x%0d",bad_gap);
repeat(3)@(negedge clk);segments=8'h3C;start=1;@(negedge clk);start=0;
wait(done);#1;if(edge_count!=16||busy) $fatal(1,"HC595 2nd bits=%0d",edge_count);
if(cap!==8'h3C) $fatal(1,"HC595 2nd data got %h want 3c",cap);
if(latch_count!==2||bad_gap!==0) $fatal(1,"HC595 2nd latch=%0d gap=%0d",latch_count,bad_gap);
$display("PASS hc595_7seg");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

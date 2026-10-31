`timescale 1ns/1ps
module tb_hc595_7seg;
reg clk=0,rst=1,start=0;reg [7:0]segments=8'hA5;wire data,shift_clk,latch_clk,busy,done;
hc595_7seg #(.DIV(2)) dut(.clk(clk),.rst(rst),.start(start),.segments(segments),.data(data),.shift_clk(shift_clk),.latch_clk(latch_clk),.busy(busy),.done(done));
integer edge_count=0;reg prev_sclk=0;
always @(negedge clk)begin if(shift_clk&&!prev_sclk)edge_count=edge_count+1;prev_sclk=shift_clk;end
initial begin repeat(3)@(negedge clk);rst=0;start=1;@(negedge clk);start=0;
wait(done);#1;if(edge_count!=8||busy) $fatal(1,"HC595 bits=%0d",edge_count);
$display("PASS hc595_7seg");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

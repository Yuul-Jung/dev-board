`timescale 1ns/1ps
module tb_light_threshold;
reg clk=0,rst=1,valid=0;reg [15:0]lux=0;wire dark,updated;
light_threshold dut(.clk(clk),.rst(rst),.lux(lux),.valid(valid),.dark(dark),.updated(updated));
initial begin repeat(2)@(negedge clk);rst=0;valid=1;lux=50;@(negedge clk);if(!dark||!updated)$fatal(1,"dark");
valid=0;@(negedge clk);if(updated)$fatal(1,"pulse");
valid=1;lux=150;@(negedge clk);if(dark)$fatal(1,"bright");
$display("PASS light_threshold");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

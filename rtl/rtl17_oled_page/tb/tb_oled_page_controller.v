`timescale 1ns/1ps
module tb_oled_page_controller;
reg clk=0,rst=1,next_page=0;wire [1:0]page;wire redraw;
oled_page_controller dut(.clk(clk),.rst(rst),.next_page(next_page),.page(page),.redraw(redraw));
initial begin repeat(2)@(negedge clk);rst=0;next_page=1;@(negedge clk);
if(page!=1||!redraw)$fatal(1,"page1");
next_page=0;@(negedge clk);if(redraw)$fatal(1,"redraw");
next_page=1;repeat(3)@(negedge clk);if(page!=0)$fatal(1,"wrap");
$display("PASS oled_page_controller");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

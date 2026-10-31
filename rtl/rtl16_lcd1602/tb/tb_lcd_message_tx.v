`timescale 1ns/1ps
module tb_lcd_message_tx;
reg clk=0,rst=1,start=0,ready=1;wire [7:0]data;wire valid,rs,done;
lcd_message_tx dut(.clk(clk),.rst(rst),.start(start),.ready(ready),.data(data),.valid(valid),.rs(rs),.done(done));
initial begin repeat(2)@(negedge clk);rst=0;start=1;@(negedge clk);start=0;
if(!valid||data!=8'h4F)$fatal(1,"O");
@(negedge clk);if(data!=8'h4B)$fatal(1,"K");
@(negedge clk);if(data!=8'h20)$fatal(1,"space");
@(negedge clk);if(!done||valid)$fatal(1,"complete");
$display("PASS lcd_message_tx");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

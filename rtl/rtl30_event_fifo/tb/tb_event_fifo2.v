`timescale 1ns/1ps
module tb_event_fifo2;
reg clk=0,rst=1,in_valid=0,out_ready=0;reg [7:0]in_data=0;wire in_ready,out_valid;wire [7:0]out_data;
event_fifo2 dut(.clk(clk),.rst(rst),.in_data(in_data),.in_valid(in_valid),.in_ready(in_ready),.out_data(out_data),.out_valid(out_valid),.out_ready(out_ready));
initial begin repeat(2)@(negedge clk);rst=0;in_valid=1;in_data=8'h11;@(negedge clk);
in_data=8'h22;@(negedge clk);if(in_ready||out_data!=8'h11)$fatal(1,"full");
in_valid=0;out_ready=1;@(negedge clk);if(out_data!=8'h22)$fatal(1,"pop1");
@(negedge clk);if(out_valid)$fatal(1,"empty");
$display("PASS event_fifo2");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

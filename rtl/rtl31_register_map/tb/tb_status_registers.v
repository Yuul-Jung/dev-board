`timescale 1ns/1ps
module tb_status_registers;
reg clk=0,rst=1,wr_en=0;reg [1:0]wr_addr=0,rd_addr=0;reg [31:0]wr_data=0;reg [7:0]status=8'hA5;wire [31:0]rd_data;
status_registers dut(.clk(clk),.rst(rst),.wr_en(wr_en),.wr_addr(wr_addr),.wr_data(wr_data),.rd_addr(rd_addr),.rd_data(rd_data),.status(status));
initial begin repeat(2)@(negedge clk);rst=0;wr_en=1;wr_addr=0;wr_data=32'hABCD1234;@(negedge clk);
if(rd_data!=32'hABCD1234)$fatal(1,"control");
rd_addr=1;#1;if(rd_data!=32'hA5)$fatal(1,"status");
$display("PASS status_registers");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

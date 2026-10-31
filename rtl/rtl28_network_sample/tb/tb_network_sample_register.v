`timescale 1ns/1ps
module tb_network_sample_register;
reg clk=0,rst=1,sample_valid=0,payload_ready=0;reg [15:0]sample=0;wire sample_ready,payload_valid;wire [15:0]payload;
network_sample_register dut(.clk(clk),.rst(rst),.sample(sample),.sample_valid(sample_valid),.sample_ready(sample_ready),.payload(payload),.payload_valid(payload_valid),.payload_ready(payload_ready));
initial begin repeat(2)@(negedge clk);rst=0;sample_valid=1;sample=16'h1234;@(negedge clk);
if(!payload_valid||payload!=16'h1234||sample_ready)$fatal(1,"hold");
sample=16'hABCD;payload_ready=1;@(negedge clk);if(payload!=16'hABCD)$fatal(1,"replace");
$display("PASS network_sample_register");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

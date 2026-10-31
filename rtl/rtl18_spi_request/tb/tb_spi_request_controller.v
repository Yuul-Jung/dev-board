`timescale 1ns/1ps
// Placeholder smoke clock. DUT 미인스턴스이므로 기능 시험으로 간주하지 않는다.
module tb_spi_request_controller;
reg clk=0;
always #5 clk=~clk;
initial begin repeat(20) @(posedge clk); $display("CLOCK SMOKE ONLY"); $finish; end
endmodule

`timescale 1ns/1ps
// 기본 Reset, Clock 구동 Smoke Test; 기능 테스트는 별도 강화.
module tb_dht22_limit_controller;
  reg clk=0;
  always #5 clk=~clk;
  initial begin
    repeat(20) @(posedge clk);
    $display("SMOKE clock %s", "dht22_limit_controller");
    $finish;
  end
endmodule

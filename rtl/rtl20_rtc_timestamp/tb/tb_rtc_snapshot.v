`timescale 1ns/1ps
module tb_rtc_snapshot;
reg clk=0,rst=1,rtc_valid=0;reg [7:0]hour=0,minute=0,second=0;wire [23:0]hms_bcd;wire updated;
rtc_snapshot dut(.clk(clk),.rst(rst),.rtc_valid(rtc_valid),.hour(hour),.minute(minute),.second(second),.hms_bcd(hms_bcd),.updated(updated));
initial begin repeat(2)@(negedge clk);rst=0;rtc_valid=1;hour=8'h12;minute=8'h34;second=8'h56;@(negedge clk);
if(hms_bcd!==24'h123456||!updated)$fatal(1,"capture");
rtc_valid=0;@(negedge clk);if(updated||hms_bcd!=24'h123456)$fatal(1,"hold");
$display("PASS rtc_snapshot");$finish;end
always #5 clk=~clk;
initial begin #1000000; $fatal(1,"test timeout"); end
endmodule

`timescale 1ns/1ps
module tb_imu_tilt_alert;
reg clk=0,rst=1,valid=0;reg signed [15:0]accel_x=0;wire alert;
imu_tilt_alert dut(.clk(clk),.rst(rst),.valid(valid),.accel_x(accel_x),.alert(alert));
initial begin repeat(2)@(negedge clk);rst=0;valid=1;accel_x=6000;@(negedge clk);if(!alert)$fatal(1,"positive tilt");
accel_x=-6000;@(negedge clk);if(!alert)$fatal(1,"negative tilt");
accel_x=0;@(negedge clk);if(alert)$fatal(1,"level");
$display("PASS imu_tilt_alert");$finish;end
always #5 clk=~clk;
initial begin #1000000;$fatal(1,"timeout");end
endmodule

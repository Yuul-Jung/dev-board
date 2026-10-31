`timescale 1ns/1ps
module tb_dual_sensor_alarm;
reg clk=0,rst=1,temp_valid=0,distance_valid=0;
reg signed [15:0]temp_x16=0;reg[15:0]distance_cm=0;
wire alarm,temp_hot,dist_near;reg eh=0,en=0;integer i;
dual_sensor_alarm dut(.clk(clk),.rst(rst),.temp_x16(temp_x16),.temp_valid(temp_valid),.distance_cm(distance_cm),.distance_valid(distance_valid),.alarm(alarm),.temp_hot(temp_hot),.dist_near(dist_near));
always #5 clk=~clk;
task step(input bit reset_next,input bit tv,input signed [15:0]t,input bit dv,input[15:0]d);
begin
 @(negedge clk);rst=reset_next;temp_valid=tv;temp_x16=t;distance_valid=dv;distance_cm=d;
 if(reset_next)begin eh=0;en=0;end
 else begin if(tv)eh=(t>=16'sd480);if(dv)en=(d<16'd20);end
 @(posedge clk);#1;
 if({alarm,temp_hot,dist_near}!=={(eh|en),eh,en})
  $fatal(1,"dual_sensor expected alarm/hot/near=%b got=%b",{(eh|en),eh,en},{alarm,temp_hot,dist_near});
end endtask
initial begin
 step(1,0,0,0,0);step(0,1,-100,1,65535);
 step(0,1,479,0,0);step(0,1,480,1,20);
 step(0,0,0,1,19);step(0,1,481,1,21);
 step(0,1,-32768,0,0);step(0,0,0,1,20);
 for(i=0;i<100;i=i+1)step(0,($random&1),$random,($random&1),$random);
 step(1,1,999,1,1);
 $display("PASS dual_sensor_alarm boundaries and independent valids");$finish;
end
endmodule

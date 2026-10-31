// RTL21 IMU 기울기 경보: 임계 ±5000 의 양끝 경계(4999/5000/5001, -4999/-5000/-5001), 극값, valid 유지, Reset 을 TB 모델과 비교.
// 다른 LIMIT(0, 1, 32767) 인스턴스로 부호/경계 처리를 확인. 실제 IMU 값(g 환산)·센서 보정은 범위 밖(미검증).
`timescale 1ns/1ps
module tb_imu_tilt_alert;
reg clk=0,rst=1,valid=0;reg signed [15:0]accel_x=0;wire alert,a0,a1,a2;
imu_tilt_alert dut(.clk(clk),.rst(rst),.valid(valid),.accel_x(accel_x),.alert(alert));
imu_tilt_alert #(.LIMIT(0)) d0(.clk(clk),.rst(rst),.valid(valid),.accel_x(accel_x),.alert(a0));
imu_tilt_alert #(.LIMIT(1)) d1(.clk(clk),.rst(rst),.valid(valid),.accel_x(accel_x),.alert(a1));
imu_tilt_alert #(.LIMIT(32767)) d2(.clk(clk),.rst(rst),.valid(valid),.accel_x(accel_x),.alert(a2));
always #5 clk=~clk;
reg m,m0,m1,m2; integer seed=8,k,r,ai,n1,n0,nh;
function lim(input integer a, input integer L); begin lim=(a>L)||(a< -L); end endfunction
initial begin
 m=0;m0=0;m1=0;m2=0;n1=0;n0=0;nh=0;
 repeat(2)@(negedge clk);
 for(k=0;k<20000;k=k+1)begin
  @(negedge clk);
  if(alert!==m||a0!==m0||a1!==m1||a2!==m2) $fatal(1,"k=%0d alert=%b/%b a0=%b/%b a1=%b/%b a2=%b/%b",k,alert,m,a0,m0,a1,m1,a2,m2);
  if(m)n1=n1+1;else n0=n0+1;
  r=$random(seed);
  rst=(r[7:0]==8'd19);valid=r[8]|r[9];
  case(r[15:12]) 0:accel_x=4999;1:accel_x=5000;2:accel_x=5001;3:accel_x=-4999;4:accel_x=-5000;5:accel_x=-5001;6:accel_x=0;7:accel_x=1;8:accel_x=-1;9:accel_x=32767;10:accel_x=-32768;11:accel_x=-32767;default:accel_x=$random(seed);endcase
  ai=accel_x;
  if(rst)begin m=0;m0=0;m1=0;m2=0;end
  else if(valid)begin m=lim(ai,5000);m0=lim(ai,0);m1=lim(ai,1);m2=lim(ai,32767);end
  else nh=nh+1;
 end
 if(n1<2000||n0<2000||nh<2000)$fatal(1,"coverage alert=%0d clear=%0d hold=%0d",n1,n0,nh);
 $display("PASS imu_tilt_alert model: +/- limit boundaries, extremes, hold, reset");$finish;end
initial begin #10_000_000; $fatal(1,"test timeout"); end
endmodule

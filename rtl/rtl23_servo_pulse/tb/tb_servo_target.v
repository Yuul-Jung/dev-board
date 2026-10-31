// RTL23 서보 목표 펄스폭: 각도 0..255 전수(180 초과는 180 으로 제한), 기대값 1000+각도*1000/180(정수 나눗셈),
// valid 가 없으면 값 유지, Reset 은 1500us. 실제 PWM 코어/서보 구동은 이 모듈 밖(미검증).
`timescale 1ns/1ps
module tb_servo_target;
reg clk=0,rst=1,valid=0;reg [7:0]angle=0;wire [15:0]pulse_us;
servo_target dut(.clk(clk),.rst(rst),.angle(angle),.valid(valid),.pulse_us(pulse_us));
always #5 clk=~clk;
integer a,la,exp_us,hold_us,seed=12,k,r;
initial begin
 repeat(2)@(negedge clk);if(pulse_us!==16'd1500)$fatal(1,"reset value %0d",pulse_us);
 rst=0;
 for(a=0;a<256;a=a+1)begin
  valid=1;angle=a;@(negedge clk);
  la=(a>180)?180:a;exp_us=1000+(la*1000)/180;
  if(pulse_us!==exp_us) $fatal(1,"angle %0d expected %0d got %0d",a,exp_us,pulse_us);
  hold_us=exp_us;
  valid=0;angle=(a*7+13)&255;@(negedge clk);
  if(pulse_us!==hold_us) $fatal(1,"valid=0 changed pulse: angle %0d got %0d expected %0d",angle,pulse_us,hold_us);
 end
 // Reset 중간 삽입
 valid=1;angle=90;@(negedge clk);rst=1;@(negedge clk);rst=0;
 if(pulse_us!==16'd1500)$fatal(1,"reset in the middle: %0d",pulse_us);
 // 단조 증가(0..180)
 hold_us=0;
 for(a=0;a<=180;a=a+1)begin valid=1;angle=a;@(negedge clk);if(pulse_us<hold_us||pulse_us<1000||pulse_us>2000)$fatal(1,"monotonic/range angle %0d pulse %0d",a,pulse_us);hold_us=pulse_us;end
 $display("PASS servo_target exhaustive 0..255 + hold + reset");$finish;end
initial begin #10_000_000; $fatal(1,"timeout"); end
endmodule

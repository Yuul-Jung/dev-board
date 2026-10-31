// RTL22 DC 모터 방향 제어: 임의 enable/forward/reverse/pwm_in/Reset 을 TB 모델과 비교(회귀/변형 검출용).
// in1/in2 는 enable 과 방향이 한 클럭 뒤 함께 반영, 둘 다 켜지면 정지, pwm_gate = pwm_in && 저장된 enable.
// L298N 실제 회로/전류/데드타임은 범위 밖(미검증). 이 모듈은 전환 시 데드타임을 두지 않는다.
`timescale 1ns/1ps
module tb_motor_dir_controller;
reg clk=0,rst=1,enable=0,forward=0,reverse=0,pwm_in=1;wire in1,in2,pwm_gate;
motor_dir_controller dut(.clk(clk),.rst(rst),.enable(enable),.forward(forward),.reverse(reverse),.pwm_in(pwm_in),.in1(in1),.in2(in2),.pwm_gate(pwm_gate));
always #5 clk=~clk;
reg m1,m2,men; integer seed=10,k,r,n_f,n_r,n_c,n_g;
initial begin
 m1=0;m2=0;men=0;n_f=0;n_r=0;n_c=0;n_g=0;
 repeat(2)@(negedge clk);
 for(k=0;k<30000;k=k+1)begin
  @(negedge clk);
  if(in1!==m1||in2!==m2) $fatal(1,"k=%0d in1=%b/%b in2=%b/%b",k,in1,m1,in2,m2);
  r=$random(seed);
  rst=(r[7:0]==8'd23);enable=r[8]|r[9];forward=r[10];reverse=r[11];pwm_in=r[12];
  #1;
  if(pwm_gate!==(pwm_in&&men)) $fatal(1,"k=%0d pwm_gate=%b expected %b (pwm_in=%b en_reg=%b)",k,pwm_gate,(pwm_in&&men),pwm_in,men);
  if(pwm_in&&men)n_g=n_g+1;
  if(rst)begin m1=0;m2=0;men=0;end
  else begin
   men=enable; m1=enable&&forward&&!reverse; m2=enable&&reverse&&!forward;
   if(m1)n_f=n_f+1; if(m2)n_r=n_r+1; if(enable&&forward&&reverse)n_c=n_c+1;
  end
 end
 if(n_f<1000||n_r<1000||n_c<500||n_g<1000)$fatal(1,"coverage fwd=%0d rev=%0d conflict=%0d gate=%0d",n_f,n_r,n_c,n_g);
 $display("PASS motor_dir_controller model: direction, conflict stop, pwm gate, reset");$finish;end
initial begin #10_000_000; $fatal(1,"test timeout"); end
endmodule

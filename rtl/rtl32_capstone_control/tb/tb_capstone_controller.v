// RTL32 캡스톤 제어: 거리 임계 20 cm(19/20/21), 온도 임계 480(479/480/481), 극값, valid 가 없을 때 유지, log_event 1클럭,
// led_alarm / motor_enable 을 TB 모델과 비교. 캡스톤은 기능 컨트롤러이며 장치 전체 통합은 아님(미검증).
// 관찰: led_alarm 과 motor_enable 은 항상 서로 반대 값(중복 출력) - 설계 의도인지 확인 필요.
`timescale 1ns/1ps
module tb_capstone_controller;
reg clk=0,rst=1,valid=0;reg [15:0]distance_cm=0;reg signed [15:0]temp_x16=0;wire motor_enable,led_alarm,log_event;
capstone_controller dut(.clk(clk),.rst(rst),.valid(valid),.distance_cm(distance_cm),.temp_x16(temp_x16),.motor_enable(motor_enable),.led_alarm(led_alarm),.log_event(log_event));
always #5 clk=~clk;
reg m_me,m_led,m_log; integer seed=22,k,r,ti,n_me,n_led,n_log,n_hold;
initial begin
 m_me=0;m_led=0;m_log=0;n_me=0;n_led=0;n_log=0;n_hold=0;
 repeat(2)@(negedge clk);
 for(k=0;k<30000;k=k+1)begin
  @(negedge clk);
  if(motor_enable!==m_me||led_alarm!==m_led||log_event!==m_log) $fatal(1,"k=%0d motor=%b/%b led=%b/%b log=%b/%b",k,motor_enable,m_me,led_alarm,m_led,log_event,m_log);
  if(m_me)n_me=n_me+1; if(m_led)n_led=n_led+1; if(m_log)n_log=n_log+1;
  r=$random(seed);
  rst=(r[7:0]==8'd43);valid=r[8]|r[9];
  case(r[15:12]) 0:temp_x16=479;1:temp_x16=480;2:temp_x16=481;3:temp_x16=0;4:temp_x16=-1;5:temp_x16=32767;6:temp_x16=-32768;default:temp_x16=$random(seed);endcase
  case(r[19:16]) 0:distance_cm=19;1:distance_cm=20;2:distance_cm=21;3:distance_cm=0;4:distance_cm=65535;5:distance_cm=1;default:distance_cm=$random(seed);endcase
  ti=temp_x16;
  if(rst)begin m_me=0;m_led=0;m_log=0;end
  else begin
   m_log=0;
   if(valid)begin m_led=(distance_cm<20)||(ti>480);m_me=(distance_cm>=20)&&(ti<=480);m_log=1;end
   else n_hold=n_hold+1;
  end
 end
 if(n_me<1000||n_led<1000||n_log<5000||n_hold<5000)$fatal(1,"coverage motor=%0d led=%0d log=%0d hold=%0d",n_me,n_led,n_log,n_hold);
 $display("PASS capstone_controller model: 20cm/480 boundaries, hold, log pulse, reset");$finish;end
initial begin #10_000_000; $fatal(1,"timeout"); end
endmodule

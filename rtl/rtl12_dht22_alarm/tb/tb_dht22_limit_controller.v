// RTL12 DHT22 한계 알람: 임의 입력을 TB 모델과 비교(회귀/변형 검출용 모델이며 사양 자체의 검증은 아님).
// 경계값 299/300/301, 799/800/801, 음수 온도, 최대·최소값, checksum 불량 시 알람 유지, Reset.
// 기본 파라미터(300,800)와 다른 파라미터(-50,60000) 두 인스턴스로 부호 있는 비교를 검사.
`timescale 1ns/1ps
module tb_dht22_limit_controller;
reg clk=0,rst=1,sample_valid=0,checksum_ok=1;reg signed [15:0]temp_x10=0;reg [15:0]hum_x10=0;wire alarm,sample_error,alarm2,err2;
dht22_limit_controller dut(.clk(clk),.rst(rst),.temp_x10(temp_x10),.hum_x10(hum_x10),.sample_valid(sample_valid),.checksum_ok(checksum_ok),.alarm(alarm),.sample_error(sample_error));
dht22_limit_controller #(.TEMP_LIMIT_X10(-50),.HUM_LIMIT_X10(60000)) dut2(.clk(clk),.rst(rst),.temp_x10(temp_x10),.hum_x10(hum_x10),.sample_valid(sample_valid),.checksum_ok(checksum_ok),.alarm(alarm2),.sample_error(err2));
always #5 clk=~clk;
integer seed=5,k,r,ti,n_hold,n_al1,n_al0,n_err; reg m_al,m_er,m_al2;
initial begin
 m_al=0;m_er=0;m_al2=0;n_hold=0;n_al1=0;n_al0=0;n_err=0;
 repeat(2)@(negedge clk);
 for(k=0;k<20000;k=k+1)begin
  @(negedge clk);
  if(alarm!==m_al||sample_error!==m_er||alarm2!==m_al2||err2!==m_er) $fatal(1,"k=%0d alarm=%b/%b err=%b/%b alarm2=%b/%b err2=%b",k,alarm,m_al,sample_error,m_er,alarm2,m_al2,err2);
  if(m_al)n_al1=n_al1+1;else n_al0=n_al0+1;
  r=$random(seed);
  rst=(r[7:0]==8'd9);sample_valid=r[8]|r[9];checksum_ok=(r[11:10]!=2'd0);
  case(r[15:12]) 0:temp_x10=299;1:temp_x10=300;2:temp_x10=301;3:temp_x10=-51;4:temp_x10=-50;5:temp_x10=-49;6:temp_x10=0;7:temp_x10=-1;8:temp_x10=32767;9:temp_x10=-32768;default:temp_x10=($random(seed)&32'h1FF)-200;endcase
  case(r[19:16]) 0:hum_x10=799;1:hum_x10=800;2:hum_x10=801;3:hum_x10=59999;4:hum_x10=60000;5:hum_x10=60001;6:hum_x10=0;7:hum_x10=65535;default:hum_x10=$random(seed)&32'h3FF;endcase
  ti=temp_x10;
  if(rst)begin m_al=0;m_er=0;m_al2=0;end
  else if(sample_valid)begin
   m_er=~checksum_ok; if(!checksum_ok)n_err=n_err+1;
   if(checksum_ok)begin m_al=(ti>=300)||(hum_x10>=800); m_al2=(ti>=-50)||(hum_x10>=60000); end
   else n_hold=n_hold+1;
  end
 end
 if(n_al1<1000||n_al0<1000||n_hold<500||n_err<500)$fatal(1,"coverage al1=%0d al0=%0d hold=%0d err=%0d",n_al1,n_al0,n_hold,n_err);
 $display("PASS dht22_limit_controller model: boundaries, signed temp, checksum hold, reset");$finish;end
initial begin #10_000_000; $fatal(1,"test timeout"); end
endmodule

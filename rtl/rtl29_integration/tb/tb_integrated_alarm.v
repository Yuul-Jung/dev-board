// RTL29 통합 경보: 온도 임계 480(=30.0 C x16) 경계 479/480/481, 거리 임계 20 경계 19/20/21, 극값, valid 별 독립 갱신,
// alert = hot || near 를 TB 모델과 비교. 실제 센서 연결/단위 변환은 범위 밖(미검증).
`timescale 1ns/1ps
module tb_integrated_alarm;
reg clk=0,rst=1,valid_temp=0,valid_distance=0;reg signed [15:0]temp_x16=0;reg [15:0]distance_cm=0;wire alert,hot,near;
integrated_alarm dut(.clk(clk),.rst(rst),.valid_temp(valid_temp),.valid_distance(valid_distance),.temp_x16(temp_x16),.distance_cm(distance_cm),.alert(alert),.hot(hot),.near(near));
always #5 clk=~clk;
reg m_hot,m_near; integer seed=18,k,r,ti,n_h,n_n,n_vt,n_vd;
initial begin
 m_hot=0;m_near=0;n_h=0;n_n=0;n_vt=0;n_vd=0;
 repeat(2)@(negedge clk);
 for(k=0;k<30000;k=k+1)begin
  @(negedge clk);
  if(hot!==m_hot||near!==m_near||alert!==(m_hot||m_near)) $fatal(1,"k=%0d hot=%b/%b near=%b/%b alert=%b",k,hot,m_hot,near,m_near,alert);
  if(m_hot)n_h=n_h+1; if(m_near)n_n=n_n+1;
  r=$random(seed);
  rst=(r[7:0]==8'd37);valid_temp=r[8]|r[9];valid_distance=r[10]|r[11];
  case(r[15:12]) 0:temp_x16=479;1:temp_x16=480;2:temp_x16=481;3:temp_x16=0;4:temp_x16=-1;5:temp_x16=32767;6:temp_x16=-32768;default:temp_x16=$random(seed);endcase
  case(r[19:16]) 0:distance_cm=19;1:distance_cm=20;2:distance_cm=21;3:distance_cm=0;4:distance_cm=65535;5:distance_cm=1;default:distance_cm=$random(seed);endcase
  ti=temp_x16;
  if(rst)begin m_hot=0;m_near=0;end
  else begin
   if(valid_temp)begin m_hot=(ti>480);n_vt=n_vt+1;end
   if(valid_distance)begin m_near=(distance_cm<20);n_vd=n_vd+1;end
  end
 end
 if(n_h<1000||n_n<1000||n_vt<5000||n_vd<5000)$fatal(1,"coverage hot=%0d near=%0d vt=%0d vd=%0d",n_h,n_n,n_vt,n_vd);
 $display("PASS integrated_alarm model: 480/20 boundaries, independent valid, alert OR, reset");$finish;end
initial begin #10_000_000; $fatal(1,"timeout"); end
endmodule

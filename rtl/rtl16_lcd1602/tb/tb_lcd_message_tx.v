// RTL16 LCD 메시지 전송: 임의 start/ready/Reset 을 TB 모델과 비교(회귀/변형 검출용; 사양 자체의 검증은 아님).
// 바이트열 "OK "(4F,4B,20), valid&&ready 핸드셰이크, busy 중 start 무시, ready 없이는 진행 안 함, 완료 시 done 1클럭.
// 실제 LCD1602 타이밍/4비트 인터페이스는 이 모듈 범위 밖(미검증).
`timescale 1ns/1ps
module tb_lcd_message_tx;
reg clk=0,rst=1,start=0,ready=1;wire [7:0]data;wire valid,rs,done;
lcd_message_tx dut(.clk(clk),.rst(rst),.start(start),.ready(ready),.data(data),.valid(valid),.rs(rs),.done(done));
always #5 clk=~clk;
reg [7:0] m_data; reg m_valid,m_rs,m_done,m_busy; integer m_idx,seed=3,k,r,n_done,n_sb,n_vnr,n_rnv;
initial begin
 m_data=0;m_valid=0;m_rs=1;m_done=0;m_busy=0;m_idx=0;n_done=0;n_sb=0;n_vnr=0;n_rnv=0;
 repeat(2)@(negedge clk);
 for(k=0;k<30000;k=k+1)begin
  @(negedge clk);
  if(data!==m_data||valid!==m_valid||rs!==m_rs||done!==m_done) $fatal(1,"k=%0d data=%h/%h valid=%b/%b rs=%b/%b done=%b/%b",k,data,m_data,valid,m_valid,rs,m_rs,done,m_done);
  r=$random(seed);
  rst=(r[7:0]==8'd11);start=r[8]&r[9];ready=r[10]|r[11];
  if(start&&m_busy)n_sb=n_sb+1; if(m_valid&&!ready)n_vnr=n_vnr+1; if(!m_valid&&ready)n_rnv=n_rnv+1;
  if(rst)begin m_data=0;m_valid=0;m_rs=1;m_done=0;m_busy=0;m_idx=0;end
  else begin
   m_done=0;
   if(!m_busy&&start)begin m_idx=0;m_data=8'h4F;m_valid=1;m_rs=1;m_busy=1;end
   else if(m_busy&&m_valid&&ready)begin
    if(m_idx==0)begin m_data=8'h4B;m_idx=1;end
    else if(m_idx==1)begin m_data=8'h20;m_idx=2;end
    else begin m_valid=0;m_busy=0;m_done=1;n_done=n_done+1;end
   end
  end
 end
 if(n_done<300||n_sb<300||n_vnr<300||n_rnv<300)$fatal(1,"coverage done=%0d start_busy=%0d valid_notready=%0d ready_novalid=%0d",n_done,n_sb,n_vnr,n_rnv);
 $display("PASS lcd_message_tx model: OK sequence, handshake, start ignore, reset");$finish;end
initial begin #10_000_000; $fatal(1,"test timeout"); end
endmodule

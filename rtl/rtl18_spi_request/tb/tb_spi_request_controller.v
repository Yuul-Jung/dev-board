// RTL18 SPI 요청 컨트롤러: 임의 start/ready/done_in/Reset 을 TB 모델과 비교(회귀/변형 검출용).
// 실제 SPI 마스터/PHY 는 없다(미검증). request 는 ready 로만 내려가고 done 은 요청 수락 뒤 done_in 에서만 발생.
`timescale 1ns/1ps
module tb_spi_request_controller;
reg clk=0,rst=1,start=0,ready=0,done_in=0;wire request,busy,done;
spi_request_controller dut(.clk(clk),.rst(rst),.start(start),.ready(ready),.done_in(done_in),.request(request),.busy(busy),.done(done));
always #5 clk=~clk;
reg m_req,m_busy,m_done,m_sent,a,b; integer seed=4,k,r,n_done,n_dnos,n_rdy,n_sb;
initial begin
 m_req=0;m_busy=0;m_done=0;m_sent=0;n_done=0;n_dnos=0;n_rdy=0;n_sb=0;
 repeat(2)@(negedge clk);
 for(k=0;k<30000;k=k+1)begin
  @(negedge clk);
  if(request!==m_req||busy!==m_busy||done!==m_done) $fatal(1,"k=%0d request=%b/%b busy=%b/%b done=%b/%b",k,request,m_req,busy,m_busy,done,m_done);
  r=$random(seed);
  rst=(r[7:0]==8'd13);start=r[8]&r[9];ready=r[10]|r[11];done_in=r[12]|r[13];
  if(done_in&&!m_sent)n_dnos=n_dnos+1; if(ready&&!m_req)n_rdy=n_rdy+1; if(start&&m_busy)n_sb=n_sb+1;
  if(rst)begin m_req=0;m_busy=0;m_done=0;m_sent=0;end
  else begin
   m_done=0;
   if(!m_busy&&start)begin m_req=1;m_busy=1;m_sent=0;end
   else if(m_busy)begin
    a=m_req&&ready; b=m_sent&&done_in;
    if(a)begin m_req=0;m_sent=1;end
    if(b)begin m_busy=0;m_done=1;m_sent=0;n_done=n_done+1;end
   end
  end
 end
 if(n_done<300||n_dnos<1000||n_rdy<1000||n_sb<300)$fatal(1,"coverage done=%0d done_in_early=%0d ready_norequest=%0d start_busy=%0d",n_done,n_dnos,n_rdy,n_sb);
 $display("PASS spi_request_controller model: request/ready/done_in ordering, start ignore, reset");$finish;end
initial begin #10_000_000; $fatal(1,"test timeout"); end
endmodule

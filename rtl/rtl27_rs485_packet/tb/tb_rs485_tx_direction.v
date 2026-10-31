// RTL27 RS-485 송신 방향 제어: 임의 start/uart_ready/uart_done/Reset 을 TB 모델과 비교(회귀/변형 검출용).
// de 는 송신 시작과 함께 올라가고 uart_done 이후에만 내려간다. 실제 트랜시버(DE/RE 지연, 종단 저항)와 UART 는 범위 밖(미검증).
`timescale 1ns/1ps
module tb_rs485_tx_direction;
reg clk=0,rst=1,start=0,uart_ready=0,uart_done=0;wire tx_valid,de,busy,done;
rs485_tx_direction dut(.clk(clk),.rst(rst),.start(start),.uart_ready(uart_ready),.uart_done(uart_done),.tx_valid(tx_valid),.de(de),.busy(busy),.done(done));
always #5 clk=~clk;
reg m_tv,m_de,m_busy,m_done,m_w,a,b; integer seed=16,k,r,n_done,n_sb,n_dn,n_rdy;
initial begin
 m_tv=0;m_de=0;m_busy=0;m_done=0;m_w=0;n_done=0;n_sb=0;n_dn=0;n_rdy=0;
 repeat(2)@(negedge clk);
 for(k=0;k<30000;k=k+1)begin
  @(negedge clk);
  if(tx_valid!==m_tv||de!==m_de||busy!==m_busy||done!==m_done) $fatal(1,"k=%0d tx_valid=%b/%b de=%b/%b busy=%b/%b done=%b/%b",k,tx_valid,m_tv,de,m_de,busy,m_busy,done,m_done);
  r=$random(seed);
  rst=(r[7:0]==8'd31);start=r[8]&r[9];uart_ready=r[10]|r[11];uart_done=r[12]|r[13];
  if(start&&m_busy)n_sb=n_sb+1; if(uart_done&&!m_w)n_dn=n_dn+1; if(uart_ready&&!m_tv)n_rdy=n_rdy+1;
  if(rst)begin m_tv=0;m_de=0;m_busy=0;m_done=0;m_w=0;end
  else begin
   m_done=0;
   if(!m_busy&&start)begin m_busy=1;m_de=1;m_tv=1;m_w=0;end
   else if(m_busy)begin
    a=m_tv&&uart_ready; b=m_w&&uart_done;
    if(a)begin m_tv=0;m_w=1;end
    if(b)begin m_de=0;m_busy=0;m_done=1;m_w=0;n_done=n_done+1;end
   end
  end
 end
 if(n_done<300||n_sb<300||n_dn<1000||n_rdy<1000)$fatal(1,"coverage done=%0d start_busy=%0d done_early=%0d ready_notx=%0d",n_done,n_sb,n_dn,n_rdy);
 $display("PASS rs485_tx_direction model: DE timing, handshake order, start ignore, reset");$finish;end
initial begin #10_000_000; $fatal(1,"timeout"); end
endmodule

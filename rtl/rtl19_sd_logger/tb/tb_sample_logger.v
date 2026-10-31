// RTL19 샘플 로거: 임의 sensor_valid/write_ready/Reset 을 TB 모델과 비교(회귀/변형 검출용).
// 확인: 캡처, 쓰기 완료(valid&&ready) 시 valid 해제, 쓰기 대기 중 새 샘플은 overflow(데이터 유지), 완료와 새 샘플 동시.
// 실제 SD 카드/파일시스템은 없다(미검증). overflow 는 Reset 으로만 해제된다.
`timescale 1ns/1ps
module tb_sample_logger;
reg clk=0,rst=1,sensor_valid=0,write_ready=0;reg [15:0]sensor_data=0;wire [15:0]write_data;wire write_valid,overflow;
sample_logger dut(.clk(clk),.rst(rst),.sensor_data(sensor_data),.sensor_valid(sensor_valid),.write_ready(write_ready),.write_data(write_data),.write_valid(write_valid),.overflow(overflow));
always #5 clk=~clk;
reg [15:0] m_wd; reg m_wv,m_ov,nwv; integer seed=6,k,r,n_ov,n_both,n_cap,n_done;
initial begin
 m_wd=0;m_wv=0;m_ov=0;n_ov=0;n_both=0;n_cap=0;n_done=0;
 repeat(2)@(negedge clk);
 for(k=0;k<30000;k=k+1)begin
  @(negedge clk);
  if(write_data!==m_wd||write_valid!==m_wv||overflow!==m_ov) $fatal(1,"k=%0d wd=%h/%h wv=%b/%b ov=%b/%b",k,write_data,m_wd,write_valid,m_wv,overflow,m_ov);
  r=$random(seed);
  rst=(r[7:0]==8'd17);sensor_valid=r[8]&r[9];write_ready=r[10]|r[11];sensor_data=$random(seed);
  if(rst)begin m_wd=0;m_wv=0;m_ov=0;end
  else begin
   nwv=m_wv;
   if(m_wv&&write_ready)begin nwv=0;n_done=n_done+1;end
   if(sensor_valid)begin
    if(m_wv&&!write_ready)begin m_ov=1;n_ov=n_ov+1;end
    else begin m_wd=sensor_data;nwv=1;n_cap=n_cap+1;if(m_wv&&write_ready)n_both=n_both+1;end
   end
   m_wv=nwv;
  end
 end
 if(n_ov<100||n_both<100||n_cap<1000||n_done<1000)$fatal(1,"coverage overflow=%0d both=%0d capture=%0d write_done=%0d",n_ov,n_both,n_cap,n_done);
 $display("PASS sample_logger model: capture, write done, overflow, simultaneous done+capture, reset");$finish;end
initial begin #10_000_000; $fatal(1,"test timeout"); end
endmodule

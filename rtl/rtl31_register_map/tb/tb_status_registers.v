// RTL31 레지스터 맵: 임의 쓰기(wr_addr 0..3, wr_en)·읽기 주소 0..3·status·Reset 을 TB 모델과 비교.
// 주소 0 만 쓰기 가능(control), 주소 1 은 status 하위 8비트(읽기 전용), 그 외 읽기는 0. 쓰기 주소 1~3 은 무시.
// 읽기는 조합논리이므로 입력 변경 1 ns 뒤 비교. 버스 프로토콜/상위 IP 는 범위 밖(미검증).
`timescale 1ns/1ps
module tb_status_registers;
reg clk=0,rst=1,wr_en=0;reg [1:0]wr_addr=0,rd_addr=0;reg [31:0]wr_data=0;reg [7:0]status=8'hA5;wire [31:0]rd_data;
status_registers dut(.clk(clk),.rst(rst),.wr_en(wr_en),.wr_addr(wr_addr),.wr_data(wr_data),.rd_addr(rd_addr),.rd_data(rd_data),.status(status));
always #5 clk=~clk;
reg [31:0] m_ctl,exp_rd; integer seed=20,k,r,n_w,n_ign,n_r[0:3];
initial begin
 m_ctl=0;n_w=0;n_ign=0;n_r[0]=0;n_r[1]=0;n_r[2]=0;n_r[3]=0;
 repeat(2)@(negedge clk);
 for(k=0;k<30000;k=k+1)begin
  @(negedge clk);
  r=$random(seed);
  rst=(r[7:0]==8'd41);wr_en=r[8]|r[9];wr_addr=r[11:10];rd_addr=r[13:12];status=r[23:16];wr_data=$random(seed);
  #1;
  exp_rd=(rd_addr==2'd0)?m_ctl:(rd_addr==2'd1)?{24'd0,status}:32'd0;
  if(rd_data!==exp_rd) $fatal(1,"k=%0d rd_addr=%0d rd_data=%h expected %h (control=%h status=%h)",k,rd_addr,rd_data,exp_rd,m_ctl,status);
  n_r[rd_addr]=n_r[rd_addr]+1;
  if(rst)m_ctl=0;
  else if(wr_en&&wr_addr==2'd0)begin m_ctl=wr_data;n_w=n_w+1;end
  else if(wr_en)n_ign=n_ign+1;
 end
 if(n_w<2000||n_ign<2000||n_r[0]<2000||n_r[1]<2000||n_r[2]<2000||n_r[3]<2000)$fatal(1,"coverage writes=%0d ignored=%0d reads=%0d/%0d/%0d/%0d",n_w,n_ign,n_r[0],n_r[1],n_r[2],n_r[3]);
 $display("PASS status_registers model: control write, status read, unmapped read 0, ignored writes, reset");$finish;end
initial begin #10_000_000; $fatal(1,"timeout"); end
endmodule

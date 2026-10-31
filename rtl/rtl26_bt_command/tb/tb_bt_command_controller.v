// RTL26 블루투스 명령 디코더: 임의 바이트(명령 '1','0','S','s' 와 이웃 바이트 '2','R','r','T','t',0x00,0xFF 등)와
// valid/Reset 을 TB 모델과 비교, ready 는 항상 1. 실제 UART 수신/HC-06 연결은 이 모듈 밖(미검증).
`timescale 1ns/1ps
module tb_bt_command_controller;
reg clk=0,rst=1,valid=0;reg [7:0]data=0;wire ready,led,servo_enable;
bt_command_controller dut(.clk(clk),.rst(rst),.data(data),.valid(valid),.ready(ready),.led(led),.servo_enable(servo_enable));
always #5 clk=~clk;
reg m_led,m_se; integer seed=14,k,r,n_on,n_off,n_son,n_soff,n_inv;
initial begin
 m_led=0;m_se=0;n_on=0;n_off=0;n_son=0;n_soff=0;n_inv=0;
 repeat(2)@(negedge clk);
 for(k=0;k<30000;k=k+1)begin
  @(negedge clk);
  if(led!==m_led||servo_enable!==m_se||ready!==1'b1) $fatal(1,"k=%0d led=%b/%b servo=%b/%b ready=%b",k,led,m_led,servo_enable,m_se,ready);
  r=$random(seed);
  rst=(r[7:0]==8'd29);valid=r[8]|r[9];
  case(r[15:12]) 0:data=8'h31;1:data=8'h30;2:data=8'h53;3:data=8'h73;4:data=8'h32;5:data=8'h52;6:data=8'h72;7:data=8'h54;8:data=8'h74;9:data=8'h00;10:data=8'hFF;11:data=8'h2F;12:data=8'h51;13:data=8'h71;default:data=$random(seed);endcase
  if(rst)begin m_led=0;m_se=0;end
  else if(valid)begin
   case(data) 8'h31:begin m_led=1;n_on=n_on+1;end 8'h30:begin m_led=0;n_off=n_off+1;end 8'h53:begin m_se=1;n_son=n_son+1;end 8'h73:begin m_se=0;n_soff=n_soff+1;end default:n_inv=n_inv+1;endcase
  end
 end
 if(n_on<300||n_off<300||n_son<300||n_soff<300||n_inv<3000)$fatal(1,"coverage on=%0d off=%0d son=%0d soff=%0d other=%0d",n_on,n_off,n_son,n_soff,n_inv);
 $display("PASS bt_command_controller model: 1/0/S/s commands, neighbor bytes ignored, valid gating, reset");$finish;end
initial begin #10_000_000; $fatal(1,"timeout"); end
endmodule

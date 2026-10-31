module i2c_scan_controller(input wire clk,rst,start,cmd_ready,reply_valid,reply_ack,output reg cmd_valid,output reg [6:0]address,output reg found,done,busy);
reg waiting;
always @(posedge clk)begin
 if(rst)begin address<=7'h08;cmd_valid<=0;found<=0;done<=0;busy<=0;waiting<=0;end
 else begin done<=0;
 if(!busy && start)begin address<=7'h08;cmd_valid<=1;found<=0;busy<=1;waiting<=0;end
 else if(busy)begin
 if(cmd_valid&&cmd_ready)begin cmd_valid<=0;waiting<=1;end
 if(waiting&&reply_valid)begin waiting<=0;// (변경) 하나라도 ACK를 받았다면 검색 완료까지 found 유지.
 found<=found | reply_ack;
 if(address==7'h77)begin busy<=0;done<=1;end
 else begin address<=address+1'b1;cmd_valid<=1;end
 end
 end
 end
end
endmodule

module event_fifo2(input wire clk,rst,input wire [7:0]in_data,input wire in_valid,output wire in_ready,output wire [7:0]out_data,output wire out_valid,input wire out_ready);
reg [7:0]m0,m1;reg [1:0]count;reg rd_ptr,wr_ptr;
assign in_ready=count<2 || (out_valid&&out_ready);
assign out_valid=count!=0;
assign out_data=rd_ptr?m1:m0;
wire push=in_valid&&in_ready;
wire pop=out_valid&&out_ready;
always @(posedge clk)begin
 if(rst)begin count<=0;rd_ptr<=0;wr_ptr<=0;m0<=0;m1<=0;end
 else begin
 if(push)begin if(wr_ptr)m1<=in_data;else m0<=in_data;wr_ptr<=~wr_ptr;end
 if(pop)rd_ptr<=~rd_ptr;
 case({push,pop})2'b10:count<=count+1'b1;2'b01:count<=count-1'b1;default:begin end endcase
 end
end
endmodule

module status_registers(input wire clk,rst,wr_en,input wire [1:0]wr_addr,input wire [31:0]wr_data,input wire [1:0]rd_addr,output reg [31:0]rd_data,input wire [7:0]status);
reg [31:0]control;
always @(posedge clk)begin
 if(rst)control<=0;
 else if(wr_en&&wr_addr==2'd0)control<=wr_data;
end
always @(*)begin
 case(rd_addr)
 0:rd_data=control;
 1:rd_data={24'd0,status};
 default:rd_data=32'd0;
 endcase
end
endmodule

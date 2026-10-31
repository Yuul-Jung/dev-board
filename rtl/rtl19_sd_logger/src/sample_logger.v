module sample_logger(input wire clk,rst,input wire [15:0]sensor_data,input wire sensor_valid,write_ready,output reg [15:0]write_data,output reg write_valid,overflow);
always @(posedge clk)begin
 if(rst)begin write_data<=0;write_valid<=0;overflow<=0;end
 else begin
 if(write_valid&&write_ready)write_valid<=0;
 if(sensor_valid)begin
 if(write_valid&&!write_ready)overflow<=1;
 else begin write_data<=sensor_data;write_valid<=1;end
 end
 end
end
endmodule

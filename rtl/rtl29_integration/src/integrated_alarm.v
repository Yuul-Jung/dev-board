module integrated_alarm(input wire clk,rst,valid_temp,valid_distance,input wire signed [15:0]temp_x16,input wire [15:0]distance_cm,output wire alert,output reg hot,near);
always @(posedge clk)begin
 if(rst)begin hot<=0;near<=0;end
 else begin if(valid_temp)hot<=temp_x16>16'sd480;if(valid_distance)near<=distance_cm<16'd20;end
end
assign alert=hot||near;
endmodule

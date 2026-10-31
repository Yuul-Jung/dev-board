module dual_sensor_alarm #(parameter integer TEMP_MAX_X16=480,DIST_MIN_CM=20)(input wire clk,rst,input wire signed [15:0]temp_x16,input wire temp_valid,input wire [15:0]distance_cm,input wire distance_valid,output wire alarm,output reg temp_hot,dist_near);
always @(posedge clk)begin
 if(rst)begin temp_hot<=0;dist_near<=0;end
 else begin if(temp_valid)temp_hot<=temp_x16>=TEMP_MAX_X16;if(distance_valid)dist_near<=distance_cm<DIST_MIN_CM;end
end
assign alarm=temp_hot|dist_near;
endmodule

module imu_tilt_alert #(parameter signed [15:0]LIMIT=5000)(input wire clk,rst,valid,input wire signed [15:0]accel_x,output reg alert);
always @(posedge clk)begin
 if(rst)alert<=0;
 else if(valid)alert <= (accel_x>LIMIT)||(accel_x< -LIMIT);
end
endmodule

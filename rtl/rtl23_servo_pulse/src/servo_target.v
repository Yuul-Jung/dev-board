module servo_target(input wire clk,rst,input wire [7:0]angle,valid,output reg [15:0]pulse_us);
reg [15:0] scaled;
always @(*) scaled=16'd1000+((angle>8'd180?8'd180:angle)*16'd1000)/16'd180;
always @(posedge clk)begin
 if(rst)pulse_us<=16'd1500;
 else if(valid)pulse_us<=scaled;
end
endmodule

module distance_motor_guard #(parameter integer STOP_CM=20)(input wire clk,rst,valid,input wire [15:0]distance_cm,output reg motor_allowed);
always @(posedge clk)begin
 if(rst)motor_allowed<=0;
 else if(valid)motor_allowed<=distance_cm>=STOP_CM;
end
endmodule

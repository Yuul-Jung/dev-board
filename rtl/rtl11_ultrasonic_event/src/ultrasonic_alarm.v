module ultrasonic_alarm #(parameter integer LIMIT_CM=20)(input wire clk,rst,input wire [15:0]distance_cm,input wire distance_valid,output reg alarm,output reg update_pulse);
always @(posedge clk)begin
 if(rst)begin alarm<=0;update_pulse<=0;end
 else begin update_pulse<=0;if(distance_valid)begin alarm<=distance_cm<LIMIT_CM;update_pulse<=1;end end
end
endmodule

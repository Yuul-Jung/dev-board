module capstone_controller(input wire clk,rst,valid,input wire [15:0]distance_cm,input wire signed [15:0]temp_x16,output reg motor_enable,output reg led_alarm,output reg log_event);
always @(posedge clk)begin
 if(rst)begin motor_enable<=0;led_alarm<=0;log_event<=0;end
 else begin log_event<=0;
 if(valid)begin
 led_alarm<=distance_cm<16'd20||temp_x16>16'sd480;
 motor_enable<=distance_cm>=16'd20&&temp_x16<=16'sd480;
 log_event<=1;
 end
 end
end
endmodule

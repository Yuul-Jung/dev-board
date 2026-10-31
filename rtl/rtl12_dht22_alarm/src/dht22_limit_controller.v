module dht22_limit_controller #(parameter integer TEMP_LIMIT_X10=300, HUM_LIMIT_X10=800)(input wire clk,rst,input wire signed [15:0]temp_x10,input wire [15:0]hum_x10,input wire sample_valid,checksum_ok,output reg alarm,output reg sample_error);
always @(posedge clk)begin
 if(rst)begin alarm<=0;sample_error<=0;end
 else if(sample_valid)begin sample_error<=~checksum_ok;if(checksum_ok)alarm<=temp_x10>=TEMP_LIMIT_X10 || hum_x10>=HUM_LIMIT_X10;end
end
endmodule

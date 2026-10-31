module rtc_snapshot(input wire clk,rst,rtc_valid,input wire [7:0]hour,minute,second,output reg [23:0]hms_bcd,output reg updated);
always @(posedge clk)begin
 if(rst)begin hms_bcd<=0;updated<=0;end
 else begin updated<=0;if(rtc_valid)begin hms_bcd<={hour,minute,second};updated<=1;end end
end
endmodule

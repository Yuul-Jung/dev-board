module lcd_message_tx(input wire clk,rst,start,ready,output reg [7:0]data,output reg valid,rs,done);
reg [1:0]index;reg busy;
always @(posedge clk)begin
 if(rst)begin data<=0;valid<=0;rs<=1;done<=0;busy<=0;index<=0;end
 else begin done<=0;
 if(!busy&&start)begin index<=0;data<=8'h4F;valid<=1;rs<=1;busy<=1;end
 else if(busy&&valid&&ready)begin
 case(index)
 0:begin data<=8'h4B;index<=1;end
 1:begin data<=8'h20;index<=2;end
 default:begin valid<=0;busy<=0;done<=1;end
 endcase
 end
 end
end
endmodule

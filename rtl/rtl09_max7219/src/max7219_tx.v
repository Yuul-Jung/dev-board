module max7219_tx #(parameter integer DIV=50)(input wire clk,rst,start,input wire [15:0] frame,output reg din,sclk,cs_n,busy,done);
reg [15:0] shift;reg [4:0] n;reg [31:0] timer;reg [1:0] state;
always @(posedge clk) begin
 if(rst)begin din<=0;sclk<=0;cs_n<=1;busy<=0;done<=0;shift<=0;n<=0;timer<=0;state<=0;end
 else begin done<=0;
 case(state)
 0:if(start)begin shift<=frame;n<=0;timer<=0;cs_n<=0;busy<=1;sclk<=0;state<=1;end
 1:if(timer==DIV-1)begin timer<=0;din<=shift[15];state<=2;end else timer<=timer+1;
 2:if(timer==DIV-1)begin timer<=0;sclk<=1;shift<={shift[14:0],1'b0};n<=n+1;state<=3;end else timer<=timer+1;
 3:if(timer==DIV-1)begin timer<=0;sclk<=0;if(n==16)state<=4;else state<=1;end else timer<=timer+1;
 4:begin cs_n<=1;busy<=0;done<=1;state<=0;end
 endcase
 end
end
endmodule

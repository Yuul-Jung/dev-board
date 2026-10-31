module hc595_7seg #(parameter integer DIV=50)(input wire clk,rst,start,input wire [7:0] segments,output reg data,shift_clk,latch_clk,busy,done);
reg [7:0] bits; reg [3:0] n; reg [31:0] delay_count; reg [1:0] state;
always @(posedge clk) begin
 if(rst) begin data<=0;shift_clk<=0;latch_clk<=0;busy<=0;done<=0;bits<=0;n<=0;delay_count<=0;state<=0;end
 else begin done<=0;
 case(state)
 0: if(start) begin bits<=segments;n<=0;busy<=1;latch_clk<=0;shift_clk<=0;delay_count<=0;state<=1;end
 1: if(delay_count==DIV-1) begin delay_count<=0;data<=bits[7];shift_clk<=0;state<=2;end else delay_count<=delay_count+1;
 2: if(delay_count==DIV-1) begin delay_count<=0;shift_clk<=1;bits<={bits[6:0],1'b0};n<=n+1;state<=3;end else delay_count<=delay_count+1;
 3: if(delay_count==DIV-1) begin delay_count<=0;shift_clk<=0;if(n==8)state<=4;else state<=1;end else delay_count<=delay_count+1;
 4: begin latch_clk<=1;state<=5;end
 5: begin latch_clk<=0;busy<=0;done<=1;state<=0;end
 endcase
 end
end
endmodule

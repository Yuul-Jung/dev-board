module light_threshold #(parameter integer LIMIT=100)(input wire clk,rst,input wire [15:0]lux,input wire valid,output reg dark,output reg updated);
always @(posedge clk)begin
 if(rst)begin dark<=0;updated<=0;end
 else begin updated<=0;if(valid)begin dark<=lux<LIMIT;updated<=1;end end
end
endmodule

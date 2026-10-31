module oled_page_controller(input wire clk,rst,next_page,output reg [1:0]page,output reg redraw);
always @(posedge clk)begin
 if(rst)begin page<=0;redraw<=1;end
 else begin redraw<=next_page;if(next_page)page<=page+1'b1;end
end
endmodule

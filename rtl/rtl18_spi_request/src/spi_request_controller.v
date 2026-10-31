module spi_request_controller(input wire clk,rst,start,ready,done_in,output reg request,busy,done);
reg sent;
always @(posedge clk)begin
 if(rst)begin request<=0;busy<=0;done<=0;sent<=0;end
 else begin done<=0;
 if(!busy&&start)begin request<=1;busy<=1;sent<=0;end
 else if(busy)begin
 if(request&&ready)begin request<=0;sent<=1;end
 if(sent&&done_in)begin busy<=0;done<=1;sent<=0;end
 end
 end
end
endmodule

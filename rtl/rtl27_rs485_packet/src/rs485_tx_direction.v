module rs485_tx_direction(input wire clk,rst,start,uart_ready,uart_done,output reg tx_valid,de,busy,done);
reg waiting_done;
always @(posedge clk)begin
 if(rst)begin tx_valid<=0;de<=0;busy<=0;done<=0;waiting_done<=0;end
 else begin done<=0;
 if(!busy&&start)begin busy<=1;de<=1;tx_valid<=1;waiting_done<=0;end
 else if(busy)begin
 if(tx_valid&&uart_ready)begin tx_valid<=0;waiting_done<=1;end
 if(waiting_done&&uart_done)begin de<=0;busy<=0;done<=1;waiting_done<=0;end
 end
 end
end
endmodule

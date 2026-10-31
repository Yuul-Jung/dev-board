module pir_event(input wire clk,rst,pir_in,output reg motion,rose,fall);
(* ASYNC_REG="TRUE" *) reg meta,sync;reg last;
always @(posedge clk)begin
 if(rst)begin meta<=0;sync<=0;motion<=0;last<=0;rose<=0;fall<=0;end
 else begin meta<=pir_in;sync<=meta;motion<=sync;last<=sync;rose<=sync&~last;fall<=~sync&last;end
end
endmodule

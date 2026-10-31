module stepper_phase(input wire clk,rst,step,reverse,output reg [3:0]coils);
reg [2:0]index;
always @(posedge clk)begin
 if(rst)index<=0;
 else if(step)index<=reverse?index-1'b1:index+1'b1;
end
always @(*) begin
 case(index)
 0:coils=4'b1000;1:coils=4'b1100;2:coils=4'b0100;3:coils=4'b0110;
 4:coils=4'b0010;5:coils=4'b0011;6:coils=4'b0001;default:coils=4'b1001;
 endcase
end
endmodule

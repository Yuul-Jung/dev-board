module motor_dir_controller(input wire clk,rst,enable,forward,reverse,output reg in1,in2,output wire pwm_gate,input wire pwm_in);
always @(posedge clk)begin
 if(rst)begin in1<=0;in2<=0;end
 else begin
 in1<=enable&&forward&&!reverse;
 in2<=enable&&reverse&&!forward;
 end
end
assign pwm_gate=pwm_in&&enable;
endmodule

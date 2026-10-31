// (변경) enable을 방향 제어와 동일한 클럭에서 저장해 PWM gate와 방향 핀의
// 제어 시점을 일치시킨다. 단, 외부 비동기 입력이면 별도 동기화 필수.
module motor_dir_controller(
    input wire clk,rst,enable,forward,reverse,
    output reg in1,in2,
    output wire pwm_gate,
    input wire pwm_in
);
reg enabled_reg;
always @(posedge clk)begin
 if(rst)begin in1<=0;in2<=0;enabled_reg<=0;end
 else begin
 enabled_reg<=enable;
 in1<=enable&&forward&&!reverse;
 in2<=enable&&reverse&&!forward;
 end
end
assign pwm_gate=pwm_in&&enabled_reg;
endmodule

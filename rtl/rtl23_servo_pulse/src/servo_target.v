// 0~180도 입력을 1000~2000us 목표 High 시간으로 변환.
// 32비트 중간 연산: 180*1000=180000은 16비트 범위를 초과한다.
module servo_target(
 input wire clk,rst,
 input wire [7:0]angle,
 input wire valid,
 output reg [15:0]pulse_us
);
 wire [7:0] limited_angle = angle>8'd180 ? 8'd180 : angle;
 wire [31:0] scaled = 32'd1000 +
                      ({24'd0,limited_angle} * 32'd1000) / 32'd180;
 always @(posedge clk) begin
  if(rst)pulse_us<=16'd1500;
  else if(valid)pulse_us<=scaled[15:0];
 end
endmodule

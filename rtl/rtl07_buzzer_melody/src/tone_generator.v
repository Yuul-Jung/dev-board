// tone_enable 동안 half_period_clks 클럭마다 buzzer 출력 반전.
// 출력 주파수: CLK_HZ / (2 * half_period_clks).
// note_change 시 카운터 및 파형을 초기화해 음 전환을 결정적으로 처리한다.
module tone_generator #(
    parameter integer COUNTER_WIDTH = 32
)(
    input wire clk,
    input wire rst,
    input wire tone_enable,
    input wire note_change,
    input wire [COUNTER_WIDTH-1:0] half_period_clks,
    output reg buzzer
);
    reg [COUNTER_WIDTH-1:0] count;
    always @(posedge clk) begin
        if (rst || !tone_enable || note_change || half_period_clks == 0) begin
            count <= 0;
            buzzer <= 1'b0;
        end else if (count == half_period_clks - 1'b1) begin
            count <= 0;
            buzzer <= ~buzzer;
        end else begin
            count <= count + 1'b1;
        end
    end
endmodule

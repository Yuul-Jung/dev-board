// 버튼 enable이 1인 동안 A4(440)→C5(523)→E5(659) 순환.
// 각 음의 길이는 NOTE_CLOCKS 클럭. note_change는 전환 시 1클럭 pulse.
// CLK_HZ/주파수/2를 반주기 카운트로 반올림 없이 정수화한다.
module melody_sequencer #(
    parameter integer CLK_HZ = 100_000_000,
    parameter integer NOTE_CLOCKS = 25_000_000
)(
    input wire clk,
    input wire rst,
    input wire enabled,
    output reg [31:0] half_period_clks,
    output reg note_change
);
    localparam integer CW = NOTE_CLOCKS > 1 ? $clog2(NOTE_CLOCKS) : 1;
    reg [CW-1:0] elapsed;
    reg [1:0] note_index;
    always @(*) begin
        case (note_index)
            2'd0: half_period_clks = CLK_HZ / (2*440);
            2'd1: half_period_clks = CLK_HZ / (2*523);
            default: half_period_clks = CLK_HZ / (2*659);
        endcase
    end
    always @(posedge clk) begin
        if (rst || !enabled) begin
            elapsed <= 0;
            note_index <= 0;
            note_change <= 1'b0;
        end else if (elapsed == NOTE_CLOCKS-1) begin
            elapsed <= 0;
            note_index <= (note_index == 2'd2) ? 2'd0 : note_index + 1'b1;
            note_change <= 1'b1;
        end else begin
            elapsed <= elapsed + 1'b1;
            note_change <= 1'b0;
        end
    end
endmodule

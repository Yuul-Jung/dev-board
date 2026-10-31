// 버튼 입력: 2FF 동기화 → 안정시간 필터 → 1클럭 press_pulse
// ACTIVE_LOW=1: 버튼 누름이 0. CLK_HZ 및 DEBOUNCE_MS는 보드에 맞게 설정.
module button_event #(
    parameter integer CLK_HZ = 100_000_000,
    parameter integer DEBOUNCE_MS = 20,
    parameter integer ACTIVE_LOW = 1
)(
    input  wire clk,
    input  wire rst,
    input  wire button_in,
    output reg  press_pulse
);
    localparam integer TICKS = CLK_HZ / 1000 * DEBOUNCE_MS;
    localparam integer CW = (TICKS > 1) ? $clog2(TICKS) : 1;

    reg sync_1, sync_2;
    reg stable;
    reg [CW-1:0] count;
    wire pressed = ACTIVE_LOW ? ~sync_2 : sync_2;

    // 첫 번째 FF는 비동기 입력을 받아들이며, 두 번째 FF는 동기화한다.
    (* ASYNC_REG = "TRUE" *) reg unused_attribute_placeholder;
    always @(posedge clk) begin
        if (rst) begin
            sync_1 <= ACTIVE_LOW ? 1'b1 : 1'b0;
            sync_2 <= ACTIVE_LOW ? 1'b1 : 1'b0;
            stable <= 1'b0;
            count <= {CW{1'b0}};
            press_pulse <= 1'b0;
        end else begin
            sync_1 <= button_in;
            sync_2 <= sync_1;
            press_pulse <= 1'b0;
            if (pressed == stable) begin
                count <= {CW{1'b0}};
            end else if (count == TICKS-1) begin
                count <= {CW{1'b0}};
                stable <= pressed;
                if (pressed) press_pulse <= 1'b1;
            end else begin
                count <= count + 1'b1;
            end
        end
    end
endmodule

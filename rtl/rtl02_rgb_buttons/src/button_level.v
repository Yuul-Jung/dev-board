// 비동기 버튼 → 2단 동기화 → 안정시간 카운터 → 눌림 레벨
// 출력 레벨은 reset 후 0; BUTTON_ACTIVE_LOW는 전기적 버튼 극성.
module button_level #(
    parameter integer CLK_HZ=100_000_000,
    parameter integer DEBOUNCE_MS=20,
    parameter integer BUTTON_ACTIVE_LOW=0
)(
    input wire clk,
    input wire rst,
    input wire button_in,
    output reg pressed
);
    localparam integer TICKS=(CLK_HZ/1000)*DEBOUNCE_MS;
    localparam integer CW=(TICKS>1)?$clog2(TICKS):1;
    (* ASYNC_REG = "TRUE" *) reg sync1;
    (* ASYNC_REG = "TRUE" *) reg sync2;
    reg [CW-1:0] cnt;
    wire value=BUTTON_ACTIVE_LOW ? ~sync2 : sync2;
    always @(posedge clk) begin
        if (rst) begin
            sync1 <= BUTTON_ACTIVE_LOW ? 1'b1 : 1'b0;
            sync2 <= BUTTON_ACTIVE_LOW ? 1'b1 : 1'b0;
            cnt <= {CW{1'b0}};
            pressed <= 1'b0;
        end else begin
            sync1 <= button_in;
            sync2 <= sync1;
            if (value==pressed) cnt <= {CW{1'b0}};
            else if (cnt==TICKS-1) begin
                cnt <= {CW{1'b0}};
                pressed <= value;
            end else cnt <= cnt+1'b1;
        end
    end
endmodule

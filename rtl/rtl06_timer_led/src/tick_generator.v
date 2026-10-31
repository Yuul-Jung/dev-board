// 지정된 CLOCKS_PER_TICK 클럭마다 정확히 1클럭 tick 출력.
// clk는 분주하지 않는다. 모든 하위 회로는 동일 clk 사용.
module tick_generator #(
    parameter integer CLOCKS_PER_TICK = 50_000_000
)(
    input wire clk,
    input wire rst,
    output reg tick
);
    localparam integer CW = (CLOCKS_PER_TICK > 1) ?
                            $clog2(CLOCKS_PER_TICK) : 1;
    reg [CW-1:0] count;
    always @(posedge clk) begin
        if (rst) begin
            count <= {CW{1'b0}};
            tick <= 1'b0;
        end else if (count == CLOCKS_PER_TICK-1) begin
            count <= {CW{1'b0}};
            tick <= 1'b1;
        end else begin
            count <= count + 1'b1;
            tick <= 1'b0;
        end
    end
endmodule

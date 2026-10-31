// 비동기 1비트 스위치를 clk로 동기화하는 2단 FF.
module switch_sync (
    input wire clk,
    input wire rst,
    input wire sw_in,
    output wire sw_out
);
    (* ASYNC_REG = "TRUE" *) reg meta;
    (* ASYNC_REG = "TRUE" *) reg sync;
    always @(posedge clk) begin
        if (rst) begin
            meta <= 1'b0;
            sync <= 1'b0;
        end else begin
            meta <= sw_in;
            sync <= meta;
        end
    end
    assign sw_out = sync;
endmodule

// Basys3 스위치 입력은 비동기: 비트마다 2FF 동기화.
// sw=0: 0%, sw=255: 100%, 나머지는 1..254/256 비율.
module switch_duty_controller (
    input wire clk,
    input wire rst,
    input wire [7:0] sw,
    output wire [8:0] duty
);
    (* ASYNC_REG = "TRUE" *) reg [7:0] sw_meta;
    (* ASYNC_REG = "TRUE" *) reg [7:0] sw_sync;
    always @(posedge clk) begin
        if (rst) begin
            sw_meta <= 8'd0;
            sw_sync <= 8'd0;
        end else begin
            sw_meta <= sw;
            sw_sync <= sw_meta;
        end
    end
    assign duty = (sw_sync == 8'hFF) ? 9'd256 : {1'b0,sw_sync};
endmodule

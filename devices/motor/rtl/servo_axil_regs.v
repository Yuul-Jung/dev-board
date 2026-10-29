// 서보 PWM용 AXI4-Lite 레지스터. 버스 로직만 있다.
// 0x00 CTRL    (RW) [0]=enable
// 0x04 WIDTH   (RW) [15:0] 펄스 폭 µs
// 0x08 PERIOD  (RW) [15:0] 주기 µs, 기본 20000
// 0x0C MIN     (RW) [15:0] 폭 하한 µs, 기본 1000
// 0x10 MAX     (RW) [15:0] 폭 상한 µs, 기본 2000
// 0x14 APPLIED (R)  [15:0] 제한이 적용된 폭
module servo_axil_regs (
    input  wire        clk,
    input  wire        rstn,
    input  wire [4:0]  awaddr,
    input  wire        awvalid,
    output wire        awready,
    input  wire [31:0] wdata,
    input  wire        wvalid,
    output wire        wready,
    output wire [1:0]  bresp,
    output reg         bvalid,
    input  wire        bready,
    input  wire [4:0]  araddr,
    input  wire        arvalid,
    output wire        arready,
    output reg  [31:0] rdata,
    output wire [1:0]  rresp,
    output reg         rvalid,
    input  wire        rready,
    output wire        enable,
    output wire [15:0] width_us,
    output wire [15:0] period_us,
    output wire [15:0] min_us,
    output wire [15:0] max_us,
    input  wire [15:0] applied_us
);
    reg        en_r;
    reg [15:0] w_r, p_r, lo_r, hi_r;
    assign enable = en_r; assign width_us = w_r; assign period_us = p_r;
    assign min_us = lo_r; assign max_us = hi_r;
    assign bresp = 2'b00; assign rresp = 2'b00;

    wire do_wr = awvalid && wvalid && !bvalid;
    assign awready = do_wr;
    assign wready  = do_wr;
    wire [2:0] widx = awaddr[4:2];

    always @(posedge clk) begin
        if (!rstn) begin
            bvalid <= 1'b0; en_r <= 1'b0; w_r <= 16'd1500; p_r <= 16'd20000;
            lo_r <= 16'd1000; hi_r <= 16'd2000;
        end else if (do_wr) begin
            bvalid <= 1'b1;
            case (widx)
                3'd0: en_r <= wdata[0];
                3'd1: w_r  <= wdata[15:0];
                3'd2: p_r  <= wdata[15:0];
                3'd3: lo_r <= wdata[15:0];
                3'd4: hi_r <= wdata[15:0];
                default: ;
            endcase
        end else if (bvalid && bready) begin
            bvalid <= 1'b0;
        end
    end

    assign arready = arvalid && !rvalid;
    wire [2:0] ridx = araddr[4:2];
    always @(posedge clk) begin
        if (!rstn) begin
            rvalid <= 1'b0; rdata <= 32'd0;
        end else if (arready) begin
            rvalid <= 1'b1;
            case (ridx)
                3'd0: rdata <= {31'd0, en_r};
                3'd1: rdata <= {16'd0, w_r};
                3'd2: rdata <= {16'd0, p_r};
                3'd3: rdata <= {16'd0, lo_r};
                3'd4: rdata <= {16'd0, hi_r};
                3'd5: rdata <= {16'd0, applied_us};
                default: rdata <= 32'd0;
            endcase
        end else if (rvalid && rready) begin
            rvalid <= 1'b0;
        end
    end
endmodule

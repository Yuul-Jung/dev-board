// HC-SR04 거리계용 AXI4-Lite 레지스터. 버스 로직만 있다.
// 0x00 CTRL    (RW) [0]=enable
// 0x04 STATUS  (R)  [0]=busy [1]=valid [2]=timeout(고정, 1을 쓰면 지움) [3]=done(고정, 1을 쓰면 지움)
// 0x08 DIST    (R)  [9:0]=거리 cm
// 0x0C BCD     (R)  [11:0]=BCD 3자리
// 0x10 IRQ_EN  (RW) [0]=측정 완료 [1]=타임아웃
module us_axil_regs (
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
    output wire        core_reset,   // 코어는 active-high 리셋
    output wire        core_enable,
    input  wire        busy,
    input  wire        valid,
    input  wire        timeout_pulse,
    input  wire [9:0]  dist_cm,
    input  wire [11:0] dist_bcd,
    output wire        irq
);
    reg enable_r, irq_en_done, irq_en_to;
    reg timeout_flag, done_flag, busy_d;

    assign core_reset  = ~rstn;
    assign core_enable = enable_r;
    assign bresp = 2'b00;
    assign rresp = 2'b00;

    wire do_wr = awvalid && wvalid && !bvalid;
    assign awready = do_wr;
    assign wready  = do_wr;
    wire [2:0] widx = awaddr[4:2];

    wire meas_end = busy_d & ~busy;

    always @(posedge clk) begin
        if (!rstn) begin
            bvalid <= 1'b0; enable_r <= 1'b0; irq_en_done <= 1'b0; irq_en_to <= 1'b0;
            timeout_flag <= 1'b0; done_flag <= 1'b0; busy_d <= 1'b0;
        end else begin
            busy_d <= busy;
            if (do_wr) begin
                bvalid <= 1'b1;
                if (widx == 3'd0) enable_r <= wdata[0];
                if (widx == 3'd4) begin irq_en_done <= wdata[0]; irq_en_to <= wdata[1]; end
            end else if (bvalid && bready) begin
                bvalid <= 1'b0;
            end
            // 설정이 우선, 지우기는 그다음
            if (timeout_pulse)             timeout_flag <= 1'b1;
            else if (do_wr && widx == 3'd1 && wdata[2]) timeout_flag <= 1'b0;
            if (meas_end && valid)         done_flag <= 1'b1;
            else if (do_wr && widx == 3'd1 && wdata[3]) done_flag <= 1'b0;
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
                3'd0:    rdata <= {31'd0, enable_r};
                3'd1:    rdata <= {28'd0, done_flag, timeout_flag, valid, busy};
                3'd2:    rdata <= {22'd0, dist_cm};
                3'd3:    rdata <= {20'd0, dist_bcd};
                3'd4:    rdata <= {30'd0, irq_en_to, irq_en_done};
                default: rdata <= 32'd0;
            endcase
        end else if (rvalid && rready) begin
            rvalid <= 1'b0;
        end
    end

    assign irq = (done_flag & irq_en_done) | (timeout_flag & irq_en_to);
endmodule

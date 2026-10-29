// 엔코더용 AXI4-Lite 레지스터.
// 0x00 COUNT  (R) 부호 있는 32비트 카운트 ; 아무 값이나 쓰면 0 으로
// 0x04 STATUS (R) [0] 버튼 눌림(현재) [1] press* [2] err* [3] step* [4] dir_up(마지막 방향)  (*: 1을 쓰면 지움)
// 0x08 CTRL   (RW) [0] invert [1] irq_en
module quad_axil_regs (
    input  wire        clk,
    input  wire        rstn,
    input  wire [3:0]  awaddr,
    input  wire        awvalid,
    output wire        awready,
    input  wire [31:0] wdata,
    input  wire        wvalid,
    output wire        wready,
    output wire [1:0]  bresp,
    output reg         bvalid,
    input  wire        bready,
    input  wire [3:0]  araddr,
    input  wire        arvalid,
    output wire        arready,
    output reg  [31:0] rdata,
    output wire [1:0]  rresp,
    output reg         rvalid,
    input  wire        rready,
    input  wire signed [31:0] count,
    input  wire        step_pulse,
    input  wire        dir_up,
    input  wire        err_pulse,
    input  wire        btn_level,
    input  wire        btn_press_pulse,
    output wire        clear,
    output wire        invert,
    output wire        irq
);
    assign bresp = 2'b00;
    assign rresp = 2'b00;
    reg ctl_inv, ctl_irq, st_press, st_err, st_step;

    wire do_wr = awvalid && wvalid && !bvalid;
    assign awready = do_wr;
    assign wready  = do_wr;
    wire [1:0] widx = awaddr[3:2];
    assign clear  = do_wr && (widx == 2'd0);
    assign invert = ctl_inv;

    always @(posedge clk) begin
        if (!rstn) begin
            bvalid <= 1'b0; ctl_inv <= 1'b0; ctl_irq <= 1'b0;
            st_press <= 1'b0; st_err <= 1'b0; st_step <= 1'b0;
        end else begin
            if (do_wr) begin
                bvalid <= 1'b1;
                if (widx == 2'd2) begin ctl_inv <= wdata[0]; ctl_irq <= wdata[1]; end
                if (widx == 2'd1) begin
                    if (wdata[1]) st_press <= 1'b0;
                    if (wdata[2]) st_err   <= 1'b0;
                    if (wdata[3]) st_step  <= 1'b0;
                end
            end else if (bvalid && bready) bvalid <= 1'b0;
            if (btn_press_pulse) st_press <= 1'b1;   // 새 이벤트가 지우기보다 우선
            if (err_pulse)       st_err   <= 1'b1;
            if (step_pulse)      st_step  <= 1'b1;
        end
    end

    assign arready = arvalid && !rvalid;
    wire [1:0] ridx = araddr[3:2];
    always @(posedge clk) begin
        if (!rstn) begin rvalid <= 1'b0; rdata <= 32'd0; end
        else if (arready) begin
            rvalid <= 1'b1;
            case (ridx)
                2'd0:    rdata <= count;
                2'd1:    rdata <= {27'd0, dir_up, st_step, st_err, st_press, btn_level};
                2'd2:    rdata <= {30'd0, ctl_irq, ctl_inv};
                default: rdata <= 32'd0;
            endcase
        end else if (rvalid && rready) rvalid <= 1'b0;
    end
    assign irq = ctl_irq & (st_press | st_step);
endmodule

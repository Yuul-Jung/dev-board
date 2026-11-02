// 엔코더용 AXI4-Lite 레지스터.
// 0x00 COUNT  (R) 부호 있는 32비트 카운트 ; 하위 바이트를 쓰면 0으로
// 0x04 STATUS (R/W1C) [0] 버튼 눌림 [1] press* [2] err* [3] step* [4] dir_up (*: 1로 지움)
// 0x08 CTRL   (RW) [0] invert [1] irq_en
module quad_axil_regs (
    input  wire        clk,
    input  wire        rstn,
    input  wire [3:0]  awaddr,
    input  wire        awvalid,
    output wire        awready,
    input  wire [31:0] wdata,
    // (추가) 레지스터 쓰기·W1C·COUNT clear에 바이트 스트로브 적용.
    input  wire [3:0]  wstrb,
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

    // (변경) 주소/데이터를 독립 수락하고 한 번만 커밋한다.
    reg [3:0]  awaddr_hold;
    reg        aw_pending;
    reg [31:0] wdata_hold;
    reg [3:0]  wstrb_hold;
    reg        w_pending;

    assign awready = rstn && !aw_pending && !bvalid;
    assign wready  = rstn && !w_pending  && !bvalid;
    wire take_aw = awvalid && awready;
    wire take_w  = wvalid  && wready;
    wire commit_wr = aw_pending && w_pending && !bvalid;
    wire [1:0] widx = awaddr_hold[3:2];
    wire write_low_byte = commit_wr && wstrb_hold[0];

    assign clear = write_low_byte && (widx == 2'd0);
    assign invert = ctl_inv;

    always @(posedge clk) begin
        if (!rstn) begin
            awaddr_hold <= 4'd0;
            aw_pending <= 1'b0;
            wdata_hold <= 32'd0;
            wstrb_hold <= 4'd0;
            w_pending <= 1'b0;
            bvalid <= 1'b0;
            ctl_inv <= 1'b0;
            ctl_irq <= 1'b0;
            st_press <= 1'b0;
            st_err <= 1'b0;
            st_step <= 1'b0;
        end else begin
            if (take_aw) begin
                awaddr_hold <= awaddr;
                aw_pending <= 1'b1;
            end
            if (take_w) begin
                wdata_hold <= wdata;
                wstrb_hold <= wstrb;
                w_pending <= 1'b1;
            end
            if (commit_wr) begin
                aw_pending <= 1'b0;
                w_pending <= 1'b0;
                bvalid <= 1'b1;
                if (write_low_byte && widx == 2'd2) begin
                    ctl_inv <= wdata_hold[0];
                    ctl_irq <= wdata_hold[1];
                end
            end else if (bvalid && bready) begin
                bvalid <= 1'b0;
            end

            // (변경) W1C는 하위 바이트에만 적용. 같은 클록의 새 이벤트가 우선.
            if (write_low_byte && widx == 2'd1) begin
                if (wdata_hold[1]) st_press <= 1'b0;
                if (wdata_hold[2]) st_err   <= 1'b0;
                if (wdata_hold[3]) st_step  <= 1'b0;
            end
            if (btn_press_pulse) st_press <= 1'b1;
            if (err_pulse)       st_err   <= 1'b1;
            if (step_pulse)      st_step  <= 1'b1;
        end
    end

    // (변경) 읽기 응답은 AR 핸드셰이크 뒤에만 발생한다.
    assign arready = rstn && !rvalid;
    wire take_ar = arvalid && arready;
    wire [1:0] ridx = araddr[3:2];
    always @(posedge clk) begin
        if (!rstn) begin
            rvalid <= 1'b0;
            rdata <= 32'd0;
        end else if (take_ar) begin
            rvalid <= 1'b1;
            case (ridx)
                2'd0:    rdata <= count;
                2'd1:    rdata <= {27'd0, dir_up, st_step, st_err, st_press, btn_level};
                2'd2:    rdata <= {30'd0, ctl_irq, ctl_inv};
                default: rdata <= 32'd0;
            endcase
        end else if (rvalid && rready) begin
            rvalid <= 1'b0;
        end
    end
    assign irq = ctl_irq & (st_press | st_step);
endmodule

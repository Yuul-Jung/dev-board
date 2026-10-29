// PS/2 수신용 AXI4-Lite 레지스터 + 16바이트 FIFO.
// 0x00 DATA   (R) [7:0] 바이트, [8] valid. 읽으면 pop. 비어 있으면 valid=0, 데이터 0
// 0x04 STATUS (R) [0] empty [1] full [2] overflow* [3] parity_err* [4] frame_err* [5] timeout*  (*: 1을 쓰면 지움)
// 0x08 CTRL   (RW) [0] irq_en ; [1] 쓰면 FIFO 비움(읽으면 0)
// 0x0C COUNT  (R) [4:0] FIFO 안의 바이트 수
module ps2_axil_regs (
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
    input  wire        byte_valid,
    input  wire [7:0]  byte_in,
    input  wire        err_parity,
    input  wire        err_frame,
    input  wire        err_timeout,
    output wire        irq
);
    assign bresp = 2'b00;
    assign rresp = 2'b00;

    reg [7:0] mem [0:15];
    reg [4:0] wp, rp;
    wire [4:0] count = wp - rp;
    wire empty = (count == 5'd0);
    wire full  = (count == 5'd16);

    reg irq_en, st_ovf, st_par, st_frm, st_to;

    wire do_wr = awvalid && wvalid && !bvalid;
    assign awready = do_wr;
    assign wready  = do_wr;
    wire [1:0] widx = awaddr[3:2];
    wire flush = do_wr && (widx == 2'd2) && wdata[1];

    assign arready = arvalid && !rvalid;
    wire [1:0] ridx = araddr[3:2];
    wire pop = arready && (ridx == 2'd0) && !empty;

    always @(posedge clk) begin
        if (!rstn) begin
            wp <= 5'd0; rp <= 5'd0; irq_en <= 1'b0;
            st_ovf <= 1'b0; st_par <= 1'b0; st_frm <= 1'b0; st_to <= 1'b0;
            bvalid <= 1'b0;
        end else begin
            if (do_wr) begin
                bvalid <= 1'b1;
                if (widx == 2'd2) irq_en <= wdata[0];
            end else if (bvalid && bready) bvalid <= 1'b0;

            if (flush) begin wp <= 5'd0; rp <= 5'd0; end
            else begin
                if (byte_valid && !full) begin mem[wp[3:0]] <= byte_in; wp <= wp + 5'd1; end
                if (pop) rp <= rp + 5'd1;
            end

            // 오류 비트: 쓰기로 지우되 새 이벤트가 우선
            if (do_wr && widx == 2'd1) begin
                if (wdata[2]) st_ovf <= 1'b0;
                if (wdata[3]) st_par <= 1'b0;
                if (wdata[4]) st_frm <= 1'b0;
                if (wdata[5]) st_to  <= 1'b0;
            end
            if (byte_valid && full && !flush) st_ovf <= 1'b1;
            if (err_parity)  st_par <= 1'b1;
            if (err_frame)   st_frm <= 1'b1;
            if (err_timeout) st_to  <= 1'b1;
        end
    end

    always @(posedge clk) begin
        if (!rstn) begin
            rvalid <= 1'b0; rdata <= 32'd0;
        end else if (arready) begin
            rvalid <= 1'b1;
            case (ridx)
                2'd0:    rdata <= empty ? 32'd0 : {23'd0, 1'b1, mem[rp[3:0]]};
                2'd1:    rdata <= {26'd0, st_to, st_frm, st_par, st_ovf, full, empty};
                2'd2:    rdata <= {31'd0, irq_en};
                default: rdata <= {27'd0, count};
            endcase
        end else if (rvalid && rready) rvalid <= 1'b0;
    end

    assign irq = irq_en & ~empty;
endmodule

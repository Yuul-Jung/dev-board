// AXI4-Lite 슬레이브 레지스터. 로직만 있고 버스 핀은 갖지 않는다.
// 0x00 CMD    (W)  [1:0]=op(0 리셋,1 쓰기,2 읽기) [2]=쓰기 비트. 쓰면 바로 시작
// 0x04 STATUS (R)  [0]=busy [1]=done(새 명령 때 지워짐) [2]=presence [3]=rbit
// 0x08 IRQ_EN (RW) [0]=done 인터럽트 허용
module onewire_axil_regs (
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
    output wire        cmd_start,
    output wire [1:0]  cmd_op,
    output wire        cmd_wbit,
    input  wire        busy,
    input  wire        done_pulse,
    input  wire        presence,
    input  wire        rbit,
    output wire        irq
);
    reg done_flag;
    reg irq_en;

    wire do_wr = awvalid && wvalid && !bvalid;
    assign awready = do_wr;
    assign wready  = do_wr;
    assign bresp   = 2'b00;
    assign rresp   = 2'b00;

    wire wr_cmd = do_wr && (awaddr[3:2] == 2'd0);
    assign cmd_start = wr_cmd && !busy;
    assign cmd_op    = wdata[1:0];
    assign cmd_wbit  = wdata[2];

    always @(posedge clk) begin
        if (!rstn) begin
            bvalid <= 1'b0; done_flag <= 1'b0; irq_en <= 1'b0;
        end else begin
            if (do_wr) begin
                bvalid <= 1'b1;
                if (awaddr[3:2] == 2'd2) irq_en <= wdata[0];
            end else if (bvalid && bready) begin
                bvalid <= 1'b0;
            end
            if (cmd_start)       done_flag <= 1'b0;
            else if (done_pulse) done_flag <= 1'b1;
        end
    end

    assign arready = arvalid && !rvalid;

    always @(posedge clk) begin
        if (!rstn) begin
            rvalid <= 1'b0; rdata <= 32'd0;
        end else if (arready) begin
            rvalid <= 1'b1;
            case (araddr[3:2])
                2'd1:    rdata <= {28'd0, rbit, presence, done_flag, busy};
                2'd2:    rdata <= {31'd0, irq_en};
                default: rdata <= 32'd0;
            endcase
        end else if (rvalid && rready) begin
            rvalid <= 1'b0;
        end
    end

    assign irq = done_flag & irq_en;
endmodule

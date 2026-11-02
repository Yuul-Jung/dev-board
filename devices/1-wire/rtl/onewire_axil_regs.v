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
    // (추가) AXI4-Lite 바이트 스트로브: 하위 8비트 CMD/IRQ_EN에 WSTRB[0] 적용.
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

    // (변경) AW/W는 AXI4-Lite 독립 채널이므로 순서와 관계없이 별도 수락.
    // 한 쌍이 완성될 때 한 번만 레지스터를 갱신하고 B 응답을 보낸다.
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
    wire write_low_byte = commit_wr && wstrb_hold[0];
    wire [1:0] widx = awaddr_hold[3:2];
    assign bresp = 2'b00;
    assign rresp = 2'b00;

    // (변경) 버퍼에 저장된 주소/데이터를 사용하여 정확히 한 클록 시작.
    // WSTRB=0 또는 상위 바이트 쓰기는 명령 시작·IRQ 변경이 없어야 한다.
    wire wr_cmd = write_low_byte && (widx == 2'd0);
    assign cmd_start = wr_cmd && !busy;
    assign cmd_op    = wdata_hold[1:0];
    assign cmd_wbit  = wdata_hold[2];

    always @(posedge clk) begin
        if (!rstn) begin
            awaddr_hold <= 4'd0;
            aw_pending <= 1'b0;
            wdata_hold <= 32'd0;
            wstrb_hold <= 4'd0;
            w_pending <= 1'b0;
            bvalid <= 1'b0;
            done_flag <= 1'b0;
            irq_en <= 1'b0;
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
                if (write_low_byte && widx == 2'd2)
                    irq_en <= wdata_hold[0];
            end else if (bvalid && bready) begin
                bvalid <= 1'b0;
            end
            if (cmd_start)       done_flag <= 1'b0;
            else if (done_pulse) done_flag <= 1'b1;
        end
    end

    assign arready = rstn && !rvalid;

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

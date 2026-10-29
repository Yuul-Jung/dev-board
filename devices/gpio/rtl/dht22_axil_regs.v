// DHT22용 AXI4-Lite 레지스터. 버스 로직만 있다.
// 0x00 CMD    (W)  [0]=1이면 읽기 시작(busy 중이면 무시)
// 0x04 STATUS (R)  [0]=busy [1]=done(새 명령 때 지워짐) [2]=err [3]=csum_ok
// 0x08 RH     (R)  [15:0] 습도 원시값
// 0x0C TEMP   (R)  [15:0] 온도 원시값
// 0x10 CHK    (R)  [7:0] 체크섬 바이트
// 0x14 IRQ_EN (RW) [0]=종료 인터럽트
module dht22_axil_regs (
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
    output wire        cmd_start,
    input  wire        busy,
    input  wire        done_pulse,
    input  wire        err,
    input  wire        csum_ok,
    input  wire [15:0] rh_raw,
    input  wire [15:0] temp_raw,
    input  wire [7:0]  chk_raw,
    output wire        irq
);
    reg done_flag, irq_en;
    assign bresp = 2'b00;
    assign rresp = 2'b00;

    wire do_wr = awvalid && wvalid && !bvalid;
    assign awready = do_wr;
    assign wready  = do_wr;
    wire [2:0] widx = awaddr[4:2];

    assign cmd_start = do_wr && (widx == 3'd0) && wdata[0] && !busy;

    always @(posedge clk) begin
        if (!rstn) begin
            bvalid <= 1'b0; done_flag <= 1'b0; irq_en <= 1'b0;
        end else begin
            if (do_wr) begin
                bvalid <= 1'b1;
                if (widx == 3'd5) irq_en <= wdata[0];
            end else if (bvalid && bready) begin
                bvalid <= 1'b0;
            end
            if (cmd_start)       done_flag <= 1'b0;
            else if (done_pulse) done_flag <= 1'b1;
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
                3'd1:    rdata <= {28'd0, csum_ok, err, done_flag, busy};
                3'd2:    rdata <= {16'd0, rh_raw};
                3'd3:    rdata <= {16'd0, temp_raw};
                3'd4:    rdata <= {24'd0, chk_raw};
                3'd5:    rdata <= {31'd0, irq_en};
                default: rdata <= 32'd0;
            endcase
        end else if (rvalid && rready) begin
            rvalid <= 1'b0;
        end
    end

    assign irq = done_flag & irq_en;
endmodule

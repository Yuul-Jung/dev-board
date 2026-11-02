// HC-SR04 거리계용 AXI4-Lite 레지스터. 버스 로직만 있다.
// 0x00 CTRL    (RW) [0]=enable
// 0x04 STATUS  (R/W1C) [0]=busy [1]=valid [2]=timeout [3]=done
// 0x08 DIST    (R) [9:0]=거리 cm
// 0x0C BCD     (R) [11:0]=BCD 3자리
// 0x10 IRQ_EN  (RW) [0]=측정 완료 [1]=타임아웃
module us_axil_regs (
    input  wire        clk,
    input  wire        rstn,
    input  wire [4:0]  awaddr,
    input  wire        awvalid,
    output wire        awready,
    input  wire [31:0] wdata,
    // (추가) WSTRB[0]이 레지스터의 하위 8비트 쓰기를 허용한다.
    input  wire [3:0]  wstrb,
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
    assign bresp = 2'b00; // OKAY
    assign rresp = 2'b00; // OKAY

    // (변경) AW/W 채널을 독립적으로 수락하는 1단 버퍼.
    // AXI4-Lite에서는 주소와 데이터가 같은 클록에 도착할 필요가 없다.
    reg [4:0]  awaddr_hold;
    reg        aw_pending;
    reg [31:0] wdata_hold;
    reg [3:0]  wstrb_hold;
    reg        w_pending;

    assign awready = rstn && !aw_pending && !bvalid;
    assign wready  = rstn && !w_pending  && !bvalid;

    wire take_aw = awvalid && awready;
    wire take_w  = wvalid  && wready;

    // 양쪽 채널을 수락한 다음 클록에서 한 번만 쓰고 응답을 생성한다.
    wire commit_wr = aw_pending && w_pending && !bvalid;
    wire [2:0] widx = awaddr_hold[4:2];
    wire write_low_byte = commit_wr && wstrb_hold[0];
    wire meas_end = busy_d && !busy;

    always @(posedge clk) begin
        if (!rstn) begin
            awaddr_hold <= 5'd0;
            aw_pending <= 1'b0;
            wdata_hold <= 32'd0;
            wstrb_hold <= 4'd0;
            w_pending <= 1'b0;
            bvalid <= 1'b0;
            enable_r <= 1'b0;
            irq_en_done <= 1'b0;
            irq_en_to <= 1'b0;
            timeout_flag <= 1'b0;
            done_flag <= 1'b0;
            busy_d <= 1'b0;
        end else begin
            busy_d <= busy;

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
                // (변경) 쓰기 스트로브가 0이면 레지스터 부작용이 없다.
                if (write_low_byte) begin
                    if (widx == 3'd0)
                        enable_r <= wdata_hold[0];
                    if (widx == 3'd4) begin
                        irq_en_done <= wdata_hold[0];
                        irq_en_to   <= wdata_hold[1];
                    end
                end
            end else if (bvalid && bready) begin
                bvalid <= 1'b0;
            end

            // 이벤트 설정 우선, W1C는 하위 바이트 스트로브가 있을 때만 허용.
            if (timeout_pulse)
                timeout_flag <= 1'b1;
            else if (write_low_byte && widx == 3'd1 && wdata_hold[2])
                timeout_flag <= 1'b0;

            if (meas_end && valid)
                done_flag <= 1'b1;
            else if (write_low_byte && widx == 3'd1 && wdata_hold[3])
                done_flag <= 1'b0;
        end
    end

    // 읽기 채널은 쓰기 응답 대기 중에도 독립적으로 동작한다.
    assign arready = rstn && !rvalid;
    wire take_ar = arvalid && arready;
    wire [2:0] ridx = araddr[4:2];

    always @(posedge clk) begin
        if (!rstn) begin
            rvalid <= 1'b0;
            rdata <= 32'd0;
        end else if (take_ar) begin
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

    assign irq = (done_flag && irq_en_done) || (timeout_flag && irq_en_to);
endmodule

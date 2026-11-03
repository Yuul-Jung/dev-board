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
    // (추가) AXI4-Lite 32비트 데이터의 바이트별 유효 스트로브.
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
    output wire        enable,
    output wire [15:0] width_us,
    output wire [15:0] period_us,
    output wire [15:0] min_us,
    output wire [15:0] max_us,
    input  wire [15:0] applied_us
);
    reg        en_r;
    reg [15:0] w_r, p_r, lo_r, hi_r;
    assign enable = en_r;
    assign width_us = w_r;
    assign period_us = p_r;
    assign min_us = lo_r;
    assign max_us = hi_r;
    assign bresp = 2'b00; // OKAY
    assign rresp = 2'b00; // OKAY

    // (변경) AW 주소와 W 데이터·WSTRB를 독립적으로 수락하는 1단 버퍼.
    // 기존 awvalid && wvalid 동시 도착 제약을 제거한다.
    reg [4:0]  awaddr_hold;
    reg        aw_pending;
    reg [31:0] wdata_hold;
    reg [3:0]  wstrb_hold;
    reg        w_pending;

    assign awready = rstn && !aw_pending && !bvalid;
    assign wready  = rstn && !w_pending && !bvalid;
    wire take_aw = awvalid && awready;
    wire take_w  = wvalid && wready;
    wire commit_wr = aw_pending && w_pending && !bvalid;
    wire [2:0] widx = awaddr_hold[4:2];

    // (추가) 16비트 레지스터의 유효 하위 2바이트만 선택적으로 변경.
    // 예: WSTRB=4'b0010이면 비트 [15:8]만 새 데이터로 교체한다.
    function [15:0] merge_wstrb16;
        input [15:0] old_value;
        input [15:0] new_value;
        input [3:0]  strb;
        integer i;
        begin
            merge_wstrb16 = old_value;
            for (i = 0; i < 2; i = i + 1)
                if (strb[i])
                    merge_wstrb16[i*8 +: 8] = new_value[i*8 +: 8];
        end
    endfunction

    always @(posedge clk) begin
        if (!rstn) begin
            awaddr_hold <= 5'd0;
            aw_pending <= 1'b0;
            wdata_hold <= 32'd0;
            wstrb_hold <= 4'd0;
            w_pending <= 1'b0;
            bvalid <= 1'b0;
            en_r <= 1'b0;
            w_r <= 16'd1500;
            p_r <= 16'd20000;
            lo_r <= 16'd1000;
            hi_r <= 16'd2000;
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
                case (widx)
                    // (변경) CTRL은 실제 유효 비트가 있는 하위 바이트만 쓴다.
                    3'd0: if (wstrb_hold[0]) en_r <= wdata_hold[0];
                    // (변경) 16비트 WIDTH/PERIOD/MIN/MAX는 하위 2바이트 부분 쓰기.
                    3'd1: w_r  <= merge_wstrb16(w_r, wdata_hold[15:0], wstrb_hold);
                    3'd2: p_r  <= merge_wstrb16(p_r, wdata_hold[15:0], wstrb_hold);
                    3'd3: lo_r <= merge_wstrb16(lo_r, wdata_hold[15:0], wstrb_hold);
                    3'd4: hi_r <= merge_wstrb16(hi_r, wdata_hold[15:0], wstrb_hold);
                    default: ; // APPLIED는 읽기 전용. 쓰기는 상태 변경 없이 OKAY.
                endcase
            end else if (bvalid && bready) begin
                bvalid <= 1'b0;
            end
        end
    end

    // (변경) 읽기 응답은 ARVALID/ARREADY 핸드셰이크가 있어야 생성한다.
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

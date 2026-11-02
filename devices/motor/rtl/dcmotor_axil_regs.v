// DC 모터용 AXI4-Lite 레지스터. 버스 로직만 있다.
// 0x00 CTRL   (RW) [0]=enable [2:1]=mode(0 정지,1 정회전,2 역회전,3 브레이크)
// 0x04 DUTY   (RW) [31:0] 켜짐 클럭 수
// 0x08 PERIOD (RW) [31:0] PWM 주기 클럭 수
// 0x0C STATUS (R)  [1:0]=적용된 mode [2]=방향 전환 대기
module dcmotor_axil_regs #(
    parameter PERIOD_DEFAULT = 100000   // 100 MHz에서 1 kHz
) (
    input  wire        clk,
    input  wire        rstn,
    input  wire [3:0]  awaddr,
    input  wire        awvalid,
    output wire        awready,
    input  wire [31:0] wdata,
    // (추가) AXI4-Lite 바이트 쓰기 스트로브: 1인 바이트만 수정한다.
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
    output wire        enable,
    output wire [1:0]  mode,
    output wire [31:0] duty,
    output wire [31:0] period,
    input  wire [1:0]  applied_mode,
    input  wire        guard
);
    reg        enable_r;
    reg [1:0]  mode_r;
    reg [31:0] duty_r;
    reg [31:0] period_r;

    assign enable = enable_r;
    assign mode   = mode_r;
    assign duty   = duty_r;
    assign period = period_r;
    assign bresp  = 2'b00;
    assign rresp  = 2'b00;

    // (변경) AW 주소와 W 데이터/스트로브를 서로 다른 클록에 독립 수락한다.
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

    // (추가) WSTRB[3:0]을 32비트 레지스터의 바이트별 쓰기 마스크로 확장.
    // 예: WSTRB=4'b0101이면 비트 [7:0], [23:16]만 새 데이터로 교체한다.
    function [31:0] merge_wstrb;
        input [31:0] old_value;
        input [31:0] new_value;
        input [3:0]  strb;
        integer i;
        begin
            merge_wstrb = old_value;
            for (i = 0; i < 4; i = i + 1)
                if (strb[i])
                    merge_wstrb[i*8 +: 8] = new_value[i*8 +: 8];
        end
    endfunction

    always @(posedge clk) begin
        if (!rstn) begin
            awaddr_hold <= 4'd0;
            aw_pending <= 1'b0;
            wdata_hold <= 32'd0;
            wstrb_hold <= 4'd0;
            w_pending <= 1'b0;
            bvalid <= 1'b0;
            enable_r <= 1'b0;
            mode_r <= 2'd0;
            duty_r <= 32'd0;
            period_r <= PERIOD_DEFAULT;
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
                    // (변경) CTRL의 실제 유효 비트는 하위 바이트에만 있다.
                    2'd0: if (wstrb_hold[0]) begin
                        enable_r <= wdata_hold[0];
                        mode_r   <= wdata_hold[2:1];
                    end
                    // (변경) 32비트 DUTY/PERIOD의 부분 바이트 쓰기 지원.
                    2'd1: duty_r   <= merge_wstrb(duty_r, wdata_hold, wstrb_hold);
                    2'd2: period_r <= merge_wstrb(period_r, wdata_hold, wstrb_hold);
                    default: ; // STATUS는 읽기 전용. 쓰기는 상태 변경 없이 OKAY 응답.
                endcase
            end else if (bvalid && bready) begin
                bvalid <= 1'b0;
            end
        end
    end

    // (변경) AR 핸드셰이크가 성립한 경우에만 읽기 응답을 생성한다.
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
                2'd0:    rdata <= {29'd0, mode_r, enable_r};
                2'd1:    rdata <= duty_r;
                2'd2:    rdata <= period_r;
                default: rdata <= {29'd0, guard, applied_mode};
            endcase
        end else if (rvalid && rready) begin
            rvalid <= 1'b0;
        end
    end
endmodule

// DC 모터용 AXI4-Lite 레지스터. 버스 로직만 있다.
// 0x00 CTRL   (RW) [0]=enable [2:1]=mode(0 정지,1 정회전,2 역회전,3 브레이크)
// 0x04 DUTY   (RW) [31:0] 켜짐 클럭 수 (예전에는 [15:0] 이었음)
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

    wire do_wr = awvalid && wvalid && !bvalid;
    assign awready = do_wr;
    assign wready  = do_wr;
    wire [1:0] widx = awaddr[3:2];

    always @(posedge clk) begin
        if (!rstn) begin
            bvalid <= 1'b0; enable_r <= 1'b0; mode_r <= 2'd0; duty_r <= 32'd0;
            period_r <= PERIOD_DEFAULT;
        end else begin
            if (do_wr) begin
                bvalid <= 1'b1;
                case (widx)
                    2'd0: begin enable_r <= wdata[0]; mode_r <= wdata[2:1]; end
                    2'd1: duty_r   <= wdata;
                    2'd2: period_r <= wdata;
                    default: ;
                endcase
            end else if (bvalid && bready) begin
                bvalid <= 1'b0;
            end
        end
    end

    assign arready = arvalid && !rvalid;
    wire [1:0] ridx = araddr[3:2];

    always @(posedge clk) begin
        if (!rstn) begin
            rvalid <= 1'b0; rdata <= 32'd0;
        end else if (arready) begin
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

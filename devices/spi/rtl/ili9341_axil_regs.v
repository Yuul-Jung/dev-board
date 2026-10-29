// ILI9341 엔진용 AXI4-Lite 레지스터. 버스 로직만 있다.
// 0x00 CTRL      (RW) [0]=RESX 레벨(1=해제) [1]=CSX 활성(1이면 CSX Low) [2]=백라이트
// 0x04 DIV       (RW) [15:0] SCL 반주기-1 (SCL = clk / (2*(DIV+1)))
// 0x08 TX        (W)  [7:0]=바이트 [8]=D/CX  쓰면 송신 시작. busy 중 쓰면 버리고 OVERRUN 기록
//      STATUS    (R)  [0]=busy [1]=OVERRUN(읽으면 지워짐)
// 0x0C FILLCOLOR (RW) [15:0]
// 0x10 FILLCNT   (W)  [19:0] 쓰면 fill 시작. busy 중 쓰면 버리고 OVERRUN 기록. 읽기는 마지막 쓴 값
module ili9341_axil_regs (
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
    output wire        resx,
    output wire        cs_active,
    output wire        backlight,
    output wire [15:0] div,
    output reg         tx_start,
    output wire [7:0]  tx_byte,
    output wire        tx_dc,
    output reg         fill_start,
    output wire [15:0] fill_color,
    output wire [19:0] fill_cnt,
    input  wire        busy
);
    reg [2:0]  ctrl_r;
    reg [15:0] div_r;
    reg [8:0]  tx_r;
    reg [15:0] color_r;
    reg [19:0] cnt_r;
    reg        overrun;

    assign resx = ctrl_r[0]; assign cs_active = ctrl_r[1]; assign backlight = ctrl_r[2];
    assign div = div_r; assign tx_byte = tx_r[7:0]; assign tx_dc = tx_r[8];
    assign fill_color = color_r; assign fill_cnt = cnt_r;
    assign bresp = 2'b00; assign rresp = 2'b00;

    wire do_wr = awvalid && wvalid && !bvalid;
    assign awready = do_wr;
    assign wready  = do_wr;
    wire [2:0] widx = awaddr[4:2];
    wire busy_eff = busy | tx_start | fill_start;   // 시작 펄스가 반영되기 전 1클럭도 busy로 본다
    wire do_rd = arvalid && !rvalid;
    assign arready = do_rd;
    wire [2:0] ridx = araddr[4:2];

    always @(posedge clk) begin
        if (!rstn) begin
            bvalid <= 1'b0; ctrl_r <= 3'd0; div_r <= 16'd49; tx_r <= 9'd0;
            color_r <= 16'd0; cnt_r <= 20'd0; overrun <= 1'b0;
            tx_start <= 1'b0; fill_start <= 1'b0;
        end else begin
            tx_start <= 1'b0; fill_start <= 1'b0;
            if (do_rd && ridx == 3'd2) overrun <= 1'b0;
            if (do_wr) begin
                bvalid <= 1'b1;
                case (widx)
                    3'd0: ctrl_r  <= wdata[2:0];
                    3'd1: div_r   <= wdata[15:0];
                    3'd2: if (busy_eff) overrun <= 1'b1;
                          else begin tx_r <= wdata[8:0]; tx_start <= 1'b1; end
                    3'd3: color_r <= wdata[15:0];
                    3'd4: if (busy_eff) overrun <= 1'b1;
                          else begin cnt_r <= wdata[19:0]; fill_start <= 1'b1; end
                    default: ;
                endcase
            end else if (bvalid && bready) begin
                bvalid <= 1'b0;
            end
        end
    end

    always @(posedge clk) begin
        if (!rstn) begin
            rvalid <= 1'b0; rdata <= 32'd0;
        end else if (do_rd) begin
            rvalid <= 1'b1;
            case (ridx)
                3'd0:    rdata <= {29'd0, ctrl_r};
                3'd1:    rdata <= {16'd0, div_r};
                3'd2:    rdata <= {30'd0, overrun, busy_eff};
                3'd3:    rdata <= {16'd0, color_r};
                3'd4:    rdata <= {12'd0, cnt_r};
                default: rdata <= 32'd0;
            endcase
        end else if (rvalid && rready) begin
            rvalid <= 1'b0;
        end
    end
endmodule

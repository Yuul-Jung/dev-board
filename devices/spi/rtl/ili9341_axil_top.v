// 배선만 한다. 로직은 하위 모듈에 있다.
module ili9341_axil_top (
    input  wire        clk,
    input  wire        rstn,
    input  wire [4:0]  s_axi_awaddr,
    input  wire        s_axi_awvalid,
    output wire        s_axi_awready,
    input  wire [31:0] s_axi_wdata,
    input  wire        s_axi_wvalid,
    output wire        s_axi_wready,
    output wire [1:0]  s_axi_bresp,
    output wire        s_axi_bvalid,
    input  wire        s_axi_bready,
    input  wire [4:0]  s_axi_araddr,
    input  wire        s_axi_arvalid,
    output wire        s_axi_arready,
    output wire [31:0] s_axi_rdata,
    output wire [1:0]  s_axi_rresp,
    output wire        s_axi_rvalid,
    input  wire        s_axi_rready,
    output wire        lcd_scl,
    output wire        lcd_sda,
    output wire        lcd_dcx,
    output wire        lcd_csx,
    output wire        lcd_resx,
    output wire        lcd_bl
);
    wire        resx, cs_active, backlight, tx_start, tx_dc, fill_start, busy;
    wire [15:0] div, fill_color;
    wire [7:0]  tx_byte;
    wire [19:0] fill_cnt;

    ili9341_axil_regs u_regs (
        .clk(clk), .rstn(rstn),
        .awaddr(s_axi_awaddr), .awvalid(s_axi_awvalid), .awready(s_axi_awready),
        .wdata(s_axi_wdata), .wvalid(s_axi_wvalid), .wready(s_axi_wready),
        .bresp(s_axi_bresp), .bvalid(s_axi_bvalid), .bready(s_axi_bready),
        .araddr(s_axi_araddr), .arvalid(s_axi_arvalid), .arready(s_axi_arready),
        .rdata(s_axi_rdata), .rresp(s_axi_rresp), .rvalid(s_axi_rvalid), .rready(s_axi_rready),
        .resx(resx), .cs_active(cs_active), .backlight(backlight), .div(div),
        .tx_start(tx_start), .tx_byte(tx_byte), .tx_dc(tx_dc),
        .fill_start(fill_start), .fill_color(fill_color), .fill_cnt(fill_cnt),
        .busy(busy)
    );

    ili9341_engine u_eng (
        .clk(clk), .rstn(rstn), .div(div),
        .tx_start(tx_start), .tx_byte(tx_byte), .tx_dc(tx_dc),
        .fill_start(fill_start), .fill_color(fill_color), .fill_cnt(fill_cnt),
        .busy(busy), .scl(lcd_scl), .sda(lcd_sda), .dcx(lcd_dcx)
    );

    assign lcd_csx  = ~cs_active;
    assign lcd_resx = resx;
    assign lcd_bl   = backlight;
endmodule

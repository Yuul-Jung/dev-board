// 배선만 한다. 로직은 하위 모듈에 있다. PS/2 두 선은 입력 전용(수신만).
module ps2_axil_top #(
    parameter CLK_MHZ = 100
) (
    input  wire        clk,
    input  wire        rstn,
    input  wire [3:0]  s_axi_awaddr,
    input  wire        s_axi_awvalid,
    output wire        s_axi_awready,
    input  wire [31:0] s_axi_wdata,
    input  wire        s_axi_wvalid,
    output wire        s_axi_wready,
    output wire [1:0]  s_axi_bresp,
    output wire        s_axi_bvalid,
    input  wire        s_axi_bready,
    input  wire [3:0]  s_axi_araddr,
    input  wire        s_axi_arvalid,
    output wire        s_axi_arready,
    output wire [31:0] s_axi_rdata,
    output wire [1:0]  s_axi_rresp,
    output wire        s_axi_rvalid,
    input  wire        s_axi_rready,
    input  wire        ps2_clk,
    input  wire        ps2_data,
    output wire        irq
);
    wire       byte_valid, err_parity, err_frame, err_timeout;
    wire [7:0] byte_out;

    ps2_rx_engine #(.CLK_MHZ(CLK_MHZ)) u_eng (
        .clk(clk), .rstn(rstn), .ps2c_i(ps2_clk), .ps2d_i(ps2_data),
        .byte_valid(byte_valid), .byte_out(byte_out),
        .err_parity(err_parity), .err_frame(err_frame), .err_timeout(err_timeout)
    );

    ps2_axil_regs u_regs (
        .clk(clk), .rstn(rstn),
        .awaddr(s_axi_awaddr), .awvalid(s_axi_awvalid), .awready(s_axi_awready),
        .wdata(s_axi_wdata), .wvalid(s_axi_wvalid), .wready(s_axi_wready),
        .bresp(s_axi_bresp), .bvalid(s_axi_bvalid), .bready(s_axi_bready),
        .araddr(s_axi_araddr), .arvalid(s_axi_arvalid), .arready(s_axi_arready),
        .rdata(s_axi_rdata), .rresp(s_axi_rresp), .rvalid(s_axi_rvalid), .rready(s_axi_rready),
        .byte_valid(byte_valid), .byte_in(byte_out),
        .err_parity(err_parity), .err_frame(err_frame), .err_timeout(err_timeout), .irq(irq)
    );
endmodule

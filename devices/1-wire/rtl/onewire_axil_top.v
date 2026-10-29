// 배선만 한다. 로직은 하위 모듈에 있다.
module onewire_axil_top #(
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
    inout  wire        ow_pad,
    output wire        irq
);
    wire        cmd_start, cmd_wbit, busy, done_pulse, presence, rbit, ow_low, ow_i;
    wire [1:0]  cmd_op;

    onewire_axil_regs u_regs (
        .clk(clk), .rstn(rstn),
        .awaddr(s_axi_awaddr), .awvalid(s_axi_awvalid), .awready(s_axi_awready),
        .wdata(s_axi_wdata), .wvalid(s_axi_wvalid), .wready(s_axi_wready),
        .bresp(s_axi_bresp), .bvalid(s_axi_bvalid), .bready(s_axi_bready),
        .araddr(s_axi_araddr), .arvalid(s_axi_arvalid), .arready(s_axi_arready),
        .rdata(s_axi_rdata), .rresp(s_axi_rresp), .rvalid(s_axi_rvalid), .rready(s_axi_rready),
        .cmd_start(cmd_start), .cmd_op(cmd_op), .cmd_wbit(cmd_wbit),
        .busy(busy), .done_pulse(done_pulse), .presence(presence), .rbit(rbit),
        .irq(irq)
    );

    onewire_bit_engine #(.CLK_MHZ(CLK_MHZ)) u_eng (
        .clk(clk), .rstn(rstn),
        .start(cmd_start), .op(cmd_op), .wbit(cmd_wbit), .ow_i(ow_i),
        .ow_low(ow_low), .busy(busy), .done_pulse(done_pulse),
        .presence(presence), .rbit(rbit)
    );

    onewire_iobuf u_io (.low(ow_low), .pad(ow_pad), .i(ow_i));
endmodule

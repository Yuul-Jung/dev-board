// 배선만 한다. 로직은 하위 모듈에 있다.
module us_axil_top #(
    parameter CLK_FREQ_HZ = 100_000_000
) (
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
    input  wire        echo,       // 레벨 변환 후 입력
    output wire        trig,
    output wire        irq
);
    wire        core_reset, core_enable, busy, valid, timeout_pulse;
    wire [9:0]  dist_cm;
    wire [11:0] dist_bcd;

    us_axil_regs u_regs (
        .clk(clk), .rstn(rstn),
        .awaddr(s_axi_awaddr), .awvalid(s_axi_awvalid), .awready(s_axi_awready),
        .wdata(s_axi_wdata), .wvalid(s_axi_wvalid), .wready(s_axi_wready),
        .bresp(s_axi_bresp), .bvalid(s_axi_bvalid), .bready(s_axi_bready),
        .araddr(s_axi_araddr), .arvalid(s_axi_arvalid), .arready(s_axi_arready),
        .rdata(s_axi_rdata), .rresp(s_axi_rresp), .rvalid(s_axi_rvalid), .rready(s_axi_rready),
        .core_reset(core_reset), .core_enable(core_enable),
        .busy(busy), .valid(valid), .timeout_pulse(timeout_pulse),
        .dist_cm(dist_cm), .dist_bcd(dist_bcd), .irq(irq)
    );

    ultrasonic_distance_meter #(.CLK_FREQ_HZ(CLK_FREQ_HZ)) u_core (
        .clk(clk), .reset(core_reset), .enable(core_enable),
        .echo(echo), .trig(trig),
        .distance_cm(dist_cm), .distance_bcd(dist_bcd),
        .distance_valid(valid), .busy(busy), .timeout_error(timeout_pulse)
    );
endmodule

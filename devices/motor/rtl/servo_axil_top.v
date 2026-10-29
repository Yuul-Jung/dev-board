// 배선만 한다. 로직은 하위 모듈에 있다.
module servo_axil_top #(
    parameter CLK_MHZ = 100
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
    output wire        servo_pwm
);
    wire        enable;
    wire [15:0] width_us, period_us, min_us, max_us, applied_us;

    servo_axil_regs u_regs (
        .clk(clk), .rstn(rstn),
        .awaddr(s_axi_awaddr), .awvalid(s_axi_awvalid), .awready(s_axi_awready),
        .wdata(s_axi_wdata), .wvalid(s_axi_wvalid), .wready(s_axi_wready),
        .bresp(s_axi_bresp), .bvalid(s_axi_bvalid), .bready(s_axi_bready),
        .araddr(s_axi_araddr), .arvalid(s_axi_arvalid), .arready(s_axi_arready),
        .rdata(s_axi_rdata), .rresp(s_axi_rresp), .rvalid(s_axi_rvalid), .rready(s_axi_rready),
        .enable(enable), .width_us(width_us), .period_us(period_us),
        .min_us(min_us), .max_us(max_us), .applied_us(applied_us)
    );

    servo_engine #(.CLK_MHZ(CLK_MHZ)) u_eng (
        .clk(clk), .rstn(rstn), .enable(enable), .period_us(period_us),
        .width_us(width_us), .min_us(min_us), .max_us(max_us),
        .pwm(servo_pwm), .applied_us(applied_us)
    );
endmodule

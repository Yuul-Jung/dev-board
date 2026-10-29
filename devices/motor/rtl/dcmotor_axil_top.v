// 배선만 한다. 로직은 하위 모듈에 있다.
module dcmotor_axil_top #(
    parameter CLK_MHZ = 100,
    parameter DEAD_US = 1000,
    parameter PERIOD_DEFAULT = 100000
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
    output wire        motor_pwm,   // L298 ENA 또는 TB6612 PWM
    output wire        motor_in1,
    output wire        motor_in2
);
    wire        enable, guard;
    wire [1:0]  mode, applied_mode;
    wire [15:0] duty;
    wire [31:0] period;

    dcmotor_axil_regs #(.PERIOD_DEFAULT(PERIOD_DEFAULT)) u_regs (
        .clk(clk), .rstn(rstn),
        .awaddr(s_axi_awaddr), .awvalid(s_axi_awvalid), .awready(s_axi_awready),
        .wdata(s_axi_wdata), .wvalid(s_axi_wvalid), .wready(s_axi_wready),
        .bresp(s_axi_bresp), .bvalid(s_axi_bvalid), .bready(s_axi_bready),
        .araddr(s_axi_araddr), .arvalid(s_axi_arvalid), .arready(s_axi_arready),
        .rdata(s_axi_rdata), .rresp(s_axi_rresp), .rvalid(s_axi_rvalid), .rready(s_axi_rready),
        .enable(enable), .mode(mode), .duty(duty), .period(period),
        .applied_mode(applied_mode), .guard(guard)
    );

    dcmotor_engine #(.CLK_MHZ(CLK_MHZ), .DEAD_US(DEAD_US)) u_eng (
        .clk(clk), .rstn(rstn), .enable(enable), .mode(mode), .duty(duty), .period(period),
        .pwm(motor_pwm), .in1(motor_in1), .in2(motor_in2),
        .applied_mode(applied_mode), .guard(guard)
    );
endmodule

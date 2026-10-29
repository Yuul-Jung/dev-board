`timescale 1ns/1ps
// 엔진 단독 테스트. 1 MHz 클럭, 주기 100, 듀티 25로 줄여 돌린다.
module tb_dcmotor_engine;
    reg clk = 0, rstn = 0;
    always #500 clk = ~clk;   // 1 MHz
    reg enable = 0; reg [1:0] mode = 0; reg [15:0] duty = 25; reg [31:0] period = 100;
    wire pwm, in1, in2, guard; wire [1:0] applied;

    dcmotor_engine #(.CLK_MHZ(1), .DEAD_US(20)) dut (
        .clk(clk), .rstn(rstn), .enable(enable), .mode(mode), .duty(duty), .period(period),
        .pwm(pwm), .in1(in1), .in2(in2), .applied_mode(applied), .guard(guard));

    integer errors = 0, i, hi, coast_clks;

    task wait_clks(input integer n); integer k; begin for (k = 0; k < n; k = k + 1) @(posedge clk); end endtask
    task check_dir(input e1, input e2, input [255:0] name);
        begin
            if (in1 !== e1 || in2 !== e2) begin
                $display("FAIL %0s in1=%b in2=%b", name, in1, in2); errors = errors + 1; end
        end
    endtask

    initial begin
        #2000 rstn = 1; wait_clks(5);
        check_dir(0, 0, "reset coast");
        // 정회전, 듀티 25/100 -> 1000 클럭 중 약 250 High
        enable = 1; mode = 1; wait_clks(10);
        check_dir(1, 0, "forward");
        hi = 0; for (i = 0; i < 1000; i = i + 1) begin @(posedge clk); if (pwm) hi = hi + 1; end
        if (hi < 240 || hi > 260) begin $display("FAIL duty hi=%0d", hi); errors = errors + 1; end
        // 역회전으로 전환: 코스트 구간을 거친다
        mode = 2; coast_clks = 0;
        for (i = 0; i < 60; i = i + 1) begin
            @(posedge clk);
            if (in1 === 1'b0 && in2 === 1'b0) coast_clks = coast_clks + 1;
        end
        if (coast_clks < 15) begin $display("FAIL guard coast=%0d", coast_clks); errors = errors + 1; end
        check_dir(0, 1, "reverse");
        // 브레이크: 둘 다 1, PWM 항상 1
        mode = 3; wait_clks(10);
        check_dir(1, 1, "brake");
        if (pwm !== 1'b1) begin $display("FAIL brake pwm"); errors = errors + 1; end
        // 비활성화
        enable = 0; wait_clks(10);
        check_dir(0, 0, "disable coast");
        if (pwm !== 1'b0) begin $display("FAIL disable pwm"); errors = errors + 1; end
        if (errors == 0) $display("PASS"); else $display("ERRORS=%0d", errors);
        $finish;
    end
endmodule

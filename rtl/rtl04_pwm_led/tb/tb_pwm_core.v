// 한 PWM 주기 동안 High 카운트로 Duty=0,128,256 검증.
// 축소 CLK_DIV=2 사용; 실제 Basys3 100 MHz 동작 주파수 주장 아님.
`timescale 1ns/1ps
module tb_pwm_core;
    reg clk = 0, rst = 1;
    reg [8:0] duty_in = 0;
    wire pwm_out;
    integer i, high_count;
    always #5 clk = ~clk;

    pwm_core #(.CLK_DIV(2)) dut (
        .clk(clk), .rst(rst), .duty_in(duty_in), .pwm_out(pwm_out)
    );

    // 매 주기 경계 전 요청값을 설정하고 완전한 다음 주기를 측정.
    task sample_period(input [8:0] request, input integer expected);
        begin
            @(negedge clk);
            duty_in = request;
            // 현재 상태에 관계없이 주기 경계까지 대기.
            wait(dut.phase == 8'hFF && dut.div_count == 1);
            @(posedge clk); #1; // 다음 주기 phase=0, duty_reg 갱신
            high_count = 0;
            for (i=0; i<512; i=i+1) begin
                if (pwm_out) high_count = high_count + 1;
                @(posedge clk); #1;
            end
            if (high_count !== expected)
                $fatal(1,"Duty %0d expected high %0d got %0d",
                       request,expected,high_count);
        end
    endtask

    initial begin
        repeat (3) @(negedge clk);
        rst = 0;
        sample_period(9'd0, 0);
        sample_period(9'd128, 256);
        sample_period(9'd256, 512);
        $display("PASS pwm_core 0/50/100 percent");
        $finish;
    end
endmodule

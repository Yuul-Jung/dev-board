// Stimulus → DUT → Expected Result: sample_valid에 의한 Duty 갱신 검증.
`timescale 1ns/1ps
module tb_xadc_duty_controller;
    reg clk = 1'b0;
    reg rst = 1'b1;
    reg [11:0] sample_data = 12'd0;
    reg sample_valid = 1'b0;
    wire [8:0] duty;
    always #5 clk = ~clk;

    xadc_duty_controller dut (
        .clk(clk), .rst(rst),
        .sample_data(sample_data), .sample_valid(sample_valid),
        .duty(duty)
    );

    task check(input [8:0] expected);
        begin
            #1;
            if (duty !== expected)
                $fatal(1, "duty expected %d got %d", expected, duty);
        end
    endtask

    task send_sample(input [11:0] adc);
        begin
            @(negedge clk);
            sample_data = adc;
            sample_valid = 1'b1;
            @(posedge clk);
            check(adc == 12'hFFF ? 9'd256 : {1'b0,adc[11:4]});
            @(negedge clk);
            sample_valid = 1'b0;
        end
    endtask

    initial begin
        repeat (2) @(negedge clk);
        check(9'd0);
        rst = 1'b0;
        send_sample(12'd0);
        send_sample(12'd2048);
        send_sample(12'd4095);
        check(9'd256);
        @(negedge clk);
        sample_data = 12'd0;
        repeat (3) @(posedge clk);
        check(9'd256); // valid=0이면 이전 값 유지
        send_sample(12'd1024);
        check(9'd64);
        @(negedge clk);
        rst = 1'b1;
        @(posedge clk);
        check(9'd0);
        $display("PASS xadc_duty_controller");
        $finish;
    end
endmodule

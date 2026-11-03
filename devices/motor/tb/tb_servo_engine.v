// 서보 엔진 펄스 폭·범위 제한·무효 레지스터 설정 fail-safe 회귀시험.
// CLK_MHZ=100, 주기 200 µs 로 줄이고 폭 80 µs 와 상한 제한을 확인한다.
`timescale 1ns/1ps
module tb_servo_engine;
    reg clk = 0, rstn = 0, enable = 0;
    always #5 clk = ~clk;
    reg [15:0] width = 16'd80;
    reg [15:0] period = 16'd200, min_width = 16'd50, max_width = 16'd120;
    wire pwm; wire [15:0] applied;
    servo_engine #(.CLK_MHZ(100)) dut (.clk(clk), .rstn(rstn), .enable(enable),
        .period_us(period), .width_us(width), .min_us(min_width), .max_us(max_width),
        .pwm(pwm), .applied_us(applied));
    integer hi = 0, errors = 0;
    initial begin
        repeat (4) @(posedge clk); rstn = 1; enable = 1;
        repeat (20000) @(posedge clk);               // 첫 주기 지나가도록 대기 (200 µs)
        hi = 0;
        repeat (20000) begin @(posedge clk); if (pwm) hi = hi + 1; end   // 한 주기 동안 High 클럭 수
        if (hi < 7900 || hi > 8100) begin $display("FAIL width80 hi=%0d", hi); errors = errors + 1; end
        width = 16'd500;                              // 상한 120 으로 제한되어야 함
        repeat (40000) @(posedge clk);
        hi = 0;
        repeat (20000) begin @(posedge clk); if (pwm) hi = hi + 1; end
        if (applied !== 16'd120) begin $display("FAIL applied=%0d", applied); errors = errors + 1; end
        if (hi < 11900 || hi > 12100) begin $display("FAIL clamp hi=%0d", hi); errors = errors + 1; end
        // (추가) C 검증을 우회한 AXI 직접 쓰기에서 PWM이 반드시 LOW인지 확인.
        period = 16'd0;
        repeat (3) @(negedge clk);
        if (pwm !== 1'b0 || applied !== 16'd0) begin
            $display("FAIL zero period"); errors = errors + 1;
        end
        period = 16'd200; min_width = 16'd121; max_width = 16'd120;
        repeat (3) @(negedge clk);
        if (pwm !== 1'b0 || applied !== 16'd0) begin
            $display("FAIL inverted bounds"); errors = errors + 1;
        end
        min_width = 16'd0; max_width = 16'd120;
        repeat (3) @(negedge clk);
        if (pwm !== 1'b0 || applied !== 16'd0) begin
            $display("FAIL zero min"); errors = errors + 1;
        end
        min_width = 16'd50; max_width = 16'd200;
        repeat (3) @(negedge clk);
        if (pwm !== 1'b0 || applied !== 16'd0) begin
            $display("FAIL max >= period"); errors = errors + 1;
        end
        max_width = 16'd120; width = 16'd80;
        repeat (200) @(negedge clk);
        if (applied !== 16'd80) begin
            $display("FAIL restore applied"); errors = errors + 1;
        end
        if (errors == 0) $display("PASS"); else $display("FAIL errors=%0d", errors);
        $finish;
    end
endmodule

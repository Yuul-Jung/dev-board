`timescale 1ns/1ps
// 코어 단독 테스트. 트리거 뒤 에코를 만들어 넣어 거리 변환과 타임아웃을 확인한다.
module tb_ultrasonic;
    reg clk = 0, reset = 1, enable = 0;
    always #5 clk = ~clk;   // 100 MHz
    reg echo = 0;
    wire trig, busy, valid, timeout_error;
    wire [9:0] dist; wire [11:0] bcd;

    ultrasonic_distance_meter dut (
        .clk(clk), .reset(reset), .enable(enable), .echo(echo), .trig(trig),
        .distance_cm(dist), .distance_bcd(bcd), .distance_valid(valid),
        .busy(busy), .timeout_error(timeout_error));

    integer errors = 0;
    // 트리거가 끝나면 지연 후 echo_us 동안 에코를 올린다
    task send_echo(input integer echo_us);
        begin
            @(negedge trig);
            #(100_000); echo = 1; #(echo_us * 1000); echo = 0;
        end
    endtask

    initial begin
        #100 reset = 0; enable = 1;
        // 10 cm = 580 us
        send_echo(580);
        wait (!busy); #1000;
        if (!valid || dist !== 10'd10) begin $display("FAIL 10cm dist=%0d valid=%b", dist, valid); errors = errors + 1; end
        if (bcd !== 12'h010) begin $display("FAIL bcd=%h", bcd); errors = errors + 1; end
        // 100 cm = 5800 us
        send_echo(5800);
        wait (!busy); #1000;
        if (dist !== 10'd100) begin $display("FAIL 100cm dist=%0d", dist); errors = errors + 1; end
        // 에코 없음 -> 타임아웃
        @(posedge timeout_error);
        #1000;
        if (valid) begin $display("FAIL valid after timeout"); errors = errors + 1; end
        if (errors == 0) $display("PASS"); else $display("ERRORS=%0d", errors);
        $finish;
    end
endmodule

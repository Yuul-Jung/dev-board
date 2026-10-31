// 축소된 4클럭 Tick의 간격과 LED 상태 확인 (셀프체크).
`timescale 1ns/1ps
module tb_timer_led;
    reg clk=0, rst=1, enabled=0;
    wire tick, led;
    integer ticks=0, led_rises=0;
    always #5 clk=~clk;
    tick_generator #(.CLOCKS_PER_TICK(4)) u_tick(
        .clk(clk), .rst(rst), .tick(tick));
    timer_led_controller u_led(
        .clk(clk), .rst(rst), .enabled(enabled),
        .tick(tick), .led(led));

    task cycles(input integer n);
        integer i;
        begin for(i=0;i<n;i=i+1) @(negedge clk); end
    endtask
    initial begin
        cycles(3);rst=0;enabled=1;
        cycles(5);
        if (led !== 1'b1) $fatal(1,"first toggle");
        cycles(4);
        if (led !== 1'b0) $fatal(1,"second toggle");
        enabled=0;
        cycles(5);
        if (led !== 1'b0) $fatal(1,"disable clear");
        enabled=1;
        cycles(4);
        if (led !== 1'b1) $fatal(1,"re-enable tick");
        $display("PASS timer_led tick/enable");
        $finish;
    end
endmodule

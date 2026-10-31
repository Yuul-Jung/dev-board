// RTL06 타이머 LED 검증(2차 보강).
//  - tick_chk: tick_generator 를 CLOCKS_PER_TICK=1..8 에서 독립 모델과 매 클럭 비교(임의 Reset 포함).
//    작은 값(1,2)은 폭 계산, 6·8 은 카운터 방향 결함을 드러냄.
//  - sync_chk: switch_sync(RTL07 도 재사용) 의 2클럭 지연·Reset 값을 모델과 비교(sw_in=0 인 채 Reset 해제 포함).
//  - 기존 직접 시나리오(LED 토글/disable/재enable) 유지.
`timescale 1ns/1ps
module tick_chk #(parameter integer N = 4, SEED = 1)(input wire clk);
    reg rst; wire tick;
    tick_generator #(.CLOCKS_PER_TICK(N)) dut(.clk(clk), .rst(rst), .tick(tick));
    integer m_cnt, seed, rhold, checks, ticks; reg m_tick, armed;
    initial begin rst = 1; seed = SEED; rhold = 2; checks = 0; ticks = 0; armed = 0; m_cnt = 0; m_tick = 0; end
    always @(posedge clk) begin
        if (rst) begin m_cnt = 0; m_tick = 0; armed = 1; end
        else if (m_cnt == N - 1) begin m_cnt = 0; m_tick = 1; end
        else begin m_cnt = m_cnt + 1; m_tick = 0; end
    end
    always @(negedge clk) begin
        if (armed) begin
            checks = checks + 1; if (m_tick) ticks = ticks + 1;
            if (tick !== m_tick) $fatal(1, "tick N=%0d t=%0t tick=%b expected %b", N, $time, tick, m_tick);
        end
        if (rhold > 0) begin rhold = rhold - 1; if (rhold == 0) rst = 0; end
        else if ((($random(seed) & 32'h7FFFFFFF) % 200) == 0) begin rst = 1; rhold = 1 + (($random(seed) & 32'h7FFFFFFF) % 3); end
    end
    task final_check;
        begin if (checks < 20000 || ticks < 100) $fatal(1, "tick_chk N=%0d coverage checks=%0d ticks=%0d", N, checks, ticks); end
    endtask
endmodule

module sync_chk #(parameter integer SEED = 3)(input wire clk);
    reg rst, sw; wire o;
    switch_sync dut(.clk(clk), .rst(rst), .sw_in(sw), .sw_out(o));
    integer m_meta, m_sync, seed, hold, rhold, checks, ones; reg armed;
    initial begin rst = 1; sw = 0; seed = SEED; hold = 0; rhold = 2; checks = 0; ones = 0; armed = 0; m_meta = 0; m_sync = 0; end
    always @(posedge clk) begin
        if (rst) begin m_meta = 0; m_sync = 0; armed = 1; end
        else begin m_sync = m_meta; m_meta = sw; end
    end
    always @(negedge clk) begin
        if (armed) begin
            checks = checks + 1; if (m_sync) ones = ones + 1;
            if (o !== m_sync[0]) $fatal(1, "switch_sync t=%0t out=%b expected %0d", $time, o, m_sync);
        end
        if (rhold > 0) begin rhold = rhold - 1; if (rhold == 0) rst = 0; end
        else if ((($random(seed) & 32'h7FFFFFFF) % 40) == 0) begin rst = 1; rhold = 1 + (($random(seed) & 32'h7FFFFFFF) % 3); end
        if (hold > 0) hold = hold - 1;
        else begin sw = ~sw; hold = ($random(seed) & 32'h7FFFFFFF) % 7; end
    end
    task final_check;
        begin if (checks < 20000 || ones < 2000) $fatal(1, "sync_chk coverage checks=%0d ones=%0d", checks, ones); end
    endtask
endmodule

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

    tick_chk #(1, 41) t1(.clk(clk));
    tick_chk #(2, 42) t2(.clk(clk));
    tick_chk #(3, 43) t3(.clk(clk));
    tick_chk #(4, 44) t4(.clk(clk));
    tick_chk #(5, 45) t5(.clk(clk));
    tick_chk #(6, 46) t6(.clk(clk));
    tick_chk #(7, 47) t7(.clk(clk));
    tick_chk #(8, 48) t8(.clk(clk));
    sync_chk #(51) s0(.clk(clk));

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
        cycles(30000);
        t1.final_check; t2.final_check; t3.final_check; t4.final_check;
        t5.final_check; t6.final_check; t7.final_check; t8.final_check; s0.final_check;
        $display("PASS timer_led tick/enable + model checks");
        $finish;
    end
    initial begin #10_000_000; $fatal(1, "test timeout"); end
endmodule

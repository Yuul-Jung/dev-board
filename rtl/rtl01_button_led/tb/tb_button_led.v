// RTL01 버튼 이벤트/LED 토글 검증(2차 보강).
//  - btn_ev_chk: button_event 를 독립 참조 모델(2FF 동기 → 연속 불일치 카운터 → 1클럭 펄스)과 매 클럭 비교.
//    ACTIVE_LOW 0/1, TICKS=1,2,3,5,6,8(작은 TICKS 는 Reset 값·CW 계산·카운터 방향 결함을 드러냄),
//    임의 길이 눌림/바운스, 눌린 채 Reset 해제를 포함.
//  - led_chk: led_toggle_controller 를 임의 펄스/Reset 으로 모델과 비교.
//  - 기존 직접 시나리오(두 번 누름) 유지.
// 100 MHz·20 ms(TICKS=2,000,000) 실제 값과 basys3_button_led_top 배선은 시뮬레이션하지 않았다(미검증).
`timescale 1ns/1ps
module btn_ev_chk #(parameter integer AL = 1, HZ = 1000, MS = 3, SEED = 1)(input wire clk);
    localparam integer TICKS = HZ / 1000 * MS;
    reg rst, btn;
    wire pulse;
    button_event #(.CLK_HZ(HZ), .DEBOUNCE_MS(MS), .ACTIVE_LOW(AL)) dut(
        .clk(clk), .rst(rst), .button_in(btn), .press_pulse(pulse));
    integer m_s1, m_s2, m_run, m_st, val, seed, hold, rhold, checks, npulse;
    reg m_pulse, armed;
    initial begin
        seed = SEED; hold = 0; rhold = 2; checks = 0; npulse = 0; armed = 0;
        rst = 1; btn = (AL != 0) ? 1'b1 : 1'b0;
        m_s1 = 0; m_s2 = 0; m_run = 0; m_st = 0; m_pulse = 0;
    end
    always @(posedge clk) begin
        if (rst) begin
            m_s1 = (AL != 0); m_s2 = (AL != 0); m_run = 0; m_st = 0; m_pulse = 0; armed = 1;
        end else begin
            val = (AL != 0) ? !m_s2 : m_s2;
            m_pulse = 0;
            if (val == m_st) m_run = 0;
            else if (m_run == TICKS - 1) begin m_run = 0; m_st = val; if (val) m_pulse = 1; end
            else m_run = m_run + 1;
            m_s2 = m_s1; m_s1 = btn;
        end
    end
    always @(negedge clk) begin
        if (armed) begin
            checks = checks + 1;
            if (pulse !== m_pulse) $fatal(1, "btn_ev AL=%0d TICKS=%0d t=%0t pulse=%b expected %b", AL, TICKS, $time, pulse, m_pulse);
            if (m_pulse) npulse = npulse + 1;
        end
        if (rhold > 0) begin rhold = rhold - 1; if (rhold == 0) rst = 0; end
        else if ((($random(seed) & 32'h7FFFFFFF) % 250) == 0) begin rst = 1; rhold = 1 + (($random(seed) & 32'h7FFFFFFF) % 3); end
        if (hold > 0) hold = hold - 1;
        else begin btn = ~btn; hold = ($random(seed) & 32'h7FFFFFFF) % (3 * TICKS + 3); end
    end
    task final_check;
        begin
            if (checks < 25000 || npulse < 15) $fatal(1, "btn_ev AL=%0d TICKS=%0d checks=%0d pulses=%0d (coverage too low)", AL, TICKS, checks, npulse);
        end
    endtask
endmodule

module led_chk #(parameter integer SEED = 5)(input wire clk);
    reg rst, pulse;
    wire led;
    led_toggle_controller dut(.clk(clk), .rst(rst), .press_pulse(pulse), .led(led));
    reg m_led, armed; integer seed, checks, toggles;
    initial begin rst = 1; pulse = 0; m_led = 0; armed = 0; seed = SEED; checks = 0; toggles = 0; end
    always @(posedge clk) begin
        if (rst) begin m_led = 0; armed = 1; end
        else if (pulse) begin m_led = ~m_led; toggles = toggles + 1; end
    end
    always @(negedge clk) begin
        if (armed) begin
            checks = checks + 1;
            if (led !== m_led) $fatal(1, "led_toggle led=%b expected %b", led, m_led);
        end
        rst = ((($random(seed) & 32'h7FFFFFFF) % 40) == 0);
        pulse = $random(seed);
    end
    task final_check;
        begin if (checks < 25000 || toggles < 500) $fatal(1, "led_chk coverage checks=%0d toggles=%0d", checks, toggles); end
    endtask
endmodule

module tb_button_led;
    reg clk=0, rst=1, btn=1;
    wire press_pulse, led;
    always #5 clk=~clk;

    button_event #(.CLK_HZ(1000), .DEBOUNCE_MS(3), .ACTIVE_LOW(1))
        u_event (.clk(clk), .rst(rst), .button_in(btn), .press_pulse(press_pulse));
    led_toggle_controller u_control (.clk(clk), .rst(rst),
                                      .press_pulse(press_pulse), .led(led));
    integer events=0;
    always @(posedge clk)
        if (!rst && press_pulse) events <= events+1;

    btn_ev_chk #(1, 1000, 1, 11) c0(.clk(clk));
    btn_ev_chk #(0, 1000, 1, 12) c1(.clk(clk));
    btn_ev_chk #(1, 1000, 2, 13) c2(.clk(clk));
    btn_ev_chk #(0, 1000, 3, 14) c3(.clk(clk));
    btn_ev_chk #(1, 1000, 5, 15) c4(.clk(clk));
    btn_ev_chk #(0, 1000, 6, 16) c5(.clk(clk));
    btn_ev_chk #(1, 2000, 3, 17) c6(.clk(clk));
    btn_ev_chk #(0, 1000, 8, 18) c7(.clk(clk));
    led_chk #(21) l0(.clk(clk));

    task run_clocks(input integer n);
        integer i;
        begin for(i=0;i<n;i=i+1) @(negedge clk); end
    endtask
    initial begin
        run_clocks(3); rst=0;
        btn=0; run_clocks(9);
        if (led !== 1'b1 || events !== 1) $fatal(1,"first press");
        btn=1; run_clocks(9);
        if (led !== 1'b1 || events !== 1) $fatal(1,"release toggled");
        btn=0; run_clocks(9);
        if (led !== 1'b0 || events !== 2) $fatal(1,"second press");
        run_clocks(30000);
        c0.final_check; c1.final_check; c2.final_check; c3.final_check;
        c4.final_check; c5.final_check; c6.final_check; c7.final_check; l0.final_check;
        $display("PASS: two debounced presses + model/random checks");
        $finish;
    end
    initial begin #20_000_000; $fatal(1, "test timeout"); end
endmodule

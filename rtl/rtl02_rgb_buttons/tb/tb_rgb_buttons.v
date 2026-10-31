// RTL02 RGB 버튼 검증(2차 보강).
//  - lvl_chk: button_level 을 독립 모델(2FF 동기 → 연속 불일치 카운터 → 눌림 레벨)과 매 클럭 비교.
//    BUTTON_ACTIVE_LOW 0/1, TICKS=1,2,3,5,6,8, 임의 눌림/바운스/눌린 채 Reset 해제.
//  - rgb_button_controller 진리표 4조합 전수.
//  - 기존 직접 시나리오 유지. 실제 100 MHz·20 ms 값과 top 배선은 시뮬레이션하지 않았다(미검증).
`timescale 1ns/1ps
module lvl_chk #(parameter integer AL = 0, HZ = 1000, MS = 3, SEED = 1)(input wire clk);
    localparam integer TICKS = (HZ / 1000) * MS;
    reg rst, btn;
    wire pressed;
    button_level #(.CLK_HZ(HZ), .DEBOUNCE_MS(MS), .BUTTON_ACTIVE_LOW(AL)) dut(
        .clk(clk), .rst(rst), .button_in(btn), .pressed(pressed));
    integer m_s1, m_s2, m_run, m_st, val, seed, hold, rhold, checks, edges;
    reg armed;
    initial begin
        seed = SEED; hold = 0; rhold = 2; checks = 0; edges = 0; armed = 0;
        rst = 1; btn = (AL != 0) ? 1'b1 : 1'b0;
        m_s1 = 0; m_s2 = 0; m_run = 0; m_st = 0;
    end
    always @(posedge clk) begin
        if (rst) begin
            m_s1 = (AL != 0); m_s2 = (AL != 0); m_run = 0; m_st = 0; armed = 1;
        end else begin
            val = (AL != 0) ? !m_s2 : m_s2;
            if (val == m_st) m_run = 0;
            else if (m_run == TICKS - 1) begin m_run = 0; m_st = val; edges = edges + 1; end
            else m_run = m_run + 1;
            m_s2 = m_s1; m_s1 = btn;
        end
    end
    always @(negedge clk) begin
        if (armed) begin
            checks = checks + 1;
            if (pressed !== m_st[0]) $fatal(1, "btn_level AL=%0d TICKS=%0d t=%0t pressed=%b expected %0d", AL, TICKS, $time, pressed, m_st);
        end
        if (rhold > 0) begin rhold = rhold - 1; if (rhold == 0) rst = 0; end
        else if ((($random(seed) & 32'h7FFFFFFF) % 250) == 0) begin rst = 1; rhold = 1 + (($random(seed) & 32'h7FFFFFFF) % 3); end
        if (hold > 0) hold = hold - 1;
        else begin btn = ~btn; hold = ($random(seed) & 32'h7FFFFFFF) % (3 * TICKS + 3); end
    end
    task final_check;
        begin
            if (checks < 25000 || edges < 30) $fatal(1, "btn_level AL=%0d TICKS=%0d checks=%0d edges=%0d (coverage too low)", AL, TICKS, checks, edges);
        end
    endtask
endmodule

module tb_rgb_buttons;
    reg clk=0,rst=1,btn_a=0,btn_b=0;
    wire a,b,r,g,bl;
    always #5 clk=~clk;
    button_level #(.CLK_HZ(1000),.DEBOUNCE_MS(3)) u_a(
        .clk(clk),.rst(rst),.button_in(btn_a),.pressed(a));
    button_level #(.CLK_HZ(1000),.DEBOUNCE_MS(3)) u_b(
        .clk(clk),.rst(rst),.button_in(btn_b),.pressed(b));
    rgb_button_controller u_rgb(
        .pressed_a(a),.pressed_b(b),.led_r(r),.led_g(g),.led_b(bl));

    reg ta = 0, tb2 = 0; wire tr, tg, tbl;
    rgb_button_controller u_tt(.pressed_a(ta), .pressed_b(tb2), .led_r(tr), .led_g(tg), .led_b(tbl));

    lvl_chk #(0, 1000, 1, 31) c0(.clk(clk));
    lvl_chk #(1, 1000, 1, 32) c1(.clk(clk));
    lvl_chk #(0, 1000, 2, 33) c2(.clk(clk));
    lvl_chk #(1, 1000, 3, 34) c3(.clk(clk));
    lvl_chk #(0, 1000, 5, 35) c4(.clk(clk));
    lvl_chk #(1, 1000, 6, 36) c5(.clk(clk));
    lvl_chk #(0, 2000, 3, 37) c6(.clk(clk));
    lvl_chk #(1, 1000, 8, 38) c7(.clk(clk));

    task clocks(input integer n);
        integer i;
        begin for(i=0;i<n;i=i+1) @(negedge clk); end
    endtask
    task check(input [2:0] expected_color);
        begin
            #1;
            if ({r,g,bl} !== expected_color)
                $fatal(1,"Expected RGB %b got %b",expected_color,{r,g,bl});
        end
    endtask
    integer k;
    initial begin
        // 조합논리는 입력이 바뀌어야 처음 계산되므로 00 을 마지막(k=4)에 둔다
        for (k = 1; k <= 4; k = k + 1) begin
            {ta, tb2} = k[1:0]; #1;
            if ({tr, tg, tbl} !== ((k[1:0] == 2'd0) ? 3'b000 : (k[1:0] == 2'd2) ? 3'b100 : (k[1:0] == 2'd1) ? 3'b010 : 3'b001))
                $fatal(1, "truth table ab=%b got %b", k[1:0], {tr, tg, tbl});
        end
        clocks(3);rst=0;clocks(4);check(3'b000);
        btn_a=1;clocks(8);check(3'b100);
        btn_b=1;clocks(8);check(3'b001);
        btn_a=0;clocks(8);check(3'b010);
        btn_b=0;clocks(8);check(3'b000);
        btn_a=1;clocks(1);btn_a=0;clocks(8);check(3'b000);
        clocks(30000);
        c0.final_check; c1.final_check; c2.final_check; c3.final_check;
        c4.final_check; c5.final_check; c6.final_check; c7.final_check;
        $display("PASS: RGB button levels, bounce rejection, model/random checks");
        $finish;
    end
    initial begin #20_000_000; $fatal(1, "test timeout"); end
endmodule

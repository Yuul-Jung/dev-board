// RTL07 멜로디/톤 생성 검증(2차 보강).
//  - melody_chk: melody_sequencer + tone_generator 를 독립 모델과 매 클럭 비교(half_period_clks, note_change, buzzer).
//    임의 enable 끊김/Reset, 음 3개 순환(wrap), NOTE_CLOCKS=1,2,3,7,30, half_period 가 2·1·0 이 되는 CLK_HZ 포함.
//  - tone_chk: tone_generator 단독. half_period_clks ∈ {0,1,2,3,5,8,13} 를 note_change 와 함께만 바꾸고(설계 제약:
//    카운트 도중 값을 줄이면 32비트가 한 바퀴 돌 때까지 토글하지 않음 - 이 TB 는 그 상황을 만들지 않는다),
//    enable/Reset/note_change 를 임의로 섞어 비교.
//  - basys3_buzzer_top: sw_play → 동기 → 첫 음(A4)의 토글 간격이 정확히 CLK_HZ/(2*440) 클럭인지(배선 확인).
// 합성/실물 부저 구동은 검증하지 않았다.
`timescale 1ns/1ps
module melody_chk #(parameter integer HZ = 10000, NC = 30, SEED = 1)(input wire clk, input wire rst, input wire enabled);
    wire [31:0] hp; wire change, buzzer;
    melody_sequencer #(.CLK_HZ(HZ), .NOTE_CLOCKS(NC)) s(.clk(clk), .rst(rst), .enabled(enabled), .half_period_clks(hp), .note_change(change));
    tone_generator t(.clk(clk), .rst(rst), .tone_enable(enabled), .note_change(change), .half_period_clks(hp), .buzzer(buzzer));
    integer m_el, m_idx, m_cnt, hpo, checks, toggles, wraps, changes; reg m_chg, m_buz, armed, pbuz;
    function integer hpf(input integer idx);
        begin hpf = (idx == 0) ? HZ / (2 * 440) : (idx == 1) ? HZ / (2 * 523) : HZ / (2 * 659); end
    endfunction
    initial begin m_el = 0; m_idx = 0; m_cnt = 0; m_chg = 0; m_buz = 0; armed = 0; checks = 0; toggles = 0; wraps = 0; changes = 0; pbuz = 0; end
    always @(posedge clk) if (tb_buzzer_melody.chk_on) begin
        hpo = hpf(m_idx);                                   // 톤은 이전 사이클의 note_change / 반주기를 본다
        if (rst || !enabled || m_chg || hpo == 0) begin m_cnt = 0; m_buz = 0; end
        else if (m_cnt == hpo - 1) begin m_cnt = 0; m_buz = ~m_buz; end
        else m_cnt = m_cnt + 1;
        if (rst || !enabled) begin m_el = 0; m_idx = 0; m_chg = 0; end
        else if (m_el == NC - 1) begin m_el = 0; if (m_idx == 2) begin m_idx = 0; wraps = wraps + 1; end else m_idx = m_idx + 1; m_chg = 1; end
        else begin m_el = m_el + 1; m_chg = 0; end
        if (rst) armed = 1;
    end
    always @(negedge clk) if (armed && tb_buzzer_melody.chk_on) begin
        checks = checks + 1; if (m_chg) changes = changes + 1; if (buzzer !== pbuz) toggles = toggles + 1; pbuz = buzzer;
        if (hp !== hpf(m_idx)) $fatal(1, "melody HZ=%0d NC=%0d t=%0t hp=%0d expected %0d", HZ, NC, $time, hp, hpf(m_idx));
        if (change !== m_chg) $fatal(1, "melody HZ=%0d NC=%0d t=%0t note_change=%b expected %b", HZ, NC, $time, change, m_chg);
        if (buzzer !== m_buz) $fatal(1, "melody HZ=%0d NC=%0d t=%0t buzzer=%b expected %b", HZ, NC, $time, buzzer, m_buz);
    end
    task final_check;
        begin if (checks < 30000 || changes < 60 || wraps < 10) $fatal(1, "melody_chk HZ=%0d NC=%0d coverage checks=%0d changes=%0d wraps=%0d", HZ, NC, checks, changes, wraps); end
    endtask
endmodule

module tone_chk #(parameter integer SEED = 9)(input wire clk);
    reg rst, en, nc; reg [31:0] hp; wire buzzer;
    tone_generator dut(.clk(clk), .rst(rst), .tone_enable(en), .note_change(nc), .half_period_clks(hp), .buzzer(buzzer));
    integer m_cnt, seed, r, checks, toggles; reg m_buz, armed, pb;
    initial begin rst = 1; en = 0; nc = 0; hp = 0; seed = SEED; m_cnt = 0; m_buz = 0; armed = 0; checks = 0; toggles = 0; pb = 0; end
    always @(posedge clk) if (tb_buzzer_melody.chk_on) begin
        if (rst || !en || nc || hp == 0) begin m_cnt = 0; m_buz = 0; end
        else if (m_cnt == hp - 1) begin m_cnt = 0; m_buz = ~m_buz; end
        else m_cnt = m_cnt + 1;
        if (rst) armed = 1;
    end
    always @(negedge clk) if (tb_buzzer_melody.chk_on) begin
        if (armed) begin
            checks = checks + 1; if (buzzer !== pb) toggles = toggles + 1; pb = buzzer;
            if (buzzer !== m_buz) $fatal(1, "tone t=%0t buzzer=%b expected %b (hp=%0d en=%b)", $time, buzzer, m_buz, hp, en);
        end
        r = $random(seed);
        rst = (r[7:0] == 8'd3);
        en = (r[15:8] > 8'd20);                                  // 약 92 % enable
        nc = (r[19:16] == 4'd0);                                 // 1/16 note_change
        if (nc || rst || !en) begin
            case (r[26:24]) 0: hp = 0; 1: hp = 1; 2: hp = 2; 3: hp = 3; 4: hp = 5; 5: hp = 8; default: hp = 13; endcase
        end
    end
    task final_check;
        begin if (checks < 30000 || toggles < 800) $fatal(1, "tone_chk coverage checks=%0d toggles=%0d", checks, toggles); end
    endtask
endmodule

module tb_buzzer_melody;
    reg clk = 0, rst = 1, enabled = 0;
    reg chk_on = 1;      // 검사기 동작 여부(top 장시간 시험 중에는 끔: 시뮬레이션 시간 절약)
    wire [31:0] hp;
    wire change, buzzer;
    integer changes = 0, toggles = 0;
    reg prev_buzzer = 0;
    always #5 clk = ~clk;
    melody_sequencer #(.CLK_HZ(10000), .NOTE_CLOCKS(30)) u_seq(
        .clk(clk), .rst(rst), .enabled(enabled), .half_period_clks(hp), .note_change(change));
    tone_generator u_tone(
        .clk(clk), .rst(rst), .tone_enable(enabled),
        .note_change(change), .half_period_clks(hp), .buzzer(buzzer));
    always @(negedge clk) begin
        if (!rst && enabled && change) changes = changes + 1;
        if (!rst && enabled && buzzer != prev_buzzer) toggles = toggles + 1;
        prev_buzzer = buzzer;
    end

    // 공용 임의 자극: Reset 은 드물게, enable 은 가끔 짧게 끊김
    integer seed = 77, r, hold = 0;
    reg rst_r = 1, en_r = 0;
    always @(negedge clk) if (chk_on) begin
        r = $random(seed);
        if (hold > 0) hold = hold - 1;
        else begin en_r = ~en_r; hold = en_r ? 40 + (r[15:8] % 200) : (r[19:16] % 4); end
        rst_r = (r[31:24] == 8'd7) && (r[7:0] < 8'd20);
    end
    melody_chk #(10000, 30, 1) m0(.clk(clk), .rst(rst_r), .enabled(en_r));
    melody_chk #(100000, 1, 2) m1(.clk(clk), .rst(rst_r), .enabled(en_r));
    melody_chk #(100000, 2, 3) m2(.clk(clk), .rst(rst_r), .enabled(en_r));
    melody_chk #(100000, 3, 4) m3(.clk(clk), .rst(rst_r), .enabled(en_r));
    melody_chk #(10000, 7, 5) m4(.clk(clk), .rst(rst_r), .enabled(en_r));
    melody_chk #(1000, 5, 6) m5(.clk(clk), .rst(rst_r), .enabled(en_r));   // 659 Hz 반주기 0
    melody_chk #(2000, 4, 7) m6(.clk(clk), .rst(rst_r), .enabled(en_r));   // 반주기 2,1,1
    tone_chk #(9) tc(.clk(clk));

    // basys3_buzzer_top: 실제 값(NOTE_CLOCKS=25,000,000) 이므로 첫 음의 토글 간격만 본다
    reg sw_play = 0; wire top_buzzer;
    basys3_buzzer_top u_top(.clk(clk), .rst(rst), .sw_play(sw_play), .buzzer(top_buzzer));
    integer tc_cyc = 0, first_t = -1, second_t = -1; reg tb_prev = 0;
    always @(negedge clk) begin
        tc_cyc = tc_cyc + 1;
        if (top_buzzer !== tb_prev && tb_prev !== 1'bx) begin
            if (first_t < 0) first_t = tc_cyc; else if (second_t < 0) second_t = tc_cyc;
        end
        tb_prev = top_buzzer;
    end

    initial begin
        repeat (3) @(negedge clk);
        rst = 0; enabled = 1;
        repeat (95) @(negedge clk);
        if (changes != 3) $fatal(1, "Expected 3 transitions, got %0d", changes);
        if (toggles < 3) $fatal(1, "Missing output toggles");
        enabled = 0;
        repeat (3) @(negedge clk);
        if (buzzer !== 0) $fatal(1, "Buzzer not muted");
        if (hp !== 10000 / (2 * 440)) $fatal(1, "Sequence did not reset");
        repeat (40000) @(negedge clk);
        m0.final_check; m1.final_check; m2.final_check; m3.final_check; m4.final_check; m5.final_check; m6.final_check; tc.final_check;
        chk_on = 0;
        // top: 첫 음의 반주기 간격 확인
        sw_play = 1;
        repeat (3 * 113636) @(negedge clk);
        if (first_t < 0 || second_t < 0) $fatal(1, "top buzzer did not toggle twice");
        if (second_t - first_t !== 113636) $fatal(1, "top half period %0d expected 113636", second_t - first_t);
        $display("PASS buzzer melody + model checks + top half-period");
        $finish;
    end
    initial begin #100_000_000; $fatal(1, "test timeout"); end
endmodule

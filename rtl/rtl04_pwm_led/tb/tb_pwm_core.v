// RTL04 PWM 검증(2차 보강). 구성:
//  1) pwm_chk: DUT(pwm_core) 와 TB 안의 독립 참조 모델을 매 클럭 비교(CLK_DIV=1,2,3). 규약은 src 머리말:
//     phase 8비트, duty 9비트(0..256, 257 이상은 256으로 제한), duty_in 은 phase==255 의 분주 경계에서만 반영,
//     pwm=(phase<duty_reg), Reset 후 duty_reg=0.
//  2) 무작위 duty(0,1,2,127,128,254,255,256,257,300,511,임의) + 중간 Reset + 샘플 경계 -1/0/+1 클럭에서의 변경.
//  3) 모델과 무관한 독립 검사: 한 주기 High 클럭 수 = min(duty,256)*CLK_DIV.
//  4) switch_duty_controller: 2FF 지연 정확히 2클럭, 0xFF→256, Reset.
//  5) basys3_pwm_led_top(CLK_DIV=390): sw=0x80 일 때 한 주기 High = 128*390.
// 축소 CLK_DIV 사용: 실제 Basys3 100 MHz 동작 주파수 주장 아님.
`timescale 1ns/1ps
module pwm_chk #(parameter integer DIV = 2)(
    input wire clk, input wire rst, input wire [8:0] duty_in, output wire pwm_out);
    pwm_core #(.CLK_DIV(DIV)) dut(.clk(clk), .rst(rst), .duty_in(duty_in), .pwm_out(pwm_out));
    integer m_div, m_phase, m_duty, checks;
    reg armed;
    initial begin m_div = 0; m_phase = 0; m_duty = 0; checks = 0; armed = 1'b0; end
    always @(posedge clk) if (tb_pwm_core.chk_on) begin
        if (rst) begin m_div = 0; m_phase = 0; m_duty = 0; armed = 1'b1; end
        else if (m_div == DIV-1) begin
            m_div = 0;
            if (m_phase == 255) m_duty = (duty_in > 256) ? 256 : duty_in;
            m_phase = (m_phase + 1) % 256;
        end else m_div = m_div + 1;
    end
    always @(negedge clk) if (armed && tb_pwm_core.chk_on) begin
        checks = checks + 1;
        if (pwm_out !== (m_phase < m_duty))
            $fatal(1, "PWM DIV=%0d t=%0t phase=%0d duty=%0d pwm=%b", DIV, $time, m_phase, m_duty, pwm_out);
    end
endmodule

module tb_pwm_core;
    reg clk = 0, rst = 1;
    reg chk_on = 1;      // 모델 검사기 동작 여부(top 장시간 시험 중에는 끔)
    reg [8:0] duty_in = 0;
    wire p1, p2, p3, p4, p5;
    always #5 clk = ~clk;
    pwm_chk #(1) c1(.clk(clk), .rst(rst), .duty_in(duty_in), .pwm_out(p1));
    pwm_chk #(2) c2(.clk(clk), .rst(rst), .duty_in(duty_in), .pwm_out(p2));
    pwm_chk #(3) c3(.clk(clk), .rst(rst), .duty_in(duty_in), .pwm_out(p3));
    pwm_chk #(4) c4(.clk(clk), .rst(rst), .duty_in(duty_in), .pwm_out(p4));   // CLK_DIV=4,5: 카운터 방향/폭 결함 노출
    pwm_chk #(5) c5(.clk(clk), .rst(rst), .duty_in(duty_in), .pwm_out(p5));

    // switch_duty_controller / top
    reg rstt = 1; reg [7:0] sw = 8'h00;
    wire [8:0] duty_s; wire led_pwm;
    switch_duty_controller u_sdc(.clk(clk), .rst(rstt), .sw(sw), .duty(duty_s));
    basys3_pwm_led_top u_top(.clk(clk), .rst(rstt), .sw(sw), .led_pwm(led_pwm));

    integer k, seed, r, highs, i;
    initial begin seed = 32'h1234; end

    task chg_at(input integer ph, input integer dv, input [8:0] d);
        begin
            @(negedge clk);
            while (!(c2.m_phase == ph && c2.m_div == dv)) @(negedge clk);
            duty_in = d;
        end
    endtask

    task count_period(input [8:0] d);
        integer exp_h;
        begin
            duty_in = d;
            repeat (2*512) @(negedge clk);          // 새 duty 가 반영될 때까지(주기 512 클럭 x2)
            highs = 0;
            for (i = 0; i < 512; i = i + 1) begin @(negedge clk); if (p2) highs = highs + 1; end
            exp_h = ((d > 256) ? 256 : d) * 2;
            if (highs !== exp_h) $fatal(1, "PWM period high duty=%0d expected %0d got %0d", d, exp_h, highs);
        end
    endtask

    task sw_step(input [7:0] v, input [8:0] exp_d);
        reg [8:0] prev;
        begin
            @(negedge clk); prev = duty_s; sw = v;
            @(negedge clk);
            if (duty_s !== prev) $fatal(1, "SDC changed after 1 clock sw=%h", v);
            @(negedge clk);
            if (duty_s !== exp_d) $fatal(1, "SDC sw=%h expected %0d got %0d", v, exp_d, duty_s);
        end
    endtask

    initial begin
        repeat (3) @(negedge clk); rst = 0;
        for (k = 0; k < 30000; k = k + 1) begin
            @(negedge clk);
            r = $random(seed);
            if (r[15:8] == 8'd0) begin
                case (($random(seed) & 32'h7FFFFFFF) % 12)
                    0: duty_in = 9'd0;   1: duty_in = 9'd1;   2: duty_in = 9'd2;   3: duty_in = 9'd127;
                    4: duty_in = 9'd128; 5: duty_in = 9'd254; 6: duty_in = 9'd255; 7: duty_in = 9'd256;
                    8: duty_in = 9'd257; 9: duty_in = 9'd300; 10: duty_in = 9'd511;
                    default: duty_in = $random(seed);
                endcase
            end
            if (k == 9000) rst = 1;
            if (k == 9003) rst = 0;
        end
        // 샘플 경계 -1 / 0 / +1 클럭에서 duty 변경(모델이 계속 비교)
        chg_at(255, 0, 9'd200);  chg_at(255, 1, 9'd77);  chg_at(0, 0, 9'd10);
        repeat (3*512) @(negedge clk);
        chg_at(255, 1, 9'd0);    chg_at(255, 1, 9'd256); chg_at(255, 1, 9'd255);
        repeat (3*512) @(negedge clk);
        // 모델과 무관한 High 클럭 수
        count_period(9'd0); count_period(9'd1); count_period(9'd128); count_period(9'd255);
        count_period(9'd256); count_period(9'd257); count_period(9'd511); count_period(9'd0);
        // switch_duty_controller
        repeat (3) @(negedge clk); rstt = 0;
        sw_step(8'h00, 9'd0);   sw_step(8'h01, 9'd1);   sw_step(8'h7F, 9'd127); sw_step(8'h80, 9'd128);
        sw_step(8'hFE, 9'd254); sw_step(8'hFF, 9'd256); sw_step(8'h00, 9'd0);   sw_step(8'hFF, 9'd256);
        @(negedge clk); rstt = 1; @(negedge clk);
        if (duty_s !== 9'd0) $fatal(1, "SDC reset duty=%0d", duty_s);
        // sw=0 으로 Reset 후 해제 직후에도 duty 가 0 이어야 한다(sw_meta/sw_sync Reset 값 검사)
        sw = 8'h00; repeat (3) @(negedge clk); rstt = 0;
        @(negedge clk); if (duty_s !== 9'd0) $fatal(1, "SDC after reset(sw=0) duty=%0d (1st)", duty_s);
        @(negedge clk); if (duty_s !== 9'd0) $fatal(1, "SDC after reset(sw=0) duty=%0d (2nd)", duty_s);
        sw = 8'hFF; repeat (3) @(negedge clk); rstt = 1; @(negedge clk);
        @(negedge clk); rstt = 0; repeat (3) @(negedge clk);
        if (duty_s !== 9'd256) $fatal(1, "SDC after reset release duty=%0d", duty_s);
        chk_on = 0;
        // top: CLK_DIV=390, sw=0x80 -> 한 주기(256*390 클럭) High = 128*390
        rstt = 1; sw = 8'h80; repeat (3) @(negedge clk); rstt = 0;
        repeat (2*256*390) @(negedge clk);
        highs = 0;
        for (i = 0; i < 256*390; i = i + 1) begin @(negedge clk); if (led_pwm) highs = highs + 1; end
        if (highs !== 128*390) $fatal(1, "top high expected %0d got %0d", 128*390, highs);
        if (c1.checks < 30000 || c2.checks < 30000 || c3.checks < 30000 || c4.checks < 30000 || c5.checks < 30000) $fatal(1, "too few model checks");
        $display("PASS pwm_core model+boundary+period+sdc+top");
        $finish;
    end
    initial begin #200_000_000; $fatal(1, "test timeout"); end
endmodule

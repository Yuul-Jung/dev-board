`timescale 1ns/1ps
// 자체 검사 테스트벤치 (시뮬레이터 없어 실행하지 못함). 클럭 주기 40 us (Tck 30~50 us 범위).
module tb_ps2_rx_engine;
    reg clk = 0, rstn = 0, c = 1, d = 1;
    wire bv, ep, ef, et; wire [7:0] bo;
    always #5 clk = ~clk;   // 100 MHz
    ps2_rx_engine #(.CLK_MHZ(100)) dut (.clk(clk), .rstn(rstn), .ps2c_i(c), .ps2d_i(d),
        .byte_valid(bv), .byte_out(bo), .err_parity(ep), .err_frame(ef), .err_timeout(et));

    integer got = 0, nerr_par = 0, nerr_frm = 0, nerr_to = 0, fails = 0;
    reg [7:0] last;
    always @(posedge clk) begin
        if (bv) begin got <= got + 1; last <= bo; end
        if (ep) nerr_par <= nerr_par + 1;
        if (ef) nerr_frm <= nerr_frm + 1;
        if (et) nerr_to  <= nerr_to + 1;
    end

    task bit_out(input b);
        begin d = b; #5000; c = 0; #20000; c = 1; #15000; end   // 데이터 먼저 5 us 뒤 클럭 Low 20 us, High 15 us
    endtask
    task send(input [7:0] v, input par_flip, input stop);
        integer i;
        begin
            bit_out(0);
            for (i = 0; i < 8; i = i + 1) bit_out(v[i]);
            bit_out(~(^v) ^ par_flip);       // 홀수 패리티 비트 = ~(^v)
            bit_out(stop);
            d = 1; #100000;
        end
    endtask

    initial begin
        #100 rstn = 1; #1000;
        send(8'h1C, 0, 1); if (got != 1 || last != 8'h1C) begin $display("FAIL 1C"); fails = fails + 1; end
        send(8'hF0, 0, 1); if (got != 2 || last != 8'hF0) begin $display("FAIL F0"); fails = fails + 1; end
        send(8'h1C, 1, 1); if (got != 2 || nerr_par != 1)  begin $display("FAIL parity"); fails = fails + 1; end
        send(8'h45, 0, 0); if (got != 2 || nerr_frm != 1)  begin $display("FAIL stop"); fails = fails + 1; end
        // 타임아웃: 시작 비트와 데이터 2비트만 보내고 멈춤
        bit_out(0); bit_out(1); bit_out(0); #2500000;
        if (nerr_to != 1) begin $display("FAIL timeout %0d", nerr_to); fails = fails + 1; end
        send(8'hE0, 0, 1); if (got != 3 || last != 8'hE0) begin $display("FAIL after timeout"); fails = fails + 1; end
        if (fails == 0) $display("ALL PASS"); else $display("FAILED %0d", fails);
        $finish;
    end
endmodule

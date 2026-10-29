// 자체 점검 테스트벤치 (미실행: 이 환경에 시뮬레이터 없음).
// 1) 명령 0x2C 송신: 상승 에지마다 SDA를 모아 0x2C, D/CX=0 확인
// 2) fill 3픽셀 0xF81F: 6바이트 F8 1F F8 1F F8 1F, D/CX=1 확인
`timescale 1ns/1ps
module tb_ili9341_engine;
    reg clk = 0, rstn = 0;
    always #5 clk = ~clk;
    reg tx_start = 0, fill_start = 0, tx_dc = 0;
    reg [7:0] tx_byte = 0;
    reg [15:0] fill_color = 0;
    reg [19:0] fill_cnt = 0;
    wire busy, scl, sda, dcx;
    ili9341_engine dut (.clk(clk), .rstn(rstn), .div(16'd1),
        .tx_start(tx_start), .tx_byte(tx_byte), .tx_dc(tx_dc),
        .fill_start(fill_start), .fill_color(fill_color), .fill_cnt(fill_cnt),
        .busy(busy), .scl(scl), .sda(sda), .dcx(dcx));

    reg [7:0] rx; integer nbits = 0, nbytes = 0, errors = 0;
    reg [7:0] got [0:15]; reg dcgot [0:15];
    always @(posedge scl) begin
        rx = {rx[6:0], sda}; nbits = nbits + 1;
        if (nbits == 8) begin got[nbytes] = rx; dcgot[nbytes] = dcx; nbytes = nbytes + 1; nbits = 0; end
    end

    task chk(input integer i, input [7:0] e, input d);
        begin
            if (got[i] !== e || dcgot[i] !== d) begin
                $display("FAIL byte %0d: got %h dc=%b, want %h dc=%b", i, got[i], dcgot[i], e, d);
                errors = errors + 1;
            end
        end
    endtask

    initial begin
        repeat (4) @(posedge clk); rstn = 1; repeat (2) @(posedge clk);
        tx_byte = 8'h2C; tx_dc = 0; tx_start = 1; @(posedge clk); tx_start = 0;
        wait (busy); wait (!busy); repeat (2) @(posedge clk);
        fill_color = 16'hF81F; fill_cnt = 20'd3; fill_start = 1; @(posedge clk); fill_start = 0;
        wait (busy); wait (!busy); repeat (2) @(posedge clk);
        if (nbytes != 7) begin $display("FAIL nbytes=%0d", nbytes); errors = errors + 1; end
        chk(0, 8'h2C, 1'b0);
        chk(1, 8'hF8, 1'b1); chk(2, 8'h1F, 1'b1); chk(3, 8'hF8, 1'b1);
        chk(4, 8'h1F, 1'b1); chk(5, 8'hF8, 1'b1); chk(6, 8'h1F, 1'b1);
        if (errors == 0) $display("PASS"); else $display("FAIL errors=%0d", errors);
        $finish;
    end
endmodule

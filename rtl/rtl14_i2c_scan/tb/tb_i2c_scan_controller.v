// RTL14 I2C 주소 스캔 컨트롤러 검증(2차 보강). 컨트롤러만 검증하며 실제 I2C 버스/IP/PHY 는 없다(미검증).
// 응답기(responder)를 TB 가 모사: 명령 수락 후 1~6 클럭 뒤 reply_valid 를 1클럭, ack 는 주소별 규칙.
// 검사(스코어보드):
//  - 시작 직후 busy=1, cmd_valid=1, address=0x08, found=0
//  - 수락된 주소가 0x08 부터 0x77 까지 정확히 112개, 순서대로, 0x78 이상 없음
//  - 수락~응답 사이 cmd_valid=0 이고 address 유지(응답 없이 진행하지 않음)
//  - found = 지금까지 받은 ack 의 OR(0x08 만 / 0x77 만 / 없음 / 일부 / 전부), 스캔 동안 값이 줄지 않음
//  - done 은 0x77 응답 직후 정확히 1클럭, 그때 busy=0
//  - busy 중 start(임의 반복) 무시, idle 중 start 없으면 시작하지 않음, 스캔 중 Reset 후 address=0x08·모든 출력 0, 재스캔 정상
`timescale 1ns/1ps
module tb_i2c_scan_controller;
reg clk=0,rst=1,start=0,cmd_ready=0,reply_valid=0,reply_ack=0;
wire cmd_valid,found,done,busy; wire [6:0]address;
i2c_scan_controller dut(.clk(clk),.rst(rst),.start(start),.cmd_ready(cmd_ready),.reply_valid(reply_valid),.reply_ack(reply_ack),.cmd_valid(cmd_valid),.address(address),.found(found),.done(done),.busy(busy));
always #5 clk=~clk;
integer seed = 99;

function ackfn(input integer mode, input [6:0] a);
    begin
        case (mode)
            0: ackfn = 1'b0;
            1: ackfn = (a == 7'h08);
            2: ackfn = (a == 7'h77);
            3: ackfn = (a == 7'h3C) || (a == 7'h50);
            default: ackfn = 1'b1;
        endcase
    end
endfunction

// mode: ack 규칙, rnd: cmd_ready 무작위/응답 지연 무작위, spam: busy 중 start 반복, rst_at: 수락 N개째에 Reset(0이면 없음)
task do_scan(input integer mode, input integer rnd, input integer spam, input integer rst_at);
    integer acc, rep, tcount, rv_cnt, cd; reg [6:0] exp_addr, acc_addr; reg or_ack, pend, pend_val, waiting_tb, done_seen, rst_done;
    begin
        @(negedge clk); start = 1; @(negedge clk); start = 0;
        if (busy !== 1'b1 || cmd_valid !== 1'b1 || address !== 7'h08 || found !== 1'b0 || done !== 1'b0)
            $fatal(1, "scan start busy=%b cmd_valid=%b addr=%h found=%b done=%b", busy, cmd_valid, address, found, done);
        exp_addr = 7'h08; acc = 0; rep = 0; or_ack = 0; pend = 0; pend_val = 0; waiting_tb = 0; done_seen = 0; rst_done = 0;
        rv_cnt = 0; tcount = 0; cmd_ready = 0; reply_valid = 0; reply_ack = 0; acc_addr = 0;
        while (!done_seen && !rst_done && tcount < 5000) begin
            tcount = tcount + 1;
            // 1) 직전 클럭에 소비된 응답 반영
            if (pend) begin or_ack = or_ack | pend_val; rep = rep + 1; pend = 0; waiting_tb = 0; reply_valid = 0; reply_ack = 0; end
            // 2) 상태 점검
            if (found !== or_ack) $fatal(1, "found=%b expected %b (acc=%0d rep=%0d)", found, or_ack, acc, rep);
            if (waiting_tb && cmd_valid !== 1'b0) $fatal(1, "cmd_valid high while waiting for reply (addr=%h)", address);
            if (waiting_tb && address !== acc_addr) $fatal(1, "address moved without reply: %h expected %h", address, acc_addr);
            if (done) begin
                if (acc !== 112 || rep !== 112) $fatal(1, "done early/late acc=%0d rep=%0d", acc, rep);
                if (busy !== 1'b0 || cmd_valid !== 1'b0) $fatal(1, "done but busy=%b cmd_valid=%b", busy, cmd_valid);
                if (found !== or_ack) $fatal(1, "found at done=%b expected %b", found, or_ack);
                done_seen = 1;
                start = 0;
            end else if (busy !== 1'b1) $fatal(1, "busy dropped before done (acc=%0d rep=%0d)", acc, rep);
            if (!done_seen) begin
                // 3) 응답 지연 카운트다운(이번 클럭에서 reply_valid 를 구동하면 다음 posedge 에 waiting=1 이어야 함)
                if (rv_cnt > 0) begin
                    rv_cnt = rv_cnt - 1;
                    if (rv_cnt == 0) begin reply_valid = 1; reply_ack = ackfn(mode, acc_addr); pend = 1; pend_val = reply_ack; end
                end
                // 4) 명령 수락 결정
                cmd_ready = rnd ? ($random(seed) & 1) : 1'b1;
                if (cmd_valid && cmd_ready) begin
                    if (address !== exp_addr) $fatal(1, "scan address %h expected %h", address, exp_addr);
                    if (address > 7'h77) $fatal(1, "scan address beyond 0x77: %h", address);
                    acc_addr = address; exp_addr = exp_addr + 1'b1; acc = acc + 1; waiting_tb = 1;
                    cd = rnd ? (1 + ($random(seed) & 32'h7FFFFFFF) % 6) : 1;
                    rv_cnt = cd;
                    if (rst_at != 0 && acc == rst_at) begin rst = 1; rst_done = 1; end
                end
                start = spam ? $random(seed) : 1'b0;
            end
        end
        if (tcount >= 5000) $fatal(1, "scan timeout acc=%0d rep=%0d", acc, rep);
        if (rst_done) begin
            reply_valid = 0; cmd_ready = 0; start = 0;
            @(negedge clk); @(negedge clk);   // posedge 에서 Reset 적용
            if (busy !== 0 || cmd_valid !== 0 || found !== 0 || done !== 0 || address !== 7'h08)
                $fatal(1, "after reset busy=%b cmd_valid=%b found=%b done=%b addr=%h", busy, cmd_valid, found, done, address);
            rst = 0;
        end else begin
            @(negedge clk);
            if (done !== 1'b0 || busy !== 1'b0 || cmd_valid !== 1'b0) $fatal(1, "after done: done=%b busy=%b cmd_valid=%b", done, busy, cmd_valid);
            if (acc !== 112 || rep !== 112) $fatal(1, "scan counts acc=%0d rep=%0d", acc, rep);
        end
        cmd_ready = 0; reply_valid = 0; reply_ack = 0; start = 0;
    end
endtask

integer i;
initial begin
    repeat (2) @(negedge clk); rst = 0;
    // idle 에서 start 가 없으면 시작하지 않는다
    for (i = 0; i < 20; i = i + 1) begin
        @(negedge clk); if (busy !== 0 || cmd_valid !== 0 || done !== 0 || address !== 7'h08) $fatal(1, "idle not quiet");
    end
    do_scan(0, 0, 0, 0);      // ack 없음
    do_scan(1, 0, 0, 0);      // 0x08 만
    do_scan(2, 0, 0, 0);      // 0x77 만(마지막 주소 경계)
    do_scan(3, 0, 0, 0);      // 일부
    do_scan(4, 0, 0, 0);      // 전부
    do_scan(3, 1, 1, 0);      // 무작위 지연 + busy 중 start 반복
    do_scan(0, 1, 1, 0);
    do_scan(2, 1, 1, 0);
    do_scan(4, 1, 1, 40);     // 스캔 도중 Reset
    do_scan(1, 1, 0, 0);      // Reset 뒤 정상 재스캔
    do_scan(3, 1, 1, 111);    // 마지막 주소 직전 Reset
    do_scan(2, 0, 0, 0);
    $display("PASS i2c_scan_controller scoreboard: 112 addresses, ack OR, done pulse, start ignore, reset");
    $finish;
end
initial begin #20_000_000; $fatal(1, "test timeout"); end
endmodule

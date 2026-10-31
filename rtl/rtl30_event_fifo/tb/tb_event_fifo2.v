// RTL30 2-엔트리 이벤트 FIFO 검증(2차 보강). 임의 push/pop/Reset 을 TB 안의 2칸 큐 모델과 매 클럭 비교.
//  - in_ready=(count<2)||(pop 동시), out_valid=(count!=0), out_data=머리 데이터, 순서 보존, 포인터 wrap
//  - full 에서 push+pop 동시, empty 에서 push, Reset 중간 삽입
//  - 구간별 입력/출력 확률을 바꿔 full/empty 를 모두 많이 만든다. 커버리지 부족 시 실패.
`timescale 1ns/1ps
module tb_event_fifo2;
reg clk=0,rst=1,in_valid=0,out_ready=0;reg [7:0]in_data=0;wire in_ready,out_valid;wire [7:0]out_data;
event_fifo2 dut(.clk(clk),.rst(rst),.in_data(in_data),.in_valid(in_valid),.in_ready(in_ready),.out_data(out_data),.out_valid(out_valid),.out_ready(out_ready));
always #5 clk=~clk;
reg [7:0] q0, q1; integer cnt, seed, k, r, phase, n_push, n_pop, n_full, n_empty, n_fullboth, n_rst_nonempty;
reg e_ready, e_valid, push, pop, pin, pout;
initial begin
    seed = 7; cnt = 0; q0 = 0; q1 = 0; n_push = 0; n_pop = 0; n_full = 0; n_empty = 0; n_fullboth = 0; n_rst_nonempty = 0;
    repeat (2) @(negedge clk);        // rst=1 로 posedge 2번 -> DUT 초기화
    for (k = 0; k < 40000; k = k + 1) begin
        @(negedge clk);
        r = $random(seed); phase = (k / 400) % 4;
        rst = (r[7:0] == 8'd5);
        case (phase)
            0: begin pin = r[8];            pout = r[9]; end
            1: begin pin = r[8] | r[12];    pout = r[9] & r[13]; end
            2: begin pin = r[8] & r[12];    pout = r[9] | r[13]; end
            default: begin pin = 1'b1;      pout = 1'b1; end
        endcase
        in_valid = pin; out_ready = pout; in_data = r[23:16];
        #1;
        e_ready = (cnt < 2) || (cnt != 0 && out_ready);
        e_valid = (cnt != 0);
        if (in_ready !== e_ready) $fatal(1, "k=%0d in_ready=%b expected %b (cnt=%0d out_ready=%b)", k, in_ready, e_ready, cnt, out_ready);
        if (out_valid !== e_valid) $fatal(1, "k=%0d out_valid=%b expected %b (cnt=%0d)", k, out_valid, e_valid, cnt);
        if (cnt != 0 && out_data !== q0) $fatal(1, "k=%0d out_data=%h expected %h (cnt=%0d)", k, out_data, q0, cnt);
        if (cnt == 2) n_full = n_full + 1;
        if (cnt == 0) n_empty = n_empty + 1;
        push = in_valid && e_ready; pop = e_valid && out_ready;
        if (rst) begin if (cnt != 0) n_rst_nonempty = n_rst_nonempty + 1; cnt = 0; end
        else begin
            if (cnt == 2 && push && pop) n_fullboth = n_fullboth + 1;
            if (pop) begin q0 = q1; cnt = cnt - 1; n_pop = n_pop + 1; end
            if (push) begin if (cnt == 0) q0 = in_data; else q1 = in_data; cnt = cnt + 1; n_push = n_push + 1; end
        end
    end
    if (n_push < 3000 || n_pop < 3000 || n_full < 2000 || n_empty < 2000 || n_fullboth < 200 || n_rst_nonempty < 20)
        $fatal(1, "coverage push=%0d pop=%0d full=%0d empty=%0d fullboth=%0d rst_nonempty=%0d", n_push, n_pop, n_full, n_empty, n_fullboth, n_rst_nonempty);
    $display("PASS event_fifo2 model: order, full/empty, simultaneous push/pop, reset");
    $finish;
end
initial begin #10_000_000; $fatal(1, "timeout"); end
endmodule

`timescale 1ns/1ps
`define SIMULATION
// 엔진 단독 테스트. 소자 모델이 리셋에 응답하고, 읽기 슬롯에서 패턴 1,0,1,1 을 돌려준다.
module tb_onewire_engine;
    reg clk = 0, rstn = 0;
    always #5 clk = ~clk;   // 100 MHz
    reg start = 0; reg [1:0] op = 0; reg wbit = 0;
    wire busy, done_pulse, presence, rbit, ow_low, ow_i;
    tri1 bus;               // 풀업

    onewire_iobuf u_io (.low(ow_low), .pad(bus), .i(ow_i));
    onewire_bit_engine #(.CLK_MHZ(100)) dut (
        .clk(clk), .rstn(rstn), .start(start), .op(op), .wbit(wbit), .ow_i(ow_i),
        .ow_low(ow_low), .busy(busy), .done_pulse(done_pulse), .presence(presence), .rbit(rbit));

    // 소자 모델
    reg busy_is_reset = 0;
    reg slave_low = 0;
    assign bus = slave_low ? 1'b0 : 1'bz;
    reg [3:0] pat = 4'b1101; // 읽기 순서 1,0,1,1 (LSB 먼저)
    integer k = 0;
    time t_fall;
    always @(negedge bus) t_fall = $time;
    always @(posedge bus) begin
        if (($time - t_fall) >= 480000) begin        // 리셋으로 판단
            #30000 slave_low = 1; #120000 slave_low = 0;  // 해제 후 30 us 뒤 120 us 동안 Presence
        end
    end
    // 읽기 슬롯: 짧은 Low(<15us) 뒤 데이터가 0이면 30 us 더 당김
    always @(negedge bus) begin
        if (!slave_low && !busy_is_reset) begin
            #1000;
            if (!pat[k%4]) begin slave_low = 1; #30000 slave_low = 0; end
        end
    end

    task run(input [1:0] o, input w);
        begin
            @(posedge clk); op <= o; wbit <= w; start <= 1; busy_is_reset <= (o != 2);
            @(posedge clk); start <= 0;
            wait (done_pulse); @(posedge clk);
        end
    endtask

    integer errors = 0;
    reg [3:0] got = 0;
    integer i;
    initial begin
        #100 rstn = 1;
        run(0, 0);
        if (!presence) begin $display("FAIL presence"); errors = errors + 1; end
        for (i = 0; i < 4; i = i + 1) begin
            k = i; run(2, 0); got[i] = rbit;
        end
        if (got !== pat) begin $display("FAIL read got=%b exp=%b", got, pat); errors = errors + 1; end
        run(1, 1); run(1, 0);
        if (errors == 0) $display("PASS"); else $display("ERRORS=%0d", errors);
        $finish;
    end
endmodule

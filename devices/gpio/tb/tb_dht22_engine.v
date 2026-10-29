`timescale 1ns/1ps
`define SIMULATION
// 엔진 단독 테스트. 소자 모델이 데이터시트 서술의 파형으로 응답한다.
module tb_dht22_engine;
    reg clk = 0, rstn = 0;
    always #5 clk = ~clk;
    reg start = 0;
    wire busy, done_pulse, err, csum_ok, dht_low, dht_i;
    wire [15:0] rh, tmp; wire [7:0] chk;
    tri1 bus;

    dht22_iobuf u_io (.low(dht_low), .pad(bus), .i(dht_i));
    dht22_engine #(.CLK_MHZ(100)) dut (
        .clk(clk), .rstn(rstn), .start(start), .dht_i(dht_i), .dht_low(dht_low),
        .busy(busy), .done_pulse(done_pulse), .err(err), .csum_ok(csum_ok),
        .rh_raw(rh), .temp_raw(tmp), .chk_raw(chk));

    // 소자 모델
    reg sensor_low = 0, responding = 0, present = 1;
    assign bus = sensor_low ? 1'b0 : 1'bz;
    reg [39:0] data = {16'h0292, 16'h010B, 8'hA0};   // 습도 65.2, 온도 26.7, 합 0x02+0x92+0x01+0x0B
    integer i; time t0;
    always @(negedge bus) if (present && !sensor_low && !responding) begin
        t0 = $time;
        @(posedge bus);
        if (($time - t0) >= 1_000_000) begin
            responding = 1;
            #30_000 sensor_low = 1; #80_000 sensor_low = 0; #80_000;
            for (i = 39; i >= 0; i = i - 1) begin
                sensor_low = 1; #50_000; sensor_low = 0;
                if (data[i]) #70_000; else #27_000;
            end
            sensor_low = 1; #50_000; sensor_low = 0;
            responding = 0;
        end
    end

    integer errors = 0;
    task run;
        begin
            @(posedge clk); start <= 1; @(posedge clk); start <= 0;
            wait (done_pulse); @(posedge clk);
        end
    endtask

    initial begin
        #100 rstn = 1;
        run;
        if (err) begin $display("FAIL unexpected err"); errors = errors + 1; end
        if (rh !== 16'h0292 || tmp !== 16'h010B || chk !== 8'hA0) begin
            $display("FAIL data rh=%h t=%h chk=%h", rh, tmp, chk); errors = errors + 1; end
        if (!csum_ok) begin $display("FAIL csum_ok"); errors = errors + 1; end
        // 잘못된 체크섬
        #1_000_000 data[7:0] = 8'h00; run;
        if (csum_ok) begin $display("FAIL csum should be bad"); errors = errors + 1; end
        // 소자 없음
        #1_000_000 present = 0; run;
        if (!err) begin $display("FAIL expected err"); errors = errors + 1; end
        if (errors == 0) $display("PASS"); else $display("ERRORS=%0d", errors);
        $finish;
    end
endmodule

`timescale 1ns/1ps
// DC 모터 AXI4-Lite: 독립 AW/W, WSTRB, 읽기 전용 STATUS, B/R 백프레셔 검증.
module tb_dcmotor_axil_regs;
    reg clk = 0;
    always #5 clk = ~clk;
    reg rstn = 0;
    reg [3:0] awaddr = 0;
    reg awvalid = 0;
    wire awready;
    reg [31:0] wdata = 0;
    reg [3:0] wstrb = 0;
    reg wvalid = 0;
    wire wready;
    wire [1:0] bresp;
    wire bvalid;
    reg bready = 0;
    reg [3:0] araddr = 0;
    reg arvalid = 0;
    wire arready;
    wire [31:0] rdata;
    wire [1:0] rresp;
    wire rvalid;
    reg rready = 0;
    wire enable;
    wire [1:0] mode;
    wire [31:0] duty, period;
    reg [1:0] applied_mode = 2'd2;
    reg guard = 1'b1;
    integer errors = 0;

    dcmotor_axil_regs #(.PERIOD_DEFAULT(100000)) dut (
        .clk(clk), .rstn(rstn),
        .awaddr(awaddr), .awvalid(awvalid), .awready(awready),
        .wdata(wdata), .wstrb(wstrb), .wvalid(wvalid), .wready(wready),
        .bresp(bresp), .bvalid(bvalid), .bready(bready),
        .araddr(araddr), .arvalid(arvalid), .arready(arready),
        .rdata(rdata), .rresp(rresp), .rvalid(rvalid), .rready(rready),
        .enable(enable), .mode(mode), .duty(duty), .period(period),
        .applied_mode(applied_mode), .guard(guard)
    );

    task check;
        input condition;
        input [255:0] label;
        begin
            if (condition !== 1'b1) begin
                $display("FAIL %0s at %0t", label, $time);
                errors = errors + 1;
            end
        end
    endtask

    task send_aw;
        input [3:0] addr;
        begin
            @(negedge clk); awaddr = addr; awvalid = 1;
            #1; check(awready, "AW independent ready");
            @(posedge clk);
            @(negedge clk); awvalid = 0;
        end
    endtask

    task send_w;
        input [31:0] data;
        input [3:0] strb;
        begin
            @(negedge clk); wdata = data; wstrb = strb; wvalid = 1;
            #1; check(wready, "W independent ready");
            @(posedge clk);
            @(negedge clk); wvalid = 0;
        end
    endtask

    task complete_b;
        integer k;
        begin
            repeat (2) @(negedge clk);
            check(bvalid && bresp == 2'b00, "BVALID OKAY");
            for (k = 0; k < 3; k = k + 1) begin
                @(negedge clk);
                check(bvalid && !awready && !wready, "B backpressure stable");
            end
            bready = 1;
            @(posedge clk);
            @(negedge clk); bready = 0;
            check(!bvalid, "B consumed");
        end
    endtask

    task axi_write;
        input [3:0] addr;
        input [31:0] data;
        input [3:0] strb;
        input w_first;
        begin
            if (w_first) begin send_w(data, strb); send_aw(addr); end
            else begin send_aw(addr); send_w(data, strb); end
            complete_b();
        end
    endtask

    task axi_read;
        input [3:0] addr;
        input [31:0] expected;
        reg [31:0] snap;
        integer k;
        begin
            @(negedge clk); araddr = addr; arvalid = 1;
            #1; check(arready, "AR ready");
            @(posedge clk);
            @(negedge clk); arvalid = 0;
            snap = rdata;
            check(rvalid && rresp == 2'b00 && snap === expected, "read expected");
            for (k = 0; k < 3; k = k + 1) begin
                @(negedge clk);
                check(rvalid && rdata === snap && !arready, "R backpressure stable");
            end
            rready = 1;
            @(posedge clk);
            @(negedge clk); rready = 0;
            check(!rvalid, "R consumed");
        end
    endtask

    initial begin
        repeat (3) @(negedge clk);
        check(!bvalid && !rvalid && !enable && duty == 0, "reset");
        rstn = 1;
        repeat (3) @(negedge clk);
        check(!rvalid && arready, "no unsolicited RVALID");
        axi_read(4'h0, 32'h0);
        axi_read(4'h4, 32'h0);
        axi_read(4'h8, 32'd100000);
        axi_read(4'hc, 32'h6);

        // (추가) AW 선행 및 W 선행, CTRL·DUTY 전체 쓰기.
        axi_write(4'h0, 32'h3, 4'b0001, 0);
        check(enable && mode == 1, "CTRL AW-first");
        axi_write(4'h4, 32'h11223344, 4'b1111, 1);
        axi_read(4'h4, 32'h11223344);

        // (추가) 32비트 DUTY에서 0번/2번 바이트만 갱신한다.
        axi_write(4'h4, 32'haabbccdd, 4'b0101, 0);
        axi_read(4'h4, 32'h11bb33dd);
        axi_write(4'h4, 32'hffffffff, 4'b0000, 1);
        axi_read(4'h4, 32'h11bb33dd);

        // (추가) PERIOD 부분 바이트 쓰기, CTRL 상위 바이트 무시.
        axi_write(4'h8, 32'h55667788, 4'b1111, 0);
        axi_write(4'h8, 32'haabbccdd, 4'b0010, 1);
        axi_read(4'h8, 32'h5566cc88);
        axi_write(4'h0, 32'h0, 4'b0010, 1);
        axi_read(4'h0, 32'h3);
        axi_write(4'hc, 32'hffffffff, 4'b1111, 0);
        axi_read(4'hc, 32'h6);

        // (추가) AW/W 동시 수락 및 B 응답이 막힌 동안 R 독립 처리.
        @(negedge clk);
        awaddr = 4'h0; awvalid = 1;
        wdata = 32'h5; wstrb = 4'b0001; wvalid = 1;
        #1; check(awready && wready, "simultaneous AW/W ready");
        @(posedge clk);
        @(negedge clk); awvalid = 0; wvalid = 0;
        repeat (2) @(negedge clk);
        check(bvalid && enable && mode == 2, "simultaneous CTRL update");
        axi_read(4'h0, 32'h5);
        check(bvalid, "R works while B blocked");
        complete_b();

        // (추가) 주소만 수락하고 리셋하면 미완료 쓰기는 취소한다.
        send_aw(4'h4);
        @(negedge clk); rstn = 0;
        repeat (2) @(negedge clk);
        check(!bvalid && !rvalid && !enable && duty == 0, "reset clears pending AW");
        rstn = 1;
        axi_read(4'h8, 32'd100000);

        if (errors == 0) $display("PASS tb_dcmotor_axil_regs");
        else $fatal(1, "FAIL tb_dcmotor_axil_regs errors=%0d", errors);
        $finish;
    end
    initial begin
        #1000000;
        $fatal(1, "FAIL tb_dcmotor_axil_regs timeout");
    end
endmodule

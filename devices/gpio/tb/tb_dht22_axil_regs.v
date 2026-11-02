`timescale 1ns/1ps
// DHT22 AXI4-Lite 독립 AW/W, WSTRB, 명령 단일 펄스, IRQ, R/B 백프레셔 검증.
module tb_dht22_axil_regs;
    reg clk = 0;
    always #5 clk = ~clk;
    reg rstn = 0;
    reg [4:0] awaddr = 0;
    reg awvalid = 0;
    wire awready;
    reg [31:0] wdata = 0;
    reg [3:0] wstrb = 0;
    reg wvalid = 0;
    wire wready;
    wire [1:0] bresp;
    wire bvalid;
    reg bready = 0;
    reg [4:0] araddr = 0;
    reg arvalid = 0;
    wire arready;
    wire [31:0] rdata;
    wire [1:0] rresp;
    wire rvalid;
    reg rready = 0;
    wire cmd_start, irq;
    reg busy = 0, done_pulse = 0, err = 0, csum_ok = 1;
    reg [15:0] rh_raw = 16'd543, temp_raw = 16'h8012;
    reg [7:0] chk_raw = 8'h6d;
    integer errors = 0, starts = 0;

    dht22_axil_regs dut (
        .clk(clk), .rstn(rstn),
        .awaddr(awaddr), .awvalid(awvalid), .awready(awready),
        .wdata(wdata), .wstrb(wstrb), .wvalid(wvalid), .wready(wready),
        .bresp(bresp), .bvalid(bvalid), .bready(bready),
        .araddr(araddr), .arvalid(arvalid), .arready(arready),
        .rdata(rdata), .rresp(rresp), .rvalid(rvalid), .rready(rready),
        .cmd_start(cmd_start), .busy(busy), .done_pulse(done_pulse),
        .err(err), .csum_ok(csum_ok), .rh_raw(rh_raw), .temp_raw(temp_raw),
        .chk_raw(chk_raw), .irq(irq)
    );

    always @(posedge clk) if (rstn && cmd_start) starts = starts + 1;

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
        input [4:0] addr;
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
        input [4:0] addr;
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
        input [4:0] addr;
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
        check(!bvalid && !rvalid && !cmd_start, "reset");
        rstn = 1;
        repeat (3) @(negedge clk);
        check(!rvalid && arready, "no unsolicited RVALID");

        // AW 먼저: IRQ_EN[0] 설정, W 먼저: CMD 1회 실행.
        axi_write(5'h14, 32'h1, 4'b0001, 0);
        axi_read(5'h14, 32'h1);
        axi_write(5'h00, 32'h1, 4'b0001, 1);
        check(starts == 1, "CMD once after W first");
        axi_read(5'h04, 32'h8);
        axi_read(5'h08, 32'd543);
        axi_read(5'h0C, 32'h8012);
        axi_read(5'h10, 32'h6d);

        @(negedge clk); done_pulse = 1;
        @(negedge clk); done_pulse = 0;
        check(irq, "done IRQ set");
        axi_read(5'h04, 32'hA);

        // WSTRB=0 및 상위 바이트만 쓰면 CMD와 IRQ_EN을 변경하지 않는다.
        axi_write(5'h00, 32'h1, 4'b0000, 0);
        axi_write(5'h00, 32'h1, 4'b0010, 1);
        axi_write(5'h14, 32'h0, 4'b0010, 0);
        check(starts == 1 && irq, "WSTRB suppresses side effects");
        axi_read(5'h14, 32'h1);

        // Busy 중 명령은 B 응답만 내고 새 명령을 내지 않는다.
        @(negedge clk); busy = 1;
        axi_write(5'h00, 32'h1, 4'b0001, 1);
        check(starts == 1, "busy CMD ignored");
        axi_read(5'h04, 32'hB);
        @(negedge clk); busy = 0;

        // AW/W 동시 수락 및 B 응답 대기 중 R 채널 독립성.
        @(negedge clk);
        awaddr = 5'h00; awvalid = 1;
        wdata = 32'h1; wstrb = 4'b0001; wvalid = 1;
        #1; check(awready && wready, "simultaneous AW/W ready");
        @(posedge clk);
        @(negedge clk); awvalid = 0; wvalid = 0;
        repeat (2) @(negedge clk);
        check(bvalid && starts == 2 && !irq, "simultaneous CMD once, done cleared");
        axi_read(5'h04, 32'h8);
        check(bvalid, "R works while B blocked");
        complete_b();

        @(negedge clk); rstn = 0;
        repeat (2) @(negedge clk);
        check(!bvalid && !rvalid && !irq, "reset clears response/IRQ");
        if (errors == 0) $display("PASS tb_dht22_axil_regs");
        else $fatal(1, "FAIL tb_dht22_axil_regs errors=%0d", errors);
        $finish;
    end
    initial begin
        #1000000;
        $fatal(1, "FAIL tb_dht22_axil_regs timeout");
    end
endmodule

`timescale 1ns/1ps
// 로터리 엔코더 AXI4-Lite 독립 AW/W, WSTRB, W1C 이벤트, clear, IRQ 검증.
module tb_quad_axil_regs;
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
    reg signed [31:0] count = -12;
    reg step_pulse = 0, dir_up = 1, err_pulse = 0;
    reg btn_level = 1, btn_press_pulse = 0;
    wire clear, invert, irq;
    integer errors = 0, clears = 0;

    quad_axil_regs dut (
        .clk(clk), .rstn(rstn),
        .awaddr(awaddr), .awvalid(awvalid), .awready(awready),
        .wdata(wdata), .wstrb(wstrb), .wvalid(wvalid), .wready(wready),
        .bresp(bresp), .bvalid(bvalid), .bready(bready),
        .araddr(araddr), .arvalid(arvalid), .arready(arready),
        .rdata(rdata), .rresp(rresp), .rvalid(rvalid), .rready(rready),
        .count(count), .step_pulse(step_pulse), .dir_up(dir_up),
        .err_pulse(err_pulse), .btn_level(btn_level),
        .btn_press_pulse(btn_press_pulse), .clear(clear),
        .invert(invert), .irq(irq)
    );

    always @(posedge clk) if (rstn && clear) clears = clears + 1;

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
        check(!bvalid && !rvalid && !clear, "reset");
        rstn = 1;
        repeat (3) @(negedge clk);
        check(!rvalid && arready, "no unsolicited RVALID");
        axi_read(4'h0, 32'hfffffff4);

        // AW 먼저 CTRL, W 먼저 COUNT clear.
        axi_write(4'h8, 32'h2, 4'b0001, 0);
        axi_read(4'h8, 32'h2);
        axi_write(4'h0, 32'hffffffff, 4'b0000, 1);
        axi_write(4'h0, 32'hffffffff, 4'b0010, 0);
        check(clears == 0, "WSTRB suppresses COUNT clear");
        axi_write(4'h0, 32'h0, 4'b0001, 1);
        check(clears == 1, "COUNT clear once");

        // 이벤트 설정 후 IRQ 및 STATUS 확인.
        @(negedge clk);
        btn_press_pulse = 1; err_pulse = 1; step_pulse = 1;
        @(negedge clk);
        btn_press_pulse = 0; err_pulse = 0; step_pulse = 0;
        check(irq, "IRQ after press/step");
        axi_read(4'h4, 32'h1f);

        // W1C는 하위 바이트만 유효, 상위 바이트 무효.
        axi_write(4'h4, 32'he, 4'b0010, 0);
        axi_read(4'h4, 32'h1f);
        axi_write(4'h4, 32'he, 4'b0001, 1);
        axi_read(4'h4, 32'h11);
        check(!irq, "IRQ cleared by W1C");

        // 같은 클록 W1C와 새 이벤트가 겹치면 새 이벤트를 우선한다.
        send_aw(4'h4);
        send_w(32'h2, 4'b0001);
        @(negedge clk); btn_press_pulse = 1;
        @(negedge clk); btn_press_pulse = 0;
        complete_b();
        axi_read(4'h4, 32'h13);
        check(irq, "new press survives same-cycle W1C");

        // WSTRB 상위 바이트로 CTRL 변경 금지.
        axi_write(4'h8, 32'h0, 4'b0010, 0);
        axi_read(4'h8, 32'h2);

        // AW/W 동시 수락, 읽기와 B 응답 독립성.
        @(negedge clk);
        awaddr = 4'h8; awvalid = 1;
        wdata = 32'h3; wstrb = 4'b0001; wvalid = 1;
        #1; check(awready && wready, "simultaneous AW/W ready");
        @(posedge clk);
        @(negedge clk); awvalid = 0; wvalid = 0;
        repeat (2) @(negedge clk);
        check(bvalid && invert && irq, "simultaneous CTRL update");
        axi_read(4'h8, 32'h3);
        check(bvalid, "R works while B blocked");
        complete_b();

        @(negedge clk); rstn = 0;
        repeat (2) @(negedge clk);
        check(!bvalid && !rvalid && !irq && !invert, "reset clears responses and CTRL");
        if (errors == 0) $display("PASS tb_quad_axil_regs");
        else $fatal(1, "FAIL tb_quad_axil_regs errors=%0d", errors);
        $finish;
    end
    initial begin
        #1000000;
        $fatal(1, "FAIL tb_quad_axil_regs timeout");
    end
endmodule

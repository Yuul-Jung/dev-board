`timescale 1ns/1ps
// 초음파 AXI4-Lite 레지스터 독립 채널/WSTRB/응답 백프레셔 회귀 테스트.
module tb_us_axil_regs;
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

    wire core_reset, core_enable, irq;
    reg busy = 0, valid = 0, timeout_pulse = 0;
    reg [9:0] dist_cm = 10'd237;
    reg [11:0] dist_bcd = 12'h237;
    integer errors = 0;

    us_axil_regs dut (
        .clk(clk), .rstn(rstn),
        .awaddr(awaddr), .awvalid(awvalid), .awready(awready),
        .wdata(wdata), .wstrb(wstrb), .wvalid(wvalid), .wready(wready),
        .bresp(bresp), .bvalid(bvalid), .bready(bready),
        .araddr(araddr), .arvalid(arvalid), .arready(arready),
        .rdata(rdata), .rresp(rresp), .rvalid(rvalid), .rready(rready),
        .core_reset(core_reset), .core_enable(core_enable),
        .busy(busy), .valid(valid), .timeout_pulse(timeout_pulse),
        .dist_cm(dist_cm), .dist_bcd(dist_bcd), .irq(irq)
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
        input [4:0] addr;
        begin
            @(negedge clk);
            awaddr = addr;
            awvalid = 1;
            #1;
            check(awready, "AW independently ready");
            @(posedge clk);
            @(negedge clk);
            awvalid = 0;
        end
    endtask

    task send_w;
        input [31:0] data;
        input [3:0] strb;
        begin
            @(negedge clk);
            wdata = data;
            wstrb = strb;
            wvalid = 1;
            #1;
            check(wready, "W independently ready");
            @(posedge clk);
            @(negedge clk);
            wvalid = 0;
        end
    endtask

    task complete_b;
        integer n;
        begin
            n = 0;
            while (!bvalid && n < 20) begin
                @(negedge clk);
                n = n + 1;
            end
            check(bvalid && bresp == 2'b00, "write response OKAY");
            // BREADY를 늦춰도 BVALID가 유지되고 중복 쓰기를 수락하지 않는다.
            repeat (3) begin
                @(negedge clk);
                check(bvalid && !awready && !wready, "B channel backpressure");
            end
            bready = 1;
            @(posedge clk);
            @(negedge clk);
            bready = 0;
            check(!bvalid, "write response consumed");
        end
    endtask

    task axi_write;
        input [4:0] addr;
        input [31:0] data;
        input [3:0] strb;
        input data_first;
        begin
            if (data_first) begin
                send_w(data, strb);
                repeat (2) @(negedge clk);
                check(!bvalid, "W only must not commit");
                send_aw(addr);
            end else begin
                send_aw(addr);
                repeat (2) @(negedge clk);
                check(!bvalid, "AW only must not commit");
                send_w(data, strb);
            end
            complete_b();
        end
    endtask

    task axi_read;
        input [4:0] addr;
        input [31:0] expected;
        reg [31:0] snapshot;
        begin
            @(negedge clk);
            araddr = addr;
            arvalid = 1;
            #1;
            check(arready, "read address ready");
            @(posedge clk);
            @(negedge clk);
            arvalid = 0;
            snapshot = rdata;
            check(rvalid && rresp == 2'b00 && snapshot === expected, "read value");
            repeat (3) begin
                @(negedge clk);
                check(rvalid && rdata === snapshot && !arready, "R channel stable");
            end
            rready = 1;
            @(posedge clk);
            @(negedge clk);
            rready = 0;
            check(!rvalid, "read response consumed");
        end
    endtask

    initial begin
        repeat (3) @(negedge clk);
        check(core_reset && !bvalid && !rvalid, "reset state");
        rstn = 1;

        // AW가 먼저 도착: 0x00 CTRL[0] 쓰기.
        axi_write(5'h00, 32'h00000001, 4'b0001, 0);
        check(core_enable, "CTRL enable via AW first");
        axi_read(5'h00, 32'h00000001);

        // W가 먼저 도착: 0x10 IRQ_EN[1:0] 쓰기.
        axi_write(5'h10, 32'h00000003, 4'b0001, 1);
        axi_read(5'h10, 32'h00000003);

        // WSTRB=0과 상위 바이트 쓰기는 CTRL[0]을 바꾸면 안 된다.
        axi_write(5'h00, 32'h00000000, 4'b0000, 0);
        check(core_enable, "zero WSTRB ignored");
        axi_write(5'h00, 32'h00000000, 4'b0010, 1);
        check(core_enable, "upper byte WSTRB ignored");

        // 센서 완료/타임아웃 플래그와 인터럽트.
        @(negedge clk); valid = 1; busy = 1;
        repeat (2) @(negedge clk);
        busy = 0;
        repeat (2) @(negedge clk);
        timeout_pulse = 1;
        @(negedge clk); timeout_pulse = 0;
        repeat (2) @(negedge clk);
        check(irq, "IRQ set");
        axi_read(5'h04, 32'h0000000E);
        axi_read(5'h08, 32'd237);
        axi_read(5'h0C, 32'h00000237);

        // W1C는 WSTRB[0]이 없으면 실행되지 않는다.
        axi_write(5'h04, 32'h0000000C, 4'b0000, 0);
        check(irq, "W1C zero strobe ignored");
        axi_write(5'h04, 32'h0000000C, 4'b0001, 1);
        check(!irq, "W1C cleared flags");
        axi_read(5'h04, 32'h00000002);

        // 같은 클록의 AW/W 수락과 BVALID 중 읽기 병행.
        @(negedge clk);
        awaddr = 5'h00; awvalid = 1;
        wdata = 32'h00000000; wstrb = 4'b0001; wvalid = 1;
        @(posedge clk);
        @(negedge clk);
        awvalid = 0; wvalid = 0;
        repeat (2) @(negedge clk);
        check(bvalid && !core_enable, "simultaneous AW/W commit");
        axi_read(5'h00, 32'd0);
        check(bvalid, "read independent of B backpressure");
        complete_b();

        if (errors == 0) $display("PASS tb_us_axil_regs");
        else $fatal(1, "FAIL tb_us_axil_regs errors=%0d", errors);
        $finish;
    end

    initial begin
        #1000000;
        $fatal(1, "FAIL tb_us_axil_regs global timeout");
    end
endmodule

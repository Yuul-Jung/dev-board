`timescale 1ns/1ps
// 서보 AXI4-Lite: AW/W 독립 수락, WSTRB 부분 쓰기, 읽기 전용 APPLIED, B/R 백프레셔.
module tb_servo_axil_regs;
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
    wire enable;
    wire [15:0] width_us, period_us, min_us, max_us;
    reg [15:0] applied_us = 16'h0555;
    integer errors = 0;

    servo_axil_regs dut (
        .clk(clk), .rstn(rstn),
        .awaddr(awaddr), .awvalid(awvalid), .awready(awready),
        .wdata(wdata), .wstrb(wstrb), .wvalid(wvalid), .wready(wready),
        .bresp(bresp), .bvalid(bvalid), .bready(bready),
        .araddr(araddr), .arvalid(arvalid), .arready(arready),
        .rdata(rdata), .rresp(rresp), .rvalid(rvalid), .rready(rready),
        .enable(enable), .width_us(width_us), .period_us(period_us),
        .min_us(min_us), .max_us(max_us), .applied_us(applied_us)
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
                check(bvalid && !awready && !wready, "B backpressure");
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
                check(rvalid && rdata === snap && !arready, "R backpressure");
            end
            rready = 1;
            @(posedge clk);
            @(negedge clk); rready = 0;
            check(!rvalid, "R consumed");
        end
    endtask

    initial begin
        repeat (3) @(negedge clk);
        check(!bvalid && !rvalid && !enable, "reset response");
        check(width_us == 1500 && period_us == 20000, "reset width/period");
        check(min_us == 1000 && max_us == 2000, "reset limits");
        rstn = 1;
        repeat (3) @(negedge clk);
        check(!rvalid && arready, "no unsolicited RVALID");

        axi_read(5'h00, 32'd0);
        axi_read(5'h04, 32'd1500);
        axi_read(5'h08, 32'd20000);
        axi_read(5'h0c, 32'd1000);
        axi_read(5'h10, 32'd2000);
        axi_read(5'h14, 32'h555);

        // (추가) 주소 선행과 데이터 선행, 하위 2바이트 전체 쓰기.
        axi_write(5'h00, 32'h1, 4'b0001, 0);
        check(enable, "CTRL AW first");
        axi_write(5'h04, 32'h1234, 4'b0011, 1);
        axi_read(5'h04, 32'h1234);

        // (추가) 상위 바이트만 바꾸고, 상위 2바이트/0 스트로브는 무시한다.
        axi_write(5'h04, 32'habcd, 4'b0010, 0);
        axi_read(5'h04, 32'hab34);
        axi_write(5'h04, 32'hffff0000, 4'b1100, 1);
        axi_write(5'h04, 32'hffff, 4'b0000, 0);
        axi_read(5'h04, 32'hab34);

        // (추가) PERIOD/MIN/MAX도 부분 바이트 쓰기, APPLIED는 읽기 전용.
        axi_write(5'h08, 32'h4321, 4'b0011, 0);
        axi_write(5'h08, 32'haa00, 4'b0010, 1);
        axi_read(5'h08, 32'haa21);
        axi_write(5'h0c, 32'h0078, 4'b0001, 0);
        axi_read(5'h0c, 32'h0378);
        axi_write(5'h10, 32'h0a00, 4'b0010, 1);
        axi_read(5'h10, 32'h0ad0);
        axi_write(5'h14, 32'h0, 4'b1111, 0);
        axi_read(5'h14, 32'h0555);
        axi_write(5'h00, 32'h0, 4'b0010, 1);
        axi_read(5'h00, 32'h1);

        // (추가) AW/W 동시 수락, B 대기 중 R 채널 독립 동작.
        @(negedge clk);
        awaddr = 5'h00; awvalid = 1;
        wdata = 32'h0; wstrb = 4'b0001; wvalid = 1;
        #1; check(awready && wready, "simultaneous AW/W");
        @(posedge clk);
        @(negedge clk); awvalid = 0; wvalid = 0;
        repeat (2) @(negedge clk);
        check(bvalid && !enable, "simultaneous CTRL");
        axi_read(5'h00, 32'h0);
        check(bvalid, "read while B blocked");
        complete_b();

        // (추가) W만 수락하고 리셋하면 미완료 트랜잭션을 취소한다.
        send_w(32'hffff, 4'b0011);
        @(negedge clk); rstn = 0;
        repeat (2) @(negedge clk);
        check(!bvalid && !rvalid && !enable, "reset clears pending W");
        check(width_us == 1500 && period_us == 20000, "reset restores defaults");
        rstn = 1;
        axi_read(5'h04, 32'd1500);

        if (errors == 0) $display("PASS tb_servo_axil_regs");
        else $fatal(1, "FAIL tb_servo_axil_regs errors=%0d", errors);
        $finish;
    end
    initial begin
        #1000000;
        $fatal(1, "FAIL tb_servo_axil_regs timeout");
    end
endmodule

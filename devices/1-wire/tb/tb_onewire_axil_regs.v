`timescale 1ns/1ps
// 1-Wire AXI4-Lite 독립 AW/W, WSTRB, CMD 단발 펄스, IRQ, B/R 백프레셔 검증.
module tb_onewire_axil_regs;
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

    wire cmd_start, cmd_wbit, irq;
    wire [1:0] cmd_op;
    reg busy = 0, done_pulse = 0, presence = 1, rbit = 1;
    integer errors = 0;
    integer starts = 0;
    reg [1:0] last_op = 0;
    reg last_wbit = 0;

    onewire_axil_regs dut (
        .clk(clk), .rstn(rstn),
        .awaddr(awaddr), .awvalid(awvalid), .awready(awready),
        .wdata(wdata), .wstrb(wstrb), .wvalid(wvalid), .wready(wready),
        .bresp(bresp), .bvalid(bvalid), .bready(bready),
        .araddr(araddr), .arvalid(arvalid), .arready(arready),
        .rdata(rdata), .rresp(rresp), .rvalid(rvalid), .rready(rready),
        .cmd_start(cmd_start), .cmd_op(cmd_op), .cmd_wbit(cmd_wbit),
        .busy(busy), .done_pulse(done_pulse), .presence(presence),
        .rbit(rbit), .irq(irq)
    );

    always @(posedge clk) begin
        if (rstn && cmd_start) begin
            starts = starts + 1;
            last_op = cmd_op;
            last_wbit = cmd_wbit;
        end
    end

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
            @(negedge clk);
            awaddr = addr; awvalid = 1;
            #1; check(awready, "AW independently ready");
            @(posedge clk);
            @(negedge clk); awvalid = 0;
        end
    endtask

    task send_w;
        input [31:0] data;
        input [3:0] strb;
        begin
            @(negedge clk);
            wdata = data; wstrb = strb; wvalid = 1;
            #1; check(wready, "W independently ready");
            @(posedge clk);
            @(negedge clk); wvalid = 0;
        end
    endtask

    task complete_b;
        integer n;
        begin
            n = 0;
            while (!bvalid && n < 20) begin
                @(negedge clk); n = n + 1;
            end
            check(bvalid && bresp == 2'b00, "B response OKAY");
            repeat (3) begin
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
        input [3:0] addr;
        input [31:0] data;
        input [3:0] strb;
        input data_first;
        begin
            if (data_first) begin
                send_w(data, strb);
                repeat (2) @(negedge clk);
                check(!bvalid, "W alone no response");
                send_aw(addr);
            end else begin
                send_aw(addr);
                repeat (2) @(negedge clk);
                check(!bvalid, "AW alone no response");
                send_w(data, strb);
            end
            complete_b();
        end
    endtask

    task axi_read;
        input [3:0] addr;
        input [31:0] expected;
        reg [31:0] snap;
        begin
            @(negedge clk);
            araddr = addr; arvalid = 1;
            #1; check(arready, "AR ready");
            @(posedge clk);
            @(negedge clk); arvalid = 0;
            snap = rdata;
            check(rvalid && rresp == 2'b00 && snap === expected, "read value");
            repeat (3) begin
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
        check(!bvalid && !rvalid && !cmd_start, "reset state");
        rstn = 1;

        // AW 먼저: CMD 0x05 → 쓰기(1), 데이터 비트 1.
        axi_write(4'h0, 32'h00000005, 4'b0001, 0);
        check(starts == 1 && last_op == 2'd1 && last_wbit, "AW first CMD once");
        axi_read(4'h4, 32'h0000000C); // presence=1, rbit=1

        // W 먼저: IRQ_EN[0] 설정.
        axi_write(4'h8, 32'h00000001, 4'b0001, 1);
        axi_read(4'h8, 32'h00000001);

        // 완료 이벤트는 STATUS.done과 IRQ에 반영.
        @(negedge clk); done_pulse = 1;
        @(negedge clk); done_pulse = 0;
        check(irq, "done interrupt");
        axi_read(4'h4, 32'h0000000E);

        // WSTRB=0 또는 상위 바이트만 선택하면 CMD 실행·IRQ 갱신 금지.
        axi_write(4'h0, 32'h00000002, 4'b0000, 0);
        axi_write(4'h0, 32'h00000002, 4'b0010, 1);
        axi_write(4'h8, 32'h00000000, 4'b0010, 0);
        check(starts == 1 && irq, "WSTRB suppresses side effects");
        axi_read(4'h8, 32'h00000001);

        // busy 중 CMD는 응답하지만 엔진 start는 생성하지 않는다.
        @(negedge clk); busy = 1;
        axi_write(4'h0, 32'h00000002, 4'b0001, 1);
        check(starts == 1, "busy CMD ignored");
        axi_read(4'h4, 32'h0000000F);
        @(negedge clk); busy = 0;

        // 같은 클록 AW/W, 읽기와 B 응답 독립성.
        @(negedge clk);
        awaddr = 4'h0; awvalid = 1;
        wdata = 32'h00000002; wstrb = 4'b0001; wvalid = 1;
        #1; check(awready && wready, "simultaneous AW/W accepted");
        @(posedge clk);
        @(negedge clk); awvalid = 0; wvalid = 0;
        repeat (2) @(negedge clk);
        check(bvalid && starts == 2 && last_op == 2'd2 && !last_wbit,
              "simultaneous CMD once");
        axi_read(4'h4, 32'h0000000C); // 새 CMD로 done 지움
        check(bvalid && !irq, "R independent during B backpressure");
        complete_b();

        // 리셋 후 쓰기·IRQ 상태 초기화.
        @(negedge clk); rstn = 0;
        repeat (2) @(negedge clk);
        check(!bvalid && !rvalid && !irq, "reset clears responses and IRQ");

        if (errors == 0) $display("PASS tb_onewire_axil_regs");
        else $fatal(1, "FAIL tb_onewire_axil_regs errors=%0d", errors);
        $finish;
    end

    initial begin
        #1000000;
        $fatal(1, "FAIL tb_onewire_axil_regs global timeout");
    end
endmodule

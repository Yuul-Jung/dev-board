// 독립 UART Byte Stream Stimulus → DUT → Expected Result 검증.
`timescale 1ns/1ps
module tb_uart_led_controller;
    reg clk = 1'b0;
    reg rst = 1'b1;
    reg [7:0] rx_data = 8'd0;
    reg rx_valid = 1'b0;
    wire rx_ready;
    wire led;

    always #5 clk = ~clk;

    uart_led_controller dut (
        .clk(clk), .rst(rst),
        .rx_data(rx_data), .rx_valid(rx_valid),
        .rx_ready(rx_ready), .led(led)
    );

    task check_led(input expected);
        begin
            #1;
            if (led !== expected)
                $fatal(1, "LED mismatch expected=%b actual=%b", expected, led);
        end
    endtask

    task send_byte(input [7:0] value);
        begin
            @(negedge clk);
            rx_data = value;
            rx_valid = 1'b1;
            if (rx_ready !== 1'b1) $fatal(1, "rx_ready low");
            @(posedge clk);
            #1;
            @(negedge clk);
            rx_valid = 1'b0;
        end
    endtask

    initial begin
        repeat (2) @(negedge clk);
        check_led(1'b0);
        rst = 1'b0;
        send_byte(8'h31); // '1'
        check_led(1'b1);
        send_byte(8'h58); // 'X': 무시
        check_led(1'b1);
        send_byte(8'h30); // '0'
        check_led(1'b0);
        // valid=0일 때 data만 바뀌어도 LED 유지.
        @(negedge clk);
        rx_data = 8'h31;
        repeat (3) @(posedge clk);
        check_led(1'b0);
        send_byte(8'h31);
        check_led(1'b1);
        // 동기 Reset이 LED를 지우는지 검사.
        @(negedge clk);
        rst = 1'b1;
        @(posedge clk);
        check_led(1'b0);
        @(negedge clk);
        rst = 1'b0;
        $display("PASS uart_led_controller");
        $finish;
    end
endmodule

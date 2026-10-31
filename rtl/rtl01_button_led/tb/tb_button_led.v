// 교육용 시뮬레이션: 작은 카운트 값으로 디바운스/한 번 누름 검증
`timescale 1ns/1ps
module tb_button_led;
    reg clk=0, rst=1, btn=1;
    wire press_pulse, led;
    always #5 clk=~clk;

    button_event #(.CLK_HZ(1000), .DEBOUNCE_MS(3), .ACTIVE_LOW(1))
        u_event (.clk(clk), .rst(rst), .button_in(btn), .press_pulse(press_pulse));
    led_toggle_controller u_control (.clk(clk), .rst(rst),
                                      .press_pulse(press_pulse), .led(led));
    integer events=0;
    always @(posedge clk)
        if (!rst && press_pulse) events <= events+1;

    task run_clocks(input integer n);
        integer i;
        begin for(i=0;i<n;i=i+1) @(negedge clk); end
    endtask
    initial begin
        run_clocks(3); rst=0;
        btn=0; run_clocks(9);
        if (led !== 1'b1 || events !== 1) $fatal(1,"first press");
        btn=1; run_clocks(9);
        if (led !== 1'b1 || events !== 1) $fatal(1,"release toggled");
        btn=0; run_clocks(9);
        if (led !== 1'b0 || events !== 2) $fatal(1,"second press");
        $display("PASS: two debounced presses");
        $finish;
    end
endmodule

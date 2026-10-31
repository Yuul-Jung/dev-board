// Reset, 단일 버튼, 두 버튼, 해제 및 바운스 무시를 검증한다.
`timescale 1ns/1ps
module tb_rgb_buttons;
    reg clk=0,rst=1,btn_a=0,btn_b=0;
    wire a,b,r,g,bl;
    always #5 clk=~clk;
    button_level #(.CLK_HZ(1000),.DEBOUNCE_MS(3)) u_a(
        .clk(clk),.rst(rst),.button_in(btn_a),.pressed(a));
    button_level #(.CLK_HZ(1000),.DEBOUNCE_MS(3)) u_b(
        .clk(clk),.rst(rst),.button_in(btn_b),.pressed(b));
    rgb_button_controller u_rgb(
        .pressed_a(a),.pressed_b(b),.led_r(r),.led_g(g),.led_b(bl));
    task clocks(input integer n);
        integer i;
        begin for(i=0;i<n;i=i+1) @(negedge clk); end
    endtask
    task check(input [2:0] expected_color);
        begin
            #1;
            if ({r,g,bl} !== expected_color)
                $fatal(1,"Expected RGB %b got %b",expected_color,{r,g,bl});
        end
    endtask
    initial begin
        clocks(3);rst=0;clocks(4);check(3'b000);
        btn_a=1;clocks(8);check(3'b100);
        btn_b=1;clocks(8);check(3'b001);
        btn_a=0;clocks(8);check(3'b010);
        btn_b=0;clocks(8);check(3'b000);
        btn_a=1;clocks(1);btn_a=0;clocks(8);check(3'b000);
        $display("PASS: RGB button levels and short bounce rejection");
        $finish;
    end
endmodule

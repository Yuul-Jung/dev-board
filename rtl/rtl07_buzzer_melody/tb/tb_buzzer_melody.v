`timescale 1ns/1ps
// 작은 파라미터로 3음 순환, disable, 출력 토글 검증.
module tb_buzzer_melody;
    reg clk=0,rst=1,enabled=0;
    wire [31:0] hp;
    wire change,buzzer;
    integer changes=0,toggles=0;
    reg prev_buzzer=0;
    always #5 clk=~clk;
    melody_sequencer #(.CLK_HZ(10000),.NOTE_CLOCKS(30)) u_seq(
        .clk(clk),.rst(rst),.enabled(enabled),
        .half_period_clks(hp),.note_change(change));
    tone_generator u_tone(
        .clk(clk),.rst(rst),.tone_enable(enabled),
        .note_change(change),.half_period_clks(hp),.buzzer(buzzer));
    always @(negedge clk) begin
        if (!rst && enabled && change) changes=changes+1;
        if (!rst && enabled && buzzer != prev_buzzer) toggles=toggles+1;
        prev_buzzer=buzzer;
    end
    initial begin
        repeat(3) @(negedge clk);
        rst=0; enabled=1;
        repeat(95) @(negedge clk);
        if(changes != 3) $fatal(1,"Expected 3 transitions, got %0d",changes);
        if(toggles < 3) $fatal(1,"Missing output toggles");
        enabled=0;
        repeat(3) @(negedge clk);
        if(buzzer !== 0) $fatal(1,"Buzzer not muted");
        if(hp !== 10000/(2*440)) $fatal(1,"Sequence did not reset");
        $display("PASS buzzer melody");
        $finish;
    end
endmodule

// Shell/Wrapper: 외부 스위치 동기화 및 재사용 모듈 간 연결만 수행.
// 외부 수동 피에조 모듈의 전압/전류/드라이버 회로는 별도 확인 필요.
module basys3_buzzer_top (
    input wire clk,
    input wire rst,
    input wire sw_play,
    output wire buzzer
);
    wire enabled, note_change;
    wire [31:0] half_period_clks;
    switch_sync u_sync(.clk(clk), .rst(rst), .sw_in(sw_play), .sw_out(enabled));
    melody_sequencer u_melody(
        .clk(clk), .rst(rst), .enabled(enabled),
        .half_period_clks(half_period_clks), .note_change(note_change));
    tone_generator u_tone(
        .clk(clk), .rst(rst), .tone_enable(enabled),
        .note_change(note_change),
        .half_period_clks(half_period_clks), .buzzer(buzzer));
endmodule

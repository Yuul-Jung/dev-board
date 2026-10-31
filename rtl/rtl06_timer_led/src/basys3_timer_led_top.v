// Top Shell/Wrapper: module 인스턴스와 신호 연결만.
// sw_en 외부 비동기 입력은 기존 버튼과 달리 별도 동기화 블록을 거침.
module basys3_timer_led_top(
    input wire clk,
    input wire rst,
    input wire sw_en,
    output wire led
);
    wire enabled;
    wire tick;

    // 기존 rtl04의 8비트 전용 동기화기를 가져오기보다 1비트 재사용 모듈 사용.
    switch_sync u_switch_sync(
        .clk(clk), .rst(rst), .sw_in(sw_en), .sw_out(enabled)
    );
    tick_generator #(.CLOCKS_PER_TICK(50_000_000)) u_tick_generator(
        .clk(clk), .rst(rst), .tick(tick)
    );
    timer_led_controller u_timer_led_controller(
        .clk(clk), .rst(rst), .enabled(enabled),
        .tick(tick), .led(led)
    );
endmodule

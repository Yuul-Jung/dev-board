// Shell/Wrapper: 변환된 ADC 샘플 스트림과 PWM 코어를 연결.
// XADC Wizard의 실제 출력 포트/샘플 채널 설정은 별도 어댑터에서 확인.
// 실제 Basys3 보드 외부 핀 연결을 포함하지 않은 재사용 인터페이스 Top.
module xadc_pwm_stream_top #(
    parameter integer PWM_CLK_DIV = 390
)(
    input  wire        clk,
    input  wire        rst,
    input  wire [11:0] sample_data,
    input  wire        sample_valid,
    output wire        led_pwm
);
    wire [8:0] duty;
    xadc_duty_controller u_xadc_duty_controller (
        .clk(clk), .rst(rst),
        .sample_data(sample_data),
        .sample_valid(sample_valid),
        .duty(duty)
    );
    pwm_core #(.CLK_DIV(PWM_CLK_DIV)) u_pwm_core (
        .clk(clk), .rst(rst), .duty_in(duty), .pwm_out(led_pwm)
    );
endmodule

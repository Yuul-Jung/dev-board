// 12-bit ADC 샘플을 PWM용 9-bit Duty(0..256)로 변환.
// 입력은 이미 clk 도메인에 동기화된 sample_valid/sample_data여야 한다.
// (XADC의 특정 Vivado IP 포트명과 직접 결합하지 않은 재사용 Controller)
module xadc_duty_controller (
    input  wire        clk,
    input  wire        rst,
    input  wire [11:0] sample_data,
    input  wire        sample_valid,
    output reg  [8:0]  duty
);
    always @(posedge clk) begin
        if (rst) begin
            duty <= 9'd0;
        end else if (sample_valid) begin
            // (변환) 최대 ADC 코드 4095는 100%인 256으로 포화 매핑
            // 일반 구간: ADC[11:4]를 하위 8비트 Duty로 사용
            duty <= (sample_data == 12'hFFF) ?
                    9'd256 : {1'b0, sample_data[11:4]};
        end
    end
endmodule

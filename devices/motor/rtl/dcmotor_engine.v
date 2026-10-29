// DC 모터 구동 엔진: PWM 1개 + 방향 2개. L298(ENA, IN1, IN2)과 TB6612FNG(PWM, IN1, IN2) 둘 다 이 출력으로 구동한다.
// mode: 0=정지(코스트), 1=정회전, 2=역회전, 3=브레이크
// 정회전<->역회전 전환은 DEAD_US 동안 코스트를 거친다(설계 선택: 드라이버 문서 값이 아님).
module dcmotor_engine #(
    parameter CLK_MHZ = 100,
    parameter DEAD_US = 1000
) (
    input  wire        clk,
    input  wire        rstn,
    input  wire        enable,
    input  wire [1:0]  mode,
    input  wire [15:0] duty,        // 0..period. period 이상이면 항상 켜짐
    input  wire [31:0] period,      // 클럭 수
    output reg         pwm,
    output reg         in1,
    output reg         in2,
    output wire [1:0]  applied_mode,
    output wire        guard        // 방향 전환 대기 중
);
    localparam integer DEAD_CLK = CLK_MHZ * DEAD_US;

    reg [31:0] cnt;
    reg [1:0]  cur;
    reg [31:0] dead;

    wire [1:0]  want = enable ? mode : 2'd0;
    wire [31:0] per  = (period == 32'd0) ? 32'd1 : period;
    wire        pwm_core = (cnt < {16'd0, duty});

    assign applied_mode = cur;
    assign guard = (dead != 32'd0);

    always @(posedge clk) begin
        if (!rstn) begin
            cnt <= 32'd0; cur <= 2'd0; dead <= 32'd0;
            pwm <= 1'b0; in1 <= 1'b0; in2 <= 1'b0;
        end else begin
            cnt <= (cnt >= per - 1) ? 32'd0 : cnt + 32'd1;

            if (dead != 32'd0) begin
                dead <= dead - 32'd1;
                if (dead == 32'd1) cur <= want;
            end else if (want != cur) begin
                if ((cur == 2'd1 && want == 2'd2) || (cur == 2'd2 && want == 2'd1)) begin
                    cur <= 2'd0; dead <= DEAD_CLK;
                end else begin
                    cur <= want;
                end
            end

            case (cur)
                2'd1:    begin in1 <= 1'b1; in2 <= 1'b0; pwm <= pwm_core; end
                2'd2:    begin in1 <= 1'b0; in2 <= 1'b1; pwm <= pwm_core; end
                2'd3:    begin in1 <= 1'b1; in2 <= 1'b1; pwm <= 1'b1;     end
                default: begin in1 <= 1'b0; in2 <= 1'b0; pwm <= 1'b0;     end
            endcase
        end
    end
endmodule

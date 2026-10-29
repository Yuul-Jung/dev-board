// 로터리 엔코더(직교 2상 A/B) 디코더 + 버튼. 입력 3개 모두 동기 + 안정 필터.
// 방향 규약(설계안): A 가 B 보다 앞서는 순서 00->10->11->01->00 을 +1 로 센다. 실제 엔코더의 시계 방향과의
// 대응은 문서에 없어(미확인) INVERT 로 뒤집는다. 불가능한 전이(두 비트 동시 변화)는 err 펄스만 내고 세지 않는다.
module quad_engine #(
    parameter CLK_MHZ      = 100,
    parameter DEBOUNCE_US  = 3000,    // 설계안: ALPS 문서의 chatter 3 ms max 이상
    parameter BTN_ACTIVE_LOW = 1      // 설계안: 풀업 + 접지 스위치 가정
) (
    input  wire        clk,
    input  wire        rstn,
    input  wire        a_i,
    input  wire        b_i,
    input  wire        sw_i,
    input  wire        invert,
    input  wire        clear,          // 1클럭: 카운트 0
    output reg  signed [31:0] count,
    output reg         step_pulse,     // 카운트가 바뀔 때 1클럭
    output reg         dir_up,         // 마지막 이동 방향
    output reg         err_pulse,
    output wire        btn_level,      // 눌림=1 (디바운스 후)
    output reg         btn_press_pulse
);
    localparam [31:0] LIM = DEBOUNCE_US * CLK_MHZ;

    reg [1:0] sa, sb, ss;
    reg       fa, fb, fs;
    reg [31:0] ca, cb, cs;

    always @(posedge clk) begin
        if (!rstn) begin
            sa <= 2'b11; sb <= 2'b11; ss <= 2'b11; fa <= 1'b1; fb <= 1'b1; fs <= 1'b1;
            ca <= 0; cb <= 0; cs <= 0;
        end else begin
            sa <= {sa[0], a_i}; sb <= {sb[0], b_i}; ss <= {ss[0], sw_i};
            if (sa[1] != fa) begin if (ca >= LIM - 1) begin fa <= sa[1]; ca <= 0; end else ca <= ca + 1; end else ca <= 0;
            if (sb[1] != fb) begin if (cb >= LIM - 1) begin fb <= sb[1]; cb <= 0; end else cb <= cb + 1; end else cb <= 0;
            if (ss[1] != fs) begin if (cs >= LIM - 1) begin fs <= ss[1]; cs <= 0; end else cs <= cs + 1; end else cs <= 0;
        end
    end

    assign btn_level = BTN_ACTIVE_LOW ? ~fs : fs;

    reg [1:0] prev;
    reg       btn_prev;
    wire [3:0] tr = {prev, fa, fb};   // {prev A, prev B, A, B}
    reg  inc, dec, bad;
    always @* begin
        inc = 1'b0; dec = 1'b0; bad = 1'b0;
        case (tr)
            4'b00_10, 4'b10_11, 4'b11_01, 4'b01_00: inc = 1'b1;
            4'b10_00, 4'b11_10, 4'b01_11, 4'b00_01: dec = 1'b1;
            4'b00_11, 4'b11_00, 4'b01_10, 4'b10_01: bad = 1'b1;
            default: ;
        endcase
    end

    always @(posedge clk) begin
        step_pulse <= 1'b0; err_pulse <= 1'b0; btn_press_pulse <= 1'b0;
        if (!rstn) begin
            prev <= 2'b11; count <= 32'sd0; dir_up <= 1'b0; btn_prev <= 1'b0;
        end else begin
            prev <= {fa, fb};
            btn_prev <= btn_level;
            if (btn_level && !btn_prev) btn_press_pulse <= 1'b1;
            if (clear) count <= 32'sd0;
            else if (inc ^ dec) begin
                step_pulse <= 1'b1;
                if (inc ^ invert) begin count <= count + 32'sd1; dir_up <= 1'b1; end
                else              begin count <= count - 32'sd1; dir_up <= 1'b0; end
            end
            if (bad) err_pulse <= 1'b1;
        end
    end
endmodule

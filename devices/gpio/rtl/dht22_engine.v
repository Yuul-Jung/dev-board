// DHT22(AM2302) 단일선 읽기 엔진.
// 시간(µs)은 데이터시트 서술에서 가져왔다: 시작 Low 1 ms 이상, 해제 후 응답 대기 20-40, 응답 Low 80 / High 80,
// 비트 = Low 50 + High(0이면 26-28, 1이면 70).
// 비트 판정은 High가 시작된 뒤 40 µs 시점의 선 레벨로 한다(28과 70 사이). 마지막 비트도 같은 방식이라 종료 신호에 의존하지 않는다.
module dht22_engine #(
    parameter CLK_MHZ = 100
) (
    input  wire        clk,
    input  wire        rstn,
    input  wire        start,       // 1클럭 펄스. busy 중에는 무시
    input  wire        dht_i,       // 버스 입력(비동기)
    output reg         dht_low,     // 1이면 버스를 Low로 당김
    output reg         busy,
    output reg         done_pulse,  // 종료(성공/실패 모두) 1클럭
    output reg         err,         // 응답 없음 또는 시간 초과 (start 때 지워짐)
    output reg         csum_ok,
    output reg  [15:0] rh_raw,
    output reg  [15:0] temp_raw,
    output reg  [7:0]  chk_raw
);
    localparam T_START   = 1100;  // 시작 Low >= 1 ms
    localparam T_REL_TO  = 100;   // 해제 후 선이 올라가기/응답 Low까지 대기 한도
    localparam T_RESP_TO = 150;   // 응답 Low(80) / High(80) 한도
    localparam T_BIT_TO  = 120;   // 비트 Low(50), High 끝(70) 한도
    localparam T_SAMP    = 40;    // High 시작 후 샘플 시점

    localparam S_IDLE=4'd0, S_START=4'd1, S_REL_H=4'd2, S_REL_L=4'd3, S_RL=4'd4,
               S_RH=4'd5, S_BL=4'd6, S_BH=4'd7, S_BW=4'd8, S_CHK=4'd9;

    reg [3:0]  st;
    reg [7:0]  c;
    reg        tick;
    reg [11:0] cnt;
    reg [5:0]  bitn;
    reg [39:0] shift;
    reg        s1, s2;

    always @(posedge clk) begin
        if (c == CLK_MHZ - 1) begin c <= 8'd0; tick <= 1'b1; end
        else begin c <= c + 8'd1; tick <= 1'b0; end
    end

    task fail;
        begin err <= 1'b1; busy <= 1'b0; done_pulse <= 1'b1; dht_low <= 1'b0; st <= S_IDLE; end
    endtask

    always @(posedge clk) begin
        s1 <= dht_i;
        s2 <= s1;
        done_pulse <= 1'b0;
        if (!rstn) begin
            st <= S_IDLE; dht_low <= 1'b0; busy <= 1'b0; err <= 1'b0; csum_ok <= 1'b0;
            rh_raw <= 16'd0; temp_raw <= 16'd0; chk_raw <= 8'd0;
            cnt <= 12'd0; bitn <= 6'd0; shift <= 40'd0; c <= 8'd0; tick <= 1'b0;
        end else begin
            case (st)
                S_IDLE: begin
                    dht_low <= 1'b0;
                    if (start) begin
                        busy <= 1'b1; err <= 1'b0; csum_ok <= 1'b0;
                        cnt <= 12'd0; bitn <= 6'd0; dht_low <= 1'b1; st <= S_START;
                    end
                end
                S_START: begin
                    dht_low <= 1'b1;
                    if (tick) begin
                        if (cnt == T_START - 1) begin dht_low <= 1'b0; cnt <= 12'd0; st <= S_REL_H; end
                        else cnt <= cnt + 12'd1;
                    end
                end
                S_REL_H: begin   // 우리가 놓은 선이 풀업으로 올라가길 기다림
                    if (s2) begin cnt <= 12'd0; st <= S_REL_L; end
                    else if (tick) begin if (cnt == T_REL_TO) fail; else cnt <= cnt + 12'd1; end
                end
                S_REL_L: begin   // 소자가 응답 Low로 당기길 기다림
                    if (!s2) begin cnt <= 12'd0; st <= S_RL; end
                    else if (tick) begin if (cnt == T_REL_TO) fail; else cnt <= cnt + 12'd1; end
                end
                S_RL: begin      // 응답 Low(80) 끝
                    if (s2) begin cnt <= 12'd0; st <= S_RH; end
                    else if (tick) begin if (cnt == T_RESP_TO) fail; else cnt <= cnt + 12'd1; end
                end
                S_RH: begin      // 응답 High(80) 끝
                    if (!s2) begin cnt <= 12'd0; st <= S_BL; end
                    else if (tick) begin if (cnt == T_RESP_TO) fail; else cnt <= cnt + 12'd1; end
                end
                S_BL: begin      // 비트 Low(50) 끝
                    if (s2) begin cnt <= 12'd0; st <= S_BH; end
                    else if (tick) begin if (cnt == T_BIT_TO) fail; else cnt <= cnt + 12'd1; end
                end
                S_BH: begin      // High 시작 후 T_SAMP µs 시점에 판정
                    if (tick) begin
                        if (cnt == T_SAMP - 1) begin
                            shift <= {shift[38:0], s2};
                            bitn  <= bitn + 6'd1;
                            cnt   <= 12'd0;
                            st    <= (bitn == 6'd39) ? S_CHK : S_BW;
                        end else cnt <= cnt + 12'd1;
                    end
                end
                S_BW: begin      // 다음 비트의 Low 시작을 기다림
                    if (!s2) begin cnt <= 12'd0; st <= S_BL; end
                    else if (tick) begin if (cnt == T_BIT_TO) fail; else cnt <= cnt + 12'd1; end
                end
                S_CHK: begin
                    rh_raw   <= shift[39:24];
                    temp_raw <= shift[23:8];
                    chk_raw  <= shift[7:0];
                    csum_ok  <= ((shift[39:32] + shift[31:24] + shift[23:16] + shift[15:8]) & 8'hFF) == {24'd0, shift[7:0]} ? 1'b1 : 1'b0;
                    busy <= 1'b0; done_pulse <= 1'b1; st <= S_IDLE;
                end
                default: st <= S_IDLE;
            endcase
        end
    end
endmodule

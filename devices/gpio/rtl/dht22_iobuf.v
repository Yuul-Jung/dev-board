// 오픈 드레인 핀. low=1이면 0을 출력하고, 아니면 Hi-Z로 외부 풀업에 맡긴다.
module dht22_iobuf (
    input  wire low,
    inout  wire pad,
    output wire i
);
`ifdef SIMULATION
    assign pad = low ? 1'b0 : 1'bz;
    assign i   = pad;
`else
    IOBUF #(.DRIVE(12), .IOSTANDARD("LVCMOS33"), .SLEW("SLOW")) u_iobuf (
        .O(i), .IO(pad), .I(1'b0), .T(~low)
    );
`endif
endmodule

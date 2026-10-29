/* 적대적 시험: 클럭·비트레이트 조합을 넓게 훑어 can_calc_timing 결과가 항상 (1) 정확한 비트레이트를 만들고 (2) 필드 범위를 지키는지 검증한다. */
#include <stdio.h>
#include "../sw/axi_can.h"
int main(void)
{
    const uint32_t clks[] = { 8000000, 16000000, 20000000, 24000000, 40000000, 50000000, 80000000, 100000000 };
    const uint32_t bps[] = { 10000, 20000, 50000, 83333, 100000, 125000, 250000, 500000, 800000, 1000000 };
    long ok = 0, none = 0;
    for (unsigned i = 0; i < sizeof clks / sizeof *clks; i++) for (unsigned j = 0; j < sizeof bps / sizeof *bps; j++) {
        can_timing_t t;
        int r = can_calc_timing(clks[i], bps[j], &t);
        if (r != CAN_OK) { none++; continue; }
        uint32_t brp = t.brpr + 1u, ts1 = (t.btr & 0xF) + 1u, ts2 = ((t.btr >> 4) & 7) + 1u, sjw = ((t.btr >> 7) & 3) + 1u;
        uint32_t ntq = 1 + ts1 + ts2;
        if ((uint64_t)brp * ntq * bps[j] != clks[i]) { printf("비트레이트 불일치 %u/%u\n", clks[i], bps[j]); return 1; }
        if (sjw > ts2 || sjw > 4 || ts1 > 16 || ts2 > 8 || (t.btr >> 9)) { printf("필드 범위 위반 %u/%u btr=%x\n", clks[i], bps[j], t.btr); return 1; }
        ok++;
    }
    printf("성공 %ld 해 없음 %ld\nALL PASS\n", ok, none);
    return 0;
}

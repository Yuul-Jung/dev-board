#include <stdio.h>
#include "../sw/ps2.h"
static int fails;
#define CHK(c) do { if (!(c)) { printf("FAIL line %d: %s\n", __LINE__, #c); fails++; } } while (0)
int main(void)
{
    ps2_dec_t s = {0}; ps2_key_t k;
    const uint8_t seq[] = { 0x1C, 0xF0, 0x1C, 0xE0, 0x75, 0xE0, 0xF0, 0x75 };   /* 누름, 뗌, 확장 누름, 확장 뗌 */
    int n = 0; ps2_key_t ev[4];
    for (unsigned i = 0; i < sizeof seq; i++) if (ps2_decode(&s, seq[i], &k)) ev[n++] = k;
    CHK(n == 4);
    CHK(ev[0].code == 0x1C && !ev[0].release && !ev[0].ext);
    CHK(ev[1].code == 0x1C && ev[1].release && !ev[1].ext);
    CHK(ev[2].code == 0x75 && !ev[2].release && ev[2].ext);
    CHK(ev[3].code == 0x75 && ev[3].release && ev[3].ext);
    printf(fails ? "FAILED %d\n" : "ALL PASS\n", fails);
    return fails != 0;
}

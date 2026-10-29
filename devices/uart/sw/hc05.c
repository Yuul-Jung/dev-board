#include "hc05.h"
#include <string.h>
#include <stdio.h>

static int fmt(char *o, size_t cap, int n) { (void)o; return (n < 0 || (size_t)n >= cap) ? -1 : n; }
int hc05_cmd_at(char *o, size_t cap) { return fmt(o, cap, snprintf(o, cap, "AT\r\n")); }
int hc05_cmd_name(char *o, size_t cap, const char *s) { return fmt(o, cap, snprintf(o, cap, "AT+NAME=%s\r\n", s)); }
int hc05_cmd_pswd(char *o, size_t cap, const char *s) { return fmt(o, cap, snprintf(o, cap, "AT+PSWD=%s\r\n", s)); }
int hc05_cmd_uart(char *o, size_t cap, unsigned b, unsigned st, unsigned p)
{ return fmt(o, cap, snprintf(o, cap, "AT+UART=%u,%u,%u\r\n", b, st, p)); }
int hc05_cmd_role(char *o, size_t cap, unsigned r) { return fmt(o, cap, snprintf(o, cap, "AT+ROLE=%u\r\n", r)); }

int hc05_send(uartlite_t *u, const char *cmd, int (*poll)(void *), void *ctx)
{
    char line[48]; unsigned n = 0;
    uartlite_write(u, cmd, strlen(cmd));
    while (poll(ctx)) {
        int c = uartlite_getc(u);
        if (c < 0) continue;
        if (c == '\n') {
            line[n] = 0;
            return strcmp(line, "OK") == 0 ? 0 : -2;
        }
        if (c != '\r' && n < sizeof line - 1) line[n++] = (char)c;
    }
    return -1; /* 시간 초과 */
}

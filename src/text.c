#include "text.h"

#include "cmd.h"

#include <assert.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#if __WORDSIZE == 64
#define int64_f "ld"
#else
#define int64_f "lld"
#endif

int
debugf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stderr, fmt, args);
    va_end(args);
    return ret;
}

void
assign_decimal(char *str, size_t maxlen, int64_t number, size_t n) {
    assert(n < 9);

    char *s = str;
    size_t len = strlen(s);

    if (number == 0) { // 特例：0 直接返回 "0"
        snprintf(s, maxlen, "0");
        return;
    }

    // 先处理负号
    if (number < 0) {
        *s = '-';
        s++;
        maxlen--;
        number = -number;
    }
    // 然后处理小数点
    char fmt[8] = "";
    snprintf(fmt, sizeof(fmt) - 1, "%c0%lu" int64_f, '%', n + 1);
    snprintf(s, maxlen, fmt, number);

    len = strlen(s);
    // 小数点后两位向右平移
    s += len - n;
    memmove(s + 1, s, 2);
    s[0] = '.';
    s[3] = '\0';
}

int
main(int argc, char **argv) {
    if (argc < 2) {
        cmd_help(argc, argv);
        return 1;
    }

    return cmd_parse(argc, argv);
}

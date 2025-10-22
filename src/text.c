#include "text.h"

#include "cmd.h"

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

/* 手动添加小数点 */
void
assign_decimal(char *str, size_t maxlen, int64_t number, size_t n) {
    char *s;
    size_t len;

    // 先处理负号
    s = str;
    if (number < 0) {
        *s = '-';
        s++;
        maxlen--;
        number = -number;
    }

    // 处理小数点
    snprintf(s, maxlen, "%03" int64_f, number);

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

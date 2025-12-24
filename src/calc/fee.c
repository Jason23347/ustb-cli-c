#include "conf.h"

#include "fee.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "flow.h"
#include "terminal.h"

#if __WORDSIZE == 64
#define int64_f "ld"
#else
#define int64_f "lld"
#endif

#define YUAN 1000

void
assign_decimal(char *str, size_t maxlen, int64_t number, size_t n) {
    assert(n < 9);

    char *s = str;
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

    size_t len = strlen(s);
    // 小数点后两位向右平移
    s += len - n;
    memmove(s + 1, s, 2);
    s[0] = '.';
    s[3] = '\0';
}

unsigned
fee_cost(uint64_t flowKB) {
    uint64_t overKB = flow_over(flowKB);

    if (overKB > 0) {
        return (FEE_PER_1000GB) * (overKB / GB);
    } else {
        return 0;
    }
}

int
cost_color(const char *fee_str) {
#ifdef WITH_COLOR
    double fee = atof(fee_str);
    if (fee < 10) {
        return GREEN;
    } else if (fee < 30) {
        return BLUE;
    } else if (fee < 50) {
        return YELLOW;
    } else {
        return RED;
    }
#else
    return 0;
#endif
}

int
balance_color(const char *fee_str) {
#ifdef WITH_COLOR
    double fee = atof(fee_str);
    if (fee > 30) {
        return GREEN;
    } else if (fee > 10) {
        return BLUE;
    } else if (fee > 3) {
        return YELLOW;
    } else {
        return RED;
    }
#else
    return 0;
#endif
}

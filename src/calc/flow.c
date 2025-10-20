#include "conf.h"

#include "flow.h"

#include "text.h"

#include <stdio.h>
#include <string.h>

#define FREE_FLOW_KB (FREE_FLOW_GB * GB)

/**
 * Calculate weight for time interval,
 * assumed to be never lesser than 0.
 */
static unsigned
flow_wight(uint64_t millisec) {
    if (millisec == 0) {
        return 100;
    } else if (millisec < 6000000) {
        return round(unsigned, 2.36e-12 * millisec * (millisec - 12e6) + 100);
    } else {
        return 15;
    }
}

uint64_t
flow_speed(flow_t arr[FLOW_NUM], int current_flow) {
    flow_t *cur, *flow, *last;
    struct timeval *tv;
    uint64_t millisec;
    unsigned weight = 0;
    double res = 0;

    /* 分支即优化 */
    if (current_flow == 0) {
        cur = arr + FLOW_NUM - 1;
        last = arr + FLOW_NUM - 2;
    } else if (current_flow == 1) {
        cur = arr;
        last = arr + FLOW_NUM - 1;
    } else {
        cur = arr + current_flow - 1;
        last = cur - 1;
    }

    tv = &cur->tval;

    millisec = (tv->tv_sec - last->tval.tv_sec) * 1000000 +
               (tv->tv_usec - last->tval.tv_usec);
    cur->speed = (cur->download - last->download) * 1.0 / (millisec + 1);

    // TODO 提供平滑选项
    // for (flow = arr; flow - arr < FLOW_NUM; flow++) {
    //     millisec = (tv->tv_sec - flow->tval.tv_sec) * 1000000 +
    //                (tv->tv_usec - flow->tval.tv_usec);
    //     unsigned w = flow_wight(millisec);
    //     weight += w;
    //     res += w * flow->speed;
    // }

    /* FIXME 谜之bug，出现奇大无比的值 */
    {
        uint64_t tmp = round(uint64_t, cur->speed * 1000000.0);
        return (tmp > 0 && tmp < 100 * MB) ? tmp : 0;
    }
}

void
flow_format(uint64_t flowKB, char *buf, size_t size) {
    if (flowKB <= 1.0 * KB) {
        snprintf(buf, size, "<1 KB");
    } else if (flowKB < MB) {
        snprintf(buf, size, "%.2f KB", (double)flowKB / KB);
    } else if (flowKB < GB) {
        snprintf(buf, size, "%.2f MB", (double)flowKB / MB);
    } else if (flowKB < 0.9 * TB) {
        snprintf(buf, size, "%.2f GB", (double)flowKB / GB);
    } else {
        snprintf(buf, size, "%.2f TB", (double)flowKB / TB);
    }
}

uint64_t
flow_left(uint64_t flowKB) {
    if (flowKB < FREE_FLOW_KB) {
        return FREE_FLOW_KB - flowKB;
    } else {
        return 0;
    }
}

uint64_t
flow_over(uint64_t flowKB) {
    if (flowKB > FREE_FLOW_KB) {
        return 0;
    } else {
        return flowKB - FREE_FLOW_KB;
    }
}

void
flow_format_speed(uint64_t flowKB, char *str, size_t len) {
    if (flowKB < 1000) {
        snprintf(str, len, uint64_spec " KB/s", flowKB);
    } else {
        snprintf(str, len, "%.2lf MB/s", (double)flowKB / KB);
    }

    return;
}

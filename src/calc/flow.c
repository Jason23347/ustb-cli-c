#include "conf.h"

#include "flow.h"

#include "terminal.h"
#include "text.h"

#include <stdio.h>
#include <string.h>

#define FREE_FLOW_KB (FREE_FLOW_GB * GB)

#define MILLION 1000000

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

static inline size_t
prev_index(size_t idx) {
    return (idx == 0) ? (FLOW_NUM - 1) : (idx - 1);
}

uint64_t
flow_speed(flow_history_t *history, uint64_t download) {
    // 新元素位置
    size_t cur_idx = (history->head + 1) % FLOW_NUM;
    size_t last_idx = history->head;

    flow_t *cur = &history->arr[cur_idx];
    flow_t *last = &history->arr[last_idx];

    // 填充当前时间和下载量
    gettimeofday(&cur->tval, NULL);
    cur->download = download;

    // 更新环形缓冲索引
    history->head = cur_idx;
    if (cur_idx == history->tail) {
        history->tail = (history->tail + 1) % FLOW_NUM; // 覆盖最老元素
    }

    // 计算速度
    uint64_t micros = (cur->tval.tv_sec - last->tval.tv_sec) * MILLION +
                      (cur->tval.tv_usec - last->tval.tv_usec);
    if (micros == 0) {
        micros = 1;
    }

    cur->speed = (double)(cur->download - last->download) / micros;

    uint64_t tmp = round(uint64_t, cur->speed *MILLION);
    return (tmp > 0 && tmp < 100 * MB) ? tmp : 0;
}

int
flow_speed_color(uint64_t speedKB) {
    if (speedKB < 1 * MB) {
        return GREEN;
    } else if (speedKB < 6 * MB) {
        return YELLOW;
    } else {
        return RED;
    }
}

void
flow_format(uint64_t flowKB, char *buf, size_t size) {
    if (flowKB <= KB) {
        snprintf(buf, size, "0 KB");
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
        return (FREE_FLOW_KB - flowKB);
    } else {
        return 0;
    }
}

uint64_t
flow_over(uint64_t flowKB) {
    if (flowKB > FREE_FLOW_KB) {
        return 0;
    } else {
        return (flowKB - FREE_FLOW_KB);
    }
}

void
flow_format_speed(uint64_t flowKB, char *str, size_t len) {
    if (flowKB <= 1 * KB) {
        snprintf(str, len, "0 KB/s");
    } else if (flowKB < 1000 * KB) {
        snprintf(str, len, uint64_spec " KB/s", flowKB);
    } else {
        snprintf(str, len, "%.2lf MB/s", (double)flowKB / MB);
    }

    return;
}

#ifndef CALC_FLOW_H
#define CALC_FLOW_H

#include <stdint.h>
#include <sys/time.h>

#define FLOW_NUM 8

#define round(_type, _num) (_type)((_num) + 0.5)

#define KB 1
#define MB (1024 * KB)
#define GB (1024 * MB)
#define TB (1024 * GB)

typedef struct {
    struct timeval tval; // time
    uint64_t download;   // KB
    double speed;        // KB/s
} flow_t;

typedef struct {
    flow_t arr[FLOW_NUM];
    size_t head;
    size_t tail;
} flow_history_t;

uint64_t flow_left(uint64_t flowKB);
uint64_t flow_over(uint64_t flowKB);
uint64_t flow_speed(flow_history_t *history, uint64_t flow);
int flow_speed_color(uint64_t speedKB);
void flow_format(uint64_t flowKB, char *buf, size_t size);
void flow_format_speed(uint64_t flowKB, char *str, size_t len);

#endif /* CALC_FLOW_H */

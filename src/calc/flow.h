#ifndef FLOW_H
#define FLOW_H

#include <stdint.h>
#include <sys/time.h>

#define FLOW_NUM 8

#define round(_type, _num) (_type)((_num) + 0.5)

#define KB 1
#define MB (1024 * KB)
#define GB (1024 * MB)
#define TB (1024 * GB)

typedef struct {
    struct timeval tval;
    uint64_t download;
    double speed;
} flow_t;

typedef struct {
    flow_t arr[FLOW_NUM];
} flow_history_t;

uint64_t flow_left(uint64_t flowKB);
uint64_t flow_over(uint64_t flowKB);
uint64_t flow_speed(flow_t arr[FLOW_NUM], int current_flow);
void flow_format(uint64_t flowKB, char *buf, size_t size);
void flow_format_speed(uint64_t flowKB, char *str, size_t len);

#endif /* FLOW_H */

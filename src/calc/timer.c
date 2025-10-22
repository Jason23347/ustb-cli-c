#include "timer.h"

#define MILLION 1000000

int
sleep_till_next_sec() {
    struct timespec ts;
    struct timeval tv;

    gettimeofday(&tv, NULL);

    ts.tv_sec = 0;
    ts.tv_nsec = (1000000 - tv.tv_usec % 1000000) * 1000;

    return nanosleep(&ts, NULL);
}

suseconds_t
microsec_interval(struct timeval start, struct timeval end) {
    return (end.tv_sec - start.tv_sec) * MILLION +
           (end.tv_usec - start.tv_usec);
}

double
speed_per_sec(uint64_t amount, suseconds_t microsec) {
    return (double)(amount * MILLION) / microsec;
}

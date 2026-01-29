#ifndef CALC_INTERVAL_H
#define CALC_INTERVAL_H

#include <stdint.h>
#include <stdlib.h>
#include <sys/time.h>

int sleep_till_next_sec(void);
suseconds_t microsec_interval(struct timeval start, struct timeval end);
double micro2sec(suseconds_t microsec);
double speed_per_sec(uint64_t flowKB, suseconds_t microsec);
static inline double
random_d() {
    return (double)(rand()) / RAND_MAX;
}

#endif /* CALC_INTERVAL_H */

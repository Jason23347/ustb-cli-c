#ifndef CALC_TIME_H
#define CALC_TIME_H

#include <stdint.h>
#include <sys/time.h>
#include <time.h>

int sleep_till_next_sec();
suseconds_t microsec_interval(struct timeval start, struct timeval end);
double speed_per_sec(uint64_t amount, suseconds_t microsec);

#endif /* CALC_TIME_H */

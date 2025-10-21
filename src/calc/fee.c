#include "conf.h"

#include "fee.h"

#include <stdint.h>
#include <stdlib.h>

#include "flow.h"
#include "terminal.h"

#define YUAN 1000

unsigned
fee_cost(uint64_t flowKB) {
    uint64_t left = flow_left(flowKB);

    if (left > 0) {
        return (FEE_PER_1000GB * MB) / (left / 1024);
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

#include "conf.h"

#include "fee.h"

#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "flow.h"

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

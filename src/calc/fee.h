#ifndef FEE_H
#define FEE_H

#include "text.h"

#include <stddef.h>
#include <stdint.h>

static inline void
fee_format(char *str, size_t len, unsigned fee) {
    assign_decimal(str, len, fee, 4);
}

unsigned fee_cost(uint64_t flowKB);

#endif /* FEE_H */

#ifndef CALC_FEE_H
#define CALC_FEE_H

#include <stddef.h>
#include <stdint.h>

/* 手动添加小数点 */
void assign_decimal(char *str, size_t maxlen, int64_t number, size_t n);
static inline void
fee_format(char *str, size_t len, unsigned fee) {
    assign_decimal(str, len, fee, 4);
}

unsigned fee_cost(uint64_t flowKB);
int cost_color(const char *fee_str);
int balance_color(const char *fee_str);

#endif /* CALC_FEE_H */

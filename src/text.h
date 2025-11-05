#ifndef TEXT_H
#define TEXT_H

#include <stddef.h>
#include <stdint.h>

/* 手动添加小数点 */
void assign_decimal(char *str, size_t maxlen, int64_t number, size_t n);

#endif /* TEXT_H */

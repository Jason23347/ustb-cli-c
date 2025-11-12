#ifndef LIB_DECODE_H
#define LIB_DECODE_H

#include "gbuff.h"

/* Decode GB2312 to UTF-8 */
int decode_gb2312(gbuff_t *utf8_out, const gbuff_t *gb_in);

#endif /* LIB_DECODE_H */

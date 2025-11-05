#ifndef LIB_DECODE_H
#define LIB_DECODE_H

#include "gstr.h"

/* Decode GB2312 to UTF-8 */
int decode_gb2312(gstr_t *utf8_out, const gstr_t *gbk_in);

#endif /* LIB_DECODE_H */

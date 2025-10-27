#ifndef LIB_DECODE_H
#define LIB_DECODE_H

#include "gstr.h"

/* Decode GB2312 to UTF-8 */
gstr_t *decode_gb2312(const gstr_t *gbk);

#endif /* LIB_DECODE_H */

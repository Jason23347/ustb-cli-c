#ifndef GSTR_H
#define GSTR_H

#include "gbuff.h"

#include <string.h>

#define EXT_UNQUOTED 0
#define EXT_QUOTED   1

typedef gbuff_t gstr_t;

struct extract {
    void *dest;
    const char *src;
    const gstr_t *fmt;
    const gstr_t *prefix;
    int quoted;
};

#define gstr_from_const(str)                                                   \
    (gstr_t) {                                                                 \
        .data = (char *)(str), .len = strlen(str), .cap = strlen(str) + 1,     \
    }

#define gstr_alloca(maxlen) gbuff_alloca(maxlen)

int gstr_extract(const struct extract *data);
int gstr_appendf(gstr_t *str, const char *fmt, ...);

#endif /* GSTR_H */

#include "gstr.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
gstr_appendf(gstr_t *str, const char *fmt, ...) {
    assert(str != NULL);
    assert(str->data != NULL);

    // full
    if (str->len >= str->cap) {
        return -1;
    }

    char *dst = &str->data[str->len];
    size_t avail = str->cap - str->len;

    va_list args;
    va_start(args, fmt);
    int written = vsnprintf(dst, avail, fmt, args);
    va_end(args);

    if (written < 0) {
        return -1;
    }

    if ((size_t)written >= avail) {
        str->len = str->cap - 1;
        str->data[str->len] = '\0';
    } else {
        str->len += (size_t)written;
    }

    return written;
}

int
gstr_extract(const struct extract *data) {
    int res = 0;
    char dummy[2];

    const gstr_t *prefix = data->prefix;
    const gstr_t *fmt = data->fmt;
    const char *src = data->src;
    void *dest = data->dest;
    int quoted = data->quoted;

    size_t len = prefix->len + fmt->len + 12;
    gstr_t tmp[1] = {gstr_alloca(len)};
    gstr_appendf(tmp, "%s=", prefix->data);

    char *p = strstr(src, tmp->data);
    if (p == NULL) {
        return -1;
    }

    if (quoted) {
        const char percent = '%';
        gstr_appendf(tmp, "%c['\"]%s%c['\"]", percent, fmt->data, percent);
        res = sscanf(p, tmp->data, &dummy[0], dest, &dummy[1]);
    } else {
        gstr_appendf(tmp, "%s", fmt->data);
        res = sscanf(p, tmp->data, dest);
    }

    return res;
}

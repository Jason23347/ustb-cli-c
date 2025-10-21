#include "gstr.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
gstr_init(gstr_t *str, size_t maxlen) {
    str->s = malloc(maxlen);
    if (str->s == NULL) {
        return -1;
    }

    str->len = 0;
    str->cap = maxlen;
    str->s[0] = '\0';

    return 0;
}

void
gstr_free(gstr_t *str) {
    free(str->s);

    str->s = NULL;
    str->len = 0;
    str->cap = 0;
}

int
gstr_appendf(gstr_t *str, const char *fmt, ...) {
    if (!str || !str->s || str->cap == 0) {
        return -1;
    }

    // full
    if (str->len >= str->cap) {
        return -1;
    }

    char *dst = &str->s[str->len];
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
        str->s[str->len] = '\0';
    } else {
        str->len += (size_t)written;
    }

    return written;
}

int
gstr_extract(void *dest, const char *src, const gstr_t *fmt,
                const gstr_t *prefix, int quoted) {
    int res = 0;

    size_t len = prefix->len + fmt->len + 4;
    char buffer[len];

    gstr_t *tmp = gstr_from_buf(buffer);
    gstr_appendf(tmp, "%s=", prefix->s);

    char *p = strstr(src, tmp->s);
    if (p == NULL) {
        return -1;
    }
    if (quoted) {
        gstr_appendf(tmp, "'%s'", fmt->s);
    } else {
        gstr_appendf(tmp, "%s", fmt->s);
    }

    res = sscanf(p, tmp->s, dest);

    return res;
}

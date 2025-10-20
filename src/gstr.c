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

const char *
strmatch(const char *str, const char *pattern, size_t size) {
    const char *p = str;
    do {
        p = strchr(p + 1, pattern[0]);
        if (p == 0) {
            return 0;
        }
    } while (strncmp(p, pattern, size - 1) != 0);

    return p;
}

int
extract_between(char *dest, const char *src,           //
                const char *prefix, size_t prefix_len, //
                const char suffix,                     //
                size_t maxlen) {
    const char *match = strmatch(src, prefix, prefix_len);
    const char *start = &match[prefix_len - 1];
    if (!start) {
        return -1;
    }

    const char *end = strchr(start, suffix);
    if (!end) {
        return -1;
    }

    int len = end - start;
    if (len > maxlen) {
        len = maxlen;
    }

    strncpy(dest, start, len);
    dest[len] = '\0';

    return len;
}

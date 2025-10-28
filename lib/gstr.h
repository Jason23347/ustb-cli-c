#ifndef GSTR_H
#define GSTR_H

#include <assert.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>

typedef struct {
    char *s;    // string pointer
    size_t len; // string length
    size_t cap; // maxsize
} gstr_t;

/* 静态字符串初始化为gstr */
#define gstr_from_buf(buf)                                                     \
    ({                                                                         \
        static_assert(!__builtin_types_compatible_p(typeof(buf), char *),      \
                      "buf must be a char array, not a pointer");              \
        size_t _cap = sizeof(buf);                                             \
        buf[0] = '\0';                                                         \
        gstr_t _g = {                                                          \
            .s = (buf),                                                        \
            .len = 0,                                                          \
            .cap = _cap,                                                       \
        };                                                                     \
        _g;                                                                    \
    })

#define gstr_from_const(str)                                                   \
    (gstr_t) { .s = (char *)(str), .len = strlen(str), .cap = strlen(str) + 1, }

#define gstr_alloca(maxlen)                                                    \
    ({                                                                         \
        size_t _cap = maxlen;                                                  \
        char *buf = alloca(_cap);                                              \
        buf[0] = '\0';                                                         \
        gstr_t _g = {                                                          \
            .s = (buf),                                                        \
            .len = 0,                                                          \
            .cap = _cap,                                                       \
        };                                                                     \
        _g;                                                                    \
    })

#define extract(dest, src, fmt, prefix, quoted)                                \
    gstr_extract((dest), (src), &gstr_from_const(fmt),                         \
                 &gstr_from_const(prefix), (quoted))
int gstr_extract(void *dest, const char *src, const gstr_t *fmt,
                 const gstr_t *prefix, int quoted);

int gstr_appendf(gstr_t *str, const char *fmt, ...);

#endif /* GSTR_H */

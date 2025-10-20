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
        gstr_t _g = {                                                          \
            .s = (buf),                                                        \
            .len = strlen(buf),                                                \
            .cap = _cap,                                                       \
        };                                                                     \
        &_g;                                                                   \
    })

#define gstr_from_const(str)                                                   \
    (gstr_t) { .s = (char *)(str), .len = strlen(str), .cap = strlen(str) + 1, }

/* 简约的写法，应当仅仅用于静态变量 */
#define strpos(str, pattern) strmatch(str, pattern, sizeof(pattern))
/* 根据fmt读取pattern首次匹配str后面的内容到prop中 */
#define strscan(str, pattern, fmt, prop)                                       \
    {                                                                          \
        str = strpos(p, pattern);                                              \
        if (!str) {                                                            \
            return -1;                                                         \
        }                                                                      \
        sscanf(str + sizeof(pattern) - 1, fmt, &prop);                         \
    }

int extract_between(char *dest, const char *src,           //
                    const char *prefix, size_t prefix_len, //
                    const char suffix,                     //
                    size_t maxlen);
const char *strmatch(const char *str, const char *pattern, size_t size);

int gstr_init(gstr_t *str, size_t maxlen);
void gstr_free(gstr_t *str);
int gstr_appendf(gstr_t *str, const char *fmt, ...);

#endif /* GSTR_H */

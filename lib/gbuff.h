#ifndef LIB_GBUFF_H
#define LIB_GBUFF_H

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

typedef struct {
    char *data; /* Where the real buffer is */
    size_t len; /* Used length */
    size_t cap; /* Total capacity */
#ifndef NDEBUG
    int _heap_flag; /* Check if allocated on heap */
#endif
} gbuff_t;

/* Create a gbuff_t from a constant string */
#define gbuff_from_const(str)                                                  \
    (gbuff_t) {                                                                \
        .data = (char *)(str), .len = strlen(str), .cap = strlen(str) + 1,     \
    }

#ifndef NDEBUG
#define gbuff_alloca(size)                                                     \
    (gbuff_t) {                                                                \
        .data = alloca(size), .len = 0, .cap = (size), ._heap_flag = 0,        \
    }
#else
/* Without _head_flag */
#define gbuff_alloca(size)                                                     \
    (gbuff_t) { .data = alloca(size), .len = 0, .cap = (size), }
#endif

/* Initialize buff with given size */
int gbuff_init(gbuff_t *buff, size_t size);
/* Realloc if buff is not large enough */
int gbuff_ensure(gbuff_t *buff, size_t size);
/* Reallocate the real buffer to the given size */
int gbuff_realloc(gbuff_t *buff, size_t size);
/* Free the real buffer in buff */
void gbuff_free(gbuff_t *buff);
/* Set all to 0 */
void gbuff_clear(gbuff_t *buff);
/* Append n bytes from src to buff */
ssize_t gbuff_put(gbuff_t *buff, const void *src, size_t n);
/* Append formatted string to buff with ending '\0' */
int gbuff_appendf(gbuff_t *buff, const char *fmt, ...);
/* Append src to dest with ending '\0' */
int gbuff_concat(gbuff_t *dest, const gbuff_t *src);

#define EXT_UNQUOTED 0
#define EXT_QUOTED   1

struct extract {
    void *dest;            /* Where to store extracted value */
    const char *src;       /* Source string */
    const gbuff_t *fmt;    /* Format string */
    const gbuff_t *prefix; /* Prefix string */
    int quoted;            /* Whether the value is quoted by ' or " */
};

/* Extract pattern prefix=['"]?(.*)['"]? */
int gbuff_extract(const struct extract *data);

#endif /* LIB_GBUFF_H */

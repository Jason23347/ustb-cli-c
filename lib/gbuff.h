#ifndef LIB_GBUFF_H
#define LIB_GBUFF_H

#include <assert.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdlib.h>

typedef struct {
    char *data;
    size_t len;
    size_t cap;
#ifndef NDEBUG
    int _heap_flag; // Check if allocated on heap
#endif
} gbuff_t;

#ifndef NDEBUG
#define gbuff_alloca(size)                                                     \
    ({                                                                         \
        (gbuff_t){                                                             \
            .data = alloca(size),                                              \
            .len = 0,                                                          \
            .cap = (size),                                                     \
            ._heap_flag = 0,                                                   \
        };                                                                     \
    })
#else
/* Without _head_flag */
#define gbuff_alloca(size)                                                     \
    ({                                                                         \
        (gbuff_t){                                                             \
            .data = alloca(size),                                              \
            .len = 0,                                                          \
            .cap = (size),                                                     \
        };                                                                     \
    })
#endif

int gbuff_init(gbuff_t *buff, size_t size);
int gbuff_ensure(gbuff_t *buff, size_t size);
int gbuff_realloc(gbuff_t *buff, size_t size);
void gbuff_free(gbuff_t *buff);
void gbuff_clear(gbuff_t *buff);
ssize_t gbuff_put(gbuff_t *buff, const void *src, size_t n);
int gbuff_appendf(gbuff_t *buff, const char *fmt, ...);

#endif /* LIB_GBUFF_H */

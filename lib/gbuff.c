#include "gbuff.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int
gbuff_init(gbuff_t *buff, size_t size) {
    char *tmp = calloc(size, 1);
    if (tmp == NULL) {
        return -1;
    }

    buff->data = tmp;
    buff->len = 0;
    buff->cap = size;
#ifndef NDEBUG
    buff->_heap_flag = 1;
#endif

    return 0;
}

int
gbuff_ensure(gbuff_t *buff, size_t size) {
    if (buff->cap < size) {
        return gbuff_realloc(buff, size);
    } else {
        return 0;
    }
}

int
gbuff_realloc(gbuff_t *buff, size_t size) {
    assert(buff != NULL);
    assert(buff->data != NULL);
#ifndef NDEBUG
    assert(buff->_heap_flag == 1);
#endif

    char *tmp = realloc(buff->data, size);
    if (tmp == NULL) {
        return -1;
    }

    buff->data = tmp;
    buff->cap = size;
    if (buff->len >= size) {
        buff->len = size - 1;
        buff->data[buff->len] = '\0';
    }

    return 0;
}

void
gbuff_free(gbuff_t *buff) {
    assert(buff != NULL);
    int flag = 0;
#ifndef NDEBUG
    flag = buff->_heap_flag;
#endif

    if (flag == 0) {
        assert(!"attempt to free stack buffer");
    } else {
        free(buff->data);
    }
}

void
gbuff_clear(gbuff_t *buff) {
    assert(buff != NULL);
    assert(buff->data != NULL);

    buff->len = 0;
    if (buff->cap > 0) {
        buff->data[0] = '\0';
    }
}

ssize_t
gbuff_put(gbuff_t *buff, const void *src, size_t n) {
    assert(buff != NULL);
    assert(buff->data != NULL);

    if (buff->len + n < buff->cap) {
        memcpy(buff->data + buff->len, src, n);
        buff->len += n;

        return n;
    }

    return -1;
}

int
gbuff_appendf(gbuff_t *buff, const char *fmt, ...) {
    assert(buff != NULL);
    assert(buff->data != NULL);

    // full
    if (buff->len >= buff->cap) {
        return -1;
    }

    char *dst = &buff->data[buff->len];
    size_t avail = buff->cap - buff->len;

    va_list args;
    va_start(args, fmt);
    ssize_t written = vsnprintf(dst, avail, fmt, args);
    va_end(args);

    if (written < 0) {
        return -1;
    }

    if (written >= avail) {
        buff->len = buff->cap - 1;
        buff->data[buff->len] = '\0';
    } else {
        buff->len += written;
    }

    return written;
}

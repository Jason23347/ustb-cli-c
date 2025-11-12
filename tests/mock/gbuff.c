#include "mem.h"

#include "lib/gbuff.h"
#include "lib/gstr.h"

int
WRAP(gbuff_init)(gbuff_t *buff, size_t size) {
    return SHOULD_FAIL(-1, REAL(gbuff_init)(buff, size));
}

int
WRAP(gbuff_ensure)(gbuff_t *buff, size_t size) {
    return SHOULD_FAIL(-1, REAL(gbuff_ensure)(buff, size));
}

int
WRAP(gbuff_realloc)(gbuff_t *buff, size_t size) {
    return SHOULD_FAIL(-1, REAL(gbuff_realloc)(buff, size));
}

ssize_t
WRAP(gbuff_put)(gbuff_t *buff, const void *src, size_t n) {
    return SHOULD_FAIL(-1, REAL(gbuff_put)(buff, src, n));
}

#include "mem.h"

#include <stddef.h>

extern void *__real__test_malloc(const size_t size, const char *file,
                                 const int line);
extern void *__real__test_realloc(void *ptr, const size_t size,
                                  const char *file, const int line);
extern void *__real__test_calloc(const size_t number_of_elements,
                                 const size_t size, const char *file,
                                 const int line);

/* __wrap__test_malloc 获取调用方的返回地址以映射 site */
void *
__wrap__test_malloc(const size_t size, const char *file, const int line) {
    return SHOULD_FAIL(NULL, __real__test_malloc(size, file, line));
}

void *
__wrap__test_calloc(const size_t number_of_elements, const size_t size,
                    const char *file, const int line) {
    return SHOULD_FAIL(
        NULL, __real__test_calloc(number_of_elements, size, file, line));
}

void *
__wrap__test_realloc(void *ptr, const size_t size, const char *file,
                     const int line) {
    return SHOULD_FAIL(NULL, __real__test_realloc(ptr, size, file, line));
}

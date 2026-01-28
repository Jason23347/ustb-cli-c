#ifndef LIB_HASH_H
#define LIB_HASH_H

#include <stddef.h>

#define MD5_LEN 33

void md5(char *dest, const char *str, size_t len);

#endif /* LIB_HASH_H */

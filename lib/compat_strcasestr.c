#if defined(__linux__) || defined(__gnu_linux__) || defined(__APPLE__)

#define _GNU_SOURCE
#include <string.h>

char *
compat_strcasestr(const char *__haystack, const char *__needle) {
    return strcasestr(__haystack, __needle);
}

#else /* defined(__linux__) || defined(__gnu_linux__) || defined(__APPLE__) */

#include <ctype.h>
#include <string.h>

// Stupid implementation of strcasestr() for non-GNU systems
char *
compat_strcasestr(const char *__haystack, const char *__needle) {
    if (!*__needle) {
        return (char *)__haystack;
    }
    for (; *__haystack; __haystack++) {
        unsigned char *h, *n;

        h = (unsigned char *)__haystack;
        n = (unsigned char *)__needle;
        if (tolower(*h) == tolower(*n)) {
            for (h = (unsigned char *)__haystack, n = (unsigned char *)__needle;
                 *h && *n; h++, n++) {
                if (tolower(*h) != tolower(*n)) {
                    break;
                }
            }
            if (!*n) {
                return (char *)__haystack;
            }
        }
    }
    return NULL;
}

#endif
/* defined(__linux__) || defined(__gnu_linux__) || defined(__APPLE__) */

#include "config.h"

#ifdef HAVE_STRCASESTR

#define _GNU_SOURCE
#include <string.h>

char *
compat_strcasestr(const char *__haystack, const char *__needle) {
    return strcasestr(__haystack, __needle);
}

#else /* ifdef HAVE_STRCASESTR */

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

#endif /* ifdef HAVE_STRCASESTR */

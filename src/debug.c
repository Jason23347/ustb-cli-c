#include "conf.h"

#include <stdarg.h>
#include <stdio.h>

int
debugf(const char *fmt, ...) {
    va_list args;
    va_start(args, fmt);
    int ret = vfprintf(stderr, fmt, args);
    va_end(args);
    return ret;
}

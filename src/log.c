#include "conf.h"

#include <stdarg.h>
#include <stdio.h>

enum LOG_LEVEL log_level = INFO;
FILE *log_file = NULL;

void
log_init(void) {
    log_file = stderr;
}

void
log_set_level(enum LOG_LEVEL level) {
    log_level = level;
}

void
log_set_file(FILE *file) {
    log_file = file;
}

int
print_log(enum LOG_LEVEL level, const char *fmt, ...) {
    if (level <= SILENT) {
        return 0;
    }

    if (level <= log_level) {
        va_list args;
        va_start(args, fmt);
        int ret = vfprintf(log_file, fmt, args);
        va_end(args);
        return ret;
    }

    return 0;
}

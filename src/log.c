#include "conf.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

enum LOG_LEVEL log_level = SILENT;
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
        // print time
        time_t now = time(NULL);
        struct tm *tm_info = localtime(&now);
        char time_str[20];
        strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);
        fprintf(log_file, "[%s] ", time_str);

        // print log message
        va_list args;
        va_start(args, fmt);
        int ret = vfprintf(log_file, fmt, args);
        va_end(args);
        return ret;
    }

    return 0;
}

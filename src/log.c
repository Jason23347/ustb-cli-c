#include "conf.h"

#include "cmd/cmd.h"
#include "terminal.h"

#include <stdarg.h>
#include <stdio.h>
#include <time.h>

#define set_log_color(f, c)                                                    \
    do {                                                                       \
        __set_color((f), (c));                                                 \
    } while ((f) == stdout)

#define reset_log_color(f)                                                  \
    do {                                                                       \
        __reset_color(f);                                                      \
    } while ((f) == stdout);

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

static const char level_map[][8] = {
    [SILENT] = "SILENT", [ERROR] = "ERROR", [WARNING] = "WARNING",
    [INFO] = "INFO",     [DEBUG] = "DEBUG",
};

static const char *
level_str(enum LOG_LEVEL level) {
    return level_map[level];
}

static int
level_color(enum LOG_LEVEL level) {
    const int color_map[] = {
        [SILENT] = BLACK, [ERROR] = RED,  [WARNING] = YELLOW,
        [INFO] = WHITE,   [DEBUG] = BLUE,
    };

    return color_map[level];
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

        set_log_color(log_file, level_color(log_level));

        fprintf(log_file, "[%s] ", level_str(log_level));

        // print log message
        va_list args;
        va_start(args, fmt);
        int ret = vfprintf(log_file, fmt, args);
        va_end(args);

        reset_log_color(log_file);

        return ret;
    }

    return 0;
}

#ifndef TERMINAL_H
#define TERMINAL_H

#include "config.h"

#include <stdio.h>

#define NORMAL       0
#define BLACK        30
#define RED          31
#define GREEN        32
#define YELLOW       33
#define BLUE         34
#define PURPLE       35
#define DARKGREEN    36
#define WHITE        37
#define BG_BLACK     40
#define BG_RED       41
#define BG_GREEN     42
#define BG_YELLOW    43
#define BG_BLUE      44
#define BG_PURPLE    45
#define BG_DARKGREEN 46
#define BG_WHITE     47

#ifdef WITH_COLOR

#define STR_HELPER(x) #x
#define STR(x)        STR_HELPER(x)
#define color(code)   "\033[" STR(code) "m"


#define set_color(c)                                                           \
    do {                                                                       \
        if (global_config.raw_output == 0) {                                   \
            printf("\033[%dm", (c));                                           \
        }                                                                      \
    } while (0)

#define reset_color()                                                          \
    do {                                                                       \
        if (global_config.raw_output == 0) {                                   \
            printf("\033[0m");                                                 \
        }                                                                      \
    } while (0)

#define clear_line()                                                           \
    do {                                                                       \
        if (global_config.raw_output == 0) {                                   \
            printf("\r\033[K");                                                \
        }                                                                      \
    } while (0)

#define move_up_head()                                                         \
    do {                                                                       \
        if (global_config.raw_output == 0) {                                   \
            printf("\r\033[F");                                                \
        }                                                                      \
    } while (0)

#else /* ifdef WITH_COLOR */

#define color(...)        ""
#define set_color(...)    ((void)0)
#define reset_color(...)  ((void)0)
#define clear_line(...)   ((void)0)
#define move_up_head(...) ((void)0)

#endif /* WITH_COLOR */

#endif /* TERMINAL_H */

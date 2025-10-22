#ifndef TERMINAL_H
#define TERMINAL_H

#include "config.h"

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

#define STR(text) #text

#define color(code) "\033[" STR(code) "m"

#define set_color(c)                                                           \
    ((global_config.raw_output == 0) ? printf("\033[%dm", c) : ((void)0))
#define reset_color()                                                          \
    ((global_config.raw_output == 0) ? printf("\033[%d;%dm", NORMAL, NORMAL)   \
                                     : ((void)0))

#define clear_line()                                                           \
    ((global_config.raw_output == 0) ? printf("\r\033[K") : ((void)0))
#define move_up_head()                                                         \
    ((global_config.raw_output == 0) ? printf("\r\033[F") : ((void)0))

#else /* WITH_COLOR */

#define color(...) ""
#define set_color(...)
#define reset_color(...)
#define clear_line(...)
#define move_up_head(...)

#endif /* WITH_COLOR */

#endif /* TERMINAL_H */

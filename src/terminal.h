#ifndef TERMINAL_H
#define TERMINAL_H

#include "config.h"

#ifdef WITH_COLOR

#define NORMAL    0
#define BG_NORMAL ;NORMAL

#define BLACK     30
#define RED       31
#define GREEN     32
#define YELLOW    33
#define BLUE      34
#define PURPLE    35
#define DARKGREEN 36
#define WHITE     37

#define BG_BLACK     40
#define BG_RED       41
#define BG_GREEN     42
#define BG_YELLOW    43
#define BG_BLUE      44
#define BG_PURPLE    45
#define BG_DARKGREEN 46
#define BG_WHITE     47

#define STR(text) #text

#define color(code) "\033[" STR(code) "m"

#define set_color(c)  (global_config.raw_output == 0) ? printf(color(%d), c) : ((void)0)
#define reset_color() (global_config.raw_output == 0) ? printf(color(NORMAL;NORMAL)) : ((void)0)

#else /* WITH_COLOR */

#define color(...) ""
#define set_color(...)
#define reset_color(...)

#endif /* WITH_COLOR */

#endif /* TERMINAL_H */

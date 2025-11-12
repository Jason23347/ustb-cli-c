#ifndef MEMFREE_REDIRECT_H
#define MEMFREE_REDIRECT_H

void redirect_stdout();
void restore_stdout();
const char *get_captured_stdout();
void free_captured_stdout();

#define WITH_CAPTURE(var, cmd)                                                 \
    for (const char *var = ({                                                  \
             redirect_stdout();                                                \
             cmd;                                                              \
             restore_stdout();                                                 \
             get_captured_stdout();                                            \
         });                                                                   \
         var; free_captured_stdout(), var = NULL)

#endif /* MEMFREE_REDIRECT_H */

#include "thread.h"

int
ustb_thread_create(ustb_thread_t *thr, ustb_thread_func_t func, void *arg) {
    return pthread_create(thr, NULL, func, arg);
}

int
ustb_thread_join(ustb_thread_t thr) {
    return pthread_join(thr, NULL);
}

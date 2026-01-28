/*
 * Cross-platform thread abstraction for ustb-cli.
 *
 * On POSIX systems this wraps pthreads, on Windows it wraps _beginthreadex
 * and Win32 HANDLEs.  The goal is to hide platform-specific details from
 * callers (e.g. speedtest).
 */

#ifndef USTB_THREAD_H
#define USTB_THREAD_H

#include <pthread.h>
typedef pthread_t ustb_thread_t;
typedef void *(*ustb_thread_func_t)(void *);

/*
 * Create a new thread.
 *
 *  - thr:  out parameter to receive the thread handle.
 *  - func: thread entry function.
 *  - arg:  argument passed to the entry function.
 *
 * Returns 0 on success, non‑zero on failure.
 */
int ustb_thread_create(ustb_thread_t *thr, ustb_thread_func_t func, void *arg);

/*
 * Join a thread created by ustb_thread_create.
 *
 * Returns 0 on success, non‑zero on failure.
 */
int ustb_thread_join(ustb_thread_t thr);

#endif /* USTB_THREAD_H */

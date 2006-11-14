/* $Id $ */

/* timed_thread.h
 * Author: Philip Kovacs 
 *
 * Abstraction layer for timed threads
 * */


#define MINIMUM_INTERVAL_USECS 50000  /* 50000 microseconds = 50 ms =  0.05 sec */

/* opaque structure for clients */
typedef struct _timed_thread timed_thread;

/* create a timed thread */
timed_thread* timed_thread_create (void *(*start_routine)(void*), void *arg, unsigned int interval_ms);

/* destroy a timed thread */
void timed_thread_destroy (timed_thread* p_timed_thread, timed_thread** addr_of_p_timed_thread);

/* lock a timed thread for critical section activity */
int timed_thread_lock (timed_thread* p_timed_thread);

/* unlock a timed thread after critical section activity */
int timed_thread_unlock (timed_thread* p_timed_thread);

/* waits required interval for termination signal and returns 1 if got it, 0 otherwise */
int timed_thread_test (timed_thread* p_timed_thread);

/* exit a timed thread */
void timed_thread_exit (timed_thread* p_timed_thread);

/* register a timed thread for future destruction */
int timed_thread_register (timed_thread* p_timed_thread, timed_thread** addr_of_p_timed_thread);

/* destroy all registered timed threads */
void timed_thread_destroy_registered_threads (void);

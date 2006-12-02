/* $Id$ */

/* timed_thread.c
 * Author: Philip Kovacs
 *
 * Abstraction layer for timed threads
 * */

#include <pthread.h>
#include <assert.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include "timed_thread.h"

/* Abstraction layer for timed threads */

/* private */
struct _timed_thread 
{
    pthread_t thread;			/* thread itself */
    pthread_attr_t thread_attr;	    	/* thread attributes */
    pthread_mutex_t cs_mutex;     	/* critical section mutex */
    pthread_mutex_t runnable_mutex; 	/* only for the runnable_cond */
    pthread_cond_t runnable_cond;  	/* signalled to stop the thread */
    unsigned int interval_usecs;    	/* timed_thread_test() wait interval in microseconds */
};

/* linked list of created threads */
typedef struct _timed_thread_list
{
    timed_thread *p_timed_thread;
    timed_thread **addr_of_p_timed_thread;
    struct _timed_thread_list *next;
} timed_thread_node, timed_thread_list;

static timed_thread_list *p_timed_thread_list_head = NULL;
static timed_thread_list *p_timed_thread_list_tail = NULL;


/* create a timed thread */
timed_thread* 
timed_thread_create (void *(*start_routine)(void*), void *arg, unsigned int interval_usecs)
{
    timed_thread *p_timed_thread;

    assert ((start_routine != NULL) && (interval_usecs >= MINIMUM_INTERVAL_USECS));

    if ((p_timed_thread = calloc (sizeof(timed_thread), 1)) == 0)
	return NULL;

    /* init attributes, e.g. joinable thread */
    pthread_attr_init (&p_timed_thread->thread_attr);
    pthread_attr_setdetachstate (&p_timed_thread->thread_attr, PTHREAD_CREATE_JOINABLE);
    /* init mutexes */
    pthread_mutex_init (&p_timed_thread->cs_mutex, NULL);
    pthread_mutex_init (&p_timed_thread->runnable_mutex, NULL);
    /* init cond */
    pthread_cond_init (&p_timed_thread->runnable_cond, NULL);

    p_timed_thread->interval_usecs = interval_usecs;

    /* create thread */
    if (pthread_create (&p_timed_thread->thread, &p_timed_thread->thread_attr, start_routine, arg))
    {
	timed_thread_destroy (p_timed_thread, NULL);
	return NULL;
    }

    /*fprintf (stderr, "created timed thread 0x%08X\n", (unsigned)p_timed_thread);*/
    return p_timed_thread;
}


/* destroy a timed thread.
 * optional addr_of_p_timed_thread to set callers pointer to NULL as a convenience. */
void 
timed_thread_destroy (timed_thread* p_timed_thread, timed_thread** addr_of_p_timed_thread)
{
    assert (p_timed_thread != NULL);
    assert ((addr_of_p_timed_thread == NULL) || (*addr_of_p_timed_thread == p_timed_thread));

    /* signal thread to stop */
    pthread_mutex_lock (&p_timed_thread->runnable_mutex);
    pthread_cond_signal (&p_timed_thread->runnable_cond);
    pthread_mutex_unlock (&p_timed_thread->runnable_mutex);

    /* join the terminating thread */
    pthread_join (p_timed_thread->thread, NULL);

    /* clean up */
    pthread_attr_destroy (&p_timed_thread->thread_attr);
    pthread_mutex_destroy (&p_timed_thread->cs_mutex);
    pthread_mutex_destroy (&p_timed_thread->runnable_mutex);
    pthread_cond_destroy (&p_timed_thread->runnable_cond);

    /*fprintf (stderr, "Conky: destroying thread 0x%08X\n", (unsigned)p_timed_thread);*/
    free (p_timed_thread);
    if (addr_of_p_timed_thread)
	*addr_of_p_timed_thread = NULL;
}


/* lock a timed thread for critical section activity */
int
timed_thread_lock (timed_thread* p_timed_thread)
{
    assert (p_timed_thread != NULL);

    return pthread_mutex_lock (&p_timed_thread->cs_mutex);
}


/* unlock a timed thread after critical section activity */
int
timed_thread_unlock (timed_thread* p_timed_thread)
{
    assert (p_timed_thread != NULL);

    return pthread_mutex_unlock (&p_timed_thread->cs_mutex);
}


/* thread waits interval_usecs for runnable_cond to be signaled.  returns 1 if signaled, 
 * -1 on error, and 0 otherwise.  caller should call timed_thread_exit() on any non-zero 
 * return value. */
int 
timed_thread_test (timed_thread* p_timed_thread)
{
    struct timespec abstime, reltime;
    int rc;

    assert (p_timed_thread != NULL);

    /* acquire runnable_cond mutex */
    if (pthread_mutex_lock (&p_timed_thread->runnable_mutex))
	return (-1);  /* could not acquire runnable_cond mutex, so tell caller to exit thread */

    /* get the absolute time in the future we stop waiting for condition to signal */
    clock_gettime (CLOCK_REALTIME, &abstime);
    /* seconds portion of the microseconds interval */
    reltime.tv_sec = (time_t)(p_timed_thread->interval_usecs / 1000000);
    /* remaining microseconds convert to nanoseconds */
    reltime.tv_nsec = (long)((p_timed_thread->interval_usecs % 1000000) * 1000);
    /* absolute future time */
    abstime.tv_sec += reltime.tv_sec;
    abstime.tv_nsec += reltime.tv_nsec;

    /* release mutex and wait until future time for runnable_cond to signal */
    rc = pthread_cond_timedwait (&p_timed_thread->runnable_cond,
			        &p_timed_thread->runnable_mutex,
				&abstime);
    /* mutex re-acquired, so release it */
    pthread_mutex_unlock (&p_timed_thread->runnable_mutex);

    if (rc==0)
	return 1; /* runnable_cond was signaled, so tell caller to exit thread */

    /* tell caller not to exit yet */
    return 0;
}


/* exit a timed thread */
void 
timed_thread_exit (timed_thread* p_timed_thread)
{
    assert (p_timed_thread != NULL);

    pthread_exit (NULL);
}


/* register a timed thread for future destruction via timed_thread_destroy_registered_threads() */
int 
timed_thread_register (timed_thread* p_timed_thread, timed_thread** addr_of_p_timed_thread)
{
    timed_thread_node *p_node;

    assert (p_timed_thread != NULL);
    assert ((addr_of_p_timed_thread == NULL) || (*addr_of_p_timed_thread == p_timed_thread));

    if ((p_node = calloc (sizeof (timed_thread_node), 1)) == 0)
	return 0;
    
    p_node->p_timed_thread = p_timed_thread;
    p_node->addr_of_p_timed_thread = addr_of_p_timed_thread;
    p_node->next = NULL;

    if (!p_timed_thread_list_tail)
    {
	/* first node of empty list */
	p_timed_thread_list_tail = p_node;
	p_timed_thread_list_head = p_node;
    }
    else
    {
	/* add node to tail of non-empty list */
	p_timed_thread_list_tail->next = p_node;
	p_timed_thread_list_tail = p_node;
    }

    return 0;
}


/* destroy all registered timed threads */
void 
timed_thread_destroy_registered_threads (void)
{
    timed_thread_node *p_node, *p_next;

    for (p_node=p_timed_thread_list_head;
	 p_node;
	 p_node=p_next)
    {
	p_next = p_node->next;
	timed_thread_destroy (p_node->p_timed_thread, p_node->addr_of_p_timed_thread);
	free (p_node);
	p_node = NULL;
    }

    p_timed_thread_list_head = NULL;
    p_timed_thread_list_tail = NULL;
}


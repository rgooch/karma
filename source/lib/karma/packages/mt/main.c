/*LINTLIBRARY*/
/*  main.c

    This code provides Multi Threading support.

    Copyright (C) 1995  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains all routines needed for the management of a pool of
  threads which may execute concurrently.


    Written by      Richard Gooch   9-JAN-1995

    Updated by      Richard Gooch   13-JAN-1995

    Last updated by Richard Gooch   24-JAN-1995: Increased number of call
  arguments to 4.


*/
#ifdef OS_Solaris
#  include <thread.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <karma.h>
#include <karma_mt.h>
#include <karma_a.h>
#include <karma_m.h>

#define MAGIC_NUMBER 590322598

#define VERIFY_POOL(pl) {if (pl == NULL) \
{(void) fprintf (stderr, "NULL thread pool passed\n"); \
 a_prog_bug (function_name); } \
if (pl->magic_number != MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid thread pool object\n"); \
 a_prog_bug (function_name); } }

#ifdef OS_Solaris
#  define LOCK_POOL(pl) {if (mutex_trylock (&pl->lock) != 0) \
{(void) fprintf (stderr, "Recursive operation on pool not permitted\n"); \
 a_prog_bug (function_name);} }
#  define UNLOCK_POOL(pl) mutex_unlock (&pl->lock)
#endif

#ifndef LOCK_POOL
#  define LOCK_POOL(pl)
#  define UNLOCK_POOL(pl)
#endif

/*  Private structures  */
struct threadpool_type
{
    unsigned int magic_number;
    unsigned int num_threads;  /*  Number of worker threads: 0 or 2 or more  */
    struct thread_type *threads;
    void *info;
#ifdef OS_Solaris
    mutex_t lock;
    sema_t semaphore;
#endif
};

struct thread_type
{
    KThreadPool pool;
#ifdef OS_Solaris
    thread_t tid;
    mutex_t startlock;
    mutex_t finishedlock;
#endif
    void (*func) (void *pool_info, void *call_info1, void *call_info2,
		  void *call_info3, void *call_info4);
    void *info1;
    void *info2;
    void *info3;
    void *info4;
};


/*  Private functions  */
STATIC_FUNCTION (void init, () );
STATIC_FUNCTION (void *thread_main, (void *arg) );


/*  Public functions follow  */


/*PUBLIC_FUNCTION*/
KThreadPool mt_create_pool (void *pool_info)
/*  [PURPOSE] This routine will create a pool of threads which may have jobs
    launched onto it.
    <pool_info> An arbitrary pointer passed to work functions.
    [MT-LEVEL] Safe
    [RETURNS] A KThreadPool object on success, else NULL.
*/
{
    KThreadPool pool;
    unsigned int count;
    extern char *sys_errlist[];
    static char function_name[] = "mt_create_pool";

    if ( ( pool = (KThreadPool) malloc (sizeof *pool) ) == NULL )
    {
	m_abort (function_name, "thread pool");
    }
    pool->magic_number = MAGIC_NUMBER;
    pool->num_threads = 0;
    pool->info = pool_info;
#ifdef OS_Solaris
    pool->num_threads = sysconf (_SC_NPROCESSORS_ONLN);
#endif
    if (pool->num_threads < 2)
    {
	pool->num_threads = 0;
	return (pool);
    }
#ifdef OS_Solaris
    mutex_init (&pool->lock, USYNC_THREAD, NULL);
    sema_init (&pool->semaphore, pool->num_threads, USYNC_THREAD, NULL);
    if ( ( pool->threads = (struct thread_type *)
	  malloc (sizeof *pool->threads * pool->num_threads) ) == NULL )
    {
	m_abort (function_name, "thread array");
    }
    for (count = 0; count < pool->num_threads; ++count)
    {
	pool->threads[count].pool = pool;
	mutex_init (&pool->threads[count].startlock, USYNC_THREAD, NULL);
	mutex_init (&pool->threads[count].finishedlock, USYNC_THREAD, NULL);
	mutex_lock (&pool->threads[count].startlock);
	pool->threads[count].func = ( void (*) () ) NULL;
	pool->threads[count].info1 = NULL;
	pool->threads[count].info2 = NULL;
	pool->threads[count].info3 = NULL;
	pool->threads[count].info4 = NULL;
	if (thr_create (NULL, 0, thread_main, pool->threads + count,
			THR_NEW_LWP, &pool->threads[count].tid) != 0)
	{
	    (void) fprintf (stderr, "Error creating thread\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
    }
#endif  /*  OS_Solaris  */
    return (pool);
}   /*  End Function mt_create_pool  */

/*PUBLIC_FUNCTION*/
void mt_destroy_pool (KThreadPool pool, flag interrupt)
/*  [PURPOSE] The function will destroy a thread pool.
    <pool) The thread pool.
    <interrupt> If TRUE, any jobs not yet completed will be killed, else the
    function will wait for uncompleted jobs to finish prior to destroying the
    pool.
    [MT-LEVEL] Safe
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    extern char *sys_errlist[];
    static char function_name[] = "mt_destroy_pool";

    VERIFY_POOL (pool);
    FLAG_VERIFY (interrupt);
    if (!interrupt) mt_wait_for_all_jobs (pool);
    LOCK_POOL (pool);
    for (count = 0; count < pool->num_threads; ++count)
    {
#ifdef OS_Solaris
	if (thr_kill (pool->threads[count].tid, SIGKILL) != 0)
	{
	    (void) fprintf (stderr, "Error killing thread\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	mutex_destroy (&pool->threads[count].startlock);
	mutex_destroy (&pool->threads[count].finishedlock);	
#endif
    }
#ifdef OS_Solaris
    sema_destroy (&pool->semaphore);
#endif
    pool->magic_number = 0;
    free ( (char *) pool );
}   /*  End Function mt_destroy_pool  */

/*PUBLIC_FUNCTION*/
unsigned int mt_num_threads (KThreadPool pool)
/*  [PURPOSE] This function will determine the number of threads that may be
    run concurrently in a thread pool.
    <pool> The thread pool.
    [MT-LEVEL] Safe
    [RETURNS] The number of concurrent threads.
*/
{
    static char function_name[] = "mt_num_threads";

    VERIFY_POOL (pool);
    if (pool->num_threads < 2) return (1);
    return (pool->num_threads);
}   /*  End Function mt_num_threads  */

/*PUBLIC_FUNCTION*/
void mt_launch_job (KThreadPool pool,
		    void (*func) (void *pool_info,
				  void *call_info1, void *call_info2,
				  void *call_info3, void *call_info4),
		    void *call_info1, void *call_info2,
		    void *call_info3, void *call_info4)
/*  [PURPOSE] This function will launch a job to a pool of threads.
    <pool> The thread pool.
    <func> The function to execute.
    <call_info1> An arbitrary argument to <<func>>.
    <call_info2> An arbitrary argument to <<func>>.
    <call_info3> An arbitrary argument to <<func>>.
    <call_info4> An arbitrary argument to <<func>>.
    [MT-LEVEL] Safe
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    static char function_name[] = "mt_launch_job";

    VERIFY_POOL (pool);
    if (func == NULL) return;
    LOCK_POOL (pool);
    if (pool->num_threads < 2)
    {
	(*func) (pool->info, call_info1, call_info2, call_info3, call_info4);
	UNLOCK_POOL (pool);
	return;
    }
#ifdef OS_Solaris
    while (sema_wait (&pool->semaphore) == EINTR);
#endif
    /*  One of the threads must be ready now  */
    for (count = 0; count < pool->num_threads; ++count)
    {
#ifdef OS_Solaris
	if (mutex_trylock (&pool->threads[count].finishedlock) == 0)
	{
	    /*  This thread ready for work  */
	    pool->threads[count].func = func;
	    pool->threads[count].info1 = call_info1;
	    pool->threads[count].info2 = call_info2;
	    pool->threads[count].info3 = call_info3;
	    pool->threads[count].info4 = call_info4;
	    mutex_unlock (&pool->threads[count].startlock);
	    UNLOCK_POOL (pool);
	    return;
	}
#endif
    }
    (void) fprintf (stderr, "Error launching job: no free thread!\n");
    a_prog_bug (function_name);
}   /*  End Function mt_launch_job  */

/*PUBLIC_FUNCTION*/
void mt_wait_for_all_jobs (KThreadPool pool)
/*  [PURPOSE] This function will wait for all previously launched jobs to
    complete.
    <pool> The thread pool.
    [MT-LEVEL] Safe
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    static char function_name[] = "mt_wait_for_all_jobs";

    VERIFY_POOL (pool);
    LOCK_POOL (pool);
    for (count = 0; count < pool->num_threads; ++count)
    {
#ifdef OS_Solaris
	mutex_lock (&pool->threads[count].finishedlock);
	mutex_unlock (&pool->threads[count].finishedlock);
#endif  /*  OS_Solaris  */
    }
    UNLOCK_POOL (pool);
}   /*  End Function mt_wait_for_all_jobs  */


/*  Private functions follow  */

static void *thread_main (void *arg)
/*  [PURPOSE] This function is the entry point for a thread.
    [RETURNS] NULL on error, else never returns.
*/
{
    struct thread_type *thread = (struct thread_type *) arg;
    KThreadPool parent = thread->pool;

    while (TRUE)
    {
#ifdef OS_Solaris
	mutex_lock (&thread->startlock);
	(*thread->func) (parent->info, thread->info1, thread->info2,
			 thread->info3, thread->info4);
	mutex_unlock (&thread->finishedlock);
	sema_post (&parent->semaphore);
#endif
    }
}   /*  End Function thread_main  */

/*LINTLIBRARY*/
/*  main.c

    This code provides Multi Threading support.

    Copyright (C) 1995-1996  Richard Gooch

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

    Updated by      Richard Gooch   24-JAN-1995: Increased number of call
  arguments to 4.

    Updated by      Richard Gooch   1-FEB-1995: Added check for MT_MAX_THREADS
  environment variable.

    Updated by      Richard Gooch   10-FEB-1995: IRIX5 support.

    Updated by      Richard Gooch   23-FEB-1995: IRIX6 support and bugfix.

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Updated by      Richard Gooch   23-JUL-1995: Created <mt_destroy_all_pools>

    Updated by      Richard Gooch   9-AUG-1995: Partially worked around a bug
  in thr_kill(3): sending SIGKILL kill the entire process.

    Updated by      Richard Gooch   21-AUG-1995: Above mentioned bug has been
  defined a "feature" by Sun support: used SIGTERM handler to kill thread to
  cope with this lossage.

    Updated by      Richard Gooch   31-AUG-1995: Made use of  __ateachexit
  routine for IRIX. Yuk!

    Updated by      Richard Gooch   24-JAN-1996: Created <mt_setlock> routine.
  Added thread_info parameter to thread function and created
  <mt_new_thread_info> routine.

    Updated by      Richard Gooch   25-JAN-1996: Increased arena space from
  64k to 256k for IRIX. Stupid limitations.

    Updated by      Richard Gooch   18-FEB-1996: Made child process die when
  parent dies with IRIX.

    Updated by      Richard Gooch   12-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   2-MAY-1996: Created <mt_get_shared_pool>
  routine.

    Last updated by Richard Gooch   3-JUN-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#ifdef OS_Solaris
#  include <thread.h>
#endif
#if defined(OS_IRIX5) || defined(OS_IRIX6)
#  define OS_IRIX
#endif
#ifdef OS_IRIX
#  include <ulocks.h>
#  include <sys/types.h>
#  include <sys/prctl.h>
#endif
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <math.h>
#include <errno.h>
#include <signal.h>
#include <karma.h>
#include <karma_mt.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_r.h>
#include <karma_c.h>
#include <os.h>

#define MAGIC_NUMBER 590322598

#define VERIFY_POOL(pl) {if (pl == NULL) \
{(void) fprintf (stderr, "NULL thread pool passed\n"); \
 a_prog_bug (function_name); } \
if (pl->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid thread pool object\n"); \
 a_prog_bug (function_name); } }

#ifdef OS_Solaris
#  define LOCK_TYPE mutex_t
#  define SEMAPHORE_TYPE sema_t
#  define TID_TYPE thread_t
#  define LOCK_POOL(pl) {if (mutex_trylock (&pl->lock) != 0) \
{(void) fprintf (stderr, "Recursive operation on pool not permitted\n"); \
 a_prog_bug (function_name);} }
#  define UNLOCK_POOL(pl) mutex_unlock (&pl->lock)
#  define FUNC_LOCK mutex_lock (&func_lock)
#  define FUNC_UNLOCK mutex_unlock (&func_lock)
#  define HAS_FUNC_LOCKS
#  define LOCK(lock) mutex_lock (&lock)
#  define UNLOCK(lock) mutex_unlock (&lock)
#  define TRYLOCK(lock) if (mutex_trylock (&lock) == 0)
#  define THREAD_RETURN void *
#  define HAS_THREADS
#endif

#ifdef OS_IRIX
#  define LOCK_TYPE ulock_t
#  define SEMAPHORE_TYPE usema_t *
#  define TID_TYPE pid_t
#  define LOCK_POOL(pl) {if (uscsetlock (pl->lock, 1) != 1) \
{(void) fprintf (stderr, "Recursive operation on pool not permitted\n"); \
 a_prog_bug (function_name);} }
#  define UNLOCK_POOL(pl) usunsetlock (pl->lock)
/*  Because IRIX locks are created we cannot use the compiler to initialise
    them. Have to forego function locks.
#  define FUNC_LOCK uswsetlock (func_lock)
#  define FUNC_UNLOCK usunsetlock (func_lock)
*/
#  define FUNC_LOCK
#  define FUNC_UNLOCK
#  define LOCK(lock) uswsetlock (lock, 5)
#  define UNLOCK(lock) usunsetlock (lock)
#  define TRYLOCK(lock) if (uscsetlock (lock, 5) == 1)
#  define THREAD_RETURN void
#  define HAS_THREADS
#endif

#ifndef LOCK_POOL
#  define LOCK_POOL(pl)
#  define UNLOCK_POOL(pl)
#  define FUNC_LOCK
#  define FUNC_UNLOCK
#endif

/*  Private structures  */
struct threadpool_type
{
    unsigned int magic_number;
    unsigned int num_threads;  /*  Number of worker threads: 0 or 2 or more  */
    struct thread_type *threads;
    KCallbackFunc callback_handle;
    void *info;
    char *thread_info_buffer;
    uaddr thread_info_buf_size;
    uaddr thread_info_size;
#ifdef HAS_THREADS
    LOCK_TYPE lock;
    LOCK_TYPE synclock;
    SEMAPHORE_TYPE semaphore;
#endif
};

struct thread_type
{
    KThreadPool pool;
#ifdef HAS_THREADS
    TID_TYPE tid;
    LOCK_TYPE startlock;
    LOCK_TYPE finishedlock;
#endif
    void (*func) (void *pool_info, void *call_info1, void *call_info2,
		  void *call_info3, void *call_info4, void *thread_info);
    void *info1;
    void *info2;
    void *info3;
    void *info4;
    void *thread_info;
#ifdef OS_Solaris
    flag exit;
#endif
};


/*  Private data  */
KCallbackList destroy_list = NULL;
#ifdef OS_IRIX
static usptr_t *arena = NULL;
#endif
KThreadPool shared_pool = NULL;

/*  Private functions  */
#ifdef HAS_THREADS
STATIC_FUNCTION (THREAD_RETURN thread_main, (void *arg) );
#endif
STATIC_FUNCTION (flag destroy_callback,
		 (KThreadPool pool, void *client1_data,
		  flag *interrupt, void *client2_data) );
STATIC_FUNCTION (void exit_callback, () );


/*  Public functions follow  */


/*PUBLIC_FUNCTION*/
KThreadPool mt_create_pool (void *pool_info)
/*  [SUMMARY] Create a pool of threads which may have jobs launched onto it.
    <pool_info> An arbitrary pointer passed to work functions.
    [NOTE] The environment variable MT_MAX_THEADS may be used to limit the
    number of threads used.
    [MT-LEVEL] Safe under Solaris 2.
    [RETURNS] A KThreadPool object on success, else NULL.
*/
{
    KThreadPool pool;
#ifdef HAS_THREADS
    unsigned int count;
#endif
    char *env;
    extern KCallbackList destroy_list;
    extern char *sys_errlist[];
#ifdef HAS_FUNC_LOCKS
    static LOCK_TYPE func_lock;
#endif
#ifdef OS_IRIX
    char txt[STRING_LENGTH];
    extern usptr_t *arena;
#endif
    static flag first_time = TRUE;
    static unsigned int max_threads = 0;
    static char function_name[] = "mt_create_pool";

    FUNC_LOCK;
    if (first_time)
    {
	first_time = FALSE;
	/*  Register function to be called upon processing of  exit(3)  */
#ifdef OS_IRIX
	if (__ateachexit (exit_callback) != 0)
	{
	    (void) fprintf (stderr, "%s: error registering exit function\n",
			    function_name);
	}
#else
#  ifdef HAS_ATEXIT
	if (atexit (exit_callback) != 0)
	{
	    (void) fprintf (stderr, "%s: error registering exit function\n",
			    function_name);
	}
#  endif
#  ifdef HAS_ON_EXIT
	on_exit (exit_callback, (caddr_t) NULL);
#  endif
#endif
	if ( ( env = r_getenv ("MT_MAX_THREADS") ) == NULL )
	{
	    max_threads = 0;
	}
	else
	{
	    max_threads = atoi (env);
	    if (max_threads < 1) max_threads = 1;
	    (void) fprintf (stderr,
			    "Forcing maximum number of threads to: %u\n",
			    max_threads);
	}
#ifdef OS_IRIX
	if (usconfig (CONF_LOCKTYPE, US_NODEBUG) == -1)
	{
	    (void) fprintf (stderr, "Error configuring\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (usconfig (CONF_ARENATYPE, US_SHAREDONLY) == -1)
	{
	    (void) fprintf (stderr, "Error configuring\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (usconfig (CONF_INITSIZE, 262144) == -1)
	{
	    (void) fprintf (stderr, "Error configuring\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (usconfig (CONF_INITUSERS, 256) == -1)
	{
	    (void) fprintf (stderr, "Error configuring\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	(void) sprintf ( txt, "/tmp/karma_arena-%d", getpid () );
	if ( ( arena = usinit (txt) ) == NULL )
	{
	    (void) fprintf (stderr, "Error creating arena\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
#endif
    }
    FUNC_UNLOCK;
    if ( ( pool = (KThreadPool) malloc (sizeof *pool) ) == NULL )
    {
	m_abort (function_name, "thread pool");
    }
    pool->magic_number = MAGIC_NUMBER;
    pool->num_threads = 0;
    pool->info = pool_info;
    pool->thread_info_buffer = NULL;
    pool->thread_info_buf_size = 0;
    pool->thread_info_size = 0;
#ifdef OS_Solaris
    pool->num_threads = sysconf (_SC_NPROCESSORS_ONLN);
#endif
#ifdef OS_IRIX
    pool->num_threads = sysconf (_SC_NPROC_ONLN);
#endif
    if (max_threads > 0)
    {
	if (pool->num_threads > max_threads) pool->num_threads = max_threads;
    }
    if (pool->num_threads < 2) pool->num_threads = 0;
    /*  Must create locks and semaphores even if number of threads forced to
	zero.  */
#ifdef OS_Solaris
    mutex_init (&pool->lock, USYNC_THREAD, NULL);
    mutex_init (&pool->synclock, USYNC_THREAD, NULL);
    sema_init (&pool->semaphore, pool->num_threads, USYNC_THREAD, NULL);
#endif
#ifdef OS_IRIX
    if ( ( pool->lock = usnewlock (arena) ) == NULL )
    {
	(void) fprintf (stderr, "Error creating pool lock\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    if ( ( pool->synclock = usnewlock (arena) ) == NULL )
    {
	(void) fprintf (stderr, "Error creating synclock\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    if ( ( pool->semaphore = usnewsema (arena, pool->num_threads) ) == NULL )
    {
	(void) fprintf (stderr, "Error creating semaphore\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
#endif
    pool->callback_handle = c_register_callback (&destroy_list,
						 destroy_callback, pool,
						 NULL, FALSE,
						 NULL, FALSE, FALSE);
    if (pool->num_threads < 2) return (pool);
#ifdef OS_Solaris
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
	LOCK (pool->threads[count].startlock);
	pool->threads[count].func = ( void (*) () ) NULL;
	pool->threads[count].info1 = NULL;
	pool->threads[count].info2 = NULL;
	pool->threads[count].info3 = NULL;
	pool->threads[count].info4 = NULL;
	pool->threads[count].thread_info = NULL;
	pool->threads[count].exit = FALSE;
	if (thr_create (NULL, 0, thread_main, pool->threads + count,
			THR_NEW_LWP, &pool->threads[count].tid) != 0)
	{
	    (void) fprintf (stderr, "Error creating thread\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
    }
#endif  /*  OS_Solaris  */
#ifdef OS_IRIX
    if ( ( pool->threads = (struct thread_type *)
	  malloc (sizeof *pool->threads * pool->num_threads) ) == NULL )
    {
	m_abort (function_name, "thread array");
    }
    for (count = 0; count < pool->num_threads; ++count)
    {
	pool->threads[count].pool = pool;
	if ( ( pool->threads[count].startlock = usnewlock (arena) ) == NULL )
	{
	    (void) fprintf (stderr, "Error creating startlock\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if ( ( pool->threads[count].finishedlock = usnewlock (arena) ) ==NULL )
	{
	    (void) fprintf (stderr, "Error creating finishedlock\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	LOCK (pool->threads[count].startlock);
	pool->threads[count].func = ( void (*) () ) NULL;
	pool->threads[count].info1 = NULL;
	pool->threads[count].info2 = NULL;
	pool->threads[count].info3 = NULL;
	pool->threads[count].info4 = NULL;
	pool->threads[count].thread_info = NULL;
	if ( ( pool->threads[count].tid = sproc (thread_main, PR_SALL,
						 pool->threads + count) )
	    == -1 )
	{
	    (void) fprintf (stderr, "Error creating thread\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
    }
#endif  /*  OS_IRIX  */
    return (pool);
}   /*  End Function mt_create_pool  */

/*PUBLIC_FUNCTION*/
KThreadPool mt_get_shared_pool ()
/*  [SUMMARY] Create or get the shared thread pool.
    [PURPOSE] This routine will get a pool of threads which may be shared
    between several services within a process. This shared pool may be used to
    reduce the number of thread pools created. Creating large numbers of thread
    pools may consume significant system resources on some platforms. The first
    time this routine is called the shared pool is created.
    [NOTE] The environment variable MT_MAX_THEADS may be used to limit the
    number of threads used.
    [NOTE] This shared pool cannot be used with the [<mt_new_thread_info>] and
    [<my_get_thread_info>] routines.
    [MT-LEVEL] Safe under Solaris 2.
    [RETURNS] The shared KThreadPool object on success. On failure the process
    aborts.
*/
{
    extern KThreadPool shared_pool;
#ifdef HAS_FUNC_LOCKS
    static LOCK_TYPE func_lock;
#endif
    static char function_name[] = "mt_get_shared_pool";

    FUNC_LOCK;
    if (shared_pool == NULL)
    {
	if ( ( shared_pool = mt_create_pool (NULL) ) == NULL )
	{
	    m_abort (function_name, "shared thread pool");
	}
    }
    FUNC_UNLOCK;
    return (shared_pool);
}   /*  End Function mt_get_shared_pool  */

/*PUBLIC_FUNCTION*/
void mt_destroy_pool (KThreadPool pool, flag interrupt)
/*  [SUMMARY] This routine will destroy a thread pool.
    <pool> The thread pool.
    <interrupt> If TRUE, any jobs not yet completed will be killed, else the
    function will wait for uncompleted jobs to finish prior to destroying the
    pool.
    [MT-LEVEL] Safe.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    extern char *sys_errlist[];
#ifdef OS_IRIX
    extern usptr_t *arena;
#endif
    extern KThreadPool shared_pool;
#ifdef HAS_FUNC_LOCKS
    static LOCK_TYPE func_lock;
#endif
    static char function_name[] = "mt_destroy_pool";

    VERIFY_POOL (pool);
    FUNC_LOCK;
    if (pool == shared_pool) shared_pool = NULL;
    FUNC_UNLOCK;
    FLAG_VERIFY (interrupt);
    if (!interrupt) mt_wait_for_all_jobs (pool);
    LOCK_POOL (pool);
    for (count = 0; count < pool->num_threads; ++count)
    {
#ifdef OS_Solaris
	/*  A bug in Solaris 2.4 caused thr_kill(3) to bugger the whole
	    process, so I try a thr_exit(3) instead.  */
	TRYLOCK (pool->threads[count].finishedlock)
	{
	    /*  Thread not busy  */
	    pool->threads[count].exit = TRUE;
	    /*  Fire away  */
	    UNLOCK (pool->threads[count].startlock);
	    (void) thr_join (pool->threads[count].tid, NULL, NULL);
	}
	else
	{
	    if (thr_kill (pool->threads[count].tid, SIGTERM) != 0)
	    {
		(void) fprintf (stderr, "Error killing thread\t%s\n",
				sys_errlist[errno]);
		exit (RV_SYS_ERROR);
	    }
	    (void) thr_join (pool->threads[count].tid, NULL, NULL);
	}
	mutex_destroy (&pool->threads[count].startlock);
	mutex_destroy (&pool->threads[count].finishedlock);	
#endif
#ifdef OS_IRIX
	if (kill (pool->threads[count].tid, SIGKILL) != 0)
	{
	    (void) fprintf (stderr, "Error killing thread\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	usfreelock (pool->threads[count].startlock, arena);
	usfreelock (pool->threads[count].finishedlock, arena);
#endif
    }
#ifdef OS_Solaris
    sema_destroy (&pool->semaphore);
#endif
#ifdef OS_IRIX
    usfreesema (pool->semaphore, arena);
#endif
    if (pool->thread_info_buf_size > 0) m_free (pool->thread_info_buffer);
    pool->magic_number = 0;
    c_unregister_callback (pool->callback_handle);
    pool->callback_handle = NULL;
    free ( (char *) pool );
}   /*  End Function mt_destroy_pool  */

/*PUBLIC_FUNCTION*/
void mt_destroy_all_pools (flag interrupt)
/*  [SUMMARY] This routine will destroy all thread pools.
    <interrupt> If TRUE, any jobs not yet completed will be killed, else the
    function will wait for uncompleted jobs to finish prior to destroying the
    pools.
    [MT-LEVEL] Safe.
    [RETURNS] Nothing.
*/
{
    extern KCallbackList destroy_list;

    (void) c_call_callbacks (destroy_list, (void *) &interrupt);
    if (interrupt) c_destroy_list (destroy_list);
}   /*  End Function mt_destroy_all_pools  */

/*PUBLIC_FUNCTION*/
unsigned int mt_num_threads (KThreadPool pool)
/*  [SUMMARY] Get the number of threads in a thread pool.
    [PURPOSE] This function will determine the number of threads that may be
    run concurrently in a thread pool.
    <pool> The thread pool.
    [MT-LEVEL] Safe.
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
				  void *call_info3, void *call_info4,
				  void *thread_info),
		    void *call_info1, void *call_info2,
		    void *call_info3, void *call_info4)
/*  [SUMMARY] Launch a job onto a thread pool.
    [PURPOSE] This function will launch a job to a pool of threads, running the
    job on the first available thread.
    <pool> The thread pool.
    <func> The function to execute. The prototype function is [<MT_PROTO_func>]
    <call_info1> An arbitrary argument to <<func>>.
    <call_info2> An arbitrary argument to <<func>>.
    <call_info3> An arbitrary argument to <<func>>.
    <call_info4> An arbitrary argument to <<func>>.
    [NOTE] Jobs must not modify any signal actions or masks.
    [MT-LEVEL] Safe.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
#ifdef HAS_THREADS
    char *thread_info;
#endif
    static char function_name[] = "mt_launch_job";

    VERIFY_POOL (pool);
    if (func == NULL) return;
    LOCK_POOL (pool);
    if (pool->num_threads < 2)
    {
	(*func) (pool->info, call_info1, call_info2, call_info3, call_info4,
		 (void *) pool->thread_info_buffer);
	UNLOCK_POOL (pool);
	return;
    }
#ifdef OS_Solaris
    while (sema_wait (&pool->semaphore) == EINTR);
#endif
#ifdef OS_IRIX
    uspsema (pool->semaphore);
#endif
    /*  One of the threads must be ready now  */
    for (count = 0; count < pool->num_threads; ++count)
    {
#ifdef HAS_THREADS
	TRYLOCK (pool->threads[count].finishedlock)
	{
	    /*  This thread ready for work  */
	    pool->threads[count].func = func;
	    pool->threads[count].info1 = call_info1;
	    pool->threads[count].info2 = call_info2;
	    pool->threads[count].info3 = call_info3;
	    pool->threads[count].info4 = call_info4;
	    /*  Ensure thread has correct private data pointer  */
	    if (pool->thread_info_buffer == NULL)
	    {
		pool->threads[count].thread_info = NULL;
	    }
	    else
	    {
		thread_info = pool->thread_info_buffer;
		thread_info += count * pool->thread_info_size;
		pool->threads[count].thread_info = thread_info;
	    }
	    /*  Start thread running  */
	    UNLOCK (pool->threads[count].startlock);
	    UNLOCK_POOL (pool);
	    return;
	}
#endif
    }
    (void) fprintf (stderr, "Error launching job: no free thread!\n");
    a_prog_bug (function_name);
}   /*  End Function mt_launch_job  */

/*PUBLIC_FUNCTION*/
void mt_setlock (KThreadPool pool, flag lock)
/*  [SUMMARY] Set a lock on a thread pool.
    [PURPOSE] This function will lock a thread pool such that no other thread
    can lock the pool at the same time. This does not prevent other threads
    from running, nor new jobs from being launched, it merely prevents them
    from aquiring the lock.
    <pool> The thread pool to lock.
    <lock> If TRUE the pool is locked. If FALSE the pool is unlocked and any
    other threads wishing to lock the pool may do so (one at a time of course).
    [MT-LEVEL] Safe.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "mt_setlock";

    VERIFY_POOL (pool);
    FLAG_VERIFY (lock);
#ifdef HAS_THREADS
    if (lock) LOCK (pool->synclock);
    else UNLOCK (pool->synclock);
#endif
}   /*  End Function mt_setlock  */

/*PUBLIC_FUNCTION*/
void mt_new_thread_info (KThreadPool pool, void *info, uaddr size)
/*  [SUMMARY] Register new thread information for the threads in a pool.
    <pool> The thread pool.
    <info> A pointer to the thread information array. If NULL and <<size>> is
    not 0 then the routine will allocate an array of sufficient size.
    <size> The size (per thread) in bytes of the thread information. When
    threads are executing each is guaranteed to have a private working space of
    this size.
    [MT-LEVEL] Safe.
    [RETURNS] Nothing. On failure the process aborts.
*/
{
    uaddr num_threads;
    extern KThreadPool shared_pool;
    static char function_name[] = "mt_new_thread_info";

    VERIFY_POOL (pool);
    if (size < 1)
    {
	(void) fprintf (stderr, "Illegal size: %lu\n", size);
	a_prog_bug (function_name);
    }
    if (pool == shared_pool)
    {
	(void) fprintf (stderr, "Operation illegal for shared thread pool\n");
	a_prog_bug (function_name);
    }
    num_threads = mt_num_threads (pool);
    LOCK_POOL (pool);
    pool->thread_info_size = size;
    if (info == NULL)
    {
	/*  Need an internally allocated array  */
	if (size * num_threads <= pool->thread_info_buf_size)
	{
	    UNLOCK_POOL (pool);
	    return;
	}
	/*  Existing buffer not big enough: free and allocate  */
	if (pool->thread_info_buf_size > 0) m_free (pool->thread_info_buffer);
	if ( ( info = (void *) m_alloc (size * num_threads) ) == NULL )
	{
	    m_abort (function_name, "thread information array");
	}
	pool->thread_info_buf_size = size * num_threads;
	pool->thread_info_buffer = (char *) info;
	UNLOCK_POOL (pool);
	return;
    }
    /*  Array supplied  */
    if (pool->thread_info_buf_size > 0)
    {
	 m_free (pool->thread_info_buffer);
	 pool->thread_info_buf_size = 0;
     }
    pool->thread_info_buffer = (char *) info;
    UNLOCK_POOL (pool);
}   /*  End Function mt_new_thread_info  */

/*PUBLIC_FUNCTION*/
void *mt_get_thread_info (KThreadPool pool)
/*  [SUMMARY] Get the thread information pointer for a pool of threads.
    <pool> The thread pool.
    [MT-LEVEL] Safe.
    [RETURNS] A pointer to the thread information array.
*/
{
    extern KThreadPool shared_pool;
    static char function_name[] = "mt_get_thread_info";

    VERIFY_POOL (pool);
    if (pool == shared_pool)
    {
	(void) fprintf (stderr, "Operation illegal for shared thread pool\n");
	a_prog_bug (function_name);
    }
    return ( (char *) pool->thread_info_buffer );
}   /*  End Function mt_get_thread_info  */

/*PUBLIC_FUNCTION*/
void mt_wait_for_all_jobs (KThreadPool pool)
/*  [SUMMARY] Wait for all previously launched jobs to complete.
    <pool> The thread pool.
    [MT-LEVEL] Safe.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    static char function_name[] = "mt_wait_for_all_jobs";

    VERIFY_POOL (pool);
    LOCK_POOL (pool);
    for (count = 0; count < pool->num_threads; ++count)
    {
#ifdef HAS_THREADS
	LOCK (pool->threads[count].finishedlock);
	UNLOCK (pool->threads[count].finishedlock);
#endif
    }
    UNLOCK_POOL (pool);
}   /*  End Function mt_wait_for_all_jobs  */


/*  Private functions follow  */

#ifdef HAS_THREADS
static THREAD_RETURN thread_main (void *arg)
/*  [PURPOSE] This function is the entry point for a thread.
    [RETURNS] NULL on error, else never returns.
*/
{
    struct thread_type *thread = (struct thread_type *) arg;
    KThreadPool parent = thread->pool;
    extern char *sys_errlist[];

#  ifdef OS_Solaris
    /*  Set up sigTERM handler  */
    if ( (long) signal (SIGTERM, ( void (*) () ) thr_exit) == -1 )
    {
	(void) fprintf (stderr, "Error setting sigTERM handler\t%s\n",
			sys_errlist[errno]);
	(void) exit (RV_SYS_ERROR);
    }
#  endif
#  ifdef OS_IRIX
    /*  Make child die when parent dies  */
    if (prctl (PR_TERMCHILD) == -1)
    {
	(void) fprintf (stderr, "Error controlling process\t%s\n",
			sys_errlist[errno]);
	(void) exit (RV_SYS_ERROR);
    }
#  endif
    while (TRUE)
    {
#  ifdef OS_Solaris
	LOCK (thread->startlock);
	if (thread->exit) thr_exit (NULL);
	(*thread->func) (parent->info, thread->info1, thread->info2,
			 thread->info3, thread->info4, thread->thread_info);
	if (thread->exit) thr_exit (NULL);
	UNLOCK (thread->finishedlock);
	sema_post (&parent->semaphore);
#  endif
#  ifdef OS_IRIX
	LOCK (thread->startlock);
	(*thread->func) (parent->info, thread->info1, thread->info2,
			 thread->info3, thread->info4, thread->thread_info);
	UNLOCK (thread->finishedlock);
	usvsema (parent->semaphore);
#  endif
    }
}   /*  End Function thread_main  */
#endif  /*  HAS_THREADS  */

static flag destroy_callback (KThreadPool pool, void *client1_data,
			      flag *interrupt, void *client2_data)
/*  [PURPOSE] This routine is the destroy callback.
    <pool> The thread pool to destroy.
    <client1_data> Ignored.
    <interrupt> Pointer to value passed to <<mt_destroy_pool>>.
    <client2_data> Ignored.
    [RETURNS] FALSE.
*/
{
    static char function_name[] = "destroy_callback";

    VERIFY_POOL (pool);
    mt_destroy_pool (pool, *interrupt);
    return (FALSE);
}   /*  End Function destroy_callback  */

static void exit_callback ()
/*  [PURPOSE] This routine is the exit callback, called when exit(3) is called.
    [RETURNS] Nothing.
*/
{
#ifdef OS_IRIX
    if (getuid () == 465) (void) fprintf (stderr, "exit_callback...\n");
#endif
    mt_destroy_all_pools (TRUE);
}   /*  End Function exit_callback  */

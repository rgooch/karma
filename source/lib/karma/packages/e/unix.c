/*LINTLIBRARY*/
/*  unix.c

    This code provides KPeriodicEvent* objects.

    Copyright (C) 1996  Richard Gooch

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

/*  This file contains all routines needed for the creation, manipulation and
  deletion of KPeriodicEvent* objects. This file contains the Unix timer code.


    Written by      Richard Gooch   30-NOV-1996

    Last updated by Richard Gooch   30-NOV-1996


*/

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <os.h>
#ifdef HAS_ITIMER
#  include <sys/time.h>
#  include <signal.h>
#endif
#include <karma.h>
#include <karma_e.h>


/*  Private functions  */
#ifdef HAS_ITIMER
STATIC_FUNCTION (flag start_timer,
		 (KPeriodicEventList list, void **timer_info,
		  unsigned long interval_us, unsigned long interval_s) );
STATIC_FUNCTION (void stop_timer,
		 (KPeriodicEventList list, void **timer_info) );
STATIC_FUNCTION (void block_sigalrm,
		 (KPeriodicEventList list, void **timer_info, flag block) );
STATIC_FUNCTION (void sig_handler, (int sig) );
#endif


/*  Private data  */
#ifdef HAS_ITIMER
static KPeriodicEventList running_list = NULL;
#endif


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KPeriodicEventList e_unix_create_list (unsigned long interval_us,
				       unsigned long interval_s,
				       void *list_info)
/*  [SUMMARY] Create a periodic event management list under Unix.
    <interval_us> The microsecond component time between timeouts on this list.
    <interval_s> The second component time between timeouts on this list.
    <list_info> An arbitrary pointer passed to event functions on this list.
    [NOTE] The specified interval may be rounded up to the system timer
    resolution (10ms on many systems).
    [RETURNS] A KPeriodicEventList on success, else the process aborts.
*/
{
#ifndef HAS_ITIMER
    static char function_name[] = "e_unix_create_list";
#endif

#ifdef HAS_ITIMER
    return e_create_list (start_timer, stop_timer, block_sigalrm,
			  NULL, interval_us, interval_s, list_info);
#else
    fprintf (stderr, "%s: no itimer support\n", function_name);
    return (NULL);
#endif
}   /*  End Function e_unix_create_list  */

/*PUBLIC_FUNCTION*/
void e_unix_dispatch_events (unsigned int dispatch_level)
/*  [SUMMARY] Dispatch events for Unix.
    <dispatch_level> The level of events that may dispatched. Event functions
    at the same level or higher are dispatched.
    [RETURNS] Nothing.
*/
{
#ifdef HAS_ITIMER
    if (running_list != NULL) e_dispatch_events (running_list, dispatch_level,
						 FALSE);
#endif
}   /*  End Function e_unix_dispatch_events  */


/*  Private functions follow  */

#ifdef HAS_ITIMER
static flag start_timer (KPeriodicEventList list, void **timer_info,
			 unsigned long interval_us, unsigned long interval_s)
/*  [SUMMARY] Start timer using Unix interval timer.
    [NOTE] This routine must not be called by the application, it is for
    internal use by the [<e>] package.
    <list> The event list.
    <timer_info> A pointer to an arbitrary information pointer.
    <interval_us> The microsecond component time between timeouts on this list.
    <interval_s> The second component time between timeouts on this list.
    [RETURNS] TRUE on success, else FALSE.
*/
{
#ifdef HAS_ITIMER
    struct sigaction new_action;
    struct itimerval timeout;
    extern char *sys_errlist[];
#endif
    static char function_name[] = "__e_unix_start_timer";

#ifdef HAS_ITIMER
    if (running_list != NULL)
    {
	fprintf (stderr, "%s: timer already running\n", function_name);
	return (FALSE);
    }
    running_list = list;
    sigemptyset (&new_action.sa_mask);
    new_action.sa_handler = sig_handler;
    if (sigaction (SIGALRM, &new_action, (struct sigaction *) NULL) != 0)
    {
	fprintf (stderr, "Error setting SIGALRM handler\t%s\n",
		 sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    timeout.it_interval.tv_usec = interval_us;
    timeout.it_interval.tv_sec = interval_s;
    timeout.it_value.tv_usec = interval_us;
    timeout.it_value.tv_sec = interval_s;
    if (setitimer (ITIMER_REAL, &timeout, NULL) != 0)
    {
	fprintf (stderr, "Error setting timer\t%s\n", sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    return (TRUE);
#else
    fprintf (stderr, "%s: no itimer support\n", function_name);
    return (FALSE);
#endif
}   /*  End Function start_timer  */

static void stop_timer (KPeriodicEventList list, void **timer_info)
/*  [SUMMARY] Stop the Unix interval timer.
    [NOTE] This routine must not be called by the application, it is for
    internal use by the [<e>] package.
    <list> The event list.
    <timer_info> A pointer to an arbitrary information pointer.
    [RETURNS] Nothing.
*/
{
#ifdef HAS_ITIMER
    struct itimerval timeout;
    extern char *sys_errlist[];
#endif
    static char function_name[] = "__e_unix_stop_timer";

#ifdef HAS_ITIMER
    if (running_list == NULL)
    {
	fprintf (stderr, "%s: timer not running\n", function_name);
	return;
    }
    if (list != running_list)
    {
	fprintf (stderr, "%s: running list: %p is not list: %p\n",
		 function_name, running_list, list);
	abort ();
    }
    timeout.it_interval.tv_usec = 0;
    timeout.it_interval.tv_sec = 0;
    timeout.it_value.tv_usec = 0;
    timeout.it_value.tv_sec = 0;
    if (setitimer (ITIMER_REAL, &timeout, NULL) != 0)
    {
	fprintf (stderr, "Error setting timer\t%s\n", sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
#else
    fprintf (stderr, "%s: no itimer support\n", function_name);
#endif
}   /*  End Function stop_timer  */

static void block_sigalrm (KPeriodicEventList list, void **timer_info,
			   flag block)
/*  [SUMMARY] Block/unblock Unix timer signal (SIGALRM).
    [NOTE] This routine must not be called by the application, it is for
    internal use by the [<e>] package.
    <list> The event list.
    <timer_info> A pointer to an arbitrary information pointer.
    <block> If TRUE, the signal is blocked, else it is unblocked.
    [RETURNS] Nothing.
*/
{
#ifdef HAS_ITIMER
    sigset_t mask;
    extern char *sys_errlist[];
#endif
    static char function_name[] = "__e_unix_block_sigalrm";

#ifdef HAS_ITIMER
    if (sigemptyset (&mask) != 0)
    {
	fprintf (stderr, "%s: error clearing signal mask\t%s\n",
		 function_name, sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    if (sigaddset (&mask, SIGALRM) != 0)
    {
	fprintf (stderr, "Error adding SIGALRM to signal mask\t%s\n",
		 sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    if (sigprocmask (block ? SIG_BLOCK : SIG_UNBLOCK, &mask, (sigset_t *) NULL)
	!= 0)
    {
	fprintf (stderr, "Error setting signal mask\t%s\n",
		 sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
#else
    fprintf (stderr, "%s: no itimer support\n", function_name);
#endif
}   /*  End Function block_sigalrm  */

static void sig_handler (int sig)
/*  [SUMMARY] SIGALRM handler.
    <sig> The signal.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "__e_unix_sig_handler";

    if (running_list == NULL)
    {
	fprintf (stderr, "%s: spurious SIGALRM\n", function_name);
	return;
    }
    e_dispatch_events (running_list, DISPATCH_ASYNCHRONOUS, TRUE);
}   /*  End Function sig_handler  */
#endif

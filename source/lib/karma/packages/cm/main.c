/*LINTLIBRARY*/
/*  cm.c

    This code provides child process management.

    Copyright (C) 1992,1993,1994,1995  Richard Gooch

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

/*  This file contains all routines needed for the management of child
  processes.


    Written by      Richard Gooch   10-OCT-1992

    Updated by      Richard Gooch   14-OCT-1992

    Updated by      Richard Gooch   24-DEC-1992: Added  cm_poll_silent  routine

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   10-MAY-1993: Added #ifdef for SGImips
  so that ANSI C would be happy.

    Updated by      Richard Gooch   24-MAY-1993: Added #ifdef for hp9000.

    Updated by      Richard Gooch   5-MAY-1994: Removed #ifdef for SGImips.

    Updated by      Richard Gooch   3-OCT-1994: Added  #include <sys/rusage.h>
  for sun4sol2.

    Updated by      Richard Gooch   4-OCT-1994: Changed to  waitpid  routine if
  HAS_WAIT3  not defined in order to avoid having to link with buggy UCB
  compatibility library in Slowaris 2.3

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/cm/main.c

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>

    Last updated by Richard Gooch   5-MAY-1995: Placate SGI compiler.


*/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <os.h>
#ifdef CAN_FORK
#  include <signal.h>
#  include <sys/wait.h>
#  include <sys/time.h>
#  include <sys/resource.h>
#endif  /*  CAN_FORK  */
#include <karma.h>
#include <karma_cm.h>
#include <karma_m.h>
#include <karma_a.h>

#ifdef CAN_FORK

/*  Structure and private data to manage child processes  */
struct child_pid_type
{
    int pid;
    void (*stop_func) ();
    void (*term_func) ();
    void (*exit_func) ();
    struct child_pid_type *next;
    struct child_pid_type *prev;
};
static struct child_pid_type *child_pid_list = NULL;

/*  Local functions  */
static void init_sig_child_handler ();
static void sig_child_handler ();

#endif  /*  CAN_FORK  */

/*  Private functions follow  */

static void init_sig_child_handler ()
/*  This routine will install (only the first time it is called) the SIGCHLD
    signal handler  sig_child_handler  .
    The routine returns nothing.
*/
{
#ifdef CAN_FORK
    extern char *sys_errlist[];
    static flag installed = FALSE;

    if (installed != TRUE)
    {
	/*  Install  */
	if ( (long) signal (SIGCHLD, sig_child_handler) == -1 )
	{
	    (void) fprintf (stderr, "Error setting SIGCHLD handler\t%s\n",
			    sys_errlist[errno]);
	    return;
	}
	installed = TRUE;
    }

#else  /*  CAN_FORK  */
    (void) fprintf (stderr, "Operating system does not support forking\n");
#endif
}   /*  End Function init_sig_child_handler  */

static void sig_child_handler ()
/*  This routine will handle SIGCHLD signals (signals sent when a child process
    changes state).
    The routine returns nothing.
*/
{
}   /*  End Function sig_child_handler  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag cm_manage ( int pid, void (*stop_func) (), void (*term_func) (),
		void (*exit_func) () )
/*  This routine will manage a child process for state changes by registering
    callback routines.
    The child process to manage must be given by  pid  .
    The routine which is called when the child is stopped (by a SIGTTIN,
    SIGTTOU, SIGTSTP, or SIGSTOP signal) must be pointed to by  stop_func  .If
    this is NULL, no callback routine is installed. The interface to this
    routine is as follows:

    void stop_func (pid, sig)
    *   This routine is called when a child process is stopped.
	The process ID is given by  pid  and the signal which stopped the
	process is given by  sig  .
	The routine returns nothing.
    *
    int pid;
    int sig;

    The routine which is called when the child terminates due to a signal must
    be pointed to by  term_func  .If this is NULL, no callback routine is
    installed. The interface to this routine is as follows:

    void term_func (pid, sig, rusage)
    *   This routine is called when a child process is killed.
	The process ID is given by  pid  and the signal which killed the
	process is given by  sig  .
	The resource usage information will be pointed to by  rusage  .If this
	is NULL, the platform does not support this.
	The routine returns nothing.
    *
    int pid;
    int sig;
    struct rusage *rusage;

    The routine which is called when the child exits due to a call to  _exit
    must be pointed to by  exit_func  .If this is NULL, no callback routine is
    installed. The interface to this routine is as follows:

    void exit_func (pid, value, rusage)
    *   This routine is called when a child process exits.
	The process ID is given by  pid  and the value passed to  _exit  by the
	process is given by  value  .
	The resource usage information will be pointed to by  rusage  .If this
	is NULL, the platform does not support this.
	The routine returns nothing.
    *
    int pid;
    int value;
    struct rusage *rusage;

    NOTE: if a child process is killed or exits, its callback routines are
    automatically removed. The  term_func  or  exit_func  MUST NOT unmanage
    the child process given by  pid  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
#ifdef CAN_FORK
    struct child_pid_type *entry;
    struct child_pid_type *new_entry;
    struct child_pid_type *last_entry = NULL; /*Initialised to keep gcc happy*/
    extern struct child_pid_type *child_pid_list;
    extern char *sys_errlist[];
    static char function_name[] = "cm_manage";

    init_sig_child_handler ();
    /*  See if child is already managed  */
    for (entry = child_pid_list; entry != NULL; entry = (*entry).next)
    {
	if (pid == (*entry).pid)
	{
	    (void) fprintf (stderr, "Child: %d is already managed\n", pid);
	    a_prog_bug (function_name);
	}
	last_entry = entry;
    }
    /*  Child is not already managed  */
    /*  Allocate new entry  */
    if ( ( new_entry =
	  (struct child_pid_type *) m_alloc (sizeof *new_entry) ) == NULL )
    {
	(void) m_error_notify (function_name, "new managed child entry");
	return (FALSE);
    }
    /*  Fill in entry  */
    (*new_entry).pid = pid;
    (*new_entry).stop_func = stop_func;
    (*new_entry).term_func = term_func;
    (*new_entry).exit_func = exit_func;
    (*new_entry).next = NULL;
    (*new_entry).prev = NULL;
    if (child_pid_list == NULL)
    {
	/*  Create new list  */
	child_pid_list = new_entry;
    }
    else
    {
	/*  Append to end of list  */
	(*last_entry).next = new_entry;
	(*new_entry).prev = last_entry;
    }
    return (TRUE);

#else  /*  CAN_FORK  */
    (void) fprintf (stderr, "Operating system does not support forking\n");
    return (FALSE);
#endif
}   /*  End Function cm_manage  */

/*PUBLIC_FUNCTION*/
void cm_unmanage (int pid)
/*  This routine will terminate the management of a child process for state
    changes.
    The child process to unmanage must be given by  pid  .
    The routine returns nothing.
*/
{
#ifdef CAN_FORK
    struct child_pid_type *entry;
    extern struct child_pid_type *child_pid_list;
    extern char *sys_errlist[];
    static char function_name[] = "cm_unmanage";

    init_sig_child_handler ();
    for (entry = child_pid_list; entry != NULL; entry = (*entry).next)
    {
	if (pid == (*entry).pid)
	{
	    /*  Remove entry  */
	    if ( (*entry).prev == NULL )
	    {
		/*  Entry is first in the list  */
		child_pid_list = (*entry).next;
	    }
	    else
	    {
		/*  Previous entry exists  */
		(* (*entry).prev ).next = (*entry).next;
	    }
	    if ( (*entry).next != NULL )
	    {
		/*  Next entry exists  */
		(* (*entry).next ).prev = (*entry).prev;
	    }
	    m_free ( (char *) entry );
	    return;
	}
    }
    /*  Child not found  */
    (void) fprintf (stderr, "Child: %d not managed\n", pid);
    a_prog_bug (function_name);

#else  /*  CAN_FORK  */
    (void) fprintf (stderr, "Operating system does not support forking\n");
#endif
}   /*  End Function cm_unmanage  */

/*PUBLIC_FUNCTION*/
void cm_poll (flag block)
/*  This routine will poll for any change in status of child processes. If the
    operating system does not support forking, the routine displays an error
    message.
    Those processes which have been registered using  cm_manage  will have any
    callback routines called.
    If a child has not been registered using  cm_manage  then the routine will
    print a warning message stating that the child was not managed.
    If the value of  block  is TRUE, the routine will poll forever until a
    child changes status or a signal is caught.
    NOTE: if the process has no children, the routine will ALWAYS return
    immediately.
    The routine returns nothing.
*/
{
#ifdef CAN_FORK
    flag unmanage;
    int pid;
    int child_status;
    int options = WUNTRACED;
    struct child_pid_type *entry;
#  ifdef HAS_WAIT3
    struct rusage rusage;
    void *rusage_ptr = &rusage;
#  else
    void *rusage_ptr = NULL;
#  endif
    extern struct child_pid_type *child_pid_list;
    extern char *sys_errlist[];

    init_sig_child_handler ();
    if (block != TRUE)
    {
	options |= WNOHANG;
    }
#  ifdef HAS_WAIT3
#    if defined(OS_Linux)
    /*  Hey! I used to have to include SGImips here. No longer! They must have
	fixed it!  */
    /*  Why oh why do they have to be different?  */
    switch ( pid = wait3 ( (union wait *) &child_status, options, &rusage ) )
#    else
    switch ( pid = wait3 (&child_status, options, &rusage) )
#    endif
#  else
    switch ( pid = waitpid (-1, &child_status, options) )
#  endif
    {
      case -1:
	if ( (errno == EINTR) || (errno == ECHILD) )
	{
	    return;
	}
	(void) fprintf (stderr, "Error in call to wait3(2)\t%s\n",
			sys_errlist[errno]);
	return;
/*
	break;
*/
      case 0:
	return;
/*
	break;
*/
      default:
	/*  Got a child PID  */
	break;
    }
    for (entry = child_pid_list; entry != NULL; entry = (*entry).next)
    {
	/*  Compare PID  */
	unmanage = FALSE;
	if (pid == (*entry).pid)
	{
	    /*  Found it  */
	    if ( ( (*entry).stop_func != NULL ) &&
		(WIFSTOPPED (child_status) == 1) )
	    {
		(* (*entry).stop_func ) ( (*entry).pid,
					 WSTOPSIG (child_status) );
	    }
	    if (WIFSIGNALED (child_status) == 1)
	    {
		if ( (*entry).term_func != NULL )
		{
		    (* (*entry).term_func ) ( (*entry).pid,
					     WTERMSIG (child_status),
					     rusage_ptr );
		}
		unmanage = TRUE;
	    }
	    if (WIFEXITED (child_status) == 1)
	    {
		if ( (*entry).exit_func != NULL )
		{
		    (* (*entry).exit_func ) ( (*entry).pid,
					     WEXITSTATUS (child_status),
					     rusage_ptr );
		}
		unmanage = TRUE;
	    }
	    if (unmanage == TRUE)
	    {
		cm_unmanage ( (*entry).pid );
	    }
	    return;
	}
    }
    /*  Child was not managed  */
    (void) fprintf (stderr, "WARNING: Child process: %d is not managed\n",
		    pid);

#else  /*  CAN_FORK  */
    (void) fprintf (stderr, "Operating system does not support forking\n");
#endif
}   /*  End Function cm_poll  */

/*PUBLIC_FUNCTION*/
void cm_poll_silent (flag block)
/*  This routine will poll for any change in status of child processes. This
    routine is similar to the  cm_poll  routine, except that if the operating
    system does not support forking, no error message is displayed.
    Those processes which have been registered using  cm_manage  will have any
    callback routines called.
    If a child has not been registered using  cm_manage  then the routine will
    print a warning message stating that the child was not managed.
    If the value of  block  is TRUE, the routine will poll forever until a
    child changes status or a signal is caught.
    NOTE: if the process has no children, the routine will ALWAYS return
    immediately.
    The routine returns nothing.
*/
{
#ifdef CAN_FORK
    cm_poll (block);
#endif
}   /*  End Function cm_poll_silent  */

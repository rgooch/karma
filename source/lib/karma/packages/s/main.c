/*LINTLIBRARY*/
/*  main.c

    This code provides signal blocking routines.

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

/*

    This file contains the various Karma utility routines which relate to
  blocking and unblocking signals.


    Written by      Richard Gooch   12-SEP-1992

    Updated by      Richard Gooch   14-NOV-1992

    Updated by      Richard Gooch   12-DEC-1992: Fixed some comments.

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   12-APR-1993: Added  s_ignore  .

    Updated by      Richard Gooch   22-MAY-1994: Added  return (NULL)  to
  s_ignore  when signals unsupported.

    Updated by      Richard Gooch   9-NOV-1994: Changed return cast in
  s_ignore  .

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/s/main.c

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>

    Last updated by Richard Gooch   5-MAY-1995: Placate SGI compiler.


*/
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <os.h>
#ifdef CAN_FORK
#include <unistd.h>
#include <sys/ioctl.h>
#include <signal.h>
#endif
#include <karma.h>
#include <karma_s.h>

/*  Private functions  */
#ifdef CAN_FORK
static void prog_bug ();
#endif  /*  CAN_FORK  */

/*  Private data  */
static flag initialised = FALSE;
static sigset_t blocked_mask;

/*PUBLIC_FUNCTION*/
void s_block (signal)
/*  This routine will cause a signal to be blocked until a subsequent call to
    s_unblock  or  s_unblock_all  .If the signal is already blocked,
    no action is taken.
    The signal to block must be given by  signal  .
    The routine returns nothing.
*/
int signal;
{
#ifdef CAN_FORK
    sigset_t old_mask;
    sigset_t new_mask;
    extern flag initialised;
    extern sigset_t blocked_mask;
    extern char *sys_errlist[];
/*
    static char function_name[] = "s_block";
*/

    if (initialised != TRUE)
    {
	/*  Initialise signal set  */
	if (sigemptyset (&blocked_mask) != 0)
	{
	    (void) fprintf (stderr, "Error clearing signal mask\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	initialised = TRUE;
    }
    /*  Get old signal mask  */
    if (sigprocmask (SIG_BLOCK, (sigset_t *) NULL, &old_mask) != 0)
    {
	(void) fprintf (stderr, "Error getting signal mask\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Block signal  */
    /*  Note that signals which are already blocked are not blocked again,
	as this could cause the call to  sigprocmask  to block (on a Convex).
	This seems to be due to the way multiple processors handle signals  */
    switch ( sigismember (&old_mask, signal) )
    {
      case 0:
	/*  Signal not already blocked  */
	if (sigaddset (&blocked_mask, signal) != 0)
	{
	    (void) fprintf (stderr,
			    "Error adding signal: %d to signal mask\t%s\n",
			    signal, sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	/*  Initialise new signal set  */
	if (sigemptyset (&new_mask) != 0)
	{
	    (void) fprintf (stderr, "Error clearing new signal mask\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (sigaddset (&new_mask, signal) != 0)
	{
	    (void) fprintf (stderr,
			    "Error adding signal: %d to new signal mask\t%s\n",
			    signal, sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	/*  Now block signal  */
	if (sigprocmask (SIG_BLOCK, &new_mask, (sigset_t *) NULL) != 0)
	{
	    (void) fprintf (stderr, "Error setting signal mask\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	break;
      case 1:
	/*  Signal already blocked  */
	break;
      default:
	(void) fprintf (stderr,
			"Error testing if signal: %d is a member\t%s\n",
			signal, sys_errlist[errno]);
	exit (RV_SYS_ERROR);
	break;
    }

#else  /*  CAN_FORK  */
    (void) fprintf (stderr,
		    "Operating system does not support signal blocking\n");
#endif  /*  CAN_FORK  */
}   /*  End Function s_block  */

/*PUBLIC_FUNCTION*/
void s_unblock (signal)
/*  This routine will cause a signal to be unblocked. If the signal is not
    already blocked, no action is taken.
    The signal to unblock must be given by  signal  .
    Note that the signal will only be unblocked if previously blocked by a call
    to  s_block  .
    The routine returns nothing.
*/
int signal;
{
#ifdef CAN_FORK
    sigset_t old_mask;
    sigset_t new_mask;
    extern flag initialised;
    extern sigset_t blocked_mask;
    extern char *sys_errlist[];
    static char function_name[] = "s_unblock";

    if (initialised != TRUE)
    {
	(void) fprintf (stderr, "No signals blocked yet\n");
	prog_bug (function_name);
    }
    /*  Test supposed signal mask  */
    switch ( sigismember (&blocked_mask, signal) )
    {
      case 0:
	/*  Signal not already blocked  */
	return;
/*
	break;
*/
      case 1:
	/*  Signal already blocked  */
	break;
      default:
	(void) fprintf (stderr,
			"Error testing if signal: %d is a member\t%s\n",
			signal, sys_errlist[errno]);
	exit (RV_SYS_ERROR);
	break;
    }
    /*  Get old signal mask  */
    if (sigprocmask (SIG_BLOCK, (sigset_t *) NULL, &old_mask) != 0)
    {
	(void) fprintf (stderr, "Error getting signal mask\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    switch ( sigismember (&old_mask, signal) )
    {
      case 0:
	/*  Signal not blocked  */
	(void) fprintf (stderr,
			"Signal: %d has been unblocked via other means\n",
			signal);
	prog_bug (function_name);
	break;
      case 1:
	/*  Signal blocked: unblock  */
	if (sigdelset (&blocked_mask, signal) != 0)
	{
	    (void) fprintf (stderr,
			    "Error adding signal: %d to signal mask\t%s\n",
			    signal, sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	/*  Initialise new signal set  */
	if (sigemptyset (&new_mask) != 0)
	{
	    (void) fprintf (stderr, "Error clearing new signal mask\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (sigaddset (&new_mask, signal) != 0)
	{
	    (void) fprintf (stderr,
			    "Error adding signal: %d to new signal mask\t%s\n",
			    signal, sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (sigprocmask (SIG_UNBLOCK, &new_mask, (sigset_t *) NULL) != 0)
	{
	    (void) fprintf (stderr, "Error restoring old signal mask\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	break;
      default:
	(void) fprintf (stderr,
			"Error testing if signal: %d is a member\t%s\n",
			signal, sys_errlist[errno]);
	exit (RV_SYS_ERROR);
	break;
    }

#else  /*  CAN_FORK  */
    (void) fprintf (stderr,
		    "Operating system does not support signal blocking\n");
#endif  /*  CAN_FORK  */
}   /*  End Function s_unblock  */

/*PUBLIC_FUNCTION*/
void s_unblock_all ()
/*  This routine will cause all signals blocked by  s_block  to be
    unblocked. If no signals were blocked, no action is taken.
    The routine returns nothing.
*/
{
#ifdef CAN_FORK
    extern flag initialised;
    extern sigset_t blocked_mask;
    extern char *sys_errlist[];
    static char function_name[] = "s_unblock_all";

    if (initialised != TRUE)
    {
	(void) fprintf (stderr, "No signals blocked yet\n");
	prog_bug (function_name);
    }
    /*  Unblock signals  */
    if (sigprocmask (SIG_UNBLOCK, &blocked_mask, (sigset_t *) NULL) != 0)
    {
	(void) fprintf (stderr, "Error restoring old signal mask\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Initialise signal set  */
    if (sigemptyset (&blocked_mask) != 0)
    {
	(void) fprintf (stderr, "Error clearing signal mask\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }

#else  /*  CAN_FORK  */
    (void) fprintf (stderr,
		    "Operating system does not support signal blocking\n");
#endif  /*  CAN_FORK  */
}   /*  End Function s_unblock_all  */

/*PUBLIC_FUNCTION*/
void *s_ignore (sig)
/*  This routine will cause a signal to be ignored.
    The signal to ignore must be given by  sig  .
    The routine returns the previous signal handler.
*/
int sig;
{
#ifdef CAN_FORK
    extern char *sys_errlist[];
/*
    static char function_name[] = "s_ignore";
*/

    return ( (void *) signal (sig, SIG_IGN) );

#else  /*  CAN_FORK  */
    (void) fprintf (stderr,
		    "Operating system does not support signal ignoring\n");
    return (NULL);
#endif  /*  CAN_FORK  */
}   /*  End Function s_ignore  */

#ifdef CAN_FORK

static void prog_bug (function_name)
char *function_name;
{
    (void) fprintf (stderr, "Program bug noted in function: %s\n",
		    function_name);
    (void) fprintf (stderr, "Aborting.%c\n", BEL);
    exit (RV_PROGRAM_BUG);
}   /*  End Function prog_bug   */

#endif  /*  CAN_FORK  */

/*LINTLIBRARY*/
/*PREFIX:"d_"*/
/*  daemon.c
    This code manipulates daemon mode for a process.

    Copyright (C) 1992,1993  Richard Gooch

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
  entering daemon mode.


    Written by      Richard Gooch   12-SEP-1992

    Updated by      Richard Gooch   14-NOV-1992

    Last updated by Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>


*/
#include <stdio.h>
#include <fcntl.h>
#include <karma.h>
#include <karma_d.h>

/*  Private functions  */
static void prog_bug ();

/*  Private data  */
static flag daemon_mode = FALSE;

/*PUBLIC_FUNCTION*/
void d_enter_daemon_mode ()
/*  This routine will cause the module to enter daemon mode. This involves
    detatching from a terminal.
    The routine returns nothing.
*/
{
    int dev_null;
    extern flag daemon_mode;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "d_enter_daemon_mode";

    if (daemon_mode == TRUE)
    {
	(void) fprintf (stderr, "Daemon mode has already been entered\n");
	prog_bug (function_name);
    }
    daemon_mode = TRUE;
    /*  Open up null device  */
    if ( ( dev_null = open ("/dev/null", O_WRONLY | O_CREAT, 0) ) < 0 )
    {
	(void) fprintf (stderr,
			"Error opening: \"/dev/null\" for output\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Close standard input  */
    if (close (0) != 0)
    {
	(void) fprintf (stderr,
			"Error closing standard input\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Attatch null device to standard output  */
    if (dup2 (dev_null, 1) < 0)
    {
	(void) fprintf (stderr,
			"Error redirecting standard output\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Attatch null device to standard error  */
    if (dup2 (dev_null, 2) < 0)
    {
	(void) fprintf (stderr,
			"Error redirecting standard error\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Close temporary descriptor for null device  */
    if (close (dev_null) != 0)
    {
	(void) fprintf (stderr,
			"Error closing temporary descriptor for null device\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
}   /*  End Function d_enter_daemon_mode  */

/*PUBLIC_FUNCTION*/
flag d_test_daemon_mode ()
/*  This routine will test if the module is running in daemon mode.
    The routine returns TRUE if the module is running in daemon mode,
    else it returns FALSE.
*/
{
    extern flag daemon_mode;

    return (daemon_mode);
}   /*  End Function d_test_daemon_mode  */

static void prog_bug (function_name)
char *function_name;
{
    (void) fprintf (stderr, "\nProgram bug noted in function: %s",
		    function_name);
    (void) fprintf (stderr, "\nAborting.%c\n", BEL);
    exit (RV_PROGRAM_BUG);
}   /*  End Function prog_bug   */

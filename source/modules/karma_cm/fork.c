/*  fork.c

    Execute file for Connection Management tool and shell.

    Copyright (C) 1992,1993,1994  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains the fork-and-exec routines needed for the Connection
    Management tool or shell to start modules.


    Written by      Richard Gooch   15-DEC-1992

    Updated by      Richard Gooch   25-DEC-1992

    Updated by      Richard Gooch   15-APR-1994: Added support for passing of
  arguments to modules.

    Last updated by Richard Gooch   10-OCT-1994: Added prototype declaration
  for  fork_cm_client_module  routine.


*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <karma.h>
#include <karma_ex.h>
#include <karma_r.h>

#define MAX_ARGS 1024


/*  Private data  */
static char *argv[MAX_ARGS];


/*  Local functions  */
EXTERN_FUNCTION (int fork_cm_client_module,
		 (char *module_name, unsigned long cm_host_addr,
		  unsigned int cm_port, int x, int y, char *args) );


static void execute_new_module (module_name, cm_host_addr, cm_port, x, y, args)
/*  This routine will replace the current process program with another
    executable such that the new process will connect to the Connection
    Management Tool.
    The name of the module to run must be pointed to by  module_name  .
    The Internet address of the machine on which the Connection Management Tool
    is running must be given by  cm_host_addr  .
    The Karma port number to connect to must be given by  cm_port  .
    The window co-ordinates of the icon for the Connection Management Tool must
    be given by  x  and  y  .
    The optional arguments to pass to the new process must be pointed to by
    args  .If this is NULL, no arguments are passed.
    The routine returns only on failure.
*/
char *module_name;
unsigned long cm_host_addr;
unsigned int cm_port;
int x;
int y;
char *args;
{
    unsigned int count;
    char env_name[STRING_LENGTH];
    char env_value[STRING_LENGTH];
    extern char *argv[MAX_ARGS];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    (void) strcpy (env_name, "KARMA_CM_CONTROL_COMMAND");
    (void) sprintf (env_value, "%lu:%u:%d:%d",
		    cm_host_addr, cm_port, x, y);
    if (r_setenv (env_name, env_value) != 0)
    {
	(void) fprintf (stderr, "Error expanding environment\n");
	return;
    }
    if ( (args == NULL) || (args[0] == '\0') )
    {
	(void) execlp (module_name, module_name, NULL);
	/*  Failure  */
	(void) fprintf (stderr, "Error executing image: \"%s\"\t%s\n",
			module_name, sys_errlist[errno]);
	return;
    }
    /*  Split up arguments  */
    argv[0] = module_name;
    for (count = 1; ( argv[count] = ex_str (args, &args) ) != NULL; ++count)
    {
	if (count > MAX_ARGS)
	{
	    (void) fprintf (stderr, "Too many arguments\n");
	    return;
	}
    }
    (void) execvp (module_name, argv);
    /*  Failure  */
    (void) fprintf (stderr, "Error executing image: \"%s\"\t%s\n",
		    module_name, sys_errlist[errno]);
}   /*  End Function execute_new_module  */

int fork_cm_client_module (module_name, cm_host_addr, cm_port, x, y, args)
/*  This routine will fork a process and run another executable such that the
    new process will connect to the Connection Management Tool.
    The name of the module to run must be pointed to by  module_name  .
    The Internet address of the machine on which the Connection Management Tool
    is running must be given by  cm_host_addr  .
    The Karma port number to connect to must be given by  cm_port  .
    The window co-ordinates of the icon for the Connection Management Tool must
    be given by  x  and  y  .
    The optional arguments to pass to the new process must be pointed to by
    args  .If this is NULL, no arguments are passed.
    The routine returns the process ID of the child on success,
    else it returns -1.
*/
char *module_name;
unsigned long cm_host_addr;
unsigned int cm_port;
int x;
int y;
char *args;
{
    int child_pid;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    switch ( child_pid = fork () )
    {
      case 0:
	/*  Child  */
	execute_new_module (module_name, cm_host_addr, cm_port, x, y, args);
	/*  Error  */
	exit (RV_UNDEF_ERROR);
	break;
      case 1:
	(void) fprintf (stderr, "Error forking\t%s\n", sys_errlist[errno]);
	return (-1);
	break;
      default:
	/*  Parent  */
	break;
    }
    /*  Return child PID  */
    return (child_pid);
}   /*  End Function fork_cm_client_module  */

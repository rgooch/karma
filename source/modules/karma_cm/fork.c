/*  fork.c

    Execute file for Connection Management tool and shell.

    Copyright (C) 1993  Richard Gooch

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

    Updated by      Richard Gooch   29-DEC-1992: Added special device support
  (eg. VX/MVX).

    Updated by      Richard Gooch   1-JAN-1993: Changed  rean_line  to use
  chs_get_line  instead of  ch_gets  .

    Updated by      Richard Gooch   1-APR-1993: Changed to execution of script
  when running slave and added option to specify KARMABASE for hosts.

    Updated by      Richard Gooch   4-APR-1993: Took account of change to
  conn_register_server_protocol  .

    Updated by      Richard Gooch   4-JUL-1993: Added shell escape commands.

    Updated by      Richard Gooch   24-JUL-1993: Changed to  ch_open_file  .

    Updated by      Richard Gooch   23-AUG-1993: Added more diagnostics to
  startup_modules  .

    Updated by      Richard Gooch   7-SEP-1993: Added test for modules loss
  prior to starting up connections.

    Last updated by Richard Gooch   27-SEP-1993: Added quiescent notification
  to modules.


    Usage:   cm_shell path

*/
#include <stdio.h>
#include <stdlib.h>
#include <karma.h>
#include <karma_r.h>


/*  Local functions  */

void execute_new_module (module_name, cm_host_addr, cm_port, x, y)
/*  This routine will replace the current process program with another
    executable such that the new process will connect to the Connection
    Management Tool.
    The name of the module to run must be pointed to by  module_name  .
    The Internet address of the machine on which the Connection Management Tool
    is running must be given by  cm_host_addr  .
    The Karma port number to connect to must be given by  cm_port  .
    The window co-ordinates of the icon for the Connection Management Tool must
    be given by  x  and  y  .
    The routine returns only on failure.
*/
char *module_name;
unsigned long cm_host_addr;
unsigned int cm_port;
int x;
int y;
{
    char env_name[STRING_LENGTH];
    char env_value[STRING_LENGTH];
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
    (void) execlp (module_name, module_name, NULL);
    /*  Failure  */
    (void) fprintf (stderr, "Error executing image: \"%s\"\t%s\n",
		    module_name, sys_errlist[errno]);
    return;
}   /*  End Function execute_new_module  */

int fork_cm_client_module (module_name, cm_host_addr, cm_port, x, y)
/*  This routine will fork a process and run another executable such that the
    new process will connect to the Connection Management Tool.
    The name of the module to run must be pointed to by  module_name  .
    The Internet address of the machine on which the Connection Management Tool
    is running must be given by  cm_host_addr  .
    The Karma port number to connect to must be given by  cm_port  .
    The window co-ordinates of the icon for the Connection Management Tool must
    be given by  x  and  y  .
    The routine returns the process ID of the child on success,
    else it returns -1.
*/
char *module_name;
unsigned long cm_host_addr;
unsigned int cm_port;
int x;
int y;
{
    int child_pid;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    switch ( child_pid = fork () )
    {
      case 0:
	/*  Child  */
	execute_new_module (module_name, cm_host_addr, cm_port, x, y);
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

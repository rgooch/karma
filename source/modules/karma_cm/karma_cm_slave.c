/*  karma_cm_slave.c

    Slave process file for Connection Management tool and shell.

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

/*  This Karma programme will connect to a Connection Management tool or shell
    in order to process module creation commands.


    Written by      Richard Gooch   12-DEC-1992

    Updated by      Richard Gooch   26-DEC-1992

    Updated by      Richard Gooch   29-SEP-1993

    Updated by      Richard Gooch   15-APR-1994: Added support for passing of
  arguments to modules.

    Last updated by Richard Gooch   19-APR-1994: Took account of change to
  interface to  slave_setup  .


    Usage:   karma_cm_slave host port display [args...]

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <karma.h>
#include <karma_conn.h>
#include <karma_pio.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_m.h>


/*  Private functions  */
static flag open_func ();
static flag read_func ();
static void close_func ();
static void stop_func ();
static void term_func ();
static void exit_func ();


/*  External functions  */
EXTERN_FUNCTION (int slave_setup, (int argc, char *argv[],
				   unsigned int *cm_port_number,
				   unsigned long *cm_host_addr,
				   flag (*open_func) (), flag (*read_func) (),
				   void (*close_func) () ) );


static unsigned long cm_host_addr = 0;
static unsigned int cm_port_number;

void main (argc, argv)
int argc;
char *argv[];
{
    extern unsigned int cm_port_number;
    extern unsigned long cm_host_addr;

    (void) slave_setup (argc, argv, &cm_port_number, &cm_host_addr,
			open_func, read_func, close_func);
    /*  Primary event loop  */
    while (TRUE)
    {
	cm_poll (FALSE);
	chm_poll (-1);
    }
}   /*  End Function main  */

static flag open_func (connection, info)
/*  This routine will register the opening of the connection to the Connection
    Management tool.
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on successful registration,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will not be called upon connection closure.
*/
Connection connection;
void **info;
{
    Channel channel;
    char my_hostname[STRING_LENGTH + 1];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    (void) fprintf (stderr, "SLAVE: open_func...\n");
    /*  Get my host information  */
    if (gethostname (my_hostname, STRING_LENGTH) != 0)
    {
	(void) fprintf (stderr, "SLAVE: Error getting hostname\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    my_hostname[STRING_LENGTH] = '\0';
    channel = conn_get_channel (connection);
    (void) fprintf (stderr, "SLAVE: open_func writing my hostname...\n");
    if (pio_write_string (channel, my_hostname) != TRUE)
    {
	(void) fprintf (stderr, "SLAVE: Error writing hostname\t%s\n",
			sys_errlist[errno]);
	exit (RV_WRITE_ERROR);
    }
    (void) fprintf (stderr, "SLAVE: open_func flushing...\n");
    if (ch_flush (channel) != TRUE)
    {
	(void) fprintf (stderr, "SLAVE: Error flushing channel\t%s\n",
			sys_errlist[errno]);
	exit (RV_WRITE_ERROR);
    }
    (void) fprintf (stderr, "SLAVE: open_func finished.\n");
    return (TRUE);
}   /*  End Function open_func  */

static flag read_func (connection, info)
/*  This routine will read from the connection to the Connection Management
    tool.
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on success,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will not be called upon connection closure.
*/
Connection connection;
void **info;
{
    long x;
    long y;
    int child_pid;
    Channel channel;
    char *module_name;
    char *args;
    extern unsigned int cm_port_number;
    extern unsigned long cm_host_addr;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    channel = conn_get_channel (connection);
    if ( ( module_name = pio_read_string (channel, (unsigned int *) NULL) )
	== NULL )
    {
	(void) fprintf (stderr, "SLAVE: Error reading module name\t%s\n",
			sys_errlist[errno]);
	exit (RV_READ_ERROR);
    }
    if (pio_read32s (channel, &x) != TRUE)
    {
	(void) fprintf (stderr, "SLAVE: Error reading x position\t%s\n",
			sys_errlist[errno]);
	exit (RV_READ_ERROR);
    }
    if (pio_read32s (channel, &y) != TRUE)
    {
	(void) fprintf (stderr, "SLAVE: Error reading y position\t%s\n",
			sys_errlist[errno]);
	exit (RV_READ_ERROR);
    }
    if ( ( args = pio_read_string (channel, (unsigned int *) NULL) )
	== NULL )
    {
	(void) fprintf (stderr, "SLAVE: Error reading arguments\t%s\n",
			sys_errlist[errno]);
	exit (RV_READ_ERROR);
    }
    if (args[0] == '\0')
    {
	m_free (args);
	args = NULL;
    }
    if ( ( child_pid = fork_cm_client_module (module_name, cm_host_addr,
					      cm_port_number,
					      (int) x, (int) y, args) )
	< 0 )
    {
	(void) fprintf (stderr, "SLAVE: Error starting module: \"%s\"\n",
			module_name);
    }
    else
    {
	if (cm_manage (child_pid, stop_func, term_func, exit_func)
	    != TRUE)
	{
	    (void) fprintf (stderr, "SLAVE: Error managing child PID: %d\n",
			    child_pid);
	}
    }
    m_free (module_name);
    if (args != NULL) m_free (args);
    return (TRUE);
}   /*  End Function read_func  */

static void close_func (connection, info)
/*  This routine will register the closing of the connection to the Connection
    Management tool.
    The connection will be given by  connection  .
    The connection information pointer will be given by  info  .
    The routine returns nothing.
*/
Connection connection;
void *info;
{
    (void) fprintf (stderr,
		    "SLAVE: Lost connection to Connection Management tool\n");
    exit (RV_OK);
}   /*  End Function close_func  */

static void stop_func (pid, sig)
/*  This routine is called when a child process is stopped.
    The process ID is given by  pid  and the signal which stopped the process
    is given by  sig  .
    The routine returns nothing.
*/
int pid;
int sig;
{
}   /*  End Function stop_func  */

static void term_func (pid, sig, rusage)
/*  This routine is called when a child process is killed.
    The process ID is given by  pid  and the signal which killed the process is
    given by  sig  .
    The resource usage information will be pointed to by  rusage  .
    The routine returns nothing.
*/
int pid;
int sig;
struct rusage *rusage;
{
}   /*  End Function term_func  */

static void exit_func (pid, value, rusage)
/*  This routine is called when a child process exits.
    The process ID is given by  pid  and the value passed to  _exit  by the
    process is given by  value  .
    The resource usage information will be pointed to by  rusage  .
    The routine returns nothing.
*/
int pid;
int value;
struct rusage *rusage;
{
}   /*  End Function exit_func  */

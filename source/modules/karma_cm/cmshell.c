/*  cmshell.c

    Main file for  cmshell  (Connection Management Shell interpreter).

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

/*  This Karma programme will interpret a file to be Connection Management tool
    directives.


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

    Updated by      Richard Gooch   27-SEP-1993: Added quiescent notification
  to modules.

    Updated by      Richard Gooch   15-APR-1994: Added support for passing of
  arguments to modules.

    Updated by      Richard Gooch   19-APR-1994: Added support for shell escape
  commands in the hostlist.

    Updated by      Richard Gooch   19-APR-1994: Added support for passing of
  arguments to CM Tool slaves.

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Updated by      Richard Gooch   2-AUG-1994: Added more diagnostic
  information for child processes.

    Last updated by Richard Gooch   10-OCT-1994: Prevent shell cleanup stage
  if child process does not exit with 0 status code.


    Usage:   cm_shell path

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <sys/resource.h>
#include "os.h"
#include <karma.h>
#include <karma_conn.h>
#include <karma_chm.h>
#include <karma_pio.h>
#include <karma_ch.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_r.h>
#include <karma_m.h>
#include <k_cm_codes.h>

#define PROTOCOL_VERSION (unsigned int) 0

/*  Structures  */
typedef struct _hosttype
{
    char *alias;
    char *public_name;
    unsigned long addr;
    Connection slave;
    struct _hosttype *next;
    struct _hosttype *prev;
} hosttype;

typedef struct _connectiontype
{
    struct _moduletype *server;
    Connection connection;
    char *protocol_name;
    struct _connectiontype *next;
    struct _connectiontype *prev;
} connectiontype;

typedef struct _moduletype
{
    char *name;
    hosttype *host;
    int x;
    int y;
    int pid;
    Connection control;
    Connection stdio;
    int port_number;
    flag shut_down;
    connectiontype *first_connection;
    struct _moduletype *next;
    struct _moduletype *prev;
} moduletype;


/*  External functions  */
EXTERN_FUNCTION (int fork_cm_client_module,
		 (char *module_name, unsigned long cm_host_addr,
		  unsigned int cm_port, int x, int y, char *args) );


/*  Private functions  */
static void event_delay ();
static void startup_hosts ();
static void startup_modules ();
static void startup_connections ();
static void process_commands ();
static void stop_func ();
static void term_func ();
static void exit_func ();
static void wait_for_slaves_to_disconnect (/* timeout */);
static void disconnect_from_slaves ();
static void notify_quiescent ();
STATIC_FUNCTION (void shutdown_modules, () );


/*  Public functions  */
flag new_host_func ();
void host_lost_func ();
hosttype *get_host_info ();
flag new_module_control ();
flag module_control_event ();
void module_control_lost ();
flag new_module_stdio ();
flag module_stdio_output ();
void module_stdio_lost ();


/*  Private data  */
static unsigned int hostcount = 0;
static moduletype *latest_module = NULL;
static unsigned int connectioncount = 0;
static char my_hostname[STRING_LENGTH + 1];
static unsigned int my_port;
static hosttype *hostlist = NULL;
static moduletype *modulelist = NULL;
static flag keep_going = TRUE;
static unsigned int child_count = 0;
static flag clean_child_exit = TRUE;


void main (argc, argv)
int argc;
char *argv[];
{
    flag end;
    int exit_status;
    unsigned int count;
    unsigned long addr;
    Channel channel;
    int def_port_number;
    hosttype *new_host;
    char buffer[STRING_LENGTH];
    char display[STRING_LENGTH];
    extern flag keep_going;
    extern flag clean_child_exit;
    extern unsigned int my_port;
    extern unsigned int child_count;
    extern hosttype *hostlist;
    extern char my_hostname[STRING_LENGTH + 1];
    extern char module_name[STRING_LENGTH + 1];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char usage_string[] = "Usage:\tcm_shell path";
    static char function_name[] = "main";

    im_register_module_name ("karma_cm");
    /*  Get my host information  */
    if (gethostname (my_hostname, STRING_LENGTH) != 0)
    {
	(void) fprintf (stderr, "Error getting hostname\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    my_hostname[STRING_LENGTH] = '\0';
    if ( ( addr = r_get_inet_addr_from_host (my_hostname, (flag *) NULL) )
	== 0 )
    {
	(void) fprintf (stderr, "Error getting local Internet address\n");
	exit (RV_UNDEF_ERROR);
    }
    if ( ( new_host = (hosttype *) m_alloc (sizeof *new_host) ) == NULL )
    {
	m_abort (function_name, "host entry");
    }
    (*new_host).alias = "localhost";
    (*new_host).public_name = my_hostname;
    (*new_host).addr = addr;
    (*new_host).slave = NULL;
    (*new_host).next = NULL;
    (*new_host).prev = NULL;
    /*  Add to list  */
    hostlist = new_host;
    if (argc > 2)
    {
	(void) fprintf (stderr, "%s\n", usage_string);
	exit (RV_TOO_MANY_PARAM);
    }
    if (argc < 2)
    {
	(void) fprintf (stderr, "%s\n", usage_string);
	exit (RV_MISSING_PARAM);
    }
    if ( ( channel = ch_open_file (argv[1], "r") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening file: \"%s\" for input\t%s\n",
			argv[1], sys_errlist[errno]);
	exit (RV_CANNOT_OPEN);
    }
    /*  Start connection library package  */
    conn_register_managers (chm_manage, chm_unmanage, ( void (*) () ) NULL);
    /*  Get default port number  */
    if ( ( def_port_number = r_get_def_port ( module_name,
					     r_getenv ("DISPLAY") ) ) < 0 )
    {
	(void) fprintf (stderr, "Could not get default port number\n");
	exit (RV_UNDEF_ERROR);
    }
    my_port = def_port_number;
    if (conn_become_server (&my_port, CONN_MAX_INSTANCES)
	!= TRUE)
    {
	(void) fprintf (stderr, "Module not operating as Karma server\n");
	exit (RV_UNDEF_ERROR);
    }
    (void) fprintf (stderr, "Port: %u\n", my_port);
    /*  Register protocols  */
    conn_register_server_protocol ("conn_mngr_slave", PROTOCOL_VERSION, 0,
				   new_host_func, ( flag (*) () ) NULL,
				   host_lost_func);
    conn_register_server_protocol ("conn_mngr_control", PROTOCOL_VERSION, 0,
				   new_module_control, module_control_event,
				   module_control_lost);
    conn_register_server_protocol ("conn_mngr_stdio", PROTOCOL_VERSION, 0,
				   new_module_stdio, module_stdio_output,
				   module_stdio_lost);
    /*  Start up listed hosts  */
    (void) fprintf (stderr, "Hosts...\n");
    startup_hosts (channel);
    /*  Startup listed modules  */
    (void) fprintf (stderr, "Modules...\n");
    startup_modules (channel);
    if (!keep_going)
    {
	(void) fprintf (stderr, "Terminating\n");
	(void) ch_close (channel);
	exit (RV_UNDEF_ERROR);
    }
    /*  Make all connections  */
    (void) fprintf (stderr, "Connections...\n");
    startup_connections (channel);
    /*  Process any more commands  */
    if (read_line (channel, buffer, STRING_LENGTH, &end) == TRUE)
    {
	if (strcmp (buffer, "COMMANDS") == 0)
	{
	    process_commands (channel);
	    (void) read_line (channel, buffer, STRING_LENGTH, &end);
	}
    }
    notify_quiescent ();
    (void) fprintf (stderr, "Entering event loop...\n");
    while (keep_going)
    {
	cm_poll (FALSE);
	chm_poll (-1);
    }
    disconnect_from_slaves ();
    shutdown_modules ();
    for (count = 500; (count > 0) && (child_count > 0); --count)
    {
	cm_poll (FALSE);
	chm_poll (10);
    }
    if (child_count > 0)
    {
	(void) fprintf (stderr,
			"Not all children died: cleanup process aborted\n");
	(void) ch_close (channel);
	exit (RV_UNDEF_ERROR);
    }
    if (strcmp (buffer, "SHELL_CLEANUP") == 0)
    {
	if (clean_child_exit)
	{
	    /*  Run shell over remainder  */
	    while (read_line (channel, buffer, STRING_LENGTH, &end) == TRUE)
	    {
		exit_status = system (buffer);
		if (exit_status == 0) continue;
		if (exit_status == 127)
		{
		    (void) fprintf(stderr,
				   "Could not execute shell command: \"%s\"\n",
				    buffer);
		    exit (RV_SYS_ERROR);
		}
		(void) fprintf (stderr,
				"Shell command: \"%s\" returned status: %d\n",
				buffer, exit_status);
	    }
	}
	else
	{
	    (void) fprintf (stderr, "Not all children exited cleanly.\n");
	    (void) fprintf (stderr, "Shell cleanup stage skipped.\n");
	}
    }
    (void) ch_close (channel);
}   /*  End Function main  */

flag read_line (channel, buffer, length, end)
/*  This routine will read a line from a channel, stripping all comments,
    leading and trailing whitespace. The comment character is '#'.
    The channel must be given by  channel  .
    The buffer to write the line into must be pointed to by  buffer  .
    The size of the buffer must be given by  length  .
    The routine will write the value TRUE to the storage pointed to by  end  if
    the line contains the string "END", else it will write the value FALSE.
    The routine returns TRUE on success, else it returns FALSE.
*/
Channel channel;
char *buffer;
unsigned int length;
flag *end;
{
    int len;
    char *ch;

    *end = FALSE;
    buffer[0] = '\0';
    if (chs_get_line (channel, buffer, length) != TRUE)
    {
	return (FALSE);
    }
    if (strcmp (buffer, "END") == 0)
    {
	*end = TRUE;
    }
    return (TRUE);
}   /*  End Function read_line  */

static void event_delay (milliseconds)
/*  This routine will wait a specified time for events. The routine will
    process events until the specified time elapses.
    The number of milliseconds to wait must be given by  milliseconds  .
    The routine returns nothing.
*/
unsigned int milliseconds;
{
    unsigned int ms_10_count;

    if (milliseconds >= 10)
    {
	ms_10_count = milliseconds / 10;
	while (ms_10_count-- > 0)
	{
	    cm_poll (FALSE);
	    chm_poll (10);
	}
	milliseconds %= 10;
    }
    while (milliseconds-- > 0)
    {
	cm_poll (FALSE);
	chm_poll (1);
    }
}   /*  End Function event_delay  */

static void startup_hosts (channel)
/*  This routine will scan the host table provided in a file and will execute a
    daemon on each host to service the Connection Management tool.
    The file must be given by  channel  .
    The routine returns nothing.
*/
Channel channel;
{
    flag end;
    flag local;
    int display_num;
    int screen_num;
    int exit_status;
    unsigned int timeleft;
    unsigned int old_hostcount;
    char *env_display;
    char *display_host;
    char *host_device;
    char *hostname;
    char *karmabase;
    char *args;
    char display[STRING_LENGTH];
    char buffer[STRING_LENGTH];
    char command[STRING_LENGTH];
    extern unsigned int my_port;
    extern unsigned int hostcount;
    extern char my_hostname[STRING_LENGTH + 1];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "startup_hosts";

    if (read_line (channel, buffer, STRING_LENGTH, &end) != TRUE)
    {
	(void) fprintf (stderr, "Premature End-Of-File\n");
	exit (RV_BAD_DATA);
    }
    if (strcmp (buffer, "HOSTLIST") != 0)
    {
	(void) fprintf (stderr, "Expected: \"HOSTLIST\" got: \"%s\"\n",
			buffer);
	exit (RV_BAD_DATA);
    }
    /*  Compute DISPLAY string  */
    if ( ( env_display = r_getenv ("DISPLAY") ) == NULL )
    {
	(void) sprintf (display, "%s:0.0", my_hostname);
    }
    else
    {
	/*  Get components of DISPLAY string  */
	if ( ( display_host = r_get_host_from_display (env_display) ) == NULL )
	{
	    exit (RV_UNDEF_ERROR);
	}
	if ( ( display_num = r_get_display_num_from_display (env_display) )
	    < 0 )
	{
	    exit (RV_UNDEF_ERROR);
	}
	if ( ( screen_num = r_get_screen_num_from_display (env_display) )
	    < 0 )
	{
	    exit (RV_UNDEF_ERROR);
	}
	/*  Determine if local X server  */
	if (r_get_inet_addr_from_host (display_host, &local) == 0)
	{
	    (void) fprintf (stderr, "Error getting Internet address\n");
	    exit (RV_UNDEF_ERROR);
	}
	if (local)
	{
	    (void) sprintf (display, "%s:%d.%d",
			    my_hostname, display_num, screen_num);
	}
	else
	{
	    (void) sprintf (display, "%s:%d.%d",
			    display_host, display_num, screen_num);
	}
    }
    while ( read_line (channel, buffer, STRING_LENGTH, &end) )
    {
	if (end) return;
	/*  Start parsing line  */
	if (buffer[0] == '!')
	{
	    /*  Shell escape  */
	    exit_status = system (buffer + 1);
	    if (exit_status == 0) continue;
	    if (exit_status == 127)
	    {
		(void) fprintf (stderr,
				"Could not execute shell command: \"%s\"\n",
				buffer + 1);
		exit (RV_SYS_ERROR);
	    }
	    (void) fprintf (stderr,
			    "Shell command: \"%s\" returned status: %d\n",
			    buffer + 1, exit_status);
	    continue;
	}
	/*  First separate optional arguments  */
	if ( ( args = strchr (buffer, '-') ) != NULL )
	{
	    if (args[1] == '-')
	    {
		args[0] = '\0';
		args += 2;
		while ( isspace (*args) ) ++args;
	    }
	    else
	    {
		args = NULL;
	    }
	}
	if ( ( hostname = ex_str (buffer, &karmabase) ) == NULL )
	{
	    m_abort (function_name, "hostname copy");
	}
	if (karmabase == NULL) karmabase = r_get_karmabase ();
	if (strcmp (hostname, "local") == 0)
	{
	    (void) fprintf (stderr, "Local host always defined\n");
	    exit (RV_BAD_DATA);
	}
	if ( ( host_device = strchr (hostname, ':') ) == NULL )
	{
	    /*  Regular host  */
	    host_device = hostname + strlen (hostname);
	}
	else
	{
	    /*  Must run on some special device (eg. VX/MVX) on host  */
	    *host_device++ = '\0';
	}
	(void) sprintf (command,
			"rsh %s %s/csh_script/%skarma_cm_slave.setup %s %u %s",
			hostname, karmabase, host_device,
			my_hostname, my_port, display);
	if (args != NULL)
	{
	    (void) strcat (command, " ");
	    (void) strcat (command, args);
	}
	if (system (command) != 0)
	{
	    (void) fprintf (stderr, "Error executing command: \"%s\"\t%s\n",
			    command, sys_errlist[errno]);
	    exit (RV_UNDEF_ERROR);
	}
	old_hostcount = hostcount;
	/*  Wait 20 seconds for connection  */
	timeleft = 200;
	while ( (timeleft > 0) && (hostcount != old_hostcount + 1) )
	{
	    cm_poll (FALSE);
	    chm_poll (100);
	    --timeleft;
	}
	if (hostcount != old_hostcount + 1)
	{
	    (void) fprintf (stderr,
			    "Did not receive slave connection from: %s in time\n",
			    hostname);
	    event_delay (5000);
	    exit (RV_UNDEF_ERROR);
	}
	m_free (hostname);
	event_delay (50);
    }
    (void) fprintf (stderr, "Premature End-Of-File\n");
    exit (RV_BAD_DATA);
}   /*  End Function startup_hosts  */

static void startup_modules (channel)
/*  This routine will scan the module table provided in a file and will contact
    the slave daemon on each host to create modules.
    The file must be given by  channel  .
    The routine returns nothing.
*/
Channel channel;
{
    flag end;
    int x;
    int y;
    int child_pid;
    unsigned int timeleft;
    Channel slave;
    hosttype *host;
    char *p;
    char *hostname;
    char *module_name;
    char *args;
    char buffer[STRING_LENGTH];
    char command[STRING_LENGTH];
    extern unsigned int my_port;
    extern unsigned int child_count;
    extern moduletype *latest_module;
    extern char my_hostname[STRING_LENGTH + 1];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "startup_modules";

    if (read_line (channel, buffer, STRING_LENGTH, &end) != TRUE)
    {
	(void) fprintf (stderr, "Premature End-Of-File\n");
	exit (RV_BAD_DATA);
    }
    if (strcmp (buffer, "MODULES") != 0)
    {
	(void) fprintf (stderr, "Expected: \"MODULES\" got: \"%s\"\n",
			buffer);
	exit (RV_BAD_DATA);
    }
    if (latest_module != NULL)
    {
	(void) fprintf (stderr, "latest_module not reset\n");
	a_prog_bug (function_name);
    }
    while ( read_line (channel, buffer, STRING_LENGTH, &end) )
    {
	if (end) return;
	/*  Start parsing line  */
	if (strcmp (buffer, "DEFAULTS") == 0)
	{
	    if (latest_module == NULL)
	    {
		(void) fprintf (stderr, "Defaults before any module\n");
		exit (RV_BAD_DATA);
	    }
	    /*  Read in defaults  */
	    while (!end)
	    {
		/*  More defaults to read  */
		if (read_line (channel, buffer, STRING_LENGTH, &end) != TRUE)
		{
		    (void) fprintf (stderr, "Premature End-Of-File\n");
		    exit (RV_BAD_DATA);
		}
		if (end) continue;
		if (ch_puts (conn_get_channel ( (*latest_module).stdio ),
			     buffer, TRUE) != TRUE)
		{
		    (void) fprintf (stderr, "Error writing string\t%s\n",
				    sys_errlist[errno]);
		    exit (RV_SYS_ERROR);
		}
		if (ch_flush ( conn_get_channel ( (*latest_module).stdio ) )
		    != TRUE)
		{
		    (void) fprintf (stderr, "Error flushing channel\t%s\n",
				    sys_errlist[errno]);
		    exit (RV_SYS_ERROR);
		}
	    }
	    continue;
	}
	/*  Start module  */
	/*  First separate optional arguments  */
	if ( ( args = strchr (buffer, '-') ) != NULL )
	{
	    if (args[1] == '-')
	    {
		args[0] = '\0';
		args += 2;
		while ( isspace (*args) ) ++args;
	    }
	    else
	    {
		args = NULL;
	    }
	}
	p = buffer;
	if ( ( hostname = ex_str (p, &p) ) == NULL )
	{
	    m_abort (function_name, "hostname");
	}
	if ( ( module_name = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (stderr, "ERR: module_name: \"%s\"\n", module_name);
	    m_abort (function_name, "module name");
	}
	x = ex_int (p, &p);
	y = ex_int (p, &p);
	if (strncmp (hostname, "local", 5) == 0)
	{
	    /*  Local module  */
	    m_free (hostname);
	    if ( ( child_pid = fork_cm_client_module (module_name,
						      0, my_port, x, y, args) )
		< 0 )
	    {
		(void) fprintf (stderr,
				"Error starting module: \"%s\"\n",
				module_name);
		exit (RV_UNDEF_ERROR);
	    }
	    m_free (module_name);
	    if (cm_manage (child_pid, stop_func, term_func, exit_func)
		!= TRUE)
	    {
		(void) fprintf (stderr, "Error managing child PID: %d\n",
				child_pid);
		exit (RV_UNDEF_ERROR);
	    }
	    ++child_count;
	}
	else
	{
	    /*  Remote module  */
	    if ( ( host = get_host_info (hostname) ) == NULL )
	    {
		(void) fprintf (stderr, "No daemon running on: \"%s\"\n",
				hostname);
		a_print_abort (function_name);
	    }
	    m_free (hostname);
	    /*  Send information to slave  */
	    slave = conn_get_channel ( (*host).slave );
	    if (pio_write_string (slave, module_name) != TRUE)
	    {
		(void) fprintf (stderr, "Error writing module name\t%s\n",
				sys_errlist[errno]);
		exit (RV_SYS_ERROR);
	    }
	    m_free (module_name);
	    if (pio_write32s (slave, x) != TRUE)
	    {
		(void) fprintf (stderr, "Error writing x value\t%s\n",
				sys_errlist[errno]);
		exit (RV_SYS_ERROR);
	    }
	    if (pio_write32s (slave, y) != TRUE)
	    {
		(void) fprintf (stderr, "Error writing y value\t%s\n",
				sys_errlist[errno]);
		exit (RV_SYS_ERROR);
	    }
	    if (pio_write_string (slave, args) != TRUE)
	    {
		(void) fprintf (stderr, "Error writing arguments\t%s\n",
				sys_errlist[errno]);
		exit (RV_SYS_ERROR);
	    }
	    if (ch_flush (slave) != TRUE)
	    {
		(void) fprintf (stderr, "Error flushing channel\t%s\n",
				sys_errlist[errno]);
		exit (RV_SYS_ERROR);
	    }
	}
	latest_module = NULL;
	/*  Wait 20 seconds for connection  */
	timeleft = 200;
	while ( (timeleft > 0) && (latest_module == NULL) )
	{
	    cm_poll (FALSE);
	    chm_poll (100);
	    --timeleft;
	}
	if (latest_module == NULL)
	{
	    (void) fprintf (stderr,
			    "Did not receive module connection from: %s in time\n",
			    buffer);
	    exit (RV_UNDEF_ERROR);
	}
	/*  Wait 20 seconds for Standard IO connection  */
	timeleft = 200;
	while ( (timeleft > 0) && ( (*latest_module).stdio == NULL ) )
	{
	    cm_poll (FALSE);
	    chm_poll (100);
	    --timeleft;
	}
	if ( (*latest_module).stdio == NULL )
	{
	    (void) fprintf (stderr,
			    "Did not receive module stdio connection from: %s in time\n",
			    buffer);
	    exit (RV_UNDEF_ERROR);
	}
	event_delay (200);
    }
    (void) fprintf (stderr, "Premature End-Of-File\n");
    exit (RV_BAD_DATA);
}   /*  End Function startup_modules  */

static void startup_connections (channel)
/*  This routine will scan the connection table provided in a file and will
    contact the modules on each host to create connections.
    The file must be given by  channel  .
    The routine returns nothing.
*/
Channel channel;
{
    flag end;
    int module_num;
    unsigned int count;
    unsigned int timeleft;
    unsigned int old_connectioncount;
    Channel module;
    moduletype *client;
    moduletype *server;
    char *p;
    char *protocol_name;
    char buffer[STRING_LENGTH];
    extern unsigned int connectioncount;
    extern moduletype *modulelist;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "startup_connections";

    if (read_line (channel, buffer, STRING_LENGTH, &end) != TRUE)
    {
	(void) fprintf (stderr, "Premature End-Of-File\n");
	exit (RV_BAD_DATA);
    }
    if (strcmp (buffer, "CONNECTIONS") != 0)
    {
	(void) fprintf (stderr, "Expected: \"CONNECTIONS\" got: \"%s\"\n",
			buffer);
	exit (RV_BAD_DATA);
    }
    while ( read_line (channel, buffer, STRING_LENGTH, &end) )
    {
	if (end) return;
	/*  Start parsing line  */
	p = buffer;
	if ( ( module_num = ex_int (p, &p) ) < 0 )
	{
	    (void) fprintf (stderr, "Illegal client module number: %d\n",
			    module_num);
	    exit (RV_BAD_DATA);
	}
	for (count = 0, client = modulelist;
	     (client != NULL) && (count < module_num);
	     ++count, client = (*client).next);
	if (client == NULL)
	{
	    (void) fprintf (stderr, "Illegal client module number: %d\n",
			    module_num);
	    exit (RV_BAD_DATA);
	}
	if ( ( module_num = ex_int (p, &p) ) < 0 )
	{
	    (void) fprintf (stderr, "Illegal server module number: %d\n",
			    module_num);
	    exit (RV_BAD_DATA);
	}
	for (count = 0, server = modulelist;
	     (server != NULL) && (count < module_num);
	     ++count, server = (*server).next);
	if (server == NULL)
	{
	    (void) fprintf (stderr, "Illegal server module number: %d\n",
			    module_num);
	    exit (RV_BAD_DATA);
	}
	if (client == server)
	{
	    (void) fprintf (stderr, "Self connect not permitted\n");
	    a_print_abort ();
	}
	/*  Wait 40 seconds for server to have port  */
	timeleft = 400;
	while ( (timeleft > 0) && ( (*server).port_number < 0 ) )
	{
	    cm_poll (FALSE);
	    chm_poll (100);
	    --timeleft;
	}
	if ( (*server).port_number < 0 )
	{
	    (void) fprintf (stderr,
			    "Cannot connect to module: %s as it has no port\n",
			    (*server).name);
	    exit (RV_BAD_DATA);
	}
	if ( ( protocol_name = ex_str (p, &p) ) == NULL )
	{
	    m_abort (function_name, "protocol name");
	}
	/*  Send information to slave  */
	module = conn_get_channel ( (*client).control );
	if (pio_write32 (module, CM_TOOL_ATTEMPT_CONNECTION) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing command\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (pio_write_string (module, (* (*server).host ).public_name) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing module name\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (pio_write32 (module, (unsigned long) (*server).port_number)
	    != TRUE)
	{
	    (void) fprintf (stderr, "Error writing port number\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (pio_write_string (module, protocol_name) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing protocol name\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	m_free (protocol_name);
	if (ch_flush (module) != TRUE)
	{
	    (void) fprintf (stderr, "Error flushing channel\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	old_connectioncount = connectioncount;
	/*  Wait 20 seconds for connection  */
	timeleft = 200;
	while ( (timeleft > 0) && (connectioncount != old_connectioncount +1) )
	{
	    cm_poll (FALSE);
	    chm_poll (100);
	    --timeleft;
	}
	if (connectioncount != old_connectioncount + 1)
	{
	    (void) fprintf (stderr,
			    "Did not receive connection OK in time\n");
	    exit (RV_UNDEF_ERROR);
	}
	event_delay (20);
    }
    (void) fprintf (stderr, "Premature End-Of-File\n");
    exit (RV_BAD_DATA);
}   /*  End Function startup_connections  */

static void process_commands (channel)
/*  This routine will scan the command table provided in a file and will
    process those commands.
    The file must be given by  channel  .
    The routine returns nothing.
*/
Channel channel;
{
    flag end;
    int module_num;
    int delay;
    int exit_status;
    unsigned int count;
    Channel module;
    moduletype *client;
    char *p;
    char buffer[STRING_LENGTH];
    extern unsigned int connectioncount;
    extern moduletype *modulelist;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "process_commands";

    while ( read_line (channel, buffer, STRING_LENGTH, &end) )
    {
	if (end) return;
	/*  Start parsing line  */
	if (buffer[0] == '!')
	{
	    /*  Shell escape  */
	    exit_status = system (buffer + 1);
	    if (exit_status == 0) continue;
	    if (exit_status == 127)
	    {
		(void) fprintf (stderr,
				"Could not execute shell command: \"%s\"\n",
				buffer + 1);
		exit (RV_SYS_ERROR);
	    }
	    (void) fprintf (stderr,
			    "Shell command: \"%s\" returned status: %d\n",
			    buffer + 1, exit_status);
	    continue;
	}
	p = buffer;
	if ( ( module_num = ex_int (p, &p) ) < 0 )
	{
	    (void) fprintf (stderr, "Illegal client module number: %d\n",
			    module_num);
	    exit (RV_BAD_DATA);
	}
	if ( ( delay = ex_int (p, &p) ) < 0 )
	{
	    (void) fprintf (stderr, "Illegal delay value: %d\n",
			    module_num);
	    exit (RV_BAD_DATA);
	}
	for (count = 0, client = modulelist;
	     (client != NULL) && (count < module_num);
	     ++count, client = (*client).next);
	if (client == NULL)
	{
	    (void) fprintf (stderr, "Illegal client module number: %d\n",
			    module_num);
	    exit (RV_BAD_DATA);
	}
	/*  Send information to slave  */
	module = conn_get_channel ( (*client).stdio );
	(void) fprintf (stderr, "Processing command: \"%s\"\n", p);
	if (ch_puts (module, p, TRUE) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing command\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (ch_flush (module) != TRUE)
	{
	    (void) fprintf (stderr, "Error flushing channel\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	event_delay (delay);
    }
    (void) fprintf (stderr, "Premature End-Of-File\n");
    exit (RV_BAD_DATA);
}   /*  End Function process_commands  */

flag new_host_func (connection, info)
/*  This routine will register the opening of a connection from a new host.
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on successful registration,
    else it returns FALSE (indicating the connection should be closed).
*/
Connection connection;
void **info;
{
    Channel channel;
    flag local;
    unsigned long addr;
    char *hostname;
    char *host_device;
    hosttype *new_host;
    hosttype *last_host;
    extern unsigned int hostcount;
    extern hosttype *hostlist;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "new_host_func";

    channel = conn_get_channel (connection);
    if ( ( hostname = pio_read_string (channel, (unsigned int *) NULL) )
	== NULL )
    {
	(void) fprintf (stderr, "Error reading hostname from slave\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if (get_host_info (hostname) != NULL)
    {
	(void) fprintf (stderr, "Spurious slave connection from: \"%s\"\n",
			hostname);
	m_free (hostname);
	return (FALSE);
    }
    (void) fprintf (stderr, "Host: %s sent ID\n", hostname);
    if ( ( new_host = (hosttype *) m_alloc (sizeof *new_host) ) == NULL )
    {
	m_abort (function_name, "host entry");
    }
    (*new_host).alias = hostname;
    if ( ( host_device = strchr (hostname, ':') ) != NULL )
    {
	/*  Special device on host: strip from public name  */
	if ( ( (*new_host).public_name = m_alloc (host_device - hostname + 1) )
	== NULL )
	{
	    m_abort (function_name, "stipped host entry");
	}
	(void) strncpy ( (*new_host).public_name, hostname,
			host_device - hostname );
	(*new_host).public_name[host_device - hostname] = '\0';
    }
    else
    {
	(*new_host).public_name = hostname;
    }
    if ( ( addr = r_get_inet_addr_from_host ( (*new_host).public_name,
					     &local ) ) == 0 )
    {
	(void) fprintf (stderr, "Error getting Internet address of: \"%s\"\n",
			hostname);
	m_free (hostname);
	m_free ( (char *) new_host );
	return (FALSE);
    }
    (*new_host).addr = addr;
    (*new_host).slave = connection;
    (*new_host).next = NULL;
    /*  Add to list  */
    if (hostlist == NULL)
    {
	hostlist = new_host;
	(*new_host).prev = NULL;
    }
    else
    {
	/*  Find end of list  */
	for (last_host = hostlist; (*last_host).next != NULL;
	     last_host = (*last_host).next);
	(*last_host).next = new_host;
	(*new_host).prev = last_host;
    }
    *info = (void *) new_host;
    ++hostcount;
    return (TRUE);
}   /*  End Function new_host_func  */

void host_lost_func (connection, info)
/*  This routine will register the closing (loss) of a connection from a host.
    The connection will be given by  connection  .
    The connection information pointer will be given by  info  .
    The routine returns nothing.
*/
Connection connection;
void *info;
{
    hosttype *host;
    extern flag keep_going;
    extern unsigned int hostcount;

    host = (hosttype *) info;
    if (keep_going)
    {
	(void) fprintf (stderr, "Lost connection to slave on host: \"%s\"\n",
			(*host).alias);
    }
    --hostcount;
    keep_going = FALSE;
}   /*  End Function host_lost_func  */

hosttype *get_host_info (hostname)
/*  This routine will get the host information for a specified host.
    The host name must be pointed to by  hostname  .
    The routine returns a pointer to the information on success,
    else it returns NULL (indicating no information for that host available).
*/
char *hostname;
{
    hosttype *host;
    extern hosttype *hostlist;

    for (host = hostlist; host != NULL; host = (*host).next)
    {
	if (strcmp (hostname, (*host).alias) == 0)
	{
	    /*  Found it  */
	    return (host);
	}
    }
    return (NULL);
}   /*  End Function get_host_info  */

flag new_module_control (connection, info)
/*  This routine will register the opening of a connection from a new module.
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on successful registration,
    else it returns FALSE (indicating the connection should be closed).
*/
Connection connection;
void **info;
{
    long x;
    long y;
    long pid;
    Channel channel;
    char *hostname;
    char *module_name;
    hosttype *host;
    moduletype *new_module;
    moduletype *last_module;
    extern moduletype *latest_module;
    extern moduletype *modulelist;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "new_module_control";

    channel = conn_get_channel (connection);
    if ( ( module_name = pio_read_string (channel, (unsigned int *) NULL) )
	== NULL )
    {
	(void) fprintf (stderr, "Error reading module name from module\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if ( ( hostname = pio_read_string (channel, (unsigned int *) NULL) )
	== NULL )
    {
	(void) fprintf (stderr, "Error reading hostname from module\t%s\n",
			sys_errlist[errno]);
	m_free (module_name);
	return (FALSE);
    }
    if (pio_read32s (channel, &x) != TRUE)
    {
	(void) fprintf (stderr, "Error reading x value from module\t%s\n",
			sys_errlist[errno]);
	m_free (module_name);
	m_free (hostname);
	return (FALSE);
    }
    if (pio_read32s (channel, &y) != TRUE)
    {
	(void) fprintf (stderr, "Error reading y value from module\t%s\n",
			sys_errlist[errno]);
	m_free (module_name);
	m_free (hostname);
	return (FALSE);
    }
    if (pio_read32s (channel, &pid) != TRUE)
    {
	(void) fprintf (stderr, "Error reading PID from module\t%s\n",
			sys_errlist[errno]);
	m_free (module_name);
	m_free (hostname);
	return (FALSE);
    }
    if ( ( host = get_host_info (hostname) ) == NULL )
    {
	(void) fprintf (stderr, "Spurious module connection from: \"%s\"\n",
			hostname);
	m_free (module_name);
	m_free (hostname);
	return (FALSE);
    }
    (void) fprintf (stderr, "New module: \"%s\" on: \"%s\" PID: %ld\n",
		    module_name, hostname, pid);
    m_free (hostname);
    if ( ( new_module = (moduletype *) m_alloc (sizeof *new_module) )
	== NULL )
    {
	m_abort (function_name, "module entry");
    }
    (*new_module).name = module_name;
    (*new_module).host = host;
    (*new_module).x = x;
    (*new_module).y = y;
    (*new_module).pid = pid;
    (*new_module).control = connection;
    (*new_module).stdio = NULL;
    (*new_module).port_number = -1;
    (*new_module).shut_down = FALSE;
    (*new_module).first_connection = NULL;
    (*new_module).next = NULL;
    /*  Add to list  */
    if (modulelist == NULL)
    {
	modulelist = new_module;
	(*new_module).prev = NULL;
    }
    else
    {
	/*  Find end of list  */
	for (last_module = modulelist; (*last_module).next != NULL;
	     last_module = (*last_module).next);
	(*last_module).next = new_module;
	(*new_module).prev = last_module;
    }
    *info = (void *) new_module;
    if (latest_module != NULL)
    {
	(void) fprintf (stderr, "latest_module not reset\n");
	a_prog_bug (function_name);
    }
    latest_module = new_module;
    return (TRUE);
}   /*  End Function new_module_control  */

flag module_control_event (connection, info)
/*  This routine will read in data from the connection given by  connection
    and will write any appropriate information to the pointer pointed to by
    info  .
    The routine returns TRUE on successful reading,
    else it returns FALSE (indicating the connection should be closed).
*/
Connection connection;
void **info;
{
    unsigned long data;
    unsigned long port_number;
    unsigned long host_addr;
    unsigned long connection_id;
    Channel channel;
    char *protocol_name;
    moduletype *module;
    extern unsigned int connectioncount;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "module_control_event";

    module = (moduletype *) *info;
    channel = conn_get_channel (connection);
    /*  Get data from module  */
    if (pio_read32 (channel, &data) != TRUE)
    {
	(void) fprintf (stderr, "Error reading data from module\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    switch (data)
    {
      case CM_LIB_PORT_NUMBER:
	(void) fprintf (stderr, "Port number\n");
	if (pio_read32 (channel, &port_number) != TRUE)
	{
	    (void) fprintf (stderr,
			    "Error reading port number from module\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	(*module).port_number = port_number;
	break;
      case CM_LIB_NEW_CONNECTION:
	(void) fprintf (stderr, "New connection\n");
	if ( ( protocol_name = pio_read_string (channel,
						(unsigned int *) NULL) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error reading string\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_UNDEF_ERROR);
	}
	if (pio_read32 (channel, &host_addr) != TRUE)
	{
	    (void) fprintf (stderr,
			    "Error reading Internet address from module\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	if (pio_read32 (channel, &port_number) != TRUE)
	{
	    (void) fprintf (stderr,
			    "Error reading port number from module\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	if (pio_read32 (channel, &connection_id) != TRUE)
	{
	    (void) fprintf (stderr,
			    "Error reading Connection ID from module\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	++connectioncount;
	break;
      case CM_LIB_CONNECTION_CLOSED:
	(void) fprintf (stderr, "Connection closed\n");
	if (pio_read32 (channel, &connection_id) != TRUE)
	{
	    (void) fprintf (stderr,
			    "Error reading connection ID from module\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	break;
      default:
	(void) fprintf (stderr,
			"Illegal value: %lu sent by module: %s on host: %s\n",
			data, (*module).name, (* (*module).host ).alias);
	return (FALSE);
/*
        break;
*/
    }
    return (TRUE);
}   /*  End Function module_control_event  */

void module_control_lost (connection, info)
/*  This routine will register the closing (loss) of a connection from a
    module.
    The connection will be given by  connection  .
    The connection information pointer will be given by  info  .
    The routine returns nothing.
*/
Connection connection;
void *info;
{
    moduletype *module;
    extern flag keep_going;
    extern moduletype *modulelist;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    module = (moduletype *) info;
    if (!(*module).shut_down)
    {
	(void) fprintf (stderr,
			"Module: \"%s\" on host: \"%s\" PID: %d  died\n",
			(*module).name, (* (*module).host ).alias,
			(*module).pid);
    }
    keep_going = FALSE;
    /*  Remove module from list  */
    if ( (*module).next != NULL )
    {
	(* (*module).next ).prev = (*module).prev;
    }
    if ( (*module).prev == NULL )
    {
	modulelist = (*module).next;
    }
    else
    {
	(* (*module).prev ).next = (*module).next;
    }
    m_free ( (*module).name );
    m_free ( (char *) module );
}   /*  End Function module_control_lost  */

flag new_module_stdio (connection, info)
/*  This routine will register the opening of a Standard IO connection from a
    module.
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on successful registration,
    else it returns FALSE (indicating the connection should be closed).
*/
Connection connection;
void **info;
{
    long pid;
    Channel channel;
    char *hostname;
    hosttype *host;
    moduletype *module;
    extern moduletype *modulelist;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "new_module_stdio";

    channel = conn_get_channel (connection);
    if ( ( hostname = pio_read_string (channel, (unsigned int *) NULL) )
	== NULL )
    {
	(void) fprintf (stderr, "Error reading hostname from module\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if (pio_read32s (channel, &pid) != TRUE)
    {
	(void) fprintf (stderr, "Error reading PID from module\t%s\n",
			sys_errlist[errno]);
	m_free (hostname);
	return (FALSE);
    }
    if ( ( host = get_host_info (hostname) ) == NULL )
    {
	(void) fprintf (stderr,
			"Spurious module stdio connection from: \"%s\"\n",
			hostname);
	m_free (hostname);
	return (FALSE);
    }
    m_free (hostname);
    /*  Search for module  */
    for (module = modulelist; module != NULL; module = (*module).next)
    {
	if ( (host == (*module).host) && (pid == (*module).pid) )
	{
	    /*  Found it!  */
	    if ( (*module).stdio != NULL )
	    {
		(void) fprintf (stderr,
				"Repeated \"conn_mngr_stdio\" connection");
		(void) fprintf (stderr,
				" from module: \"%s\" on host: \"%s\"\n",
				(*module).name, (*host).alias);
		return (FALSE);
	    }
	    (*module).stdio = connection;
	    *info = (void *) module;
	    return (TRUE);
	}
    }
    (void) fprintf (stderr,
		    "\"conn_mngr_stdio\" connection from unknown module");
    (void) fprintf (stderr, " on host: \"%s\" with PID: %ld\n",
		    (*host).alias, pid);
    return (FALSE);
}   /*  End Function new_module_stdio  */

flag module_stdio_output (connection, info)
/*  This routine will read in data from the connection given by  connection
    and will write any appropriate information to the pointer pointed to by
    info  .
    The routine returns TRUE on successful reading,
    else it returns FALSE (indicating the connection should be closed).
*/
Connection connection;
void **info;
{
    int bytes_readable;
    Channel channel;
    char *buffer;
    moduletype *module;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "module_stdio_output";

    (void) fprintf (stderr, "%s: started\n", function_name);
    module = (moduletype *) *info;
    channel = conn_get_channel (connection);
    if ( ( bytes_readable = ch_get_bytes_readable (channel) ) < 0 )
    {
	(void) fprintf (stderr, "Error getting bytes readable\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    if ( ( buffer = m_alloc (bytes_readable) ) == NULL )
    {
	m_abort (function_name, "buffer");
    }
    if (ch_read (channel, buffer, bytes_readable) < bytes_readable)
    {
	(void) fprintf (stderr, "Error reading stdio connection\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    (void) fprintf (stderr, "%s!%s: %s", (* (*module).host ).alias,
		    (*module).name, buffer);
    m_free (buffer);
    return (TRUE);
}   /*  End Function module_stdio_output  */

void module_stdio_lost (connection, info)
/*  This routine will register the closing (loss) of a connection from a
    module.
    The connection will be given by  connection  .
    The connection information pointer will be given by  info  .
    The routine returns nothing.
*/
Connection connection;
void *info;
{
    moduletype *module;
    extern moduletype *modulelist;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    module = (moduletype *) info;
    if (!(*module).shut_down)
    {
	(void) fprintf (stderr,
			"Module: \"%s\" on host: \"%s\" PID: %d closed stdio\n",
			(*module).name, (* (*module).host ).alias,
			(*module).pid);
    }
    (*module).stdio = NULL;
}   /*  End Function module_stdio_lost  */

static void stop_func (pid, sig)
/*  This routine is called when a child process is stopped.
    The process ID is given by  pid  and the signal which stopped the process
    is given by  sig  .
    The routine returns nothing.
*/
int pid;
int sig;
{
    (void) fprintf (stderr, "WARNING: child: %d received stop signal: %d\n",
		    pid, sig);
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
    extern flag clean_child_exit;
    extern unsigned int child_count;
    static char function_name[] = "term_func";

    clean_child_exit = FALSE;
    (void) fprintf (stderr,
		    "WARNING: child: %d received termination signal: %d\n",
		    pid, sig);
    if (child_count < 1)
    {
	(void) fprintf (stderr, "But  child_count  is zero!\n");
	a_prog_bug (function_name);
    }
    --child_count;
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
    extern flag clean_child_exit;
    extern unsigned int child_count;
    static char function_name[] = "exit_func";

    if (value == 0)
    {
	if (child_count < 1)
	{
	    (void)fprintf(stderr,
			  "Child exited cleanly but  child_count  is zero!\n");
	    a_prog_bug (function_name);
	}
	--child_count;
	return;
    }
    clean_child_exit = FALSE;
    (void) fprintf (stderr,
		    "WARNING: child: %d exited with status: %d\n",
		    pid, value);
    if (child_count < 1)
    {
	(void) fprintf (stderr, "But  child_count  is zero!\n");
	a_prog_bug (function_name);
    }
    --child_count;
}   /*  End Function exit_func  */

/*  The following code is redundant because the slave connections are all
    closed elsewhere.
*/
#ifdef obsolete
static void wait_for_slaves_to_disconnect (timeout)
/*  This routine will wait for all slaves to disconnect.
    The timeout in seconds must be given by  timeout  .
    The routine returns nothing.
*/
unsigned int timeout;
{
    extern unsigned int hostcount;

    (void) fprintf (stderr,
		    "Waiting: %u seconds for all slaves to disconnect\n",
		    timeout);
    timeout *= 10;
    while ( (timeout > 0) && (hostcount > 0) )
    {
	cm_poll (FALSE);
	chm_poll (100);
	(void) fprintf (stderr, ".");
	--timeout;
    }
    (void) fprintf (stderr, "\n");
    if (hostcount > 0)
    {
	(void) fprintf (stderr,
			"Timed out waiting for slaves to die: %u left\n",
			hostcount);
    }
}   /*  End Function wait_for_slaves_to_disconnect  */
#endif

static void disconnect_from_slaves ()
/*  This routine will disconnect from all slaves.
    The routine returns nothing.
*/
{
    Connection conn;
    unsigned int conn_count;
    unsigned int num_conn;
    static char function_name[] = "disconnect_from_slaves";

    num_conn = conn_get_num_serv_connections ("conn_mngr_slave");
    for (conn_count = 0; conn_count < num_conn; ++conn_count)
    {
	if ( ( conn = conn_get_serv_connection ("conn_mngr_slave",
						conn_count) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "Server connection: %u for protocol: \"conn_mngr_slave\" not found\n",
			    conn_count);
	    a_prog_bug (function_name);
	}
	(void) conn_close (conn);
    }
}   /*  End Function disconnect_from_slaves  */

static void notify_quiescent ()
/*  This routine will notify all modules that the Connection Management shell
    is quiescent.
    The routine returns nothing.
*/
{
    Channel channel;
    moduletype *module;
    extern moduletype *modulelist;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "notify_quiescent";

    for (module = modulelist; module != NULL; module = (*module).next)
    {
	channel = conn_get_channel ( (*module).control );
	if (pio_write32 (channel, CM_TOOL_NOW_QUIESCENT) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing command\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (ch_flush (channel) != TRUE)
	{
	    (void) fprintf (stderr, "Error flushing channel\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
    }
}   /*  End Function notify_quiescent  */

static void shutdown_modules ()
/*  This routine will close all connections to modules.
    The routine returns nothing.
*/
{
    moduletype *module, *next;
    extern moduletype *modulelist;

    for (module = modulelist; module != NULL; module = next)
    {
	next = (*module).next;
	if ( (*module).control != NULL )
	{
	    (*module).shut_down = TRUE;
	    if (conn_close ( (*module).control ) != TRUE)
	    {
		(void) fprintf (stderr, "Error closing control connection\n");
	    }
	}
    }
}   /*  End Function shutdown_modules  */

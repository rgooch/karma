/*LINTLIBRARY*/
/*PREFIX:"module_"*/
/*  module.c

    This code provides initialisation, control and sequencing for many modules.

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

/*  This file contains all routines needed for the initialisation and execution
  of generic modules.


    Written by      Richard Gooch   4-OCT-1992

    Updated by      Richard Gooch   15-DEC-1992

    Updated by      Richard Gooch   23-DEC-1992: Added Connection Management
  tool support.

    Updated by      Richard Gooch   1-JAN-1993: Took account of change to
  ch_gets  .

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   7-MAR-1993: Added  #ifdef ARCH_VXMX

    Updated by      Richard Gooch   4-APR-1993: Took account of change to
  conn_register_server_protocol  .

    Updated by      Richard Gooch   8-APR-1993: Worked around bug in sprintf(3)
  in C library for VX/MVX which caused prompt to be garbled.

    Updated by      Richard Gooch   24-MAY-1993: Changed from  CAN_FORK  to
  HAS_GETRUSAGE.

    Updated by      Richard Gooch   3-AUG-1993: Changed from maximum 1 server
  connection for "command_line" protocol to unlimited connections.

    Updated by      Richard Gooch   20-AUG-1993: Changed from becoming server
  if controlled by CM Tool and supporting incoming "multi_array" connections to
  if controlled by CM Tool and module has a service number.

    Updated by      Richard Gooch   14-SEP-1993: Changed from becoming server
  if controlled by CM Tool and module has a service number to if controlled by
  CM Tool and  server  parameter is TRUE.

    Last updated by Richard Gooch   19-SEP-1993: Added quoting of command line
  parameters which have spaces.


*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#ifdef ARCH_VXMVX
#include <vxsignal.h>
#else
#include <signal.h>
#endif
#include <sys/resource.h>
#include <string.h>
#include <os.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_dsxfr.h>
#include <karma_conn.h>
#include <karma_arln.h>
#include <karma_chm.h>
#include <karma_im.h>
#include <karma_hi.h>
#include <karma_s.h>
#include <karma_r.h>

#define COMMAND_LINE_LENGTH 4096
#define WALL_CLOCK_TIME_DETECT 60
#define CPUTIME_DETECT 2

/*  Local functions  */
static flag process_one_line (/* line, decode_func */);
static flag read_a_line (/* line, length, prompt */);
static flag command_read_func (/* connection, info */);

/*  Private data  */
static flag (*decode_function) () = NULL;


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void module_run (argc, argv, name_string, version_string, decode_func,
		 max_incoming, max_outgoing, server)
/*  This routine will initialise and then run a generic module.
    The number of command line arguments must be given by  argc  .
    The command line arguments must be pointed to by  argv  .
    The name of the module must be given by  name_string  .
    The version date of the module must be given by  version_string  .
    The command line decode function must be pointed to by  decode_func  .The
    interface to this function is as follows:

    flag decode_func (command, fp)
    *   This function is called whenever a command string is to be processed.
        The command string will be pointed to by  command  .
	The output file must be pointed to by  fp  .
	The routine returns TRUE on success, else it returns FALSE (indicating
	that the module should terminate immediately).
    *
    char *command;
    FILE *fp;

    The maximum number of incoming "multi_array" protocol connections must be
    given by  max_incoming  .
    If this is less than 0, no connections are permitted. If this is 0, an
    unlimited number of connections is permitted.
    The maximum number of outgoing "multi_array" protocol connections must be
    given by  max_outgoing  .
    If this is less than 0, no connections are permitted. If this is 0, an
    unlimited number of connections is permitted.
    If the module can operate as a server (for any protocol), then the value of
    server  must be TRUE.
    The routine returns nothing. Upon return from the function, the module
    should exit.
*/
int argc;
char **argv;
char *name_string;
char *version_string;
flag (*decode_func) ();
int max_incoming;
int max_outgoing;
flag server;
{
    int arg_count;
    int def_port_number;
    unsigned int server_port_number;
    char line[COMMAND_LINE_LENGTH];
    char prompt[STRING_LENGTH + 3];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    extern char module_name[STRING_LENGTH + 1];
    extern flag (*decode_function) ();
    static char function_name[] = "module_run";

    FLAG_VERIFY (server);
    /*  Initialise module  */
    decode_function = decode_func;
    im_register_module_name (name_string);
    im_register_module_version_date (version_string);
    conn_register_managers (chm_manage, chm_unmanage, ( void (*) () ) NULL);
    if (conn_controlled_by_cm_tool () && server)
    {
	/* Controlled by Connection Management tool and has a service number */
	/*  Get default port number  */
	if ( ( def_port_number = r_get_def_port ( module_name,
						 r_getenv ("DISPLAY") ) ) < 0 )
	{
	    (void) fprintf (stderr, "Could not get default port number\n");
	    exit (RV_UNDEF_ERROR);
	}
	server_port_number = def_port_number;
	if (conn_become_server (&server_port_number, CONN_MAX_INSTANCES)
	    != TRUE)
	{
	    (void) fprintf (stderr, "Module not operating as Karma server\n");
	    exit (RV_UNDEF_ERROR);
	}
    }
    dsxfr_register_connection_limits (max_incoming, max_outgoing);
    conn_register_server_protocol ("command_line", 0, 0,
				   ( flag (*) () ) NULL, command_read_func,
				   ( void (*) () ) NULL);
    /*  Set up control_c handler  */
    if ( (int) signal (SIGINT, s_int_handler) == -1 )
    {
	(void) fprintf (stderr, "Error setting control_c handler\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
#ifdef SIGTERM
    /*  Set up sigTERM handler  */
    if ( (int) signal (SIGTERM, s_term_handler) == -1 )
    {
	(void) fprintf (stderr, "Error setting sigTERM handler\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) fprintf (stderr, "Module: %s quitting\n", module_name);
	(void) exit (RV_SYS_ERROR);
    }
#endif
    if (conn_controlled_by_cm_tool () != TRUE)
    {
	/*  Read in defaults  */
	hi_read (module_name, decode_func);
    }
    if (s_check_for_int () == TRUE)
    {
	(void) fprintf (stderr, "control_c abort\n");
	(void) exit (RV_CONTROL_C);
    }
    if (argc > 1)
    {
	/*  Concatenate command line parameters into a single line  */
	(void) strcpy (line, argv[1]);
	for (arg_count = 2; arg_count < argc; ++arg_count)
	{
	    if (strchr (argv[arg_count], ' ') == NULL)
	    {
		/*  No spaces in parameter: raw copy  */
		(void) strcat (line, " ");
		(void) strcat (line, argv[arg_count]);
	    }
	    else
	    {
		/*  Spaces: add some quotes  */
		(void) strcat (line, " \"");
		(void) strcat (line, argv[arg_count]);
		(void) strcat (line, "\"");
	    }
	}
	if (process_one_line (line, decode_func) != TRUE)
	{
	    exit (RV_UNDEF_ERROR);
	}
	/*  Save defaults  */
	hi_write (module_name, decode_func);
	return;
    }
    /*  Prompt user for commands  */
    /*  Generate prompt. Don't use  sprintf(3)  because it's broken in the
	C library for the VX/MVX
	*/
    (void) strcpy (prompt, module_name);
    (void) strcat (prompt, "> ");
    /*  Read lines and processes until eof on input  */
    while ( read_a_line (line, COMMAND_LINE_LENGTH, prompt) )
    {
	if (process_one_line (line, decode_func) != TRUE)
	{
	    exit (RV_UNDEF_ERROR);
	}
    }
    if (conn_controlled_by_cm_tool () != TRUE)
    {
	/*  Save defaults  */
	hi_write (module_name, decode_func);
    }
}   /*  End Function module_run   */


/*  Private functions follow  */

static flag process_one_line (line, decode_func)
char *line;
flag (*decode_func) ();
{
#ifdef HAS_GETRUSAGE
    long wall_clock_time_taken;
    long cputime_taken;
    struct timeval start_time;
    struct timeval stop_time;
    struct rusage start_usage;
    struct rusage stop_usage;
    static struct timezone tz = {0, 0};
#endif  /*  HAS_GETRUSAGE  */
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

#ifdef dummy
    log_input_line (line);
#endif
#ifdef HAS_GETRUSAGE
    if (gettimeofday (&start_time, &tz) != 0)
    {
	(void) fprintf (stderr, "Error getting time of day\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
    if (getrusage (RUSAGE_SELF, &start_usage) != 0)
    {
	(void) fprintf (stderr, "Error getting resource usage\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
#endif  /*  HAS_GETRUSAGE  */
    (*decode_func) (line, stderr);
    if (s_check_for_int () == TRUE)
    {
	(void) fprintf (stderr, "control_c abort\n");
	return (TRUE);
    }
#ifdef HAS_GETRUSAGE
    if (gettimeofday (&stop_time, &tz) != 0)
    {
	(void) fprintf (stderr, "Error getting time of day\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
    if (getrusage (RUSAGE_SELF, &stop_usage) != 0)
    {
	(void) fprintf (stderr, "Error getting resource usage\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
    wall_clock_time_taken = stop_time.tv_sec - start_time.tv_sec;
    cputime_taken = (stop_usage.ru_utime.tv_sec -
		     start_usage.ru_utime.tv_sec);
    if ( (wall_clock_time_taken >= WALL_CLOCK_TIME_DETECT) &&
	(cputime_taken >= CPUTIME_DETECT) )
    {
	(void) fprintf (stderr,
			"Long operation finished. Time taken: %ld seconds\t%ld cpu seconds%c\n",
			wall_clock_time_taken, cputime_taken, BEL);
    }
#endif  /*  HAS_GETRUSAGE  */
    return (TRUE);
}   /*  End Function process_one_line  */

static flag read_a_line (line, length, prompt)
char *line;
unsigned int length;
char *prompt;
{
    if (arln_read_from_stdin (line, length, prompt) == FALSE)
    {
	return (FALSE);
    }
    if (*line == '\0')
    {
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function read_a_line  */

static flag command_read_func (connection, info)
/*  This routine will read in data from the connection given by  connection
    and will write any appropriate information to the pointer pointed to by
    info  .
    The routine returns TRUE on successful reading,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will be called upon connection closure.
*/
Connection connection;
void **info;
{
    extern flag (*decode_function) ();
    char buffer[STRING_LENGTH];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "command_read_func";

    if (ch_gets (conn_get_channel (connection), buffer, STRING_LENGTH) != TRUE)
    {
	(void) fprintf (stderr, "Error reading string\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return ( (*decode_function) (buffer, stderr) );
}   /*  End Function command_read_func  */

/*  slave_setup.c

    Slave process setup file for Connection Management tool and shell.

    Copyright (C) 1993,1994,1995  Richard Gooch

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
    This file contains the slave setup code.


    Written by      Richard Gooch   12-DEC-1992

    Updated by      Richard Gooch   26-DEC-1992

    Updated by      Richard Gooch   4-APR-1993: Took account of change to
  conn_register_server_protocol  .

    Updated by      Richard Gooch   29-SEP-1993

    Updated by      Richard Gooch   19-APR-1994: Made specification of host,
  port and display mandatory, and return position of unprocessed (optional)
  arguments.

    Updated by      Richard Gooch   1-MAR-1995: Removed code to prepend
  "/usr/local/karma/bin" to the PATH: not necessary and old PATH exceeded the
  allocated string space anyway (not a problem unless started manaully).

    Last updated by Richard Gooch   14-MAR-1995: Incremented protocol version.


    Usage:   *karma_cm_slave host port display [args...]

*/
#include <stdio.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <karma.h>
#include <karma_conn.h>
#include <karma_chm.h>
#include <karma_r.h>
#include <karma_m.h>

#define PROTOCOL_VERSION (unsigned int) 1

int slave_setup (argc, argv, cm_port_number, cm_host_addr,
		 open_func, read_func, close_func)
/*  This routine will set up a CM Tool slave.
    Command line argument information must be given by  argc  and  argv  .
    The port number of the CM Tool will be written to the storage pointed to by
    cm_port_number  .
    The Internet address of the CM Tool will be written to the storage pointed
    to by  cm_host_addr  .
    The functions to handle the "conn_mngr_slave" protocol must be given by
    open_func  ,  read_func  and  close_func  .
    The routine returns the position of the first remaining (optional) argument
*/
int argc;
char *argv[];
unsigned int *cm_port_number;
unsigned long *cm_host_addr;
flag (*open_func) ();
flag (*read_func) ();
void (*close_func) ();
{
    flag local;
    int display_num;
    int screen_num;
    int out_fd;
    int port_number;
    pid_t sid;
    char *host;
    char *path;
    char *display_host;
    char txt[STRING_LENGTH];
    extern char *sys_errlist[];
    static char usage_string[] = "Usage:\tkarma_cm_slave host port display";
    static char function_name[] = "slave_setup";

    if (argc < 3)
    {
	(void) fprintf (stderr, "%s\n", usage_string);
	exit (RV_MISSING_PARAM);
    }
    /*  Become a background process  */
    switch ( fork () )
    {
      case 0:
	/*  Child  */
	break;
      case 1:
	(void) fprintf (stderr, "Error forking\t%s\n", sys_errlist[errno]);
	exit (RV_SYS_ERROR);
/*
	break;
*/
      default:
	/*  Parent  */
	exit (RV_OK);
/*
	break;
*/
    }
    /*  Disconnect from terminal  */
    /*  Close standard input  */
    if (close (0) != 0)
    {
	(void) fprintf (stderr,
			"Error closing standard input\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    (void) sprintf ( txt, "/tmp/karma_cm_slave.log.%d", (int) getuid () );
    if ( ( out_fd = open (txt, O_CREAT | O_TRUNC | O_WRONLY,
			  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) ) < 0 )
    {
	(void) fprintf (stderr,
			"Error opening: \"%s\" for output\t%s\n",
			txt, sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Attatch log file to standard output  */
    if (dup2 (out_fd, 1) < 0)
    {
	(void) fprintf (stderr,
			"Error redirecting standard output\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Attatch log file to standard error  */
    if (dup2 (out_fd, 2) < 0)
    {
	(void) fprintf (stderr,
			"Error redirecting standard error\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Close temporary descriptor for log file  */
    if (close (out_fd) != 0)
    {
	(void) fprintf (stderr,
			"Error closing temporary descriptor for log file\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Disconnect from session  */
    if ( ( sid = setsid () ) == -1 )
    {
	(void) fprintf (stderr, "Error disconnecting from session\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    if (chdir ("/tmp") != 0)
    {
	(void) fprintf (stderr, "Error changing directory to: \"/tmp\"\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Initialise path  */
    if ( ( path = r_getenv ("PATH") ) == NULL )
    {
	(void) fprintf (stderr, "SLAVE: Strange: no path!\n");
	if (r_setenv ("PATH", "/usr/local/karma/bin") != 0)
	{
	    m_abort (function_name, "PATH environment");
	}
    }
    /*  Start connection library package  */
    conn_register_managers (chm_manage, chm_unmanage, ( void (*) () ) NULL);
    conn_register_client_protocol ("conn_mngr_slave", PROTOCOL_VERSION, 1,
				   ( flag (*) () ) NULL,
				   open_func, read_func, close_func);
    /*  Need to get info from command line  */
    host = argv[1];
    if ( ( port_number = atoi (argv[2]) ) < 0 )
    {
	(void) fprintf (stderr, "SLAVE: bad port parameter: \"%s\"\n",
			argv[2]);
	(void) fprintf (stderr, "SLAVE: %s\n", usage_string);
	exit (RV_BAD_PARAM);
    }
    /*  Get display info  */
    if ( ( display_host = r_get_host_from_display (argv[3]) ) == NULL )
    {
	exit (RV_UNDEF_ERROR);
    }
    if ( ( display_num = r_get_display_num_from_display (argv[3]) )
	< 0 )
    {
	exit (RV_UNDEF_ERROR);
    }
    if ( ( screen_num = r_get_screen_num_from_display (argv[3]) )
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
	(void) sprintf (txt, ":%d.%d", display_num, screen_num);
	if (r_setenv ("DISPLAY", txt) != 0)
	{
	    m_abort (function_name, "DISPLAY environment");
	}
    }
    else
    {
	if (r_setenv ("DISPLAY", argv[3]) != 0)
	{
	    m_abort (function_name, "DISPLAY environment");
	}
    }
    if ( ( *cm_host_addr = r_get_inet_addr_from_host (host, (flag *) NULL) )
	== 0 )
    {
	(void) fprintf (stderr,
			"SLAVE: Error getting Internet address of host: \"%s\"\t%s\n",
			host, sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    *cm_port_number = port_number;
    /*  Now have host and port number  */
    /*  Try to connect  */
    (void) fprintf (stderr, "SLAVE: attempting connect to: %s %u\n",
		    host, *cm_port_number);
    if ( !conn_attempt_connection (host, *cm_port_number, "conn_mngr_slave") )
    {
	(void) fprintf (stderr,
			"SLAVE: Error connecting to Connection Management tool\n");
	exit (RV_UNDEF_ERROR);
    }
    (void) fprintf (stderr, "SLAVE: connected\n");
    return (4);
} /*  End Function slave_setup  */

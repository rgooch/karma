/*  rx.c

    Source file for  rx  (data srtucture receiver module).

    Copyright (C) 1993-1996  Richard Gooch

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

/*  This Karma module will read in a Karma data file from a connection and
    will write it out to a disc.


    Written by      Richard Gooch   26-OCT-1992

    Updated by      Richard Gooch   14-FEB-1993

    Updated by      Richard Gooch   4-APR-1993: Took account of change to
  conn_register_server_protocol  .

    Updated by      Richard Gooch   24-MAY-1993: Changed from  CAN_FORK  to
  HAS_GETRUSAGE  .

    Updated by      Richard Gooch   6-OCT-1993: Changed over to  panel_
  package for command line user interface.

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   10-MAR-1995: Added support for "spray"
  protocol.

    Updated by      Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   22-AUG-1996: Upgraded "spray" protocol to
  support synchronisation.


*/
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#ifdef OS_VXMVX
#  include <vxsignal.h>
#else
#  include <signal.h>
#endif
#include <sys/resource.h>
#include <string.h>
#include <errno.h>
#include <os.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_conn.h>
#include <karma_dsrw.h>
#include <karma_arln.h>
#include <karma_chm.h>
#include <karma_pio.h>
#include <karma_ds.h>
#include <karma_ch.h>
#include <karma_ex.h>
#include <karma_hi.h>
#include <karma_im.h>
#include <karma_a.h>
#include <karma_s.h>
#include <karma_r.h>
#include <karma_m.h>


#define VERSION "1.2"

#define COMMAND_LINE_LENGTH 4096
#define WALL_CLOCK_TIME_DETECT 60
#define CPUTIME_DETECT 2


/*  Structure definitions  */
typedef struct
{
    unsigned int bytes_to_read;
    flag synchronous;
} spray_info;

STATIC_FUNCTION (flag read_multi_array, (Connection connection, void **info) );
static flag process_one_line (/* line, decode_func */);
static flag internal_decode_func (/* line, fp */);
STATIC_FUNCTION (flag open_spray, (Connection connection, void **info) );
STATIC_FUNCTION (flag read_spray, (Connection connection, void **info) );
STATIC_FUNCTION (void close_spray, (Connection connection, void *info) );


static char *arrayfile = "";
static flag synchronous_spray = FALSE;


int main (int argc, char **argv)
{
    KControlPanel panel;
    int def_port_number;
    unsigned int server_port_number;
    char line[COMMAND_LINE_LENGTH];
    char prompt[STRING_LENGTH + 3];
    extern char *sys_errlist[];
    extern char module_name[STRING_LENGTH + 1];
    static char function_name[] = "main";

    /*  Initialise module  */
    im_register_module_name ("rx");
    im_register_module_version_date (VERSION);
    conn_register_managers (chm_manage, chm_unmanage, ( void (*) () ) NULL);
    conn_register_server_protocol ("multi_array", 0, 0,
				   ( flag (*) () ) NULL,
				   read_multi_array,
				   ( void (*) () ) NULL);
    conn_register_server_protocol ("spray", 1, 0,
				   open_spray, read_spray, close_spray);
    if ( ( def_port_number = r_get_def_port (module_name, NULL) ) < 0 )
    {
	fprintf (stderr, "Could not get default port number\n");
	exit (RV_UNDEF_ERROR);
    }
    server_port_number = def_port_number;
    if (conn_become_server (&server_port_number, CONN_MAX_INSTANCES) != TRUE)
    {
	fprintf (stderr, "Error becoming server\n");
	exit (RV_UNDEF_ERROR);
    }
    def_port_number = server_port_number;
    fprintf (stderr, "Port allocated: %d\n", def_port_number);
    /*  Set up control_c handler  */
    if ( (int) signal (SIGINT, s_int_handler) == -1 )
    {
	fprintf (stderr, "Error setting control_c handler\t%s%c\n",
			sys_errlist[errno], BEL);
	exit (RV_SYS_ERROR);
    }
#ifdef SIGTERM
    /*  Set up sigTERM handler  */
    if ( (int) signal (SIGTERM, s_term_handler) == -1 )
    {
	fprintf (stderr, "Error setting sigTERM handler\t%s%c\n",
			sys_errlist[errno], BEL);
	exit (RV_SYS_ERROR);
    }
#endif
    /*  Setup user interface  */
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel form");
    }
    panel_add_item (panel, "synchronous_spray", "flag", PIT_FLAG,
		    &synchronous_spray,
		    PIA_END);
    panel_add_item (panel, "arrayfile", "output filename", K_VSTRING,
		    &arrayfile,
		    PIA_END);
    panel_push_onto_stack (panel);
    /*  Read in defaults  */
    hi_read (module_name, internal_decode_func);
    if (s_check_for_int () == TRUE)
    {
	fprintf (stderr, "control_c abort\n");
	exit (RV_CONTROL_C);
    }
    if (argc > 1)
    {
	/*  Concatenate command line arguments into a single line  */
	module_process_argvs (argc - 1, argv + 1, ( flag (*) () ) NULL);
	/*  Save defaults  */
	hi_write (module_name, internal_decode_func);
	exit (RV_OK);
    }
    /*  Prompt user for commands  */
    /*  Generate prompt. Don't use  sprintf(3)  because it's broken in the
	C library for the VX/MVX
	*/
    strcpy (prompt, module_name);
    strcat (prompt, "> ");
    /*  Read lines and processes until eof on input  */
    while ( arln_read_from_stdin (line, COMMAND_LINE_LENGTH, prompt) &&
	   process_one_line (line, ( flag (*) () ) NULL) );
    /*  Save defaults  */
    hi_write (module_name, internal_decode_func);
    return (RV_OK);
}   /*  End Function main   */


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
    extern char *sys_errlist[];
#endif  /*  HAS_GETRUSAGE  */

#ifdef dummy
    log_input_line (line);
#endif
#ifdef HAS_GETRUSAGE
    if (gettimeofday (&start_time, &tz) != 0)
    {
	fprintf (stderr, "Error getting time of day\t%s%c\n",
			sys_errlist[errno], BEL);
	exit (RV_SYS_ERROR);
    }
    if (getrusage (RUSAGE_SELF, &start_usage) != 0)
    {
	fprintf (stderr, "Error getting resource usage\t%s%c\n",
			sys_errlist[errno], BEL);
	exit (RV_SYS_ERROR);
    }
#endif  /*  HAS_GETRUSAGE  */
    if (panel_process_command_with_stack (line, decode_func, stderr) != TRUE)
    {
	return (FALSE);
    }
    if (s_check_for_int () == TRUE)
    {
	fprintf (stderr, "control_c abort\n");
	return (TRUE);
    }
#ifdef HAS_GETRUSAGE
    if (gettimeofday (&stop_time, &tz) != 0)
    {
	fprintf (stderr, "Error getting time of day\t%s%c\n",
			sys_errlist[errno], BEL);
	exit (RV_SYS_ERROR);
    }
    if (getrusage (RUSAGE_SELF, &stop_usage) != 0)
    {
	fprintf (stderr, "Error getting resource usage\t%s%c\n",
			sys_errlist[errno], BEL);
	exit (RV_SYS_ERROR);
    }
    wall_clock_time_taken = stop_time.tv_sec - start_time.tv_sec;
    cputime_taken = (stop_usage.ru_utime.tv_sec -
		     start_usage.ru_utime.tv_sec);
    if ( (wall_clock_time_taken >= WALL_CLOCK_TIME_DETECT) &&
	(cputime_taken >= CPUTIME_DETECT) )
    {
	fprintf (stderr,
			"Long operation finished. Time taken: %ld seconds\t%ld cpu seconds%c\n",
			wall_clock_time_taken, cputime_taken, BEL);
    }
#endif  /*  HAS_GETRUSAGE  */
    return (TRUE);
}   /*  End Function process_one_line  */

static flag read_multi_array (Connection connection, void **info)
/*  This routine will read in data from the connection given by  connection
    and will write any appropriate information to the pointer pointed to by
    info  .
    The routine returns TRUE on successful reading,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will be called upon connection closure.
*/
{
    Channel channel;
    multi_array *multi_desc;
    extern char *arrayfile;

    channel = conn_get_channel (connection);
    if ( ( multi_desc = dsrw_read_multi (channel) ) == NULL )
    {
	/*  Bad read: ignore  */
	return (FALSE);
    }
    if (strlen (arrayfile) > 0)
    {
	/*  Should write arrayfile  */
	if (dsxfr_put_multi (arrayfile, multi_desc) != TRUE)
	{
	    fprintf (stderr, "Error writing arrayfile\n");
	}
    }
    ds_dealloc_multi (multi_desc);
    return (TRUE);
}   /*  End Function read_multi_array  */

static flag internal_decode_func (line, fp)
char *line;
FILE *fp;
{
    return ( panel_process_command_with_stack (line, ( flag (*) () ) NULL,
					       fp) );
}

static flag open_spray (Connection connection, void **info)
/*  [SUMMARY] Connection open event callback.
    [PURPOSE] This routine is called when a connection opens.
    <connection> The connection object.
    <info> A pointer to the arbitrary information pointer. This may be modified
    [RETURNS] TRUE on successful registration, else FALSE (indicating the
    connection should be closed).
    [NOTE] The <<close_func>> will not be called if this routine returns
    FALSE.
*/
{
    spray_info *sinfo;
    static char function_name[] = "open_spray";

    if ( ( sinfo = (spray_info *) m_alloc (sizeof *sinfo) ) == NULL )
    {
	m_error_notify (function_name, "spray info");
	return (FALSE);
    }
    sinfo->bytes_to_read = 0;
    sinfo->synchronous = FALSE;
    *info = sinfo;
    return (TRUE);
}   /*  End Function open_spray  */

static flag read_spray (Connection connection, void **info)
/*  [SUMMARY] Connection read event callback.
    [PURPOSE] This routine is called when data is ready to be read from a
    connection.
    <connection> The connection object.
    <info> A pointer to the arbitrary information pointer. This may be modified
    [RETURNS] TRUE on successful reading, else FALSE (indicating the connection
    should be closed).
    [NOTE] The <<close_func>> will not be called if this routine returns
    FALSE.
*/
{
    Channel channel;
    int bytes_readable;
    unsigned int bytes_to_read;
    unsigned long value;
    spray_info *sinfo = (spray_info *) *info;
    extern flag synchronous_spray;
    extern char *sys_errlist[];
    /*static char function_name[] = "read_spray";*/

    channel = conn_get_channel (connection);
    if (sinfo->bytes_to_read == 0)
    {
	/*  New transaction: read number of bytes that will need to be read  */
	if ( !pio_read32 (channel, &value) ) return (FALSE);
	sinfo->bytes_to_read = value;
	if ( !pio_read32 (channel, &value) ) return (FALSE);
	sinfo->synchronous = value;
    }
    if (synchronous_spray) bytes_readable = sinfo->bytes_to_read;
    else if ( ( bytes_readable = ch_get_bytes_readable (channel) ) < 0 )
    {
	exit (RV_SYS_ERROR);
    }
    bytes_to_read = (bytes_readable > sinfo->bytes_to_read) ? sinfo->bytes_to_read : bytes_readable;
    if (ch_drain (channel, bytes_to_read) < bytes_to_read)
    {
	fprintf (stderr, "Error reading: %u bytes\t%s\n",
		 bytes_to_read, sys_errlist[errno]);
	return (FALSE);
    }
    sinfo->bytes_to_read -= bytes_to_read;
    if (sinfo->bytes_to_read < 1)
    {
	if (sinfo->synchronous)
	{
	    if (ch_fill (channel, 1, 0) < 1)
	    {
		fprintf (stderr, "Error writing synchronisation byte\t%s\n",
			 sys_errlist[errno]);
		return (FALSE);
	    }
	    if ( !ch_flush (channel) )
	    {
		fprintf (stderr, "Error flushing synchronisation byte\t%s\n",
			 sys_errlist[errno]);
		return (FALSE);
	    }
	}
    }
    return (TRUE);
}   /*  End Function read_spray  */

static void close_spray (Connection connection, void *info)
/*  [SUMMARY] Connection close event callback.
    [PURPOSE] This routine is called when a connection closed.
    When this routine is called, this is the last chance to read any
    buffered data from the channel associated with the connection object.
    <connection> The connection object.
    <info> The arbitrary connection information pointer.
    [RETURNS] Nothing.
*/
{
    m_free (info);
}   /*  End Function close_spray  */

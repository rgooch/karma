/*  collect_struct.c

    Source file for  collect_struct  (data structure collection module).

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

/*  This Karma module will read in a Karma data file containing a single
    general data structure from a connection and cache that structure. Later, a
    multi_array data structure may be generated from all the structures and
    written out to disc.


    Written by      Richard Gooch   26-OCT-1992

    Updated by      Richard Gooch   14-FEB-1993

    Updated by      Richard Gooch   4-APR-1993: Took account of change to
  conn_register_server_protocol  .

    Updated by      Richard Gooch   24-MAY-1993: Changed from  CAN_FORK  to
  HAS_GETRUSAGE  .

    Updated by      Richard Gooch   6-OCT-1993: Changed over to  panel_
  package for command line user interface.

    Updated by      Richard Gooch   22-MAY-1994: Fixed bug in  flush_data  .

    Last updated by Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/resource.h>
#include <string.h>
#include <os.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_conn.h>
#include <karma_dsrw.h>
#include <karma_arln.h>
#include <karma_chm.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_hi.h>
#include <karma_im.h>
#include <karma_ch.h>
#include <karma_a.h>
#include <karma_s.h>
#include <karma_r.h>
#include <karma_m.h>


#define VERSION "1.1"

#define COMMAND_LINE_LENGTH 4096
#define WALL_CLOCK_TIME_DETECT 60
#define CPUTIME_DETECT 2
#define MAX_ARRAYS 1000


/*  Local functions  */


/*  Private functions  */
static flag read_multi_array ();
static char *dump_arrays ();
static char *flush_data ();
static flag command_read_func (/* connection, info */);
static flag process_one_line (/* line, decode_func */);
static flag internal_decode_func (/* line, fp */);


/*  Private data  */
static unsigned int array_num = 0;
static packet_desc *top_pack_desc[MAX_ARRAYS];
static char *top_packet[MAX_ARRAYS];


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
    im_register_module_name ("collect_struct");
    im_register_module_version_date (VERSION);
    conn_register_managers (chm_manage, chm_unmanage, ( void (*) () ) NULL);
    conn_register_server_protocol ("multi_array", 0, 1,
				   ( flag (*) () ) NULL,
				   read_multi_array,
				   ( void (*) () ) NULL);
    conn_register_server_protocol ("command_line", 0, 1,
				   ( flag (*) () ) NULL, command_read_func,
				   ( void (*) () ) NULL);
    if ( ( def_port_number = r_get_def_port (module_name, NULL) ) < 0 )
    {
	(void) fprintf (stderr, "Could not get default port number\n");
	exit (RV_UNDEF_ERROR);
    }
    server_port_number = def_port_number;
    if (conn_become_server (&server_port_number, CONN_MAX_INSTANCES) != TRUE)
    {
	(void) fprintf (stderr, "Error becoming server\n");
	exit (RV_UNDEF_ERROR);
    }
    def_port_number = server_port_number;
    (void) fprintf (stderr, "Port allocated: %d\n", def_port_number);
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
	(void) exit (RV_SYS_ERROR);
    }
#endif
    /*  Setup user interface  */
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel form");
    }
    panel_add_item (panel, "flush_data", "action",
		    PIT_FUNCTION, (void *) flush_data,
		    PIA_END);
    panel_add_item (panel, "save_arrayfile", "action",
		    PIT_FUNCTION, (void *) dump_arrays,
		    PIA_END);
    panel_push_onto_stack (panel);
    /*  Read in defaults  */
    hi_read (module_name, internal_decode_func);
    if (s_check_for_int () == TRUE)
    {
	(void) fprintf (stderr, "control_c abort\n");
	(void) exit (RV_CONTROL_C);
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
    (void) strcpy (prompt, module_name);
    (void) strcat (prompt, "> ");
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
    if (panel_process_command_with_stack (line, decode_func, stderr) != TRUE)
    {
	return (FALSE);
    }
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

static flag read_multi_array (connection, info)
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
    Channel channel;
    multi_array *multi_desc;
    extern unsigned int array_num;
    extern char *top_packet[MAX_ARRAYS];
    extern packet_desc *top_pack_desc[MAX_ARRAYS];

    channel = conn_get_channel (connection);
    if ( ( multi_desc = dsrw_read_multi (channel) ) == NULL )
    {
	/*  Bad read: ignore  */
	return (FALSE);
    }
    if (array_num > MAX_ARRAYS)
    {
	(void) fprintf (stderr, "Too many arrays: discarding\n");
	ds_dealloc_multi (multi_desc);
    }
    if ( (*multi_desc).num_arrays > 1 )
    {
	(void) fprintf (stderr, "Only one array permitted: discarding\n");
	ds_dealloc_multi (multi_desc);
    }
    top_pack_desc[array_num] = (*multi_desc).headers[0];
    top_packet[array_num] = (*multi_desc).data[0];
    (*multi_desc).headers[0] = NULL;
    (*multi_desc).data[0] = NULL;
    ds_dealloc_multi (multi_desc);
    ++array_num;
    return (TRUE);
}   /*  End Function read_multi_array  */

static char *dump_arrays (p)
/*  This routine will dump the accumulated arrays.
    The parameter  p  must point to the name of the arrayfile to create.
    The remainder of  p  is returned.
*/
char *p;
{
    unsigned int array_count;
    char *arrayfile;
    multi_array *multi_desc;
    char txt[STRING_LENGTH];
    extern unsigned int array_num;
    extern char *top_packet[MAX_ARRAYS];
    extern packet_desc *top_pack_desc[MAX_ARRAYS];
    static char function_name[] = "dump_arrays";

    if ( ( arrayfile = ex_str (p, &p) ) == NULL )
    {
	(void) fprintf (stderr, "Must specify arrayfile to create\n");
	return (NULL);
    }
    if (array_num < 1)
    {
	(void) fprintf (stderr, "No arrays accumulated yet\n");
	return (NULL);
    }
    if ( ( multi_desc = ds_alloc_multi (array_num) ) == NULL )
    {
	m_abort (function_name, "multi_array descriptor");
    }
    for (array_count = 0; array_count < array_num; ++array_count)
    {
	(void) sprintf (txt, "Frame %d", array_count);
	if ( ( (*multi_desc).array_names[array_count] = st_dup (txt) ) ==NULL )
	{
	    m_abort (function_name, "array name");
	}
	(*multi_desc).headers[array_count] = top_pack_desc[array_count];
	(*multi_desc).data[array_count] = top_packet[array_count];
    }
    array_num = 0;
    dsxfr_put_multi (arrayfile, multi_desc);
    ds_dealloc_multi (multi_desc);
    return (p);
}   /*  End Function dump_arrays  */

static char *flush_data (p)
/*  This routine will flush the accumulated arrays.
    The remainder of  p  is returned.
*/
char *p;
{
    unsigned int array_count;
    extern unsigned int array_num;
    extern char *top_packet[MAX_ARRAYS];
    extern packet_desc *top_pack_desc[MAX_ARRAYS];
    /*static char function_name[] = "flush_data";*/

    if (array_num < 1)
    {
	(void) fprintf (stderr, "No arrays accumulated yet\n");
	return (NULL);
    }
    for (array_count = 0; array_count < array_num; ++array_count)
    {
	ds_dealloc_packet (top_pack_desc[array_count],top_packet[array_count]);
    }
    array_num = 0;
    return (p);
}   /*  End Function flush_data  */

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
    static char buffer[STRING_LENGTH];
    extern char *sys_errlist[];
    /*static char function_name[] = "command_read_func";*/

    if (ch_gets (conn_get_channel (connection), buffer, STRING_LENGTH) != TRUE)
    {
	(void) fprintf (stderr, "Error reading string\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return ( internal_decode_func (buffer, stderr) );
}   /*  End Function command_read_func  */

static flag internal_decode_func (line, fp)
char *line;
FILE *fp;
{
    return ( panel_process_command_with_stack (line, ( flag (*) () ) NULL,
					       fp) );
}

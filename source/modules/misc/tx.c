/*  tx.c

    Source file for  tx  (data structure transmitter module).

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

/*  This Karma module will read in a Karma data file and transmit the
    multi-array over one or more Karma connections to a Karma server module.
    The module will also allow a list of files to be transmitted, one after the
    other.


    Written by      Richard Gooch   4-OCT-1992

    Updated by      Richard Gooch   19-DEC-1992

    Updated by      Richard Gooch   1-JAN-1993: Took account of change to
  ch_gets  .

    Updated by      Richard Gooch   27-MAR-1993: Took account of change to
  dsxfr_get_multi  .

    Updated by      Richard Gooch   17-MAY-1993: Took account of change to
  dsxfr_get_multi  .

    Updated by      Richard Gooch   24-JUL-1993: Changed to  ch_open_file  .

    Updated by      Richard Gooch   8-AUG-1993: Tidied up by deallocating
  multi_array data structure when finished.

    Updated by      Richard Gooch   5-OCT-1993: Changed over to  panel_
  package for command line user interface.

    Updated by      Richard Gooch   6-OCT-1993: Moved  main  into this file.

    Updated by      Richard Gooch   23-NOV-1993: Changed to use of
  ex_word_skip  .

    Updated by      Richard Gooch   10-MAR-1995: Added support for "spray"
  protocol.

    Updated by      Richard Gooch   14-JUN-1995: Made use of <ex_uint>.

    Updated by      Richard Gooch   9-AUG-1995: Added support for "hog_request"
  protocol.

    Updated by      Richard Gooch   29-JAN-1996: Added call to
  <im_register_lib_version>.

    Last updated by Richard Gooch   1-JUN-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <sys/time.h>
#include <sys/times.h>
#include <sys/file.h>
#include <errno.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_conn.h>
#include <karma_ex.h>
#include <karma_ds.h>
#include <karma_im.h>
#include <karma_ch.h>
#include <karma_a.h>
#include <karma_s.h>
#include <karma_m.h>


#define BUF_SIZE 16384

EXTERN_FUNCTION (flag tx, (char *command, FILE *fp) );

flag transmit_arrayfile ();
void run_animate_file ();
STATIC_FUNCTION (void write_spray, (char *p) );

static flag animate_flag = FALSE;
static int num_iterations = 1;

#define VERSION "1.1"

int main (int argc, char **argv)
{
    KControlPanel panel;
    static char function_name[] = "main";

    conn_register_client_protocol ("spray", 0, 0,
				   ( flag (*) () ) NULL,
				   ( flag (*) () ) NULL, ( flag (*) () ) NULL,
				   ( void (*) () ) NULL);
    conn_register_client_protocol ("hog_request", 0, 0,
				   ( flag (*) () ) NULL,
				   ( flag (*) () ) NULL, ( flag (*) () ) NULL,
				   ( void (*) () ) NULL);
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "spray", "spray specified number of bytes",
		    PIT_FUNCTION, (void *) write_spray,
		    PIA_END);
    panel_add_item (panel, "num_iterations", "number", K_INT, &num_iterations,
		    PIA_END);
    panel_add_item (panel, "animate", "flag", PIT_FLAG, &animate_flag,
		    PIA_END);
    panel_push_onto_stack (panel);
    im_register_lib_version (KARMA_VERSION);
    module_run (argc, argv, "tx", VERSION, tx, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

flag tx (p, fp)
char *p;
FILE *fp;
{
    char filename[STRING_LENGTH+1];
    extern int animate_flag;

    for ( ; p != NULL; p = ex_word_skip (p) )
    {
	(void) sscanf (p, "%s", filename);
	if (animate_flag == TRUE)
	{
	    run_animate_file (filename);
	}
	else
	{
	    (void) transmit_arrayfile (filename);
	}
    }
    return (TRUE);
}   /*  End Function tx  */

flag transmit_arrayfile (arrayfile)
/*  This routine will transmit an arrayfile over any Karma "multi_array"
    connections.
    The name of the arrayfile must be pointed to by  arrayfile  .
    The routine returns TRUE on success, else it returns FALSE.
*/
char *arrayfile;
{
    multi_array *multi_desc;
    static char function_name[] = "operate_file";

    if (arrayfile == NULL)
    {
	(void) fprintf (stderr, "NULL array pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Load input array  */
    if ( ( multi_desc = dsxfr_get_multi (arrayfile, TRUE, K_CH_MAP_LOCAL,
					 FALSE) )
	== NULL )
    {
	return (FALSE);
    }
    /*  Transmit multi-array over connection  */
    if (dsxfr_put_multi ("connections", multi_desc) != TRUE)
    {
	(void) fprintf (stderr, "Error transmitting data over connection\n");
	/*  Decrement attachment count  */
	ds_dealloc_multi (multi_desc);
	return (FALSE);
    }
    /*  Decrement attachment count  */
    ds_dealloc_multi (multi_desc);
    return (TRUE);
}   /*  End Function transmit_arrayfile  */

void run_animate_file (animate_file)
/*  This routine will read and transmit a number of arrayfiles over any Karma
    "multi_array" connections.
    The names of the arrayfiles to transmit must be contained in the file
    with name pointed to by  animate_file  .
    The routine returns nothing.
*/
char *animate_file;
{
    Channel channel;
    int iter_count;
    char line[STRING_LENGTH + 1];
    extern int num_iterations;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    for (iter_count = 0; (iter_count < num_iterations) || (num_iterations < 0);
	 ++iter_count)
    {
	/*  Load animate file  */
	if ( ( channel = ch_open_file (animate_file, "r") ) == NULL )
	{
	    (void) fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
			    animate_file, sys_errlist[errno]);
	    return;
	}
	/*  Read lines  */
	while ( ch_gets (channel, line, STRING_LENGTH) )
	{
	    /*  Not end of file  */
	    if ( (line[0] == '#') || (line[0] == '\n') )
	    {
		/*  Comment or blank line: skip  */
		continue;
	    }
	    if (transmit_arrayfile (line) != TRUE)
	    {
		return;
	    }
	    if (s_check_for_int () == TRUE)
	    {
		(void) fprintf (stderr,
				"Control C: quitting animate file: \"%s\" after: %d iterations\n",
				animate_file, iter_count);
		(void) ch_close (channel);
		return;
	    }
	}
	(void) ch_close (channel);
    }
}   /*  End Function run_animate_file  */

static void write_spray (char *p)
/*  [PURPOSE] This routine will spray bytes to all "spray" servers.
    <p> A string containing the number of bytes to spray.
    [RETURNS] Nothing.
*/
{
    Connection connection;
    Channel channel;
    flag wrote_all;
    unsigned int num_bytes;
    unsigned int byte_count, block_size, conn_count, num_conn;
    float time_taken, usec, rate;
    struct timeval start_time;
    struct timeval stop_time;
    char *name;
    char buffer[BUF_SIZE];
    extern char *sys_errlist[];
    static struct timezone tz = {0, 0};
    static char function_name[] = "write_spray";

    if (p == NULL)
    {
	(void) fprintf (stderr, "Must specify number of bytes to spray\n");
	return;
    }
    if ( ( num_bytes = ex_uint (p, &p) ) < 1 )
    {
	(void) (void) fprintf (stderr, "Must specify at least 1 byte\n");
	return;
    }
    if ( ( num_conn = conn_get_num_client_connections ("spray") ) < 1 )
    {
	(void) fprintf (stderr, "No connections to spray servers\n");
	return;
    }
    (void) fprintf (stderr, "Spraying: %d bytes to all spray servers...\n",
		    num_bytes);
    m_clear (buffer, BUF_SIZE);
    for (conn_count = 0; conn_count < num_conn; ++conn_count)
    {
	if ( ( connection = conn_get_client_connection ("spray", conn_count) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Connection: %u does not exist!\n",
			    conn_count);
	    a_prog_bug (function_name);
	}
	if ( ( name = conn_get_connection_module_name (connection) ) == NULL )
	{
	    (void) fprintf (stderr, "NULL module name for connection: %u\n",
			    conn_count);
	    a_prog_bug (function_name);
	}
	channel = conn_get_channel (connection);
	(void) fprintf (stderr,
			"Spraying connection: %u (module: \"%s\")...   ",
			conn_count, name);
	if (gettimeofday (&start_time, &tz) != 0)
	{
	    (void) fprintf (stderr, "Error getting time of day\t%s%c\n",
			    sys_errlist[errno], BEL);
	    (void) exit (RV_SYS_ERROR);
	}
	byte_count = num_bytes;
	wrote_all = TRUE;
	while (byte_count > 0)
	{
	    block_size = (byte_count > BUF_SIZE) ? BUF_SIZE : byte_count;
	    if (ch_write (channel, buffer, block_size) < block_size)
	    {
		(void) fprintf (stderr, "error writing\n");
		byte_count = 0;
		wrote_all = FALSE;
		continue;
	    }
	    byte_count -= block_size;
	}
	if ( !ch_flush (channel) )
	{
	    (void) fprintf (stderr, "error flushing\n");
	    wrote_all = FALSE;
	}
	if (wrote_all)
	{
	    if (gettimeofday (&stop_time, &tz) != 0)
	    {
		(void) fprintf (stderr, "Error getting time of day\t%s%c\n",
				sys_errlist[errno], BEL);
		(void) exit (RV_SYS_ERROR);
	    }
	    usec = (float) (stop_time.tv_usec - start_time.tv_usec) * 1e-6;
	    time_taken = (float) (stop_time.tv_sec - start_time.tv_sec) + usec;
	    rate = (float) num_bytes / time_taken;
	    (void) fprintf (stderr, "%u bytes/second\n", (unsigned int) rate);
	}
    }
}   /*  End Function write_spray  */

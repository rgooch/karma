/*  tx.c

    Source file for  tx  (data structure transmitter module).

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

    Last updated by Richard Gooch   23-NOV-1993: Changed to use of
  ex_word_skip  .


*/
#include <stdio.h>
#include <math.h>
#include <sys/file.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_ex.h>
#include <karma_ch.h>
#include <karma_a.h>

EXTERN_FUNCTION (flag tx, (char *command, FILE *fp) );

flag transmit_arrayfile ();
void run_animate_file ();

static flag animate_flag = FALSE;
static int num_iterations = 1;

#define VERSION "1.1"

main (argc, argv)
int argc;       /*  Count of parameters on command line */
char **argv;    /*  List of command line parameters     */
{
    KControlPanel panel;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "num_iterations", "number", K_INT, &num_iterations,
		    PIA_END);
    panel_add_item (panel, "animate", "flag", PIT_FLAG, &animate_flag,
		    PIA_END);
    panel_push_onto_stack (panel);
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

/*  kprinthead.c

    Source file for  kprinthead  (print header in Karma format file).

    Copyright (C) 1996  Richard Gooch

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

/*  This Karma module will print the header information contained in a Karma
  format file.


    Written by      Richard Gooch   29-MAY-1996

    Last updated by Richard Gooch   15-JUN-1996: Print array descriptor too.


*/
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_dmp.h>
#include <karma_ex.h>
#include <karma_m.h>
#include <karma_n.h>

#define VERSION "1.0"

STATIC_FUNCTION (flag kprinthead, (char *command, FILE *fp) );
STATIC_FUNCTION (void process_file, (char *arrayfile) );


/*  Private data  */


int main (int argc, char **argv)
{
    KControlPanel panel;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_push_onto_stack (panel);
    module_run (argc, argv, "kprinthead", VERSION, kprinthead, -1, -1, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag kprinthead (char *p, FILE *fp)
{
    char *arrayfile;

    for ( ; p; p = ex_command_skip (p) )
    {
	if ( ( arrayfile = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp, "Error extracting arrayfile name\n");
	    return (TRUE);
	}
	process_file (arrayfile);
	m_free (arrayfile);
    }
    return (TRUE);
}   /*  End Function kprinthead  */

static void process_file (char *arrayfile)
/*  [SUMMARY] Process an arrayfile.
    <arrayfile> The name of the arrayfile.
    [RETURNS] Nothing.
*/
{
    unsigned int ftype;
    unsigned int count, type;
    multi_array *multi_desc;
    packet_desc *top_pack_desc;
    char *top_packet;
    /*static char function_name[] = "process_file";*/

    /*  Read file in whatever format  */
    if ( ( multi_desc = foreign_guess_and_read (arrayfile, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading\n");
	return;
    }
    /* Assume all the interesting information is in the first data structure */
    top_pack_desc = multi_desc->headers[0];
    top_packet = multi_desc->data[0];
    for (count = 0; count < top_pack_desc->num_elements; ++count)
    {
	type = top_pack_desc->element_types[count];
	if (type == K_ARRAY)
	{
	    dmp_array_desc (stdout,
			    (array_desc *) top_pack_desc->element_desc[count],
			    FALSE);
	}
	else if (type == LISTP)
	{
	    (void) puts ("Linked List header");
	}
	else if ( ds_element_is_named (type) )
	{
	    (void) printf ("Element: \"%s\"\t\t",
			   top_pack_desc->element_desc[count]);
	    dmp_element (stdout, type, top_pack_desc->element_desc[count],
			 top_packet +
			 ds_get_element_offset (top_pack_desc, count),
			 FALSE);
	}
	else
	{
	    (void) printf ("Unknown element type: %u\n", type);
	}
    }
    ds_dealloc_multi (multi_desc);
}   /*  End Function process_file  */

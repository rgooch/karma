/*  kgen.c

    Source file for  kgen  (data structure generation module).

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

/*  This Karma module will generate a Karma data file, using the general data
    structure format.


    Written by      Richard Gooch   3-OCT-1992

    Updated by      Richard Gooch   13-FEB-1993

    Updated by      Richard Gooch   28-MAR-1993: Took account of changes to
  list_header  structure and replaced reading routines with  dsra_  package.

    Updated by      Richard Gooch   26-APR-1993: Fixed bug in call to
  dsra_multi_data  .

    Updated by      Richard Gooch   24-JUL-1993: Changed to  ch_open_file  .

    Updated by      Richard Gooch   13-OCT-1993: Changed over to  panel_
  package for command line user interface and moved  main  into this file.

    Updated by      Richard Gooch   23-NOV-1993: Changed to use of
  ex_word_skip  .

    Last updated by Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <unistd.h>
#include <math.h>
#include <sys/file.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_dsrw.h>
#include <karma_dsra.h>
#include <karma_ds.h>
#include <karma_ch.h>
#include <karma_ex.h>
#include <karma_m.h>

#define VERSION "1.1"

EXTERN_FUNCTION (flag kgen, (char *command, FILE *fp) );

void generate_file ();


#define DATA_FORMAT_ASCII 0
#define DATA_FORMAT_BINARY 1
#define NUM_DATA_FORMATS 2

static char *data_format_alternatives[] =
{
    "ascii",
    "binary"
};
static int data_format = DATA_FORMAT_ASCII;

int main (int argc, char **argv)
{
    KControlPanel panel;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "data_format", "choice", PIT_CHOICE_INDEX,
		    &data_format,
		    PIA_NUM_CHOICE_STRINGS, NUM_DATA_FORMATS,
		    PIA_CHOICE_STRINGS, data_format_alternatives,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "kgen", VERSION, kgen, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

flag kgen (p, fp)
char *p;
FILE *fp;
{
    char filename[STRING_LENGTH+1];

    for ( ; p; p = ex_word_skip (p) )
    {
	(void) sscanf (p, "%s", filename);
        generate_file (filename);
    }
    return (TRUE);
}   /*  End Function kgen  */

void generate_file (filename)
/*  This routine will generate a Karma data file using the  filename  specified
    as a basis.
    The routine returns nothing.
*/
char filename[];
{
    Channel channel;
    unsigned int array_count;
    char desc_file[STRING_LENGTH];
    char dat_file[STRING_LENGTH];
    char *data;
    multi_array *multi_desc;
    extern int data_format;
    /*static char function_name[] = "generate_file";*/

    /*  Generate filenames  */
    sprintf (desc_file, "%s.desc", filename);
    sprintf (dat_file, "%s.dat", filename);
    /*  Test readability of files  */
    if ( ( channel = ch_open_file (desc_file, "r") ) == NULL )
    {
	(void) fprintf (stderr, "File: %s is not readable\n", desc_file);
        return;
    }
    if (access (dat_file, R_OK) != 0)
    {
	(void) fprintf (stderr, "File: %s is not readable\n", dat_file);
        return;
    }
    /*  Read multi array descriptor */
    if ( ( multi_desc = dsra_multi_desc (channel) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading multi_array structure\n");
	(void) ch_close (channel);
	return;
    }
    (void) fprintf (stderr, "File: %s read in\n", desc_file);
    /*  Allocate all the required data space  */
    for (array_count = 0; array_count < (*multi_desc).num_arrays;
	 ++array_count)
    {
	if ( ( data = ds_alloc_data ( (*multi_desc).headers[array_count],
				     FALSE, TRUE ) ) ==
            NULL )
        {
	    (void) fprintf (stderr, "Error allocating memory for array number %u\n",
			    array_count);
            ds_dealloc_multi (multi_desc);
            return;
        }
        (*multi_desc).data[array_count] = data;
    }
    (void) fprintf (stderr, "Allocated data space\n");
    /*  Open data file  */
    if ( ( channel = ch_open_file (dat_file, "r") ) == NULL )
    {
	(void) fprintf (stderr, "File: %s is not readable\n", dat_file);
	return;
    }
    /*  Load the data  */
    if (data_format == DATA_FORMAT_ASCII)
    {
	if (dsra_multi_data (channel, multi_desc) != TRUE)
	{
	    (void) fprintf (stderr, "Error reading array\n");
	    (void) ch_close (channel);
	    return;
	}
    }
    else
    {
	for (array_count = 0; array_count < (*multi_desc).num_arrays;
	     ++array_count)
	{
	    /*  Load the data for one array */
	    if (dsrw_read_packet (channel, (*multi_desc).headers[array_count],
				  (*multi_desc).data[array_count]) == FALSE)
	    {
		(void) fprintf (stderr, "Error reading array number %u\n",
				array_count);
		(void) ch_close (channel);
		return;
	    }
	}
    }
    (void) fprintf (stderr, "File: %s read in\n", dat_file);
    if (dsxfr_put_multi (filename, multi_desc) != TRUE)
    {
	(void) fprintf (stderr, "Error transmitting data structure\n");
    }
    /*  Close channel afterwards in case data was memory mapped  */
    (void) ch_close (channel);
    ds_dealloc_multi (multi_desc);
}   /*  End Function generate_file  */

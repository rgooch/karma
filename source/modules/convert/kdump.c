/*  kdump.c

    Source file for  kdump  (data structure dumping module).

    Copyright (C) 1992-1996  Richard Gooch

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

/*  This Karma module will generate from a Karma data file a descriptor file
    and a data file.
    This reverses the process used in  kgen  .


    Written by      Richard Gooch   5-NOV-1992

    Updated by      Richard Gooch   12-FEB-1993

    Updated by      Richard Gooch   27-MAR-1993: Took account of change to
  dsxfr_get_multi  .

    Updated by      Richard Gooch   16-MAY-1993: Took account of replacement of
  dsrw_write_multi_desc  and  dsrw_write_multi_data  with  dsrw_write_multi  .

    Updated by      Richard Gooch   17-MAY-1993: Took account of change to
  dsxfr_get_multi  .

    Updated by      Richard Gooch   24-JUL-1993: Changed to  ch_open_file  .

    Updated by      Richard Gooch   8-AUG-1993: Removed memory leak due to
  multi_array data structures not being deallocated.

    Updated by      Richard Gooch   13-OCT-1993: Changed over to  panel_
  package for command line user interface and moved  main  into this file.

    Updated by      Richard Gooch   23-NOV-1993: Changed to use of
  ex_word_skip  .

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Updated by      Richard Gooch   7-AUG-1994: Added  descriptor_only
  parameter.

    Last updated by Richard Gooch   1-JUN-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_dsxfr.h>
#include <karma_panel.h>
#include <karma_dsrw.h>
#include <karma_dmp.h>
#include <karma_ds.h>
#include <karma_ch.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_m.h>


#define VERSION "1.2"

EXTERN_FUNCTION (flag kdump, (char *command, FILE *fp) );

void extract_files ();

static flag show_comments = FALSE;
static flag descriptor_only = FALSE;

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
    panel_add_item (panel, "descriptor_only", "flag", PIT_FLAG,
		    &descriptor_only,
		    PIA_END);
    panel_add_item (panel, "comments", "flag", PIT_FLAG, &show_comments,
		    PIA_END);
    panel_add_item (panel, "data_format", "choice", PIT_CHOICE_INDEX,
		    &data_format,
		    PIA_NUM_CHOICE_STRINGS, NUM_DATA_FORMATS,
		    PIA_CHOICE_STRINGS, data_format_alternatives,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "kdump", VERSION, kdump, -1, -1, FALSE);
    return (RV_OK);
}   /*  End Function main   */

flag kdump (p, fp)
char *p;
FILE *fp;
{
    char arrayname[STRING_LENGTH + 1];
    extern flag show_comments;

    for ( ; p; p = ex_word_skip (p) )
    {
	(void) sscanf (p, "%s", arrayname);
        extract_files (arrayname, (flag) show_comments);
    }
    return (TRUE);
}   /*  End Function kdump  */

void extract_files (arrayname, comments)
/*  This routine will dump a Karma data file to a pair of files: one of type
    .desc  and the other  .dat  .
    These files will contain representations of the data structure
    descriptors and the data itself, respectively.
    If  comments  is TRUE, the output files will have comments in various
    key points.
    The routine returns nothing.
*/
char arrayname[];
flag comments;
{
    Channel channel;
    unsigned int array_count;
    FILE *fp;
    char desc_file[STRING_LENGTH];
    char dat_file[STRING_LENGTH];
    char *source_ptr;
    char *dest_ptr;
    multi_array *multi_desc;
    extern int data_format;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "extract_files";

    if ( ( multi_desc = dsxfr_get_multi (arrayname, FALSE, K_CH_MAP_LOCAL,
					 FALSE) )
	== NULL )
    {
	(void) fprintf (stderr, "Array: %s not found\n", arrayname);
        return;
    }
    /*  Create  .desc  filename */
    source_ptr = arrayname;
    dest_ptr = desc_file;
    while ( (*source_ptr != '\0') && (*source_ptr != '[') )
    {
	*dest_ptr++ = *source_ptr++;
    }
    *dest_ptr = '\0';
    (void) strcat (dest_ptr, ".desc");
    /*  Create  .dat  filename  */
    source_ptr = arrayname;
    dest_ptr = dat_file;
    while ( (*source_ptr != '\0') && (*source_ptr != '[') )
    {
	*dest_ptr++ = *source_ptr++;
    }
    *dest_ptr = '\0';
    (void) strcat (dest_ptr, ".dat");
    /*  Open  .desc  file   */
    if ( ( fp = fopen (desc_file, "w") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening file: %s for output\t%s\n",
			desc_file, sys_errlist[errno]);
	ds_dealloc_multi (multi_desc);
        return;
    }
    /*  Dump data structure descriptor  */
    dmp_multi_desc (fp, multi_desc, comments);
    (void) fclose (fp);
    (void) fprintf (stderr, "Written file: %s\n", desc_file);
    if (descriptor_only)
    {
	(void) fprintf (stderr, "descriptor_only  flag set: no data dumped\n");
	ds_dealloc_multi (multi_desc);
	return;
    }
    /*  Dump data structure */
    /*  Open  .dat  file    */
    switch (data_format)
    {
      case DATA_FORMAT_ASCII:
	if ( (fp = fopen (dat_file, "w") ) == NULL )
	{
	    (void) fprintf (stderr, "Error opening file: %s for output\t%s\n",
			    dat_file, sys_errlist[errno]);
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	dmp_multi_data (fp, multi_desc, comments);
	(void) fclose (fp);
	break;
      case DATA_FORMAT_BINARY:
	if ( (channel = ch_open_file (dat_file, "w") ) == NULL )
	{
	    (void) fprintf (stderr, "Error opening file: %s for output\t%s\n",
			    dat_file, sys_errlist[errno]);
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	for (array_count = 0; array_count < (*multi_desc).num_arrays;
	     ++array_count)
	{
	    dsrw_write_packet (channel, (*multi_desc).headers[array_count],
			       (*multi_desc).data[array_count]);
	}
	(void) ch_close (channel);
	break;
      default:
	(void) fprintf (stderr, "Bad value of format: %u\n", data_format);
	a_prog_bug (function_name);
	break;
    }
    (void) fprintf (stderr, "Written file: %s\n", dat_file);
    ds_dealloc_multi (multi_desc);
}   /*  End Function extract_files  */

/*  merge_planes.c

    Source file for  merge_planes  (image plane merge module).

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

/*  This Karma module will merge several planes into one 2 dimensional array.


    Written by      Richard Gooch   19-SEP-1993

    Updated by      Richard Gooch   19-SEP-1993

    Updated by      Richard Gooch   5-OCT-1993: Changed over to  panel_
  package for command line user interface.

    Updated by      Richard Gooch   6-OCT-1993: Moved  main  into this file.

    Last updated by Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_iarray.h>
#include <karma_dsxfr.h>
#include <karma_panel.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>


#define MAX_FILES 100

EXTERN_FUNCTION (flag merge_planes, (char *command, FILE *fp) );

#define VERSION "1.1"


static void process_files ();


int main (int argc, char **argv)
{
    KControlPanel panel;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "parameter form");
    }
    panel_push_onto_stack (panel);
    module_run (argc, argv, "merge_planes", VERSION, merge_planes, -1, 0,
		FALSE);
    return (RV_OK);
}   /*  End Function main   */

flag merge_planes (p, fp)
char *p;
FILE *fp;
{
    int num_files;
    char *str1, *str2;
    char *filenames[MAX_FILES];
    char *elem_names[MAX_FILES];

    num_files = 0;
    while ( ( str1 = ex_str (p, &p) ) != NULL )
    {
	if ( ( str2 = ex_str (p, &p) ) == NULL )
	{
	    /*  Only one name: output name  */
	    if (num_files < 1)
	    {
		(void) fprintf (stderr, "No input filenames specified\n");
		return (TRUE);
	    }
	    /*  Process files  */
	    process_files ( (unsigned int) num_files, filenames, elem_names,
			   str1 );
	    while (--num_files >= 0)
	    {
		m_free (filenames[num_files]);
		m_free (elem_names[num_files]);
	    }
	    m_free (str1);
	    return (TRUE);
	}
	/*  Filename/ element name pair  */
	if (num_files > MAX_FILES)
	{
	    (void) fprintf (stderr, "Too many files\n");
	    return (TRUE);
	}
	filenames[num_files] = str1;
	elem_names[num_files] = str2;
	++num_files;
    }
    if (num_files < 1)
    {
	(void) fprintf (stderr, "No input filenames specified\n");
	return (TRUE);
    }
    (void) fprintf (stderr, "No output filename specified\n");
    while (--num_files >= 0)
    {
	m_free (filenames[num_files]);
	m_free (elem_names[num_files]);
    }
    return (TRUE);
}   /*  End Function merge_planes  */

static void process_files (num_files, filenames, elem_names, out_filename)
/*  This routine will process a number of input arrayfiles.
    The number of input arrayfiles must be given by  num_files  .
    The names of the arrayfiles must be pointed to by  filenames  .
    The names of the elements of must be pointed to by  elem_names  .
    The output filename must be pointed to by  out_filename  .
    The routine returns nothing.
*/
unsigned int num_files;
char **filenames;
CONST char **elem_names;
char *out_filename;
{
    iarray out;
    int count;
    char *out_array;
    multi_array *multi_desc;
    iarray planes[MAX_FILES];
    unsigned int types[MAX_FILES];
    unsigned long lengths[2];
    static char function_name[] = "process_files";

    for (count = 0; count < num_files; ++count)
    {
	if ( ( planes[count] = iarray_read_nD (filenames[count], TRUE, NULL,
					       2, NULL, NULL,
					       K_CH_MAP_IF_AVAILABLE) )
	    == NULL )
	{
	    while (--count >= 0)
	    {
		iarray_dealloc (planes[count]);
	    }
	    return;
	}
	types[count] = iarray_type (planes[count]);
	if (count == 0)
	{
	    lengths[0] = iarray_dim_length (planes[0], 0);
	    lengths[1] = iarray_dim_length (planes[0], 1);
	}
	else
	{
	    if (iarray_dim_length (planes[count], 0) != lengths[0])
	    {
		(void) fprintf (stderr,
				"File: \"%s\" ylen: %lu differs from first: %lu\n",
				filenames[count],
				iarray_dim_length (planes[count], 0),
				lengths[0]);
		{
		    while (--count >= 0)
		    {
			iarray_dealloc (planes[count]);
		    }
		    return;
		}
	    }
	    if (iarray_dim_length (planes[count], 1) != lengths[1])
	    {
		(void) fprintf (stderr,
				"File: \"%s\" xlen: %lu differs from first: %lu\n",
				filenames[count],
				iarray_dim_length (planes[count], 1),
				lengths[1]);
		{
		    while (--count >= 0)
		    {
			iarray_dealloc (planes[count]);
		    }
		    return;
		}
	    }
	}
    }
    /*  Have all planes: create new data structure  */
    if ( ( out_array = ds_easy_alloc_n_element_array (&multi_desc, 2, lengths,
						      (double *) NULL,
						      (double *) NULL,
						      NULL,
						      num_files, types,
						      elem_names) )
	== NULL )
    {
	m_error_notify (function_name, "output data structure");
	for (count = 0; count < num_files; ++count)
	{
	    iarray_dealloc (planes[count]);
	}
	return;
    }
    /*  Copy data  */
    for (count = 0; count < num_files; ++count)
    {
	if ( ( out = iarray_get_from_multi_array (multi_desc, NULL,
						  2, NULL,
						  elem_names[count]) )
	    == NULL )
	{
	    m_error_notify (function_name, "alias plane");
	    for (count = 0; count < num_files; ++count)
	    {
		iarray_dealloc (planes[count]);
	    }
	    return;
	}
	if ( !iarray_copy_data (out, planes[count], FALSE) )
	{
	    (void) fprintf (stderr, "Error copying data\n");
	    for (count = 0; count < num_files; ++count)
	    {
		iarray_dealloc (planes[count]);
	    }
	    return;
	}
	iarray_dealloc (out);
    }
    for (count = 0; count < num_files; ++count)
    {
	iarray_dealloc (planes[count]);
    }
    dsxfr_put_multi (out_filename, multi_desc);
    ds_dealloc_multi (multi_desc);
}   /*  End Function process_files  */

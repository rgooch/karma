/*  raw_grey2karma.c

    Source file for  raw_grey2karma  (convert raw data to Karma format).

    Copyright (C) 1993,1994  Richard Gooch

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

/*  This Karma module will generate a multi-dimensional array of data from a
    raw data file containing greyscale (or intensity) values and will write the
    data to a Karma data file.
    The raw data file must be in host natural format.


    Written by      Richard Gooch   1-NOV-1992

    Updated by      Richard Gooch   4-DEC-1992

    Updated by      Richard Gooch   13-OCT-1993: Moved  main  into this file.

    Last updated by Richard Gooch   22-MAY-1994: Changed over to  panel_
  package.


*/
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_m.h>

#define VERSION "1.2"

#define MAX_DIMENSIONS (unsigned int) 100

STATIC_FUNCTION (flag raw_grey2karma, (char *command, FILE *fp) );

void generate_file ();
flag convert_image ();


/*  Private data  */
static unsigned int num_dimensions = 0;
static char *names[MAX_DIMENSIONS];
static unsigned long lengths[MAX_DIMENSIONS];
static double minima[MAX_DIMENSIONS];
static double maxima[MAX_DIMENSIONS];
static unsigned int elem_type = K_FLOAT;
static char *elem_name = "Intensity";
static flag ignore_excess = FALSE;
static flag invert_flag = TRUE;


main (argc, argv)
int argc;
char **argv;
{
    KControlPanel panel, group;
    extern char *data_type_names[];
    static char function_name[] = "main";

    if ( ( group = panel_create_group () ) == NULL )
    {
	m_abort (function_name, "group panel");
    }
    panel_add_item (group, "maximum", "", K_DOUBLE, (char *) maxima,
		    PIA_END);
    panel_add_item (group, "minimum", "", K_DOUBLE, (char *) minima,
		    PIA_END);
    panel_add_item (group, "length", "", K_ULONG, (char *) lengths,
		    PIA_END);
    panel_add_item (group, "name", "", K_VSTRING, (char *) names,
		    PIA_END);
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "dimensions", "", PIT_GROUP, (char *) group,
		    PIA_ARRAY_MAX_LENGTH, MAX_DIMENSIONS,
		    PIA_ARRAY_LENGTH, (char *) &num_dimensions,
		    PIA_END);
    panel_add_item (panel, "elem_name", "", K_VSTRING, (char *) &elem_name,
		    PIA_END);
    panel_add_item (panel, "elem_type", "", PIT_CHOICE_INDEX,
		    (char *) &elem_type,
		    PIA_NUM_CHOICE_STRINGS, NUMTYPES,
		    PIA_CHOICE_STRINGS, data_type_names,
		    PIA_END);
    panel_add_item (panel, "ignore_excess", "flag", PIT_FLAG, &ignore_excess,
		    PIA_END);
    panel_add_item (panel, "invert_image", "flag", PIT_FLAG, &invert_flag,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "raw_grey2karma", VERSION, raw_grey2karma, -1, 0,
		FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag raw_grey2karma (p, fp)
char *p;
FILE *fp;
{
    char *arrayfile;
    char *input_filename;
    extern unsigned int elem_type;
    extern unsigned int num_dimensions;

    if (num_dimensions < 1)
    {
	(void) fprintf (fp,
			"Must have at least one dimension to create\n");
	return (TRUE);
    }
    if (ds_element_is_atomic ( (unsigned int) elem_type ) == FALSE)
    {
        (void) fprintf (fp,
			"elem_type must be atomic: defaulting to K_FLOAT\n");
        elem_type = K_FLOAT;
        return (TRUE);
    }
    for ( ; p; p = ex_command_skip (p) )
    {
	if ( ( input_filename = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp, "Error extracting input filename\n");
	    return (TRUE);
	}
	if ( ( arrayfile = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp, "Error extracting arrayfile name\n");
	    m_free (input_filename);
	    return (TRUE);
	}
	generate_file (input_filename, arrayfile, elem_type);
	m_free (input_filename);
	m_free (arrayfile);
    }
    return (TRUE);
}   /*  End Function raw_grey2karma  */

void generate_file (inp_filename, arrayname, elem_type)
/*  This routine will generate a Karma arrayfile of the new format with a
    multi-dimensional array of a single atomic element. The data will be
    converted from the input file.
    The name of the input file must be pointed to by  inp_filename  .
    the name of the Karma arrayfile must be pointed to by  arrayname  .
    The type of the data elements must be in  elem_type  .
    The routine returns nothing.
*/
char *inp_filename;
char *arrayname;
unsigned int elem_type;
{
    FILE *inp_fp;
    unsigned int array_size;
    unsigned int block_size;
    unsigned int block_count;
    unsigned int dim_count;
    struct stat stat_buf;
    char *array;
    multi_array *multi_desc;
    extern flag ignore_excess;
    extern flag invert_flag;
    extern unsigned int num_dimensions;
    extern unsigned long lengths[MAX_DIMENSIONS];
    extern double minima[MAX_DIMENSIONS];
    extern double maxima[MAX_DIMENSIONS];
    extern char *names[MAX_DIMENSIONS];
    extern char *elem_name;
    extern char host_type_sizes[NUMTYPES];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "generate_file";

    /*  Determine array size (in co-ordinates)  */
    array_size = 1;
    for (dim_count = 0; dim_count < num_dimensions; ++dim_count)
    {
	if (lengths[dim_count] < 1)
	{
	    (void) fprintf (stderr,
			    "\nDimension: %u has length: %d: must be greater than 0",
			    dim_count, lengths[dim_count]);
	    return;
	}
	array_size *= (unsigned int) lengths[dim_count];
    }
    /*  Get stats on file  */
    if (stat (inp_filename, &stat_buf) != 0)
    {
	(void) fprintf (stderr, "\nError getting stats on file: \"%s\"\t%s",
			inp_filename, sys_errlist[errno]);
	return;
    }
    /*  Compare with specified array size  */
    if (stat_buf.st_size < array_size *
	(unsigned) host_type_sizes[elem_type])
    {
	(void) fprintf (stderr,
			"\nFile: \"%s\" is: %u bytes which is smaller than specified: %u bytes",
			inp_filename, stat_buf.st_size,
			array_size * 
			(unsigned) host_type_sizes[elem_type]);
	return;
    }
    if (stat_buf.st_size >
	array_size * (unsigned) host_type_sizes[elem_type])
    {
	if (ignore_excess == TRUE)
	{
	    (void) fprintf (stderr, "\nIgnored: %u excess bytes",
			    stat_buf.st_size - array_size *
			    (unsigned) host_type_sizes[elem_type]);
	}
	else
	{
	    (void) fprintf (stderr,
			    "\nFile: \"%s\" is: %u bytes which is larger than specified: %u bytes",
			    inp_filename, stat_buf.st_size,
			    array_size *
			    (unsigned) host_type_sizes[elem_type]);
	    return;
	}
    }
    /*  Try to open input file  */
    if ( ( inp_fp = fopen (inp_filename, "r") ) == NULL )
    {
	(void) fprintf (stderr, "\nError opening file: \"%s\"\t%s",
			inp_filename, sys_errlist[errno]);
	return;
    }
    if ( ( array = ds_easy_alloc_array (&multi_desc,
					(unsigned int) num_dimensions,
					lengths, minima, maxima, names,
					elem_type, elem_name) )
	== NULL )
    {
	(void) fprintf (stderr,
			"\nNULL return value from function: easy_alloc_array");
	(void) fclose (inp_fp);
	return;
    }
    /*  Determine if inversion of image is needed  */
    if (invert_flag == TRUE)
    {
	/*  Must invert image  */
	block_size = array_size / (unsigned int) lengths[0];
	for (block_count = 0; block_count < lengths[0]; ++block_count)
	{
	    errno = 0;
	    if (fread (array + host_type_sizes[elem_type] *
		       block_size * (lengths[0] - block_count - 1),
		       host_type_sizes[elem_type], block_size,
		       inp_fp)
		< block_size)
	    {
		(void) fprintf (stderr, "\nError reading file: \"%s\"\t%s",
				inp_filename, sys_errlist[errno]);
		ds_dealloc_multi (multi_desc);
		(void) fclose (inp_fp);
		return;
	    }
	}
    }
    else
    {
	/*  Do a block read of entire image  */
	errno = 0;
	if (fread (array, host_type_sizes[elem_type], array_size, inp_fp)
	    < array_size)
	{
	    (void) fprintf (stderr, "\nError reading file: \"%s\"\t%s",
			    inp_filename, sys_errlist[errno]);
	    ds_dealloc_multi (multi_desc);
	    (void) fclose (inp_fp);
	    return;
	}
    }
    (void) fclose (inp_fp);
    /*  Write to arrayfile  */
    if (dsxfr_put_multi (arrayname, multi_desc) != TRUE)
    {
	(void) fprintf (stderr, "Error putting array\n");
    }
    /*  Write comments to new array  */
#ifdef dummy
    ez_description (comp_array, vble);
    (void) sprintf (txt, "Read raw data file: \"%s\"", inp_filename);
    composite_descr (txt, comp_array);
#endif
    ds_dealloc_multi (multi_desc);
}   /*  End Function generate_file  */

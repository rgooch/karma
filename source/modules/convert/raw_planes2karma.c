/*  raw_planes2karma.c

    Source file for  raw_planes2karma  (convert raw data to Karma format).

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

/*  This Karma module will generate a multi-dimensional array of multi element
    data from a number of raw data files containing values and will write the
    data to a Karma data file.
    The raw data files must be in host natural format.


    Written by      Richard Gooch   1-NOV-1992

    Updated by      Richard Gooch   4-DEC-1992

    Updated by      Richard Gooch   3-JAN-1993: Took acccount of change to
  ds_alloc_data

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
#define MAX_ELEMENTS (unsigned int) 10

STATIC_FUNCTION (flag raw_planes2karma, (char *command, FILE *fp) );

void generate_file ();
flag convert_image ();
char *alloc_array ();


/*  Private data  */
static int num_dimensions = 0;
static char *names[MAX_DIMENSIONS];
static unsigned int lengths[MAX_DIMENSIONS];
static double minima[MAX_DIMENSIONS];
static double maxima[MAX_DIMENSIONS];
extern char *data_type_names[];
static unsigned int elem_type = K_FLOAT;
static char *elem_names[MAX_ELEMENTS] =
{   "Red Intensity", "Green Intensity", "Blue Intensity"   };
static unsigned int num_elements = 3;
static flag ignore_excess = FALSE;
static flag invert_flag = FALSE;


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
    panel_add_item (group, "length", "", K_UINT, (char *) lengths,
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
    panel_add_item (panel, "elem_names", "", K_VSTRING, (char *) elem_names,
		    PIA_ARRAY_MAX_LENGTH, MAX_ELEMENTS,
		    PIA_ARRAY_LENGTH, (char *) &num_elements,
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
    module_run (argc, argv, "raw_planes2karma", VERSION, raw_planes2karma,
		-1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag raw_planes2karma (p, fp)
char *p;
FILE *fp;
{
    int plane_count;
    char *arrayfile;
    char *input_filenames[MAX_ELEMENTS];
    extern unsigned int num_elements;
    extern unsigned int elem_type;
    extern int num_dimensions;

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
	for (plane_count = 0; plane_count < num_elements; ++plane_count)
	{
	    if ( ( input_filenames[plane_count] = ex_str (p, &p) ) == NULL )
	    {
		(void) fprintf (fp, "Error extracting input filename[%d]\n",
				plane_count);
		while (plane_count-- > 0)
		{
		    m_free (input_filenames[plane_count]);
		}
		return (TRUE);
	    }
	}
	if ( ( arrayfile = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp, "Error extracting arrayfile name\n");
	    for (plane_count = 0; plane_count < num_elements; ++plane_count)
	    {
		m_free (input_filenames[plane_count]);
	    }
	    return (TRUE);
	}
	generate_file (input_filenames, arrayfile, elem_type);
	for (plane_count = 0; plane_count < num_elements; ++plane_count)
	{
	    m_free (input_filenames[plane_count]);
	}
	m_free (arrayfile);
    }
    return (TRUE);
}   /*  End Function raw_planes2karma  */

void generate_file (inp_filenames, arrayname, elem_type)
/*  This routine will generate a Karma arrayfile with a multi-dimensional array
    of multiple atomic elements. The data will be converted from the input
    files.
    The names of the input files must be pointed to by  inp_filenames  .
    The name of the Karma arrayfile must be pointed to by  arrayname  .
    The type of the data elements must be in  elem_type  .
    The routine returns nothing.
*/
char *inp_filenames[MAX_ELEMENTS];
char *arrayname;
unsigned int elem_type;
{
    FILE *inp_fps[MAX_ELEMENTS];
    unsigned int plane_count;
    unsigned int array_size;
    unsigned int row_size;
    unsigned int dim_count;
    unsigned int packet_count;
    unsigned int row_count;
    int row_step;
    struct stat stat_buf;
    char *array;
    char *ptr;
    multi_array *multi_desc;
    extern flag ignore_excess;
    extern flag invert_flag;
    extern int num_dimensions;
    extern unsigned int num_elements;
    extern unsigned int lengths[MAX_DIMENSIONS];
    extern double minima[MAX_DIMENSIONS];
    extern double maxima[MAX_DIMENSIONS];
    extern char *names[MAX_DIMENSIONS];
    extern char *elem_names[MAX_ELEMENTS];
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
			    "Dimension: %u has length: %d: must be greater than 0\n",
			    dim_count, lengths[dim_count]);
	    return;
	}
	array_size *= (unsigned int) lengths[dim_count];
    }
    /*  Get stats on files  */
    for (plane_count = 0; plane_count < num_elements; ++plane_count)
    {
	if (stat (inp_filenames[plane_count], &stat_buf) != 0)
	{
	    (void) fprintf (stderr,
			    "Error getting stats on file: \"%s\"\t%s\n",
			    inp_filenames[plane_count], sys_errlist[errno]);
	    while (plane_count-- > 0)
	    {
		(void) fclose (inp_fps[plane_count]);
	    }
	    return;
	}
	/*  Compare with specified array size  */
	if (stat_buf.st_size < array_size *
	    (unsigned) host_type_sizes[elem_type])
	{
	    (void) fprintf (stderr,
			    "File: \"%s\" is: %u bytes which is smaller than specified: %u bytes\n",
			    inp_filenames[plane_count], stat_buf.st_size,
			    array_size * 
			    (unsigned) host_type_sizes[elem_type]);
	    while (plane_count-- > 0)
	    {
		(void) fclose (inp_fps[plane_count]);
	    }
	    return;
	}
	if (stat_buf.st_size >
	    array_size * (unsigned) host_type_sizes[elem_type])
	{
	    if (ignore_excess == TRUE)
	    {
		(void) fprintf (stderr, "Ignored: %u excess bytes\n",
				stat_buf.st_size - array_size *
				(unsigned) host_type_sizes[elem_type]);
	    }
	    else
	    {
		(void) fprintf (stderr,
				"File: \"%s\" is: %u bytes which is larger than specified: %u bytes\n",
				inp_filenames[plane_count], stat_buf.st_size,
				array_size *
				(unsigned) host_type_sizes[elem_type]);
		while (plane_count-- > 0)
		{
		    (void) fclose (inp_fps[plane_count]);
		}
		return;
	    }
	}
	/*  Try to open input file  */
	if ( ( inp_fps[plane_count] = fopen (inp_filenames[plane_count], "r") )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
			    inp_filenames[plane_count], sys_errlist[errno]);
	    while (plane_count-- > 0)
	    {
		(void) fclose (inp_fps[plane_count]);
	    }
	    return;
	}
    }
    if ( ( array = alloc_array (&multi_desc,
				(unsigned int) num_dimensions,
				lengths, minima, maxima, names,
				elem_type,
				(unsigned int) num_elements, elem_names) )
	== NULL )
    {
	(void) fprintf (stderr,
			"NULL return value from function: easy_alloc_array\n");
	for (plane_count = 0; plane_count < num_elements; ++plane_count)
	{
	    (void) fclose (inp_fps[plane_count]);
	}
	return;
    }
    /*  Determine if inversion of image is needed  */
    row_size = array_size * (unsigned int) host_type_sizes[elem_type];
    row_size *= (unsigned int) num_elements;
    row_size /= (unsigned int) lengths[0];
    if (invert_flag)
    {
	array += row_size * (lengths[0] - 1);
	row_step = -1;
    }
    else
    {
	row_step = 1;
    }
    /*  Read entire array  */
    for (row_count = 0; row_count < lengths[0];
	 ++row_count, array += row_size * row_step)
    {
	/*  Read one "row" (highest dimension)  */
	for (packet_count = 0, ptr = array;
	     packet_count < array_size / (unsigned int) lengths[0];
	     ++packet_count)
	{
	    /*  Read one packet  */
	    for (plane_count = 0; plane_count < num_elements;
		 ++plane_count, ptr += host_type_sizes[elem_type])
	    {
		/*  Read one element  */
		if (fread (ptr, host_type_sizes[elem_type], 1,
			   inp_fps[plane_count]) < 1)
		{
		    (void) fprintf (stderr, "Error reading file: \"%s\"\t%s\n",
				    inp_filenames[plane_count],
				    sys_errlist[errno]);
		    ds_dealloc_multi (multi_desc);
		    for (plane_count = 0; plane_count <num_elements; ++plane_count)
		    {
			(void) fclose (inp_fps[plane_count]);
		    }
		    return;
		}
	    }
	}
    }
    for (plane_count = 0; plane_count < num_elements; ++plane_count)
    {
	(void) fclose (inp_fps[plane_count]);
    }
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

char *alloc_array (multi_desc, num_dim, lengths, minima, maxima, names,
		   data_type, num_elements, data_names)
/*  This routine will allocate memory for a multi-dimensional, regular array,
    and the required headers and the multi-array header.
    NOTE: this routine differs from  ds_easy_alloc_array  in that it allows a
    multiple element array to be allocated.
    Note that this routine does NOT create a Karma arrayfile. This must be done
    with a call to  dsxfr_put_multi  .
    The multi-array structure pointer to the created structure will be
    written to the storage pointed to by  multi_desc  .
    The number of dimensions of the array must be in  num_dim  ,the lengths
    of each dimension (axis) must be in the array pointed to by  lengths  .
    The minimum and maximum co-ordinate values of each dimension must be in the
    arrays pointed to by  minima  and maxima  ,respectively. If either of
    these pointers is NULL, the range of the co-ordinates will be the lengths
    of the dimensions minus 1. If one of the pointers is NULL, the other
    array is used to tie one end of the range for each dimension. If both
    pointers are NULL,the minima are 0.0 and the maxima are the lengths minus 1
    The names of each dimension must be pointed to by the list of character
    pointers  names  .If this is NULL, the names "Axis 0", "Axis 1", ...etc
    will be used. Note: the character arrays are copied, so the arrays of
    characters and the array of pointers may be subsequently deallocated.
    The data packet that may be stored in the array is a multiple of an atomic
    datum, of type  data_type  .
    The following data types are not permitted:
        NONE, K_ARRAY, LISTP, MULTI_ARRAY.
    The number of elements per packet must be given by  num_elements  .
    The names of the data types must be pointed to by  data_names  .
    The name strings are copied, thus the memory used for the input strings may
    be subsequently deallocated.
    The routine returns a pointer to the start of the array. If memory could
    not be allocated, or any error occurs, NULL is returned.
*/
multi_array **multi_desc;
unsigned int num_dim;
unsigned int *lengths;
double *minima;
double *maxima;
char **names;
unsigned int data_type;
unsigned int num_elements;
char **data_names;
{
    unsigned int dim_count = 0;
    unsigned int elem_count;
    double minimum;
    double maximum;
    char *return_value;
    char *tmp_name;
    char *tmp_pointer;
    multi_array *multi_header;
    packet_desc *pack_descriptor;
    array_desc *arr_descriptor;
    dim_desc *dimension;
    static char function_name[] = "alloc_array";

    if ( (multi_desc == NULL) || (lengths == NULL) || (data_names == NULL ) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (names == NULL)
    {
	/*  No names supplied   */
        if ( (int) log10 ( (double) num_dim ) + 1 > 122 )
        {
	    /*  Too many dimensions for sprintf to print    */
	    a_func_abort (function_name, "Too many dimensions");
            return (NULL);
        }
        if ( ( tmp_name = m_alloc ( (unsigned int) 129 ) ) == NULL )
        {
	    m_error_notify (function_name, "dimension name");
            return (NULL);
        }
    }
    switch (data_type)
    {
      case K_FLOAT:
      case K_DOUBLE:
      case K_BYTE:
      case K_INT:
      case K_SHORT:
      case K_COMPLEX:
      case K_DCOMPLEX:
      case K_BCOMPLEX:
      case K_ICOMPLEX:
      case K_SCOMPLEX:
      case K_LONG:
      case K_LCOMPLEX:
      case K_UBYTE:
      case K_UINT:
      case K_USHORT:
      case K_ULONG:
      case K_UBCOMPLEX:
      case K_UICOMPLEX:
      case K_USCOMPLEX:
      case K_ULCOMPLEX:
	/*  Data type permitted */
	break;
      default:
	/*  Bad data type   */
	(void) fprintf (stderr, "Bad data type: %u\n", data_type);
	a_prog_bug (function_name);
	break;
    }
    if ( ( multi_header = ds_alloc_multi ( (unsigned int) 1 ) ) == NULL )
    {
	m_error_notify (function_name, "multi_desc descriptor");
        return (NULL);
    }
    if ( ( pack_descriptor = ds_alloc_packet_desc ( (unsigned) 1 ) )
	== NULL )
    {
	ds_dealloc_multi (multi_header);
	m_error_notify (function_name, "packet descriptor");
        return (NULL);
    }
    (*multi_header).headers[0] = pack_descriptor;
    (*pack_descriptor).element_types[0] = K_ARRAY;
    if ( ( arr_descriptor = ds_alloc_array_desc (num_dim, 0) ) == NULL )
    {
	ds_dealloc_multi (multi_header);
	m_error_notify (function_name, "array descriptor");
        return (NULL);
    }
    (*pack_descriptor).element_desc[0] = (char *) arr_descriptor;
    while (dim_count < num_dim)
    {
	/*  Allocate and add adimension descriptor  */
        if (names == NULL)
        {
	    /*  Create temporary dimension name */
            (void) sprintf (tmp_name, "Axis %u", dim_count);
            tmp_pointer = tmp_name;
        }
        else
        {
	    /*  Copy over character pointer */
            tmp_pointer = names[dim_count];
        }
        if ( (minima == NULL) && (maxima == NULL) )
        {
	    /*  Range is from  0.0  to  lengths - 1 */
            minimum = 0.0;
            maximum = (lengths[dim_count] - 1);
        }
        else
        {
	    /*  Range has at least one boundary supplied    */
            if (minima == NULL)
            {
		/*  Maximum only supplied   */
                maximum = maxima[dim_count];
                minimum = maximum - (lengths[dim_count] - 1);
            }
            else
            {
		/*  Minimum supplied    */
                minimum = minima[dim_count];
                if (maxima == NULL)
                {
		    /*  Minimum only supplied   */
                    maximum = minimum + (lengths[dim_count] - 1);
                }
                else
                {
		    /*  Maximum also supplied   */
                    maximum = maxima[dim_count];
                }
            }
        }
        if ( ( dimension = ds_alloc_dim_desc (tmp_pointer,
					      lengths[dim_count],
					      minimum, maximum, TRUE) )
	    == NULL )
        {
	    ds_dealloc_multi (multi_header);
	    m_error_notify (function_name, "dimension descriptor");
            return (NULL);
        }
        (*arr_descriptor).dimensions[dim_count] = dimension;
        ++dim_count;
    }

    if ( ( pack_descriptor = ds_alloc_packet_desc (num_elements) )
	== NULL )
    {
	ds_dealloc_multi (multi_header);
	m_error_notify (function_name, "array packet descriptor");
        return (NULL);
    }
    (*arr_descriptor).packet = pack_descriptor;
    for (elem_count = 0; elem_count < num_elements; ++elem_count)
    {
	(*pack_descriptor).element_types[elem_count] = data_type;
	if ( ( (*pack_descriptor).element_desc[elem_count] = 
	      m_alloc ( (unsigned int) strlen (data_names[elem_count]) +
		       (unsigned int) 1 ) ) == NULL )
	{
	    ds_dealloc_multi (multi_header);
	    m_error_notify (function_name, "element name");
	    return (NULL);
	}
	(void) strcpy ( (*pack_descriptor).element_desc[elem_count],
		       data_names[elem_count]);
    }
    /*  Allocate the data space  */
    if ( ( (*multi_header).data[0] =
	  ds_alloc_data ( (*multi_header).headers[0], TRUE, TRUE ) )
        == NULL )
    {
	ds_dealloc_multi (multi_header);
	m_error_notify (function_name, "top level packet");
        return (NULL);
    }
    return_value = *( (char **) (*multi_header).data[0] );
    *multi_desc = multi_header;
    return (return_value);
}   /*  End Function alloc_array   */

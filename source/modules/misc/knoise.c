/*  knoise.c

    Source file for  knoise  (generate noise in Karma format).

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

/*  This Karma module will generate a multi-dimensional array of noise values
    and will write the data to a Karma data file.


    Written by      Richard Gooch   24-JAN-1996

    Last updated by Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


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
#include <karma_n.h>
#include <karma_a.h>


#define VERSION "1.0"

#define MAX_DIMENSIONS (unsigned int) 100

STATIC_FUNCTION (flag knoise, (char *command, FILE *fp) );
STATIC_FUNCTION (void generate_file,
		 (char *arrayname, unsigned int elem_type) );


/*  Private data  */
static unsigned int num_dimensions = 0;
static char *names[MAX_DIMENSIONS];
static unsigned long lengths[MAX_DIMENSIONS];
static double minima[MAX_DIMENSIONS];
static double maxima[MAX_DIMENSIONS];
static unsigned int elem_type = K_FLOAT;
static char *elem_name = "Intensity";
static double lower_range = -1.0;
static double upper_range = 1.0;
static double mean = 0.0;
static double variance = 1.0;

#define DISTRIBUTION_UNIFORM  0
#define DISTRIBUTION_GAUSSIAN 1
#define NUM_DISTRIBUTIONS 2
static char *distribution_alternatives[NUM_DISTRIBUTIONS] =
{
    "uniform",
    "guassian"
};
static char *distribution_comments[NUM_DISTRIBUTIONS] =
{
    "specify range",
    "specify mean and variance"
};
static unsigned int distribution = DISTRIBUTION_UNIFORM;

int main (int argc, char **argv)
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
    panel_add_item (panel, "upper_range", "absolute", K_DOUBLE, &upper_range,
		    PIA_END);
    panel_add_item (panel, "lower_range", "absolute", K_DOUBLE, &lower_range,
		    PIA_END);
    panel_add_item (panel, "variance", "absolute", K_DOUBLE, &variance,
		    PIA_END);
    panel_add_item (panel, "mean", "absolute", K_DOUBLE, &mean,
		    PIA_END);
    panel_add_item (panel, "distribution", "", PIT_CHOICE_INDEX, &distribution,
		    PIA_NUM_CHOICE_STRINGS, NUM_DISTRIBUTIONS,
		    PIA_CHOICE_STRINGS, distribution_alternatives,
		    PIA_CHOICE_COMMENTS, distribution_comments,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "knoise", VERSION, knoise, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag knoise (p, fp)
char *p;
FILE *fp;
{
    char *arrayfile;
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
    if (lower_range >= upper_range)
    {
	(void) fprintf (stderr,
			"lower_range: %e is not less that upper_range: %e\n",
			lower_range, upper_range);
	return (TRUE);
    }
    for ( ; p; p = ex_command_skip (p) )
    {
	if ( ( arrayfile = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp, "Error extracting arrayfile name\n");
	    return (TRUE);
	}
	generate_file (arrayfile, elem_type);
	m_free (arrayfile);
    }
    return (TRUE);
}   /*  End Function knoise  */

static void generate_file (char *arrayname, unsigned int elem_type)
/*  This routine will generate a Karma arrayfile with a multi-dimensional array
    of a single atomic element.
    The name of the Karma arrayfile must be pointed to by  arrayname  .
    The type of the data elements must be in  elem_type  .
    The routine returns nothing.
*/
{
    unsigned int array_size;
    unsigned int elem_count;
    unsigned int dim_count;
    double factor = 0.0;  /*  Initialised to keep compiler happy  */
    double value[2];
    char *array;
    multi_array *multi_desc;
    extern unsigned int num_dimensions;
    extern unsigned int distribution;
    extern double lower_range, upper_range;
    extern double mean, variance;
    extern unsigned long lengths[MAX_DIMENSIONS];
    extern double minima[MAX_DIMENSIONS];
    extern double maxima[MAX_DIMENSIONS];
    extern char *names[MAX_DIMENSIONS];
    extern char *elem_name;
    extern char *sys_errlist[];
    static char function_name[] = "generate_file";

    /*  Determine array size (in co-ordinates)  */
    array_size = 1;
    for (dim_count = 0; dim_count < num_dimensions; ++dim_count)
    {
	if (lengths[dim_count] < 1)
	{
	    (void) fprintf (stderr,
			    "Dimension: %u has length: %lu: must be greater than 0\n",
			    dim_count, lengths[dim_count]);
	    return;
	}
	array_size *= (unsigned int) lengths[dim_count];
    }
    if ( ( array = ds_easy_alloc_array (&multi_desc,
					(unsigned int) num_dimensions,
					lengths, minima, maxima,
					(CONST char **) names,
					elem_type, elem_name) )
	== NULL )
    {
	(void) fprintf (stderr,
			"NULL return value from function: ds_easy_alloc_array\n");
	return;
    }
    switch (distribution)
    {
      case DISTRIBUTION_UNIFORM:
	factor = upper_range - lower_range;
	break;
      case DISTRIBUTION_GAUSSIAN:
	break;
      default:
	(void) fprintf (stderr, "Illegal distribution: %u\n",
			distribution);
	a_prog_bug (function_name);
	break;
    }
    value[1] = 0.0;
    for (elem_count = 0; elem_count < array_size; ++elem_count)
    {
	switch (distribution)
	{
	  case DISTRIBUTION_UNIFORM:
	    value[0] = lower_range + n_uniform () * factor;
	    break;
	  case DISTRIBUTION_GAUSSIAN:
	    value[0] = mean + n_gaussian () * variance;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal distribution: %u\n",
			    distribution);
	    a_prog_bug (function_name);
	    break;
	}
	if ( ( array = ds_put_element (array, elem_type, value) ) == NULL )
	{
	    (void) fprintf (stderr, "Error writing element!\n");
	    a_prog_bug (function_name);
	}
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

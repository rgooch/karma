/*  kfuncgen.c

    Source file for  kfuncgen  (generate 1-D function in Karma format).

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

/*  This Karma module will generate a 1-dimensional array of function values
    and will write the data to a Karma data file.


    Written by      Richard Gooch   25-JAN-1996

    Last updated by Richard Gooch   1-JUN-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <math.h>
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
#include <karma_s.h>


#define VERSION "1.0"

#define MAX_DIMENSIONS (unsigned int) 1
#define MAX_POLY_COEFFICIENTS (unsigned int) 10



/*  Private data  */
static unsigned int num_dimensions = 0;
static char *names[MAX_DIMENSIONS];
static unsigned long lengths[MAX_DIMENSIONS];
static double minima[MAX_DIMENSIONS];
static double maxima[MAX_DIMENSIONS];
static unsigned int elem_type = K_FLOAT;
static char *elem_name = "Intensity";

static double amplitude = 1.0;
static unsigned int num_denominator_poly_coefficients = 0;
static double denominator_poly_coefficients[MAX_POLY_COEFFICIENTS];
static double duty_cycle = 0.5;
static double minimum_value = -1.0;
static double maximum_value = 1.0;
static double offset = 0.0;
static double period = 1.0;
static unsigned int num_poly_coefficients = 0;
static double poly_coefficients[MAX_POLY_COEFFICIENTS];
static double position = 0.0;
/*
static int seed = 1;
*/
static double sinc_width = 1.0;

#define FUNC_POLYNOMIAL (unsigned int) 0
#define FUNC_EXPONENTIAL (unsigned int) 1
#define FUNC_SINUSOID (unsigned int) 2
#define FUNC_SQUARE_WAVE (unsigned int) 3
#define FUNC_TRIANGLE_WAVE (unsigned int) 4
#define FUNC_SINC (unsigned int) 5
#define FUNC_UNIFORM_NOISE (unsigned int) 6
#define FUNC_GAUSSIAN_NOISE (unsigned int) 7
#define FUNC_IMPULSE (unsigned int) 8
#define FUNC_RATIO_OF_POLYNOMIALS (unsigned int) 9
#define NUM_FUNC_ALTERNATIVES (unsigned int) 10

char *function_alternatives[NUM_FUNC_ALTERNATIVES] =
{
    "polynomial",
    "exponential",
    "sinusoid",
    "square-wave",
    "triangle-wave",
    "sinc",
    "uniform-noise",
    "gaussian-noise",
    "impulse",
    "ratio-of-polynomials"
};
static unsigned int function_type = FUNC_POLYNOMIAL;


/*  Private functions  */
STATIC_FUNCTION (flag kfuncgen, (char *command, FILE *fp) );
STATIC_FUNCTION (void generate_file,
		 (char *arrayname, unsigned int elem_type) );
STATIC_FUNCTION (flag compute_polynomial,
		 (unsigned int length, double min, double max,
		  char *array, unsigned int elem_type) );
STATIC_FUNCTION (flag compute_exponential,
		 (unsigned int length, double min, double max,
		  char *array, unsigned int elem_type) );
STATIC_FUNCTION (flag compute_sinusoid,
		 (unsigned int length, double min, double max,
		  char *array, unsigned int elem_type) );
STATIC_FUNCTION (flag compute_sinc,
		 (unsigned int length, double min, double max,
		  char *array, unsigned int elem_type) );
STATIC_FUNCTION (flag compute_random,
		 (unsigned int length, char *array,
		  unsigned int elem_type, unsigned int rand_type) );
STATIC_FUNCTION (flag compute_impulse,
		 (unsigned int length, double min, double max,
		  char *array, unsigned int elem_type,
		  double position, double amplitude) );
STATIC_FUNCTION (flag compute_square_wave,
		 (unsigned int length, double min, double max,
		  char *array, unsigned int elem_type,
		  double period, double duty_cycle,
		  double position, double min_value, double max_value) );
STATIC_FUNCTION (flag compute_triangle_wave,
		 (unsigned int length, double min, double max,
		  char *array, unsigned int elem_type,
		  double period, double duty_cycle,
		  double position, double min_value, double max_value) );
STATIC_FUNCTION (flag compute_ratio_of_polynomials,
		 (unsigned int length, double min, double max, char *array,
		  unsigned int elem_type) );


/*  Public functions follow  */

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
    panel_add_item (panel, "dimension", "", PIT_GROUP, (char *) group,
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
    panel_add_item (panel, "sinc_width", "value", K_DOUBLE, &sinc_width,
		    PIA_END);
/*
    panel_add_item (panel, "seed", "value", K_INT, &seed,
		    PIA_END);
*/
    panel_add_item (panel, "position", "value", K_DOUBLE, &position,
		    PIA_END);
    panel_add_item (panel, "poly_coeff", "values", K_DOUBLE, poly_coefficients,
		    PIA_ARRAY_MAX_LENGTH, MAX_POLY_COEFFICIENTS,
		    PIA_ARRAY_LENGTH, &num_poly_coefficients,
		    PIA_END);
    panel_add_item (panel, "period", "value", K_DOUBLE, &period,
		    PIA_END);
    panel_add_item (panel, "offset", "value", K_DOUBLE, &offset,
		    PIA_END);
    panel_add_item (panel, "min_value", "value", K_DOUBLE, &minimum_value,
		    PIA_END);
    panel_add_item (panel, "max_value", "value", K_DOUBLE, &maximum_value,
		    PIA_END);
    panel_add_item (panel, "duty_cycle", "fraction", K_DOUBLE, &duty_cycle,
		    PIA_END);
    panel_add_item (panel, "dpoly_coeff", "values", K_DOUBLE,
		    denominator_poly_coefficients,
		    PIA_ARRAY_MAX_LENGTH, MAX_POLY_COEFFICIENTS,
		    PIA_ARRAY_LENGTH, &num_denominator_poly_coefficients,
		    PIA_END);
    panel_add_item (panel, "amplitude", "value", K_DOUBLE, &amplitude,
		    PIA_END);
    panel_add_item (panel, "function", "", PIT_CHOICE_INDEX, &function_type,
		    PIA_NUM_CHOICE_STRINGS, NUM_FUNC_ALTERNATIVES,
		    PIA_CHOICE_STRINGS, function_alternatives,
/*
		    PIA_CHOICE_COMMENTS, function_comments,
*/
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "kfuncgen", VERSION, kfuncgen, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */


/*  Private functions follow  */

static flag kfuncgen (char *p, FILE *fp)
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
}   /*  End Function kfuncgen  */

static void generate_file (char *arrayname, unsigned int elem_type)
/*  This routine will generate a Karma arrayfile with a 1-dimensional array of
    a single atomic element.
    The name of the Karma arrayfile must be pointed to by  arrayname  .
    The type of the data elements must be in  elem_type  .
    The routine returns nothing.
*/
{
    char *array;
    multi_array *multi_desc;
    extern unsigned int num_dimensions;
    extern unsigned int function_type;
    extern unsigned long lengths[MAX_DIMENSIONS];
    extern double minima[MAX_DIMENSIONS];
    extern double maxima[MAX_DIMENSIONS];
    extern char *names[MAX_DIMENSIONS];
    extern char *elem_name;
    static char function_name[] = "generate_file";

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
    switch (function_type)
    {
      case FUNC_POLYNOMIAL:
	if ( !compute_polynomial (lengths[0], minima[0], maxima[0],
				  array, elem_type) )
	{
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	break;
      case FUNC_EXPONENTIAL:
	if ( !compute_exponential (lengths[0], minima[0], maxima[0],
				   array, elem_type ) )
	{
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	break;
      case FUNC_SINUSOID:
	if ( !compute_sinusoid (lengths[0], minima[0], maxima[0],
				array, elem_type ) )
	{
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	break;
	case FUNC_SINC:
	if ( !compute_sinc (lengths[0], minima[0], maxima[0],
			    array, elem_type ) )
	{
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	break;
      case FUNC_UNIFORM_NOISE:
      case FUNC_GAUSSIAN_NOISE:
	if ( !compute_random (lengths[0], array, elem_type, function_type ) )
	{
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	break;
      case FUNC_IMPULSE:
	if ( !compute_impulse (lengths[0], minima[0], maxima[0],
			       array, elem_type,
			       position, amplitude ) )
	{
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	break;
      case FUNC_SQUARE_WAVE:
	if ( !compute_square_wave (lengths[0], minima[0], maxima[0],
				   array, elem_type,
				   period, duty_cycle, position,
				   minimum_value, maximum_value ) )
	{
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	break;
      case FUNC_TRIANGLE_WAVE:
	if ( !compute_triangle_wave (lengths[0], minima[0], maxima[0],
				     array, elem_type,
				     period, duty_cycle, position,
				     minimum_value, maximum_value ) )
	{
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	break;
      case FUNC_RATIO_OF_POLYNOMIALS:
	if ( !compute_ratio_of_polynomials (lengths[0], minima[0], maxima[0],
					    array, elem_type ) )
	{
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	break;
      default:
	(void) fprintf (stderr, "Illegal function type: %u\n", function_type);
	a_prog_bug (function_name);
	break;
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

static flag compute_polynomial (unsigned int length, double min, double max,
				char *array, unsigned int elem_type)
/*  This routine will compute a polynomial for  length  points.
    The minimum and maximum of all the points must be given by  min  and  max
    The routine will write the data into the array pointed to by  array  .
    The type of the element to write must be given by  elem_type  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int coord_count;
    int power_count;
    double sum[2];
    double coord;
    double coord_n;
    extern unsigned int num_poly_coefficients;
    extern double poly_coefficients[MAX_POLY_COEFFICIENTS];
    static char function_name[] = "compute_polynomial";

    if (array == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if (num_poly_coefficients < 1)
    {
	(void) fprintf (stderr,
			"Must have at least one polynomial co-efficient\n");
	return (FALSE);
    }
    sum[1] = 0.0;
    for (coord_count = 0; coord_count < length; ++coord_count)
    {
	coord = min + (double) coord_count * (max -min) / (double) (length -1);
	sum[0] = 0.0;
	coord_n = 1.0;
	for (power_count = num_poly_coefficients - 1; power_count >= 0;
	     --power_count)
	{
	    sum[0] += coord_n * poly_coefficients[power_count];
	    coord_n *= coord;
	}
	if ( ( array = ds_put_element (array, elem_type, sum) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "NULL return from function: ds_put_element\n");
	    a_prog_bug (function_name);
	}
	/*  Do test for control_c  */
	if ( s_check_for_int () )
	{
	    (void) fprintf (stderr, "User interrupt: no arrayfile created\n");
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function compute_polynomial  */

static flag compute_exponential (unsigned int length, double min, double max,
				 char *array, unsigned int elem_type)
/*  This routine will compute the exponential of a polynomial for
    length  points.
    The minimum and maximum of all the points must be given by  min  and  max
    The routine will write the data into the array pointed to by  array  .
    The type of the element to write must be given by  elem_type  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int coord_count;
    int power_count;
    double sum[2];
    double coord;
    double coord_n;
    extern unsigned int num_poly_coefficients;
    extern double amplitude;
    extern double poly_coefficients[MAX_POLY_COEFFICIENTS];
    static char function_name[] = "compute_exponential";

    if (array == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if (num_poly_coefficients < 1)
    {
	(void) fprintf (stderr,
			"Must have at least one polynomial co-efficient\n");
	return (FALSE);
    }
    sum[1] = 0.0;
    for (coord_count = 0; coord_count < length; ++coord_count)
    {
	coord = min + (double) coord_count * (max -min) / (double) (length -1);
	sum[0] = 0.0;
	coord_n = 1.0;
	for (power_count = num_poly_coefficients - 1; power_count >= 0;
	     --power_count)
	{
	    sum[0] += coord_n * poly_coefficients[power_count];
	    coord_n *= coord;
	}
	sum[0] = amplitude * exp (sum[0]);
	if ( ( array = ds_put_element (array, elem_type, sum) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "NULL return from function: ds_put_element\n");
	    a_prog_bug (function_name);
	}
	/*  Do test for control_c  */
	if ( s_check_for_int () )
	{
	    (void) fprintf (stderr, "User interrupt: no arrayfile created\n");
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function compute_exponential  */

static flag compute_sinusoid (unsigned int length, double min, double max,
			      char *array, unsigned int elem_type)
/*  This routine will compute the sinusoid of a polynomial for
    length  points.
    The minimum and maximum of all the points must be given by  min  and  max
    The routine will write the data into the array pointed to by  array  .
    The type of the element to write must be given by  elem_type  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int coord_count;
    int power_count;
    double sum[2];
    double coord;
    double coord_n;
    extern unsigned int num_poly_coefficients;
    extern double amplitude;
    extern double offset;
    extern double poly_coefficients[MAX_POLY_COEFFICIENTS];
    static char function_name[] = "compute_sinusoid";

    if (array == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if (num_poly_coefficients < 1)
    {
	(void) fprintf (stderr,
			"Must have at least one polynomial co-efficient\n");
	return (FALSE);
    }
    sum[1] = 0.0;
    for (coord_count = 0; coord_count < length; ++coord_count)
    {
	coord = min + (double) coord_count * (max -min) / (double) (length -1);
	sum[0] = 0.0;
	coord_n = 1.0;
	for (power_count = num_poly_coefficients - 1; power_count >= 0;
	     --power_count)
	{
	    sum[0] += coord_n * poly_coefficients[power_count];
	    coord_n *= coord;
	}
	sum[0] = amplitude * sin (sum[0]) + offset;
	if ( ( array = ds_put_element (array, elem_type, sum) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "NULL return from function: ds_put_element\n");
	    a_prog_bug (function_name);
	}
	/*  Do test for control_c  */
	if ( s_check_for_int () )
	{
	    (void) fprintf (stderr, "User interrupt: no arrayfile created\n");
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function compute_sinusoid  */

static flag compute_sinc (unsigned int length, double min, double max,
			  char *array, unsigned int elem_type)
/*  This routine will compute a sinc function for  length  points.
    The minimum and maximum of all the points must be given by  min  and  max
    The routine will write the data into the array pointed to by  array  .
    The type of the element to write must be given by  elem_type  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    double pi;
    unsigned int coord_count;
    double value[2];
    double coord;
    extern double amplitude;
    extern double sinc_width;
    static char function_name[] = "compute_sinc";

    if (array == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    pi = 4.0 * atan (1.0);
    for (coord_count = 0; coord_count < length; ++coord_count)
    {
	coord = min + (double) coord_count * (max -min) / (double) (length -1);
	coord *= pi / sinc_width;
	if (coord == 0.0)
	{
	    value[0] = amplitude;
	}
	else
	{
	    value[0] = amplitude * sin (coord) / coord;
	}
	if ( ( array = ds_put_element (array, elem_type, value) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "NULL return from function: ds_put_element\n");
	    a_prog_bug (function_name);
	}
	/*  Do test for control_c  */
	if ( s_check_for_int () )
	{
	    (void) fprintf (stderr, "User interrupt: no arrayfile created\n");
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function compute_sinc  */

static flag compute_random (unsigned int length, char *array,
			    unsigned int elem_type, unsigned int rand_type)
/*  This routine will compute random numbers for  length  points.
    The routine will write the data into the array pointed to by  array  .
    The type of the element to write must be given by  elem_type  .
    The distribution of random numbers to generate must be given by  rand_type
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int array_count;
    unsigned int elem_size;
    double uniform_scale;
    double noise_val = 0.0;  /*  Initialised to keep compiler happy  */
    extern double amplitude;
    extern double minimum_value;
    extern double maximum_value;
    extern double offset;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "compute_random";

    /*  If element type is complex, then double array size and change element
	type to a real  */
    switch (elem_type)
    {
      case K_COMPLEX:
	length *= 2;
	elem_type = K_FLOAT;
	break;
      case K_DCOMPLEX:
	length *= 2;
	elem_type = K_DOUBLE;
	break;
      case K_BCOMPLEX:
	length *= 2;
	elem_type = K_BYTE;
	break;
      case K_ICOMPLEX:
	length *= 2;
	elem_type = K_INT;
	break;
      case K_SCOMPLEX:
	length *= 2;
	elem_type = K_SHORT;
	break;
      default:
	break;
    }
    elem_size = host_type_sizes[elem_type];
    uniform_scale = maximum_value - minimum_value;
    for (array_count = 0; array_count < length; ++array_count)
    {
	/*  Generate noise value  */
	switch (rand_type)
	{
	  case FUNC_UNIFORM_NOISE:
	    /*  Generate uniform distribution  */
	    noise_val = minimum_value + uniform_scale * n_uniform ();
	    break;
	  case FUNC_GAUSSIAN_NOISE:
	    /*  Generate Gaussian distribution  */
	    noise_val = offset + n_gaussian () * amplitude;
	    break;
	  default:
	    (void) fprintf (stderr, "Bad distribution value: %u",
			    rand_type);
	    a_prog_bug (function_name);
	    break;
	}
	/*  Write noise value into array  */
	switch (elem_type)
	{
	  case K_FLOAT:
	    *(float *) array = noise_val;
	    break;
	  case K_DOUBLE:
	    *(double *) array = noise_val;
	    break;
	  case K_BYTE:
	    *array = noise_val;
	    break;
	  case K_SHORT:
	    *(short *) array = noise_val;
	    break;
	  case K_INT:
	    *(int *) array = noise_val;
	    break;
	  default:
	    (void) fprintf (stderr, "Bad element type value: %u", elem_type);
	    a_prog_bug (function_name);
	    break;
	}
	array += elem_size;
	/*  Do test for control_c  */
	if ( s_check_for_int () )
	{
	    (void) fprintf (stderr, "User interrupt: no arrayfile created\n");
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function compute_random  */

static flag compute_impulse (unsigned int length, double min, double max,
			     char *array, unsigned int elem_type,
			     double position, double amplitude)
/*  This routine will compute an impulse function for  length  points.
    The minimum and maximum of all the points must be given by  min  and  max
    The routine will write the data into the array pointed to by  array  .
    The type of the element to write must be given by  elem_type  .
    The position of the impulse must be given by  position  .The co-ordinate
    which is nearest to  position  will be given the value of  amplitude  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int index;
    double value[2];
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "compute_impulse";

    if (array == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    value[0] = amplitude;
    value[1] = 0.0;
    if (position <= min)
    {
	index = 0;
    }
    else if (position >= max)
    {
	index = length - 1;
    }
    else
    {
	index = ( (position - min) /
		 (max - min) * (double) (length - 1) + 0.5 );
    }
    /*  Index  */
    array += host_type_sizes[elem_type] * index;
    if (ds_put_element (array, elem_type, value) == NULL)
    {
	(void) fprintf (stderr,
			"NULL return from function: ds_put_element\n");
	a_prog_bug (function_name);
    }
    return (TRUE);
}   /*  End Function compute_impulse  */

static flag compute_square_wave (unsigned int length, double min, double max,
				 char *array, unsigned int elem_type,
				 double period, double duty_cycle,
				 double position, double min_value,
				 double max_value)
/*  This routine will compute a square wave function for  length  points.
    The minimum and maximum of all the points must be given by  min  and  max
    The routine will write the data into the array pointed to by  array  .
    The type of the element to write must be given by  elem_type  .
    The period of the square wave must be given by  period  .
    The duty cycle of the square wave must be given by  duty_cycle  .
    A co-ordinate where the function will have a falling edge must be given by
    position  .
    The minimum and maximum values of the square wave must be given by
    min_value  and  max_value  ,respectively.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int coord_count;
    double coord;
    double frac;
    double value[2];
    static char function_name[] = "compute_square_wave";

    if (array == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    value[1] = 0.0;
    for (coord_count = 0; coord_count < length; ++coord_count)
    {
	coord = min + (double) coord_count * (max -min) / (double) (length -1);
	/*  Compute which part of period this co-ordinate is in  */
	frac = (coord - position) / period;
	frac -= floor (frac);
	if (frac < duty_cycle)
	{
	    value[0] = min_value;
	}
	else
	{
	    value[0] = max_value;
	}
	if ( ( array = ds_put_element (array, elem_type, value) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "NULL return from function: ds_put_element\n");
	    a_prog_bug (function_name);
	}
	/*  Do test for control_c  */
	if ( s_check_for_int () )
	{
	    (void) fprintf (stderr, "User interrupt: no arrayfile created\n");
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function compute_square_wave  */

static flag compute_triangle_wave (unsigned int length, double min, double max,
				   char *array, unsigned int elem_type,
				   double period, double duty_cycle,
				   double position, double min_value,
				   double max_value)
/*  This routine will compute a triangle wave function for  length  points.
    The minimum and maximum of all the points must be given by  min  and  max
    The routine will write the data into the array pointed to by  array  .
    The type of the element to write must be given by  elem_type  .
    The period of the triangle wave must be given by  period  .
    The duty cycle of the triangle wave must be given by  duty_cycle  .
    A co-ordinate where the function will be at a minimum must be given by
    position  .
    The minimum and maximum values of the triangle wave must be given by
    min_value  and  max_value  ,respectively.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int coord_count;
    double coord;
    double frac;
    double range;
    double value[2];
    static char function_name[] = "compute_triangle_wave";

    if (array == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    range = max_value - min_value;
    value[1] = 0.0;
    for (coord_count = 0; coord_count < length; ++coord_count)
    {
	coord = min + (double) coord_count * (max -min) / (double) (length -1);
	/*  Compute which part of period this co-ordinate is in  */
	frac = (coord - position) / period;
	frac -= floor (frac);
	if (frac < duty_cycle)
	{
	    value[0] = min_value + range * frac / duty_cycle;
	}
	else
	{
	    value[0] = min_value + range * (1.0 - frac) / (1.0 - duty_cycle);
	}
	if ( ( array = ds_put_element (array, elem_type, value) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "NULL return from function: ds_put_element\n");
	    a_prog_bug (function_name);
	}
	/*  Do test for control_c  */
	if ( s_check_for_int () )
	{
	    (void) fprintf (stderr, "User interrupt: no arrayfile created\n");
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function compute_triangle_wave  */

static flag compute_ratio_of_polynomials (unsigned int length,
					  double min, double max, char *array,
					  unsigned int elem_type)
/*  This routine will compute the ratio of two polynomials for  length  points.
    The minimum and maximum of all the points must be given by  min  and  max
    The routine will write the data into the array pointed to by  array  .
    The type of the element to write must be given by  elem_type  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int coord_count;
    int power_count;
    double numerator_sum;
    double denominator_sum;
    double coord;
    double coord_n;
    double value[2];
    extern unsigned int num_poly_coefficients;
    extern double poly_coefficients[MAX_POLY_COEFFICIENTS];
    extern unsigned int num_denominator_poly_coefficients;
    extern double denominator_poly_coefficients[MAX_POLY_COEFFICIENTS];
    static char function_name[] = "compute_ratio_of_polynomials";

    if (array == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if (num_poly_coefficients < 1)
    {
	(void) fprintf (stderr,
			"Must have at least one numerator polynomial co-efficient\n");
	return (FALSE);
    }
    if (num_denominator_poly_coefficients < 1)
    {
	(void) fprintf (stderr,
			"Must have at least one denominator polynomial co-efficient\n");
	return (FALSE);
    }
    value[1] = 0.0;
    for (coord_count = 0; coord_count < length; ++coord_count)
    {
	coord = min + (double) coord_count * (max -min) / (double) (length -1);
	/*  Compute numerator polynomial  */
	numerator_sum = 0.0;
	coord_n = 1.0;
	for (power_count = num_poly_coefficients - 1; power_count >= 0;
	     --power_count)
	{
	    numerator_sum += coord_n * poly_coefficients[power_count];
	    coord_n *= coord;
	}
	/*  Compute denominator polynomial  */
	denominator_sum = 0.0;
	coord_n = 1.0;
	for (power_count = num_denominator_poly_coefficients - 1;
	     power_count >= 0;
	     --power_count)
	{
	    denominator_sum += (coord_n *
				   denominator_poly_coefficients[power_count]);
	    coord_n *= coord;
	}
	if (denominator_sum == 0.0)
	{
	    value[0] = TOOBIG;
	}
	else
	{
	    value[0] = numerator_sum / denominator_sum;
	}
	if ( ( array = ds_put_element (array, elem_type, value) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "NULL return from function: ds_put_element\n");
	    a_prog_bug (function_name);
	}
	/*  Do test for control_c  */
	if ( s_check_for_int () )
	{
	    (void) fprintf (stderr, "User interrupt: no arrayfile created\n");
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function compute_ratio_of_polynomials  */

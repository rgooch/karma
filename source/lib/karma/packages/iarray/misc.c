/*LINTLIBRARY*/
/*  misc.c

    This code provides miscellaneous Intelligent Array routines.

    Copyright (C) 1996  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains all routines needed for the simple processing of
  n-dimensional data structures.


    Written by      Richard Gooch   28-FEB-1996

    Updated by      Richard Gooch   1-APR-1996: Moved remaing functions to new
  documentation style.

    Updated by      Richard Gooch   16-JUN-1996: Created
  <iarray_get_data_scaling> routine.

    Last updated by Richard Gooch   19-JUN-1996: Created <iarray_format_value>
  routine.


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <karma_iarray.h>
#include <karma_a.h>


#define VERIFY_IARRAY(array) if (array == NULL) \
{(void) fprintf (stderr, "NULL iarray passed\n"); \
 a_prog_bug (function_name); }
#ifdef dummy
if (array->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid iarray\n"); \
 a_prog_bug (function_name); }
#endif


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
unsigned int iarray_dim_index (iarray array, CONST char *name)
/*  [SUMMARY] This routine will find the index of a named dimension.
    <array> The array.
    <name> The name of the dimension.
    [RETURNS] The dimension index if found, else the number of dimension in the
    array is returned.
*/
{
    unsigned int count, num_dim;
    static char function_name[] = "iarray_dim_index";

    VERIFY_IARRAY (array);
    num_dim = iarray_num_dim (array);
    for (count = 0; count < num_dim; ++count)
    {
	if (strcmp (iarray_dim_name (array, count), name) == 0) return (count);
    }
    return (num_dim);
}   /*  End Function iarray_dim_index  */

/*PUBLIC_FUNCTION*/
flag iarray_get_data_scaling (iarray array, double *scale, double *offset)
/*  [SUMMARY] Get the scale and offset for data in an Intelligent Array.
    [PURPOSE] This routine will determine the scale and offset for data in an
    Intelligent Array. This may be important when a floating-point array has
    been converted to an integer array to save space. Scaling information
    should be attached to the array so that the original data values may be
    reconstructed (aside from quantisation effects). The following expression
    may be used to convert scaled values to real values:
    (output = input * scale + offset). The scaling and offset values should
    previously have been attached to the Intelligent Array using the
    [<iarray_set_data_scaling>] routine.
    <array> The array.
    <scale> The scaling value will be written here. The name of the scaling
    value is constructed by appending "__SCALE" to the array value name (see
    [<iarray_get_value_name>]). If no scaling value is found, 1.0 is written
    here.
    <offset> The offset value will be written here. The name of the offset
    value is constructed by appending "__OFFSET" to the array value name (see
    [<iarray_get_value_name>]). If no offset value is found, 0.0 is written
    here.
    [RETURNS] TRUE if either the scaling or offset value were found, else FALSE
*/
{
    flag found = FALSE;
    double value[2];
    char txt[STRING_LENGTH];
    static char function_name[] = "iarray_get_data_scaling";

    VERIFY_IARRAY (array);
    (void) sprintf ( txt, "%s__SCALE", iarray_value_name (array) );
    if ( iarray_get_named_value (array, txt, NULL, value) )
    {
	found = TRUE;
	*scale = value[0];
    }
    else
    {
	/*  Not found: use default  */
	*scale = 1.0;
    }
    sprintf ( txt, "%s__OFFSET", iarray_value_name (array) );
    if ( iarray_get_named_value (array, txt, NULL, value) )
    {
	found = TRUE;
	*offset = value[0];
    }
    else
    {
	/*  Not found: use default  */
	*offset = 0.0;
    }
    return (found);
}   /*  End Function iarray_get_data_scaling  */

/*PUBLIC_FUNCTION*/
flag iarray_set_data_scaling (iarray array, double scale, double offset)
/*  [SUMMARY] Set the scale and offset for data in an Intelligent Array.
    [PURPOSE] This routine will set the scale and offset for data in an
    Intelligent Array. This may be important when a floating-point array has
    been converted to an integer array to save space. The scaling information
    will be attached to the array so that the original data values may be
    reconstructed (aside from quantisation effects). The following expression
    may be used to convert scaled values to real values:
    (output = input * scale + offset).
    <array> The array.
    <scale> The scaling value. The name of the scaling value is constructed by
    appending "__SCALE" to the array value name (see [<iarray_get_value_name>])
    <offset> The offset value. The name of the offset value is constructed by
    appending "__OFFSET" to the array value name(see [<iarray_get_value_name>])
    [RETURNS] TRUE if the scaling information is different from what was
    already attached to the array, else FALSE.
*/
{
    double sc, off;
    double value[2];
    char txt[STRING_LENGTH];
    static char function_name[] = "iarray_set_data_scaling";

    VERIFY_IARRAY (array);
    (void) iarray_get_data_scaling (array, &sc, &off);
    if ( (scale == sc) && (offset == off) ) return (FALSE);
    (void) sprintf ( txt, "%s__SCALE", iarray_value_name (array) );
    value[0] = scale;
    value[1] = 0.0;
    if ( !iarray_put_named_value (array, txt, K_DOUBLE, value) )
    {
	(void) fprintf (stderr, "Error attaching \"%s\" element\n", txt);
	return (TRUE);
    }
    sprintf ( txt, "%s__OFFSET", iarray_value_name (array) );
    value[0] = offset;
    if ( !iarray_put_named_value (array, txt, K_DOUBLE, value) )
    {
	(void) fprintf (stderr, "Error attaching \"%s\" element\n", txt);
	return (TRUE);
    }
    return (TRUE);
}   /*  End Function iarray_set_data_scaling  */

/*EXPERIMENTAL_FUNCTION*/
void iarray_format_value (iarray array, char string[STRING_LENGTH],
			  double value, double scale, double offset)
/*  [SUMMARY] Format a data value into a string.
    <array> The Intelligent Array the value is associated with.
    <string> The string to write to.
    <value> The value to format.
    <scale> The scale value to apply to the data. If this is TOOBIG the routine
    uses the scale and offset attached to the array. See
    [<iarray_set_data_scaling>] for details.
    <offset> The offset to apply after scaling the data.
    [RETURNS] Nothing.
*/
{
    double scaled_value;
    char value_str[STRING_LENGTH];
    CONST char *value_name;
    static char function_name[] = "iarray_format_value";

    VERIFY_IARRAY (array);
    if (scale >= TOOBIG)
    {
	(void) iarray_get_data_scaling (array, &scale, &offset);
    }
    /*  Compute value  */
    scaled_value = scale * value + offset;
    value_name = iarray_value_name (array);
    if (strcmp (value_name, "Data Value") == 0)
    {
	/*  No useful unit name  */
	if ( (scale == 1.0) && (offset == 0.0) )
	{
	    (void) sprintf (string, "value: %e", value);
	}
	else (void) sprintf (string, "raw: %e  sc: %e", value, scaled_value);
	return;
    }
    /*  Have a useful value name: first check if it should be scaled and the
	units fiddled to make (some) users happier  */
    if (value >= TOOBIG)
    {
	(void) sprintf (value_str, "blank");
    }
    else if (strncmp (value_name, "M/S", 3) == 0)
    {
	(void) sprintf (value_str, "%.1f km/s", scaled_value * 1e-3);
    }
    else if (strncmp (value_name, "KM/S", 4) == 0)
    {
	(void) sprintf (value_str, "%.1f km/s", scaled_value);
    }
    else if (strncmp (value_name, "JY/BEAM", 7) == 0)
    {
	(void) sprintf (value_str, "%.1f mJy/Beam", scaled_value * 1e+3);
    }
    else if (strncmp (value_name, "FREQ", 4) == 0)
    {
	(void) sprintf (value_str, "%.3f MHz", scaled_value * 1e-6);
    }
    else if (strncmp (value_name, "FELO", 4) == 0)
    {
	(void) sprintf (value_str, "%.2f km/s", scaled_value * 1e-3);
    }
    else if (strncmp (value_name, "VELO", 4) == 0)
    {
	(void) sprintf (value_str, "%.2f km/s", scaled_value * 1e-3);
    }
    else (void) sprintf (value_str, "%e %s", scaled_value, value_name);
    if ( (scale == 1.0) && (offset == 0.0) )
    {
	(void) sprintf (string, "value: %s", value_str);
    }
    else (void) sprintf (string, "raw: %e  sc: %s", value, value_str);
}   /*  End Function iarray_format_value  */

/*LINTLIBRARY*/
/*  misc.c

    This code provides miscellaneous data structure manipulation routines.

    Copyright (C) 1992-1996  Richard Gooch

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

/*

    This file contains the miscellaneous utility routines for the
    general data structure supported in Karma.


    Written by      Richard Gooch   2-NOV-1996

    Last updated by Richard Gooch   2-NOV-1996


*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>
#include <os.h>


/*EXPERIMENTAL_FUNCTION*/
void ds_format_unit (char unit[STRING_LENGTH], double *scale,
		     CONST char *value_name)
/*  [SUMMARY] Format a unit string.
    <unit> The unit string will be written here.
    <scale> The scale value to apply to data to convert to displayed units will
    be written here. If this is NULL nothing is written here.
    <value_name> The name of the value to format.
    [RETURNS] Nothing.
*/
{
    double multiplier = 1.0;
    static char function_name[] = "ds_format_unit";

    if ( (unit == NULL) || (value_name == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (strncmp (value_name, "M/S", 3) == 0)
    {
	strcpy (unit, "km/s");
	multiplier = 1e-3;
    }
    else if (strncmp (value_name, "KM/S", 4) == 0)
    {
	strcpy (unit, "km/s");
    }
    else if (strncmp (value_name, "JY/BEAM", 7) == 0)
    {
	strcpy (unit, "mJy/Beam");
	multiplier = 1e+3;
    }
    else if (strncmp (value_name, "FREQ", 4) == 0)
    {
	strcpy (unit, "MHz");
	multiplier = 1e-6;
    }
    else if (strcmp (value_name, "HZ") == 0)
    {
	strcpy (unit, "MHz");
	multiplier = 1e-6;
    }
    else if (strncmp (value_name, "FELO", 4) == 0)
    {
	strcpy (unit, "km/s");
	multiplier = 1e-3;
    }
    else if (strncmp (value_name, "VELO", 4) == 0)
    {
	strcpy (unit, "km/s");
	multiplier = 1e-3;
    }
    else strcpy (unit, value_name);
    if (scale != NULL) *scale = multiplier;
}   /*  End Function ds_format_unit  */

/*EXPERIMENTAL_FUNCTION*/
void ds_format_value (char string[STRING_LENGTH], double value,
		      CONST char *value_name, double scale, double offset,
		      CONST packet_desc *pack_desc, CONST char *packet)
/*  [SUMMARY] Format a data value into a string.
    <string> The string to write to.
    <value> The value to format.
    <value_name> The name of the value to format.
    <scale> The scale value to apply to the data. If this is TOOBIG and
    <<pack_desc>> and <<packet>> are non-NULL, the routine uses the scale and
    offset attached to the data. See [<ds_get_data_scaling>] for details.
    <offset> The offset to apply after scaling the data.
    <pack_desc> The packet descriptor containing associated data scaling.
    <packet> The packet containing associated data scaling.
    [RETURNS] Nothing.
*/
{
    double scaled_value, unit_scale;
    char unit_string[STRING_LENGTH];
    static char function_name[] = "ds_format_value";

    if ( (string == NULL) || (value_name == NULL) )
    {
	fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (value >= TOOBIG)
    {
	sprintf (string, "value: blank");
	return;
    }
    if (scale >= TOOBIG)
    {
	if ( (pack_desc != NULL) && (packet != NULL) )
	{
	    ds_get_data_scaling (value_name, pack_desc, packet,
				 &scale, &offset);
	}
	else
	{
	    scale = 1.0;
	    offset = 0.0;
	}
    }
    /*  Compute value  */
    scaled_value = scale * value + offset;
    if (strcmp (value_name, "Data Value") == 0)
    {
	/*  No useful unit name  */
	if ( (scale == 1.0) && (offset == 0.0) )
	{
	    sprintf (string, "value: %e", value);
	}
	else sprintf (string, "raw: %e  sc: %e", value, scaled_value);
	return;
    }
    /*  Have a useful value name: first check if it should be scaled and the
	units fiddled to make (some) users happier  */
    ds_format_unit (unit_string, &unit_scale, value_name);
    if ( (scale == 1.0) && (offset == 0.0) )
    {
	sprintf (string, "value: %e %s",
		 scaled_value * unit_scale, unit_string);
    }
    else sprintf (string, "raw: %e  sc: %e %s",
		  value, scaled_value * unit_scale, unit_string);
}   /*  End Function ds_format_value  */

/*LINTLIBRARY*/
/*  track.c

    This code provides ViewableImage objects.

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

/*  This file contains convenience routines to support track displays.


    Written by      Richard Gooch   3-NOV-1996

    Last updated by Richard Gooch   3-NOV-1996


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <karma.h>
#define KWIN_GENERIC_ONLY
#define NEW_WIN_SCALE
#include <karma_viewimg.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_a.h>


/*  Private structures  */


/*  Private functions  */


/*  Public functions follow  */

/*EXPERIMENTAL_FUNCTION*/
void viewimg_track_compute (ViewableImage vimage,
			    void *value, unsigned int value_type,
			    double x, double y,
			    double x_lin, double y_lin, KwcsAstro ap,
			    char pix_string[STRING_LENGTH],
			    char world_string[STRING_LENGTH],
			    char extra_string[STRING_LENGTH],
			    unsigned long *x_index, unsigned long *y_index)
/*  [SUMMARY] Compute track strings.
    <vimage> The active ViewableImage object.
    <value> A pointer to the data value in the viewable image corresponding
    to the event co-ordinates.
    <value_type> The type of the data value. This may be K_DCOMPLEX or
    K_UB_RGB.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    <x_lin> The linear horizontal world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <y_lin> The linear vertical world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <ap> The KwcsAstro object. This may be NULL.
    <pix_string> The pixel co-ordinates and data value will be written here.
    <world_string> The world co-ordinates and restriction information will be
    written here.
    <extra_string> Extra information is written here. This may be NULL if
    <<world_string>> is also NULL.
    <x_index> The horizontal data co-ordinate index is written here. This may
    be NULL
    <y_index> The vertical data co-ordinate index is written here. This may be
    NULL
    [RETURNS] Nothing.
*/
{
    unsigned int hdim, vdim, rdim, elem_index;
    unsigned int num_restr, count;
    unsigned long px, py, r_index;
    double scale, offset;
    unsigned char *rgb_ptr = (unsigned char *) value;
    char *xlabel, *ylabel;
    char **restr_names;
    double *restr_values;
    array_desc *arr_desc;
    char txt[STRING_LENGTH], value_string[STRING_LENGTH];
    static char function_name[] = "viewimg_track_compute";

    viewimg_get_attributes (vimage,
			    VIEWIMG_VATT_ARRAY_DESC, &arr_desc,
			    VIEWIMG_VATT_HDIM, &hdim,
			    VIEWIMG_VATT_VDIM, &vdim,
			    VIEWIMG_VATT_END);
    switch (value_type)
    {
      case K_DCOMPLEX:
	/*  Compute value  */
	viewimg_get_attributes (vimage,
				VIEWIMG_VATT_PSEUDO_INDEX, &elem_index,
				VIEWIMG_VATT_DATA_SCALE, &scale,
				VIEWIMG_VATT_DATA_OFFSET, &offset,
				VIEWIMG_VATT_END);
	ds_format_value (value_string, *(double *) value,
			 arr_desc->packet->element_desc[elem_index],
			 scale, offset, NULL, NULL);
	break;
      case K_UB_RGB:
	sprintf (value_string, "RGB: %u %u %u",
		 rgb_ptr[0], rgb_ptr[1], rgb_ptr[2]);
	break;
      default:
	fprintf (stderr, "Illegal type: %u\n", value_type);
	a_prog_bug (function_name);
	break;
    }
    canvas_get_specification (viewimg_get_worldcanvas (vimage),
			      &xlabel, &ylabel, &num_restr,
			      &restr_names, &restr_values);
    /*  Convert linear world co-ordinates to array indices and display  */
    px = ds_get_coord_num (arr_desc->dimensions[hdim], x_lin,
			   SEARCH_BIAS_CLOSEST);
    py = ds_get_coord_num (arr_desc->dimensions[vdim], y_lin,
			   SEARCH_BIAS_CLOSEST);
    sprintf (pix_string, "x: %lu  y: %lu  ", px, py);
    if (x_index != NULL) *x_index = px;
    if (y_index != NULL) *y_index = py;
    /*  Add any restriction information  */
    for (count = 0; count < num_restr; ++count)
    {
	if ( ( rdim = ds_f_dim_in_array (arr_desc, restr_names[count]) )
	     >= arr_desc->num_dimensions ) continue;
	r_index = ds_get_coord_num (arr_desc->dimensions[rdim],
				    restr_values[count], SEARCH_BIAS_CLOSEST);
	sprintf (txt, "z%u: %lu  ", rdim, r_index);
	strcat (pix_string, txt);
    }
    strcat (pix_string, value_string);
    if ( (world_string == NULL) && (extra_string == NULL) ) return;
    /*  Now display the world co-ordinate information  */
    if (ap == NULL)
    {
	sprintf (world_string, "%5e %s  %5e %s  ", x, xlabel, y, ylabel);
	/*  Add any restriction information  */
	for (count = 0; count < num_restr; ++count)
	{
	    sprintf (txt, "%5e %s  ", restr_values[count], restr_names[count]);
	    strcat (world_string, txt);
	}
	if (extra_string != NULL) extra_string[0] = '\0';
    }
    else
    {
	wcs_astro_format_all (ap, world_string,
			      xlabel, x_lin, ylabel, y_lin, NULL, 0.0,
			      num_restr, (CONST char **) restr_names,
			      restr_values, extra_string);
    }
}   /*  End Function viewimg_track_compute  */

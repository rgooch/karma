/*LINTLIBRARY*/
/*  statistics.c

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

/*  This file contains convenience routines which implement a statistics
  gathering user interface.


    Written by      Richard Gooch   22-OCT-1996

    Updated by      Richard Gooch   27-OCT-1996: Created
  <viewimg_statistics_compute> routine.

    Last updated by Richard Gooch   3-NOV-1996: Incorporated data and unit
  scaling.


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#define KWIN_GENERIC_ONLY
#define NEW_WIN_SCALE
#include <k_event_codes.h>
#include <karma_viewimg.h>
#include <karma_ds.h>
#include <karma_a.h>


#define swap(a,b) {tmp = a; a = b; b = tmp;}


/*  Private structures  */


/*  Private functions  */


/*  Public functions follow  */

/*EXPERIMENTAL_FUNCTION*/
flag viewimg_statistics_position_func (ViewableImage vimage,
				       double x, double y,
				       void *value, unsigned int event_code,
				       void *e_info, void **f_info,
				       double x_lin, double y_lin,
				       unsigned int value_type)
/*  [SUMMARY] Position event callback.
    [PURPOSE] This routine is a position event consumer for a world canvas
    which has a number of ViewableImage objects associated with it.
    <viewimg> The active viewable image.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    These values will have been transformed by the registered transform
    function (see [<canvas_register_transform_func>]) for the associated
    world canvas.
    <value> A pointer to the data value in the viewable image corresponding
    to the event co-ordinates.
    <event_code> The arbitrary event code.
    <e_info> The arbitrary event information.
    <f_info> The arbitrary function information pointer.
    <x_lin> The linear horizontal world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <y_lin> The linear vertical world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <value_type> The type of the data value. This may be K_DCOMPLEX or
    K_UB_RGB.
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    KWorldCanvas canvas;
    double left_x, right_x, bottom_y, top_y;
    CONST char *string = e_info;
    /*static char function_name[] = "viewimg_statistics_position_func";*/

    if (event_code != K_CANVAS_EVENT_PLAIN_KEY_PRESS) return (FALSE);
    if (strcmp (string, "s") != 0) return (FALSE);
    if (value_type != K_DCOMPLEX) return (FALSE);
    /*  Determine start and stop co-ordinates along each dimension  */
    canvas = viewimg_get_worldcanvas (vimage);
    canvas_get_attributes (canvas,
			   CANVAS_ATT_LEFT_X, &left_x,
			   CANVAS_ATT_RIGHT_X, &right_x,
			   CANVAS_ATT_BOTTOM_Y, &bottom_y,
			   CANVAS_ATT_TOP_Y, &top_y,
			   CANVAS_ATT_END);
    viewimg_statistics_compute (vimage, left_x, bottom_y, right_x, top_y);
    return (TRUE);
}   /*  End Function viewimg_statistics_position_func  */

/*EXPERIMENTAL_FUNCTION*/
flag viewimg_statistics_compute (ViewableImage vimage,
				 double lx0, double ly0, double lx1,double ly1)
/*  [SUMMARY] Compute and display statistics for a subimage.
    <vimage> The active viewable image.
    <lx0> The first horizontal linear world co-ordinate.
    <ly0> The first vertical linear world co-ordinate.
    <lx1> The second horizontal linear world co-ordinate.
    <ly1> The second vertical linear world co-ordinate.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int hstart, hend, hlength, vstart, vend, vlength, tmp;
    unsigned int hdim, vdim, elem_index;
    unsigned long npoints;
    double min, max, mean, stddev, sum, sumsq, scale, offset, unit_scale;
    char *array;
    uaddr *hoffsets, *voffsets;
    array_desc *arr_desc;
    packet_desc *pack_desc;
    dim_desc *hdim_desc, *vdim_desc;
    char unit[STRING_LENGTH];

    viewimg_get_attributes (vimage,
			    VIEWIMG_VATT_ARRAY_DESC, &arr_desc,
			    VIEWIMG_VATT_SLICE, &array,
			    VIEWIMG_VATT_HDIM, &hdim,
			    VIEWIMG_VATT_VDIM, &vdim,
			    VIEWIMG_VATT_PSEUDO_INDEX, &elem_index,
			    VIEWIMG_VATT_DATA_SCALE, &scale,
			    VIEWIMG_VATT_DATA_OFFSET, &offset,
			    VIEWIMG_VATT_END);
    hoffsets = arr_desc->offsets[hdim];
    voffsets = arr_desc->offsets[vdim];
    hdim_desc = arr_desc->dimensions[hdim];
    vdim_desc = arr_desc->dimensions[vdim];
    pack_desc = arr_desc->packet;
    /*  Determine start and stop co-ordinates along each dimension  */
    hstart = ds_get_coord_num (hdim_desc, lx0, SEARCH_BIAS_CLOSEST);
    hend = ds_get_coord_num (hdim_desc, lx1, SEARCH_BIAS_CLOSEST);
    if (hstart >= hend) swap (hstart, hend);
    hlength = hend - hstart + 1;
    vstart = ds_get_coord_num (vdim_desc, ly0, SEARCH_BIAS_CLOSEST);
    vend = ds_get_coord_num (vdim_desc, ly1, SEARCH_BIAS_CLOSEST);
    if (vstart >= vend) swap (vstart, vend);
    vlength = vend - vstart + 1;
    array += ds_get_element_offset (pack_desc, elem_index);
    min = TOOBIG;
    max = -TOOBIG;
    if ( !ds_find_2D_stats (array, vlength, voffsets + vstart,
			    hlength, hoffsets + hstart,
			    pack_desc->element_types[elem_index],
			    CONV_CtoR_REAL,
			    &min, &max, &mean, &stddev, &sum, &sumsq,
			    &npoints) )
	return (FALSE);
    /*  Apply data scaling to the results  */
    min = min * scale + offset;
    max = max * scale + offset;
    sumsq = scale * scale * sumsq + 2.0 * scale * offset * sum +
	(double) npoints * offset * offset;
    mean = scale * sum / (double) npoints + offset;
    sum = scale * sum + (double) npoints * offset;
    stddev = sqrt (sumsq / (double) npoints - mean * mean);
    /*  Apply unit scale to the results  */
    ds_format_unit (unit, &unit_scale, pack_desc->element_desc[elem_index]);
    min *= unit_scale;
    max *= unit_scale;
    sumsq *= unit_scale * unit_scale;
    mean *= unit_scale;
    sum *= unit_scale;
    stddev *= unit_scale;
    fprintf
	(stderr,
	 "npoints  mean %-8s std dev       min           max           sum\n",
	 unit);
    fprintf (stderr, "%-8lu %-+11e %-+11e %-+11e %-+11e %+e\n",
	     npoints, mean, stddev, min, max, sum);
    return (TRUE);
}   /*  End Function viewimg_statistics_compute  */

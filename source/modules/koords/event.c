/*  event.c

    Event file for  koords  (X11 co-ordinate generator tool for Karma).

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

/*
    This Karma module will enable interactive generation of an astronomical
    co-ordinate system on a target image, using a reference image.
    This module runs on an X11 server.


    Written by      Richard Gooch   14-OCT-1996: Copied from kshell/main_xt.c

    Updated by      Richard Gooch   17-OCT-1996: Completed centroid hunter.

    Last updated by Richard Gooch   21-OCT-1996: Made <get_colour> public.
  Created <update_magnifier>.


*/

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <karma.h>
#include <k_event_codes.h>
#define KWIN_GENERIC_ONLY
#include <karma_viewimg.h>
#include <karma_iarray.h>
#include <karma_canvas.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>
#include "koords.h"


#define ELLIPSE_RADIUS 5.0
#define CURSOR_RADIUS 8.0


/*  Private functions  */
STATIC_FUNCTION (unsigned long get_colour,
		 (KWorldCanvas canvas, CONST char *colourname) );
STATIC_FUNCTION (void find_object,
		 (KWorldCanvas canvas, iarray array,
		  double *xpos, double *ypos, double cursor_radius) );
STATIC_FUNCTION (double get_value, (iarray array, int y, int x) );
STATIC_FUNCTION (void find_peak, (iarray array, int *xpos, int *ypos,
				  double radius_x, double radius_y) );
STATIC_FUNCTION (void walk_up_hill, (iarray array, int *xpos, int *ypos) );
STATIC_FUNCTION (int find_size,
		 (iarray array, int xpos, int ypos, double threshold) );
STATIC_FUNCTION (void find_centroid, (iarray array, double *xpos, double *ypos,
				      double max_radius) );


/*  Private data  */


/*  Public functions follow  */

flag reference_event_func (ViewableImage vimage, double x, double y,
			   void *value, unsigned int event_code,
			   void *e_info, void **f_info,
			   double x_lin, double y_lin, unsigned int value_type)
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
    KWorldCanvas wc;
    unsigned int index;
    extern iarray ref_array;
    extern KwcsAstro reference_ap;
    extern flag last_click_was_reference;
    extern unsigned int num_reference_points;
    extern double reference_ra[MAX_PAIRS], reference_dec[MAX_PAIRS];
    /*static char function_name[] = "reference_event_func";*/

    if (event_code != K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK) return (FALSE);
    wc = viewimg_get_worldcanvas (vimage);
    find_object (wc, ref_array, &x_lin, &y_lin, CURSOR_RADIUS);
    x = x_lin;
    y = y_lin;
    wcs_astro_transform (reference_ap, 1,
			 &x, FALSE, &y, FALSE, NULL, FALSE,
			 0, NULL, NULL);
    index = num_reference_points;
    if (last_click_was_reference) --index;
    reference_ra[index] = x;
    reference_dec[index] = y;
    if (!last_click_was_reference) ++num_reference_points;
    last_click_was_reference = TRUE;
    kwin_resize (canvas_get_pixcanvas (wc), FALSE, 0, 0, 0, 0);
    return (TRUE);
}   /*  End Function reference_event_func  */

flag target_event_func (ViewableImage vimage, double x, double y,
			void *value, unsigned int event_code,
			void *e_info, void **f_info,
			double x_lin, double y_lin, unsigned int value_type)
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
    KWorldCanvas wc;
    extern iarray tar_array;
    extern flag last_click_was_reference;
    extern unsigned int num_reference_points;
    extern unsigned int num_target_points;
    extern double target_x[MAX_PAIRS], target_y[MAX_PAIRS];
    /*static char function_name[] = "target_event_func";*/

    if (event_code != K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK) return (FALSE);
    if (last_click_was_reference)
    {
	last_click_was_reference = FALSE;
    }
    else
    {
	if (num_reference_points < 1)
	{
	    ring_bell ();
	    return (TRUE);
	}
    }
    wc = viewimg_get_worldcanvas (vimage);
    find_object (wc, tar_array, &x_lin, &y_lin, CURSOR_RADIUS);
    num_target_points = num_reference_points;
    target_x[num_target_points - 1] = x_lin;
    target_y[num_target_points - 1] = y_lin;
    kwin_resize (canvas_get_pixcanvas (wc), FALSE, 0, 0, 0, 0);
    return (TRUE);
}   /*  End Function target_event_func  */

void ref_refresh_func (KWorldCanvas canvas, int width, int height,
		       struct win_scale_type *win_scale,
		       Kcolourmap cmap, flag cmap_resize, void **info,
		       PostScriptPage pspage,
		       unsigned int num_areas,
		       KPixCanvasRefreshArea *areas, flag *honoured_areas)
/*  [SUMMARY] Refresh event callback.
    [PURPOSE] This routine is a refresh event consumer for a world canvas.
    <canvas> The world canvas being refreshed.
    <width> The width of the canvas in pixels.
    <height> The height of the canvas in pixels.
    <win_scale> A pointer to the window scaling information.
    <cmap> The colourmap associated with the canvas.
    <cmap_resize> TRUE if the refresh function was called as a result of a
    colourmap resize, else FALSE.
    <info> A pointer to the arbitrary canvas information pointer.
    <pspage> If not NULL, the PostScriptPage object the refresh is
    redirected to.
    <num_areas> The number of areas that need to be refreshed. If this is
    0 then the entire pixel canvas needs to be refreshed.
    <areas> The list of areas that need to be refreshed. Note that these
    areas are given in pixel co-ordinates.
    <honoured_areas> If the value TRUE is written here it is assumed the
    routine honoured the list of refresh areas and did not write outside
    these areas and hence the list of areas will be passed to subsequent
    registered refresh routines. If FALSE is written here (or nothing is
    written here), implying the routine refreshed the entire canvas,
    subsequent refresh routines will be told to refresh the entire canvas.
    [RETURNS] Nothing.
*/
{
    KPixCanvas pixcanvas;
    unsigned int count;
    unsigned long pixel_value;
    char txt[STRING_LENGTH];
    double px[MAX_PAIRS], py[MAX_PAIRS];
    extern unsigned int num_reference_points;
    extern double reference_ra[MAX_PAIRS], reference_dec[MAX_PAIRS];

    pixel_value = get_colour (canvas, "green");
    pixcanvas = canvas_get_pixcanvas (canvas);
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE,
				       num_reference_points,
				       reference_ra, reference_dec, px, py);
    for (count = 0; count < num_reference_points; ++count)
    {
	kwin_draw_ellipse (pixcanvas, px[count], py[count],
			   ELLIPSE_RADIUS, ELLIPSE_RADIUS, pixel_value);
	sprintf (txt, "%u", count);
	kwin_draw_string (pixcanvas, px[count], py[count] - ELLIPSE_RADIUS * 2,
			  txt, pixel_value, FALSE);
    }
}   /*  End Function ref_refresh_func  */

void tar_refresh_func (KWorldCanvas canvas, int width, int height,
		       struct win_scale_type *win_scale,
		       Kcolourmap cmap, flag cmap_resize, void **info,
		       PostScriptPage pspage,
		       unsigned int num_areas,
		       KPixCanvasRefreshArea *areas, flag *honoured_areas)
/*  [SUMMARY] Refresh event callback.
    [PURPOSE] This routine is a refresh event consumer for a world canvas.
    <canvas> The world canvas being refreshed.
    <width> The width of the canvas in pixels.
    <height> The height of the canvas in pixels.
    <win_scale> A pointer to the window scaling information.
    <cmap> The colourmap associated with the canvas.
    <cmap_resize> TRUE if the refresh function was called as a result of a
    colourmap resize, else FALSE.
    <info> A pointer to the arbitrary canvas information pointer.
    <pspage> If not NULL, the PostScriptPage object the refresh is
    redirected to.
    <num_areas> The number of areas that need to be refreshed. If this is
    0 then the entire pixel canvas needs to be refreshed.
    <areas> The list of areas that need to be refreshed. Note that these
    areas are given in pixel co-ordinates.
    <honoured_areas> If the value TRUE is written here it is assumed the
    routine honoured the list of refresh areas and did not write outside
    these areas and hence the list of areas will be passed to subsequent
    registered refresh routines. If FALSE is written here (or nothing is
    written here), implying the routine refreshed the entire canvas,
    subsequent refresh routines will be told to refresh the entire canvas.
    [RETURNS] Nothing.
*/
{
    KPixCanvas pixcanvas;
    unsigned int count;
    unsigned long pixel_value;
    char txt[STRING_LENGTH];
    double px[MAX_PAIRS], py[MAX_PAIRS];
    extern unsigned int num_target_points;
    extern double target_x[MAX_PAIRS], target_y[MAX_PAIRS];

    pixel_value = get_colour (canvas, "green");
    pixcanvas = canvas_get_pixcanvas (canvas);
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, num_target_points,
				       target_x, target_y, px, py);
    for (count = 0; count < num_target_points; ++count)
    {
	kwin_draw_ellipse (pixcanvas, px[count], py[count],
			   ELLIPSE_RADIUS, ELLIPSE_RADIUS, pixel_value);
	sprintf (txt, "%u", count);
	kwin_draw_string (pixcanvas, px[count], py[count] - ELLIPSE_RADIUS * 2,
			  txt, pixel_value, FALSE);
    }
}   /*  End Function tar_refresh_func  */

void update_magnifier (KWorldCanvas canvas, double x, double y,
		       KWorldCanvas main_canvas)
/*  [SUMMARY] Update a magnifier canvas.
    <canvas> The magnifier canvas.
    <x> The new centre horizontal linear co-ordinate.
    <y> The new centre vertical linear co-ordinate.
    <main_canvas> The main canvas.
    [RETURNS] Nothing.
*/
{
    ViewableImage vimage;
    KPixCanvas pixcanvas;
    flag visible;
    int width, height;
    unsigned int hdim, vdim;
    unsigned long pointer_x_index, pointer_y_index;
    unsigned long pixel_value;
    double px, py, rx, ry;
    array_desc *arr_desc;

    pixcanvas = canvas_get_pixcanvas (canvas);
    kwin_get_attributes (pixcanvas,
			 KWIN_ATT_VISIBLE, &visible,
			 KWIN_ATT_END);
    if (!visible) return;
    vimage = viewimg_get_active (canvas);
    if (vimage == NULL) return;
    viewimg_get_attributes (vimage,
			    VIEWIMG_VATT_ARRAY_DESC, &arr_desc,
			    VIEWIMG_VATT_HDIM, &hdim,
			    VIEWIMG_VATT_VDIM, &vdim,
			    VIEWIMG_VATT_END);
    /*  Convert linear world co-ordinates to array indices and display  */
    pointer_x_index = ds_get_coord_num (arr_desc->dimensions[hdim], x,
					SEARCH_BIAS_CLOSEST);
    pointer_y_index = ds_get_coord_num (arr_desc->dimensions[vdim], y,
					SEARCH_BIAS_CLOSEST);
    viewimg_set_canvas_attributes (canvas,
				   VIEWIMG_ATT_PAN_CENTRE_X, pointer_x_index,
				   VIEWIMG_ATT_PAN_CENTRE_Y, pointer_y_index,
				   VIEWIMG_ATT_END);
    kwin_resize (pixcanvas, FALSE, 0, 0, 0, 0);
    canvas_convert_from_canvas_coords (canvas, FALSE, TRUE, 1,
				       &x, &y, &px, &py);
    pixel_value = get_colour (canvas, "green");
    kwin_get_size (pixcanvas, &width, &height);
    kwin_draw_line (pixcanvas, 0, py, width - 1, py, pixel_value);
    kwin_draw_line (pixcanvas, px, 0, px, height - 1, pixel_value);
    /*  Compute size of circle cursor in main canvas and draw here  */
    canvas_convert_from_canvas_coords (main_canvas, FALSE, TRUE, 1,
				       &x, &y, &rx, &ry);
    rx += CURSOR_RADIUS;
    ry -= CURSOR_RADIUS;
    canvas_convert_to_canvas_coords (main_canvas, FALSE, 1, &rx, &ry, &rx, &ry,
				     NULL, NULL);
    rx = fabs (rx - x);
    ry = fabs (ry - y);
    canvas_draw_ellipse_p (canvas, x, y, rx, ry, pixel_value);
}   /*  End Function update_magnifier  */


/*  Private functions follow  */

static unsigned long get_colour (KWorldCanvas canvas, CONST char *colourname)
/*  [SUMMARY] Get colour.
    <canvas> The world canvas.
    <colourname> The name of the colour.
    [RETURNS] The specified colour if available, else a default colour.
*/
{
    unsigned long pixel_value;

    if ( canvas_get_colour (canvas, colourname, &pixel_value, NULL,NULL,NULL) )
    {
	return (pixel_value);
    }
    if ( canvas_get_colour (canvas, "white", &pixel_value, NULL, NULL, NULL) )
    {
	fprintf (stderr,
		 "Error allocating colour: \"%s\" defaulting to white\n",
		 colourname);
	return (pixel_value);
    }
    fprintf (stderr, "Error allocating colour: \"%s\" defaulting to 0\n",
	     colourname);
    return (0);
}   /*  End Function get_colour  */

static void find_object (KWorldCanvas canvas, iarray array,
			 double *xpos, double *ypos, double cursor_radius)
/*  [SUMMARY] Find the nearby object.
    [PURPOSE] This routine will search for the centroid of an object near to a
    specified pixel position.
    <canvas> The KWorldCanvas in which the event occurred.
    <array> The Intelligent Array.
    <xpos> The horizontal data pixel position. This is modified to the centroid
    position.
    <ypos> The vertical data pixel position. This is modified to the centroid
    position.
    <cursor_radius> The radius of the cursor in screen pixels.
    [NOTE] (0.0, 0.0) corresponds to the centre of the first pixel.
    [RETURNS] Nothing.
*/
{
    int ix, iy, size;
    double px, py, radius_x, radius_y;
    double value;

    /*  Determine size of cursor in data pixels (same scale as linear world
	co-ordinates)  */
    px = *xpos;
    py = *ypos;
    canvas_convert_from_canvas_coords (canvas, FALSE, TRUE, 1,
				       &px, &py, &px, &py);
    px += cursor_radius;
    py += cursor_radius;
    canvas_convert_to_canvas_coords (canvas, FALSE, 1, &px, &py, &px, &py,
				     NULL, NULL);
    radius_x = fabs (px - *xpos);
    radius_y = fabs (py - *ypos);
    /*  Find peak with this ellipse  */
    ix = *xpos;
    iy = *ypos;
    find_peak (array, &ix, &iy, radius_x, radius_y);
    walk_up_hill (array, &ix, &iy);
    value = get_value (array, iy, ix);
    size = find_size (array, ix, iy, value * 0.5);
    *xpos = ix;
    *ypos = iy;
    find_centroid (array, xpos, ypos, size);
}   /*  End Function find_object  */

static double get_value (iarray array, int y, int x)
/*  [SUMMARY] Get value from a 2-dimensional Intelligent Array.
    <array> The Intelligent Array.
    <y> The vertical pixel position.
    <x> The horizontal pixel position.
    [RETURNS] The pixel value.
*/
{
    int xlen, ylen;
    unsigned int type;
    double value[2];
    char *ptr;

    xlen = iarray_dim_length (array, 1);
    ylen = iarray_dim_length (array, 0);
    if (x < 0) x = 0;
    else if (x >= xlen) x = xlen - 1;
    if (y < 0) y = 0;
    else if (y >= ylen) y = ylen - 1;
    type = iarray_type (array);
    ptr = iarray_get_element_2D (array, type, y, x);
    ds_get_element (ptr, type, value, NULL);
    return (value[0]);
}   /*  End Function get_value  */

static void find_peak (iarray array, int *xpos, int *ypos,
		       double radius_x, double radius_y)
/*  [SUMMARY] Find the centroid of a feature.
    [PURPOSE] This routine will search for the peak of the nearest object
    to a specified pixel position. It does this by finding the largest value
    inside an ellipse.
    <array> The Intelligent Array.
    <xpos> The horizontal pixel position. This is modified to the centroid
    position.
    <ypos> The vertical pixel position. This is modified to the centroid
    position.
    <radius_x> The horizontal radius of the ellipse.
    <radius_y> The vertical radius of the ellipse.
    [NOTE] (0.0, 0.0) corresponds to the centre of the first pixel.
    [RETURNS] Nothing.
*/
{
    int x, y, old_x, old_y;
    double distance, tmp;
    double value, old_value;

    old_x = *xpos;
    old_y = *ypos;
    old_value = get_value (array, old_y, old_x);
    for (y = *ypos - (int) radius_y; y < *ypos + (int) radius_y; ++y)
    {
	for (x = *xpos - (int) radius_x; x < *xpos + (int) radius_x; ++x)
	{
	    tmp = x - *xpos;
	    distance = tmp * tmp / (radius_x * radius_x);
	    tmp = y - *ypos;
	    distance += tmp * tmp / (radius_y * radius_y);
	    if (distance > 1.0) continue;
	    if ( ( value = get_value (array, y, x) ) <= old_value ) continue;
	    old_x = x;
	    old_y = y;
	    old_value = value;
	}
    }
    *xpos = old_x;
    *ypos = old_y;
}   /*  End Function find_peak  */

static void walk_up_hill (iarray array, int *xpos, int *ypos)
/*  [SUMMARY] Find a nearby peak.
    [PURPOSE] This routine will search for the peak of the nearest object
    to a specified pixel position. It does this by walking "up the hill" from
    an initial point until it can go no higher.
    <array> The Intelligent Array.
    <xpos> The horizontal pixel position. This is modified to the centroid
    position.
    <ypos> The vertical pixel position. This is modified to the centroid
    position.
    [NOTE] (0.0, 0.0) corresponds to the centre of the first pixel.
    [RETURNS] Nothing.
*/
{
    flag keep_going = TRUE;
    int x, y, old_x, old_y;
    double value, old_value, tmp;

    /*  Set initial position  */
    old_x = *xpos;
    old_y = *ypos;
    old_value = get_value (array, old_y, old_x);
    while (keep_going)
    {
	x = old_x;
	y = old_y;
	value = old_value;
	if ( ( tmp = get_value (array, old_y - 1, old_x - 1) ) > value )
	{
	    x = old_x - 1;
	    y = old_y - 1;
	    value = tmp;
	}
	if ( ( tmp = get_value (array, old_y - 1, old_x) ) > value )
	{
	    x = old_x;
	    y = old_y - 1;
	    value = tmp;
	}
	if ( ( tmp = get_value (array, old_y - 1, old_x + 1) ) > value )
	{
	    x = old_x + 1;
	    y = old_y - 1;
	    value = tmp;
	}
	if ( ( tmp = get_value (array, old_y, old_x - 1) ) > value )
	{
	    x = old_x - 1;
	    y = old_y;
	    value = tmp;
	}
	if ( ( tmp = get_value (array, old_y, old_x + 1) ) > value )
	{
	    x = old_x + 1;
	    y = old_y;
	    value = tmp;
	}
	if ( ( tmp = get_value (array, old_y + 1, old_x - 1) ) > value )
	{
	    x = old_x - 1;
	    y = old_y + 1;
	    value = tmp;
	}
	if ( ( tmp = get_value (array, old_y + 1, old_x) ) > value )
	{
	    x = old_x;
	    y = old_y + 1;
	    value = tmp;
	}
	if ( ( tmp = get_value (array, old_y + 1, old_x + 1) ) > value )
	{
	    x = old_x + 1;
	    y = old_y + 1;
	    value = tmp;
	}
	if (value > old_value)
	{
	    old_x = x;
	    old_y = y;
	    old_value = value;
	}
	else keep_going = FALSE;
    }
    *xpos = old_x;
    *ypos = old_y;
}   /*  End Function walk_up_hill  */

static int find_size (iarray array, int xpos, int ypos, double threshold)
/*  [SUMMARY] Find radial size of an object.
    [PURPOSE] This routine will find the size of an object. It does this by
    constructing circles of ever increasing radius and checking if all values
    just outside that circle lie below a threshold. If all values just outside
    do fall below the threshold, the radius of that circle is deemed to be the
    radial size of the object.
    <array> The Intelligent Array.
    <xpos> The horizontal pixel position.
    <ypos> The vertical pixel position.
    <threshold> The threshold value.
    [NOTE] (0.0, 0.0) corresponds to the centre of the first pixel.
    [RETURNS] The radial size.
*/
{
    flag keep_going = TRUE;
    int radius, sq_radius, radiusP1, sq_radiusP1;
    int x, y, sq_distance;
    double value;

    for (radius = 1; keep_going; ++radius)
    {
	sq_radius = radius * radius;
	radiusP1 = radius + 1;
	sq_radiusP1 = radiusP1 * radiusP1;
	keep_going = FALSE;
	/*  Scan through a box just bigger than the circle  */
	for (y = ypos - radiusP1; y < ypos + radiusP1; ++y)
	{
	    for (x = xpos - radiusP1; x < xpos + radiusP1; ++x)
	    {
		sq_distance = (x - xpos) * (x - xpos) + (y - ypos) * (y -ypos);
		if (sq_distance < sq_radius) continue;
		if (sq_distance > sq_radiusP1) continue;
		/*  Should be just outside the circle  */
		value = get_value (array, y, x);
		if (value > threshold) keep_going = TRUE;
	    }
	}
	if (radius > 50)
	{
	    fprintf (stderr, "Enormous object: setting size to 50\n");
	    return (radius);
	}
    }
    fprintf (stderr, "radius: %d\n", radius);
    return (radius);
}   /*  End Function find_size  */

static void find_centroid (iarray array, double *xpos, double *ypos,
			   double max_radius)
/*  [SUMMARY] Find the centroid of a feature.
    [PURPOSE] This routine will search for the centroid of an object. It does
    this by computing weighted means.
    <array> The Intelligent Array.
    <xpos> The horizontal data pixel position. This is modified to the centroid
    position.
    <ypos> The vertical data pixel position. This is modified to the centroid
    position.
    <max_radius> The maximum radius the computation is performed to.
    [NOTE] (0.0, 0.0) corresponds to the centre of the first pixel.
    [RETURNS] Nothing.
*/
{
    int startx, stopx, x, starty, stopy, y;
    double dx, dy, sq_radius, sq_distance;
    double weighted_sum_x, weighted_sum_y, sum, value;

    startx = *xpos - max_radius;
    stopx = *xpos + max_radius + 1.0;
    starty = *ypos - max_radius;
    stopy = *ypos + max_radius + 1.0;
    sq_radius = max_radius * max_radius;
    weighted_sum_x = 0.0;
    weighted_sum_y = 0.0;
    sum = 0.0;
    for (y = starty; y < stopy; ++y) for (x = startx; x < stopx; ++x)
    {
	dx = x - *xpos;
	dy = y - *ypos;
	sq_distance = dx * dx + dy * dy;
	if (sq_distance > sq_radius) continue;
	value = get_value (array, y, x);
	weighted_sum_x += dx * value;
	weighted_sum_y += dy * value;
	sum += value;
    }
    *xpos += weighted_sum_x / sum;
    *ypos += weighted_sum_y / sum;
}   /*  End Function find_centroid  */

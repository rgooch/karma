/*  event.c

    Event file for  kshell  (X11 ellipse integrator tool for Karma).

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
    This Karma module will enable interactive integration along the
    circumference of concentric ellipses.
    This module runs on an X11 server.


    Written by      Richard Gooch   24-SEP-1996: Copied from kpvslice module.

    Updated by      Richard Gooch   26-SEP-1996: Setup frequency/velocity
  co-ordinates.

    Updated by      Richard Gooch   28-SEP-1996: Setup ANGLE co-ordinates.

    Last updated by Richard Gooch   2-OCT-1996: Made <compute_ellipses>
  tolerate missing "CTYPEn" keyword for cube z axis and more robust trapping of
  too small radii. Don't apply cos (dec) correction to horizontal axis if a
  projection object is not available.


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
#include "kshell.h"


#define ELLIPSE_RADIUS 5


/*  Private functions  */
STATIC_FUNCTION (void compute_area_from_ellipse,
		 (KWorldCanvas canvas, KPixCanvasRefreshArea *area,
		  double centre_x, double centre_y, double radius) );
STATIC_FUNCTION (unsigned long get_colour,
		 (KWorldCanvas canvas, CONST char *colourname) );
STATIC_FUNCTION (void integrate_ellipses,
		 (KWorldCanvas canvas, double centre_x, double centre_y,
		  double radius) );


/*  Private data  */
static double centre_x = TOOBIG; /*  Proper world co-ordinates  */
static double centre_y = TOOBIG; /*  Proper world co-ordinates  */
static double radius;
static iarray image_arr = NULL;


/*  Public functions follow  */

flag endpoint_position_func (KWorldCanvas canvas, double x, double y,
			     unsigned int event_code, void *e_info,
			     void **f_info, double x_lin, double y_lin)
/*  [SUMMARY] Position event callback.
    [PURPOSE] This routine is a position event consumer for a world canvas.
    <canvas> The canvas on which the event occurred.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    [NOTE] These values will have been transformed by the registered
    transform function (see [<canvas_register_transform_func>]).
    <event_code> The arbitrary event code.
    <e_info> A pointer to arbitrary event information.
    <f_info> A pointer to an arbitrary function information pointer.
    <x_lin> The horizontal linear world co-ordinate prior to the transform
    function being called.
    <y_lin> The vertical linear world co-ordinate prior to the transform
    function being called.
    
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    KWorldCanvas aux_canvas = (KWorldCanvas) *f_info;
    KPixCanvasRefreshArea area;
    unsigned long pixel_value;
    double tmp;
    char window_string[STRING_LENGTH];
    char ra_str[STRING_LENGTH], dec_str[STRING_LENGTH];
    extern KwcsAstro main_ap;
    extern iarray image_arr, cube_arr;
    /*static char function_name[] = "endpoint_position_func";*/

    if (centre_x < TOOBIG)
    {
	/*  Compute area to be refreshed when old line is to be erased  */
	compute_area_from_ellipse (canvas, &area, centre_x, centre_y, radius);
    }
    else
    {
	area.startx = 0;
	area.endx = -1;
	area.starty = 0;
	area.endy = -1;
	area.clear = FALSE;
    }
    switch (event_code)
    {
      case K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK:
	/*  Erase the old ellipse  */
	centre_x = TOOBIG;
	viewimg_partial_refresh (canvas, 1, &area);
	centre_x = x;
	centre_y = y;
	return (TRUE);
	/*break;*/
      case K_CANVAS_EVENT_MIDDLE_MOUSE_DRAG:
      case K_CANVAS_EVENT_MIDDLE_MOUSE_RELEASE:
	tmp = x - centre_x;
	if (main_ap != NULL) tmp *= cos (centre_y * PION180);
	radius = sqrt ( tmp * tmp + (y - centre_y) * (y - centre_y) );
	break;
      default:
	return (FALSE);
	/*break;*/
    }
    pixel_value = get_colour (canvas, "green");
    /*  Force a refresh, which will in turn draw the ellipse  */
    viewimg_partial_refresh (canvas, 1, &area);
    if (event_code != K_CANVAS_EVENT_MIDDLE_MOUSE_RELEASE) return (TRUE);
    if (main_ap != NULL)
    {
	wcs_astro_format_ra (ra_str, centre_x);
	wcs_astro_format_dec (dec_str, centre_y);
	sprintf (window_string, "Centre: Ra %s  Dec %s\n", ra_str, dec_str);
	print_slice_info (window_string);
    }
    if (cube_arr == NULL) return (TRUE);
    integrate_ellipses (aux_canvas, centre_x, centre_y, radius);
    return (TRUE);
}   /*  End Function endpoint_position_func  */

flag track_aux_canvas_event (ViewableImage vimage, double x, double y,
			     void *value, unsigned int event_code,
			     void *e_info, void **f_info,
			     double x_lin, double y_lin,
			     unsigned int value_type)
/*  [PURPOSE] This routine is a position event consumer for a world canvas
    which has a number of ViewableImage objects associated with it.
    It differs slightly from track_canvas_event, in that it does fewer things.
    <viewimg> The active viewable image.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    These values will have been transformed by the registered transform
    function (see  canvas_register_transform_func ) for the associated
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
    unsigned int hdim, vdim, rdim;
    unsigned int num_restr, count;
    unsigned long pointer_x_index, pointer_y_index, r_index;
    unsigned char *rgb_ptr = (unsigned char *) value;
    char *xlabel, *ylabel;
    char **restr_names;
    double *restr_values;
    array_desc *arr_desc;
    char txt[STRING_LENGTH];
    char value_string[STRING_LENGTH], index_string[STRING_LENGTH];
    char world_string[STRING_LENGTH], extra_string[STRING_LENGTH];
    extern iarray image_arr;
    extern KwcsAstro aux_ap;
    static char function_name[] = "track_canvas_event";

    if (event_code != K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    track_aux_canvas_xtmagupd (vimage, x_lin, y_lin);
    /*  Create value string  */
    switch (value_type)
    {
      case K_DCOMPLEX:
	/*  Compute value  */
	iarray_format_value (image_arr, value_string,
			     *(double *) value, TOOBIG, TOOBIG);
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
    viewimg_get_attributes (vimage,
			    VIEWIMG_VATT_ARRAY_DESC, &arr_desc,
			    VIEWIMG_VATT_HDIM, &hdim,
			    VIEWIMG_VATT_VDIM, &vdim,
			    VIEWIMG_VATT_END);
    /*  Convert linear world co-ordinates to array indices and display  */
    pointer_x_index = ds_get_coord_num (arr_desc->dimensions[hdim], x_lin,
					SEARCH_BIAS_CLOSEST);
    pointer_y_index = ds_get_coord_num (arr_desc->dimensions[vdim], y_lin,
					SEARCH_BIAS_CLOSEST);
    sprintf (index_string, "x: %lu  y: %lu  ",
	     pointer_x_index, pointer_y_index);
    /*  Add any restriction information  */
    for (count = 0; count < num_restr; ++count)
    {
	if ( ( rdim = ds_f_dim_in_array (arr_desc, restr_names[count]) )
	     >= arr_desc->num_dimensions ) continue;
	r_index = ds_get_coord_num (arr_desc->dimensions[rdim],
				    restr_values[count], SEARCH_BIAS_CLOSEST);
	sprintf (txt, "z%u: %lu  ", rdim, r_index);
	strcat (index_string, txt);
    }
    strcat (index_string, value_string);
    /*  Now display the world co-ordinate information  */
    if (aux_ap == NULL)
    {
	sprintf (world_string, "%5e %s  %5e %s  ", x, xlabel, y, ylabel);
	/*  Add any restriction information  */
	for (count = 0; count < num_restr; ++count)
	{
	    sprintf (txt, "%5e %s  ", restr_values[count], restr_names[count]);
	    strcat (world_string, txt);
	}
    }
    else
    {
	wcs_astro_format_all (aux_ap, world_string,
			      xlabel, x_lin, ylabel, y_lin, NULL, 0.0,
			      num_restr, (CONST char **) restr_names,
			      restr_values, extra_string);
    }
    track_aux_canvas_xtcoord (index_string, 1);
    track_aux_canvas_xtcoord (world_string, 2);
    return (TRUE);
}   /*  End Function track_aux_canvas_event  */

void main_refresh_func (KWorldCanvas canvas, int width, int height,
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
    unsigned long pixel_value;
    double px[2], py[2];
    extern KwcsAstro main_ap;

    if (centre_x >= TOOBIG) return;
    px[0] = centre_x;
    py[0] = centre_y;
    if (main_ap == NULL) px[1] = centre_x + radius;
    else px[1] = centre_x + radius * cos (centre_y * PION180);
    py[1] = centre_y + radius;
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 2, px, py, px,py);
    px[1] = fabs (px[1] - px[0]);
    py[1] = fabs (py[1] - py[0]);
    pixel_value = get_colour (canvas, "green");
    kwin_draw_ellipse (canvas_get_pixcanvas (canvas),
		       px[0], py[0], py[1], py[1], pixel_value);
}   /*  End Function main_refresh_func  */


/*  Private functions follow  */

static void compute_area_from_ellipse (KWorldCanvas canvas,
				       KPixCanvasRefreshArea *area,
				       double centre_x, double centre_y,
				       double radius)
/*  [SUMMARY] Compute a pixel area from ellipse.
    <canvas> The world canvas.
    <area> The area will be written here.
    <centre_x> The horizontal world co-ordinate of the centre of the ellipse.
    <centre_y> The vertical world co-ordinate of the centre of the ellipse.
    <radius> The radius in world co-ordinates.
    [RETURNS] Nothing.
*/
{
    double px[2], py[2];
    extern KwcsAstro main_ap;

    px[0] = centre_x;
    py[0] = centre_y;
    if (main_ap == NULL) px[1] = centre_x + radius;
    else px[1] = centre_x + radius * cos (centre_y * PION180);
    py[1] = centre_y + radius;
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 2, px, py, px,py);
    px[1] = fabs (px[1] - px[0]);
    py[1] = fabs (py[1] - py[0]);
    area->startx = px[0] - py[1] - 1.0;
    area->endx = px[0] + py[1] + 1.0;
    area->starty = py[0] - py[1] - 1.0;
    area->endy = py[0] + py[1] + 1.0;
    area->clear = TRUE;
}   /*  End Function compute_area_from_ellipse  */

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

static void integrate_ellipses (KWorldCanvas canvas,
				double centre_x, double centre_y,double radius)
/*  [SUMMARY] Integrate around the circumferences of ellipses.
    <canvas> The KWorldCanvas to draw the new image to.
    <centre_x> The horizontal world co-ordinate of the centre of the ellipse.
    <centre_y> The vertical world co-ordinate of the centre of the ellipse.
    <radius> The radius in world co-ordinates.
    [RETURNS] Nothing.
*/
{
    iarray count_arr;
    int startx, endx, starty, endy, xlen, ylen, zlen, x, y, z;
    int num_radii_bins, val_rad;
    unsigned int vel_axis;
    float val;
    float toobig = TOOBIG;
    double sq_radius, distance, wx, wy, rad_scale;
    dim_desc *dim;
    char txt[STRING_LENGTH];
    unsigned long dim_lengths[2];
    double dx[2], dy[2];
    double crval[2], crpix[2], cdelt[2];
    CONST char *dim_names[2];
    extern KwcsAstro main_ap, cube_ap, aux_ap;
    extern iarray cube_arr, cube_ra_coords, cube_dec_coords;
    static ViewableImage vimage = NULL;
    static char function_name[] = "integrate_ellipses";

    vel_axis = iarray_get_fits_axis (cube_arr, 0);
    /*  Clean up from before  */
    if (image_arr != NULL) iarray_dealloc (image_arr);
    image_arr = NULL;
    if (vimage != NULL) viewimg_destroy (vimage);
    vimage = NULL;
    if (aux_ap != NULL) wcs_astro_destroy (aux_ap);
    aux_ap = NULL;
    if (radius <= 0.0) return;
    /*  Create new image: first determine ellipse limits in cube pixel
	co-ordinates  */
    dx[0] = centre_x - radius;
    dy[0] = centre_y - radius;
    dx[1] = centre_x + radius;
    dy[1] = centre_y + radius;
    if (cube_ap != NULL) wcs_astro_transform (cube_ap, 2,
					      dx, TRUE, dy, TRUE, NULL, FALSE,
					      0, NULL, NULL);
    dim = iarray_get_dim_desc (cube_arr, 2);
    dx[0] = ds_get_coord_num (dim, dx[0], SEARCH_BIAS_CLOSEST);
    dx[1] = ds_get_coord_num (dim, dx[1], SEARCH_BIAS_CLOSEST);
    dim = iarray_get_dim_desc (cube_arr, 1);
    dy[0] = ds_get_coord_num (dim, dy[0], SEARCH_BIAS_CLOSEST);
    dy[1] = ds_get_coord_num (dim, dy[1], SEARCH_BIAS_CLOSEST);
    if (dx[0] < dx[1])
    {
	startx = dx[0];
	endx = dx[1];
    }
    else
    {
	startx = dx[1];
	endx = dx[0];
    }
    if (dy[0] < dy[1])
    {
	starty = dy[0];
	endy = dy[1];
    }
    else
    {
	starty = dy[1];
	endy = dy[0];
    }
    xlen = (endx - startx + 1) / 2;
    ylen = (endy - starty + 1) / 2;
    zlen = iarray_dim_length (cube_arr, 0);
    num_radii_bins = sqrt (xlen * xlen + ylen * ylen);
    if (num_radii_bins < 2) return;
    rad_scale = (double) (num_radii_bins - 1) / radius;
    dim_lengths[0] = zlen;
    dim_lengths[1] = num_radii_bins;
    dim_names[0] = iarray_dim_name (cube_arr, 0);
    dim_names[1] = "ANGLE";
    if ( ( image_arr = iarray_create (K_FLOAT, 2, dim_names, dim_lengths,
				  iarray_value_name (cube_arr), NULL) )
	 == NULL )
    {
	m_abort (function_name, "image");
    }
    if (vel_axis > 0)
    {
	/*  Set the "CTYPE2" keyword with the vertical dimension name so that
	    the <wcs> package can deal with it  */
	iarray_put_named_string ( image_arr, "CTYPE2",
				  iarray_dim_name (image_arr, 0) );
	/*  Copy over frequency/velocity co-ordinate information from cube  */
	sprintf (txt, "CRVAL%u", vel_axis);
	if ( iarray_get_named_value (cube_arr, txt, NULL, crval) )
	{
	    iarray_put_named_value (image_arr, "CRVAL2", K_DOUBLE, crval);
	}
	sprintf (txt, "CRPIX%u", vel_axis);
	if ( iarray_get_named_value (cube_arr, txt, NULL, crpix) )
	{
	    iarray_put_named_value (image_arr, "CRPIX2", K_DOUBLE, crpix);
	}
	sprintf (txt, "CDELT%u", vel_axis);
	if ( iarray_get_named_value (cube_arr, txt, NULL, cdelt) )
	{
	    iarray_put_named_value (image_arr, "CDELT2", K_DOUBLE, cdelt);
	}
    }
    /*  Set the co-ordinate scale for the radius/angle dimension  */
    /*  Set the "CTYPE1" keyword with the horizontal dimension name so that the
	<wcs> package can deal with it  */
    iarray_put_named_string ( image_arr, "CTYPE1",
			      iarray_dim_name (image_arr, 1) );
    crval[0] = 0.0;
    crpix[0] = 1.0;
    cdelt[0] = radius / (double) (num_radii_bins - 1);
    iarray_put_named_value (image_arr, "CRVAL1", K_DOUBLE, crval);
    iarray_put_named_value (image_arr, "CRPIX1", K_DOUBLE, crpix);
    iarray_put_named_value (image_arr, "CDELT1", K_DOUBLE, cdelt);
    /*  Create the projection object for the slice canvas  */
    aux_ap = wcs_astro_setup (image_arr->top_pack_desc,*image_arr->top_packet);
    if ( ( count_arr = iarray_create_2D (zlen, num_radii_bins, K_UINT) )
	 == NULL )
    {
	m_abort (function_name, "count array");
    }
    sq_radius = radius * radius;
    for (y = starty; y <= endy; ++y) for (x = startx; x <= endx; ++x)
    {
	/*  Test if this pixel is within the ellipse  */
	wx = (D2 (cube_ra_coords, y, x) - centre_x);
	if (main_ap != NULL) wx *= cos (centre_y * PION180);
	wy = D2 (cube_dec_coords, y, x) - centre_y;
	distance = wx * wx + wy * wy;
	if (distance > sq_radius) continue;
	distance = sqrt (distance);
	val_rad = (distance * rad_scale);
	/*  Loop through channels  */
	for (z = 0; z < zlen; ++z)
	{
	    val = F3 (cube_arr, z, y, x);
	    if (val >= toobig) continue;
	    F2 (image_arr, z, val_rad) += val;
	    ++UI2 (count_arr, z, val_rad);
	}
    }
    /*  Divide sums by counts (average computation)  */
    for (y = 0; y < zlen; ++y) for (x = 0; x < num_radii_bins; ++x)
    {
	val_rad = UI2 (count_arr, y, x);
	if (val_rad > 0) F2 (image_arr, y, x) /= (float) val_rad;
    }
    iarray_dealloc (count_arr);
    if ( ( vimage = viewimg_create_from_iarray (canvas, image_arr, FALSE) )
	 == NULL )
    {
	m_abort (function_name, "ViewableImage");
    }
    viewimg_make_active (vimage);
}   /*  End Function integrate_ellipses  */

/*  event.c

    Event file for  kpvslice  (X11 Position-Velocity slice tool for Karma).

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
    This Karma module will enable interactive selection of a Position-Velocity
    slice through a data cube.
    This module runs on an X11 server.


    Written by      Richard Gooch   12-JUN-1996

    Updated by      Richard Gooch   15-JUN-1996: Used proper world co-ordinates

    Updated by      Richard Gooch   17-JUN-1996: Took account of Vince's new
  code which separates locus computation and slice extraction.

    Updated by      Richard Gooch   18-JUN-1996: Added code to save position
  angle and slice length (in offset degrees) and use right mouse button to move
  centre of slice while maintining length and position angle.

    Last updated by Richard Gooch   19-JUN-1996: Fixed bug when moving slice.


*/
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <karma.h>
#include <k_event_codes.h>
#define KWIN_GENERIC_ONLY
#include <karma_viewimg.h>
#include <karma_iarray.h>
#include <karma_canvas.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_m.h>


/*  External functions  */
/*  File: main_xt.c  */
EXTERN_FUNCTION (void print_slice_info, (CONST char *window_string) );
/*  File: pvget.c  */
EXTERN_FUNCTION (unsigned int get_slice,
		 (iarray cube, int x1, int y1, int x2, int y2,
		  double **x_locus, double **y_locus) );
EXTERN_FUNCTION (iarray pvslice,
		 (iarray cube, unsigned int num_points, double *x_locus,
		  double *y_locus) );


/*  Local functions  */
EXTERN_FUNCTION (flag endpoint_position_func,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info,
		  void **f_info, double x_lin, double y_lin) );


/*  Private functions  */
STATIC_FUNCTION (void draw_slice,
		 (iarray cube_arr, KWorldCanvas canvas,
		  double x0, double y0, double x1, double y1) );


/*  Private data  */
static double slice_length = TOOBIG;          /*  Units: Offset degrees  */
static double slice_position_angle = TOOBIG;  /*  Units: Degrees         */


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
    unsigned long pixel_value;
    double ra_off, dec_off, slice_centre_ra, slice_centre_dec, theta;
    char window_string[STRING_LENGTH];
    char ra_str[STRING_LENGTH], dec_str[STRING_LENGTH];
    double dx[2], dy[2];
    extern iarray cube_arr;
    extern KwcsAstro image_ap, cube_ap;
    extern flag auto_update;
    static double first_x, first_y; /*  Proper world co-ordinates  */
    /*static char function_name[] = "endpoint_position_func";*/

    switch (event_code)
    {
      case K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK:
	slice_position_angle = TOOBIG;
	first_x = x;
	first_y = y;
	if ( wcs_astro_test_radec (image_ap) )
	{
	    wcs_astro_format_ra (ra_str, x);
	    wcs_astro_format_dec (dec_str, y);
	    (void) fprintf (stderr, "First: Ra %s  Dec %s\n", ra_str, dec_str);
	}
	return (TRUE);
/*
	break;
*/
      case K_CANVAS_EVENT_MIDDLE_MOUSE_RELEASE:
	slice_position_angle = TOOBIG;
	if ( wcs_astro_test_radec (image_ap) )
	{
	    wcs_astro_format_ra (ra_str, x);
	    wcs_astro_format_dec (dec_str, y);
	    (void) fprintf (stderr, "Second: Ra %s  Dec %s\n",
			    ra_str, dec_str);
	}
	break;
      case K_CANVAS_EVENT_MIDDLE_MOUSE_DRAG:
	slice_position_angle = TOOBIG;
	break;
      case K_CANVAS_EVENT_RIGHT_MOUSE_CLICK:
      case K_CANVAS_EVENT_RIGHT_MOUSE_DRAG:
      case K_CANVAS_EVENT_RIGHT_MOUSE_RELEASE:
	/*  Move slice so that centre is over mouse  */
	if (slice_position_angle >= TOOBIG) return (TRUE);
	slice_centre_ra = x;
	slice_centre_dec = y;
	wcs_astro_format_ra (ra_str, slice_centre_ra);
	wcs_astro_format_dec (dec_str, slice_centre_dec);
	(void) sprintf (window_string,
			"Centre: Ra %s  Dec %s  PA: %.1f deg\n",
			ra_str, dec_str, slice_position_angle);
	print_slice_info (window_string);
	/*  Convert position angle to radians  */
	theta = slice_position_angle + 90.0;
	if (theta >= 360.0) theta -= 360.0;
	theta *= PION180;
	/*  Compute RA and DEC offsets. Note RA must have an inverse scaling
	 applied depending on the DEC  */
	ra_off = -slice_length / cos (slice_centre_dec * PION180) * cos(theta);
	dec_off = slice_length * sin (theta);
	if ( !canvas_get_colour (canvas, "green", &pixel_value,
				 NULL, NULL, NULL) )
	{
	    (void) fprintf (stderr, "Error allocating colour\n");
	    return (TRUE);
	}
	kwin_resize (canvas_get_pixcanvas (canvas), FALSE, 0, 0, 0, 0);
	dx[0] = slice_centre_ra - ra_off;
	dy[0] = slice_centre_dec - dec_off;
	dx[1] = slice_centre_ra + ra_off;
	dy[1] = slice_centre_dec + dec_off;
	canvas_draw_line_p (canvas, dx[0], dy[0], dx[1], dy[1], pixel_value);
	if ( !auto_update &&
	     (event_code == K_CANVAS_EVENT_RIGHT_MOUSE_DRAG) )
	{
	    /*  Don't update on drag if auto_update is not set  */
	    return (TRUE);
	}
	/*  Convert new endpoints in RA and DEC to cube linear world
	    co-ordinates  */
	wcs_astro_transform (cube_ap, 2,
			     dx, TRUE,
			     dy, TRUE,
			     NULL, FALSE,
			     0, NULL, NULL);
	draw_slice (cube_arr, aux_canvas, dx[0], dy[0], dx[1], dy[1]);
	return (TRUE);
/*
	break;
*/
      default:
	return (FALSE);
/*
	break;
*/
    }
    if ( !canvas_get_colour (canvas, "green", &pixel_value, NULL, NULL, NULL) )
    {
	(void) fprintf (stderr, "Error allocating colour\n");
	return (TRUE);
    }
    kwin_resize (canvas_get_pixcanvas (canvas), FALSE, 0, 0, 0, 0);
    canvas_draw_line_p (canvas, first_x, first_y, x, y, pixel_value);
    if ( !auto_update && (event_code != K_CANVAS_EVENT_MIDDLE_MOUSE_RELEASE) )
    {
	/*  Don't update on drag if auto_update is not set  */
	return (TRUE);
    }
    if (cube_arr == NULL) return (TRUE);
    dx[0] = first_x;
    dy[0] = first_y;
    dx[1] = x;
    dy[1] = y;
    if ( wcs_astro_test_radec (image_ap) && !wcs_astro_test_radec (cube_ap) )
    {
	/*  Have proper world co-ordinates for the canvas but not the cube:
	    better to convert back to linear world co-ordinates  */
	wcs_astro_transform (image_ap, 2,
			     dx, TRUE,
			     dy, TRUE,
			     NULL, FALSE,
			     0, NULL, NULL);
    }
    else if ( wcs_astro_test_radec (image_ap) &&
	      wcs_astro_test_radec (cube_ap) )
    {
	/*  Since both canvas and cube have proper world co-ordinates, will
	    need to convert canvas proper world co-ordinates to cube linear
	    world co-ordinates. However, first take a snapshot of these values
	    to compute centre and position angle. RA and DEC are in degrees
	    */
	slice_centre_ra = (dx[1] + dx[0]) / 2.0;
	slice_centre_dec = (dy[1] + dy[0]) / 2.0;
	/*  Have to convert second position to an offset relative to the
	    centre. Both these come in units of degrees, but to convert to
	    offset degrees (or offset arcseconds for that matter), the RA
	    offset must be scaled by cos (DEC), since the scale lengths of the
	    offsets must be the same
	    */
	ra_off = cos (slice_centre_dec * PION180) * (dx[1] - slice_centre_ra);
	dec_off = dy[1] - slice_centre_dec;
	/*  Finally the position angle may be computed  */
	slice_position_angle = atan2 (dec_off, -ra_off) / PION180 - 90.0;
	slice_length = sqrt (ra_off * ra_off + dec_off * dec_off);
	if (slice_position_angle < 0.0) slice_position_angle += 360.0;
	wcs_astro_format_ra (ra_str, slice_centre_ra);
	wcs_astro_format_dec (dec_str, slice_centre_dec);
	(void) sprintf (window_string,
			"Centre: Ra %s  Dec %s  PA: %.1f deg\n",
			ra_str, dec_str, slice_position_angle);
	print_slice_info (window_string);
	wcs_astro_transform (cube_ap, 2,
			     dx, TRUE,
			     dy, TRUE,
			     NULL, FALSE,
			     0, NULL, NULL);
    }
    draw_slice (cube_arr, aux_canvas, dx[0], dy[0], dx[1], dy[1]);
    return (TRUE);
}   /*  End Function endpoint_position_func  */

static void draw_slice (iarray cube_arr, KWorldCanvas canvas,
			double x0, double y0, double x1, double y1)
/*  [SUMMARY] Draw slice.
    <cube_arr> The cube Intelligent Array.
    <canvas> The KWorldCanvas object to draw the slice on.
    <x0> The first horizontal linear world co-ordinate.
    <y0> The first vertical linear world co-ordinate.
    <x1> The first horizontal linear world co-ordinate.
    <y1> The first vertical linear world co-ordinate.
    [RETURNS] Nothing.
*/
{
    unsigned int num_points;
    unsigned long px0, py0, px1, py1;
    double *x_locus, *y_locus;
    static ViewableImage vimage = NULL;
    static iarray image = NULL;
    static char function_name[] = "draw_slice";

    /*  Convert cube linear world co-ordinates to pixel co-ordinates  */
    px0 = ds_get_coord_num (iarray_get_dim_desc (cube_arr, 2), x0,
			    SEARCH_BIAS_CLOSEST);
    py0 = ds_get_coord_num (iarray_get_dim_desc (cube_arr, 1), y0,
			    SEARCH_BIAS_CLOSEST);
    px1 = ds_get_coord_num (iarray_get_dim_desc (cube_arr, 2), x1,
			    SEARCH_BIAS_CLOSEST);
    py1 = ds_get_coord_num (iarray_get_dim_desc (cube_arr, 1), y1,
			    SEARCH_BIAS_CLOSEST);
    if (image != NULL) iarray_dealloc (image);
    image = NULL;
    if (vimage != NULL) viewimg_destroy (vimage);
    vimage = NULL;
    if ( ( num_points = get_slice (cube_arr, px0, py0, px1, py1,
				   &x_locus, &y_locus) ) < 1 ) return;
    if (num_points < 2)
    {
	m_free ( (char *) x_locus );
	m_free ( (char *) y_locus );
	return;
    }
    if ( ( image = pvslice (cube_arr, num_points, x_locus, y_locus) ) == NULL )
    {
	m_free ( (char *) x_locus );
	m_free ( (char *) y_locus );
	return;
    }
    m_free ( (char *) x_locus );
    m_free ( (char *) y_locus );
    if ( ( vimage = viewimg_create_from_iarray (canvas, image, FALSE) )
	 == NULL )
    {
	m_abort (function_name, "ViewableImage");
    }
    (void) viewimg_make_active (vimage);
}   /*  End Function draw_slice  */

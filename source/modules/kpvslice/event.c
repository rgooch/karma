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

    Updated by      Richard Gooch   19-JUN-1996: Fixed bug when moving slice.

    Updated by      Vince McIntyre  06-AUG-1996: Added save button to slice
  window and proper header keywords so the slice can be imported to MIRIAD
  etc.

    Updated by      Vince McIntyre  06-AUG-1996: Added various FITS & history
  cards to o/p slice image.

    Updated by      Richard Gooch   10-AUG-1996: Added trace display over slice
  canvas and tracking cursor over image (main) canvas.

    Updated by      Richard Gooch   11-AUG-1996: Added trace display of
    computed 1st moment map.

    Updated by      Vince McIntyre  12-AUG-1996: Added ability to rotate slit
  position angle with right-mouse button, while preserving ability to drag
  across image at fixed p.a.

    Updated by      Richard Gooch   4-SEP-1996: Created <get_colour> routine.

    Updated by      Richard Gooch   11-SEP-1996: Use <show_trace> flag.

    Updated by      Richard Gooch   26-SEP-1996: Changed to implement the new
  FITS-style co-ordinate handling, where dimension co-ordinates range from 0
  to length - 1.

    Updated by      Richard Gooch   28-OCT-1996: Changed from <abs> to <fabs>.

    Last updated by Richard Gooch   30-OCT-1996: Removed requirement for a FITS
  velocity/frequency axis to be present.


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
#include "kpvslice.h"


#define ELLIPSE_RADIUS 5


/*  Private functions  */
STATIC_FUNCTION (void draw_slice,
		 (iarray cube_arr, KWorldCanvas canvas,
		  double x0, double y0, double x1, double y1,
		  double slice_centre_ra, double slice_centre_dec,
		  double slice_position_angle) );
STATIC_FUNCTION (flag fix_slice_header,
		 (iarray cube_arr, iarray image, unsigned int num_points,
		  double slice_centre_ra, double slice_centre_dec,
		  double slice_position_angle) );
STATIC_FUNCTION (void draw_trace,
		 (KWorldCanvas canvas, iarray image, CONST double *xarr,
		  CONST double *yarr, unsigned int num_points,
		  unsigned long pixel_value, KwcsAstro ap, flag same_scale,
		  flag pixel_coords) );
STATIC_FUNCTION (void compute_area_from_line,
		 (KWorldCanvas canvas, KPixCanvasRefreshArea *area,
		  double x0, double y0, double x1, double y1) );
STATIC_FUNCTION (unsigned long get_colour,
		 (KWorldCanvas canvas, CONST char *colourname) );


/*  Private data  */
static double slice_length = TOOBIG;          /*  Units: Offset degrees      */
static double slice_position_angle = TOOBIG;  /*  Units: Degrees             */
static double *offset_coord;                  /*  Units: Offset degrees      */
static double *x_locus = NULL;                /*  Units: cube co-ord indices */
static double *y_locus = NULL;                /*  Units: cube co-ord indices */
static double *ra_coord, *dec_coord;          /*  Units: degrees             */
static unsigned int num_points_in_slice = 0;
static double curr_line_x0 = TOOBIG;          /*  proper world coords        */
static double curr_line_y0 = TOOBIG;
static double curr_line_x1 = TOOBIG;
static double curr_line_y1 = TOOBIG;
static flag   rotation_mode = FALSE;


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
    double ra_off, dec_off, slice_centre_ra, slice_centre_dec, theta;
    double offset_0, offset_1, new_position_angle, sin_term, cos_term, dec;
    flag   end_0, end_1;
    char window_string[STRING_LENGTH];
    char ra_str[STRING_LENGTH], dec_str[STRING_LENGTH];
    double dx[2], dy[2];
    extern iarray image_arr, cube_arr, mom1_arr;
    extern KwcsAstro main_ap, cube_ap;
    extern flag auto_update, rotation_mode;
    static double first_x, first_y; /*  Proper world co-ordinates  */
    /*static char function_name[] = "endpoint_position_func";*/

    if (curr_line_x0 < TOOBIG)
    {
	/*  Compute area to be refreshed when old line is to be erased  */
	compute_area_from_line (canvas, &area, curr_line_x0, curr_line_y0,
				curr_line_x1, curr_line_y1);
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
	slice_position_angle = TOOBIG;
	first_x = x;
	first_y = y;
	if ( wcs_astro_test_radec (main_ap) )
	{
	    wcs_astro_format_ra (ra_str, x);
	    wcs_astro_format_dec (dec_str, y);
	    fprintf (stderr, "First: Ra %s  Dec %s\n", ra_str, dec_str);
	}
	return (TRUE);
/*
  break;
  */
      case K_CANVAS_EVENT_MIDDLE_MOUSE_RELEASE:
	slice_position_angle = TOOBIG;
	if ( wcs_astro_test_radec (main_ap) )
	{
	    wcs_astro_format_ra (ra_str, x);
	    wcs_astro_format_dec (dec_str, y);
	    fprintf (stderr, "Second: Ra %s  Dec %s\n",
		     ra_str, dec_str);
	}
	break;
      case K_CANVAS_EVENT_MIDDLE_MOUSE_DRAG:
	slice_position_angle = TOOBIG;
	break;
      case K_CANVAS_EVENT_RIGHT_MOUSE_CLICK:
      case K_CANVAS_EVENT_RIGHT_MOUSE_DRAG:
      case K_CANVAS_EVENT_RIGHT_MOUSE_RELEASE:
	/* If event position near to the ends of slit, twist it to new p.a.
	   It's too hard to keep cursor near end of slit when dragging to a
	   new p.a. (continuous rotation), so implement "rotation_mode".
	   This turns on if click occurs near either end, and stays on through
	   drag events, until the mouse is released.
	   In all other cases, move slice so that centre is over mouse.
	   */

	/* first get the current center */
	dx[0] = curr_line_x0;
	dy[0] = curr_line_y0;
	dx[1] = curr_line_x1;
	dy[1] = curr_line_y1;
	slice_centre_ra  = (dx[1] + dx[0]) / 2.0;
	slice_centre_dec = (dy[1] + dy[0]) / 2.0;
	/* centre up on pixel */

	/* now compute the length */
	ra_off = dx[1] - slice_centre_ra;
	cos_term =   sin(dy[1]*PION180)*sin(slice_centre_dec*PION180)
	    + cos(dy[1]*PION180)*cos(slice_centre_dec*PION180)*cos(ra_off*PION180);
	sin_term = sqrt( 1 - cos_term * cos_term );
	slice_length = 2.0 * atan2(sin_term, cos_term) / PION180;
	/*  what is the offset from each end. */
	cos_term =   sin(y*PION180)*sin(dy[0]*PION180)
	    + cos(y*PION180)*cos(dy[0]*PION180)*cos((x-dx[0])*PION180);
	sin_term = sqrt( 1 - cos_term * cos_term );
	offset_0 = atan2(sin_term, cos_term) / PION180;
	cos_term =   sin(y*PION180)*sin(dy[1]*PION180)
	    + cos(y*PION180)*cos(dy[1]*PION180)*cos((x-dx[1])*PION180);
	sin_term = sqrt( 1 - cos_term * cos_term );
	offset_1 = atan2(sin_term, cos_term) / PION180;
	end_0 = ( ( 5.0 * offset_0 ) < slice_length ); 
	end_1 = ( ( 5.0 * offset_1 ) < slice_length );
	if ( (end_0 || end_1) &&
	     (event_code == K_CANVAS_EVENT_RIGHT_MOUSE_CLICK) )
	{
	    rotation_mode = TRUE;
	}
	if (rotation_mode)
	{
	    /* Compute the new angle. Use proper w.c. */
	    ra_off   =  x - slice_centre_ra;
	    sin_term =     cos(y*PION180) * sin(ra_off*PION180);
	    cos_term =  (  sin(y*PION180) * cos(slice_centre_dec*PION180) )
		- (  cos(y*PION180) * sin(slice_centre_dec*PION180)
		     * cos(ra_off*PION180) );
	    new_position_angle = atan2 (sin_term, cos_term) / PION180;
	    if (new_position_angle < 0.0) new_position_angle += 360.0;
	    /* Careful not to flip direction of slice. */
	    if ( (fabs (new_position_angle - slice_position_angle) >  90.0) &&
		 (fabs (new_position_angle - slice_position_angle) < 270.0) )
	    {
		new_position_angle -= 180.0;
		if (new_position_angle < 0.0) new_position_angle += 360.0;
	    }
	    slice_position_angle = new_position_angle;
	    /* display centre & new pa */
	    wcs_astro_format_ra (ra_str, slice_centre_ra);
	    wcs_astro_format_dec (dec_str, slice_centre_dec);
	    sprintf (window_string, "Centre: Ra %s  Dec %s  PA: %.1f deg\n",
		     ra_str, dec_str, slice_position_angle);
	    print_slice_info (window_string);

	    /* Compute new RA and DEC offsets for endpoints. */
	    /*  Convert position angle to radians  */
	    theta = slice_position_angle + 90.0;
	    if (theta >= 360.0) theta -= 360.0;
	    theta *= PION180;
	    /* Note RA must have an inverse scaling applied depending on the DEC */
	    /* This method of fixing up RA assumes GLS geometry */
	    /* do this in spherical trig, not this way */
	    /*
	      ra_off = -slice_length * cos(theta) / cos (slice_centre_dec * PION180);
	      dec_off = slice_length * sin(theta);
	      */
	    /* have to be careful here, offsets are from slice centre */
	    sin_term = (    cos(slice_position_angle * PION180) 
			    * sin(slice_length / 2.0   * PION180) 
			    * cos(slice_centre_dec     * PION180) )
		+ (    cos(slice_length / 2.0   * PION180) 
		       * sin(slice_centre_dec     * PION180) );
	    cos_term = sqrt( 1 - sin_term * sin_term );
	    dec      = atan2(sin_term,cos_term)/PION180;
	    dec_off  = dec - slice_centre_dec;

	    sin_term =   sin(slice_position_angle * PION180) 
		* sin(slice_length / 2.0   * PION180) ;
	    if ( dec >-80 && dec < 80 ) {
		sin_term = sin_term / cos(dec * PION180);
	    }else{
		sin_term = sin_term /
		    sqrt( 1 - cos(dec * PION180) * cos(dec * PION180) );
	    }
	    cos_term = sqrt( 1 - sin_term * sin_term );
	    ra_off  = atan2(sin_term, cos_term) / PION180;
	    pixel_value = get_colour (canvas, "green");
	    /* store endpoints in global vars and for use below */
	    dx[0] = slice_centre_ra - ra_off;  
	    dy[0] = slice_centre_dec - dec_off;
	    dx[1] = slice_centre_ra + ra_off;  
	    dy[1] = slice_centre_dec + dec_off;
	    curr_line_x0 = dx[0];
	    curr_line_y0 = dy[0];
	    curr_line_x1 = dx[1];
	    curr_line_y1 = dy[1];

	}  /* end if angle should be changed */
	else {
	    /* just move the centre */
	    if (slice_position_angle >= TOOBIG) return (TRUE);
	    slice_centre_ra  = x;
	    slice_centre_dec = y;

	    /* display new position */
	    wcs_astro_format_ra (ra_str, slice_centre_ra);
	    wcs_astro_format_dec (dec_str, slice_centre_dec);
	    sprintf (window_string, "Centre: Ra %s  Dec %s  PA: %.1f deg\n",
		     ra_str, dec_str, slice_position_angle);
	    print_slice_info (window_string);

	    /* Compute RA and DEC offsets for new endpoints. */
	    /*  Convert position angle to radians  */
	    theta = slice_position_angle + 90.0;
	    if (theta >= 360.0) theta -= 360.0;
	    theta *= PION180;
	    /* Note RA must have an inverse scaling applied depending on the DEC */
	    /* This method of fixing up RA assumes GLS geometry. */
	    /*
	      ra_off = -slice_length * cos(theta) / cos (slice_centre_dec * PION180);
	      dec_off = slice_length * sin(theta);
	      */
	    /* do by spherical trig.
	       Take slice_length/2 to get offsets from slice centre */
	    sin_term = (    cos(slice_position_angle * PION180) 
			    * sin(slice_length / 2.0   * PION180) 
			    * cos(slice_centre_dec     * PION180) )
		+ (    cos(slice_length / 2.0   * PION180) 
		       * sin(slice_centre_dec     * PION180) );
	    cos_term = sqrt( 1 - sin_term * sin_term );
	    dec      = atan2(sin_term,cos_term)/PION180;
	    dec_off  = dec - slice_centre_dec;

	    sin_term =   sin(slice_position_angle * PION180) 
		* sin(slice_length / 2.0   * PION180) ;
	    if ( dec >-80 && dec < 80 ) {
		sin_term = sin_term / cos(dec * PION180);
	    }else{
		sin_term = sin_term /
		    sqrt( 1 - cos(dec * PION180) * cos(dec * PION180) );
	    }
	    cos_term = sqrt( 1 - sin_term * sin_term );
	    ra_off  = atan2(sin_term, cos_term) / PION180;
	    pixel_value = get_colour (canvas, "green");
	    dx[0] = slice_centre_ra  - ra_off;
	    dy[0] = slice_centre_dec - dec_off;
	    dx[1] = slice_centre_ra  + ra_off;
	    dy[1] = slice_centre_dec + dec_off;
	    curr_line_x0 = dx[0];
	    curr_line_y0 = dy[0];
	    curr_line_x1 = dx[1];
	    curr_line_y1 = dy[1];

	} /* end "not rotating, compute new centre" */
        if (event_code == K_CANVAS_EVENT_RIGHT_MOUSE_RELEASE)
	    rotation_mode = FALSE;
        /*  Force a refresh, which will in turn draw the line  */
	viewimg_partial_refresh (canvas, 1, &area);
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
	draw_slice (cube_arr, aux_canvas, dx[0], dy[0], dx[1], dy[1],
		    slice_centre_ra, slice_centre_dec, slice_position_angle);
	draw_trace (aux_canvas, image_arr, ra_coord, dec_coord,
		    num_points_in_slice, pixel_value, main_ap, FALSE, FALSE);
	if ( !canvas_get_colour (canvas, "red", &pixel_value,
				 NULL, NULL, NULL) )
	{
	    fprintf (stderr, "Error allocating colour\n");
	    return (TRUE);
	}
	draw_trace (aux_canvas, mom1_arr,x_locus, y_locus, num_points_in_slice,
		    pixel_value, cube_ap, TRUE, TRUE);
	return (TRUE);
/*
  break;
  */
      default:
	return (FALSE);
/*
  break;
  */
    } /* end  switch(event_code) */

    pixel_value = get_colour (canvas, "green");
    curr_line_x0 = first_x;
    curr_line_y0 = first_y;
    curr_line_x1 = x;
    curr_line_y1 = y;
    /*  Force a refresh, which will in turn draw the line  */
    viewimg_partial_refresh (canvas, 1, &area);
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
    /*  May need to convert co-ordinates to linear cube world co-ordinates, but
	first take a snapshot of these values to compute centre and position
	angle. RA and DEC are in degrees  */
    slice_centre_ra  = (dx[1] + dx[0]) / 2.0;
    slice_centre_dec = (dy[1] + dy[0]) / 2.0;
    /* old way */
    /*  Have to convert second position to an offset relative to the
	centre. Both these come in units of degrees, but to convert to
	offset degrees (or offset arcseconds for that matter), the RA
	offset must be scaled by cos (DEC), since the scale lengths of the
	offsets must be the same
	*/
    /*
      ra_off  = (dx[1] - slice_centre_ra) * cos (slice_centre_dec * PION180) ;
      dec_off =  dy[1] - slice_centre_dec;
      slice_length = sqrt (ra_off * ra_off + dec_off * dec_off);
      slice_position_angle = atan2 (dec_off, -ra_off) / PION180 - 90.0;
      */
    /* new way: spherical trig.
       Slice length is length of full slice, not half-slice */
    ra_off = dx[1] - slice_centre_ra;
    cos_term =   sin(dy[1]*PION180)*sin(slice_centre_dec*PION180)
	+ cos(dy[1]*PION180)*cos(slice_centre_dec*PION180)*cos(ra_off*PION180) ;
    sin_term = sqrt( 1 - cos_term * cos_term );
    slice_length = 2.0 * atan2(sin_term, cos_term) / PION180;

    sin_term =   cos(dy[1]*PION180) * sin(ra_off*PION180);
    cos_term =   sin(dy[1]*PION180) * cos(slice_centre_dec*PION180)
	- cos(dy[1]*PION180) * sin(slice_centre_dec*PION180) * cos(ra_off*PION180) ;
    slice_position_angle = atan2(sin_term, cos_term) / PION180;
    if (slice_position_angle < 0.0) slice_position_angle += 360.0;

    wcs_astro_format_ra (ra_str, slice_centre_ra);
    wcs_astro_format_dec (dec_str, slice_centre_dec);
    sprintf (window_string, "Centre: Ra %s  Dec %s  PA: %.1f deg\n",
	     ra_str, dec_str, slice_position_angle);
    print_slice_info (window_string);
    if ( wcs_astro_test_radec (main_ap) && !wcs_astro_test_radec (cube_ap) )
    {
	/*  Have proper world co-ordinates for the canvas but not the cube:
	    better to convert back to linear world co-ordinates  */
	/* this assumes the cube & image have same projections, ie the same values
	   of linear world coords in each refer to same RA, DEC. */
	wcs_astro_transform (main_ap, 2,
			     dx, TRUE,
			     dy, TRUE,
			     NULL, FALSE,
			     0, NULL, NULL);
    }
    if ( wcs_astro_test_radec (main_ap) && wcs_astro_test_radec (cube_ap) )
    {
	/*  Since both canvas and cube have proper world co-ordinates, will
	    need to convert canvas proper world co-ordinates to cube linear
	    world co-ordinates, to perform the extraction.  */
	wcs_astro_transform (cube_ap, 2,
			     dx, TRUE,
			     dy, TRUE,
			     NULL, FALSE,
			     0, NULL, NULL);
    }
    draw_slice (cube_arr, aux_canvas, dx[0], dy[0], dx[1], dy[1],
		slice_centre_ra, slice_centre_dec, slice_position_angle);
    draw_trace (aux_canvas, image_arr,ra_coord, dec_coord, num_points_in_slice,
		pixel_value, main_ap, FALSE, FALSE);
    if ( !canvas_get_colour (canvas, "red", &pixel_value, NULL, NULL, NULL) )
    {
	fprintf (stderr, "Error allocating colour\n");
	return (TRUE);
    }
    draw_trace (aux_canvas, mom1_arr, x_locus, y_locus, num_points_in_slice,
		pixel_value, cube_ap, TRUE, TRUE);
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
    KWorldCanvas main_canvas = (KWorldCanvas) *f_info;
    KPixCanvasRefreshArea area;
    unsigned int  hdim, vdim;
    unsigned int  num_restr;
    unsigned long pixel_value;
    double        offset, px, py;
    double        current_ra, current_dec;
    char          *xlabel, *ylabel;
    char          **restr_names;
    double        *restr_values;
    array_desc    *arr_desc;
    char          txt[STRING_LENGTH];
    char          second_track_txt[STRING_LENGTH];
    char          third_track_txt[STRING_LENGTH];
    extern KwcsAstro cube_ap;
    extern double    *ra_coord, *dec_coord;
    static int old_cursor_x = -1;
    static int old_cursor_y = -1;
    /*static char   function_name[] = "track_aux_canvas_event";*/

    if (event_code != K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    canvas_get_specification (viewimg_get_worldcanvas (vimage),
			      &xlabel, &ylabel, &num_restr,
			      &restr_names, &restr_values);
    viewimg_get_attributes (vimage,
			    VIEWIMG_VATT_ARRAY_DESC, &arr_desc,
			    VIEWIMG_VATT_HDIM, &hdim,
			    VIEWIMG_VATT_VDIM, &vdim,
			    VIEWIMG_VATT_END);
    /* The slice centre and angle is already displayed, in the first line */
    /* Offset is given relative to this. */

    /*  Now display the world co-ordinate information. There are two sets of */
    /*  coordinates; (offset,velocity) and (RA,DEC). */
    /*  Convert x_lin (co-ordinate index along slice) to offset  */
    offset = offset_coord[(int) x_lin];
    if (cube_ap == NULL)
    {
	sprintf (second_track_txt, "%5e %s  %5e %s  ",
		 x, xlabel, y, ylabel);
    }
    else
    {
	/* vel from image world coords should be y*/
	sprintf (second_track_txt, "Offset ");
	wcs_astro_format_dec (txt, offset);
	strcat (second_track_txt, txt);
	sprintf (txt, "  ");
	strcat (second_track_txt, txt);
	wcs_astro_format_vel (cube_ap, txt, y);
	strcat (second_track_txt, txt);
	track_aux_canvas_xtcoord (second_track_txt, 1);
    }
    current_ra = ra_coord[(int) x_lin];
    current_dec = dec_coord[(int) x_lin];
    if (cube_ap == NULL)
    {
	sprintf (third_track_txt, "%5e %s  %5e %s  ",
		 x, xlabel, y, ylabel);
    }
    else
    {
	sprintf (third_track_txt, "Ra ");
	wcs_astro_format_ra (txt, current_ra);
	strcat (third_track_txt, txt);
	strcat (third_track_txt, "  Dec ");
	wcs_astro_format_dec (txt, current_dec);
	strcat (third_track_txt, txt);
	track_aux_canvas_xtcoord (third_track_txt, 2);
    }
    /* update magnifier window */
    track_aux_canvas_xtmagupd (vimage, x_lin, y_lin);
    /*  Move a cursor over main canvas  */
    if (old_cursor_x > -1)
    {
	/*  First erase old cursor  */
	area.startx = old_cursor_x - ELLIPSE_RADIUS - 1;
	area.endx = old_cursor_x + ELLIPSE_RADIUS + 1;
	area.starty = old_cursor_y - ELLIPSE_RADIUS - 1;
	area.endy = old_cursor_y + ELLIPSE_RADIUS + 1;
	area.clear = TRUE;
	viewimg_partial_refresh (main_canvas, 1, &area);
    }
    canvas_convert_from_canvas_coords (main_canvas, FALSE, FALSE, 1,
				       &current_ra, &current_dec,
				       &px, &py);
    pixel_value = get_colour (main_canvas, "green");
    kwin_draw_ellipse (canvas_get_pixcanvas (main_canvas), px, py,
		       ELLIPSE_RADIUS, ELLIPSE_RADIUS,
		       pixel_value);
    old_cursor_x = px;
    old_cursor_y = py;
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

    if (curr_line_x0 >= TOOBIG) return;
    pixel_value = get_colour (canvas, "green");
    canvas_draw_line_p (canvas, curr_line_x0, curr_line_y0,
			curr_line_x1, curr_line_y1, pixel_value);
}   /*  End Function main_refresh_func  */

void trace_refresh_func (KWorldCanvas canvas, int width, int height,
			 struct win_scale_type *win_scale,
			 Kcolourmap cmap, flag cmap_resize, void **info,
			 PostScriptPage pspage,
			 unsigned int num_areas,
			 KPixCanvasRefreshArea *areas,
			 flag *honoured_areas)
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
    extern KwcsAstro main_ap;
    extern iarray image_arr, mom1_arr;
    extern double *x_locus, *y_locus, *ra_coord, *dec_coord;

    if (x_locus == NULL) return;
    if ( canvas_get_colour (canvas, "red", &pixel_value, NULL, NULL, NULL) )
    {
	draw_trace (canvas, mom1_arr, x_locus, y_locus, num_points_in_slice,
		    pixel_value, NULL, TRUE, TRUE);
    }
    else fprintf (stderr, "Error allocating colour\n");
    if (ra_coord == NULL) return;
    pixel_value = get_colour (canvas, "green");
    draw_trace (canvas, image_arr, ra_coord, dec_coord, num_points_in_slice,
		pixel_value, main_ap, FALSE, FALSE);
}   /*  End Function trace_refresh_func  */


/*  Private functions follow  */

static void convert_locus (double *x_locus, double *y_locus,
			   unsigned int num_points,
			   iarray array, double *x_coord, double *y_coord)
/*  [SUMMARY] Convert real-valued pixel coords into linear world coords,
    the hard way. Assumes that linear interpolation is enough; in general
    it won't be.
    [RETURNS] Nothing, updates arguments
*/
{
    unsigned int count;
    dim_desc *xdim, *ydim;

    xdim = iarray_get_dim_desc (array, 2);
    ydim = iarray_get_dim_desc (array, 1);
    for (count = 0; count < num_points; count++)
    {
	x_coord[count] = ds_get_coordinate (xdim, x_locus[count]);
	y_coord[count] = ds_get_coordinate (ydim, y_locus[count]);
    }
}   /*  End Function convert_locus  */
/* --------------------------------------------------------------------------- */
static void draw_slice (iarray cube_arr, KWorldCanvas canvas,
			double x0, double y0, double x1, double y1,
			double slice_centre_ra, double slice_centre_dec,
			double slice_position_angle)
/*  [SUMMARY] Draw slice.
    <cube_arr> The cube Intelligent Array.
    <canvas> The KWorldCanvas object to draw the slice on.
    <x0> The first horizontal linear world co-ordinate.
    <y0> The first vertical linear world co-ordinate.
    <x1> The second horizontal linear world co-ordinate.
    <y1> The second vertical linear world co-ordinate.
    <slice_centre_ra> Horizontal proper world co-ordinate of the centre of
    slice.
    <slice_centre_dec> Vertical proper world co-ordinate of the centre of
    slice.
    <slice_position_angle> in degrees
    [RETURNS] Nothing.
*/
{
    unsigned int vel_axis;
    unsigned long        px0, py0, px1, py1;
    unsigned long        xdim, ydim, vdim;
    double               ra_off, dec_off, offset_zero;
    unsigned int         count;
    char txt[STRING_LENGTH];
    double crval[2], crpix[2], cdelt[2];
    extern KwcsAstro     cube_ap, aux_ap;
    extern double        *offset_coord;
    static ViewableImage vimage = NULL;
    static iarray image = NULL;
    static char function_name[] = "draw_slice";

    vel_axis = iarray_get_fits_axis (cube_arr, 0);
    /*  Convert cube linear world co-ordinates to pixel co-ordinates  */
    px0 = ds_get_coord_num (iarray_get_dim_desc (cube_arr, 2), x0,
			    SEARCH_BIAS_CLOSEST);
    py0 = ds_get_coord_num (iarray_get_dim_desc (cube_arr, 1), y0,
			    SEARCH_BIAS_CLOSEST);
    px1 = ds_get_coord_num (iarray_get_dim_desc (cube_arr, 2), x1,
			    SEARCH_BIAS_CLOSEST);
    py1 = ds_get_coord_num (iarray_get_dim_desc (cube_arr, 1), y1,
			    SEARCH_BIAS_CLOSEST);

    /* free memory allocated to old slice & coordinates */
    if (image != NULL) iarray_dealloc (image);
    image = NULL;
    if (vimage != NULL) viewimg_destroy (vimage);
    vimage = NULL;
    if (x_locus != NULL) m_free ( (char *) x_locus );
    x_locus = NULL;
    if (y_locus != NULL) m_free ( (char *) y_locus );
    y_locus = NULL;
    if (ra_coord != NULL) m_free ( (char *) ra_coord );
    ra_coord = NULL;
    if (dec_coord != NULL) m_free ( (char *) dec_coord );
    dec_coord = NULL;
    if (offset_coord != NULL) m_free ( (char *) offset_coord );
    offset_coord = NULL;
    if (aux_ap != NULL) wcs_astro_destroy (aux_ap);
    aux_ap = NULL;

    if ( ( num_points_in_slice = get_slice (cube_arr, px0, py0, px1, py1,
					    &x_locus, &y_locus) ) < 1 ) return;
    if (num_points_in_slice < 2)
    {
	m_free ( (char *) x_locus );
	m_free ( (char *) y_locus );
	x_locus = NULL;
	y_locus = NULL;
	return;
    }
    if ( ( image = pvslice (cube_arr, num_points_in_slice, x_locus, y_locus) )
	 == NULL )
    {
	m_free ( (char *) x_locus );
	m_free ( (char *) y_locus );
	x_locus = NULL;
	y_locus = NULL;
	return;
    }

    /* allocate enough memory. */
    ra_coord = (double *) m_alloc (num_points_in_slice * sizeof *ra_coord);
    dec_coord = (double *) m_alloc (num_points_in_slice * sizeof *dec_coord);
    offset_coord = (double *) m_alloc (num_points_in_slice * sizeof *offset_coord);

    /* first have to get from pixels back to linear world coords */
    /* NB locus points are doubles, not ints - fraction of a pixel is known */
    /* ds_get_coordinate will give coord for input integer pixel. */
    /* do I have to get nearest int coords and interpolate? Yuck */
    convert_locus (x_locus, y_locus, num_points_in_slice, cube_arr, ra_coord,dec_coord);

    /* convert the locus points to proper world coords */
    /* store in arrays global to this module. */
    wcs_astro_transform (cube_ap, num_points_in_slice,
			 ra_coord, FALSE,
			 dec_coord, FALSE,
			 NULL, FALSE,
			 0, NULL, NULL);

    /* centre is arithmetic mean of endpoints in each coord, (in proper world
       coords). It is obtained by endpoint_position_func. */
    /* compute offsets from first point in slice, but set 0 to be centre point */
    ra_off = cos (slice_centre_dec*PION180) * (slice_centre_ra - ra_coord[0]);
    dec_off = (slice_centre_dec - dec_coord[0]);
    offset_zero = sqrt (ra_off * ra_off + dec_off * dec_off);

    for (count=0; count<num_points_in_slice; count++) {
      ra_off = cos (slice_centre_dec*PION180) * (ra_coord[count]-ra_coord[0]);
      dec_off = (dec_coord[count]-dec_coord[0]);
      offset_coord[count] = sqrt(ra_off * ra_off + dec_off * dec_off) - offset_zero;
    }

    vdim=0; ydim=1; xdim=2;
   
    /* Set the linear coordinate system for offsets */
    if (vel_axis > 0)
    {
	/* Copy velocity coords from input cube. Assumed to be zeroth axis of
	   iarray.
	   This will be wrong if pvslice doesn't extract all velocity channels,
	   but the way it is done above ensures this so. NB axes numbered from
	   0 */
	sprintf (txt, "CRVAL%u", vel_axis);
	if ( iarray_get_named_value (cube_arr, txt, NULL, crval) )
	{
	    iarray_put_named_value (image, "CRVAL2", K_DOUBLE, crval);
	}
	sprintf (txt, "CRPIX%u", vel_axis);
	if ( iarray_get_named_value (cube_arr, txt, NULL, crpix) )
	{
	    iarray_put_named_value (image, "CRPIX2", K_DOUBLE, crpix);
	}
	sprintf (txt, "CDELT%u", vel_axis);
	if ( iarray_get_named_value (cube_arr, txt, NULL, cdelt) )
	{
	    iarray_put_named_value (image, "CDELT2", K_DOUBLE, cdelt);
	}
    }
    /*  Set the "CTYPEn" keywords with the dimension names so that the <wcs>
	package can deal with it  */
    iarray_put_named_string ( image, "CTYPE2", iarray_dim_name (image, 0) );
    iarray_put_named_string ( image, "CTYPE1", iarray_dim_name (image, 1) );
    /*  Create the projection object for the slice canvas  */
    aux_ap = wcs_astro_setup (image->top_pack_desc, *image->top_packet);
    /* copy various FITS cards from cube to new slice */
    /* this call should be made only when the slice is to be saved */
    if ( ! fix_slice_header(cube_arr, image, num_points_in_slice,
			    slice_centre_ra, slice_centre_dec,
			    slice_position_angle) ) {
      /* do nothing fprintf(stderr, "Some problem with headers\n"); */
    }

    if ( ( vimage = viewimg_create_from_iarray (canvas, image, FALSE) )
	 == NULL )
    {
	m_abort (function_name, "ViewableImage");
    }
    viewimg_make_active (vimage);
}   /*  End Function draw_slice  */

/* --------------------------------------------------------------------------- */

static flag fix_slice_header (iarray cube_arr, iarray image,
			      unsigned int num_points,
			      double slice_centre_ra, double slice_centre_dec,
			      double slice_position_angle)
/*  [SUMMARY] Add relevant info from the cube to the slice image
    [RETURNS] TRUE on success, FALSE if there was a problem.
*/
{
    flag           ok = TRUE;
    char           tmp_str[STRING_LENGTH];
    char           hist_str[STRING_LENGTH];
    extern char module_version_date[STRING_LENGTH + 1];
    /*static char    function_name[] = "fix_slice_new";*/

    /* NB:
       EPOCH is used by FITS to mean EQUINOX, ie the equinox the coords
       are referred to. The epoch of observation is stored in DATE-OBS.
       The revised FITS standard ecourages EQUINOX & deprecates EPOCH, but
       I don't attempt to fix headers that use the old way.

      */
    copy_header_info (image, cube_arr);
    /* add history */
    sprintf (hist_str, "KPVSLICE: Version %s\n", module_version_date);
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    wcs_astro_format_ra (tmp_str, slice_centre_ra);
    sprintf (hist_str, "KPVSLICE: Slice Centre RA %s\n", tmp_str);
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    wcs_astro_format_dec (tmp_str, slice_centre_dec);
    sprintf (hist_str, "KPVSLICE: Slice Centre DEC %s\n", tmp_str);
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    wcs_astro_format_dec (tmp_str, slice_position_angle);
    sprintf (hist_str, "KPVSLICE: Slice PA %s\n", tmp_str);
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    wcs_astro_format_ra (tmp_str, ra_coord[0]);
    sprintf (hist_str, "KPVSLICE: Pixel 1 RA %s\n", tmp_str);
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    wcs_astro_format_dec (tmp_str, dec_coord[0]);
    sprintf (hist_str, "KPVSLICE: Pixel 1 DEC %s\n", tmp_str );
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    wcs_astro_format_ra (tmp_str, ra_coord[num_points-1]);
    sprintf (hist_str, "KPVSLICE: Pixel %u RA %s\n", num_points,tmp_str);
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    wcs_astro_format_dec (tmp_str, dec_coord[num_points-1]);
    sprintf (hist_str, "KPVSLICE: Pixel %u DEC %s\n", num_points,tmp_str);
    if ( !iarray_append_history_string (image, hist_str, TRUE) ) ok = FALSE;

    return (ok);
}
/* end function fix_slice_header */
/* --------------------------------------------------------------------------*/

static void draw_trace (KWorldCanvas canvas, iarray image, CONST double *xarr,
			CONST double *yarr, unsigned int num_points,
			unsigned long pixel_value, KwcsAstro ap,
			flag same_scale, flag pixel_coords)
/*  [SUMMARY] Draw the trace from the image over the slice canvas.
    <canvas> The KWorldCanvas object.
    <image> The image array.
    <xarr> The array of non-linear horizontal world co-ordinates.
    <yarr> The array of non-linear vertical world co-ordinates.
    <num_points> The number of points to draw.
    <pixel_value> The pixel value to draw with.
    <ap> The KwcsAstro object used to convert non-linear world co-ordinates to
    linear world co-ordinates.
    <same_scale> If TRUE, the routine assumes the same scale is present.
    <pixel_coords> If TRUE, the routine assumes the co-ordinates are in pixels.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    double xscale, yscale, x, y, xmax, ymax, value;
    double scale;
    CONST char *image_data_name, *cube_z_name;
    dim_desc *xdim, *ydim;
    extern iarray cube_arr;
    extern flag show_trace;
    static unsigned int buf_len;
    static double *xbuf = NULL;
    static double *ybuf = NULL;
    static char function_name[] = "draw_trace";

    if (!show_trace) return;
    if (xarr == NULL) return; /* assume xarr, yarr work in pairs */
    if (image == NULL) return;
    /*  Compute scaling for image data to fit vertically in slice canvas. If
	the units are the same, do not scale  */
    image_data_name = iarray_value_name (image);
    cube_z_name = iarray_dim_name (cube_arr, 0);
    scale = TOOBIG;
    if (same_scale) scale = 1.0;
    else if (strncmp (cube_z_name, "FREQ", 4) == 0)
    {
	/*  Slice has frequency in Hz  */
	if (st_icmp (image_data_name, "Hz") == 0) scale = 1.0;
	else if (st_icmp (image_data_name, "kHz") == 0) scale = 1e3;
	else if (st_icmp (image_data_name, "MHz") == 0) scale = 1e6;
	else if (st_icmp (image_data_name, "GHz") == 0) scale = 1e9;
    }
    else if ( (strncmp (cube_z_name, "VELO", 4) == 0) ||
	      (strncmp (cube_z_name, "FELO", 4) == 0) )
    {
	/*  Slice has velocity in m/s  */
	if (st_icmp (image_data_name, "m/s") == 0) scale = 1.0;
	else if (st_icmp (image_data_name, "m/sec") == 0) scale = 1.0;
	else if (st_icmp (image_data_name, "km/s") == 0) scale = 1e3;
	else if (st_icmp (image_data_name, "km/sec") == 0) scale = 1e3;
    }
    else if (st_icmp (cube_z_name, image_data_name) == 0)
    {
	/*  Something unknown, but they agree: no scaling  */
	scale = 1.0;
    }
    if (scale >= TOOBIG)
    {
	/*  Scaling unknown: don't draw anything as it's probably not
	    meaningful  */
	return;
    }
    /*  Ensure buffers are big enough  */
    if (num_points > buf_len)
    {
	if (xbuf != NULL) m_free ( (char *) xbuf );
	if (ybuf != NULL) m_free ( (char *) ybuf );
	if ( ( xbuf = (double *) m_alloc (num_points * sizeof *xbuf) )
	     == NULL ) m_abort (function_name, "x buffer");
	if ( ( ybuf = (double *) m_alloc (num_points * sizeof *ybuf) )
	     == NULL ) m_abort (function_name, "y buffer");
	buf_len = num_points;
    }
    /*  Copy to buffers  */
    for (count = 0; count < num_points; ++count)
    {
	xbuf[count] = xarr[count];
	ybuf[count] = yarr[count];
    }
    /*  Convert back to image linear world co-ordinates  */
    if ( !pixel_coords && (ap != NULL) )
	wcs_astro_transform (ap, num_points,
			     xbuf, TRUE, ybuf, TRUE, NULL, FALSE,
			     0, NULL, NULL);
    /*  Convert to co-ordinate indices, extract data and scale  */
    xdim = iarray_get_dim_desc (image, 1);
    ydim = iarray_get_dim_desc (image, 0);
    xmax = xdim->length - 1;
    ymax = ydim->length - 1;
    xscale = xmax / (xdim->last_coord - xdim->first_coord);
    yscale = ymax / (ydim->last_coord - ydim->first_coord);
    for (count = 0; count < num_points; ++count)
    {
	if (pixel_coords)
	{
	    x = xbuf[count];
	    y = ybuf[count];
	}
	else
	{
	    x = (xbuf[count] - xdim->first_coord) * xscale;
	    y = (ybuf[count] - ydim->first_coord) * yscale;
	}
	/*  Clip to boundaries  */
	if (x < 0.0) x = 0.0;
	else if (x > xmax) x = xmax;
	if (y < 0.0) y = 0.0;
	else if (y > ymax) y = ymax;
	/*  Place point index in x array  */
	xbuf[count] = count;
	/*  Extract value, scale and place in y array  */
	if ( ( value = F2 (image, (int) y, (int) x) ) < TOOBIG )
	    value *= scale;
	ybuf[count] = value;
    }
    /*  Draw the trace  */
    canvas_draw_lines_p (canvas, xbuf, ybuf, num_points, pixel_value);
}   /*  End Function draw_trace  */

static void compute_area_from_line (KWorldCanvas canvas,
				    KPixCanvasRefreshArea *area,
				    double x0, double y0, double x1, double y1)
/*  [SUMMARY] Compute a pixel area from line.
    <canvas> The world canvas.
    <area> The area will be written here.
    <x0> The horizontal world co-ordinate of the first endpoint of the line.
    <y0> The vertical world co-ordinate of the first endpoint of the line.
    <x1> The horizontal world co-ordinate of the second endpoint of the line.
    <y1> The verticak world co-ordinate of the second endpoint of the line.
*/
{
    double px[2], py[2];

    px[0] = x0;
    py[0] = y0;
    px[1] = x1;
    py[1] = y1;
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 2, px, py, px,py);
    if (px[0] < px[1])
    {
	area->startx = px[0] - 1.0;
	area->endx = px[1] + 1.0;
    }
    else
    {
	area->startx = px[1] - 1.0;
	area->endx = px[0] + 1.0;
    }
    if (py[0] < py[1])
    {
	area->starty = py[0] - 1.0;
	area->endy = py[1] + 1.0;
    }
    else
    {
	area->starty = py[1] - 1.0;
	area->endy = py[0] + 1.0;
    }
    area->clear = TRUE;
}   /*  End Function compute_area_from_line  */

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

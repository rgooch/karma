/*  kmovie_event.c

    Event consumer file for  kmovie  (X11 movie display tool for Karma).

    Copyright (C) 1993  Richard Gooch

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
    This Karma module will enable on-screen image animation of a single
    arrayfile of the general data structure format. The arrayfile must contain
    a 3 dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   8-SEP-1993: Copied from  kview_2d_event.c

    Last updated by Richard Gooch   20-SEP-1993


*/
#include <stdio.h>
#include <k_event_codes.h>
#include "kmovie.h"
#include <karma_viewimg.h>
#include <karma_kcmap.h>


flag unit_canvas_position_func (canvas, x, y, event_code, e_info, f_info)
/*  This routine is a position event consumer for a world canvas.
    The canvas is given by  canvas  .
    The horizontal world co-ordinate of the event will be given by  x  .
    The vertical world co-ordinate of the event will be given by  y  .
    The arbitrary event code is given by  event_code  .
    The arbitrary event information is pointed to by  e_info  .
    The arbitrary function information pointer is pointed to by  f_info  .
    The routine returns TRUE if the event was consumed, else it return
    FALSE indicating that the event is still to be processed.
*/
KWorldCanvas canvas;
double x;
double y;
unsigned int event_code;
void *e_info;
void **f_info;
{
    Kcolourmap cmap;
    static char function_name[] = "unit_canvas_position_func";

    if ( (cmap = (Kcolourmap) *f_info) == NULL )
    {
	(void) fprintf (stderr, "NULL Kcolourmap\n");
	a_prog_bug (function_name);
    }
    if ( (event_code != K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK) &&
	(event_code != K_CANVAS_EVENT_MIDDLE_MOUSE_DRAG) ) return (FALSE);
    kcmap_modify (cmap, x, y, (void *) NULL);
    return (TRUE);
}   /*  End Function unit_canvas_position_func  */

void draw_crosshair (x, y)
/*  This routine will draw (using an XOR graphics context) a crosshair on the
    canvas paint window at the co-ordinate specified by  x  and  y  .
    The routine returns nothing.
*/
double x;
double y;
{
    int width, height;
    double range;
    double value[2];
    struct win_scale_type win_scale;
    extern KPixCanvas image_pixcanvas;
    extern KWorldCanvas image_worldcanvas;
    extern GC image_gc;
    extern GC crosshair_gc;

    value[0] = 0.0;
    value[1] = 0.0;
    kwin_set_gc_x (image_pixcanvas, crosshair_gc);
    canvas_get_size (image_worldcanvas, &width, &height, &win_scale);
    /*  Draw vertical hair  */
    range = win_scale.y_max - win_scale.y_min;
    canvas_draw_line (image_worldcanvas,
		      x, win_scale.y_min - range,
		      x, win_scale.y_max + range, value);
    /*  Draw horizontal hair  */
    range = win_scale.x_max - win_scale.x_min;
    canvas_draw_line (image_worldcanvas,
		      win_scale.x_min - range, y,
		      win_scale.x_max + range, y, value);
    kwin_set_gc_x (image_pixcanvas, image_gc);
}   /*  End Function draw_crosshair  */

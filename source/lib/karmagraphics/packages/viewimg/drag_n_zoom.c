/*LINTLIBRARY*/
/*  drag_n_zoom.c

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

/*  This file contains convenience routines which implement a drag and zoom
  user interface.


    Written by      Richard Gooch   26-MAY-1996

    Updated by      Richard Gooch   4-JUN-1996: Switched to left-right
  bottom-top co-ordinate specification instead of min-max x and y.

    Updated by      Richard Gooch   17-JUN-1996: Made use of new
  <canvas_convert_to_canvas_coords> routine so that linear to non-linear
  transformation could be avoided.

    Updated by      Richard Gooch   21-OCT-1996: Added zoom-in 'i' and zoom-out
  'o' feature.

    Updated by      Richard Gooch   25-OCT-1996: Use fallback when green could
  not be allocated.

    Last updated by Richard Gooch   27-OCT-1996: Compute and display statistics
  when 's' key is pressed during drag.


*/
#include <stdio.h>
#include <math.h>
#include <stdarg.h>
#include <karma.h>
#define KWIN_GENERIC_ONLY
#define NEW_WIN_SCALE
#include <k_event_codes.h>
#include <karma_viewimg.h>
#include <karma_m.h>
#include <karma_a.h>

#define MAGIC_NUMBER (unsigned int) 1876423498
#define swap(a,b) {tmp = a; a = b; b = tmp;}

#define VERIFY_DRAGZOOM(dragzoom) if (dragzoom == NULL) \
{fprintf (stderr, "NULL viewable image passed\n"); \
 a_prog_bug (function_name); } \
if (dragzoom->magic_number != MAGIC_NUMBER) \
{fprintf (stderr, "Invalid KDragZoom object\n"); \
 a_prog_bug (function_name); }


/*  Private structures  */

typedef struct
{
    unsigned int magic_number;
    KWorldCanvas worldcanvas;
    KPixCanvas pixcanvas;
    int first_px, first_py;       /*  Pixel co-ordinates         */
    int second_px, second_py;     /*  Pixel co-ordinates         */
    double first_lx, first_ly;    /*  Linear world co-ordinates  */
    double second_lx, second_ly;  /*  Linear world co-ordinates  */
    flag in_drag;
    KCallbackFunc pos_func;
} * KDragZoom;


/*  Private functions  */
STATIC_FUNCTION (flag position_func,
		 (KPixCanvas canvas, int x, int y,
		  unsigned int event_code, void *e_info,
		  void **f_info) );
STATIC_FUNCTION (void clear_zoom_box, (KDragZoom dragzoom) );


/*  Public functions follow  */

/*EXPERIMENTAL_FUNCTION*/
void viewimg_create_drag_and_zoom_interface (KWorldCanvas canvas)
/*  [SUMMARY] Create a drag and zoom user interface for a canvas.
    [PURPOSE] This routine will create a drag and zoom user interface for a
    world canvas. When the user presses the left mouse button in the canvas and
    moves the mouse with the button still pressed a rubber-band box will be
    drawn on the canvas. When the left mouse button is released the image in
    the rubber-banded box is zoomed to fill as much of the canvas as is
    possible, depending on the zoom policy.
    <canvas> The world canvas object.
    [RETURNS] Nothing.
*/
{
    KDragZoom new;
    static char function_name[] = "viewimg_create_drag_and_zoom_interface";

    if ( ( new = (KDragZoom) m_alloc (sizeof *new) ) == NULL )
    {
	m_abort (function_name, "KDragZoom object");
    }
    new->worldcanvas = canvas;
    new->pixcanvas = canvas_get_pixcanvas (canvas);
    new->in_drag = FALSE;
    new->pos_func = kwin_register_position_event_func (new->pixcanvas,
						       position_func, new);
    new->magic_number = MAGIC_NUMBER;
}   /*  End Function viewimg_create_drag_and_zoom_interface  */


/*  Private functions follow  */

static flag position_func (KPixCanvas canvas, int x, int y,
			   unsigned int event_code, void *e_info,
			   void **f_info)
/*  [SUMMARY] Process a position event on a pixel canvas.
    <canvas> The pixel canvas on which the event occurred.
    <x> The horizontal position of the event, relative to the canvas origin
    <y> The vertical position of the event, relative to the canvas origin.
    <event_code> The arbitrary event code.
    <e_info> A pointer to arbitrary event information.
    <f_info> A pointer to an arbitrary function information pointer.
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    ViewableImage vimage;
    int width, height;
    unsigned int visual;
    unsigned long pixel_value, mask;
    double px, py, x_lin, y_lin, tmp;
    double left_x, right_x, bottom_y, top_y;
    double win_left_x, win_right_x, win_bottom_y, win_top_y;
    KDragZoom dragzoom = (KDragZoom) *f_info;
    CONST char *string = e_info;
    static char function_name[] = "__viewimg_drag_and_zoom_position_func";

    VERIFY_DRAGZOOM (dragzoom);
    if (dragzoom->pixcanvas != canvas)
    {
	fprintf (stderr,
		 "Event canvas: %p does not match KDragZoom canvas: %p\n",
		 dragzoom->pixcanvas, canvas);
	a_prog_bug (function_name);
    }
    /*  Discard mouse move events quickly  */
    if (event_code == K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    px = x;
    py = y;
    canvas_convert_to_canvas_coords (dragzoom->worldcanvas, FALSE, 1, &px, &py,
				     &x_lin, &y_lin, NULL, NULL);
    canvas_get_attributes (dragzoom->worldcanvas,
			   CANVAS_ATT_LEFT_X, &win_left_x,
			   CANVAS_ATT_RIGHT_X, &win_right_x,
			   CANVAS_ATT_BOTTOM_Y, &win_bottom_y,
			   CANVAS_ATT_TOP_Y, &win_top_y,
			   CANVAS_ATT_END);
    if (event_code == K_CANVAS_EVENT_PLAIN_KEY_PRESS)
    {
	if (strcmp (string, "u") == 0)
	{
	    /*  Unzoom  */
	    dragzoom->in_drag = FALSE;
	    viewimg_set_canvas_attributes (dragzoom->worldcanvas,
					   VIEWIMG_ATT_AUTO_X, TRUE,
					   VIEWIMG_ATT_AUTO_Y, TRUE,
					   VIEWIMG_ATT_END);
	    /*  Redraw the canvas  */
	    if ( !kwin_resize (dragzoom->pixcanvas, TRUE, 0, 0, 0, 0) )
	    {
		fprintf (stderr, "%s: Error refreshing window\n",
			 function_name);
	    }
	    return (TRUE);
	}
	else if (strcmp (string, "i") == 0)
	{
	    /*  Zoom in 2x  */
	    dragzoom->in_drag = FALSE;
	    if ( fabs (x_lin - win_left_x) < fabs (win_right_x - x_lin) )
	    {
		/*  Near the left  */
		tmp = (x_lin - win_left_x) / 2.0;
	    }
	    else
	    {
		/*  Near the right  */
		tmp = (win_right_x - x_lin) / 2.0;
	    }
	    left_x = x_lin - tmp;
	    right_x = x_lin + tmp;
	    if ( fabs (y_lin - win_bottom_y) < fabs (win_top_y - y_lin) )
	    {
		/*  Near the bottom  */
		tmp = (y_lin - win_bottom_y) / 2.0;
	    }
	    else
	    {
		/*  Near the top  */
		tmp = (win_top_y - y_lin) / 2.0;
	    }
	    bottom_y = y_lin - tmp;
	    top_y = y_lin + tmp;
	    canvas_set_attributes (dragzoom->worldcanvas,
				   CANVAS_ATT_LEFT_X, left_x,
				   CANVAS_ATT_RIGHT_X, right_x,
				   CANVAS_ATT_BOTTOM_Y, bottom_y,
				   CANVAS_ATT_TOP_Y, top_y,
				   CANVAS_ATT_END);
	    viewimg_set_canvas_attributes (dragzoom->worldcanvas,
					   VIEWIMG_ATT_AUTO_X, FALSE,
					   VIEWIMG_ATT_AUTO_Y, FALSE,
					   VIEWIMG_ATT_END);
	    if ( !kwin_resize (dragzoom->pixcanvas, TRUE, 0, 0, 0, 0) )
	    {
		fprintf (stderr, "%s: Error refreshing window\n",
			 function_name);
	    }
	    return (TRUE);
	}
	else if (strcmp (string, "o") == 0)
	{
	    /*  Zoom out 2x  */
	    tmp = (win_right_x - win_left_x) / 2.0;
	    left_x = win_left_x - tmp;
	    right_x = win_right_x + tmp;
	    tmp = (win_top_y - win_bottom_y) / 2.0;
	    bottom_y = win_bottom_y - tmp;
	    top_y = win_top_y + tmp;
	    canvas_set_attributes (dragzoom->worldcanvas,
				   CANVAS_ATT_LEFT_X, left_x,
				   CANVAS_ATT_RIGHT_X, right_x,
				   CANVAS_ATT_BOTTOM_Y, bottom_y,
				   CANVAS_ATT_TOP_Y, top_y,
				   CANVAS_ATT_END);
	    viewimg_set_canvas_attributes (dragzoom->worldcanvas,
					   VIEWIMG_ATT_AUTO_X, FALSE,
					   VIEWIMG_ATT_AUTO_Y, FALSE,
					   VIEWIMG_ATT_END);
	    if ( !kwin_resize (dragzoom->pixcanvas, TRUE, 0, 0, 0, 0) )
	    {
		fprintf (stderr, "%s: Error refreshing window\n",
			 function_name);
	    }
	    return (TRUE);
	}
	else return (FALSE);
    }
    else if (event_code == K_CANVAS_EVENT_MODIFIED_KEY_PRESS)
    {
	m_copy ( (char *) &mask, string + strlen (string) + 1, sizeof mask );
	if ( (strcmp (string, "s") == 0) &&
	     (mask == K_CANVAS_EVENT_LEFT_MOUSE_MASK) )
	{
	    /*  Only compute statistics for the box  */
	    if (!dragzoom->in_drag) return (TRUE);
	    dragzoom->in_drag = FALSE;
	    vimage = viewimg_get_active (dragzoom->worldcanvas);
	    if (vimage == NULL) return (TRUE);
	    viewimg_statistics_compute (vimage,
					dragzoom->first_lx, dragzoom->first_ly,
					dragzoom->second_lx,
					dragzoom->second_ly);
	    return (TRUE);
	}
	else return (FALSE);
    }
    /*  Have to do something with the zoom box  */
    if (event_code == K_CANVAS_EVENT_LEFT_MOUSE_CLICK)
    {
	dragzoom->first_px = px;
	dragzoom->first_py = py;
	dragzoom->first_lx = x_lin;
	dragzoom->first_ly = y_lin;
	dragzoom->second_px = px;
	dragzoom->second_py = py;
	dragzoom->second_lx = x_lin;
	dragzoom->second_ly = y_lin;
	dragzoom->in_drag = TRUE;
	return (TRUE);
    }
    if (event_code == K_CANVAS_EVENT_LEFT_MOUSE_RELEASE)
    {
	if (!dragzoom->in_drag) return (TRUE);
	dragzoom->second_px = px;
	dragzoom->second_py = py;
	dragzoom->second_lx = x_lin;
	dragzoom->second_ly = y_lin;
	dragzoom->in_drag = FALSE;
	/*  Reject infinite zoom  */
	if ( (dragzoom->first_px == dragzoom->second_px) ||
	     (dragzoom->first_py == dragzoom->second_py) ) return (TRUE);
	/*  Zoom  */
	left_x = dragzoom->first_lx;
	bottom_y = dragzoom->first_ly;
	right_x = dragzoom->second_lx;
	top_y = dragzoom->second_ly;
	/*  Clip to canvas boundaries  */
#ifdef DEBUG
	fprintf (stderr, "wlx: %e  wrx: %e\n", win_left_x, win_right_x);
	fprintf (stderr, "lx: %e  lr: %e    ", left_x, right_x);
#endif
	if (win_left_x < win_right_x)
	{
	    if (right_x < left_x) swap (left_x, right_x);
	}
	else if (right_x > left_x) swap (left_x, right_x);
#ifdef DEBUG
	fprintf (stderr, "lx: %e  lr: %e\n", left_x, right_x);
#endif
	if (win_bottom_y < win_top_y)
	{
	    if (top_y < bottom_y) swap (bottom_y, top_y);
	}
	else if (top_y > bottom_y) swap (bottom_y, top_y);
	canvas_set_attributes (dragzoom->worldcanvas,
			       CANVAS_ATT_LEFT_X, left_x,
			       CANVAS_ATT_RIGHT_X, right_x,
			       CANVAS_ATT_BOTTOM_Y, bottom_y,
			       CANVAS_ATT_TOP_Y, top_y,
			       CANVAS_ATT_END);
	viewimg_set_canvas_attributes (dragzoom->worldcanvas,
				       VIEWIMG_ATT_AUTO_X, FALSE,
				       VIEWIMG_ATT_AUTO_Y, FALSE,
				       VIEWIMG_ATT_END);
	if ( !kwin_resize (dragzoom->pixcanvas, TRUE, 0, 0, 0, 0) )
	{
	    fprintf (stderr, "%s: Error refreshing window\n",
		     function_name);
	}
	return (TRUE);
    }
    if (event_code != K_CANVAS_EVENT_LEFT_MOUSE_DRAG)
    {
	dragzoom->in_drag = FALSE;
	return (FALSE);
    }
    /*  Drag to new position: undraw the zoom box and draw the next one  */
    if (!dragzoom->in_drag) return (TRUE);
    clear_zoom_box (dragzoom);
    dragzoom->second_px = px;
    dragzoom->second_py = py;
    dragzoom->second_lx = x_lin;
    dragzoom->second_ly = y_lin;
    if (dragzoom->first_px < dragzoom->second_px)
    {
	px = dragzoom->first_px;
	width = dragzoom->second_px - dragzoom->first_px;
    }
    else
    {
	px = dragzoom->second_px;
	width = dragzoom->first_px - dragzoom->second_px;
    }
    if (dragzoom->first_py < dragzoom->second_py)
    {
	py = dragzoom->first_py;
	height = dragzoom->second_py - dragzoom->first_py;
    }
    else
    {
	py = dragzoom->second_py;
	height = dragzoom->first_py - dragzoom->second_py;
    }
    kwin_get_attributes (dragzoom->pixcanvas,
			 KWIN_ATT_VISUAL, &visual,
			 KWIN_ATT_END);
    if (visual == KWIN_VISUAL_PSEUDOCOLOUR)
    {
	if ( !kwin_get_colour (dragzoom->pixcanvas, "green", &pixel_value,
			       NULL, NULL, NULL) )
	{
	    /*  Can't get green: try white instead  */
	    if ( !kwin_get_colour (dragzoom->pixcanvas, "white", &pixel_value,
				   NULL, NULL, NULL) )
	    {
		/*  Can't even get white (this should never actually happen
		    with X), so just pick 0  */
		pixel_value = 0;
	    }
	}
    }
    else
    {
	kwin_get_attributes (dragzoom->pixcanvas,
			     KWIN_ATT_PIX_GREEN_MASK, &pixel_value,
			     KWIN_ATT_END);
    }
    kwin_draw_rectangle (dragzoom->pixcanvas, px, py, width, height,
			 pixel_value);
    return (TRUE);
}   /*  End Function position_func  */

static void clear_zoom_box (KDragZoom dragzoom)
/*  [SUMMARY] Clear the zoom box.
    <dragzoom> The KDragZoom object.
    [RETURNS] Nothing.
*/
{
    int x0, y0, x1, y1;
    KPixCanvasRefreshArea areas[4];

    if (dragzoom->first_px > dragzoom->second_px)
    {
	x0 = dragzoom->second_px;
	x1 = dragzoom->first_px;
    }
    else
    {
	x0 = dragzoom->first_px;
	x1 = dragzoom->second_px;
    }
    if (dragzoom->first_py > dragzoom->second_py)
    {
	y0 = dragzoom->second_py;
	y1 = dragzoom->first_py;
    }
    else
    {
	y0 = dragzoom->first_py;
	y1 = dragzoom->second_py;
    }
    /*  Define top horizontal line  */
    areas[0].startx = x0 - 1;
    areas[0].endx = x1 + 1;
    areas[0].starty = y0 - 1;
    areas[0].endy = y0 + 1;
    areas[0].clear = TRUE;
    /*  Define bottom horizontal line  */
    areas[1].startx = x0 - 1;
    areas[1].endx = x1 + 1;
    areas[1].starty = y1 - 1;
    areas[1].endy = y1 + 1;
    areas[1].clear = TRUE;
    /*  Define left vertical line  */
    areas[2].startx = x0 - 1;
    areas[2].endx = x0 + 1;
    areas[2].starty = y0 - 1;
    areas[2].endy = y1 + 1;
    areas[2].clear = TRUE;
    /*  Define right vertical line  */
    areas[3].startx = x1 - 1;
    areas[3].endx = x1 + 1;
    areas[3].starty = y0 - 1;
    areas[3].endy = y1 + 1;
    areas[3].clear = TRUE;
    viewimg_partial_refresh (dragzoom->worldcanvas, 4, areas);
}   /*  End Function clear_zoom_box  */

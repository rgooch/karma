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

    Last updated by Richard Gooch   17-JUN-1996: Made use of new
  <canvas_convert_to_canvas_coords> routine so that linear to non-linear
  transformation could be avoided.


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
{(void) fprintf (stderr, "NULL viewable image passed\n"); \
 a_prog_bug (function_name); } \
if (dragzoom->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid KDragZoom object\n"); \
 a_prog_bug (function_name); }


/*  Private structures  */

typedef struct
{
    unsigned int magic_number;
    KWorldCanvas worldcanvas;
    KPixCanvas pixcanvas;
    int first_x;
    int first_y;
    int second_x;
    int second_y;
    flag in_drag;
    KCallbackFunc pos_func;
} * KDragZoom;


/*  Private functions  */
STATIC_FUNCTION (flag position_func,
		 (KPixCanvas canvas, int x, int y,
		  unsigned int event_code, void *e_info, void **f_info) );
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
    int width, height;
    unsigned int visual;
    unsigned long pixel_value;
    double px, py;
    double left_x, right_x, bottom_y, top_y, tmp;
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
    /*  Discard spurious mouse move events  */
    if (event_code == K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    if (event_code == K_CANVAS_EVENT_PLAIN_KEY_PRESS)
    {
	if (strcmp (string, "u") != 0) return (FALSE);
	/*  Unzoom  */
	dragzoom->in_drag = FALSE;
	viewimg_set_canvas_attributes (dragzoom->worldcanvas,
				       VIEWIMG_ATT_AUTO_X, TRUE,
				       VIEWIMG_ATT_AUTO_Y, TRUE,
				       VIEWIMG_ATT_END);
	/*  Redraw the canvas  */
	if ( !kwin_resize (dragzoom->pixcanvas, TRUE, 0, 0, 0, 0) )
	{
	    (void) fprintf (stderr, "%s: Error refreshing window\n",
			    function_name);
	}
	return (TRUE);
    }
    if (event_code == K_CANVAS_EVENT_LEFT_MOUSE_CLICK)
    {
	dragzoom->first_x = x;
	dragzoom->first_y = y;
	dragzoom->in_drag = TRUE;
	return (TRUE);
    }
    if (event_code == K_CANVAS_EVENT_LEFT_MOUSE_RELEASE)
    {
	if (!dragzoom->in_drag) return (TRUE);
	dragzoom->second_x = x;
	dragzoom->second_y = y;
	dragzoom->in_drag = FALSE;
	/*  Reject infinite zoom  */
	if ( (dragzoom->first_x == dragzoom->second_x) ||
	     (dragzoom->first_y == dragzoom->second_y) ) return (TRUE);
	/*  Zoom  */
	canvas_get_attributes (dragzoom->worldcanvas,
			       CANVAS_ATT_LEFT_X, &win_left_x,
			       CANVAS_ATT_RIGHT_X, &win_right_x,
			       CANVAS_ATT_BOTTOM_Y, &win_bottom_y,
			       CANVAS_ATT_TOP_Y, &win_top_y,
			       CANVAS_ATT_END);
	px = dragzoom->first_x;
	py = dragzoom->first_y;
	canvas_convert_to_canvas_coords (dragzoom->worldcanvas, TRUE, 1,
					 &px, &py,
					 &left_x, &bottom_y,
					 NULL, NULL);
	px = dragzoom->second_x;
	py = dragzoom->second_y;
	canvas_convert_to_canvas_coords (dragzoom->worldcanvas, TRUE, 1,
					 &px, &py,
					 &right_x, &top_y,
					 NULL, NULL);
	/*  Clip to canvas boundaries  */
#ifdef DEBUG
	(void) fprintf (stderr, "wlx: %e  wrx: %e\n", win_left_x, win_right_x);
	(void) fprintf (stderr, "lx: %e  lr: %e    ", left_x, right_x);
#endif
	if (win_left_x < win_right_x)
	{
	    if (right_x < left_x) swap (left_x, right_x);
	}
	else if (right_x > left_x) swap (left_x, right_x);
#ifdef DEBUG
	(void) fprintf (stderr, "lx: %e  lr: %e\n", left_x, right_x);
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
	    (void) fprintf (stderr, "%s: Error refreshing window\n",
			    function_name);
	}
	return (TRUE);
    }
    if (event_code != K_CANVAS_EVENT_LEFT_MOUSE_DRAG)
    {
	dragzoom->in_drag = FALSE;
	return (FALSE);
    }
    /*  Undraw the zoom box and draw the next one  */
    clear_zoom_box (dragzoom);
    dragzoom->second_x = x;
    dragzoom->second_y = y;
    if (dragzoom->first_x < dragzoom->second_x)
    {
	x = dragzoom->first_x;
	width = dragzoom->second_x - dragzoom->first_x;
    }
    else
    {
	x = dragzoom->second_x;
	width = dragzoom->first_x - dragzoom->second_x;
    }
    if (dragzoom->first_y < dragzoom->second_y)
    {
	y = dragzoom->first_y;
	height = dragzoom->second_y - dragzoom->first_y;
    }
    else
    {
	y = dragzoom->second_y;
	height = dragzoom->first_y - dragzoom->second_y;
    }
    kwin_get_attributes (dragzoom->pixcanvas,
			 KWIN_ATT_VISUAL, &visual,
			 KWIN_ATT_END);
    if (visual == KWIN_VISUAL_PSEUDOCOLOUR)
    {
	if ( !kwin_get_colour (dragzoom->pixcanvas, "green", &pixel_value,
			       NULL, NULL, NULL) ) return (TRUE);
    }
    else
    {
	kwin_get_attributes (dragzoom->pixcanvas,
			     KWIN_ATT_PIX_GREEN_MASK, &pixel_value,
			     KWIN_ATT_END);
    }
    kwin_draw_rectangle (dragzoom->pixcanvas, x, y, width, height,pixel_value);
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

    if (dragzoom->first_x > dragzoom->second_x)
    {
	x0 = dragzoom->second_x;
	x1 = dragzoom->first_x;
    }
    else
    {
	x0 = dragzoom->first_x;
	x1 = dragzoom->second_x;
    }
    if (dragzoom->first_y > dragzoom->second_y)
    {
	y0 = dragzoom->second_y;
	y1 = dragzoom->first_y;
    }
    else
    {
	y0 = dragzoom->first_y;
	y1 = dragzoom->second_y;
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
    (void) viewimg_partial_refresh (dragzoom->worldcanvas, 4, areas);
}   /*  End Function clear_zoom_box  */

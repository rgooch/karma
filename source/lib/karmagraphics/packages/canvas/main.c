/*LINTLIBRARY*/
/*  main.c

    This code provides KWorldCanvas objects.

    Copyright (C) 1993,1994,1995  Richard Gooch

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

/*  This file contains all routines needed for manipulating a World Canvas
    (window with world co-ordinates) independent of the graphics system in use.


    Written by      Richard Gooch   16-APR-1993

    Updated by      Richard Gooch   27-APR-1993

    Updated by      Richard Gooch   3-JUN-1993: Improved symmetry in
  co-ordinate conversion routines.

    Updated by      Richard Gooch   15-OCT-1993: Documented placement of
  KWorldCanvas origin.

    Updated by      Richard Gooch   21-NOV-1993: Added  canvas_draw_point_p
  ,  canvas_draw_line_p  ,  canvas_fill_ellipse_p  ,  canvas_draw_rectangle
  ,  canvas_draw_rectangle_p  ,  canvas_fill_rectangle  and
  canvas_fill_rectangle_p  routines.

    Updated by      Richard Gooch   23-NOV-1993: Added
  canvas_register_convert_func  .

    Updated by      Richard Gooch   26-NOV-1993: Added  canvas_draw_lines  ,
  canvas_draw_lines_p  and  canvas_get_pixcanvas  .

    Updated by      Richard Gooch   29-NOV-1993: Added commented test for
  magic_number  field of  win_scale_type  structure in  verify_win_scale  .Will
  be activated in version 2.0 of Karma. Added  canvas_draw_*_c  routines.

    Updated by      Richard Gooch   2-DEC-1993: Removed  canvas_draw_*_c
  routines and added  canvas_get_colour  routine.

    Updated by      Richard Gooch   6-DEC-1993: Added  canvas_specify  and
  canvas_get_specification  routines.

    Updated by      Richard Gooch   8-DEC-1993: Added  canvas_set_dressing  .

    Updated by      Richard Gooch   11-DEC-1993: Added  canvas_draw_dressing  .

    Updated by      Richard Gooch   19-MAY-1994: Modified  canvas_get_colour
  to also give RGB colour values.

    Updated by      Richard Gooch   28-JUL-1994: Added  canvas_get_cmap  .

    Updated by      Richard Gooch   2-AUG-1994: Allowed passing of NULL
  Kcolourmap  to  canvas_create  .

    Updated by      Richard Gooch   8-AUG-1994: Created  canvas_draw_rgb_image

    Updated by      Richard Gooch   2-OCT-1994: Created  canvas_coord_transform

    Updated by      Richard Gooch   22-NOV-1994: Created
  canvas_unregister_position_event_func  .

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/canvas/main.c

    Last updated by Richard Gooch   28-NOV-1994: Made use of  c_  package.

    Last updated by Richard Gooch   2-JAN-1995: Took account of change to
  kwin_draw_rgb_image  .


*/
#include <stdio.h>
#include <math.h>
#include <varargs.h>
#include <karma.h>
#define KWIN_GENERIC_ONLY
#define KCANVAS_INTERNAL
#include <karma_canvas.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_c.h>


#define MAGIC_NUMBER (unsigned int) 357474556

#define DRESSING_ITERATE_MAX (unsigned int) 10
#define DEFAULT_DRESSING_COLOUR "green"
#define MAJOR_TICK_PIXELS (unsigned int) 12
#define MEDIUM_TICK_PIXELS (unsigned int) 8
#define MINOR_TICK_PIXELS (unsigned int) 4
#define MAX_MAJOR_TICKS 11
#define MAX_MINOR_TICKS 55
#define MAX_LINEAR_SPACINGS (unsigned int) 3
#define NUM_MINOR_TICKS_PER_MAJOR_TICK (double) 10.0
#define NUM_MINOR_TICKS_PER_MEDIUM_TICK (double) 5.0
#define LIN_SCALE_CUTOFF1 (double) 4.0
#define LIN_SCALE_CUTOFF2 (double) -2.0
#define ABS_SCALE_SPACING_PIXELS (unsigned int) 40
#define TICK_BUF_SIZE 256

#define VERIFY_CANVAS(canvas) if (canvas == NULL) \
{(void) fprintf (stderr, "NULL canvas passed\n"); \
 a_prog_bug (function_name); } \
if (canvas->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid canvas object\n"); \
 a_prog_bug (function_name); }


/*  Internal definition of KWorldCanvas object structure type  */
struct worldcanvas_type
{
    unsigned int magic_number;
    KPixCanvas pixcanvas;
    Kcolourmap cmap;
    struct win_scale_type win_scale;
    /*  Callback information  */
    void (*size_control_func) ();
    void *size_control_info;
    flag (*coord_convert_func) ();
    void *coord_convert_info;
    KCallbackList refresh_list;
    KCallbackList position_list;
    flag quash_negotiate;
    flag in_size_control_func;
    void (*coord_transform_func) ();
    void *coord_transform_info;
    /*  Specification information  */
    char *specify_xlabel;
    char *specify_ylabel;
    unsigned int num_restrictions;
    char **restriction_names;
    double *restriction_values;
    /*  Dressing display parameters  */
    flag dressing_drawn;
    flag display_dressing;
    flag show_top_ticks;
    flag show_bottom_ticks;
    flag show_left_ticks;
    flag show_right_ticks;
    flag internal_ticks;
    char *axes_colour;
    char *axes_fontname;
    char *scale_colour;
    char *scale_fontname;
    char *dressing_hlabel;
    char *dressing_vlabel;
    char *label_colour;
    char *label_fontname;
    char *title;
    char *title_colour;
    char *title_fontname;
};

struct refresh_struct
{
    int width;
    int height;
    flag cmap_resize;
};

struct position_struct
{
    double x;
    double y;
    unsigned int event_code;
    void *e_info;
    double x_lin;
    double y_lin;
};


/*  Private data  */


/*  Local functions  */
void canvas_set_dressing ();


/*  Private functions  */
STATIC_FUNCTION (void pixcanvas_refresh_func,
		 (KPixCanvas pixcanvas, int width, int height, void **info) );
STATIC_FUNCTION (void cmap_resize_func, (Kcolourmap cmap, void **info) );
STATIC_FUNCTION (void refresh_canvas,
		 (KWorldCanvas canvas, int width, int height,
		  flag cmap_resize) );
STATIC_FUNCTION (unsigned long get_pixel_from_value,
		 (double value[2], struct win_scale_type *win_scale,
		  Kcolourmap cmap) );
STATIC_FUNCTION (flag pixcanvas_position_event,
		 (KPixCanvas pixcanvas, int x, int y, unsigned int event_code,
		  void *event_info, void **f_info) );
STATIC_FUNCTION (void verify_win_scale, (struct win_scale_type *win_scale,
					 char *function_name) );
STATIC_FUNCTION (flag negotiate_scale_change,
		 (KWorldCanvas canvas, int width, int height,
		  flag *boundary_clear) );
STATIC_FUNCTION (void get_dressing_size,
		 (KWorldCanvas canvas, int *p_left, int *p_right,
		  int *p_top, int *p_bottom) );
STATIC_FUNCTION (void dressing_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize, void **info) );
STATIC_FUNCTION (flag pixel_to_world_coord_convert,
		 (KWorldCanvas canvas, int xin, int yin,
		  double *xout, double *yout, double *x_lin, double *y_lin) );
STATIC_FUNCTION (flag refresh_event_func,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );
STATIC_FUNCTION (flag position_event_func,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KWorldCanvas canvas_create (KPixCanvas pixcanvas, Kcolourmap cmap,
			    struct win_scale_type *win_scale)
/*  This routine will create a world canvas, ready for drawing, from a
    KPixCanvas (pixel canvas) object.
    Note that the origin of a KWorldCanvas is the lower-left corner.
    The pixel canvas must be given by  pixcanvas  .
    The colourmap must be given by  cmap  .If this is NULL, the canvas will
    only permit drawing using pixel values. This precludes the use of drawing
    images and drawing geometric primitives using data values.
    The window scaling information must be pointed to by  win_scale  .The
        x_offset  x_pixels  y_offset  y_pixels  fields are determined
    internally.
    The routine returns a world canvas on success, else it returns NULL.
*/
{
    KWorldCanvas canvas;
    int width, height;
    static char function_name[] = "canvas_create";

    if (pixcanvas == NULL)
    {
	(void) fprintf (stderr, "NULL KPixCanvas passed\n");
	a_prog_bug (function_name);
    }
    if (win_scale == NULL)
    {
	(void) fprintf (stderr,
			"NULL window scaling structure pointer passed\n");
	a_prog_bug (function_name);
    }
    verify_win_scale (win_scale, function_name);
    kwin_get_size (pixcanvas, &width, &height);
    if ( ( canvas = (KWorldCanvas) m_alloc (sizeof *canvas) ) == NULL )
    {
	m_error_notify (function_name, "world canvas");
	return (NULL);
    }
    canvas->magic_number = MAGIC_NUMBER;
    canvas->pixcanvas = pixcanvas;
    canvas->cmap = cmap;
    m_copy ( (char *) &canvas->win_scale, (char *) win_scale,
	    sizeof *win_scale );
    canvas->win_scale.x_offset = 0;
    canvas->win_scale.x_pixels = width;
    canvas->win_scale.y_offset = 0;
    canvas->win_scale.y_pixels = height;
    canvas->size_control_func = NULL;
    canvas->size_control_info = NULL;
    canvas->coord_convert_func = NULL;
    canvas->coord_convert_info = NULL;
    canvas->coord_transform_func = NULL;
    canvas->coord_transform_info = NULL;
    canvas->refresh_list = NULL;
    canvas->position_list = NULL;
    canvas->quash_negotiate = FALSE;
    canvas->in_size_control_func = FALSE;
    canvas->specify_xlabel = NULL;
    canvas->specify_ylabel = NULL;
    canvas->num_restrictions = 0;
    canvas->restriction_names = NULL;
    canvas->restriction_values = NULL;
    canvas->dressing_drawn = FALSE;
    canvas->display_dressing = FALSE;
    canvas->show_top_ticks = FALSE;
    canvas->show_bottom_ticks = FALSE;
    canvas->show_left_ticks = FALSE;
    canvas->show_right_ticks = FALSE;
    canvas->internal_ticks = FALSE;
    canvas->axes_colour = NULL;
    canvas->axes_fontname = NULL;
    canvas->scale_colour = NULL;
    canvas->scale_fontname = NULL;
    canvas->dressing_hlabel = NULL;
    canvas->dressing_vlabel = NULL;
    canvas->label_colour = NULL;
    canvas->label_fontname = NULL;
    canvas->title = NULL;
    canvas->title_colour = NULL;
    canvas->title_fontname = NULL;
    /*  Process refreshes from lower down  */
    kwin_register_refresh_func (pixcanvas, pixcanvas_refresh_func,
				(void *) canvas);
    /*  Process position events from lower down  */
    (void) kwin_register_position_event_func (pixcanvas,
					      pixcanvas_position_event,
					      (void *) canvas);
    /*  Process colourmap resizes  */
    if (cmap != NULL) kcmap_register_resize_func (cmap, cmap_resize_func,
						  (void *) canvas);
    return (canvas);
}   /*  End Function canvas_create  */

/*PUBLIC_FUNCTION*/
KCallbackFunc canvas_register_refresh_func (KWorldCanvas canvas,
					    void (*refresh_func) (),void *info)
/*  This routine will register a refresh function for a world canvas. The
    refresh function will be called whenever the contents of the canvas need to
    be redrawn. Many refresh functions may be registered per canvas. The first
    function registered is the first function called upon refresh.
    The canvas must be given by  canvas  .
    The function that is called when the canvas is to be refreshed must be
    pointed to by  refresh_func  .
    The interface to this routine is as follows:

    void refresh_func (canvas, width, height, win_scale, cmap,cmap_resize,info)
    *   This routine is a refresh event consumer for a world canvas.
        The canvas is given by  canvas  .
	The width of the canvas in pixels is given by  width  .
	The height of the canvas in pixels is given by  height  .
	The window scaling information is pointed to by  win_scale  .
	The colourmap associated with the canvas is given by  cmap  .
	If the refresh function was called as a result of a colourmap resize
	the value of  cmap_resize  will be TRUE.
	The arbitrary canvas information pointer is pointed to by  info  .
	The routine returns nothing.
    *
    KWorldCanvas canvas;
    int width;
    int height;
    struct win_scale_type *win_scale;
    Kcolourmap cmap;
    flag cmap_resize;
    void **info;

    The initial arbitrary canvas information pointer must be given by  info  .
    The routine returns a KCallbackFunc object.
*/
{
    static char function_name[] = "canvas_register_refresh_func";

    VERIFY_CANVAS (canvas);
    return ( c_register_callback (&canvas->refresh_list,
				  refresh_event_func,
				  canvas,
				  info, TRUE, (void *) refresh_func, FALSE,
				  FALSE) );
}   /*  End Function canvas_register_refresh_func  */

/*PUBLIC_FUNCTION*/
void canvas_register_size_control_func (KWorldCanvas canvas,
					void (*size_control_func) (),
					void *info)
/*  This routine will register the size control function for a world canvas.
    This function will be called whenever the lower level pixel canvas resizes,
    or the associated Kcolourmap object is resized, OR a call is made to
    canvas_resize  .
    This function may alter the window scaling information for the canvas.
    Only one size control function is permitted per canvas. The size control
    routine is called before any refresh functions are called. This is
    a means for a higher level object to take control of the world canvas.
    The canvas must be given by  canvas  .
    The function that is called when the canvas is to be refreshed must be
    pointed to by  size_control_func  .
    The interface to this routine is as follows:

    void size_control_func (canvas, width, height, win_scale, info,
                            boundary_clear)
    *   This routine will modify the window scaling information for a world
        canvas. While this routine is running, colourmap resize events are
	ignored. Hence this routine may safely cause the associated Kcolourmap
	object to be resized.
        The canvas is given by  canvas  .
	The width of the canvas in pixels is given by  width  .
	The height of the canvas in pixels is given by  height  .
	The window scaling information is pointed to by  win_scale  .The data
	herein may be modified.
	The arbitrary canvas information pointer is pointed to by  info  .
	If the value TRUE is written to the storage pointed to by
	boundary_clear  then the  canvas_resize  routine will attempt to clear
	only the boundary between the pixel canvas and the world canvas. If
	the value FALSE is written here or nothing is written here, the
	canvas_resize  routine will clear the entire pixel canvas as
	appropriate.
	The routine should return nothing.
    *
    KWorldCanvas canvas;
    int width;
    int height;
    struct win_scale_type *win_scale;
    void **info;
    flag *boundary_clear;

    The initial arbitrary canvas information pointer must be given by  info  .
    The routine returns nothing.
*/
{
    static char function_name[] = "canvas_register_size_control_func";

    VERIFY_CANVAS (canvas);
    if (size_control_func == NULL) return;
    if (canvas->size_control_func != NULL)
    {
	(void) fprintf (stderr, "size_control_func already registered\n");
	a_prog_bug (function_name);
    }
    canvas->size_control_func = size_control_func;
    canvas->size_control_info = info;
}   /*  End Function canvas_register_size_control_func  */

/*PUBLIC_FUNCTION*/
KCallbackFunc canvas_register_position_event_func (KWorldCanvas canvas,
						   flag (*func) (),
						   void *f_info)
/*  This routine will register a position event function for a world canvas.
    The position event function will be called whenever a position event on the
    canvas has not been consumed. Many position event functions may be
    registered per canvas. The first function registered is the first function
    called upon a position event.
    The canvas must be given by  canvas  .
    The function that is called when a position event occurs must be pointed to
    by  func  .
    The interface to this routine is as follows:

    flag func (canvas, x, y, event_code, e_info, f_info, x_lin, y_lin)
    *   This routine is a position event consumer for a world canvas.
        The canvas is given by  canvas  .
	The horizontal world co-ordinate of the event will be given by  x  .
	The vertical world co-ordinate of the event will be given by  y  .
	These values will have been transformed by the registered transform
	function (see  canvas_register_transform_func  ).
	The arbitrary event code is given by  event_code  .
	The arbitrary event information is pointed to by  e_info  .
	The arbitrary function information pointer is pointed to by  f_info  .
	The linear world co-ordinates (the co-ordinates prior to the transform
	function being called) will be given by  x_lin  and  y_lin  .
	The routine returns TRUE if the event was consumed, else it returns
	FALSE indicating that the event is still to be processed.
    *
    KWorldCanvas canvas;
    double x;
    double y;
    unsigned int event_code;
    void *e_info;
    void **f_info;
    double x_lin;
    double y_lin;

    The initial arbitrary function information pointer must be given by  f_info
    The routine returns a KCallbackFunc object.
*/
{
    static char function_name[] = "canvas_register_position_func";

    VERIFY_CANVAS (canvas);
    return ( c_register_callback (&canvas->position_list,
				  position_event_func,
				  canvas, f_info, TRUE, (void *) func, FALSE,
				  TRUE) );
}   /*  End Function canvas_register_position_func  */

/*PUBLIC_FUNCTION*/
flag canvas_resize (KWorldCanvas canvas, struct win_scale_type *win_scale,
		    flag always_clear)
/*  This routine will register a resize in the world canvas size. This will
    cause any refresh routines registered for the canvas to be called. The
    associated pixel canvas is refreshed, hence, refresh routines registered
    for the pixel canvas will also be called.
    The canvas must be given by  canvas  .
    The new window scaling information must be pointed to by  win_scale  .If
    this is NULL no change is made to the window scaling information, and a
    simple refresh occurs.
    If  always_clear  is FALSE,  win_scale  is NULL and there is a size
    control function registered for the world canvas, then if that function
    does not change the values of the  x_offset  ,  x_pixels  ,  y_offset  or
    y_pixels  fields in the window scaling information, the associated pixel
    canvas is NOT cleared.
    Under all other circumstances, the pixel canvas is cleared prior to calling
    any refresh functions.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    KPixCanvas pixcanvas;
    flag clear = TRUE, boundary_clear = FALSE;
    int x_offset, x_pixels, y_offset, y_pixels;
    int width, height;
    static char function_name[] = "canvas_resize";

    VERIFY_CANVAS (canvas);
    FLAG_VERIFY (always_clear);
    verify_win_scale (win_scale, function_name);
    /*  Get pixel canvas size  */
    kwin_get_size (canvas->pixcanvas, &width, &height);
    if ( (win_scale == NULL) && (!always_clear) &&
	(canvas->size_control_func != NULL) )
    {
	x_offset = canvas->win_scale.x_offset;
	x_pixels = canvas->win_scale.x_pixels;
	y_offset = canvas->win_scale.y_offset;
	y_pixels = canvas->win_scale.y_pixels;
	/*  Call size control function first  */
	if ( !negotiate_scale_change (canvas, width, height, &boundary_clear) )
	{
	    (void) fprintf (stderr, "Pixel canvas not big enough\n");
	    return (FALSE);
	}
	FLAG_VERIFY (boundary_clear);
/*
	if ( (x_offset == canvas->win_scale.x_offset) &&
	    (x_pixels == canvas->win_scale.x_pixels) &&
	    (y_offset == canvas->win_scale.y_offset) &&
	    (y_pixels == canvas->win_scale.y_pixels) )
	{
	    if (boundary_clear) clear = FALSE;
	}
*/
	if (boundary_clear) clear = TRUE;
	canvas->quash_negotiate = TRUE;
    }
    if (win_scale != NULL)
    {
	m_copy ( (char *) &canvas->win_scale, (char *) win_scale,
		sizeof *win_scale );
    }
    if (clear && boundary_clear)
    {
	/*  Clear only the boundary. First the top  */
	kwin_clear (canvas->pixcanvas, 0, 0,
		    -1, canvas->win_scale.y_offset);
	/*  Now the bottom  */
	kwin_clear (canvas->pixcanvas,
		    0,
		    canvas->win_scale.y_offset +canvas->win_scale.y_pixels,
		    -1, -1);
	/*  Now the bit at the left between the top and bottom bits  */
	kwin_clear (canvas->pixcanvas,
		    0, canvas->win_scale.y_offset,
		    canvas->win_scale.x_offset,canvas->win_scale.y_pixels);
	/*  And finally the bit at the right between the top and bottom bits */
	kwin_clear (canvas->pixcanvas,
		    canvas->win_scale.x_offset +canvas->win_scale.x_pixels,
		    canvas->win_scale.y_offset,
		    -1, canvas->win_scale.y_pixels);
	return ( kwin_resize (canvas->pixcanvas, FALSE, 0, 0, 0, 0) );
    }
    return ( kwin_resize (canvas->pixcanvas, TRUE, 0, 0, 0, 0) );
}   /*  End Function canvas_resize  */

/*PUBLIC_FUNCTION*/
void canvas_get_size (KWorldCanvas canvas, int *width, int *height,
		      struct win_scale_type *win_scale)
/*  This routine will get the size of a world canvas.
    The world canvas must be given by  canvas  .
    The number of horizontal pixels will be written to the storage pointed to
    by  width  .
    The number of vertical pixels will be written to the storage pointed to by
    height  .
    The window scaling information will be written to the storage pointed to by
    win_scale  .
    The routine returns nothing.
*/
{
    static char function_name[] = "canvas_get_size";

    VERIFY_CANVAS (canvas);
    if (win_scale == NULL)
    {
	(void) fprintf (stderr, "NULL win_scale structure pointer passed\n");
	a_prog_bug (function_name);
    }
    kwin_get_size (canvas->pixcanvas, width, height);
    m_copy ( (char *) win_scale, (char *) &canvas->win_scale,
	    sizeof *win_scale );
}   /*  End Function canvas_get_size  */

/*PUBLIC_FUNCTION*/
flag canvas_specify (KWorldCanvas canvas, char *xlabel, char *ylabel,
		     unsigned int num_restr, char **restr_names,
		     double *restr_values)
/*  This routine will change the specifications for a canvas. This includes the
    horizontal and vertical dimension labels and any restrictions. The entire
    specification is copied (ie. the inputs may be freed).
    The world canvas must be given by  canvas  .
    The horizontal dimension label must be pointed to by  xlabel  .
    The vertical dimension label must be pointed to by  ylabel  .
    The number of restrictions must be given by  num_restr  .
    The array of restriction names must by pointed to by  restr_names  .
    The restriction values must be pointed to by  restr_values  .
    NOTE: this routine DOES NOT cause the canvas to be refreshed. It is highly
    recommended that the canvas is refreshed after this routine, as higher
    level packages may depend on the specification information.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int count;
    char *xlab;
    char *ylab;
    char **names;
    double *values;
    static char function_name[] = "canvas_specify";

    VERIFY_CANVAS (canvas);
    if (xlabel == NULL)
    {
	xlab = NULL;
    }
    else
    {
	if ( ( xlab = st_dup (xlabel) ) == NULL )
	{
	    m_error_notify (function_name, "horizontal dimension label");
	    return (FALSE);
	}
    }
    if (ylabel == NULL)
    {
	ylab = NULL;
    }
    else
    {
	if ( ( ylab = st_dup (ylabel) ) == NULL )
	{
	    m_error_notify (function_name, "vertical dimension label");
	    m_free (xlab);
	    return (FALSE);
	}
    }
    if (num_restr > 0)
    {
	/*  Restrictions needed  */
	if ( ( names = (char **) m_alloc (sizeof *names * num_restr) ) ==NULL )
	{
	    m_error_notify (function_name, "array of restriction names");
	    if (xlab != NULL) m_free (xlab);
	    if (ylab != NULL) m_free (ylab);
	    return (FALSE);
	}
	if ( ( values = (double *) m_alloc (sizeof *values * num_restr) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of restriction values");
	    if (xlab != NULL) m_free (xlab);
	    if (ylab != NULL) m_free (ylab);
	    m_free ( (char *) names );
	    return (FALSE);
	}
	for (count = 0; count < num_restr; ++count)
	{
	    if ( ( names[count] = st_dup (restr_names[count]) ) == NULL )
	    {
		m_error_notify (function_name, "restriction name");
		if (xlab != NULL) m_free (xlab);
		if (ylab != NULL) m_free (ylab);
		for (; count > 0; --count)
		{
		    m_free (names[count - 1]);
		}
		m_free ( (char *) names );
		m_free ( (char *) values );
		return (FALSE);
	    }
	    values[count] = restr_values[count];
	}
    }
    else
    {
	/*  No restrictions needed  */
	names = NULL;
	values = NULL;
    }
    /*  Remove old info  */
    if (canvas->specify_xlabel != NULL) m_free (canvas->specify_xlabel);
    if (canvas->specify_ylabel != NULL) m_free (canvas->specify_ylabel);
    if (canvas->restriction_names != NULL)
    {
	for (count = 0; count < canvas->num_restrictions; ++count)
	{
	    if (canvas->restriction_names[count] != NULL)
	    {
		m_free (canvas->restriction_names[count]);
	    }
	}
	m_free ( (char *) canvas->restriction_names );
	canvas->restriction_names = NULL;
    }
    if (canvas->restriction_values != NULL)
    {
	m_free ( (char *) canvas->restriction_values );
	canvas->restriction_values = NULL;
    }
    canvas->specify_xlabel = xlab;
    canvas->specify_ylabel = ylab;
    canvas->num_restrictions = num_restr;
    canvas->restriction_names = names;
    canvas->restriction_values = values;
    return (TRUE);
}   /*  End Function canvas_specify  */

/*PUBLIC_FUNCTION*/
void canvas_get_specification (KWorldCanvas canvas, char **xlabel,
			       char **ylabel, unsigned int *num_restr,
			       char ***restr_names, double **restr_values)
/*  This routine will get the specifications for a canvas. This includes the
    horizontal and vertical dimension labels and any restrictions. Only
    pointers are copied (ie. the outputs may NOT be freed).
    The world canvas must be given by  canvas  .
    The pointer to the horizontal dimension label will be written to the
    storage pointed to by  xlabel  .
    The pointer to the vertacal dimension label will be written to the
    storage pointed to by  ylabel  .
    The number of restrictions will be written to the storage pointed to by
    num_restr  .
    The pointer to the array of restriction names will be written to the
    storage pointed to by  restr_names  .
    The pointer to the array of restriction values will be written to the
    storage pointed to by  restr_values  .
    The routine returns nothing.
*/
{
    static char function_name[] = "canvas_get_specification";

    VERIFY_CANVAS (canvas);
    *xlabel = canvas->specify_xlabel;
    *ylabel = canvas->specify_ylabel;
    *num_restr = canvas->num_restrictions;
    *restr_names = canvas->restriction_names;
    *restr_values = canvas->restriction_values;
}   /*  End Function canvas_get_specification  */

/*PUBLIC_FUNCTION*/
flag canvas_convert_to_canvas_coord (KWorldCanvas canvas, int xin, int yin,
				     double *xout, double *yout)
/*  This routine will convert co-ordinates in a pixel canvas to co-ordinates in
    a world canvas.
    The world canvas must be given by  canvas  .
    The lower level horizontal co-ordinate must be given by  xin  .
    The lower level vertical co-ordinate must be given by  yin  .
    The horizontal world co-ordinate will be written to the storage pointed to
    by  xout  .
    The vertical world co-ordinate will be written to the storage pointed to
    by  xout  .
    The routine returns TRUE if the co-ordinate lies within the canvas
    boundaries, else it returns FALSE (although a conversion is still
    performed).
*/
{
    double lx, ly;
    static char function_name[] = "canvas_convert_to_canvas_coord";

    VERIFY_CANVAS (canvas);
    return ( pixel_to_world_coord_convert (canvas, xin, yin, xout, yout,
					   &lx, &ly) );
}   /*  End Function canvas_convert_to_canvas_coord  */

/*PUBLIC_FUNCTION*/
flag canvas_convert_from_canvas_coord (KWorldCanvas canvas,
				       double xin, double yin,
				       int *xout, int *yout)
/*  This routine will convert co-ordinates in a world canvas to co-ordinates in
    a pixel canvas. The world co-ordinates may be non-linear.
    The world canvas must be given by  canvas  .
    The horizontal world co-ordinate must be given by  xin  .
    The vertical world co-ordinate must be given by  yin  .
    The lower level horizontal canvas co-ordinate will be written to the
    storage pointed to by  xout  .
    The lower level vertical canvas co-ordinate will be written to the storage
    pointed to by  xout  .
    The routine returns TRUE if the co-ordinate lies within the canvas
    boundaries, else it returns FALSE (although a conversion is still
    performed).
*/
{
    flag converted = FALSE;
    struct win_scale_type win_scale;
    double px, py;
    double wx, wy;
    int iy;
    static char function_name[] = "canvas_convert_from_canvas_coord";

    VERIFY_CANVAS (canvas);
    (void) canvas_coord_transform (canvas, &xin, &yin, TRUE);
    if (canvas->coord_convert_func != NULL)
    {
	m_copy ( (char *) &win_scale, (char *) &canvas->win_scale,
		sizeof win_scale );
	wx = xin;
	wy = yin;
	converted = ( (*canvas->coord_convert_func)
		     (canvas, &win_scale, xout, yout, &wx, &wy, FALSE,
		      &canvas->coord_convert_info) );
    }
    if (!converted)
    {
	/*  Convert x  */
	px = xin - canvas->win_scale.x_min;
	px /= canvas->win_scale.x_max - canvas->win_scale.x_min;
	px *= (double) (canvas->win_scale.x_pixels - 1);
	px += 0.01;
	/*  Convert y  */
	py = yin - canvas->win_scale.y_min;
	py /= canvas->win_scale.y_max - canvas->win_scale.y_min;
	py *= (double) (canvas->win_scale.y_pixels - 1);
	iy = (int) (py + 0.01);
	/*  Convert world co-ordinates to pixel co-ordinates  */
	*xout = (int) px + canvas->win_scale.x_offset;
	/*  Flip vertical  */
	*yout = canvas->win_scale.y_offset+canvas->win_scale.y_pixels-1-iy;
    }
    if ( (*xout < canvas->win_scale.x_offset) ||
	(*xout >= canvas->win_scale.x_offset + canvas->win_scale.x_pixels) ||
	(*yout < canvas->win_scale.y_offset) ||
	(*yout >= canvas->win_scale.y_offset + canvas->win_scale.y_pixels) )
    return (FALSE);
    return (TRUE);
}   /*  End Function canvas_convert_from_canvas_coord  */

/*PUBLIC_FUNCTION*/
void canvas_register_convert_func (KWorldCanvas canvas,
				   flag (*coord_convert_func) (), void *info)
/*  This routine will register the co-ordinate conversion function for a world
    canvas.
    This function will be called whenever conversions between world
    co-ordinates and pixel co-ordinates are performed.
    Only one co-ordinate conversion function is permitted per canvas. This is
    a means for a higher level object to take control of the world canvas.
    The canvas must be given by  canvas  .
    The function that is called when co-ordinates are to be converted must be
    pointed to by  coord_convert_func  .
    The interface to this routine is as follows:

    flag coord_convert_func (canvas, win_scale, px, py, wx, wy,
                             to_world, info)
    *   This routine will modify the window scaling information for a world
        canvas.
        The canvas is given by  canvas  .
	The window scaling information is pointed to by  win_scale  .The data
	herein may be modified.
	The horizontal pixel co-ordinate storage must be pointed to by  px  .
	The vertical pixel co-ordinate storage must be pointed to by  py  .
	The horizontal world co-ordinate storage must be pointed to by  wx  .
	The vertical world co-ordinate storage must be pointed to by  wy  .
	If the value of  to_world  is TRUE, then a pixel to world co-ordinate
	transform is required, else a world to pixel co-ordinate transform is
	required.
	The arbitrary canvas information pointer is pointed to by  info  .
	The routine should return TRUE if the conversion was completed,
	else it returns FALSE indicating that the default conversions should be
	used.
    *
    KWorldCanvas canvas;
    struct win_scale_type *win_scale;
    int *px;
    int *py;
    double *wx;
    double *wy;
    flag to_world;
    void **info;

    The initial arbitrary canvas information pointer must be given by  info  .
    The routine returns nothing.
*/
{
    static char function_name[] = "canvas_register_convert_func";

    VERIFY_CANVAS (canvas);
    if (coord_convert_func == NULL) return;
    if (canvas->coord_convert_func != NULL)
    {
	(void) fprintf (stderr, "coord_convert_func already registered\n");
	a_prog_bug (function_name);
    }
    canvas->coord_convert_func = coord_convert_func;
    canvas->coord_convert_info = info;
}   /*  End Function canvas_register_convert_func  */

/*PUBLIC_FUNCTION*/
flag canvas_coord_transform (KWorldCanvas canvas, double *x, double *y,
			     flag to_linear)
/*  This routine will transform co-ordinates in a world canvas using the
    registered co-ordinate transform function (see
    canvas_register_transform_func  ).
    The world canvas must be given by  canvas  .
    The horizontal world co-ordinate must be pointed to by  x  .This value is
    modified.
    The vertical world co-ordinate must be pointed to by  y  .This value is
    modified.
    If the value of  to_linear  is TRUE  then the routine will transform
    (possibly) non-linear world co-ordinates to linear world co-ordinates,
    else the transform is from linear to non-linear co-ordinates.
    The routine returns TRUE if the co-ordinate lies within the canvas
    boundaries, else it returns FALSE (although a conversion is still
    performed).
*/
{
    struct win_scale_type win_scale;
    static char function_name[] = "canvas_coord_transform";

    VERIFY_CANVAS (canvas);
    if (canvas->coord_transform_func != NULL)
    {
	(*canvas->coord_transform_func) (x, y, to_linear,
					 canvas->win_scale.x_min,
					 canvas->win_scale.x_max,
					 canvas->win_scale.y_min,
					 canvas->win_scale.y_max,
					 &canvas->coord_transform_info);
    }
    if ( (*x < canvas->win_scale.x_offset) ||
	(*x >= canvas->win_scale.x_offset+canvas->win_scale.x_pixels) ||
	(*y < canvas->win_scale.y_offset) ||
	(*y >= canvas->win_scale.y_offset +canvas->win_scale.y_pixels) )
    return (FALSE);
    return (TRUE);
}   /*  End Function canvas_coord_transform  */

/*PUBLIC_FUNCTION*/
void canvas_register_transform_func (KWorldCanvas canvas,
				     void (*coord_transform_func) (),
				     void *info)
/*  This routine will register the co-ordinate transform function for a world
    canvas. The co-ordinate transform function transforms the linear world
    co-ordinates used internally by the world canvas to be transformed into
    non-linear co-ordinates for external use.
    Only one co-ordinate transform function is permitted per canvas.
    The canvas must be given by  canvas  .
    The function that is called when co-ordinates are to be transformed must be
    pointed to by  coord_transform_func  .
    The interface to this routine is as follows:

    void coord_transform_func (x, y, to_linear, x_min, x_max, y_min,y_max,info)
    *   This routine will transform between linear and non-linear world
        co-ordinates.
        The window scaling information is pointed to by  win_scale  .
	The horizontal world co-ordinate storage must be pointed to by  x  .
	The vertical world co-ordinate storage must be pointed to by  y  .
	If the value of  to_linear  is TRUE, then a non-linear to linear
	transform is required, else a linear to non-linear transform is
	required.
	The arbitrary transform information pointer is pointed to by  info  .
	The routine returns nothing.
    *
    double *x;
    double *y;
    flag to_linear;
    double x_min;
    double x_max;
    double y_min;
    double y_max;
    void **info;

    The initial arbitrary transform information pointer must be given by  info
    The routine returns nothing.
*/
{
    static char function_name[] = "canvas_register_transform_func";

    VERIFY_CANVAS (canvas);
    if (coord_transform_func == NULL) return;
    if (canvas->coord_transform_func != NULL)
    {
	(void) fprintf (stderr, "coord_transform_func already registered\n");
	a_prog_bug (function_name);
    }
    canvas->coord_transform_func = coord_transform_func;
    canvas->coord_transform_info = info;
}   /*  End Function canvas_register_transform_func  */

/*PUBLIC_FUNCTION*/
KPixCanvas canvas_get_pixcanvas (KWorldCanvas canvas)
/*  This routine will get the underlying pixel canvas of a world canvas.
    The world canvas must be given by  canvas  .
    The routine returns the KPixCanvas object.
*/
{
    static char function_name[] = "canvas_get_pixcanvas";

    VERIFY_CANVAS (canvas);
    return (canvas->pixcanvas);
}   /*  End Function canvas_get_pixcanvas  */

/*PUBLIC_FUNCTION*/
flag canvas_get_colour (KWorldCanvas canvas, char *colourname,
			unsigned long *pixel_value, unsigned short *red,
			unsigned short *green, unsigned short *blue)
/*  This routine will get (possibly allocating) a colourcell for a canvas.
    The canvas must be given by  canvas  .
    The name of the colour to get must be pointed to by  colourname  .
    The pixel value will be written to the storage pointed to by  pixel_value
    NOTE: the pixel value is valid ONLY for the underlying pixel canvas.
    ALSO: the pixel value becomes invalid EVERY time the underlying pixel
    canvas is refreshed/ resized. If the pixel canvas is refreshed/ resized,
    this routine MUST be called again.
    The red intensity in the hardware colourmap for the pixel will be written
    to the storage pointed to by  red  .If this is NULL, nothing is written
    here.
    The green intensity in the hardware colourmap for the pixel will be written
    to the storage pointed to by  green  .If this is NULL, nothing is written
    here.
    The blue intensity in the hardware colourmap for the pixel will be written
    to the storage pointed to by  blue  .If this is NULL, nothing is written
    here.
    The routine returns TRUE if the colourcell was allocated,
    else it returns FALSE.
*/
{
    static char function_name[] = "canvas_get_colour";

    VERIFY_CANVAS (canvas);
    return ( kwin_get_colour (canvas->pixcanvas, colourname, pixel_value,
			      red, green, blue) );
}   /*  End Function canvas_get_colour  */

/*PUBLIC_FUNCTION*/
void canvas_set_dressing (canvas, va_alist)
/*  [PURPOSE] This routine will set dressing parameters for a world canvas.
    <canvas> The world canvas.
    <va_alist> The list of parameter attribute-key attribute-value pairs. This
    list must be terminated with the value  KCD_END  .
    [NOTE] The dressing code is not yet complete.
    [RETURNS] Nothing.
*/
KWorldCanvas canvas;
va_dcl
{
    va_list arg_pointer;
    flag bool;
    flag no_changes = TRUE;
    unsigned int att_key;
    char *string, *ptr;
    static char function_name[] = "canvas_set_dressing";

    va_start (arg_pointer);
    VERIFY_CANVAS (canvas);
    while ( ( att_key = va_arg (arg_pointer, unsigned int) ) != KCD_END )
    {
	switch (att_key)
	{
	  case KCD_DISPLAY:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    if (bool == canvas->display_dressing) continue;
	    no_changes = FALSE;
	    canvas->display_dressing = bool;
	    break;
	  case KCD_TOP_TICKS:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    if (bool == canvas->show_top_ticks) continue;
	    no_changes = FALSE;
	    canvas->show_top_ticks = bool;
	    break;
	  case KCD_BOTTOM_TICKS:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    if (bool == canvas->show_bottom_ticks) continue;
	    no_changes = FALSE;
	    canvas->show_bottom_ticks = bool;
	    break;
	  case KCD_LEFT_TICKS:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    if (bool == canvas->show_left_ticks) continue;
	    no_changes = FALSE;
	    canvas->show_left_ticks = bool;
	    break;
	  case KCD_RIGHT_TICKS:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    if (bool == canvas->show_right_ticks) continue;
	    no_changes = FALSE;
	    canvas->show_right_ticks = bool;
	    break;
	  case KCD_INTERNAL_TICKS:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    if (bool == canvas->internal_ticks) continue;
	    no_changes = FALSE;
	    canvas->internal_ticks = bool;
	    break;
	  case KCD_AXES_COLOUR:
	    ptr = va_arg (arg_pointer, char *);
	    if ( (ptr == NULL) && (canvas->axes_colour == NULL) ) continue;
	    if ( (ptr != NULL) && (canvas->axes_colour != NULL) &&
		(strcmp (ptr, canvas->axes_colour) == 0) ) continue;
	    if ( ( string = st_dup (ptr) ) == NULL )
	    {
		m_abort (function_name, "axes colour string");
	    }
	    if (canvas->axes_colour != NULL) m_free (canvas->axes_colour);
	    no_changes = FALSE;
	    canvas->axes_colour = string;
	    break;
	  case KCD_AXES_FONT:
	    ptr = va_arg (arg_pointer, char *);
	    if ( (ptr ==NULL) && (canvas->axes_fontname == NULL) ) continue;
	    if ( (ptr != NULL) && (canvas->axes_fontname != NULL) &&
		(strcmp (ptr, canvas->axes_fontname) == 0) ) continue;
	    if ( ( string = st_dup (ptr) ) == NULL )
	    {
		m_abort (function_name, "axes fontname string");
	    }
	    if (canvas->axes_fontname
		!= NULL ) m_free (canvas->axes_fontname);
	    no_changes = FALSE;
	    canvas->axes_fontname = string;
	    break;
	  case KCD_SCALE_COLOUR:
	    ptr = va_arg (arg_pointer, char *);
	    if ( (ptr == NULL) && (canvas->scale_colour == NULL) ) continue;
	    if ( (ptr != NULL) && (canvas->scale_colour != NULL) &&
		(strcmp (ptr, canvas->scale_colour) == 0) ) continue;
	    if ( ( string = st_dup (ptr) ) == NULL )
	    {
		m_abort (function_name, "scale colour string");
	    }
	    if (canvas->scale_colour != NULL) m_free(canvas->scale_colour);
	    no_changes = FALSE;
	    canvas->scale_colour = string;
	    break;
	  case KCD_SCALE_FONT:
	    ptr = va_arg (arg_pointer, char *);
	    if ( (ptr ==NULL) && (canvas->scale_fontname == NULL) ) continue;
	    if ( (ptr != NULL) && (canvas->scale_fontname != NULL) &&
		(strcmp (ptr, canvas->scale_fontname) == 0) ) continue;
	    if ( ( string = st_dup (ptr) ) == NULL )
	    {
		m_abort (function_name, "scale fontname string");
	    }
	    if (canvas->scale_fontname!= NULL) m_free (canvas->scale_fontname);
	    no_changes = FALSE;
	    canvas->scale_fontname = string;
	    break;
	  case KCD_HLABEL:
	    ptr = va_arg (arg_pointer, char *);
	    if ( (ptr ==NULL) && (canvas->dressing_hlabel == NULL) ) continue;
	    if ( (ptr != NULL) && (canvas->dressing_hlabel != NULL) &&
		(strcmp (ptr, canvas->dressing_hlabel) == 0) ) continue;
	    if ( ( string = st_dup (ptr) ) == NULL )
	    {
		m_abort (function_name, "horizontal label string");
	    }
	    if (canvas->dressing_hlabel
		!= NULL) m_free (canvas->dressing_hlabel);
	    no_changes = FALSE;
	    canvas->dressing_hlabel = string;
	    break;
	  case KCD_VLABEL:
	    ptr = va_arg (arg_pointer, char *);
	    if ( (ptr ==NULL) && (canvas->dressing_vlabel == NULL) ) continue;
	    if ( (ptr != NULL) && (canvas->dressing_vlabel != NULL) &&
		(strcmp (ptr, canvas->dressing_vlabel) == 0) ) continue;
	    if ( ( string = st_dup (ptr) ) == NULL )
	    {
		m_abort (function_name, "vertical label string");
	    }
	    if (canvas->dressing_vlabel
		!= NULL) m_free (canvas->dressing_vlabel);
	    no_changes = FALSE;
	    canvas->dressing_vlabel = string;
	    break;
	  case KCD_LABEL_COLOUR:
	    ptr = va_arg (arg_pointer, char *);
	    if ( (ptr == NULL) && (canvas->label_colour == NULL) ) continue;
	    if ( (ptr != NULL) && (canvas->label_colour != NULL) &&
		(strcmp (ptr, canvas->label_colour) == 0) ) continue;
	    if ( ( string = st_dup (ptr) ) == NULL )
	    {
		m_abort (function_name, "label colour string");
	    }
	    if (canvas->label_colour
		!= NULL) m_free (canvas->label_colour);
	    no_changes = FALSE;
	    canvas->label_colour = string;
	    break;
	  case KCD_LABEL_FONT:
	    ptr = va_arg (arg_pointer, char *);
	    if ( (ptr ==NULL) && (canvas->label_fontname == NULL) ) continue;
	    if ( (ptr != NULL) && (canvas->label_fontname != NULL) &&
		(strcmp (ptr, canvas->label_fontname) == 0) ) continue;
	    if ( ( string = st_dup (ptr) ) == NULL )
	    {
		m_abort (function_name, "label fontname string");
	    }
	    if (canvas->label_fontname !=NULL) m_free (canvas->label_fontname);
	    no_changes = FALSE;
	    canvas->label_fontname = string;
	    break;
	  case KCD_TITLE:
	    ptr = va_arg (arg_pointer, char *);
	    if ( (ptr == NULL) && (canvas->title == NULL) ) continue;
	    if ( (ptr != NULL) && (canvas->title != NULL) &&
		(strcmp (ptr, canvas->title) == 0) ) continue;
	    if ( ( string = st_dup (ptr) ) == NULL )
	    {
		m_abort (function_name, "title string");
	    }
	    if (canvas->title != NULL) m_free (canvas->title);
	    no_changes = FALSE;
	    canvas->title = string;
	    break;
	  case KCD_TITLE_COLOUR:
	    ptr = va_arg (arg_pointer, char *);
	    if ( (ptr == NULL) && (canvas->title_colour == NULL) ) continue;
	    if ( (ptr != NULL) && (canvas->title_colour != NULL) &&
		(strcmp (ptr, canvas->title_colour) == 0) ) continue;
	    if ( ( string = st_dup (ptr) ) == NULL )
	    {
		m_abort (function_name, "title colour string");
	    }
	    if (canvas->title_colour != NULL) m_free (canvas->title_colour);
	    no_changes = FALSE;
	    canvas->title_colour = string;
	    break;
	  case KCD_TITLE_FONT:
	    ptr = va_arg (arg_pointer, char *);
	    if ( (ptr ==NULL) && (canvas->title_fontname == NULL) ) continue;
	    if ( (ptr != NULL) && (canvas->title_fontname != NULL) &&
		(strcmp (ptr, canvas->title_fontname) == 0) ) continue;
	    if ( ( string = st_dup (ptr) ) == NULL )
	    {
		m_abort (function_name, "title fontname string");
	    }
	    if (canvas->title_fontname !=NULL) m_free (canvas->title_fontname);
	    no_changes = FALSE;
	    canvas->title_fontname = string;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (arg_pointer);
    if (no_changes) return;
    /*  Refresh canvas  */
    (void) canvas_resize (canvas, (struct win_scale_type *) NULL, TRUE);
}   /*  End Function canvas_set_dressing  */

/*PUBLIC_FUNCTION*/
void canvas_sequence_dressing_refresh (KWorldCanvas canvas)
/*  This routine will sequence the dressing (axes, etc.) refresh function for a
    world canvas. If this routine is not called, the dressing will be drawn
    after *all* registered refresh functions have been called (see
    canvas_register_refresh_func  ). If this routine is called, all preceeding
    refresh functions (registered with  canvas_register_refresh_func  ) will be
    called on refresh prior to the dressing being drawn and all refresh
    functions registered after this routine is called will be called *after*
    the dressing is drawn.
    The world canvas must be given by  canvas  .
    The routine returns nothing.
*/
{
    static char function_name[] = "canvas_sequence_dressing_refresh";

    VERIFY_CANVAS (canvas);
    canvas_register_refresh_func (canvas, dressing_refresh_func, (void *)NULL);
}   /*  End Function canvas_sequence_dressing_refresh  */

/*PUBLIC_FUNCTION*/
void canvas_draw_dressing (KWorldCanvas canvas)
/*  This routine will draw the dressing (axes, etc.) for a world canvas.
    NOTE: this routine is called automatically each time the canvas is
    refreshed.
    The world canvas must be given by  canvas  .
    The routine returns nothing.
*/
{
    unsigned long pixel_value;
    int x[5], y[5];
    char *colourname;
    char *def_colourname = DEFAULT_DRESSING_COLOUR;
    static char function_name[] = "canvas_draw_dressing";

    VERIFY_CANVAS (canvas);
    canvas->dressing_drawn = TRUE;
    if (!canvas->display_dressing) return;
    /*  Draw axes  */
    if ( (colourname = canvas->axes_colour) == NULL )
    {
	colourname = def_colourname;
    }
    if ( !kwin_get_colour (canvas->pixcanvas, colourname, &pixel_value,
			   (unsigned short *) NULL, (unsigned short *) NULL,
			   (unsigned short *) NULL) )
    {
	(void) fprintf (stderr,
			"Could not allocate colour: \"%s\" for dressing\n",
			colourname);
	return;
    }
    x[0] = canvas->win_scale.x_offset - 1;
    y[0] = canvas->win_scale.y_offset - 1;
    x[1] = canvas->win_scale.x_offset + canvas->win_scale.x_pixels;
    y[1] = y[0];
    x[2] = x[1];
    y[2] = canvas->win_scale.y_offset + canvas->win_scale.y_pixels;
    x[3] = x[0];
    y[3] = y[2];
    x[4] = x[0];
    y[4] = y[0];
    kwin_draw_lines (canvas->pixcanvas, x, y, 5, pixel_value);
}   /*  End Function canvas_draw_dressing  */

/*PUBLIC_FUNCTION*/
Kcolourmap canvas_get_cmap (KWorldCanvas canvas)
/*  This routine will get the Kcolourmap object associated with a world canvas.
    The canvas must be given by  canvas  .
    The routine returns the Kcolourmap object. This may be NULL (indicating
    there is no colourmap associated with the canvas).
*/
{
    static char function_name[] = "canvas_get_cmap";

    VERIFY_CANVAS (canvas);
    return (canvas->cmap);
}   /*  End Function canvas_get_cmap  */


/*  Drawing routines follow  */

/*PUBLIC_FUNCTION*/
flag canvas_draw_image (KWorldCanvas canvas, array_desc *arr_desc, char *slice,
			unsigned int hdim, unsigned int vdim,
			unsigned int elem_index,
			KPixCanvasImageCache *cache_ptr)
/*  This routine will draw a 2-dimensional slice of a Karma array onto a world
    canvas.
    This slice may be tiled.
    The canvas must be given by  canvas  .
    The array descriptor must be pointed to by  arr_desc  .
    The start of the slice data must be pointed to by  slice  .
    The dimension index of the horizontal dimension must be given by  hdim  .
    The dimension index of the vertical dimension must be given by  vdim  .
    The element index of the data packets must be given by  elem_index  .
    The routine may produce cache data which will vastly increase the speed of
    subsequent operations on this data. The routine will write a pointer to
    this data to the storage pointed to by  cache_data  .Prior to process
    exit, a call MUST be made to  kwin_free_cache_data  ,otherwise shared
    memory segments could remain after the process exits.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int num_pixels;
    unsigned long *pixel_values;
    static char function_name[] = "canvas_draw_image";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	(void) fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    num_pixels = kcmap_get_pixels (canvas->cmap, &pixel_values);
    return ( kwin_draw_image (canvas->pixcanvas, arr_desc, slice,
			      hdim, vdim, elem_index,
			      num_pixels, pixel_values,
			      &canvas->win_scale, cache_ptr) );
}   /*  End Function canvas_draw_image  */

/*PUBLIC_FUNCTION*/
flag canvas_draw_rgb_image (KWorldCanvas canvas, array_desc *arr_desc,
			    CONST char *slice,
			    unsigned int hdim, unsigned int vdim,
			    unsigned int red_index, unsigned int green_index,
			    unsigned int blue_index,
			    KPixCanvasImageCache *cache_ptr)
/*  This routine will draw a 2-dimensional slice of a Karma array onto a world
    canvas.
    This slice may be tiled.
    The canvas must be given by  canvas  .
    The array descriptor must be pointed to by  arr_desc  .
    The start of the slice data must be pointed to by  slice  .
    The dimension index of the horizontal dimension must be given by  hdim  .
    The dimension index of the vertical dimension must be given by  vdim  .
    The element index of the red components must be given by  red_index  .
    The element index of the green components must be given by  green_index  .
    The element index of the blue components must be given by  blue_index  .
    The routine may produce cache data which will vastly increase the speed of
    subsequent operations on this data. The routine will write a pointer to
    this data to the storage pointed to by  cache_data  .Prior to process
    exit, a call MUST be made to  kwin_free_cache_data  ,otherwise shared
    memory segments could remain after the process exits.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int abs_start_coord;
    unsigned int abs_end_coord;
    unsigned int ord_start_coord;
    unsigned int ord_end_coord;
    unsigned int num_abs_coords;
    unsigned int num_ord_coords;
    uaddr *hoffsets, *voffsets;
    CONST unsigned char *red_slice, *green_slice, *blue_slice;
    packet_desc *pack_desc;
    dim_desc *hdim_desc, *vdim_desc;
    static char function_name[] = "canvas_draw_rgb_image";

    VERIFY_CANVAS (canvas);
    pack_desc = arr_desc->packet;
    if (red_index >= pack_desc->num_elements)
    {
	(void) fprintf (stderr,
			"red_index: %u greater than number of elements: %u\n",
			red_index, pack_desc->num_elements);
	a_prog_bug (function_name);
    }
    if (pack_desc->element_types[red_index] != K_UBYTE)
    {
	(void) fprintf (stderr, "Red component type: %u is not K_UBYTE\n",
			pack_desc->element_types[red_index]);
	return (FALSE);
    }
    if (green_index >= pack_desc->num_elements)
    {
	(void) fprintf(stderr,
		       "green_index: %u greater than number of elements: %u\n",
		       green_index, pack_desc->num_elements);
	a_prog_bug (function_name);
    }
    if (pack_desc->element_types[green_index] != K_UBYTE)
    {
	(void) fprintf (stderr, "Green component type: %u is not K_UBYTE\n",
			pack_desc->element_types[green_index]);
	return (FALSE);
    }
    if (blue_index >= pack_desc->num_elements)
    {
	(void) fprintf (stderr,
			"blue_index: %u greater than number of elements: %u\n",
			blue_index, pack_desc->num_elements);
	a_prog_bug (function_name);
    }
    if (pack_desc->element_types[blue_index] != K_UBYTE)
    {
	(void) fprintf (stderr, "Blue component type: %u is not K_UBYTE\n",
			pack_desc->element_types[blue_index]);
	return (FALSE);
    }
    if (arr_desc->offsets == NULL)
    {
	if ( !ds_compute_array_offsets (arr_desc) )
	{
	    a_func_abort (function_name, "error computing array offsets");
	    return (FALSE);
	}
    }
    hoffsets = arr_desc->offsets[hdim];
    voffsets = arr_desc->offsets[vdim];
    hdim_desc = arr_desc->dimensions[hdim];
    vdim_desc = arr_desc->dimensions[vdim];
    /*  Determine start and stop co-ordinates along each dimension  */
    abs_start_coord = ds_get_coord_num (hdim_desc, canvas->win_scale.x_min,
					SEARCH_BIAS_CLOSEST);
    abs_end_coord = ds_get_coord_num (hdim_desc, canvas->win_scale.x_max,
				      SEARCH_BIAS_CLOSEST);
    num_abs_coords = abs_end_coord - abs_start_coord + 1;
    ord_start_coord = ds_get_coord_num (vdim_desc, canvas->win_scale.y_min,
					SEARCH_BIAS_CLOSEST);
    ord_end_coord = ds_get_coord_num (vdim_desc, canvas->win_scale.y_max,
				      SEARCH_BIAS_CLOSEST);
    num_ord_coords = ord_end_coord - ord_start_coord + 1;
    slice += hoffsets[abs_start_coord] + voffsets[ord_start_coord];
    red_slice = (CONST unsigned char *) slice;
    red_slice += ds_get_element_offset (pack_desc, red_index);
    green_slice = (CONST unsigned char *) slice;
    green_slice += ds_get_element_offset (pack_desc, green_index);
    blue_slice = (CONST unsigned char *) slice;
    blue_slice += ds_get_element_offset (pack_desc, blue_index);
    return ( kwin_draw_rgb_image (canvas->pixcanvas,
				  canvas->win_scale.x_offset,
				  canvas->win_scale.y_offset,
				  canvas->win_scale.x_pixels,
				  canvas->win_scale.y_pixels,
				  red_slice, green_slice, blue_slice,
				  hoffsets, voffsets,
				  num_abs_coords, num_ord_coords, cache_ptr) );
}   /*  End Function canvas_draw_rgb_image  */

/*PUBLIC_FUNCTION*/
void canvas_draw_point (KWorldCanvas canvas, double x, double y,
			double value[2])
/*  This routine will draw a single point onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal world co-ordinate of the point must be given by  x  .
    The vertical world co-ordinate of the point must be given by  y  .
    The data value to use must be given by  value  .This must be of type
    K_DCOMPLEX.
    The routine returns nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_point";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	(void) fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (value, &canvas->win_scale,
					canvas->cmap);
    canvas_draw_point_p (canvas, x, y, pixel_value);
}   /*  End Function canvas_draw_point  */

/*PUBLIC_FUNCTION*/
void canvas_draw_point_p (KWorldCanvas canvas, double x, double y,
			  unsigned long pixel_value)
/*  This routine will draw a single point onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal world co-ordinate of the point must be given by  x  .
    The vertical world co-ordinate of the point must be given by  y  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
{
    int px, py;
    static char function_name[] = "canvas_draw_point_p";

    VERIFY_CANVAS (canvas);
    if ( (x < canvas->win_scale.x_min) || (x > canvas->win_scale.x_max) ||
	(y < canvas->win_scale.y_min) || (y > canvas->win_scale.y_max) )
    {
	return;
    }
    (void) canvas_convert_from_canvas_coord (canvas, x, y, &px, &py);
    kwin_draw_point (canvas->pixcanvas, px, py, pixel_value);
}   /*  End Function canvas_draw_point_p  */

/*PUBLIC_FUNCTION*/
void canvas_draw_line (KWorldCanvas canvas,
		       double x0, double y0, double x1, double y1,
		       double value[2])
/*  This routine will draw a single line onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal world co-ordinate of the first point must be given by  x0  .
    The vertical world co-ordinate of the first point must be given by  y0  .
    The horizontal world co-ordinate of the second point must be given by  x0
    The vertical world co-ordinate of the second point must be given by  y0  .
    The data value to use must be given by  value  .This must be of type
    K_DCOMPLEX.
    The routine returns nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_line";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	(void) fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (value, &canvas->win_scale,
					canvas->cmap);
    canvas_draw_line_p (canvas, x0, y0, x1, y1, pixel_value);
}   /*  End Function canvas_draw_line  */

/*PUBLIC_FUNCTION*/
void canvas_draw_line_p (KWorldCanvas canvas,
			 double x0, double y0, double x1, double y1,
			 unsigned long pixel_value)
/*  This routine will draw a single line onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal world co-ordinate of the first point must be given by  x0  .
    The vertical world co-ordinate of the first point must be given by  y0  .
    The horizontal world co-ordinate of the second point must be given by  x0
    The vertical world co-ordinate of the second point must be given by  y0  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
{
    int px0, py0;
    int px1, py1;
    static char function_name[] = "canvas_draw_line_p";

    VERIFY_CANVAS (canvas);
    (void) canvas_convert_from_canvas_coord (canvas, x0, y0, &px0, &py0);
    (void) canvas_convert_from_canvas_coord (canvas, x1, y1, &px1, &py1);
    kwin_draw_line (canvas->pixcanvas,
		    px0, py0, px1, py1, pixel_value);
}   /*  End Function canvas_draw_line_p  */

/*PUBLIC_FUNCTION*/
void canvas_fill_ellipse (KWorldCanvas canvas,
			  double centre_x, double centre_y,
			  double radius_x, double radius_y, double value[2])
/*  This routine will draw a filled ellipse onto a world canvas.
    The canvas must be given by  canvas  .
    The world co-ordinates of the centre of the ellipse must be given by
    centre_x  and centre_y  .
    The radii must be given by  radius_x  and  radius_y  .
    The complex value to fill the ellipse with must be pointed to be  value  .
    This must be of type K_DCOMPLEX.
    The routine returns nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_fill_ellipse";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	(void) fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (value, &canvas->win_scale,
					canvas->cmap);
    canvas_fill_ellipse_p (canvas,
			   centre_x, centre_y, radius_x, radius_y,
			   pixel_value);
}   /*  End Function canvas_fill_ellipse  */

/*PUBLIC_FUNCTION*/
void canvas_fill_ellipse_p (KWorldCanvas canvas,
			    double centre_x, double centre_y,
			    double radius_x, double radius_y,
			    unsigned long pixel_value)
/*  This routine will draw a filled ellipse onto a world canvas.
    The canvas must be given by  canvas  .
    The world co-ordinates of the centre of the ellipse must be given by
    centre_x  and centre_y  .
    The radii must be given by  radius_x  and  radius_y  .
    The pixel value to fill the ellipse with must be given by  pixel_value  .
    The routine returns nothing.
*/
{
    int cx, cy, rx, ry;
    static char function_name[] = "canvas_fill_ellipse_p";

    VERIFY_CANVAS (canvas);
    (void) canvas_convert_from_canvas_coord (canvas, centre_x, centre_y,
					     &cx, &cy);
    /*  Offset radius value  */
    radius_x += centre_x;
    radius_y += centre_y;
    (void) canvas_convert_from_canvas_coord (canvas, radius_x, radius_y,
					     &rx, &ry);
    /*  Undo radius offset  */
    rx -= cx;
    ry = cy - ry;
    kwin_fill_ellipse (canvas->pixcanvas, cx, cy, rx, ry, pixel_value);
}   /*  End Function canvas_fill_ellipse_p  */

/*PUBLIC_FUNCTION*/
flag canvas_fill_polygon (KWorldCanvas canvas, edit_coord *coords,
			  unsigned int num_vertices, double value[2],
			  flag convex)
/*  This routine will draw a filled polygon onto a world canvas.
    The canvas must be given by  canvas  .
    The array of world co-ordinates of vertices of the polygon must be pointed
    to by  coords  .
    The number of vertices in the polygon must be given by  num_vertices  .
    The complex value to fill the polygon with must be pointed to be  value  .
    This must be of type K_DCOMPLEX.
    If the value of  convex  is TRUE, then the points must form a convex
    polygon  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int coord_count;
    unsigned long pixel_value;
    static unsigned int num_points_allocated = 0;
    static int *point_x = NULL;
    static int *point_y = NULL;
    static char function_name[] = "canvas_fill_polygon";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	(void) fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    if (num_vertices > num_points_allocated)
    {
	if (point_x != NULL)
	{
	    m_free ( (char *) point_x );
	    point_x = NULL;
	}
	if (point_y != NULL)
	{
	    m_free ( (char *) point_y );
	    point_y = NULL;
	}
	num_points_allocated = 0;
	if ( ( point_x = (int *) m_alloc (sizeof *point_x * num_vertices) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of x co-ordinates");
	    return (FALSE);
	}
	if ( ( point_y = (int *) m_alloc (sizeof *point_y * num_vertices) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of y co-ordinates");
	    m_free ( (char *) point_x );
	    point_x = NULL;
	    return (FALSE);
	}
	num_points_allocated = num_vertices;
    }
    /*  Convert world co-ordinates to pixel co-ordinates  */
    for (coord_count = 0; coord_count < num_vertices; ++coord_count)
    {
	(void) canvas_convert_from_canvas_coord (canvas,
						 coords[coord_count].abscissa,
						 coords[coord_count].ordinate,
						 point_x + coord_count,
						 point_y + coord_count);
    }
    pixel_value = get_pixel_from_value (value, &canvas->win_scale,
					canvas->cmap);
    return ( kwin_fill_polygon (canvas->pixcanvas,
				point_x, point_y, num_vertices,
				pixel_value, convex) );
}   /*  End Function canvas_fill_polygon  */

/*PUBLIC_FUNCTION*/
void canvas_draw_rectangle (KWorldCanvas canvas, double x, double y,
			    double width, double height, double value[2])
/*  This routine will draw a single rectangle onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the rectangle must be given by  x  .
    The vertical offset of the rectangle must be given by  y  .
    The width of the rectangle must be given by  width  .
    The height of the rectangle must be given by  height  .
    The data value to use must be given by  value  .This must be of type
    K_DCOMPLEX.
    The routine returns nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_rectangle";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	(void) fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (value, &canvas->win_scale,
					canvas->cmap);
    canvas_draw_rectangle_p (canvas, x, y, width, height, pixel_value);
}   /*  End Function canvas_draw_rectangle  */

/*PUBLIC_FUNCTION*/
void canvas_draw_rectangle_p (KWorldCanvas canvas, double x, double y,
			      double width, double height,
			      unsigned long pixel_value)
/*  This routine will draw a single rectangle onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the rectangle must be given by  x  .
    The vertical offset of the rectangle must be given by  y  .
    The width of the rectangle must be given by  width  .
    The height of the rectangle must be given by  height  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
{
    int px, py;
    int pwidth, pheight, py1;
    static char function_name[] = "canvas_draw_rectangle_p";

    VERIFY_CANVAS (canvas);
    (void) canvas_convert_from_canvas_coord (canvas, x, y, &px, &py);
    /*  Offset width and height  */
    x += width;
    y += height;
    (void) canvas_convert_from_canvas_coord (canvas, x, y,
					     &pwidth, &py1);
    /*  Undo radius offset  */
    pwidth -= px;
    pheight = py - py1;
    kwin_draw_rectangle (canvas->pixcanvas,
			 px, py1, pwidth, pheight, pixel_value);
}   /*  End Function canvas_draw_rectangle_p  */

/*PUBLIC_FUNCTION*/
void canvas_fill_rectangle (KWorldCanvas canvas, double x, double y,
			    double width, double height, double value[2])
/*  This routine will fill a single rectangle onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the rectangle must be given by  x  .
    The vertical offset of the rectangle must be given by  y  .
    The width of the rectangle must be given by  width  .
    The height of the rectangle must be given by  height  .
    The data value to use must be given by  value  .This must be of type
    K_DCOMPLEX.
    The routine returns nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_fill_rectangle";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	(void) fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (value, &canvas->win_scale,
					canvas->cmap);
    canvas_fill_rectangle_p (canvas, x, y, width, height, pixel_value);
}   /*  End Function canvas_fill_rectangle  */

/*PUBLIC_FUNCTION*/
void canvas_fill_rectangle_p (KWorldCanvas canvas, double x, double y,
			      double width, double height,
			      unsigned long pixel_value)
/*  This routine will fill a single rectangle onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the rectangle must be given by  x  .
    The vertical offset of the rectangle must be given by  y  .
    The width of the rectangle must be given by  width  .
    The height of the rectangle must be given by  height  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
{
    int px, py;
    int pwidth, pheight, py1;
    static char function_name[] = "canvas_fill_rectangle_p";

    VERIFY_CANVAS (canvas);
    (void) canvas_convert_from_canvas_coord (canvas, x, y, &px, &py);
    /*  Offset width and height  */
    x += width;
    y += height;
    (void) canvas_convert_from_canvas_coord (canvas, x, y,
					     &pwidth, &py1);
    /*  Undo radius offset  */
    pwidth -= px;
    pheight = py - py1;
    kwin_fill_rectangle (canvas->pixcanvas,
			 px, py1, pwidth, pheight, pixel_value);
}   /*  End Function canvas_fill_rectangle_p  */

/*PUBLIC_FUNCTION*/
void canvas_draw_lines (KWorldCanvas canvas, double *x_array, double *y_array,
			int num_points, double value[2])
/*  This routine will draw multiple connected lines onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal world co-ordinates of the points must be pointed to by
    x_array  .If this is NULL, the routine assumes horizontal co-ordinates
    equally spaced across the world canvas.
    The vertical world co-ordinates of the points must be pointed to by
    y_array  .If this is NULL, the routine assumes vertical co-ordinates
    equally spaced across the world canvas.
    The number of points must be given by  num_points  .The number of lines
    drawn is 1 less than this value.
    The data value to use must be given by  value  .This must be of type
    K_DCOMPLEX.
    The routine returns nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_lines";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	(void) fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (value, &canvas->win_scale,
					canvas->cmap);
    canvas_draw_lines_p (canvas, x_array, y_array, num_points, pixel_value);
}   /*  End Function canvas_draw_lines  */

/*PUBLIC_FUNCTION*/
void canvas_draw_lines_p (KWorldCanvas canvas,
			  double *x_array, double *y_array, int num_points,
			  unsigned long pixel_value)
/*  This routine will draw multiple connected lines onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal world co-ordinates of the points must be pointed to by
    x_array  .If this is NULL, the routine assumes horizontal co-ordinates
    equally spaced across the world canvas.
    The vertical world co-ordinates of the points must be pointed to by
    y_array  .If this is NULL, the routine assumes vertical co-ordinates
    equally spaced across the world canvas.
    The number of points must be given by  num_points  .The number of lines
    drawn is 1 less than this value.
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
{
    int count;
    double xscale, yscale;
    double wx, wy;
    static int num_allocated;
    static int *px = NULL;
    static int *py = NULL;
    static char function_name[] = "canvas_draw_lines_p";

    VERIFY_CANVAS (canvas);
    if (num_points > num_allocated)
    {
	if (px != NULL) m_free ( (char *) px );
	if (py != NULL) m_free ( (char *) py );
	if ( ( px = (int *) m_alloc (sizeof *px * num_points) ) == NULL )
	{
	    m_abort (function_name, "x pixel co-ordinate array");
	}
	if ( ( py = (int *) m_alloc (sizeof *py * num_points) ) == NULL )
	{
	    m_abort (function_name, "y pixel co-ordinate array");
	}
	num_allocated = num_points;
    }
    xscale = canvas->win_scale.x_max - canvas->win_scale.x_min;
    xscale /= (double) (num_points - 1);
    yscale = canvas->win_scale.y_max - canvas->win_scale.y_min;
    yscale /= (double) (num_points - 1);
    for (count = 0; count < num_points; ++count)
    {
	if (x_array == NULL)
	{
	    wx = canvas->win_scale.x_min + xscale * (count);
	}
	else
	{
	    wx = x_array[count];
	}
	if (y_array == NULL)
	{
	    wy = canvas->win_scale.y_min + yscale * (count);
	}
	else
	{
	    wy = y_array[count];
	}
	(void) canvas_convert_from_canvas_coord (canvas, wx, wy,
						 px + count, py + count);
    }
    kwin_draw_lines (canvas->pixcanvas, px, py, num_points, pixel_value);
}   /*  End Function canvas_draw_lines_p  */


/*  Private functions follow  */
static void pixcanvas_refresh_func (KPixCanvas pixcanvas,
				    int width, int height, void **info)
/*  This routine is a refresh event consumer for a pixel canvas.
    The canvas is given by  pixcanvas  .
    The width of the canvas is given by  width  .
    The height of the canvas is given by  height  .
    The arbitrary canvas information pointer is pointed to by  info  .
    The routine returns nothing.
*/
{
    KWorldCanvas canvas;
    static char function_name[] = "pixcanvas_refresh_func";

    canvas = (KWorldCanvas) *info;
    VERIFY_CANVAS (canvas);
    if (pixcanvas != canvas->pixcanvas)
    {
	(void) fprintf (stderr, "Pixel canvases do not match\n");
	a_prog_bug (function_name);
    }
    refresh_canvas (canvas, width, height, FALSE);
}   /*  End Function pixcanvas_refresh_func  */

static void cmap_resize_func (Kcolourmap cmap, void **info)
/*  This routine registers a change in the size of a colourmap.
    The colourmap must be given by  cmap  .
    The arbitrary colourmap information pointer is pointed to by  info  .
    The routine returns nothing.
*/
{
    KWorldCanvas canvas;
    int width, height;
    struct win_scale_type win_scale;
    struct refresh_func *curr_func;
    static char function_name[] = "cmap_resize_func";

    canvas = (KWorldCanvas) *info;
    VERIFY_CANVAS (canvas);
    if (cmap != canvas->cmap)
    {
	(void) fprintf (stderr, "Colourmaps do not match\n");
	a_prog_bug (function_name);
    }
    if (canvas->in_size_control_func) return;
    /*  Get pixel canvas size  */
    kwin_get_size (canvas->pixcanvas, &width, &height);
    refresh_canvas (canvas, width, height, TRUE);
}   /*  End Function cmap_resize_func  */

static void refresh_canvas (KWorldCanvas canvas, int width, int height,
			    flag cmap_resize)
/*  This routine will refresh a world canvas (by calling all registered
    refresh functions).
    The world canvas must be given by  canvas  .
    The width of the canvas is given by  width  .
    The height of the canvas is given by  height  .
    If the refresh function was called as a result of a colourmap resize the
    value of  cmap_resize  will be TRUE.
    The routine returns nothing.
*/
{
    flag dummy = FALSE;
    struct refresh_struct data;
    static char function_name[] = "__canvas_refresh_canvas";

    VERIFY_CANVAS (canvas);
    canvas->dressing_drawn = FALSE;
    if (canvas->quash_negotiate)
    {
	canvas->quash_negotiate = FALSE;
    }
    else
    {
	if ( !negotiate_scale_change (canvas, width, height, &dummy) )
	{
	    (void) fprintf (stderr, "Pixel canvas not big enough\n");
	    return;
	}
    }
    /*  Call refresh functions  */
    data.width = width;
    data.height = height;
    data.cmap_resize = cmap_resize;
    (void) c_call_callbacks (canvas->refresh_list, &data);
    if (canvas->dressing_drawn) return;
    if (canvas->display_dressing) canvas_draw_dressing (canvas);
}   /*  End Function refresh_canvas  */

static unsigned long get_pixel_from_value (double value[2],
					   struct win_scale_type *win_scale,
					   Kcolourmap cmap)
/*  This routine will get a pixel value from a data value.
    The data value must be given by  value  .
    The window scaling information must be pointed to by  win_scale  .
    The colourmap must be given by  cmap  .
    The routine returns the pixel value.
*/
{
    unsigned int num_pixels;
    unsigned long pixel_value;
    unsigned long *pixel_values;
    static char function_name[] = "get_pixel_from_value";

    if (cmap == NULL)
    {
	(void) fprintf (stderr, "NULL colourmap passed\n");
	a_prog_bug (function_name);
    }
    num_pixels = kcmap_get_pixels (cmap, &pixel_values);
    switch (win_scale->conv_type)
    {
      case KIMAGE_COMPLEX_CONV_REAL:
	break;
      case KIMAGE_COMPLEX_CONV_IMAG:
	value[0] = value[1];
	break;
      case KIMAGE_COMPLEX_CONV_ABS:
	value[0] = sqrt (value[0] * value[0] + value[1] * value[1]);
	break;
      case KIMAGE_COMPLEX_CONV_SQUARE_ABS:
	value[0] = value[0] * value[0] + value[1] * value[1];
	break;
      case KIMAGE_COMPLEX_CONV_PHASE:
	if ( (value[0] == 0.0) && (value[1] == 0.0) )
	{
	    value[0] = 0.0;
	}
	else
	{
	    value[0] = atan2 (value[1], value[0]);
	}
	break;
      case KIMAGE_COMPLEX_CONV_CONT_PHASE:
	(void) fprintf (stderr, "Not finished continuous phase\n");
	return (FALSE);
/*
	break;
*/
      default:
	(void) fprintf (stderr, "Illegal value of conversion: %u\n",
			win_scale->conv_type);
	a_prog_bug (function_name);
	break;
    }
    /*  Take real value and convert to pixel value  */
    switch (win_scale->z_scale)
    {
      case K_INTENSITY_SCALE_LINEAR:
	if (value[0] < win_scale->z_min)
	{
	    pixel_value = win_scale->min_sat_pixel;
	}
	else if (value[0] > win_scale->z_max)
	{
	    pixel_value = win_scale->max_sat_pixel;
	}
	else
	{
	    pixel_value = pixel_values[(unsigned int)
				       ( (value[0] - win_scale->z_min)
					* (num_pixels - 1) /
					(win_scale->z_max -
					 win_scale->z_min) + 0.5 )];
	}
	break;
      case K_INTENSITY_SCALE_LOGARITHMIC:
	if (value[0] < win_scale->z_min)
	{
	    pixel_value = win_scale->min_sat_pixel;
	}
	else if (value[0] > win_scale->z_max)
	{
	    pixel_value = win_scale->max_sat_pixel;
	}
	else
	{
	    pixel_value = pixel_values[(unsigned int)
				       ( log10 (value[0] / win_scale->z_min)
					* (num_pixels - 1) /
					log10 (win_scale->z_max /
					       win_scale->z_min) )];
	}
	break;
      default:
	(void) fprintf (stderr, "Illegal value of intensity scale: %u\n",
			win_scale->z_scale);
	a_prog_bug (function_name);
	break;
    }
    return (pixel_value);
}   /*  End Function get_pixel_from_value  */

static flag pixcanvas_position_event (KPixCanvas pixcanvas, int x, int y,
				      unsigned int event_code,
				      void *event_info, void **f_info)
/*  This routine is a position event consumer for a pixel canvas.
    The pixel canvas is given by  pixcanvas  .
    The horizontal position of the event, relative to the canvas origin,
    will be given by  x  .
    The vertical position of the event, relative to the canvas origin,
    will be given by  y  .
    The arbitrary event code is given by  event_code  .
    The arbitrary event information is pointed to by  event_info  .
    The arbitrary function information pointer is pointed to by  f_info  .
    The routine returns TRUE if the event was consumed, else it returns
    FALSE indicating that the event is still to be processed.
*/
{
    double wx, wy, lx, ly;
    KWorldCanvas canvas;
    struct position_struct data;
    static char function_name[] = "pixcanvas_position_event";

    canvas = (KWorldCanvas) *f_info;
    VERIFY_CANVAS (canvas);
    /*  Convert pixel co-ordinates to world co-ordinates  */
    (void) pixel_to_world_coord_convert (canvas, x, y, &wx, &wy, &lx, &ly);
    /*  Call event consumer functions  */
    data.x = wx;
    data.y = wy;
    data.event_code = event_code;
    data.e_info = event_info;
    data.x_lin = lx;
    data.y_lin = ly;
    return ( c_call_callbacks (canvas->position_list, &data) );
}   /*  End Function pixcanvas_position_event  */

static flag refresh_event_func (void *object, void *client1_data,
				void *call_data, void *client2_data)
/*  This routine is called when object callbacks are called.
    The object information pointer will be given by  object  .
    The first client information pointer will be given by  client1_data  .
    The call information pointer will be given by  call_data  .
    The second client information pointer will be given by  client2_data  .
    The routine returns TRUE if further callbacks should not be called.
*/
{
    KWorldCanvas canvas;
    struct win_scale_type win_scale;
    void (*func) ();
    struct refresh_struct *data;
    static char function_name[] = "refresh_event_func";

    canvas = (KWorldCanvas) object;
    VERIFY_CANVAS (canvas);
    data = (struct refresh_struct *) call_data;
    func = ( void (*) () ) client2_data;
    m_copy ( (char *) &win_scale, (char *) &canvas->win_scale,
	    sizeof win_scale );
    (*func) (canvas, data->width, data->height,
	     &win_scale, canvas->cmap, data->cmap_resize, client1_data);
    return (FALSE);
}   /*  End Function refresh_event_func  */

static flag position_event_func (void *object, void *client1_data,
				 void *call_data, void *client2_data)
/*  This routine is called when object callbacks are called.
    The object information pointer will be given by  object  .
    The first client information pointer will be given by  client1_data  .
    The call information pointer will be given by  call_data  .
    The second client information pointer will be given by  client2_data  .
    The routine returns TRUE if further callbacks should not be called.
*/
{
    KWorldCanvas canvas;
    flag (*func) ();
    struct position_struct *data;
    static char function_name[] = "position_event_func";

    canvas = (KWorldCanvas) object;
    VERIFY_CANVAS (canvas);
    data = (struct position_struct *) call_data;
    func = ( flag (*) () ) client2_data;
    return ( (*func) (canvas, data->x, data->y, data->event_code,
		      data->e_info, client1_data,
		      data->x_lin, data->y_lin) );
}   /*  End Function position_event_func  */

static void verify_win_scale (struct win_scale_type *win_scale,
			      char *function_name)
/*  This routine will verify if window scaling information is sensible.
    The window scaling information must be pointed to by  win_scale  .
    The name of the calling function must be pointed to by  function_name  .
    The routine returns nothing (the process aborts on error).
*/
{
    if (win_scale == NULL) return;
    if (K_WIN_SCALE_MAGIC_NUMBER != win_scale->magic_number)
    {
	(void) fprintf (stderr,"Bad magic number for  win_scale  structure\n");
	(void) fprintf (stderr, "This will break in Karma version 2.0\n");
	(void) fprintf (stderr,
			"Make sure a call is made to canvas_init_win_scale\n");
	(void) fprintf (stderr, "and then recompile the application\n");
/*      Coming in Version 2.0 of Karma
	a_prog_bug (function_name);
*/
    }
    if (win_scale->x_max <= win_scale->x_min)
    {
	(void) fprintf (stderr, "x_max: %e must be greater than x_min: %e\n",
			win_scale->x_max, win_scale->x_min);
	a_prog_bug (function_name);
    }
    if (win_scale->y_max <= win_scale->y_min)
    {
	(void) fprintf (stderr, "y_max: %e must be greater than y_min: %e\n",
			win_scale->y_max, win_scale->y_min);
	a_prog_bug (function_name);
    }
}   /*  End Function verify_win_scale  */

static flag negotiate_scale_change (KWorldCanvas canvas, int width, int height,
				    flag *boundary_clear)
/*  This routine will negotiate a scale change for a world canvas. This
    negotiation occurss between the size control function which may be
    registered and the dressing display control system. This negotiation
    process is, er, intricate :-)
    The world canvas must be given by  canvas  .
    The width of the pixel canvas is given by  width  .
    The height of the pixel canvas is given by  height  .
    If the value TRUE is written to the storage pointed to by
    boundary_clear  then the  canvas_resize  routine will attempt to clear
    only the boundary between the pixel canvas and the world canvas. If
    the value FALSE is written here or nothing is written here, the
    canvas_resize  routine will clear the entire pixel canvas as
    appropriate.
    The routine returns TRUE on succes, else it returns FALSE, indicating the
    scale change could not be negotiated.
*/
{
    flag dressing_toobig;
    int p_left;
    int p_right;
    int p_top;
    int p_bottom;
    int tmp;
    unsigned int count;
    struct win_scale_type *win_scale;
    static char function_name[] = "negotiate_scale_change";

    win_scale = &canvas->win_scale;
    *boundary_clear = FALSE;
    if (canvas->size_control_func == NULL)
    {
	/*  Must automatically produce some window scaling information  */
	get_dressing_size (canvas, &p_left, &p_right, &p_top, &p_bottom);
	if (p_left + p_right >= width)
	{
	    (void) fprintf (stderr,
			    "Dressing (%d pixels) too wide for pixel canvas (%d pixels)\n",
			    p_left + p_right, width);
	    return (FALSE);
	}
	if (p_top + p_bottom >= height)
	{
	    (void) fprintf (stderr,
			    "Dressing (%d pixels) too high for pixel canvas (%d pixels)\n",
			    p_top + p_bottom, height);
	    return (FALSE);
	}
	win_scale->x_offset = p_left;
	win_scale->x_pixels = width - p_left - p_right;
	win_scale->y_offset = p_top;
	win_scale->y_pixels = height - p_top - p_bottom;
	return (TRUE);
    }
    p_left = p_right = p_top = p_bottom = 0;
    for (count = 0, dressing_toobig = TRUE;
	 (count < DRESSING_ITERATE_MAX) && dressing_toobig; ++count)
    {
	/*  Call size control function first  */
	canvas->in_size_control_func = TRUE;
	(* canvas->size_control_func) (canvas,
					 width - p_left - p_right,
					 height - p_top - p_bottom,
					 win_scale,
					 &canvas->size_control_info,
					 boundary_clear);
	canvas->in_size_control_func = FALSE;
	verify_win_scale (win_scale, function_name);
	get_dressing_size (canvas, &p_left, &p_right, &p_top, &p_bottom);
	if ( (p_left + p_right + win_scale->x_pixels <= width) &&
	    (p_top + p_bottom + win_scale->y_pixels <= height) )
	{
	    /*  Looks like it will fit  */
	    dressing_toobig = FALSE;
	}
    }
    if (dressing_toobig)
    {
	if (p_left + p_right + win_scale->x_pixels > width)
	{
	    (void) fprintf (stderr,
			    "Dressing (%d pixels) too wide for pixel canvas (%d pixels)\n",
			    p_left + p_right + win_scale->x_pixels,
			    width);
	    return (FALSE);
	}
	if (p_top + p_bottom + win_scale->y_pixels > height)
	{
	    (void) fprintf (stderr,
			    "Dressing (%d pixels) too high for pixel canvas (%d pixels)\n",
			    p_top + p_bottom + win_scale->y_pixels,
			    height);
	    return (FALSE);
	}
    }
    /*  Dressing will fit!  */
    if ( (p_left > win_scale->x_offset) ||
	(p_right + win_scale->x_offset + win_scale->x_pixels > width) )
    {
	tmp = width - win_scale->x_pixels - p_left - p_right;
	win_scale->x_offset = tmp / 2 + p_left;
    }
    if ( (p_top > win_scale->y_offset) ||
	(p_bottom + win_scale->y_offset + win_scale->y_pixels > height) )
    {
	tmp = height - win_scale->y_pixels - p_top - p_bottom;
	win_scale->y_offset = tmp / 2 + p_top;
    }
    return (TRUE);
}   /*  End Function negotiate_scale_change  */

static void get_dressing_size (KWorldCanvas canvas, int *p_left, int *p_right,
			       int *p_top, int *p_bottom)
/*  This routine will determine the size of the dressing.
    The world canvas must be given by  canvas  .
    The number of horizontal pixels required to the left will be written to the
    storage pointed to by  p_left  .
    The number of horizontal pixels required to the right will be written to
    the storage pointed to by  p_right  .
    The number of vertical pixels required at the top will be written to the
    storage pointed to by  p_top  .
    The number of vertical pixels required at the bottom will be written to
    the storage pointed to by  p_bottom  .
    The routine returns nothing.
*/
{
    if (!canvas->display_dressing)
    {
	*p_left = 0;
	*p_right = 0;
	*p_top = 0;
	*p_bottom = 0;
	return;
    }
    /*  Make room for box  */
    *p_left = 1;
    *p_right = 1;
    *p_top = 1;
    *p_bottom = 1;
    if (!canvas->internal_ticks)
    {
	/*  External ticks  */
	if (canvas->show_top_ticks) *p_top += MAJOR_TICK_PIXELS;
	if (canvas->show_bottom_ticks) *p_bottom += MAJOR_TICK_PIXELS;
	if (canvas->show_left_ticks) *p_left += MAJOR_TICK_PIXELS;
	if (canvas->show_right_ticks) *p_right += MAJOR_TICK_PIXELS;
    }
    if (canvas->show_top_ticks || canvas->show_bottom_ticks)
    {
	/*  Show horizontal scale  */
	*p_bottom += 20;   /*  TEMPORARY: get font size later  */
    }
    if (canvas->show_left_ticks || canvas->show_right_ticks)
    {
	/*  Show vertical scale  */
	*p_left += 20;   /*  TEMPORARY: get font size later  */
    }
    if (canvas->dressing_hlabel != NULL)
    {
	/*  Show horizontal label  */
	*p_bottom += 20;   /*  TEMPORARY: get font size later  */
    }
    if (canvas->dressing_vlabel != NULL)
    {
	/*  Show vertical label  */
	*p_left += 20;   /*  TEMPORARY: get font size later  */
    }
    if (canvas->title != NULL)
    {
	/*  Show title  */
	*p_top += 20;   /*  TEMPORARY: get font size later  */
    }
}   /*  End Function get_dressing_size  */

static void dressing_refresh_func (KWorldCanvas canvas, int width, int height,
				   struct win_scale_type *win_scale,
				   Kcolourmap cmap, flag cmap_resize,
				   void **info)
/*  This routine is a refresh event consumer for a world canvas.
    The canvas is given by  canvas  .
    The width of the canvas in pixels is given by  width  .
    The height of the canvas in pixels is given by  height  .
    The window scaling information is pointed to by  win_scale  .
    The colourmap associated with the canvas is given by  cmap  .
    If the refresh function was called as a result of a colourmap resize
    the value of  cmap_resize  will be TRUE.
    The arbitrary canvas information pointer is pointed to by  info  .
    The routine returns nothing.
*/
{
    static char function_name[] = "__canvas_dressing_refresh_func";

    VERIFY_CANVAS (canvas);
    canvas_draw_dressing (canvas);
}   /*  End Function dressing_refresh_func  */

static flag pixel_to_world_coord_convert (KWorldCanvas canvas, int xin,int yin,
					  double *xout, double *yout,
					  double *x_lin, double *y_lin)
/*  This routine will convert co-ordinates in a pixel canvas to co-ordinates in
    a world canvas.
    The world canvas must be given by  canvas  .
    The lower level horizontal co-ordinate must be given by  xin  .
    The lower level vertical co-ordinate must be given by  yin  .
    The horizontal world co-ordinate will be written to the storage pointed to
    by  xout  .
    The vertical world co-ordinate will be written to the storage pointed to
    by  xout  .
    The linear world co-ordinates will by written to the storage pointed to by
    x_lin  and  y_lin  .
    The routine returns TRUE if the co-ordinate lies within the canvas
    boundaries, else it returns FALSE (although a conversion is still
    performed).
*/
{
    flag converted = FALSE;
    int px, py;
    struct win_scale_type win_scale;
    static char function_name[] = "pixel_to_world_coord_convert";

    VERIFY_CANVAS (canvas);
    if (canvas->coord_convert_func != NULL)
    {
	m_copy ( (char *) &win_scale, (char *) &canvas->win_scale,
		sizeof win_scale );
	px = xin;
	py = yin;
	converted = ( (*canvas->coord_convert_func)
		     (canvas, &win_scale, &px, &py, x_lin, y_lin, TRUE,
		      &canvas->coord_convert_info) );
    }
    if (!converted)
    {
	/*  Convert pixel co-ordinates to world co-ordinates  */
	/*  Convert x  */
	*x_lin = xin - canvas->win_scale.x_offset;
	*x_lin /= (double) (canvas->win_scale.x_pixels - 1);
	*x_lin *= (canvas->win_scale.x_max - canvas->win_scale.x_min);
	*x_lin += canvas->win_scale.x_min;
	/*  Flip vertical  */
	yin -= canvas->win_scale.y_offset;
	yin = canvas->win_scale.y_pixels - yin - 1;
	/*  Convert y  */
	*y_lin = yin;
	*y_lin /= (double) (canvas->win_scale.y_pixels - 1);
	*y_lin *= (canvas->win_scale.y_max - canvas->win_scale.y_min);
	*y_lin += canvas->win_scale.y_min;
    }
    *xout = *x_lin;
    *yout = *y_lin;
    if ( (xin < canvas->win_scale.x_offset) ||
	(xin >= canvas->win_scale.x_offset + canvas->win_scale.x_pixels) ||
	(yin < canvas->win_scale.y_offset) ||
	(yin >= canvas->win_scale.y_offset + canvas->win_scale.y_pixels) )
    return (FALSE);
    /*  Apply non-linear transform function  */
    return ( canvas_coord_transform (canvas, xout, yout, FALSE) );
}   /*  End Function pixel_to_world_coord_convert  */

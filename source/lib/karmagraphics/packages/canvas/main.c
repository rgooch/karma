/*LINTLIBRARY*/
/*  main.c

    This code provides KWorldCanvas objects.

    Copyright (C) 1993-1996  Richard Gooch

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

    Updated by      Richard Gooch   28-NOV-1994: Made use of  c_  package.

    Updated by      Richard Gooch   2-JAN-1995: Took account of change to
  kwin_draw_rgb_image  .

    Updated by      Richard Gooch   15-MAR-1995: Added propagation of
  PostScriptPage object to refresh callbacks.

    Updated by      Richard Gooch   30-APR-1995: Added support for intensity
  scaling function.

    Updated by      Richard Gooch   13-JUL-1995: Improved informative output in
   <negotiate_scale_change>.

    Updated by      Richard Gooch   29-AUG-1995: Created
  <canvas_get_attributes> and <canvas_set_attributes>.

    Updated by      Richard Gooch   6-SEP-1995: Made experimental use of
  <kwin_draw_pc_image>.

    Updated by      Richard Gooch   15-SEP-1995: Took account of increased
  precision of <kwin_draw_point> and <kwin_draw_line>.

    Updated by      Richard Gooch   22-SEP-1995: Fixed bug in
  <convert_from_canvas_coord>: passing wrong pointer type.

    Updated by      Richard Gooch   5-JAN-1996: Created
  <canvas_register_d_convert_func> and deprecated integer version.

    Uupdated by     Richard Gooch   19-FEB-1996: Supported list of areas to
  refresh in refresh routines.

    Updated by      Richard Gooch   14-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   19-MAY-1996: Deprecated <canvas_get_size>.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   4-JUN-1996: Switched to left-right
  bottom-top co-ordinate specification instead of min-max x and y.

    Updated by      Richard Gooch   15-JUN-1996: Created
  <canvas_register_transforms_func> and ellipse drawing routines.

    Updated by      Richard Gooch   17-JUN-1996: Created
  <canvas_convert_to_canvas_coords> routine and deprecated
  <canvas_convert_to_canvas_coord> routine.

    Updated by      Richard Gooch   19-JUL-1996: Created
  <canvas_convert_from_canvas_coords> routine and deprecated
  <canvas_convert_from_canvas_coord> routine.

    Updated by      Richard Gooch   20-JUL-1996: Created
  <canvas_register_coords_convert_func> routine and deprecated
  <canvas_register_d_convert_func> routine.

    Updated by      Richard Gooch   10-AUG-1996: Checked for TOOBIG
  co-ordinates in <canvas_draw_lines_p>.

    Updated by      Richard Gooch   16-SEP-1996: Allow NULL win_scale for
  <canvas_create>.

    Updated by      Richard Gooch   28-SEP-1996: Fixed bug in
  <canvas_set_attributes> when setting iscale_func.

    Updated by      Richard Gooch   5-NOV-1996: Fixed <canvas_draw_lines_p> to
  divide equally in non-linear world co-ordinates rather than in linear
  co-ordinates.

    Last updated by Richard Gooch   8-DEC-1996: Added support for
  "!valueR valueI" colournames in <canvas_get_colour>. Undocumented.


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <karma.h>
#define KWIN_GENERIC_ONLY
#define NEW_WIN_SCALE
#include <karma_canvas.h>
#include <karma_ds.h>
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
#define COORD_BUF_SIZE 2048

#define VERIFY_CANVAS(canvas) if (canvas == NULL) \
{fprintf (stderr, "NULL canvas passed\n"); \
 a_prog_bug (function_name); } \
if (canvas->magic_number != MAGIC_NUMBER) \
{fprintf (stderr, "Invalid canvas object\n"); \
 a_prog_bug (function_name); }


/*  Internal definition of KWorldCanvas object structure type  */
struct worldcanvas_type
{
    unsigned int magic_number;
    KPixCanvas pixcanvas;
    Kcolourmap cmap;
    unsigned int cmap_num_pixels;
    unsigned long *cmap_pixels;
    unsigned short *cmap_rgb_values;
    struct win_scale_type win_scale;
    flag auto_min_sat;
    flag auto_max_sat;
    /*  Callback information  */
    void (*size_control_func) ();
    void *size_control_info;
    flag (*coords_convert_func) (KWorldCanvas canvas, unsigned int num_coords,
				 CONST double *xin, CONST double *yin,
				 double *xout, double *yout,
				 flag to_world, void **info);
    flag (*deprecated_coord_d_convert_func) (KWorldCanvas canvas,
					     struct win_scale_type *win_scale,
					     double *x, double *y,
					     flag to_world, void **info);
    flag (*deprecated_coord_convert_func) (KWorldCanvas canvas,
					   struct win_scale_type *win_scale,
					   int *px, int *py,
					   double *wx, double *wy,
					   flag to_world, void **info);
    void *coord_convert_info;
    KCallbackList refresh_list;
    KCallbackList position_list;
    flag quash_negotiate;
    flag in_size_control_func;
    void (*coord_transforms_func) (KWorldCanvas canvas,unsigned int num_coords,
				   double *x, flag x_to_linear,
				   double *y, flag y_to_linear,
				   double left_x, double right_x,
				   double bottom_y, double top_y,
				   void **info);
    void (*deprecated_coord_transform_func) ();
    void *coord_transform_info;
    /*  Specification information  */
    char specify_xlabel[STRING_LENGTH];
    char specify_ylabel[STRING_LENGTH];
    unsigned int num_restrictions;
    unsigned int restrictions_allocated;
    char **restriction_names;  /*  Strings of length STRING_LENGTH  */
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
    PostScriptPage pspage;
    unsigned int num_areas;
    KPixCanvasRefreshArea *areas;
    flag honoured_areas;
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


/*  Private functions  */
STATIC_FUNCTION (void pixcanvas_refresh_func,
		 (KPixCanvas pixcanvas, int width, int height, void **info,
		  PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );
STATIC_FUNCTION (void cmap_resize_func, (Kcolourmap cmap, void **info) );
STATIC_FUNCTION (void refresh_canvas,
		 (KWorldCanvas canvas, int width, int height,
		  flag cmap_resize, PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );
STATIC_FUNCTION (unsigned long get_pixel_from_value,
		 (KWorldCanvas canvas, double value[2],
		  unsigned short *red, unsigned short *green,
		  unsigned short *blue) );
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
STATIC_FUNCTION (flag refresh_event_func,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );
STATIC_FUNCTION (flag position_event_func,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );
STATIC_FUNCTION (void set_sat_pixels, (KWorldCanvas canvas) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KWorldCanvas canvas_create (KPixCanvas pixcanvas, Kcolourmap cmap,
			    struct win_scale_type *win_scale)
/*  [SUMMARY] Create world canvas.
    [PURPOSE] This routine will create a world canvas, ready for drawing, from
    a KPixCanvas (pixel canvas) object. The origin of a KWorldCanvas is the
    lower-left corner.
    <pixcanvas> The pixel canvas object.
    <cmap> The colourmap. If this is NULL, the canvas will only permit drawing
    using pixel values. This precludes the use of drawing PseudoColour images
    and drawing geometric primitives using data values.
    [NOTE] Resize events on the colourmap will cause the canvas to be
    refreshed.
    <win_scale> The window scaling information. The <<x_offset>> <<x_pixels>>
    <<y_offset>> and <<y_pixels>> fields are determined internally. If this is
    NULL a default scale is choosen.
    [RETURNS] A world canvas object on success, else NULL.
*/
{
    KWorldCanvas canvas;
    int width, height;
    struct win_scale_type win_scale_local;
    static char function_name[] = "canvas_create";

    if (pixcanvas == NULL)
    {
	fprintf (stderr, "NULL KPixCanvas passed\n");
	a_prog_bug (function_name);
    }
    if (win_scale == NULL)
    {
	canvas_init_win_scale (&win_scale_local, K_WIN_SCALE_MAGIC_NUMBER);
	win_scale = &win_scale_local;
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
    if (cmap == NULL)
    {
	canvas->cmap_num_pixels = 0;
	canvas->cmap_rgb_values = NULL;
    }
    else
    {
	canvas->cmap_num_pixels = kcmap_get_pixels (cmap,
						    &canvas->cmap_pixels);
	canvas->cmap_rgb_values = kcmap_get_rgb_values (cmap, NULL);
    }
    canvas->auto_min_sat = FALSE;
    canvas->auto_max_sat = FALSE;
    m_copy ( (char *) &canvas->win_scale, (char *) win_scale,
	     sizeof *win_scale );
    canvas->win_scale.x_offset = 0;
    canvas->win_scale.x_pixels = width;
    canvas->win_scale.y_offset = 0;
    canvas->win_scale.y_pixels = height;
    canvas->size_control_func = NULL;
    canvas->size_control_info = NULL;
    canvas->coords_convert_func = NULL;
    canvas->deprecated_coord_d_convert_func = NULL;
    canvas->deprecated_coord_convert_func = NULL;
    canvas->coord_convert_info = NULL;
    canvas->coord_transforms_func = NULL;
    canvas->deprecated_coord_transform_func = NULL;
    canvas->coord_transform_info = NULL;
    canvas->refresh_list = NULL;
    canvas->position_list = NULL;
    canvas->quash_negotiate = FALSE;
    canvas->in_size_control_func = FALSE;
    canvas->specify_xlabel[0] = '\0';
    canvas->specify_ylabel[0] = '\0';
    canvas->num_restrictions = 0;
    canvas->restrictions_allocated = 0;
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
    kwin_register_position_event_func (pixcanvas,
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
/*  [SUMMARY] Register world canvas refresh callback.
    [PURPOSE] This routine will register a refresh function for a world canvas.
    The refresh function will be called whenever the contents of the canvas
    need to be redrawn. Many refresh functions may be registered per canvas.
    The first function registered is the first function called upon refresh.
    <canvas> The world canvas.
    <refresh_func> The function that is called when the canvas is to be
    refreshed. The prototype function is [<CANVAS_PROTO_refresh_func>].
    <info> The initial arbitrary canvas information pointer.
    [RETURNS] A KCallbackFunc object.
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
/*  [SUMMARY] Register size control callback.
    [PURPOSE] This routine will register the size control function for a world
    canvas. This function will be called whenever the lower level pixel canvas
    resizes, or the associated Kcolourmap object is resized, OR a call is made
    to [<canvas_resize>]. The function may alter the window scaling information
    for the canvas. Only one size control function is permitted per canvas. The
    size control routine is called before any refresh functions are called.
    This is a means for a higher level object to take control of the world
    canvas.
    <canvas> The world canvas object.
    <size_control_func> The function that is called when the canvas is to be
    refreshed. The prototype function is [<CANVAS_PROTO_size_control_func>].
    <info> The initial arbitrary canvas information pointer.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_register_size_control_func";

    VERIFY_CANVAS (canvas);
    if (size_control_func == NULL) return;
    if (canvas->size_control_func != NULL)
    {
	fprintf (stderr, "size_control_func already registered\n");
	a_prog_bug (function_name);
    }
    canvas->size_control_func = size_control_func;
    canvas->size_control_info = info;
}   /*  End Function canvas_register_size_control_func  */

/*PUBLIC_FUNCTION*/
KCallbackFunc canvas_register_position_event_func (KWorldCanvas canvas,
						   flag (*func) (),
						   void *f_info)
/*  [SUMMARY] Register position event callback.
    [PURPOSE] This routine will register a position event function for a world
    canvas. The position event function will be called whenever a position
    event on the canvas has not been consumed. Many position event functions
    may be registered per canvas. The first function registered is the first
    function called upon a position event.
    <canvas> The world canvas.
    <func> The function that is called when a position event occurs. The
    prototype function is [<CANVAS_PROTO_position_func>].
    <f_info> The initial arbitrary function information pointer.
    [RETURNS] A KCallbackFunc object.
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
void canvas_get_attributes (KWorldCanvas canvas, ...)
/*  [SUMMARY] Get canvas attributes.
    <canvas> The world canvas.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pointer pairs. The list must be terminated with
    CANVAS_ATT_END. See [<CANVAS_ATTRIBUTES>] for the list of attributes.
    [RETURNS] Nothing.
*/
{
    va_list argp;
    unsigned int att_key;
    static char function_name[] = "canvas_get_attributes";

    va_start (argp, canvas);
    VERIFY_CANVAS (canvas);
    while ( ( att_key = va_arg (argp, unsigned int) ) != CANVAS_ATT_END )
    {
	switch (att_key)
	{
	  case CANVAS_ATT_X_OFFSET:
	    *( va_arg (argp, int *) ) = canvas->win_scale.x_offset;
	    break;
	  case CANVAS_ATT_Y_OFFSET:
	    *( va_arg (argp, int *) ) = canvas->win_scale.y_offset;
	    break;
	  case CANVAS_ATT_X_PIXELS:
	    *( va_arg (argp, int *) ) = canvas->win_scale.x_pixels;
	    break;
	  case CANVAS_ATT_Y_PIXELS:
	    *( va_arg (argp, int *) ) = canvas->win_scale.y_pixels;
	    break;
	  case CANVAS_ATT_BLANK_PIXEL:
	    *( va_arg (argp, unsigned long *) ) =canvas->win_scale.blank_pixel;
	    break;
	  case CANVAS_ATT_MIN_SAT_PIXEL:
	    *( va_arg (argp, unsigned long *) ) = canvas->win_scale.min_sat_pixel;
	    break;
	  case CANVAS_ATT_MAX_SAT_PIXEL:
	    *( va_arg (argp, unsigned long *) ) = canvas->win_scale.max_sat_pixel;
	    break;
	  case CANVAS_ATT_LEFT_X:
	    *( va_arg (argp, double *) ) = canvas->win_scale.left_x;
	    break;
	  case CANVAS_ATT_RIGHT_X:
	    *( va_arg (argp, double *) ) = canvas->win_scale.right_x;
	    break;
	  case CANVAS_ATT_BOTTOM_Y:
	    *( va_arg (argp, double *) ) = canvas->win_scale.bottom_y;
	    break;
	  case CANVAS_ATT_TOP_Y:
	    *( va_arg (argp, double *) ) = canvas->win_scale.top_y;
	    break;
	  case CANVAS_ATT_VALUE_MIN:
	    *( va_arg (argp, double *) ) = canvas->win_scale.z_min;
	    break;
	  case CANVAS_ATT_VALUE_MAX:
	    *( va_arg (argp, double *) ) = canvas->win_scale.z_max;
	    break;
	  case CANVAS_ATT_ISCALE_FUNC:
	    *( va_arg (argp, void **) ) =(void *)canvas->win_scale.iscale_func;
	    break;
	  case CANVAS_ATT_ISCALE_FREE_FUNC:
	    *( va_arg (argp, void **) ) =
		(void *) canvas->win_scale.iscale_free_info_func;
	    break;
	  case CANVAS_ATT_ISCALE_INFO:
	    *( va_arg (argp, void **) ) = canvas->win_scale.iscale_info;
	    break;
	  case CANVAS_ATT_AUTO_MIN_SAT:
	    *( va_arg (argp, flag *) ) = canvas->auto_min_sat;
	    break;
	  case CANVAS_ATT_AUTO_MAX_SAT:
	    *( va_arg (argp, flag *) ) = canvas->auto_max_sat;
	    break;
	  default:
	    fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
}   /*  End Function canvas_get_attributes  */

/*PUBLIC_FUNCTION*/
flag canvas_set_attributes (KWorldCanvas canvas, ...)
/*  [SUMMARY] Set canvas attributes.
    <canvas> The world canvas.
    [VARARGS] The optional attributes are given as pairs of attribute-key
    attribute-value pairs. The list must be terminated with
    CANVAS_ATT_END. See [<CANVAS_ATTRIBUTES>] for the list of attributes.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    unsigned int att_key;
    static char function_name[] = "canvas_set_attributes";

    va_start (argp, canvas);
    VERIFY_CANVAS (canvas);
    while ( ( att_key = va_arg (argp, unsigned int) ) != CANVAS_ATT_END )
    {
	switch (att_key)
	{
	  case CANVAS_ATT_X_OFFSET:
	    canvas->win_scale.x_offset = va_arg (argp, int);
	    break;
	  case CANVAS_ATT_Y_OFFSET:
	    canvas->win_scale.y_offset = va_arg (argp, int);
	    break;
	  case CANVAS_ATT_X_PIXELS:
	    canvas->win_scale.x_pixels = va_arg (argp, int);
	    break;
	  case CANVAS_ATT_Y_PIXELS:
	    canvas->win_scale.y_pixels = va_arg (argp, int);
	    break;
	  case CANVAS_ATT_BLANK_PIXEL:
	    canvas->win_scale.blank_pixel = va_arg (argp, unsigned long);
	    break;
	  case CANVAS_ATT_MIN_SAT_PIXEL:
	    canvas->win_scale.min_sat_pixel = va_arg (argp, unsigned long);
	    break;
	  case CANVAS_ATT_MAX_SAT_PIXEL:
	    canvas->win_scale.max_sat_pixel = va_arg (argp, unsigned long);
	    break;
	  case CANVAS_ATT_LEFT_X:
	    canvas->win_scale.left_x = va_arg (argp, double);
	    break;
	  case CANVAS_ATT_RIGHT_X:
	    canvas->win_scale.right_x = va_arg (argp, double);
	    break;
	  case CANVAS_ATT_BOTTOM_Y:
	    canvas->win_scale.bottom_y = va_arg (argp, double);
	    break;
	  case CANVAS_ATT_TOP_Y:
	    canvas->win_scale.top_y = va_arg (argp, double);
	    break;
	  case CANVAS_ATT_VALUE_MIN:
	    canvas->win_scale.z_min = va_arg (argp, double);
	    break;
	  case CANVAS_ATT_VALUE_MAX:
	    canvas->win_scale.z_max = va_arg (argp, double);
	    break;
	  case CANVAS_ATT_ISCALE_FUNC:
	    *(void **) &canvas->win_scale.iscale_func = va_arg (argp, void *);
	    break;
	  case CANVAS_ATT_ISCALE_FREE_FUNC:
	    *(void **) &canvas->win_scale.iscale_free_info_func =
		va_arg (argp, void *);
	    break;
	  case CANVAS_ATT_ISCALE_INFO:
	    canvas->win_scale.iscale_info = va_arg (argp, void *);
	    break;
	  case CANVAS_ATT_AUTO_MIN_SAT:
	    canvas->auto_min_sat = va_arg (argp, flag);
	    break;
	  case CANVAS_ATT_AUTO_MAX_SAT:
	    canvas->auto_max_sat = va_arg (argp, flag);
	    break;
	  default:
	    fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    verify_win_scale (&canvas->win_scale, function_name);
    set_sat_pixels (canvas);
    return (TRUE);
}   /*  End Function canvas_set_attributes  */

/*PUBLIC_FUNCTION*/
flag canvas_resize (KWorldCanvas canvas, struct win_scale_type *win_scale,
		    flag always_clear)
/*  [SUMMARY] Resize a world canvas.
    [PURPOSE] This routine will register a resize in the world canvas size.
    This will cause any refresh routines registered for the canvas to be
    called. The associated pixel canvas is refreshed, hence, refresh routines
    registered for the pixel canvas will also be called.
    <canvas> The world canvas object.
    <win_scale> The new window scaling information. If this is NULL no change
    is made to the window scaling information, and a simple refresh occurs.
    <always_clear> If FALSE, <<win_scale>> is NULL and there is a size
    control function registered for the world canvas, then if that function
    does not change the values of the <<x_offset>>, <<x_pixels>>, <<y_offset>>
    or <<y_pixels>> fields in the window scaling information, the associated
    pixel canvas is NOT cleared. Under all other circumstances, the pixel
    canvas is cleared prior to calling any refresh functions.
    [RETURNS] TRUE on success, else FALSE.
*/
{
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
	    fprintf (stderr, "Pixel canvas not big enough\n");
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
	kwin_clear (canvas->pixcanvas, 0, 0, -1, canvas->win_scale.y_offset);
	/*  Now the bottom  */
	kwin_clear (canvas->pixcanvas,
		    0, canvas->win_scale.y_offset + canvas->win_scale.y_pixels,
		    -1, -1);
	/*  Now the bit at the left between the top and bottom bits  */
	kwin_clear (canvas->pixcanvas,
		    0, canvas->win_scale.y_offset,
		    canvas->win_scale.x_offset, canvas->win_scale.y_pixels);
	/*  And finally the bit at the right between the top and bottom bits */
	kwin_clear (canvas->pixcanvas,
		    canvas->win_scale.x_offset + canvas->win_scale.x_pixels,
		    canvas->win_scale.y_offset,
		    -1, canvas->win_scale.y_pixels);
	return ( kwin_resize (canvas->pixcanvas, FALSE, 0, 0, 0, 0) );
    }
    return ( kwin_resize (canvas->pixcanvas, TRUE, 0, 0, 0, 0) );
}   /*  End Function canvas_resize  */

/*PUBLIC_FUNCTION*/
flag canvas_specify (KWorldCanvas canvas, char *xlabel, char *ylabel,
		     unsigned int num_restr, char **restr_names,
		     double *restr_values)
/*  [SUMMARY] Set specification information for a world canvas.
    [PURPOSE] This routine will change the specifications for a canvas. This
    includes the horizontal and vertical dimension labels and any restrictions.
    The entire specification is copied (i.e. the inputs may be freed).
    <canvas> The world canvas object.
    <xlabel> The horizontal dimension label.
    <ylabel> The vertical dimension label.
    <num_restr> The number of restrictions.
    <restr_names> The array of restriction names.
    <restr_values> The restriction values.
    [NOTE] This routine DOES NOT cause the canvas to be refreshed. It is highly
    recommended that the canvas is refreshed after this routine, as higher
    level packages may depend on the specification information.
    [NOTE] All string values are copied.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int count;
    char **names;
    double *values;
    static char function_name[] = "canvas_specify";

    VERIFY_CANVAS (canvas);
    if (xlabel == NULL) canvas->specify_xlabel[0] = '\0';
    else strcpy (canvas->specify_xlabel, xlabel);
    if (ylabel == NULL) canvas->specify_ylabel[0] = '\0';
    else strcpy (canvas->specify_ylabel, ylabel);
    if (num_restr > canvas->restrictions_allocated)
    {
	/*  Allocate restriction space  */
	if ( ( names = (char **) m_alloc (sizeof *names * num_restr) ) ==NULL )
	{
	    m_error_notify (function_name, "array of restriction names");
	    return (FALSE);
	}
	if ( ( values = (double *) m_alloc (sizeof *values * num_restr) )
	     == NULL )
	{
	    m_error_notify (function_name, "array of restriction values");
	    m_free ( (char *) names );
	    return (FALSE);
	}
	for (count = 0; count < num_restr; ++count)
	{
	    if ( ( names[count] = m_alloc (STRING_LENGTH) ) == NULL )
	    {
		m_error_notify (function_name, "restriction names");
		m_free ( (char *) names );
		m_free ( (char *) values );
		while (count > 0)
		{
		    m_free (names[count]);
		    --count;
		}
		return (FALSE);
	    }
	}
	/*  Free old  */
	if (canvas->restriction_names != NULL)
	{
	    for (count = 0; count < canvas->restrictions_allocated; ++count)
	    {
		m_free (canvas->restriction_names[count]);
	    }
	    m_free ( (char *) canvas->restriction_names );
	}
	if (canvas->restriction_values != NULL)
	    m_free ( (char *) canvas->restriction_values );
	canvas->restriction_names = names;
	canvas->restriction_values = values;
	canvas->restrictions_allocated = num_restr;
    }
    else
    {
	names = canvas->restriction_names;
	values = canvas->restriction_values;
    }
    /*  Copy the restriction data  */
    canvas->num_restrictions = num_restr;
    for (count = 0; count < num_restr; ++count)
    {
	strncpy (names[count], restr_names[count], STRING_LENGTH - 1);
	names[count][STRING_LENGTH - 1] = '\0';
	values[count] = restr_values[count];
    }
    return (TRUE);
}   /*  End Function canvas_specify  */

/*PUBLIC_FUNCTION*/
void canvas_get_specification (KWorldCanvas canvas, char **xlabel,
			       char **ylabel, unsigned int *num_restr,
			       char ***restr_names, double **restr_values)
/*  [SUMMARY] Get specification information for a world canvas.
    [PURPOSE] This routine will get the specifications for a canvas. This
    includes the horizontal and vertical dimension labels and any restrictions.
    Only pointers are copied (i.e. the outputs may NOT be freed).
    <canvas> The world canvas object.
    <xlabel> The pointer to the horizontal dimension label will be written here
    <ylabel> The pointer to the vertical dimension label will be written here.
    <num_restr> The number of restrictions will be written here.
    <restr_names> The pointer to the array of restriction names will be written
    here.
    <restr_values> The pointer to the array of restriction values will be
    written here.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_get_specification";

    VERIFY_CANVAS (canvas);
    if (canvas->specify_xlabel[0] == '\0') *xlabel = NULL;
    else *xlabel = canvas->specify_xlabel;
    if (canvas->specify_ylabel[0] == '\0') *ylabel = NULL;
    else *ylabel = canvas->specify_ylabel;
    *num_restr = canvas->num_restrictions;
    *restr_names = canvas->restriction_names;
    *restr_values = canvas->restriction_values;
}   /*  End Function canvas_get_specification  */

/*PUBLIC_FUNCTION*/
void canvas_convert_to_canvas_coords (KWorldCanvas canvas, flag clip,
				      unsigned int num_coords,
				      CONST double *xin, CONST double *yin,
				      double *xout_lin, double *yout_lin,
				      double *xout, double *yout)
/*  [SUMMARY] Convert pixel co-ordinates to world co-ordinates.
    <canvas> The world canvas object.
    <clip> If TRUE, pixel co-ordinates are first clipped to the canvas
    boundaries prior to conversion.
    <num_coords> The number of co-ordinates to convert.
    <xin> The array of input horizontal pixel co-ordinates.
    <yin> The array of input vertical pixel co-ordinates.
    <xout_lin> The array of output horizontal linear world co-ordinates are
    written here. If this is NULL, nothing is written here.
    <yout_lin> The array of output vertical linear world co-ordinates are
    written here. If this is NULL, nothing is written here.
    <xout> The array of output horizontal non-linear world co-ordinates are
    written here. If this is NULL, nothing is written here.
    <yout> The array of output vertical non-linear world co-ordinates are
    written here. If this is NULL, nothing is written here.
    [RETURNS] Nothing.
*/
{
    flag converted;
    int px, py;
    unsigned int count;
    double dx, dy;
    struct win_scale_type win_scale;
    static char function_name[] = "canvas_convert_to_canvas_coords";

    VERIFY_CANVAS (canvas);
    for (count = 0; count < num_coords; ++count)
    {
	dx = xin[count];
	dy = yin[count];
	if ( clip && (dx < canvas->win_scale.x_offset) )
	{
	    dx = canvas->win_scale.x_offset;
	}
	if ( clip && (dx >= canvas->win_scale.x_offset +
		      canvas->win_scale.x_pixels) )
	{
	    dx = canvas->win_scale.x_offset + canvas->win_scale.x_pixels - 1;
	}
	if ( clip && (dy < canvas->win_scale.y_offset) )
	{
	    dy = canvas->win_scale.y_offset;
	}
	if ( clip && (dy >= canvas->win_scale.y_offset +
		      canvas->win_scale.y_pixels) )
	{
	    dy = canvas->win_scale.y_offset + canvas->win_scale.y_pixels - 1;
	}
	converted = FALSE;
	if (canvas->coords_convert_func != NULL)
	{
	    converted = ( (*canvas->coords_convert_func)
			  (canvas, 1, &dx, &dy, &dx, &dy, TRUE,
			   &canvas->coord_convert_info) );
	}
	else if (canvas->deprecated_coord_d_convert_func != NULL)
	{
	    m_copy ( (char *) &win_scale, (char *) &canvas->win_scale,
		     sizeof win_scale );
	    converted = ( (*canvas->deprecated_coord_d_convert_func)
			  (canvas, &win_scale, &dx, &dy, TRUE,
			   &canvas->coord_convert_info) );
	}
	else if (canvas->deprecated_coord_convert_func != NULL)
	{
	    m_copy ( (char *) &win_scale, (char *) &canvas->win_scale,
		     sizeof win_scale );
	    px = dx;
	    py = dy;
	    converted = ( (*canvas->deprecated_coord_convert_func)
			  (canvas, &win_scale, &px, &py, &dx, &dy, TRUE,
			   &canvas->coord_convert_info) );
	}
	if (!converted)
	{
	    /*  Convert pixel co-ordinates to world co-ordinates  */
	    /*  Convert x  */
	    dx = dx - canvas->win_scale.x_offset;
	    dx /= (double) (canvas->win_scale.x_pixels - 1);
	    dx *= (canvas->win_scale.right_x - canvas->win_scale.left_x);
	    dx += canvas->win_scale.left_x;
	    /*  Flip vertical  */
	    dy -= canvas->win_scale.y_offset;
	    dy = canvas->win_scale.y_pixels - dy - 1;
	    /*  Convert y  */
	    dy /= (double) (canvas->win_scale.y_pixels - 1);
	    dy *= (canvas->win_scale.top_y - canvas->win_scale.bottom_y);
	    dy += canvas->win_scale.bottom_y;
	}
	/*  dx and dy should now be linear world co-ordinates  */
	if (xout_lin != NULL) xout_lin[count] = dx;
	if (yout_lin != NULL) yout_lin[count] = dy;
	/*  Apply non-linear transform function  */
	if ( (xout != NULL) || (yout != NULL) )
	{
	    canvas_coords_transform (canvas, 1, &dx, FALSE, &dy, FALSE);
	    if (xout != NULL) xout[count] = dx;
	    if (yout != NULL) yout[count] = dy;
	}
    }
}   /*  End Function canvas_convert_to_canvas_coords  */

/*PUBLIC_FUNCTION*/
void canvas_convert_from_canvas_coords (KWorldCanvas canvas,
					flag clip, flag linear,
					unsigned int num_coords,
					CONST double *xin, CONST double *yin,
					double *xout, double *yout)
/*  [SUMMARY] Convert world co-ordinates to pixel co-ordinates.
    <canvas> The world canvas object.
    <clip> If TRUE, world co-ordinates are first clipped to the canvas
    boundaries prior to conversion.
    <linear> If TRUE, the input world co-ordinates are assumed to be linear,
    hence no non-linear to linear co-ordinate transformation is applied.
    <num_coords> The number of co-ordinates to convert.
    <xin> The array of input horizontal world co-ordinates.
    <yin> The array of input vertical world co-ordinates.
    <xout> The array of output horizontal pixel co-ordinates are written here.
    <yout> The array of output vertical pixel co-ordinates are written here.
    [RETURNS] Nothing.
*/
{
    flag converted = FALSE;
    int ix, iy;
    unsigned int count;
    double px, py;
    struct win_scale_type win_scale;
    static char function_name[] = "canvas_convert_from_canvas_coords";

    VERIFY_CANVAS (canvas);
    if ( (canvas->coord_transforms_func == NULL) &&
	 (canvas->deprecated_coord_transform_func == NULL) ) linear = TRUE;
    if (!linear)
    {
	/*  Input is non-linear: first convert to linear co-ordinates  */
	/*  Copy input to working array  */
	for (count = 0; count < num_coords; ++count) xout[count] = xin[count];
	for (count = 0; count < num_coords; ++count) yout[count] = yin[count];
	canvas_coords_transform (canvas, num_coords, xout, TRUE, yout, TRUE);
	if ( !clip && (canvas->coords_convert_func != NULL) )
	{
	    if ( (*canvas->coords_convert_func) (canvas, num_coords,
						 xout, yout, xout, yout, FALSE,
						 &canvas->coord_convert_info) )
		return;
	}
	/*  Co-ordinates have not been converted to pixel co-ordinates yet  */
	canvas_convert_from_canvas_coords (canvas, clip, TRUE, num_coords,
					   xout, yout, xout, yout);
	return;
    }
    /*  Linear conversion only  */
    if ( !clip && (canvas->coords_convert_func != NULL) )
    {
	if ( (*canvas->coords_convert_func) (canvas, num_coords,
					     xin, yin, xout, yout, FALSE,
					     &canvas->coord_convert_info) )
	    return;
    }
    /*  Have to do this the hard way  */
    m_copy ( (char *) &win_scale, (char *) &canvas->win_scale,
	     sizeof win_scale );
    for (count = 0; count < num_coords; ++count)
    {
	px = xin[count];
	py = yin[count];
	if (clip)
	{
	    if (win_scale.left_x < win_scale.right_x)
	    {
		if (px < win_scale.left_x) px = win_scale.left_x;
		else if (px > win_scale.right_x) px = win_scale.right_x;
	    }
	    else
	    {
		if (px > win_scale.left_x) px = win_scale.left_x;
		else if (px < win_scale.right_x) px = win_scale.right_x;
	    }
	    if (win_scale.bottom_y < win_scale.top_y)
	    {
		if (py < win_scale.bottom_y) py = win_scale.bottom_y;
		else if (py > win_scale.top_y) py = win_scale.top_y;
	    }
	    else
	    {
		if (py > win_scale.bottom_y) py = win_scale.bottom_y;
		else if (py < win_scale.top_y) py = win_scale.top_y;
	    }
	}
	if (canvas->coords_convert_func != NULL)
	{
	    converted = ( (*canvas->coords_convert_func)
			  (canvas, 1, &px, &py, &px, &py, FALSE,
			   &canvas->coord_convert_info) );
	}
	else if (canvas->deprecated_coord_d_convert_func != NULL)
	{
	    converted = ( (*canvas->deprecated_coord_d_convert_func)
			  (canvas, &win_scale, &px, &py, FALSE,
			   &canvas->coord_convert_info) );
	}
	else if (canvas->deprecated_coord_convert_func != NULL)
	{
	    converted = ( (*canvas->deprecated_coord_convert_func)
			  (canvas, &win_scale, &ix, &iy, &px, &px, FALSE,
			   &canvas->coord_convert_info) );
	    px = ix;
	    py = iy;
	}
	if (!converted)
	{
	    /*  Convert x  */
	    px = px - (double) canvas->win_scale.left_x;
	    px /= (double) (canvas->win_scale.right_x - canvas->win_scale.left_x);
	    px *= (double) (canvas->win_scale.x_pixels - 1);
	    px += 0.01 + (double) canvas->win_scale.x_offset;
	    /*  Convert y  */
	    py = py - canvas->win_scale.bottom_y;
	    py /= canvas->win_scale.top_y - canvas->win_scale.bottom_y;
	    py *= (double) (canvas->win_scale.y_pixels - 1);
	    py = (double) (canvas->win_scale.y_offset +
			   canvas->win_scale.y_pixels - 1) - (py + 0.01);
	}
	xout[count] = px;
	yout[count] = py;
    }
}   /*  End Function canvas_convert_from_canvas_coords  */

/*PUBLIC_FUNCTION*/
void canvas_register_coords_convert_func (KWorldCanvas canvas,
					  flag (*coord_convert_func) (),
					  void *info)
/*  [SUMMARY] Register co-ordinate conversion function.
    [PURPOSE] This routine will register the co-ordinate conversion function
    for a world canvas. This function will be called whenever conversions
    between world co-ordinates and pixel co-ordinates are performed. Only one
    co-ordinate conversion function is permitted per canvas. This is a means
    for a higher level object to take control of the world canvas.
    <canvas> The canvas.
    <func> The function that is called when co-ordinates are to be converted.
    The prototype function is [<CANVAS_PROTO_coord_convert_func>].
    <info> The initial arbitrary canvas information pointer.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_register_coords_convert_func";

    VERIFY_CANVAS (canvas);
    if (coord_convert_func == NULL) return;
    if (canvas->coords_convert_func != NULL)
    {
	fprintf (stderr, "coord_convert_func already registered\n");
	a_prog_bug (function_name);
    }
    canvas->coords_convert_func = coord_convert_func;
    canvas->coord_convert_info = info;
}   /*  End Function canvas_register_coords_convert_func  */

/*PUBLIC_FUNCTION*/
void canvas_coords_transform (KWorldCanvas canvas, unsigned int num_coords,
			      double *x, flag x_to_linear,
			      double *y, flag y_to_linear)
/*  [SUMMARY] Transform world co-ordinates.
    [PURPOSE] This routine will transform co-ordinates in a world canvas using
    the registered co-ordinate transform function (see
    [<canvas_register_transforms_func>]).
    <canvas> The world canvas object.
    <num_coords> The number of co-ordinates to transform.
    <x> The array of horizontal world co-ordinates. These are modified.
    <x_to_linear> If TRUE, then the horizontal co-ordinates are converted from
    non-linear to linear, else they are converted from linear to non-linear.
    <y> The array of vertical world co-ordinates. These are modified.
    <y_to_linear> If TRUE, then the vertical co-ordinates are converted from
    non-linear to linear, else they are converted from linear to non-linear.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    static char function_name[] = "canvas_coords_transform";

    VERIFY_CANVAS (canvas);
    if (canvas->coord_transforms_func != NULL)
    {
	(*canvas->coord_transforms_func) (canvas, num_coords,
					  x, x_to_linear, y, y_to_linear,
					  canvas->win_scale.left_x,
					  canvas->win_scale.right_x,
					  canvas->win_scale.bottom_y,
					  canvas->win_scale.top_y,
					  &canvas->coord_transform_info);
	return;
    }
    if (canvas->deprecated_coord_transform_func != NULL)
    {
	if (x_to_linear != y_to_linear)
	{
	    fputs ("x_to_linear and y_to_linear do not match: deprecated ",
		   stderr);
	    fputs ("co-ordinate transform\nroutine not supported\n", stderr);
	    return;
	}
	for (count = 0; count < num_coords; ++count)
	{
	    (*canvas->deprecated_coord_transform_func)
		(x + count, y + count, x_to_linear,
		 canvas->win_scale.left_x,
		 canvas->win_scale.right_x,
		 canvas->win_scale.bottom_y,
		 canvas->win_scale.top_y,
		 &canvas->coord_transform_info);
	}
    }
}   /*  End Function canvas_coords_transform  */

/*PUBLIC_FUNCTION*/
void canvas_register_transforms_func (KWorldCanvas canvas,
				      void (*coord_transform_func) (),
				      void *info)
/*  [SUMMARY] Register non-linear co-ordinate transformation function.
    [PURPOSE] This routine will register the co-ordinate transform function
    for a world canvas. The co-ordinate transform function transforms the
    linear world co-ordinates used internally by the world canvas to be
    transformed into non-linear co-ordinates for external use. Only one
    co-ordinate transform function is permitted per canvas.
    <canvas> The world canvas object.
    <coord_transform_func> The function that is called when co-ordinates are to
    be transformed. The prototype function is
    [<CANVAS_PROTO_coord_transform_func>].
    <info> The initial arbitrary transform information pointer.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_register_transforms_func";

    VERIFY_CANVAS (canvas);
    if (coord_transform_func == NULL) return;
    if (canvas->coord_transforms_func != NULL)
    {
	fprintf (stderr, "coord_transform_func already registered\n");
	a_prog_bug (function_name);
    }
    canvas->coord_transforms_func = coord_transform_func;
    canvas->coord_transform_info = info;
}   /*  End Function canvas_register_transforms_func  */

/*PUBLIC_FUNCTION*/
KPixCanvas canvas_get_pixcanvas (KWorldCanvas canvas)
/*  [SUMMARY] Get the underlying pixel canvas of a world canvas.
    <canvas> The world canvas object.
    [RETURNS] The KPixCanvas object.
*/
{
    static char function_name[] = "canvas_get_pixcanvas";

    VERIFY_CANVAS (canvas);
    return (canvas->pixcanvas);
}   /*  End Function canvas_get_pixcanvas  */

/*PUBLIC_FUNCTION*/
flag canvas_get_colour (KWorldCanvas canvas, CONST char *colourname,
			unsigned long *pixel_value, unsigned short *red,
			unsigned short *green, unsigned short *blue)
/*  [SUMMARY] Get (possibly allocating) a colourcell for a canvas.
    <canvas> The world canvas object.
    <colourname> The name of the colour to get.
    <pixel_value> The pixel value will be written here.
    [NOTE] The pixel value is valid ONLY for the underlying pixel canvas.
    ALSO: the pixel value becomes invalid EVERY time the underlying pixel
    canvas is refreshed/ resized. If the pixel canvas is refreshed/ resized,
    this routine MUST be called again.
    <red> The red intensity in the hardware colourmap for the pixel will be
    written here. If this is NULL, nothing is written here.
    <green> The green intensity in the hardware colourmap for the pixel will be
    written here. If this is NULL, nothing is written here.
    <blue> The blue intensity in the hardware colourmap for the pixel will be
    written here. If this is NULL, nothing is written here.
    [RETURNS] TRUE if the colourcell was allocated, else FALSE.
*/
{
    double value[2];
    static char function_name[] = "canvas_get_colour";

    VERIFY_CANVAS (canvas);
    if ( (canvas->cmap != NULL) && (colourname[0] == '!') )
    {
	/*  Embedded value in colourname  */
	value[0] = 0.0;
	value[1] = 0.0;
	sscanf (colourname + 1, "%le %le", value, value + 1);
	return get_pixel_from_value (canvas, value, red, green, blue);
    }
    return kwin_get_colour (canvas->pixcanvas, colourname, pixel_value,
			    red, green, blue);
}   /*  End Function canvas_get_colour  */

/*PUBLIC_FUNCTION*/
void canvas_set_dressing (KWorldCanvas canvas, ...)
/*  [SUMMARY] Set dressing parameters for a world canvas.
    <canvas> The world canvas.
    [VARARGS] The list of parameter attribute-key attribute-value pairs. This
    list must be terminated with the value KCD_END.
    [NOTE] The dressing code is not yet complete.
    [RETURNS] Nothing.
*/
{
    va_list arg_pointer;
    flag bool;
    flag no_changes = TRUE;
    unsigned int att_key;
    char *string, *ptr;
    static char function_name[] = "canvas_set_dressing";

    va_start (arg_pointer, canvas);
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
	    fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (arg_pointer);
    if (no_changes) return;
    /*  Refresh canvas  */
    canvas_resize (canvas, (struct win_scale_type *) NULL, TRUE);
}   /*  End Function canvas_set_dressing  */

/*PUBLIC_FUNCTION*/
void canvas_sequence_dressing_refresh (KWorldCanvas canvas)
/*  [SUMMARY] Set order in which dressing is refreshed.
    [PURPOSE] This routine will sequence the dressing (axes, etc.) refresh
    function for a world canvas. If this routine is not called, the dressing
    will be drawn after *all* registered refresh functions have been called
    (see [<canvas_register_refresh_func>]). If this routine is called, all
    preceeding refresh functions (registered with
    [<canvas_register_refresh_func>]) will be called on refresh prior to the
    dressing being drawn and all refresh functions registered after this
    routine is called will be called *after* the dressing is drawn.
    <canvas> The world canvas object.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_sequence_dressing_refresh";

    VERIFY_CANVAS (canvas);
    canvas_register_refresh_func (canvas, dressing_refresh_func, (void *)NULL);
}   /*  End Function canvas_sequence_dressing_refresh  */

/*PUBLIC_FUNCTION*/
void canvas_draw_dressing (KWorldCanvas canvas)
/*  [SUMMARY] Draw the dressing (axes, etc.) for a world canvas.
    [NOTE] This routine is called automatically each time the canvas is
    refreshed.
    <canvas> The world canvas object.
    [RETURNS] Nothing.
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
	fprintf (stderr,
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
/*  [SUMMARY] Get the Kcolourmap object associated with a world canvas.
    <canvas> The world canvas object.
    [RETURNS] The Kcolourmap object. This may be NULL (indicating there is no
    colourmap associated with the canvas).
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
/*  [SUMMARY] Draw a PseudoColour image onto a world canvas.
    [PURPOSE] This routine will draw a 2-dimensional PseudoColour slice of a
    Karma array onto a world canvas. This slice may be tiled.
    <canvas> The world canvas object.
    <arr_desc> The array descriptor for the image.
    <slice> The start of the slice (image) data.
    <hdim> The dimension index of the horizontal dimension.
    <vdim> The dimension index of the vertical dimension.
    <elem_index> The element index of the data packets.
    <cache_ptr> The routine may produce cache data which will vastly increase
    the speed of subsequent operations on this data. The routine will write a
    pointer to this data here. Prior to process exit, a call MUST be made to
    [<kwin_free_cache_data>], otherwise shared memory segments could remain
    after the process exits.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int abs_start_index;
    unsigned int abs_end_index;
    unsigned int ord_start_index;
    unsigned int ord_end_index;
    unsigned int num_abs_coords;
    unsigned int num_ord_coords;
    unsigned int num_pixels;
    uaddr *hoffsets, *voffsets;
    packet_desc *pack_desc;
    dim_desc *hdim_desc, *vdim_desc;
    unsigned long *pixel_values;
    static char function_name[] = "canvas_draw_image";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    num_pixels = kcmap_get_pixels (canvas->cmap, &pixel_values);
    pack_desc = arr_desc->packet;
    if (elem_index >= pack_desc->num_elements)
    {
	fprintf (stderr,
			"elem_index: %u greater than number of elements: %u\n",
			elem_index, pack_desc->num_elements);
	a_prog_bug (function_name);
    }
    hoffsets = arr_desc->offsets[hdim];
    voffsets = arr_desc->offsets[vdim];
    hdim_desc = arr_desc->dimensions[hdim];
    vdim_desc = arr_desc->dimensions[vdim];
    /*  Determine start and stop co-ordinates along each dimension  */
    abs_start_index = ds_get_coord_num (hdim_desc, canvas->win_scale.left_x,
					SEARCH_BIAS_CLOSEST);
    abs_end_index = ds_get_coord_num (hdim_desc, canvas->win_scale.right_x,
				      SEARCH_BIAS_CLOSEST);
    if (abs_start_index >= abs_end_index)
    {
	fprintf (stderr,
			"Left co-ordinate index: %u not less than right: %u\n",
			abs_start_index, abs_end_index);
	a_prog_bug (function_name);
    }
    num_abs_coords = abs_end_index - abs_start_index + 1;
    ord_start_index = ds_get_coord_num (vdim_desc, canvas->win_scale.bottom_y,
					SEARCH_BIAS_CLOSEST);
    ord_end_index = ds_get_coord_num (vdim_desc, canvas->win_scale.top_y,
				      SEARCH_BIAS_CLOSEST);
    if (ord_start_index >= ord_end_index)
    {
	fprintf (stderr,
			"Bottom co-ordinate index: %u not less than top: %u\n",
			ord_start_index, ord_end_index);
	a_prog_bug (function_name);
    }
    num_ord_coords = ord_end_index - ord_start_index + 1;
    return ( kwin_draw_pc_image (canvas->pixcanvas,
				 canvas->win_scale.x_offset,
				 canvas->win_scale.y_offset,
				 canvas->win_scale.x_pixels,
				 canvas->win_scale.y_pixels,
				 slice + ds_get_element_offset (pack_desc,
								elem_index),
				 hoffsets + abs_start_index,
				 voffsets + ord_start_index,
				 num_abs_coords, num_ord_coords,
				 pack_desc->element_types[elem_index],
				 canvas->win_scale.conv_type,
				 num_pixels, pixel_values,
				 canvas->win_scale.blank_pixel,
				 canvas->win_scale.min_sat_pixel,
				 canvas->win_scale.max_sat_pixel,
				 canvas->win_scale.z_min,
				 canvas->win_scale.z_max,
				 canvas->win_scale.iscale_func,
				 canvas->win_scale.iscale_info,
				 cache_ptr) );
}   /*  End Function canvas_draw_image  */

/*PUBLIC_FUNCTION*/
flag canvas_draw_rgb_image (KWorldCanvas canvas, array_desc *arr_desc,
			    CONST char *slice,
			    unsigned int hdim, unsigned int vdim,
			    unsigned int red_index, unsigned int green_index,
			    unsigned int blue_index,
			    KPixCanvasImageCache *cache_ptr)
/*  [SUMMARY] Draw a TrueColour image onto a world canvas.
    [PURPOSE] This routine will draw a 2-dimensional TrueColour slice of a
    Karma array onto a world canvas. This slice may be tiled.
    <canvas> The world canvas.
    <arr_desc> The array descriptor for the image.
    <slice> The start of the slice (image) data.
    <hdim> The dimension index of the horizontal dimension.
    <vdim> The dimension index of the vertical dimension.
    <red_index> The element index of the red components.
    <green_index> The element index of the green components.
    <blue_index> The element index of the blue components.
    <cache_ptr> The routine may produce cache data which will vastly increase
    the speed of subsequent operations on this data. The routine will write a
    pointer to this data here. Prior to process exit, a call MUST be made to
    [<kwin_free_cache_data>], otherwise shared memory segments could remain
    after the process exits.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int abs_start_index;
    unsigned int abs_end_index;
    unsigned int ord_start_index;
    unsigned int ord_end_index;
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
	fprintf (stderr,
			"red_index: %u greater than number of elements: %u\n",
			red_index, pack_desc->num_elements);
	a_prog_bug (function_name);
    }
    if (pack_desc->element_types[red_index] != K_UBYTE)
    {
	fprintf (stderr, "Red component type: %u is not K_UBYTE\n",
			pack_desc->element_types[red_index]);
	return (FALSE);
    }
    if (green_index >= pack_desc->num_elements)
    {
	fprintf(stderr,
		       "green_index: %u greater than number of elements: %u\n",
		       green_index, pack_desc->num_elements);
	a_prog_bug (function_name);
    }
    if (pack_desc->element_types[green_index] != K_UBYTE)
    {
	fprintf (stderr, "Green component type: %u is not K_UBYTE\n",
			pack_desc->element_types[green_index]);
	return (FALSE);
    }
    if (blue_index >= pack_desc->num_elements)
    {
	fprintf (stderr,
			"blue_index: %u greater than number of elements: %u\n",
			blue_index, pack_desc->num_elements);
	a_prog_bug (function_name);
    }
    if (pack_desc->element_types[blue_index] != K_UBYTE)
    {
	fprintf (stderr, "Blue component type: %u is not K_UBYTE\n",
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
    abs_start_index = ds_get_coord_num (hdim_desc, canvas->win_scale.left_x,
					SEARCH_BIAS_CLOSEST);
    abs_end_index = ds_get_coord_num (hdim_desc, canvas->win_scale.right_x,
				      SEARCH_BIAS_CLOSEST);
    if (abs_start_index >= abs_end_index)
    {
	fprintf (stderr,
			"Left co-ordinate index: %u not less than right: %u\n",
			abs_start_index, abs_end_index);
	a_prog_bug (function_name);
    }
    num_abs_coords = abs_end_index - abs_start_index + 1;
    ord_start_index = ds_get_coord_num (vdim_desc, canvas->win_scale.bottom_y,
					SEARCH_BIAS_CLOSEST);
    ord_end_index = ds_get_coord_num (vdim_desc, canvas->win_scale.top_y,
				      SEARCH_BIAS_CLOSEST);
    if (ord_start_index >= ord_end_index)
    {
	fprintf (stderr,
			"Bottom co-ordinate index: %u not less than top: %u\n",
			ord_start_index, ord_end_index);
	a_prog_bug (function_name);
    }
    num_ord_coords = ord_end_index - ord_start_index + 1;
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
				  hoffsets + abs_start_index,
				  voffsets + ord_start_index,
				  num_abs_coords, num_ord_coords, cache_ptr) );
}   /*  End Function canvas_draw_rgb_image  */

/*PUBLIC_FUNCTION*/
void canvas_draw_point (KWorldCanvas canvas, double x, double y,
			double value[2])
/*  [SUMMARY] Draw a single point onto a world canvas.
    <canvas> The world canvas object.
    <x> The horizontal world co-ordinate of the point.
    <y> The vertical world co-ordinate of the point.
    <value> The data value to use. This is of type K_DCOMPLEX.
    [RETURNS] Nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_point";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (canvas, value, NULL, NULL, NULL);
    canvas_draw_point_p (canvas, x, y, pixel_value);
}   /*  End Function canvas_draw_point  */

/*PUBLIC_FUNCTION*/
void canvas_draw_point_p (KWorldCanvas canvas, double x, double y,
			  unsigned long pixel_value)
/*  [SUMMARY] Draw a single point onto a world canvas.
    <canvas> The world canvas object.
    <x> The horizontal world co-ordinate of the point.
    <y> The vertical world co-ordinate of the point.
    <pixel_value> The pixel value to use.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_draw_point_p";

    VERIFY_CANVAS (canvas);
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
				       &x, &y, &x, &y);
    if (x < canvas->win_scale.x_offset) return;
    if (x >= canvas->win_scale.x_offset + canvas->win_scale.x_pixels) return;
    if (y < canvas->win_scale.y_offset) return;
    if (y >= canvas->win_scale.y_offset + canvas->win_scale.y_pixels) return;
    kwin_draw_point (canvas->pixcanvas, x, y, pixel_value);
}   /*  End Function canvas_draw_point_p  */

/*PUBLIC_FUNCTION*/
void canvas_draw_line (KWorldCanvas canvas,
		       double x0, double y0, double x1, double y1,
		       double value[2])
/*  [SUMMARY] Draw a single line onto a world canvas.
    <canvas> The world canvas object.
    <x0> The horizontal world co-ordinate of the first point.
    <y0> The vertical world co-ordinate of the first point.
    <x1> The horizontal world co-ordinate of the second point.
    <y1> The vertical world co-ordinate of the second point.
    <value> The data value to use. This is of type K_DCOMPLEX.
    [RETURNS] Nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_line";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (canvas, value, NULL, NULL, NULL);
    canvas_draw_line_p (canvas, x0, y0, x1, y1, pixel_value);
}   /*  End Function canvas_draw_line  */

/*PUBLIC_FUNCTION*/
void canvas_draw_line_p (KWorldCanvas canvas,
			 double x0, double y0, double x1, double y1,
			 unsigned long pixel_value)
/*  [SUMMARY] Draw a single line onto a world canvas.
    <canvas> The world canvas object.
    <x0> The horizontal world co-ordinate of the first point.
    <y0> The vertical world co-ordinate of the first point.
    <x1> The horizontal world co-ordinate of the second point.
    <y1> The vertical world co-ordinate of the second point.
    <pixel_value> The pixel value to use.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_draw_line_p";

    VERIFY_CANVAS (canvas);
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
				       &x0, &y0, &x0, &y0);
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
				       &x1, &y1, &x1, &y1);
    kwin_draw_line (canvas->pixcanvas, x0, y0, x1, y1, pixel_value);
}   /*  End Function canvas_draw_line_p  */

/*PUBLIC_FUNCTION*/
void canvas_draw_ellipse (KWorldCanvas canvas,
			  double centre_x, double centre_y,
			  double radius_x, double radius_y, double value[2])
/*  [SUMMARY] Draw an ellipse onto a world canvas.
    <canvas> The world canvas object.
    <centre_x> The horizontal world co-ordinate of the centre of the ellipse.
    <centre_y> The vertical world co-ordinate of the centre of the ellipse.
    <radius_x> The horizontal radius in world co-ordinates.
    <radius_y> The vertical radius in world co-ordinates.
    <value> The complex value to draw the ellipse with. This must be of type
    K_DCOMPLEX.
    [RETURNS] Nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_ellipse";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (canvas, value, NULL, NULL, NULL);
    canvas_draw_ellipse_p (canvas,
			   centre_x, centre_y, radius_x, radius_y,
			   pixel_value);
}   /*  End Function canvas_draw_ellipse  */

/*PUBLIC_FUNCTION*/
void canvas_draw_ellipse_p (KWorldCanvas canvas,
			    double centre_x, double centre_y,
			    double radius_x, double radius_y,
			    unsigned long pixel_value)
/*  [SUMMARY] Draw an ellipse onto a world canvas.
    <canvas> The world canvas object.
    <centre_x> The horizontal world co-ordinate of the centre of the ellipse.
    <centre_y> The vertical world co-ordinate of the centre of the ellipse.
    <radius_x> The horizontal radius in world co-ordinates.
    <radius_y> The vertical radius in world co-ordinates.
    <pixel_value> The pixel value to draw the ellipse with.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_draw_ellipse_p";

    VERIFY_CANVAS (canvas);
    /*  Offset radius value  */
    radius_x += centre_x;
    radius_y += centre_y;
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
				       &centre_x, &centre_y,
				       &centre_x, &centre_y);
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
				       &radius_x, &radius_y,
				       &radius_x, &radius_y);
    /*  Undo radius offset  */
    radius_x = fabs (radius_x - centre_x);
    radius_y = fabs (radius_y - centre_y);
    kwin_draw_ellipse (canvas->pixcanvas, centre_x, centre_y,
		       radius_x, radius_y, pixel_value);
}   /*  End Function canvas_draw_ellipse_p  */

/*PUBLIC_FUNCTION*/
void canvas_fill_ellipse (KWorldCanvas canvas,
			  double centre_x, double centre_y,
			  double radius_x, double radius_y, double value[2])
/*  [SUMMARY] Draw a filled ellipse onto a world canvas.
    <canvas> The world canvas object.
    <centre_x> The horizontal world co-ordinate of the centre of the ellipse.
    <centre_y> The vertical world co-ordinate of the centre of the ellipse.
    <radius_x> The horizontal radius in world co-ordinates.
    <radius_y> The vertical radius in world co-ordinates.
    <value> The complex value to fill the ellipse with. This must be of type
    K_DCOMPLEX.
    [RETURNS] Nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_fill_ellipse";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (canvas, value, NULL, NULL, NULL);
    canvas_fill_ellipse_p (canvas,
			   centre_x, centre_y, radius_x, radius_y,
			   pixel_value);
}   /*  End Function canvas_fill_ellipse  */

/*PUBLIC_FUNCTION*/
void canvas_fill_ellipse_p (KWorldCanvas canvas,
			    double centre_x, double centre_y,
			    double radius_x, double radius_y,
			    unsigned long pixel_value)
/*  [SUMMARY] Draw a filled ellipse onto a world canvas.
    <canvas> The world canvas object.
    <centre_x> The horizontal world co-ordinate of the centre of the ellipse.
    <centre_y> The vertical world co-ordinate of the centre of the ellipse.
    <radius_x> The horizontal radius in world co-ordinates.
    <radius_y> The vertical radius in world co-ordinates.
    <pixel_value> The pixel value to fill the ellipse with.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_fill_ellipse_p";

    VERIFY_CANVAS (canvas);
    /*  Offset radius value  */
    radius_x += centre_x;
    radius_y += centre_y;
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
				       &centre_x, &centre_y,
				       &centre_x, &centre_y);
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
				       &radius_x, &radius_y,
				       &radius_x, &radius_y);
    /*  Undo radius offset  */
    radius_x = fabs (radius_x - centre_y);
    radius_y = fabs (radius_y - centre_y);
    kwin_fill_ellipse (canvas->pixcanvas, centre_x, centre_y,
		       radius_x, radius_y, pixel_value);
}   /*  End Function canvas_fill_ellipse_p  */

/*PUBLIC_FUNCTION*/
flag canvas_fill_polygon (KWorldCanvas canvas, edit_coord *coords,
			  unsigned int num_vertices, double value[2],
			  flag convex)
/*  [SUMMARY] Draw a filled polygon onto a world canvas.
    <canvas> The world canvas object.
    <coords> The array of world co-ordinates of vertices of the polygon.
    <num_vertices> The number of vertices in the polygon.
    <value> The complex value to fill the polygon with. This must be of type
    K_DCOMPLEX.
    <convex> If TRUE, then the points must form a convex polygon.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int coord_count;
    unsigned long pixel_value;
    double px, py;
    static unsigned int num_points_allocated = 0;
    static int *point_x = NULL;
    static int *point_y = NULL;
    static char function_name[] = "canvas_fill_polygon";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	fprintf (stderr,"Canvas has no colourmap associated with it\n");
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
	canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
					   &coords[coord_count].abscissa,
					   &coords[coord_count].ordinate,
					   &px, &py);
	point_x[coord_count] = px;
	point_y[coord_count] = py;
    }
    pixel_value = get_pixel_from_value (canvas, value, NULL, NULL, NULL);
    return ( kwin_fill_polygon (canvas->pixcanvas,
				point_x, point_y, num_vertices,
				pixel_value, convex) );
}   /*  End Function canvas_fill_polygon  */

/*PUBLIC_FUNCTION*/
void canvas_draw_rectangle (KWorldCanvas canvas, double x, double y,
			    double width, double height, double value[2])
/*  [SUMMARY] Draw a single rectangle onto a world canvas.
    <canvas> The world canvas object.
    <x> The horizontal offset of the rectangle.
    <y> The vertical offset of the rectangle.
    <width> The width of the rectangle.
    <height> The height of the rectangle.
    <value> The data value to use. This is of type K_DCOMPLEX.
    [RETURNS] Nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_rectangle";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (canvas, value, NULL, NULL, NULL);
    canvas_draw_rectangle_p (canvas, x, y, width, height, pixel_value);
}   /*  End Function canvas_draw_rectangle  */

/*PUBLIC_FUNCTION*/
void canvas_draw_rectangle_p (KWorldCanvas canvas, double x, double y,
			      double width, double height,
			      unsigned long pixel_value)
/*  [SUMMARY] Draw a single rectangle onto a world canvas.
    <canvas> The world canvas object.
    <x> The horizontal offset of the rectangle.
    <y> The vertical offset of the rectangle.
    <width> The width of the rectangle.
    <height> The height of the rectangle.
    <pixel_value> The pixel value to use.
    [RETURNS] Nothing.
*/
{
    double pwidth, pheight, py1;
    double px, py;
    static char function_name[] = "canvas_draw_rectangle_p";

    VERIFY_CANVAS (canvas);
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
				       &x, &y, &px, &py);
    /*  Offset width and height  */
    x += width;
    y += height;
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
				       &x, &y, &pwidth, &py1);
    /*  Undo radius offset  */
    pwidth -= px;
    pheight = py - py1;
    kwin_draw_rectangle (canvas->pixcanvas,
			 px, py1, pwidth, pheight, pixel_value);
}   /*  End Function canvas_draw_rectangle_p  */

/*PUBLIC_FUNCTION*/
void canvas_fill_rectangle (KWorldCanvas canvas, double x, double y,
			    double width, double height, double value[2])
/*  [SUMMARY] Fill a single rectangle onto a world canvas.
    <canvas> The world canvas object.
    <x> The horizontal offset of the rectangle.
    <y> The vertical offset of the rectangle.
    <width> The width of the rectangle.
    <height> The height of the rectangle.
    <value> The data value to use. This is of type K_DCOMPLEX.
    [RETURNS] Nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_fill_rectangle";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (canvas, value, NULL, NULL, NULL);
    canvas_fill_rectangle_p (canvas, x, y, width, height, pixel_value);
}   /*  End Function canvas_fill_rectangle  */

/*PUBLIC_FUNCTION*/
void canvas_fill_rectangle_p (KWorldCanvas canvas, double x, double y,
			      double width, double height,
			      unsigned long pixel_value)
/*  [SUMMARY] Fill a single rectangle onto a world canvas.
    <canvas> The world canvas object.
    <x> The horizontal offset of the rectangle.
    <y> The vertical offset of the rectangle.
    <width> The width of the rectangle.
    <height> The height of the rectangle.
    <pixel_value> The pixel value to use.
    [RETURNS] Nothing.
*/
{
    double pwidth, pheight, py1;
    double px, py;
    static char function_name[] = "canvas_fill_rectangle_p";

    VERIFY_CANVAS (canvas);
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
				       &x, &y, &px, &py);
    /*  Offset width and height  */
    x += width;
    y += height;
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1,
				       &x, &y, &pwidth, &py1);
    /*  Undo radius offset  */
    pwidth -= px;
    pheight = py - py1;
    kwin_fill_rectangle (canvas->pixcanvas,
			 px, py1, pwidth, pheight, pixel_value);
}   /*  End Function canvas_fill_rectangle_p  */

/*PUBLIC_FUNCTION*/
void canvas_draw_lines (KWorldCanvas canvas,
			CONST double *x_array, CONST double *y_array,
			int num_points, double value[2])
/*  [SUMMARY] Draw multiple connected lines onto a world canvas.
    <canvas> The world canvas object.
    <x_array> The horizontal world co-ordinates of the points. If this is NULL,
    the routine assumes horizontal co-ordinates equally spaced across the world
    canvas.
    <y_array> The vertical world co-ordinates of the points. If this is NULL,
    the routine assumes vertical co-ordinates equally spaced across the world
    canvas.
    <num_points> The number of points. The number of lines drawn is 1 less than
    this value.
    <value> The data value to use. This is of type K_DCOMPLEX.
    [RETURNS] Nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_lines";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (canvas, value, NULL, NULL, NULL);
    canvas_draw_lines_p (canvas, x_array, y_array, num_points, pixel_value);
}   /*  End Function canvas_draw_lines  */

/*PUBLIC_FUNCTION*/
void canvas_draw_lines_p (KWorldCanvas canvas,
			  CONST double *x_array, CONST double *y_array,
			  int num_points, unsigned long pixel_value)
/*  [SUMMARY] Draw multiple connected lines onto a world canvas.
    <canvas> The world canvas object.
    <x_array> The horizontal world co-ordinates of the points. If this is NULL,
    the routine assumes horizontal co-ordinates equally spaced across the world
    canvas.
    <y_array> The vertical world co-ordinates of the points. If this is NULL,
    the routine assumes vertical co-ordinates equally spaced across the world
    canvas.
    <num_points> The number of points. The number of lines drawn is 1 less than
    this value.
    <pixel_value> The pixel value to use.
    [RETURNS] Nothing.
*/
{
    flag draw_block;
    int count, pcount;
    double xscale, yscale, wx, wy;
    double wlx, wby, wrx, wty;
    double px[COORD_BUF_SIZE], py[COORD_BUF_SIZE];
    static char function_name[] = "canvas_draw_lines_p";

    VERIFY_CANVAS (canvas);
    /*  Compute extrema in non-linear world co-ordinates  */
    wlx = canvas->win_scale.left_x;
    wby = canvas->win_scale.bottom_y;
    canvas_coords_transform (canvas, 1, &wlx, FALSE, &wby, FALSE);
    wrx = canvas->win_scale.right_x;
    wty = canvas->win_scale.top_y;
    canvas_coords_transform (canvas, 1, &wrx, FALSE, &wty, FALSE);
    xscale = (wrx - wlx) / (double) (num_points - 1);
    yscale = (wty - wby) / (double) (num_points - 1);
    /*  Divide request into managable blocks  */
    for (count = 0, pcount = 0, draw_block = FALSE; count < num_points;
	 ++count)
    {
	if (x_array == NULL) wx = wlx + xscale * (double) count;
	else if ( (wx = x_array[count]) >= TOOBIG ) draw_block = TRUE;
	if (y_array == NULL) wy = wby + yscale * (double) count;
	else if ( (wy = y_array[count]) >= TOOBIG ) draw_block = TRUE;
	if (!draw_block)
	{
	    px[pcount] = wx;
	    py[pcount] = wy;
	    if (++pcount >= COORD_BUF_SIZE) draw_block = TRUE;
	    if (count >= num_points - 1) draw_block = TRUE;
	}
	if (draw_block)
	{
	    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, pcount,
					       px, py, px, py);
	    kwin_draw_lines_TRANSITION (canvas->pixcanvas, px, py, pcount,
					pixel_value);
	    draw_block = FALSE;
	    pcount = 0;
	}
    }
}   /*  End Function canvas_draw_lines_p  */

/*PUBLIC_FUNCTION*/
void canvas_draw_segments (KWorldCanvas canvas,
			   CONST double *x0, CONST double *y0,
			   CONST double *x1, CONST double *y1,
			   int num_segments, double value[2])
/*  [SUMMARY] Draw multiple disjoint lines onto a world canvas.
    <canvas> The world canvas object.
    <x0> The horizontal start co-ordinates of the segments.
    <y0> The vetical start co-ordinates of the segments.
    <x1> The horizontal end co-ordinates of the segments.
    <y1> The vetical end co-ordinates of the segments.
    <num_segments> The number of segments.
    <value> The data value to use. This is of type K_DCOMPLEX.
    [RETURNS] Nothing.
*/
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_segments";

    VERIFY_CANVAS (canvas);
    if (canvas->cmap == NULL)
    {
	fprintf (stderr,"Canvas has no colourmap associated with it\n");
	a_prog_bug (function_name);
    }
    pixel_value = get_pixel_from_value (canvas, value, NULL, NULL, NULL);
    canvas_draw_segments_p (canvas, x0, y0, x1, y1, num_segments, pixel_value);
}   /*  End Function canvas_draw_segments  */

/*PUBLIC_FUNCTION*/
void canvas_draw_segments_p (KWorldCanvas canvas,
			     CONST double *x0, CONST double *y0,
			     CONST double *x1, CONST double *y1,
			     int num_segments, unsigned long pixel_value)
/*  [SUMMARY] Draw multiple disjoint lines onto a world canvas.
    <canvas> The world canvas object.
    <x0> The horizontal start co-ordinates of the segments.
    <y0> The vetical start co-ordinates of the segments.
    <x1> The horizontal end co-ordinates of the segments.
    <y1> The vetical end co-ordinates of the segments.
    <num_segments> The number of segments.
    <pixel_value> The pixel value to use.
    [RETURNS] Nothing.
*/
{
    int num_seg;
    double px0[COORD_BUF_SIZE], py0[COORD_BUF_SIZE];
    double px1[COORD_BUF_SIZE], py1[COORD_BUF_SIZE];
    static char function_name[] = "canvas_draw_segments_p";

    VERIFY_CANVAS (canvas);
    /*  Divide request into managable blocks  */
    for (; num_segments > 0; num_segments -= num_seg,
	     x0 += num_seg, y0 += num_seg, x1 += num_seg, y1 += num_seg)
    {
	num_seg = (num_segments>COORD_BUF_SIZE) ? COORD_BUF_SIZE :num_segments;
	/*  Convert to pixel co-ordinates  */
	canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, num_seg,
					   x0, y0, px0, py0);
	canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, num_seg,
					   x1, y1, px1, py1);
	kwin_draw_segments_TRANSITION (canvas->pixcanvas, px0, py0, px1, py1,
				       num_seg, pixel_value);
    }
}   /*  End Function canvas_draw_segments_p  */


/*  Obsolete public functions follow  */

/*OBSOLETE_FUNCTION*/
void canvas_get_size (KWorldCanvas canvas, int *width, int *height,
		      struct win_scale_type *win_scale)
/*  [SUMMARY] Get the size of a world canvas.
    <canvas> The world canvas object.
    <width> The number of horizontal pixels will be written here.
    <height> The number of vertical pixels will be written here.
    <win_scale> The window scaling information will be written here.
    [RETURNS] Nothing.
*/
{
    static flag first_time = TRUE;
    static char function_name[] = "canvas_get_size";

    VERIFY_CANVAS (canvas);
    if (win_scale == NULL)
    {
	fputs ("NULL win_scale structure pointer passed\n", stderr);
	a_prog_bug (function_name);
    }
    if (first_time)
    {
	fprintf (stderr,
			"WARNING: the <%s> routine will be removed in Karma",
			function_name);
	fputs (" version 2.0\nUse ", stderr);
	fputs ("the <canvas_get_attributes> routine instead\n", stderr);
	first_time = FALSE;
    }
    kwin_get_size (canvas->pixcanvas, width, height);
    m_copy ( (char *) win_scale, (char *) &canvas->win_scale,
	    sizeof *win_scale );
}   /*  End Function canvas_get_size  */

/*OBSOLETE_FUNCTION*/
flag canvas_convert_to_canvas_coord (KWorldCanvas canvas,
				     double xin, double yin,
				     double *xout, double *yout)
/*  [SUMMARY] Convert pixel co-ordinates to world co-ordinates.
    <canvas> The world canvas object.
    <xin> The lower level horizontal co-ordinate.
    <yin> The lower level vertical co-ordinate.
    <xout> The horizontal world co-ordinate will be written here.
    <yout> The vertical world co-ordinate will be written here.
    [RETURNS] TRUE if the co-ordinate lies within the canvas boundaries,
    else FALSE (although a conversion is still performed).
*/
{
    flag in_bounds = TRUE;
    static flag first_time = TRUE;
    static char function_name[] = "canvas_convert_to_canvas_coord";

    VERIFY_CANVAS (canvas);
    if (first_time)
    {
	fprintf (stderr,
			"WARNING: the <%s> routine will be removed in Karma",
			function_name);
	fputs (" version 2.0\nUse the ", stderr);
	fputs ("<canvas_convert_to_canvas_coords> routine instead\n",
		      stderr);
	first_time = FALSE;
    }
    if ( (xin < canvas->win_scale.x_offset) ||
	(xin >= canvas->win_scale.x_offset + canvas->win_scale.x_pixels) ||
	(yin < canvas->win_scale.y_offset) ||
	(yin >= canvas->win_scale.y_offset + canvas->win_scale.y_pixels) )
	in_bounds = FALSE;
    canvas_convert_to_canvas_coords (canvas, FALSE, 1, &xin, &yin,
				     NULL, NULL, xout, yout);
    return (in_bounds);
}   /*  End Function canvas_convert_to_canvas_coord  */

/*OBSOLETE_FUNCTION*/
flag canvas_convert_from_canvas_coord (KWorldCanvas canvas,
				       double xin, double yin,
				       int *xout, int *yout)
/*  [SUMMARY] Convert non-linear world co-ordinates to pixel co-ordinates
    <canvas> The world canvas object.
    <xin> The horizontal world co-ordinate.
    <yin> The vertical world co-ordinate.
    <xout> The lower level horizontal canvas co-ordinate will be written here.
    <yout> The lower level vertical canvas co-ordinate will be written here.
    [RETURNS] TRUE if the co-ordinate lies within the canvas boundaries,
    else FALSE (although a conversion is still performed).
*/
{
    static flag first_time = TRUE;
    static char function_name[] = "canvas_convert_from_canvas_coord";

    VERIFY_CANVAS (canvas);
    if (first_time)
    {
	fprintf (stderr,
			"WARNING: the <%s> routine will be removed in Karma",
			function_name);
	fputs (" version 2.0\nUse the ", stderr);
	fputs ("<canvas_convert_from_canvas_coords> routine instead\n",
		      stderr);
	first_time = FALSE;
    }
    canvas_convert_from_canvas_coords (canvas, FALSE, FALSE, 1, &xin, &yin,
				       &xin, &yin);
    *xout = xin;
    *yout = yin;
    if (xin < canvas->win_scale.x_offset) return (FALSE);
    if (xin >= canvas->win_scale.x_offset + canvas->win_scale.x_pixels)
	return (FALSE);
    if (yin < canvas->win_scale.y_offset) return (FALSE);
    if (yin >= canvas->win_scale.y_offset + canvas->win_scale.y_pixels)
	return (FALSE);
    return (TRUE);
}   /*  End Function canvas_convert_from_canvas_coord  */

/*OBSOLETE_FUNCTION*/
void canvas_register_d_convert_func (KWorldCanvas canvas,
				     flag (*coord_convert_func) (), void *info)
/*  [SUMMARY] Register co-ordinate conversion function.
    [PURPOSE] This routine will register the co-ordinate conversion function
    for a world canvas. This function will be called whenever conversions
    between world co-ordinates and pixel co-ordinates are performed. Only one
    co-ordinate conversion function is permitted per canvas. This is a means
    for a higher level object to take control of the world canvas.
    <canvas> The canvas.
    <func> The function that is called when co-ordinates are to be converted.
    The prototype function is [<CANVAS_PROTO_coord_convert_func>].
    <info> The initial arbitrary canvas information pointer.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_register_d_convert_func";

    VERIFY_CANVAS (canvas);
    fprintf (stderr, "WARNING: the <%s> routine will be removed in Karma",
	     function_name);
    fprintf (stderr, " version 2.0\nUse ");
    fprintf (stderr,
	     "the <canvas_register_coords_convert_func> routine instead\n");
    if (coord_convert_func == NULL) return;
    if ( (canvas->coords_convert_func != NULL) ||
	 (canvas->deprecated_coord_d_convert_func != NULL) )
    {
	fprintf (stderr, "coord_convert_func already registered\n");
	a_prog_bug (function_name);
    }
    canvas->deprecated_coord_d_convert_func = coord_convert_func;
    canvas->coord_convert_info = info;
}   /*  End Function canvas_register_d_convert_func  */

/*OBSOLETE_FUNCTION*/
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

    flag coord_convert_func (KWorldCanvas canvas,
                             struct win_scale_type *win_scale,
			     int *px, int *py, double *wx, double *wy,
			     flag to_world, void **info)
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
    ;

    The initial arbitrary canvas information pointer must be given by  info  .
    The routine returns nothing.
*/
{
    static char function_name[] = "canvas_register_convert_func";

    VERIFY_CANVAS (canvas);
    fprintf (stderr, "WARNING: the <%s> routine will be removed in Karma",
	     function_name);
    fprintf (stderr, " version 2.0\nUse ");
    fprintf (stderr,
	     "the <canvas_register_coords_convert_func> routine instead\n");
    if (coord_convert_func == NULL) return;
    if ( (canvas->coords_convert_func != NULL) ||
	 (canvas->deprecated_coord_d_convert_func != NULL) ||
	 (canvas->deprecated_coord_convert_func != NULL) )
    {
	fprintf (stderr, "coord_convert_func already registered\n");
	a_prog_bug (function_name);
    }
    canvas->deprecated_coord_convert_func = coord_convert_func;
    canvas->coord_convert_info = info;
}   /*  End Function canvas_register_convert_func  */

/*OBSOLETE_FUNCTION*/
flag canvas_coord_transform (KWorldCanvas canvas, double *x, double *y,
			     flag to_linear)
/*  [SUMMARY] Transform world co-ordinates.
    [PURPOSE] This routine will transform co-ordinates in a world canvas using
    the registered co-ordinate transform function (see
    [<canvas_register_transforms_func>]).
    <canvas> The world canvas object.
    <x> A pointer to the horizontal world co-ordinate. This is modified.
    <y> A pointer to the vertical world co-ordinate. This is modified.
    <to_linear> If TRUE then the routine will transform (possibly) non-linear
    world co-ordinates to linear world co-ordinates, else the transform is from
    linear to non-linear co-ordinates.
    [RETURNS] TRUE if the linear world co-ordinate lies within the canvas
    boundaries, else FALSE (although a conversion is still performed).
*/
{
    flag in_bounds = TRUE;
    static char function_name[] = "canvas_coord_transform";

    VERIFY_CANVAS (canvas);
    fprintf (stderr, "WARNING: the <%s> routine will be removed in Karma",
	     function_name);
    fprintf (stderr, " version 2.0\nUse ");
    fprintf (stderr,
	     "the <canvas_coords_transform> routine instead\n");
    if (!to_linear)
    {
	/*  Input data are linear co-ordinates: test bounds  */
	if (canvas->win_scale.left_x < canvas->win_scale.right_x)
	{
	    if ( (*x < canvas->win_scale.left_x) ||
		 (*x > canvas->win_scale.right_x) ) in_bounds = FALSE;
	}
	else if ( (*x > canvas->win_scale.left_x) ||
		  (*x < canvas->win_scale.right_x) ) in_bounds = FALSE;
	if (canvas->win_scale.bottom_y < canvas->win_scale.top_y)
	{
	    if ( (*y < canvas->win_scale.bottom_y) ||
		 (*y > canvas->win_scale.top_y) ) in_bounds = FALSE;
	}
	else if ( (*y > canvas->win_scale.bottom_y) ||
		  (*y < canvas->win_scale.top_y) ) in_bounds = FALSE;
    }
    canvas_coords_transform (canvas, 1, x, to_linear, y, to_linear);
    if (to_linear)
    {
	/*  Output data are linear co-ordinates: test bounds  */
	if (canvas->win_scale.left_x < canvas->win_scale.right_x)
	{
	    if ( (*x < canvas->win_scale.left_x) ||
		 (*x > canvas->win_scale.right_x) ) in_bounds = FALSE;
	}
	else if ( (*x > canvas->win_scale.left_x) ||
		  (*x < canvas->win_scale.right_x) ) in_bounds = FALSE;
	if (canvas->win_scale.bottom_y < canvas->win_scale.top_y)
	{
	    if ( (*y < canvas->win_scale.bottom_y) ||
		 (*y > canvas->win_scale.top_y) ) in_bounds = FALSE;
	}
	else if ( (*y > canvas->win_scale.bottom_y) ||
		  (*y < canvas->win_scale.top_y) ) in_bounds = FALSE;
    }
    return (in_bounds);
}   /*  End Function canvas_coord_transform  */

/*OBSOLETE_FUNCTION*/
void canvas_register_transform_func (KWorldCanvas canvas,
				     void (*coord_transform_func) (),
				     void *info)
/*  [SUMMARY] Register non-linear co-ordinate transformation function.
    [PURPOSE] This routine will register the co-ordinate transform function
    for a world canvas. The co-ordinate transform function transforms the
    linear world co-ordinates used internally by the world canvas to be
    transformed into non-linear co-ordinates for external use. Only one
    co-ordinate transform function is permitted per canvas.
    <canvas> The world canvas object.
    <coord_transform_func> The function that is called when co-ordinates are to
    be transformed. The prototype function is
    [<CANVAS_PROTO_coord_transform_func>].
    <info> The initial arbitrary transform information pointer.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_register_transform_func";

    VERIFY_CANVAS (canvas);
    fprintf (stderr,
		    "WARNING: the <%s> routine will be removed in Karma",
		    function_name);
    fprintf (stderr, " version 2.0\nUse ");
    fprintf (stderr,
		    "the <canvas_register_transforms_func> routine instead\n");
    if (coord_transform_func == NULL) return;
    if ( (canvas->coord_transforms_func != NULL) ||
	 (canvas->deprecated_coord_transform_func != NULL) )
    {
	fprintf (stderr, "coord_transform_func already registered\n");
	a_prog_bug (function_name);
    }
    canvas->deprecated_coord_transform_func = coord_transform_func;
    canvas->coord_transform_info = info;
}   /*  End Function canvas_register_transform_func  */


/*  Private functions follow  */

static void pixcanvas_refresh_func (KPixCanvas pixcanvas, int width,int height,
				    void **info, PostScriptPage pspage,
				    unsigned int num_areas,
				    KPixCanvasRefreshArea *areas,
				    flag *honoured_areas)
/*  [PURPOSE] This routine is a refresh event consumer for a pixel canvas.
    <canvas> The pixel canvas.
    <width> The width of the canvas in pixels.
    <height> The height of the canvas in pixels.
    <info> A pointer to the arbitrary information pointer.
    <pspage> The PostScriptPage object. If this is NULL, the refresh is *not*
    destined for a PostScript page.
    <num_areas> The number of areas that need to be refreshed. If this is
    0 then the entire pixel canvas needs to be refreshed.
    <areas> The list of areas that need to be refreshed.
    <honoured_areas> If the value TRUE is written here it is assumed the
    routine honoured the list of refresh areas and did not write outside
    these areas and hence the list of areas will be passed to subsequent
    registered refresh routines. If FALSE is written here (or nothing is
    written here), implying the routine refreshed the entire pixel canvas,
    subsequent refresh routines will be told to refresh the entire canvas.
    [RETURNS] Nothing.
*/
{
    KWorldCanvas canvas;
    static char function_name[] = "__canvas_pixcanvas_refresh_func";

    canvas = (KWorldCanvas) *info;
    VERIFY_CANVAS (canvas);
    if (pixcanvas != canvas->pixcanvas)
    {
	fprintf (stderr, "Pixel canvases do not match\n");
	a_prog_bug (function_name);
    }
    refresh_canvas (canvas, width, height, FALSE, pspage,
		    num_areas, areas, honoured_areas);
}   /*  End Function pixcanvas_refresh_func  */

static void cmap_resize_func (Kcolourmap cmap, void **info)
/*  This routine registers a change in the size of a colourmap.
    The colourmap must be given by  cmap  .
    The arbitrary colourmap information pointer is pointed to by  info  .
    [RETURNS] Nothing.
*/
{
    KWorldCanvas canvas;
    int width, height;
    static char function_name[] = "__canvas_cmap_resize_func";

    canvas = (KWorldCanvas) *info;
    VERIFY_CANVAS (canvas);
    if (cmap != canvas->cmap)
    {
	fprintf (stderr, "Colourmaps do not match\n");
	a_prog_bug (function_name);
    }
    if (canvas->in_size_control_func) return;
    set_sat_pixels (canvas);
    m_free ( (char *) canvas->cmap_rgb_values );
    canvas->cmap_num_pixels = kcmap_get_pixels (cmap, &canvas->cmap_pixels);
    canvas->cmap_rgb_values = kcmap_get_rgb_values (cmap, NULL);
    /*  Get pixel canvas size  */
    kwin_get_size (canvas->pixcanvas, &width, &height);
    refresh_canvas (canvas, width, height, TRUE, NULL, 0, NULL, NULL);
}   /*  End Function cmap_resize_func  */

static void refresh_canvas (KWorldCanvas canvas, int width, int height,
			    flag cmap_resize, PostScriptPage pspage,
			    unsigned int num_areas,
			    KPixCanvasRefreshArea *areas, flag *honoured_areas)
/*  [PURPOSE] This routine will refresh a world canvas (by calling all
    registered refresh functions).
    <canvas> The world canvas.
    <width> The width of of the canvas in pixels.
    <height> The height of the canvas in pixels.
    <cmap_resize> If TRUE, the refresh is due to a colourmap resize.
    <pspage> The PostScriptPage object. If this is NULL, the refresh is *not*
    destined for a PostScript page.
    <num_areas> The number of areas that need to be refreshed. If this is
    0 then the entire pixel canvas needs to be refreshed.
    <areas> The list of areas that need to be refreshed.
    <honoured_areas> If the value TRUE is written here it is assumed the
    routine honoured the list of refresh areas and did not write outside
    these areas and hence the list of areas will be passed to subsequent
    registered refresh routines. If FALSE is written here (or nothing is
    written here), implying the routine refreshed the entire pixel canvas,
    subsequent refresh routines will be told to refresh the entire canvas.
    [RETURNS] Nothing.
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
	    fprintf (stderr, "Pixel canvas not big enough\n");
	    return;
	}
    }
    /*  Call refresh functions  */
    data.width = width;
    data.height = height;
    data.cmap_resize = cmap_resize;
    data.pspage = pspage;
    data.num_areas = num_areas;
    data.areas = areas;
    data.honoured_areas = FALSE;
    c_call_callbacks (canvas->refresh_list, &data);
    if (data.honoured_areas && (honoured_areas != NULL) )
    {
	*honoured_areas = TRUE;
    }
    if (canvas->dressing_drawn) return;
    if (canvas->display_dressing) canvas_draw_dressing (canvas);
}   /*  End Function refresh_canvas  */

static unsigned long get_pixel_from_value (KWorldCanvas canvas,double value[2],
					   unsigned short *red,
					   unsigned short *green,
					   unsigned short *blue)
/*  [SUMMARY] Get a pixel value from a data value.
    <canvas> The world canvas.
    <value> The data value.
    <red> The red intensity in the colourmap for the pixel will be
    written here. If this is NULL, nothing is written here.
    <green> The green intensity in the colourmap for the pixel will be
    written here. If this is NULL, nothing is written here.
    <blue> The blue intensity in the colourmap for the pixel will be
    written here. If this is NULL, nothing is written here.
    [RETURNS] The pixel value.
*/
{
    unsigned int index;
    unsigned long pixel_value;
    struct win_scale_type *win_scale;
    static char function_name[] = "get_pixel_from_value";

    VERIFY_CANVAS (canvas);
    win_scale = &canvas->win_scale;
    if ( (value[0] >= TOOBIG) || (value[1] >= TOOBIG) )
    {
	return (win_scale->blank_pixel);
    }
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
	fprintf (stderr, "Not finished continuous phase\n");
	return (FALSE);
	/*break;*/
      default:
	fprintf (stderr, "Illegal value of conversion: %u\n",
		 win_scale->conv_type);
	a_prog_bug (function_name);
	break;
    }
    if (value[0] >= TOOBIG) return (win_scale->blank_pixel);
    /*  Take real value and convert to pixel value  */
    if (win_scale->iscale_func != NULL)
    {
	if ( !(*win_scale->iscale_func) (value, 1, value, 1, 1,
					 win_scale->z_min, win_scale->z_max,
					 win_scale->iscale_info) )
	{
	    fprintf (stderr, "Error scaling data\n");
	    a_prog_bug (function_name);
	}
    }
    if (value[0] < win_scale->z_min) pixel_value = win_scale->min_sat_pixel;
    else if (value[0] >win_scale->z_max) pixel_value =win_scale->max_sat_pixel;
    else
    {
	index = ( (value[0] - win_scale->z_min)
		  * (canvas->cmap_num_pixels - 1) /
		  (win_scale->z_max - win_scale->z_min) + 0.5 );
	pixel_value = canvas->cmap_pixels[index];
	if (red != NULL) *red = canvas->cmap_rgb_values[index * 3];
	if (green != NULL) *green = canvas->cmap_rgb_values[index * 3 + 1];
	if (blue != NULL) *blue = canvas->cmap_rgb_values[index * 3 + 2];
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
    KWorldCanvas canvas;
    double dx, dy;
    struct position_struct data;
    static char function_name[] = "pixcanvas_position_event";

    canvas = (KWorldCanvas) *f_info;
    VERIFY_CANVAS (canvas);
    /*  Convert pixel co-ordinates to world co-ordinates  */
    dx = x;
    dy = y;
    canvas_convert_to_canvas_coords (canvas, TRUE, 1, &dx, &dy,
				     &data.x_lin, &data.y_lin,
				     &data.x, &data.y);
    /*  Call event consumer functions  */
    data.event_code = event_code;
    data.e_info = event_info;
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
    flag honoured_areas;
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
    honoured_areas = FALSE;
    (*func) (canvas, data->width, data->height, &win_scale, canvas->cmap,
	     data->cmap_resize, client1_data, data->pspage,
	     data->num_areas, data->areas, &honoured_areas);
    if (honoured_areas) data->honoured_areas = TRUE;
    else
    {
	data->num_areas = 0;
	data->areas = NULL;
	data->honoured_areas = FALSE;
    }
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
	fprintf (stderr,"Bad magic number for  win_scale  structure\n");
	fprintf (stderr, "This will break in Karma version 2.0\n");
	fprintf (stderr,
			"Make sure a call is made to canvas_init_win_scale\n");
	fprintf (stderr, "and then recompile the application\n");
/*      Coming in Version 2.0 of Karma
	a_prog_bug (function_name);
*/
    }
    if (win_scale->right_x == win_scale->left_x)
    {
	fprintf (stderr, "right_x: %e must not equal left_x: %e\n",
			win_scale->right_x, win_scale->left_x);
	a_prog_bug (function_name);
    }
    if (win_scale->top_y == win_scale->bottom_y)
    {
	fprintf (stderr, "top_y: %e must not equal bottom_y: %e\n",
			win_scale->top_y, win_scale->bottom_y);
	a_prog_bug (function_name);
    }
    if (win_scale->z_scale != K_INTENSITY_SCALE_LINEAR)
    {
	fprintf (stderr, "z_scale: %u never was supported.\n",
			win_scale->z_scale);
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
    static char function_name[] = "__canvas_negotiate_scale_change";

    win_scale = &canvas->win_scale;
    *boundary_clear = FALSE;
    if (canvas->size_control_func == NULL)
    {
	/*  Must automatically produce some window scaling information  */
	get_dressing_size (canvas, &p_left, &p_right, &p_top, &p_bottom);
	if (p_left + p_right >= width)
	{
	    fprintf (stderr,
			    "Dressing (%d pixels) too wide for pixel canvas (%d pixels)\n",
			    p_left + p_right, width);
	    return (FALSE);
	}
	if (p_top + p_bottom >= height)
	{
	    fprintf (stderr,
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
	(*canvas->size_control_func) (canvas,
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
	    fprintf (stderr,
		     "Dressing+active (%d + %d = %d) too wide for pixel canvas (%d pixels)\n",
		     p_left + p_right, win_scale->x_pixels,
		     p_left + p_right + win_scale->x_pixels,
		     width);
	    return (FALSE);
	}
	if (p_top + p_bottom + win_scale->y_pixels > height)
	{
	    fprintf (stderr,
		     "Dressing+active (%d + %d = %d) too high for pixel canvas (%d pixels)\n",
		     p_top + p_bottom, win_scale->y_pixels,
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
    [RETURNS] Nothing.
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
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "__canvas_dressing_refresh_func";

    VERIFY_CANVAS (canvas);
    canvas_draw_dressing (canvas);
}   /*  End Function dressing_refresh_func  */

static void set_sat_pixels (KWorldCanvas canvas)
/*  [PURPOSE] This routine will set the saturation pixels as appropriate.
    <canvas> The world canvas.
    [RETURNS] Nothing.
*/
{
    unsigned int num_pixels;
    unsigned long *pixel_values;

    if (canvas->cmap == NULL) return;
    if (canvas->auto_min_sat || canvas->auto_max_sat)
    {
	num_pixels = kcmap_get_pixels (canvas->cmap, &pixel_values);
	if (canvas->auto_min_sat)
	{
	    canvas->win_scale.min_sat_pixel = pixel_values[0];
	}
	if (canvas->auto_max_sat)
	{
	    if (num_pixels < 1) ++num_pixels;
	    canvas->win_scale.max_sat_pixel = pixel_values[num_pixels - 1];
	}
    }
}   /*  End Function set_sat_pixels  */

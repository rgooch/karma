/*LINTLIBRARY*/
/*PREFIX:"canvas_"*/
/*   canvas.c

    This code provides KWorldCanvas objects.

    Copyright (C) 1992,1993  Richard Gooch

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


    Written by      Richard Gooch   16-APR-1992

    Updated by      Richard Gooch   27-APR-1993

    Updated by      Richard Gooch   3-JUN-1993: Improved symmetry in
  co-ordinate conversion routines.

    Updated by      Richard Gooch   15-OCT-1993: Documented placement of
  KWorldCanvas origin.

    Updated by      Richard Gooch   21-NOV-1993: Added  canvas_draw_point_p
  ,  canvas_draw_line_p  ,  canvas_fill_ellipse_p  ,  canvas_draw_rectangle
  ,  canvas_draw_rectangle_p  ,  canvas_fill_rectangle  and
  canvas_fill_rectangle_p  routines.

    Last updated by Richard Gooch   23-NOV-1993: Added
  canvas_register_convert_func  .


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_a.h>

#define KWIN_GENERIC_ONLY

typedef struct worldcanvas_type * KWorldCanvas;

#define KWORLDCANVAS_DEFINED
#include <karma_canvas.h>

#define MAGIC_NUMBER (unsigned int) 357474556

#define VERIFY_CANVAS(canvas) if (canvas == NULL) \
{(void) fprintf (stderr, "NULL canvas passed\n"); \
 a_prog_bug (function_name); } \
if ( (*canvas).magic_number != MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid canvas object\n"); \
 a_prog_bug (function_name); }

struct worldcanvas_type
{
    unsigned int magic_number;
    KPixCanvas pixcanvas;
    Kcolourmap cmap;
    struct win_scale_type win_scale;
    void (*size_control_func) ();
    void *size_control_info;
    flag (*coord_convert_func) ();
    void *coord_convert_info;
    struct refresh_func *first_refresh_func;
    struct refresh_func *last_refresh_func;
    struct position_func *first_position_func;
    struct position_func *last_position_func;
};

struct refresh_func
{
    void (*func) ();
    void *info;
    struct refresh_func *next;
};

struct position_func
{
    flag (*func) ();
    void *info;
    struct position_func *next;
};


/*  Private data  */
static flag quash_size_control_call = FALSE;


/*  Local functions  */
static void pixcanvas_refresh_func (/* pixcanvas, width, height, info */);
static void cmap_resize_func (/* cmap, info */);
static unsigned long get_pixel_from_value (/* value, win_scale, cmap */);
static flag pixcanvas_position_event (/* pixcanvas, x, y, event_code,
					 e_info, f_info */);


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KWorldCanvas canvas_create (pixcanvas, cmap, win_scale)
/*  This routine will create a world canvas, ready for drawing, from a
    KPixCanvas (pixel canvas) object.
    Note that the origin of a KWorldCanvas is the lower-left corner.
    The pixel canvas must be given by  pixcanvas  .
    The colourmap must be given by  cmap  .
    The window scaling information must be pointed to by  win_scale  .The
        x_offset  x_pixels  y_offset  y_pixels  fields are determined
    internally.
    The routine returns a world canvas on success, else it returns NULL.
*/
KPixCanvas pixcanvas;
Kcolourmap cmap;
struct win_scale_type *win_scale;
{
    KWorldCanvas canvas;
    int width, height;
    static char function_name[] = "canvas_create";

    if (pixcanvas == NULL)
    {
	(void) fprintf (stderr, "NULL KPixCanvas passed\n");
	a_prog_bug (function_name);
    }
    if (cmap == NULL)
    {
	(void) fprintf (stderr, "NULL Kcolourmap passed\n");
	a_prog_bug (function_name);
    }
    if (win_scale == NULL)
    {
	(void) fprintf (stderr,
			"NULL window scaling structure pointer passed\n");
	a_prog_bug (function_name);
    }
    kwin_get_size (pixcanvas, &width, &height);
    if ( ( canvas = (KWorldCanvas) m_alloc (sizeof *canvas) ) == NULL )
    {
	m_error_notify (function_name, "world canvas");
	return (NULL);
    }
    (*canvas).magic_number = MAGIC_NUMBER;
    (*canvas).pixcanvas = pixcanvas;
    (*canvas).cmap = cmap;
    m_copy ( (char *) &(*canvas).win_scale, (char *) win_scale,
	    sizeof *win_scale );
    (*canvas).win_scale.x_offset = 0;
    (*canvas).win_scale.x_pixels = width;
    (*canvas).win_scale.y_offset = 0;
    (*canvas).win_scale.y_pixels = height;
    (*canvas).size_control_func = NULL;
    (*canvas).size_control_info = NULL;
    (*canvas).coord_convert_func = NULL;
    (*canvas).coord_convert_info = NULL;
    (*canvas).first_refresh_func = NULL;
    (*canvas).last_refresh_func = NULL;
    (*canvas).first_position_func = NULL;
    (*canvas).last_position_func = NULL;
    /*  Process refreshes from lower down  */
    kwin_register_refresh_func (pixcanvas, pixcanvas_refresh_func,
				(void *) canvas);
    /*  Process position events from lower down  */
    kwin_register_position_event_func (pixcanvas, pixcanvas_position_event,
				       (void *) canvas);
    /*  Process colourmap resizes  */
    kcmap_register_resize_func (cmap, cmap_resize_func, (void *) canvas);
    return (canvas);
}   /*  End Function canvas_create  */

/*PUBLIC_FUNCTION*/
void canvas_register_refresh_func (canvas, refresh_func, info)
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
    The routine returns nothing.
*/
KWorldCanvas canvas;
void (*refresh_func) ();
void *info;
{
    struct refresh_func *new_func;
    static char function_name[] = "canvas_register_refresh_func";

    VERIFY_CANVAS (canvas);
    if (refresh_func == NULL) return;
    if ( ( new_func = (struct refresh_func *) m_alloc (sizeof *new_func) )
	== NULL )
    {
	m_abort (function_name, "resize function entry");
    }
    (*new_func).func = refresh_func;
    (*new_func).info = info;
    (*new_func).next = NULL;
    if ( (*canvas).first_refresh_func == NULL )
    {
	/*  Create list of refresh functions  */
	(*canvas).first_refresh_func = new_func;
    }
    else
    {
	/*  Append to list of refresh functions  */
	(* (*canvas).last_refresh_func ).next = new_func;
    }
    (*canvas).last_refresh_func = new_func;
}   /*  End Function canvas_register_refresh_func  */

/*PUBLIC_FUNCTION*/
void canvas_register_size_control_func (canvas, size_control_func, info)
/*  This routine will register the size control function for a world canvas.
    This function will be called whenever the lower level pixel canvas resizes
    OR a call is made to  canvas_resize  .
    This function may alter the window scaling information for the canvas.
    Only one size control function is permitted per canvas. The size control
    routine is called before any refresh functions are called. This is
    a means for a higher level object to take control of the world canvas.
    The canvas must be given by  canvas  .
    The function that is called when the canvas is to be refreshed must be
    pointed to by  size_control_func  .
    The interface to this routine is as follows:

    void size_control_func (canvas, width, height, win_scale, info)
    *   This routine will modify the window scaling information for a world
        canvas.
        The canvas is given by  canvas  .
	The width of the canvas in pixels is given by  width  .
	The height of the canvas in pixels is given by  height  .
	The window scaling information is pointed to by  win_scale  .The data
	herein may be modified.
	The arbitrary canvas information pointer is pointed to by  info  .
	The routine should return nothing.
    *
    KWorldCanvas canvas;
    int width;
    int height;
    struct win_scale_type *win_scale;
    void **info;

    The initial arbitrary canvas information pointer must be given by  info  .
    The routine returns nothing.
*/
KWorldCanvas canvas;
void (*size_control_func) ();
void *info;
{
    static char function_name[] = "canvas_register_size_control_func";

    VERIFY_CANVAS (canvas);
    if (size_control_func == NULL) return;
    if ( (*canvas).size_control_func != NULL )
    {
	(void) fprintf (stderr, "size_control_func already registered\n");
	a_prog_bug (function_name);
    }
    (*canvas).size_control_func = size_control_func;
    (*canvas).size_control_info = info;
}   /*  End Function canvas_register_size_control_func  */

/*PUBLIC_FUNCTION*/
void canvas_register_position_event_func (canvas, position_func, f_info)
/*  This routine will register a position event function for a world canvas.
    The position event function will be called whenever a position event on the
    canvas has not been consumed. Many position event functions may be
    registered per canvas. The first function registered is the first function
    called upon a position event.
    The canvas must be given by  canvas  .
    The function that is called when a position event occurs must be pointed to
    by  position_func  .
    The interface to this routine is as follows:

    flag position_func (canvas, x, y, event_code, e_info, f_info)
    *   This routine is a position event consumer for a world canvas.
        The canvas is given by  canvas  .
	The horizontal world co-ordinate of the event will be given by  x  .
	The vertical world co-ordinate of the event will be given by  y  .
	The arbitrary event code is given by  event_code  .
	The arbitrary event information is pointed to by  e_info  .
	The arbitrary function information pointer is pointed to by  f_info  .
	The routine returns TRUE if the event was consumed, else it returns
	FALSE indicating that the event is still to be processed.
    *
    KWorldCanvas canvas;
    double x;
    double y;
    unsigned int event_code;
    void *e_info;
    void **f_info;

    The initial arbitrary function information pointer must be given by  f_info
    The routine returns nothing.
*/
KWorldCanvas canvas;
flag (*position_func) ();
void *f_info;
{
    struct position_func *new_func;
    static char function_name[] = "canvas_register_position_func";

    VERIFY_CANVAS (canvas);
    if (position_func == NULL) return;
    if ( ( new_func = (struct position_func *) m_alloc (sizeof *new_func) )
	== NULL )
    {
	m_abort (function_name, "position event function entry");
    }
    (*new_func).func = position_func;
    (*new_func).info = f_info;
    (*new_func).next = NULL;
    if ( (*canvas).first_position_func == NULL )
    {
	/*  Create list of position functions  */
	(*canvas).first_position_func = new_func;
    }
    else
    {
	/*  Append to list of position functions  */
	(* (*canvas).last_position_func ).next = new_func;
    }
    (*canvas).last_position_func = new_func;
}   /*  End Function canvas_register_position_func  */

/*PUBLIC_FUNCTION*/
flag canvas_resize (canvas, win_scale, always_clear)
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
KWorldCanvas canvas;
struct win_scale_type *win_scale;
flag always_clear;
{
    KPixCanvas pixcanvas;
    flag clear = TRUE;
    int x_offset, x_pixels, y_offset, y_pixels;
    int width, height;
    extern flag quash_size_control_call;
    static char function_name[] = "canvas_resize";

    VERIFY_CANVAS (canvas);
    FLAG_VERIFY (always_clear);
    /*  Get pixel canvas size  */
    kwin_get_size ( (*canvas).pixcanvas, &width, &height );
    if ( (win_scale == NULL) && (always_clear != TRUE) &&
	( (*canvas).size_control_func != NULL ) )
    {
	x_offset = (*canvas).win_scale.x_offset;
	x_pixels = (*canvas).win_scale.x_pixels;
	y_offset = (*canvas).win_scale.y_offset;
	y_pixels = (*canvas).win_scale.y_pixels;
	/*  Call size control function first  */
	(* (*canvas).size_control_func ) (canvas, width, height,
					  &(*canvas).win_scale,
					  &(*canvas).size_control_info);
	if ( (x_offset == (*canvas).win_scale.x_offset) &&
	    (x_pixels == (*canvas).win_scale.x_pixels) &&
	    (y_offset == (*canvas).win_scale.y_offset) &&
	    (y_pixels == (*canvas).win_scale.y_pixels) )
	{
	    clear = FALSE;
	}
	quash_size_control_call = TRUE;
    }
    if (win_scale != NULL)
    {
	m_copy ( (char *) &(*canvas).win_scale, (char *) win_scale,
		sizeof *win_scale );
    }
    return ( kwin_resize ( (*canvas).pixcanvas, clear, 0, 0, 0, 0 ) );
}   /*  End Function canvas_resize  */

/*PUBLIC_FUNCTION*/
void canvas_get_size (canvas, width, height, win_scale)
/*  This routine will get the size of a world canvas.
    The number of horizontal pixels will be written to the storage pointed to
    by  width  .
    The number of vertical pixels will be written to the storage pointed to by
    height  .
    The window scaling information will be written to the storage pointed to by
    win_scale  .
    The routine returns nothing.
*/
KWorldCanvas canvas;
int *width;
int *height;
struct win_scale_type *win_scale;
{
    static char function_name[] = "canvas_get_size";

    VERIFY_CANVAS (canvas);
    if (win_scale == NULL)
    {
	(void) fprintf (stderr, "NULL win_scale structure pointer passed\n");
	a_prog_bug (function_name);
    }
    kwin_get_size ( (*canvas).pixcanvas, width, height );
    m_copy ( (char *) win_scale, (char *) &(*canvas).win_scale,
	    sizeof *win_scale );
}   /*  End Function canvas_get_size  */

/*PUBLIC_FUNCTION*/
flag canvas_convert_to_canvas_coord (canvas, xin, yin, xout, yout)
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
KWorldCanvas canvas;
int xin;
int yin;
double *xout;
double *yout;
{
    flag converted = FALSE;
    int px, py;
    struct win_scale_type win_scale;
    static char function_name[] = "canvas_convert_to_canvas_coord";

    VERIFY_CANVAS (canvas);
    if ( (*canvas).coord_convert_func != NULL )
    {
	m_copy ( (char *) &win_scale, (char *) &(*canvas).win_scale,
		sizeof win_scale );
	px = xin;
	py = yin;
	converted = ( (* (*canvas).coord_convert_func )
		     (canvas, &win_scale, &px, &py, xout, yout, TRUE,
		      &(*canvas).coord_convert_info) );
    }
    if (!converted)
    {
	/*  Convert pixel co-ordinates to world co-ordinates  */
	*xout = xin - (*canvas).win_scale.x_offset;
	*xout /= (double) ( (*canvas).win_scale.x_pixels - 1 );
	*xout *= ( (*canvas).win_scale.x_max - (*canvas).win_scale.x_min );
	*xout += (*canvas).win_scale.x_min;
	/*  Flip vertical  */
	yin -= (*canvas).win_scale.y_offset;
	yin = (*canvas).win_scale.y_pixels - yin - 1;
	/*  Convert y  */
	*yout = yin;
	*yout /= (double) ( (*canvas).win_scale.y_pixels - 1 );
	*yout *= ( (*canvas).win_scale.y_max - (*canvas).win_scale.y_min );
	*yout += (*canvas).win_scale.y_min;
    }
    if ( (xin < (*canvas).win_scale.x_offset) ||
	(xin >= (*canvas).win_scale.x_offset + (*canvas).win_scale.x_pixels) ||
	(yin < (*canvas).win_scale.y_offset) ||
	(yin >= (*canvas).win_scale.y_offset + (*canvas).win_scale.y_pixels) )
    return (FALSE);
    return (TRUE);
}   /*  End Function canvas_convert_to_canvas_coord  */

/*PUBLIC_FUNCTION*/
flag canvas_convert_from_canvas_coord (canvas, xin, yin, xout, yout)
/*  This routine will convert co-ordinates in a world canvas to co-ordinates in
    a pixel canvas.
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
KWorldCanvas canvas;
double xin;
double yin;
int *xout;
int *yout;
{
    flag converted = FALSE;
    struct win_scale_type win_scale;
    double px, py;
    double wx, wy;
    int iy;
    static char function_name[] = "canvas_convert_from_canvas_coord";

    VERIFY_CANVAS (canvas);
    if ( (*canvas).coord_convert_func != NULL )
    {
	m_copy ( (char *) &win_scale, (char *) &(*canvas).win_scale,
		sizeof win_scale );
	wx = xin;
	wy = yin;
	converted = ( (* (*canvas).coord_convert_func )
		     (canvas, &win_scale, xout, yout, &wx, &wy, FALSE,
		      &(*canvas).coord_convert_info) );
    }
    if (!converted)
    {
	px = xin - (*canvas).win_scale.x_min;
	px *= (double) ( (*canvas).win_scale.x_pixels - 1 );
	px /= (*canvas).win_scale.x_max - (*canvas).win_scale.x_min;
	px += 0.01;
	py = ( (yin - (*canvas).win_scale.y_min) /
	      ( (*canvas).win_scale.y_max - (*canvas).win_scale.y_min ) *
	      (double) ( (*canvas).win_scale.y_pixels - 1 ) );
	iy = (int) (py + 0.01);
	/*  Convert world co-ordinates to pixel co-ordinates  */
	*xout = (int) px + (*canvas).win_scale.x_offset;
	/*  Flip vertical  */
	*yout = (*canvas).win_scale.y_offset+(*canvas).win_scale.y_pixels-1-iy;
    }
    if ( (*xout < (*canvas).win_scale.x_offset) ||
	(*xout >= (*canvas).win_scale.x_offset+(*canvas).win_scale.x_pixels) ||
	(*yout < (*canvas).win_scale.y_offset) ||
	(*yout >= (*canvas).win_scale.y_offset +(*canvas).win_scale.y_pixels) )
    return (FALSE);
    return (TRUE);
}   /*  End Function canvas_convert_from_canvas_coord  */

/*PUBLIC_FUNCTION*/
void canvas_register_convert_func (canvas, coord_convert_func, info)
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
KWorldCanvas canvas;
flag (*coord_convert_func) ();
void *info;
{
    static char function_name[] = "canvas_register_convert_func";

    VERIFY_CANVAS (canvas);
    if (coord_convert_func == NULL) return;
    if ( (*canvas).coord_convert_func != NULL )
    {
	(void) fprintf (stderr, "coord_convert_func already registered\n");
	a_prog_bug (function_name);
    }
    (*canvas).coord_convert_func = coord_convert_func;
    (*canvas).coord_convert_info = info;
}   /*  End Function canvas_register_convert_func  */


/*  Drawing routines follow  */

/*PUBLIC_FUNCTION*/
flag canvas_draw_image (canvas, arr_desc, slice, hdim, vdim, elem_index,
			cache_ptr)
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
    The colourmap to use when translating the data into pixel values must be
    given by  cmap  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KWorldCanvas canvas;
array_desc *arr_desc;
char *slice;
unsigned int hdim;
unsigned int vdim;
unsigned int elem_index;
KPixCanvasImageCache *cache_ptr;
{
    unsigned int num_pixels;
    unsigned long *pixel_values;
    static char function_name[] = "canvas_draw_image";

    VERIFY_CANVAS (canvas);
    num_pixels = kcmap_get_pixels ( (*canvas).cmap, &pixel_values );
    return ( kwin_draw_image ( (*canvas).pixcanvas, arr_desc, slice,
			      hdim, vdim, elem_index,
			      num_pixels, pixel_values,
			      &(*canvas).win_scale, cache_ptr) );
}   /*  End Function canvas_draw_image  */

/*PUBLIC_FUNCTION*/
void canvas_draw_point (canvas, x, y, value)
/*  This routine will draw a single point onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal world co-ordinate of the point must be given by  x  .
    The vertical world co-ordinate of the point must be given by  y  .
    The data value to use must be given by  value  .This must be of type
    K_DCOMPLEX.
    The routine returns nothing.
*/
KWorldCanvas canvas;
double x;
double y;
double value[2];
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_point";

    VERIFY_CANVAS (canvas);
    pixel_value = get_pixel_from_value (value, &(*canvas).win_scale,
					(*canvas).cmap);
    canvas_draw_point_p (canvas, x, y, pixel_value);
}   /*  End Function canvas_draw_point  */

/*PUBLIC_FUNCTION*/
void canvas_draw_point_p (canvas, x, y, pixel_value)
/*  This routine will draw a single point onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal world co-ordinate of the point must be given by  x  .
    The vertical world co-ordinate of the point must be given by  y  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KWorldCanvas canvas;
double x;
double y;
unsigned long pixel_value;
{
    int px, py;
    static char function_name[] = "canvas_draw_point_p";

    VERIFY_CANVAS (canvas);
    if ( (x < (*canvas).win_scale.x_min) || (x > (*canvas).win_scale.x_max) ||
	(y < (*canvas).win_scale.y_min) || (y > (*canvas).win_scale.y_max) )
    {
	return;
    }
    (void) canvas_convert_from_canvas_coord (canvas, x, y, &px, &py);
    kwin_draw_point ( (*canvas).pixcanvas, px, py, pixel_value );
}   /*  End Function canvas_draw_point_p  */

/*PUBLIC_FUNCTION*/
void canvas_draw_line (canvas, x0, y0, x1, y1, value)
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
KWorldCanvas canvas;
double x0;
double y0;
double x1;
double y1;
double value[2];
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_line";

    VERIFY_CANVAS (canvas);
    pixel_value = get_pixel_from_value (value, &(*canvas).win_scale,
					(*canvas).cmap);
    canvas_draw_line_p (canvas, x0, y0, x1, y1, pixel_value);
}   /*  End Function canvas_draw_line  */

/*PUBLIC_FUNCTION*/
void canvas_draw_line_p (canvas, x0, y0, x1, y1, pixel_value)
/*  This routine will draw a single line onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal world co-ordinate of the first point must be given by  x0  .
    The vertical world co-ordinate of the first point must be given by  y0  .
    The horizontal world co-ordinate of the second point must be given by  x0
    The vertical world co-ordinate of the second point must be given by  y0  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KWorldCanvas canvas;
double x0;
double y0;
double x1;
double y1;
unsigned long pixel_value;
{
    int px0, py0;
    int px1, py1;
    static char function_name[] = "canvas_draw_line_p";

    VERIFY_CANVAS (canvas);
    (void) canvas_convert_from_canvas_coord (canvas, x0, y0, &px0, &py0);
    (void) canvas_convert_from_canvas_coord (canvas, x1, y1, &px1, &py1);
    kwin_draw_line ( (*canvas).pixcanvas,
		    px0, py0, px1, py1, pixel_value );
}   /*  End Function canvas_draw_line_p  */

/*PUBLIC_FUNCTION*/
void canvas_fill_ellipse (canvas, centre_x, centre_y, radius_x, radius_y,value)
/*  This routine will draw a filled ellipse onto a world canvas.
    The canvas must be given by  canvas  .
    The world co-ordinates of the centre of the ellipse must be given by
    centre_x  and centre_y  .
    The radii must be given by  radius_x  and  radius_y  .
    The complex value to fill the ellipse with must be pointed to be  value  .
    This must be of type K_DCOMPLEX.
    The routine returns nothing.
*/
KWorldCanvas canvas;
double centre_x;
double centre_y;
double radius_x;
double radius_y;
double value[2];
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_fill_ellipse";

    VERIFY_CANVAS (canvas);
    pixel_value = get_pixel_from_value (value, &(*canvas).win_scale,
					(*canvas).cmap);
    canvas_fill_ellipse_p (canvas,
			   centre_x, centre_y, radius_x, radius_y,
			   pixel_value);
}   /*  End Function canvas_fill_ellipse  */

/*PUBLIC_FUNCTION*/
void canvas_fill_ellipse_p (canvas, centre_x, centre_y, radius_x, radius_y,
			    pixel_value)
/*  This routine will draw a filled ellipse onto a world canvas.
    The canvas must be given by  canvas  .
    The world co-ordinates of the centre of the ellipse must be given by
    centre_x  and centre_y  .
    The radii must be given by  radius_x  and  radius_y  .
    The pixel value to fill the ellipse with must be given by  pixel_value  .
    The routine returns nothing.
*/
KWorldCanvas canvas;
double centre_x;
double centre_y;
double radius_x;
double radius_y;
unsigned long pixel_value;
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
    kwin_fill_ellipse ( (*canvas).pixcanvas, cx, cy, rx, ry, pixel_value );
}   /*  End Function canvas_fill_ellipse_p  */

/*PUBLIC_FUNCTION*/
flag canvas_fill_polygon (canvas, coords, num_vertices, value, convex)
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
KWorldCanvas canvas;
edit_coord *coords;
unsigned int num_vertices;
double value[2];
flag convex;
{
    unsigned int coord_count;
    unsigned long pixel_value;
    static unsigned int num_points_allocated = 0;
    static int *point_x = NULL;
    static int *point_y = NULL;
    static char function_name[] = "canvas_fill_polygon";

    VERIFY_CANVAS (canvas);
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
    pixel_value = get_pixel_from_value (value, &(*canvas).win_scale,
					(*canvas).cmap);
    return ( kwin_fill_polygon ( (*canvas).pixcanvas,
				point_x, point_y, num_vertices,
				pixel_value, convex ) );
}   /*  End Function canvas_fill_polygon  */

/*PUBLIC_FUNCTION*/
void canvas_draw_rectangle (canvas, x, y, width, height, value)
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
KWorldCanvas canvas;
double x;
double y;
double width;
double height;
double value[2];
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_draw_rectangle";

    VERIFY_CANVAS (canvas);
    pixel_value = get_pixel_from_value (value, &(*canvas).win_scale,
					(*canvas).cmap);
    canvas_draw_rectangle_p (canvas, x, y, width, height, pixel_value);
}   /*  End Function canvas_draw_rectangle  */

/*PUBLIC_FUNCTION*/
void canvas_draw_rectangle_p (canvas, x, y, width, height, pixel_value)
/*  This routine will draw a single rectangle onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the rectangle must be given by  x  .
    The vertical offset of the rectangle must be given by  y  .
    The width of the rectangle must be given by  width  .
    The height of the rectangle must be given by  height  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KWorldCanvas canvas;
double x;
double y;
double width;
double height;
unsigned long pixel_value;
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
    kwin_draw_rectangle ( (*canvas).pixcanvas,
			 px, py1, pwidth, pheight, pixel_value );
}   /*  End Function canvas_draw_rectangle_p  */

/*PUBLIC_FUNCTION*/
void canvas_fill_rectangle (canvas, x, y, width, height, value)
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
KWorldCanvas canvas;
double x;
double y;
double width;
double height;
double value[2];
{
    unsigned long pixel_value;
    static char function_name[] = "canvas_fill_rectangle";

    VERIFY_CANVAS (canvas);
    pixel_value = get_pixel_from_value (value, &(*canvas).win_scale,
					(*canvas).cmap);
    canvas_fill_rectangle_p (canvas, x, y, width, height, pixel_value);
}   /*  End Function canvas_fill_rectangle  */

/*PUBLIC_FUNCTION*/
void canvas_fill_rectangle_p (canvas, x, y, width, height, pixel_value)
/*  This routine will fill a single rectangle onto a world canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the rectangle must be given by  x  .
    The vertical offset of the rectangle must be given by  y  .
    The width of the rectangle must be given by  width  .
    The height of the rectangle must be given by  height  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KWorldCanvas canvas;
double x;
double y;
double width;
double height;
unsigned long pixel_value;
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
    kwin_fill_rectangle ( (*canvas).pixcanvas,
			 px, py1, pwidth, pheight, pixel_value );
}   /*  End Function canvas_fill_rectangle_p  */


/*  Private functions follow  */
static void pixcanvas_refresh_func (pixcanvas, width, height, info)
/*  This routine is a refresh event consumer for a pixel canvas.
    The canvas is given by  pixcanvas  .
    The width of the canvas is given by  width  .
    The height of the canvas is given by  height  .
    The arbitrary canvas information pointer is pointed to by  info  .
    The routine returns nothing.
*/
KPixCanvas pixcanvas;
int width;
int height;
void **info;
{
    KWorldCanvas canvas;
    struct win_scale_type win_scale;
    struct refresh_func *curr_func;
    extern flag quash_size_control_call;
    static char function_name[] = "pixcanvas_refresh_func";

    canvas = (KWorldCanvas) *info;
    VERIFY_CANVAS (canvas);
    if ( (*canvas).size_control_func == NULL )
    {
	/*  Must automatically produce some window scaling information  */
	(*canvas).win_scale.x_offset = 0;
	(*canvas).win_scale.x_pixels = width;
	(*canvas).win_scale.y_offset = 0;
	(*canvas).win_scale.y_pixels = height;
    }
    else
    {
	/*  Call size control function first  */
	if (quash_size_control_call)
	{
	    quash_size_control_call = FALSE;
	}
	else
	{
	    (* (*canvas).size_control_func) (canvas, width, height,
					     &(*canvas).win_scale,
					     &(*canvas).size_control_info);
	}
    }
    /*  Make a temporary copy of win_scale  */
    m_copy ( (char *) &win_scale, (char *) &(*canvas).win_scale,
	    sizeof win_scale );
    /*  Call refresh functions  */
    for (curr_func = (*canvas).first_refresh_func; curr_func != NULL;
	 curr_func = (*curr_func).next)
    {
	(* (*curr_func).func ) (canvas, width, height,
				&win_scale, (*canvas).cmap, FALSE,
				&(*curr_func).info);
    }
}   /*  End Function pixcanvas_refresh_func  */

static void cmap_resize_func (cmap, info)
/*  This routine registers a change in the size of a colourmap.
    The colourmap must be given by  cmap  .
    The arbitrary colourmap information pointer is pointed to by  info  .
    The routine returns nothing.
*/
Kcolourmap cmap;
void **info;
{
    KWorldCanvas canvas;
    int width, height;
    struct win_scale_type win_scale;
    struct refresh_func *curr_func;
    static char function_name[] = "cmap_resize_func";

    canvas = (KWorldCanvas) *info;
    VERIFY_CANVAS (canvas);
    /*  Get pixel canvas size  */
    kwin_get_size ( (*canvas).pixcanvas, &width, &height );
    /*  Make a temporary copy of win_scale  */
    m_copy ( (char *) &win_scale, (char *) &(*canvas).win_scale,
	    sizeof win_scale );
    /*  Call refresh functions  */
    for (curr_func = (*canvas).first_refresh_func; curr_func != NULL;
	 curr_func = (*curr_func).next)
    {
	(* (*curr_func).func ) (canvas, width, height,
				&win_scale, (*canvas).cmap, TRUE,
				&(*curr_func).info);
    }
}   /*  End Function cmap_resize_func  */

static unsigned long get_pixel_from_value (value, win_scale, cmap)
/*  This routine will get a pixel value from a data value.
    The data value must be given by  value  .
    The window scaling information must be pointed to by  win_scale  .
    The colourmap must be given by  cmap  .
    The routine returns the pixel value.
*/
double value[2];
struct win_scale_type *win_scale;
Kcolourmap cmap;
{
    unsigned int num_pixels;
    unsigned long pixel_value;
    unsigned long *pixel_values;
    static char function_name[] = "get_pixel_from_value";

    num_pixels = kcmap_get_pixels (cmap, &pixel_values);
    switch ( (*win_scale).conv_type )
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
	break;
      default:
	(void) fprintf (stderr, "Illegal value of conversion: %u\n",
			(*win_scale).conv_type);
	a_prog_bug (function_name);
	break;
    }
    /*  Take real value and convert to pixel value  */
    switch ( (*win_scale).z_scale )
    {
      case K_INTENSITY_SCALE_LINEAR:
	if (value[0] < (*win_scale).z_min)
	{
	    pixel_value = (*win_scale).min_sat_pixel;
	}
	else if (value[0] > (*win_scale).z_max)
	{
	    pixel_value = (*win_scale).max_sat_pixel;
	}
	else
	{
	    pixel_value = pixel_values[(unsigned int)
				       ( (value[0] - (*win_scale).z_min)
					* (num_pixels - 1) /
					( (*win_scale).z_max -
					 (*win_scale).z_min ) + 0.5 )];
	}
	break;
      case K_INTENSITY_SCALE_LOGARITHMIC:
	if (value[0] < (*win_scale).z_min)
	{
	    pixel_value = (*win_scale).min_sat_pixel;
	}
	else if (value[0] > (*win_scale).z_max)
	{
	    pixel_value = (*win_scale).max_sat_pixel;
	}
	else
	{
	    pixel_value = pixel_values[(unsigned int)
				       ( log10 (value[0] / (*win_scale).z_min)
					* (num_pixels - 1) /
					log10 ( (*win_scale).z_max /
					       (*win_scale).z_min ) )];
	}
	break;
      default:
	(void) fprintf (stderr, "Illegal value of intensity scale: %u\n",
			(*win_scale).z_scale);
	a_prog_bug (function_name);
	break;
    }
    return (pixel_value);
}   /*  End Function get_pixel_from_value  */

static flag pixcanvas_position_event (pixcanvas, x, y, event_code,
				      event_info, f_info)
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
KPixCanvas pixcanvas;
int x;
int y;
unsigned int event_code;
void *event_info;
void **f_info;
{
    double wx, wy;
    KWorldCanvas canvas;
    struct position_func *curr_func;
    static char function_name[] = "pixcanvas_position_event";

    canvas = (KWorldCanvas) *f_info;
    VERIFY_CANVAS (canvas);
    /*  Convert pixel co-ordinates to world co-ordinates  */
    (void) canvas_convert_to_canvas_coord (canvas, x, y, &wx, &wy);
    /*  Call event consumer functions  */
    for (curr_func = (*canvas).first_position_func; curr_func != NULL;
	 curr_func = (*curr_func).next)
    {
	if ( (* (*curr_func).func ) (canvas, wx, wy, event_code, event_info,
				     &(*curr_func).info) )
	{
	    /*  Consumed  */
	    return (TRUE);
	}
    }
    return (FALSE);
}   /*  End Function pixcanvas_position_event  */

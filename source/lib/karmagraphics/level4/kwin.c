/*LINTLIBRARY*/
/*PREFIX:"kwin_"*/
/*  kwin.c

    This code provides KPixCanvas objects.

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

/*  This file contains all routines needed for manipulating a simple pixel
    canvas (window) independent of the graphics system in use.


    Written by      Richard Gooch   15-APR-1992

    Last updated by Richard Gooch   27-APR-1993


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#ifdef X11
#include <karma_drw.h>
#include <karma_xi.h>
#endif
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>

typedef struct pixcanvas_type * KPixCanvas;
typedef struct cache_data_type * KPixCanvasImageCache;

#define KPIXCANVAS_DEFINED
#define KPIXCANVASIMAGECACHE_DEFINED
#include <karma_kwin.h>

#define CANVAS_MAGIC_NUMBER (unsigned int) 245724438
#define CACHE_DATA_MAGIC_NUMBER (unsigned int) 924723489

#define VERIFY_CANVAS(canvas) if (canvas == NULL) \
{(void) fprintf (stderr, "NULL canvas passed\n"); \
 a_prog_bug (function_name); } \
if ( (*canvas).magic_number != CANVAS_MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid canvas object\n"); \
 a_prog_bug (function_name); }


struct pixcanvas_type
{
    unsigned int magic_number;
    int xoff;
    int yoff;
    int width;
    int height;
    flag shm_available;
    struct refresh_func *first_refresh_func;
    struct refresh_func *last_refresh_func;
    struct position_func *first_position_func;
    struct position_func *last_position_func;
#ifdef X11
    Display *display;
    Window window;
    GC gc;
    XGCValues gcvalues;
#endif
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

struct cache_data_type
{
    unsigned int magic_number;
    KPixCanvas canvas;
#ifdef X11
    Display *display;
    XImage *ximage;
    flag shared;
    unsigned int width;
    unsigned int height;
#endif
};


/*  Local functions  */
static KPixCanvas alloc_canvas ();
static KPixCanvasImageCache alloc_cache_data_struct (/* canvas */);


/*  Public functions follow  */

/*  X specific functions follow  */
#ifdef X11

/*PUBLIC_FUNCTION*/
KPixCanvas kwin_create_x (display, window, gc, xoff, yoff, width, height)
/*  This routine will create a pixel canvas, ready for drawing, from an X
    window.
    NOTE: this routine is only available with X windows.
    The X display must be pointed to by  display  .
    The window ID of must be given by  window  .
    The graphics context must be given by  gc  .
    The horizontal offset of the canvas origin (upper-left corner) relative to
    the parent window must be given by  xoff  .
    The vertical offset of the canvas origin (upper-left corner) relative to
    the parent window must be given by  yoff  .
    The width (vertical extent) of the canvas must be given by  width  .
    The height (horizontal extent) of the canvas must be given by  height  .
    The routine returns a pixel canvas on success, else it returns NULL.
*/
Display *display;
Window window;
GC gc;
int xoff;
int yoff;
int width;
int height;
{
    KPixCanvas canvas;
    static char function_name[] = "kwin_create_x";

    if ( ( canvas = alloc_canvas () ) == NULL )
    {
	m_error_notify (function_name, "X pixel canvas");
	return (NULL);
    }
    (*canvas).display = display;
    (*canvas).window = window;
    (*canvas).gc = gc;
    /*  Get GCValues  */
    if (XGetGCValues (display, gc,
		      GCFunction | GCPlaneMask |
		      GCForeground | GCBackground | GCLineWidth | GCLineStyle |
		      GCCapStyle | GCJoinStyle | GCFillStyle | GCFillRule |
		      GCTile | GCStipple | GCTileStipXOrigin |
		      GCTileStipYOrigin | GCFont | GCSubwindowMode |
		      GCGraphicsExposures | GCClipXOrigin | GCClipYOrigin |
		      GCDashOffset | GCArcMode,
		      &(*canvas).gcvalues) == 0)
    {
	(void) fprintf (stderr, "Error getting GC values\n");
	m_free ( (char *) canvas );
	return (NULL);
    }
    (*canvas).xoff = xoff;
    (*canvas).yoff = yoff;
    (*canvas).width = width;
    (*canvas).height = height;
    (*canvas).shm_available = xi_check_shared_images_available (display);
    return (canvas);
}   /*  End Function kwin_create_x  */

/*PUBLIC_FUNCTION*/
void kwin_set_gc_x (canvas, gc)
/*  This routine will register a new Graphics Context to be used when drawing
    into the pixel canvas. Subsequent drawing operations will use the new
    Graphics Context.
    NOTE: this routine is only available with X windows.
    The canvas must be given by  canvas  .
    The Graphics Context must be given by  gc  .
    The routine returns nothing.
*/
KPixCanvas canvas;
GC gc;
{
    static char function_name[] = "kwin_set_gc_x";

    VERIFY_CANVAS (canvas);
    (*canvas).gc = gc;
    /*  Get GCValues  */
    if (XGetGCValues ( (*canvas).display, (*canvas).gc,
		      GCFunction | GCPlaneMask |
		      GCForeground | GCBackground | GCLineWidth | GCLineStyle |
		      GCCapStyle | GCJoinStyle | GCFillStyle | GCFillRule |
		      GCTile | GCStipple | GCTileStipXOrigin |
		      GCTileStipYOrigin | GCFont | GCSubwindowMode |
		      GCGraphicsExposures | GCClipXOrigin | GCClipYOrigin |
		      GCDashOffset | GCArcMode,
		      &(*canvas).gcvalues ) == 0)
    {
	(void) fprintf (stderr, "Error getting GC values\n");
	return;
    }
}   /*  End Function kwin_set_gc_x  */

#endif  /*  X11  */

/*  VX specific functions follow  */
#ifdef ARCH_VXMVX

/*PUBLIC_FUNCTION*/
KPixCanvas kwin_create_vx (pseudo_colour, xoff, yoff, width, height)
/*  This routine will create a pixel canvas, ready for drawing, on a VX screen.
    NOTE: this routine is only available when running on a VX.
    If the value of  pseudo_colour  is TRUE, then the canvas is an 8 bit
    PseudoColour canvas, else it is a 24 bit DirectColour canvas.
    NOTE: when drawing onto the canvas, changes will not be visible unless
    vc_set_visual  has been called with the appropriate parameters.
    The horizontal offset of the canvas origin (upper-left corner) relative to
    the parent window must be given by  xoff  .
    The vertical offset of the canvas origin (upper-left corner) relative to
    the parent window must be given by  yoff  .
    The width (vertical extent) of the canvas must be given by  width  .
    The height (horizontal extent) of the canvas must be given by  height  .
    The routine returns a pixel canvas on success, else it returns NULL.
*/
flag pseudo_colour;
int xoff;
int yoff;
int width;
int height;
{
    static char function_name[] = "kwin_create_vx";

    (void) fprintf (stderr, "%s: not implemented\n", function_name);
    return (NULL);
}   /*  End Function kwin_create_vx  */

#endif  /*  ARCH_VXMVX  */

/*  Generic functions follow  */

/*PUBLIC_FUNCTION*/
void kwin_register_refresh_func (canvas, refresh_func, info)
/*  This routine will register a refresh function for a pixel canvas. The
    refresh function will be called whenever the contents of the canvas need to
    be redrawn. Many refresh functions may be registered per canvas. The first
    function registered is the first function called upon refresh.
    The canvas must be given by  canvas  .
    The function that is called when the canvas is to be refreshed must be
    pointed to by  refresh_func  .
    The interface to this routine is as follows:

    void refresh_func (canvas, width, height, info)
    *   This routine will process a refresh event  for a pixel canvas.
        The canvas is given by  canvas  .
	The width of the canvas in pixels is given by  width  .
	The height of the canvas in pixels is given by  height  .
	The arbitrary canvas information pointer is pointed to by  info  .
	The routine returns nothing.
    *
    KPixCanvas canvas;
    int width;
    int height;
    void **info;

    The initial arbitrary canvas information pointer must be given by  info  .
    The routine returns nothing.
*/
KPixCanvas canvas;
void (*refresh_func) ();
void *info;
{
    struct refresh_func *new_func;
    static char function_name[] = "kwin_register_refresh_func";

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
}   /*  End Function kwin_register_refresh_func  */

/*PUBLIC_FUNCTION*/
void kwin_register_position_event_func (canvas, position_func, f_info)
/*  This routine will register a position event function for a pixel canvas.
    The position event function will be called whenever a position event on the
    canvas has not been consumed. Many position event functions may be
    registered per canvas. The first function registered is the first function
    called upon a position event.
    The canvas must be given by  canvas  .
    The function that is called when a position event occurs must be pointed to
    by  position_func  .
    The interface to this routine is as follows:

    flag position_func (canvas, x, y, event_code, e_info, f_info)
    *   This routine is a position event consumer for a pixel canvas.
        The canvas is given by  canvas  .
	The horizontal position of the event, relative to the canvas origin,
	will be given by  x  .
	The vertical position of the event, relative to the canvas origin,
	will be given by  y  .
	The arbitrary event code is given by  event_code  .
	The arbitrary event information is pointed to by  e_info  .
	The arbitrary function information pointer is pointed to by  f_info  .
	The routine returns TRUE if the event was consumed, else it return
	FALSE indicating that the event is still to be processed.
    *
    KPixCanvas canvas;
    int x;
    int y;
    unsigned int event_code;
    void *e_info;
    void **f_info;

    The initial arbitrary function information pointer must be given by  f_info
    The routine returns nothing.
*/
KPixCanvas canvas;
flag (*position_func) ();
void *f_info;
{
    struct position_func *new_func;
    static char function_name[] = "kwin_register_position_func";

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
}   /*  End Function kwin_register_position_func  */

/*PUBLIC_FUNCTION*/
flag kwin_resize (canvas, clear, xoff, yoff, width, height)
/*  This routine will register a resize in the pixel canvas size. This will
    cause any refresh routines registered for the canvas to be called. This
    routine is meant to be called by an X event handler for the underlying
    window, however the routine is available for all graphics systems.
    The canvas must be given by  canvas  .
    If the value of  clear  is TRUE, the canvas is first cleared.
    The vertical offset of the canvas origin (upper-left corner) relative to
    the parent window must be given by  yoff  .
    The new width (vertical extent) of the canvas must be given by  width  .
    The new height (horizontal extent) of the canvas must be given by  height
    If either  width  or  height  are less than 1 the canvas is not resized,
    it is only refreshed.
    The routine returns TRUE on success, else it returns FALSE.
*/
KPixCanvas canvas;
flag clear;
int xoff;
int yoff;
int width;
int height;
{
    struct refresh_func *curr_func;
    static char function_name[] = "kwin_resize";

    VERIFY_CANVAS (canvas);
    FLAG_VERIFY (clear);
    /*  Resize canvas  */
    if (xoff < 0)
    {
	(void) fprintf (stderr, "xoff: %d  less than 0\n", xoff);
	return (FALSE);
    }
    if (yoff < 0)
    {
	(void) fprintf (stderr, "yoff: %d  less than 0\n", yoff);
	return (FALSE);
    }
    if ( (width > 0) && (height > 0) && (xoff >= 0) && (yoff >= 0) )
    {
	(*canvas).width = width;
	(*canvas).height = height;
	(*canvas).xoff = xoff;
	(*canvas).yoff = yoff;
    }
    if (clear)
    {
#ifdef X11
	XClearArea ( (*canvas).display, (*canvas).window,
		    (*canvas).xoff, (*canvas).yoff,
		    (unsigned int) (*canvas).width,
		    (unsigned int) (*canvas).height, False );
#endif
    }
    /*  Call refresh functions  */
    for (curr_func = (*canvas).first_refresh_func; curr_func != NULL;
	 curr_func = (*curr_func).next)
    {
	(* (*curr_func).func ) (canvas, (*canvas).width, (*canvas).height,
				&(*curr_func).info);
    }
    return (TRUE);
}   /*  End Function kwin_resize  */

/*PUBLIC_FUNCTION*/
flag kwin_process_position_event (canvas, x, y, clip, event_code, event_info)
/*  This routine will process a position event on the lower level object
    (parent, ie. X window) for a pixel canvas. This event is processed with all
    position event consumer routines until one successfully consumes the event.
    The canvas must be given by  canvas  .
    The horizontal position of the event, relative to the parent window, must
    be given by  x  .
    The vertical position of the event, relative to the parent window, must be
    given by  y  .
    If the event is outside of the canvas boundaries, one of two things may
    happen:
        If  clip  is TRUE, the event co-ordinates are clipped to the nearest
	boundary and passed on to the registered position event consumers.
	If  clip  is FALSE, the event is not consumed.
    The arbitrary event code must be given by  event_code  .
    The arbitrary event information must be pointed to by  event_info  .
    The routine returns TRUE if the event was consumed, else it returns FALSE.
*/
KPixCanvas canvas;
int x;
int y;
flag clip;
unsigned int event_code;
void *event_info;
{
    struct position_func *curr_func;
    static char function_name[] = "kwin_process_position_event";

    VERIFY_CANVAS (canvas);
    if (clip)
    {
	/*  Clip x  */
	if (x < (*canvas).xoff)
	{
	    x = 0;
	}
	else if (x >= (*canvas).xoff + (*canvas).width)
	{
	    x = (*canvas).width - 1;
	}
	else
	{
	    x -= (*canvas).xoff;
	}
	/*  Clip y  */
	if (y < (*canvas).yoff)
	{
	    y = 0;
	}
	else if (y >= (*canvas).yoff + (*canvas).height)
	{
	    y = (*canvas).height - 1;
	}
	else
	{
	    y -= (*canvas).yoff;
	}
    }
    else
    {
	/*  Discard if outside  */
	if ( (x < (*canvas).xoff) || (x >= (*canvas).xoff + (*canvas).width) ||
	    (y < (*canvas).yoff) || (y >= (*canvas).yoff + (*canvas).height) )
	{
	    return (FALSE);
	}
	x -= (*canvas).xoff;
	y -= (*canvas).yoff;
    }
    /*  Have event in canvas co-ordinates  */
    /*  Call event consumer functions  */
    for (curr_func = (*canvas).first_position_func; curr_func != NULL;
	 curr_func = (*curr_func).next)
    {
	if ( (* (*curr_func).func ) (canvas, x, y, event_code, event_info,
				     &(*curr_func).info) )
	{
	    /*  Consumed  */
	    return (TRUE);
	}
    }
    return (FALSE);
}   /*  End Function kwin_process_position_event  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_image (canvas, arr_desc, slice, hdim, vdim, elem_index,
		      num_pixels, pixel_values, win_scale, cache_ptr)
/*  This routine will draw a 2-dimensional slice of a Karma array onto a pixel
    canvas. This slice may be tiled.
    The canvas must be given by  canvas  .
    The array descriptor must be pointed to by  arr_desc  .
    The start of the slice data must be pointed to by  slice  .
    The dimension index of the horizontal dimension must be given by  hdim  .
    The dimension index of the vertical dimension must be given by  vdim  .
    The element index of the data packets must be given by  elem_index  .
    The computed minimum value of the slice should be given by  computed_min  .
    This must be of type K_DCOMPLEX.
    The number of pixels in the colourmap must be given by  num_pixels  .
    The array of colourmap pixel values must be pointed to by  pixel_values  .
    These pixel values are used when translating the data into pixel values.
    The window scaling information must be pointed to by  win_scale  .
    The routine may produce cache data which will vastly increase the speed of
    subsequent operations on this data. The routine will write a pointer to
    this data to the storage pointed to by  cache_ptr  .Prior to process
    exit, a call MUST be made to  kwin_free_cache_data  ,otherwise shared
    memory segments could remain after the process exits.
    The routine returns TRUE on success, else it returns FALSE.
*/
KPixCanvas canvas;
array_desc *arr_desc;
char *slice;
unsigned int hdim;
unsigned int vdim;
unsigned int elem_index;
unsigned int num_pixels;
unsigned long *pixel_values;
struct win_scale_type *win_scale;
KPixCanvasImageCache *cache_ptr;
{
#ifdef X11
    flag allocate = FALSE;
    unsigned int dim_count;
    unsigned int hstride, vstride;
#endif
    KPixCanvasImageCache cache;
    static char function_name[] = "kwin_draw_image";

    VERIFY_CANVAS (canvas);
    if ( (cache_ptr == NULL) || (*cache_ptr == NULL) )
    {
	if ( ( cache = alloc_cache_data_struct (canvas) )
	    == NULL )
	{
	    m_error_notify (function_name, "cache data structure");
	    return (FALSE);
	}
    }
    else
    {
	cache = *cache_ptr;
    }
    if ( (*cache).magic_number != CACHE_DATA_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid cache data\n");
	a_prog_bug (function_name);
    }
#ifdef X11
    /*  Check if existing XImage is big enough  */
    if ( (*cache).ximage != NULL )
    {
	if ( ( (*win_scale).x_pixels > (* (*cache).ximage ).width ) ||
	    ( (*win_scale).y_pixels > (* (*cache).ximage ).height ) )
	{
	    /*  XImage is too small  */
	    xi_destroy_image ( (*cache).display, (*cache).ximage,
			      (*cache).shared );
	    (*cache).ximage = NULL;
	    (*cache).shared = FALSE;
	    allocate = TRUE;
	}
    }
    else
    {
	allocate = TRUE;
    }
    if (allocate)
    {
	/*  Allocate XImage  */
	if ( (*canvas).shm_available )
	{
	    (*cache).shared = TRUE;
	}
	if ( ( (*cache).ximage = xi_create_image ( (*canvas).display,
						  (*canvas).window,
						  (*win_scale).x_pixels,
						  (*win_scale).y_pixels,
						  &(*cache).shared ) )
	    == NULL )
	{
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL)
	    {
		*cache_ptr = NULL;
	    }
	    return (FALSE);
	}
    }
    /*  Now have an XImage big enough  */
    hstride = ds_get_packet_size ( (*arr_desc).packet );
    for (dim_count = (*arr_desc).num_dimensions - 1; dim_count > hdim;
	 --dim_count)
    {
	hstride *= (* (*arr_desc).dimensions[dim_count] ).length;
    }
    vstride = ds_get_packet_size ( (*arr_desc).packet );
    for (dim_count = (*arr_desc).num_dimensions - 1; dim_count > vdim;
	 --dim_count)
    {
	vstride *= (* (*arr_desc).dimensions[dim_count] ).length;
    }
    if (drw_single_plane ( (*cache).ximage, num_pixels, pixel_values,
			  slice,
			  (* (*arr_desc).packet ).element_types[elem_index],
			  (*win_scale).conv_type,
			  (*arr_desc).dimensions[hdim], hstride,
			  (*arr_desc).dimensions[vdim], vstride, win_scale )
	!= TRUE)
    {
	(void) fprintf (stderr, "Error drawing image into KPixCanvas\n");
	kwin_free_cache_data (cache);
	if (cache_ptr != NULL)
	{
	    *cache_ptr = NULL;
	}
	return (FALSE);
    }
    (*cache).width = (*win_scale).x_pixels;
    (*cache).height = (*win_scale).y_pixels;
    xi_put_image ( (*canvas).display, (*canvas).window, (*canvas).gc,
		  (*cache).ximage,
		  0, 0,
		  (*win_scale).x_offset + (*canvas).xoff,
		  (*win_scale).y_offset + (*canvas).yoff,
		  (*cache).width, (*cache).height,
		  (*cache).shared, TRUE );
#endif  /*  X11  */
    if (cache_ptr == NULL)
    {
	kwin_free_cache_data (cache);
    }
    else
    {
	*cache_ptr = cache;
    }
#undef FINISHED
#ifdef X11
#define FINISHED
    return (TRUE);
#endif

#ifndef FINISHED
    (void) fprintf (stderr, "%s: not finished yet\n", function_name);
    return (FALSE);
#endif
}   /*  End Function kwin_draw_image  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_cached_image (cache, x_off, y_off)
/*  This routine will draw a previously computed image cache data (computed by
    kwin_draw_image  ) onto the canvas which the original image was drawn.
    The cache data must be given by  cache  .
    The horizontal offset, relative to the top-left corner of the canvas, must
    be given by  x_off  .
    The vertical offset, relative to the top-left corner of the canvas, must
    be given by  y_off  .
    The routine returns TRUE on success, if there is valid cache data,
    else it returns FALSE, indicating that the image must be recomputed and
    drawn using  kwin_draw_image  .
*/
KPixCanvasImageCache cache;
int x_off;
int y_off;
{
    KPixCanvas canvas;
    static char function_name[] = "kwin_draw_cached_image";

    if (cache == NULL) return (FALSE);
    if ( (*cache).magic_number != CACHE_DATA_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid cache data\n");
	a_prog_bug (function_name);
    }
    canvas = (*cache).canvas;
#undef FINISHED
#ifdef X11
#define FINISHED
    if ( (*cache).ximage == NULL ) return (FALSE);
    xi_put_image ( (*canvas).display, (*canvas).window, (*canvas).gc,
		  (*cache).ximage,
		  0, 0, x_off + (*canvas).xoff, y_off + (*canvas).yoff,
		  (*cache).width, (*cache).height,
		  (*cache).shared, TRUE );
    return (TRUE);
#endif  /*  X11  */

#ifndef FINISHED
    return (FALSE);
#endif
}   /*  End Function kwin_draw_cached_image  */

/*PUBLIC_FUNCTION*/
void kwin_draw_point (canvas, x, y, pixel_value)
/*  This routine will draw a single point onto a pixel canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the point must be given by  x  .
    The vertical offset of the point must be given by  y  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KPixCanvas canvas;
int x;
int y;
unsigned long pixel_value;
{
    static char function_name[] = "kwin_draw_point";
#ifdef X11
#endif

    VERIFY_CANVAS (canvas);
#undef FINISHED
#ifdef X11
#define FINISHED
    if (pixel_value != (*canvas).gcvalues.foreground)
    {
	(*canvas).gcvalues.foreground = pixel_value;
	XChangeGC ( (*canvas).display, (*canvas).gc, GCForeground,
		   &(*canvas).gcvalues );
    }
    XDrawPoint ( (*canvas).display, (*canvas).window, (*canvas).gc,
		(*canvas).xoff + x, (*canvas).yoff + y );
    return;
#endif

#ifndef FINISHED
    (void) fprintf (stderr, "%s: not finished yet\n", function_name);
#endif
}   /*  End Function kwin_draw_point  */

/*PUBLIC_FUNCTION*/
void kwin_draw_line (canvas, x0, y0, x1, y1, pixel_value)
/*  This routine will draw a single point onto a pixel canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the first point must be given by  x0  .
    The vertical offset of the first point must be given by  y0  .
    The horizontal offset of the second point must be given by  x1  .
    The vertical offset of the second point must be given by  y1  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KPixCanvas canvas;
int x0;
int y0;
int x1;
int y1;
unsigned long pixel_value;
{
    static char function_name[] = "kwin_draw_line";

    VERIFY_CANVAS (canvas);
#undef FINISHED
#ifdef X11
#define FINISHED
    if (pixel_value != (*canvas).gcvalues.foreground)
    {
	(*canvas).gcvalues.foreground = pixel_value;
	XChangeGC ( (*canvas).display, (*canvas).gc, GCForeground,
		   &(*canvas).gcvalues );
    }
    XDrawLine ( (*canvas).display, (*canvas).window, (*canvas).gc,
	       (*canvas).xoff + x0, (*canvas).yoff + y0,
	       (*canvas).xoff + x1, (*canvas).yoff + y1 );
    return;
#endif

#ifndef FINISHED
    (void) fprintf (stderr, "%s: not finished yet\n", function_name);
#endif
}   /*  End Function kwin_draw_line  */

/*PUBLIC_FUNCTION*/
void kwin_fill_ellipse (canvas, cx, cy, rx, ry, pixel_value)
/*  This routine will draw a filled ellipse onto a pixel canvas.
    The canvas must be given by  canvas  .
    The co-ordinates of the centre of the ellipse must be given by  cx  and cy
    The radii must be given by  rx  and  ry  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KPixCanvas canvas;
int cx;
int cy;
int rx;
int ry;
unsigned long pixel_value;
{
    static char function_name[] = "kwin_fill_ellipse";

    VERIFY_CANVAS (canvas);
#undef FINISHED
#ifdef X11
#define FINISHED
    if (pixel_value != (*canvas).gcvalues.foreground)
    {
	(*canvas).gcvalues.foreground = pixel_value;
	XChangeGC ( (*canvas).display, (*canvas).gc, GCForeground,
		   &(*canvas).gcvalues );
    }
    XFillArc ( (*canvas).display, (*canvas).window, (*canvas).gc,
	      (*canvas).xoff + cx - rx, (*canvas).yoff + cy - ry,
	      2 * rx, 2 * ry,
	      64 * 360, 64 * 360);
    return;
#endif

#ifndef FINISHED
    (void) fprintf (stderr, "%s: not finished yet\n", function_name);
#endif
}   /*  End Function kwin_fill_ellipse  */

/*PUBLIC_FUNCTION*/
flag kwin_fill_polygon (canvas, point_x, point_y, num_vertices, pixel_value,
			convex)
/*  This routine will draw a filled polygon onto a world canvas.
    The canvas must be given by  canvas  .
    The array of x co-ordinates of vertices of the polygon must be pointed
    to by  point_x  .
    The array of y co-ordinates of vertices of the polygon must be pointed
    to by  point_y  .
    The number of vertices in the polygon must be given by  num_vertices  .
    The pixel value to use must be given by  pixel_value  .
    If the value of  convex  is TRUE, then the points must form a convex
    polygon  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KPixCanvas canvas;
int *point_x;
int *point_y;
unsigned int num_vertices;
unsigned long pixel_value;
flag convex;
{
    unsigned int coord_count;
#ifdef X11
    static unsigned int num_points_allocated = 0;
    static XPoint *points = NULL;
#endif
    static char function_name[] = "kwin_fill_polygon";

    VERIFY_CANVAS (canvas);
    FLAG_VERIFY (convex);
#undef FINISHED
#ifdef X11
#define FINISHED
    if (pixel_value != (*canvas).gcvalues.foreground)
    {
	(*canvas).gcvalues.foreground = pixel_value;
	XChangeGC ( (*canvas).display, (*canvas).gc, GCForeground,
		   &(*canvas).gcvalues );
    }
    if (num_vertices > num_points_allocated)
    {
	if (points != NULL)
	{
	    m_free ( (char *) points );
	    points = NULL;
	}
	num_points_allocated = 0;
	if ( ( points = (XPoint *) m_alloc (sizeof *points * num_vertices) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of XPoint structures");
	    return (FALSE);
	}
	num_points_allocated = num_vertices;
    }
    /*  Convert world co-ordinates to pixel co-ordinates  */
    for (coord_count = 0; coord_count < num_vertices; ++coord_count)
    {
	points[coord_count].x = (*canvas).xoff + point_x[coord_count];
	points[coord_count].y = (*canvas).yoff + point_y[coord_count];
    }
    XFillPolygon ( (*canvas).display, (*canvas).window, (*canvas).gc,
		  points, (int) num_vertices, convex ? Convex : Complex,
		  CoordModeOrigin);
    return (TRUE);
#endif

#ifndef FINISHED
    (void) fprintf (stderr, "%s: not finished yet\n", function_name);
    return (FALSE);
#endif
}   /*  End Function kwin_fill_polygon  */

/*PUBLIC_FUNCTION*/
void kwin_get_size (canvas, width, height)
/*  This routine will get the size of a pixel canvas.
    The number of horizontal pixel will be written to the storage pointed to by
    width  .
    The number of vertical pixel will be written to the storage pointed to by
    height  .
    The routine returns nothing.
*/
KPixCanvas canvas;
int *width;
int *height;
{
    static char function_name[] = "kwin_get_size";

    VERIFY_CANVAS (canvas);
    *width = (*canvas).width;
    *height = (*canvas).height;
}   /*  End Function kwin_get_size  */

/*PUBLIC_FUNCTION*/
void kwin_free_cache_data (cache)
/*  This routine will free some cache data allocated by  kwin_draw_image  .
    The cache data must be given by  cache  .
    The routine returns nothing.
*/
KPixCanvasImageCache cache;
{
    static char function_name[] = "kwin_free_cache_data";

    if (cache == NULL) return;
    if ( (*cache).magic_number != CACHE_DATA_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid cache data\n");
	a_prog_bug (function_name);
    }
#ifdef X11
    xi_destroy_image ( (*cache).display, (*cache).ximage, (*cache).shared );
#endif
    (*cache).magic_number = 0;
    m_free ( (char *) cache );
}   /*  End Function kwin_free_cache_data  */

/*PUBLIC_ FUNCTION*/
void kwin_free_all_cache_data ()
/*  This routine will free all cache data allocated by  kwin_draw_image  .
    This routine is an alternative to calling  kwin_free_cache_data  for each
    set of cache data.
    The routine returns nothing.
*/
{
}   /*  End Function kwin_free_all_cache_data  */

/*PUBLIC_FUNCTION*/
flag kwin_convert_to_canvas_coord (canvas, xin, yin, xout, yout)
/*  This routine will convert co-ordinates in a lower level object (parent,
    ie. X window) to co-ordinates in a pixel canvas.
    The canvas must be given by  canvas  .
    The lower level horizontal co-ordinate must be given by  xin  .
    The lower level vertical co-ordinate must be given by  yin  .
    The horizontal canvas co-ordinate will be written to the storage pointed to
    by  xout  .
    The vertical canvas co-ordinate will be written to the storage pointed to
    by  xout  .
    The routine returns TRUE if the co-ordinate lies within the canvas
    boundaries, else it returns FALSE (although a conversion is still
    performed).
*/
KPixCanvas canvas;
int xin;
int yin;
int *xout;
int *yout;
{
    static char function_name[] = "kwin_convert_to_canvas_coord";

    VERIFY_CANVAS (canvas);
    *xout = xin - (*canvas).xoff;
    *yout = yin - (*canvas).yoff;
    if ( (*xout < 0) || (*xout >= (*canvas).width) ||
	(*yout < 0) || (*yout >= (*canvas).height) ) return (FALSE);
    return (TRUE);
}   /*  End Function kwin_convert_to_canvas_coord  */


/*  Private functions follow  */

static KPixCanvas alloc_canvas ()
/*  This routine will allocate a canvas.
    The routine returns a canvas on success, else it returns NULL.
*/
{
    KPixCanvas canvas;
    static char function_name[] = "alloc_canvas";

    if ( ( canvas = (KPixCanvas) m_alloc (sizeof *canvas) ) == NULL )
    {
	m_error_notify (function_name, "pixel canvas");
	return (NULL);
    }
    (*canvas).magic_number = CANVAS_MAGIC_NUMBER;
    (*canvas).xoff = 0;
    (*canvas).yoff = 0;
    (*canvas).width = 0;
    (*canvas).height = 0;
    (*canvas).shm_available = FALSE;
    (*canvas).first_refresh_func = NULL;
    (*canvas).last_refresh_func = NULL;
    (*canvas).first_position_func = NULL;
    (*canvas).last_position_func = NULL;
#ifdef X11
    (*canvas).display = NULL;
    (*canvas).window = 0;
    (*canvas).gc = 0;
#endif
    return (canvas);
}   /*  End Function alloc_canvas  */

static KPixCanvasImageCache alloc_cache_data_struct (canvas)
/*  This routine will allocate a cache data struct which may be used for a
    canvas.
    The canvas must be given by  canvas  .
    The routine returns a pointer to the structure on success,
    else it returns NULL.
*/
KPixCanvas canvas;
{
    KPixCanvasImageCache cache;
    static char function_name[] = "alloc_cache_data_struct";

    VERIFY_CANVAS (canvas);
    if ( ( cache = (KPixCanvasImageCache) m_alloc (sizeof *cache) )
	== NULL )
    {
	m_error_notify (function_name, "cache data structure");
	return (NULL);
    }
    (*cache).magic_number = CACHE_DATA_MAGIC_NUMBER;
    (*cache).canvas = canvas;
#ifdef X11
    (*cache).display = (*canvas).display;
    (*cache).ximage = NULL;
    (*cache).shared = FALSE;
    (*cache).width = 0;
    (*cache).height = 0;
#endif
    return (cache);
}   /*  End Function alloc_cache_data_struct  */

/*LINTLIBRARY*/
#ifdef X11
/*  X11.c

    This code provides KPixCanvas objects.

    Copyright (C) 1995  Richard Gooch

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
    canvas (window) independent of the graphics system in use. This file
    contains the X11 code.


    Written by      Richard Gooch   20-SEP-1995

    Last updated by Richard Gooch   22-SEP-1995: Completed passing of RGB masks
  and offsets to <kwin_create_generic>.


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_kwin.h>
#include <karma_drw.h>
#include <karma_xi.h>
#include <X11/Xutil.h>
#include <karma_psw.h>
#include <karma_imw.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_r.h>
#include <sys/time.h>
#include <sys/resource.h>


#define CANVAS_MAGIC_NUMBER (unsigned int) 32987453
#define CACHE_DATA_MAGIC_NUMBER (unsigned int) 924723489
#define FONT_MAGIC_NUMBER (unsigned int) 298732498

#define VERIFY_CANVAS(canvas) if (canvas == NULL) \
{(void) fprintf (stderr, "NULL canvas passed\n"); \
 a_prog_bug (function_name); } \
if (canvas->magic_number != CANVAS_MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid canvas object\n"); \
 a_prog_bug (function_name); }

#define VERIFY_FONT(font) if (font == NULL) \
{(void) fprintf (stderr, "NULL font passed\n"); \
 a_prog_bug (function_name); } \
if (font->magic_number != FONT_MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid font object\n"); \
 a_prog_bug (function_name); }

#define MAX_POINTS 10000
#define MAX_ARCS 10000
#define MAX_SEGMENTS 10000

/*  Structure declarations  */

typedef struct X11canvas_type
{
    unsigned int magic_number;
    KPixCanvas pixcanvas;
    Display *display;
    Window window;
    Colormap cmap;
    XVisualInfo vinfo;
    flag shm_available;
    GC gc;
    XGCValues gcvalues;
    long max_request_size;
    struct colourcell_type *colours;
    XImage *common_ximage;       /*  Only used for dumping to Pixmaps  */
} *X11Canvas;

struct cache_data_type
{
    KPixCanvas pixcanvas;
    /*  Specific routines define bigger strucures  */
    X11Canvas x11canvas;
    unsigned int magic_number;
    unsigned int width;          /*  The width of the image in the cache   */
    unsigned int height;         /*  The height of the image in the cache  */
    XImage *ximage;
    flag shared;
    Pixmap pixmap;
    unsigned int p_width;        /*  The width of the Pixmap               */
    unsigned int p_height;       /*  The height of the Pixmap              */
};

struct colourcell_type
{
    char *name;
    unsigned long pixel_value;
    unsigned short red;
    unsigned short green;
    unsigned short blue;
    unsigned short pad;
    struct colourcell_type *next;
};


/*  Private data  */
static int num_xpoints_allocated = 0;
static XPoint *xpoints = NULL;
static int num_xarcs_allocated = 0;
static XArc *xarcs = NULL;
static int num_xsegments_allocated = 0;
static XSegment *xsegments = NULL;

/*  Mandatory functions  */
STATIC_FUNCTION (flag draw_point, (void *v_canvas, double x, double y,
				   unsigned long pixel_value) );
STATIC_FUNCTION (void *create_child, (void *v_parent, KPixCanvas child) );
STATIC_FUNCTION (flag clear_area, (void *v_canvas, int x, int y,
				   int width, int height) );
/*  Optional hook functions  */
STATIC_FUNCTION (flag draw_pc_image,
		 (X11Canvas x11canvas,
		  int x_off, int y_off, int x_pixels, int y_pixels,
		  CONST char *slice,
		  CONST uaddr *hoffsets, CONST uaddr *voffsets,
		  unsigned int width, unsigned int height,
		  unsigned int type, unsigned int conv_type,
		  unsigned int num_pixels,unsigned long *pixel_values,
		  unsigned long blank_pixel,
		  unsigned long min_sat_pixel, unsigned long max_sat_pixel,
		  double i_min, double i_max,
		  flag (*iscale_func) (), void *iscale_info,
		  KPixCanvasImageCache *cache_ptr) );
STATIC_FUNCTION (flag draw_rgb_image,
		 (X11Canvas x11canvas,
		  int x_off, int y_off, int x_pixels, int y_pixels,
		  CONST unsigned char *red_slice,
		  CONST unsigned char *green_slice,
		  CONST unsigned char *blue_slice,
		  CONST uaddr *hoffsets, CONST uaddr *voffsets,
		  unsigned int width, unsigned int height,
		  KPixCanvasImageCache *cache_ptr) );
STATIC_FUNCTION (flag draw_cached_image,
		 (KPixCanvasImageCache cache, int x_off, int y_off) );
STATIC_FUNCTION (void free_cache_data, (KPixCanvasImageCache cache) );
STATIC_FUNCTION (flag draw_line, (X11Canvas x11canvas,
				  double x0, double y0, double x1, double y1,
				  unsigned long pixel_value) );
STATIC_FUNCTION (flag draw_arc,
		 (X11Canvas x11canvas,
		  double x, double y, double width, double height,
		  int angle1, int angle2, unsigned long pixel_value,
		  flag fill) );
STATIC_FUNCTION (flag draw_polygon,
		 (X11Canvas x11canvas, double *x_arr, double *y_arr,
		  unsigned int num_vertices, unsigned long pixel_value,
		  flag convex, flag fill) );
STATIC_FUNCTION (flag draw_string,
		 (X11Canvas x11canvas, double x, double y, CONST char *string,
		  unsigned long pixel_value, flag clear_under) );
STATIC_FUNCTION (flag draw_rectangle,
		 (X11Canvas x11canvas,
		  double x, double y, double width, double height,
		  unsigned long pixel_value, flag fill) );
STATIC_FUNCTION (flag draw_lines,
		 (X11Canvas x11canvas, double *x_arr, double *y_arr,
		  unsigned int num_points, unsigned long pixel_value) );
STATIC_FUNCTION (flag draw_arcs,
		 (X11Canvas x11canvas,
		  double *x, double *y, double *width, double *height,
		  int *angle1, int *angle2, unsigned int num_ellipses,
		  unsigned long pixel_value, flag fill) );
STATIC_FUNCTION (flag draw_segments,
		 (X11Canvas x11canvas,
		  double *x0, double *y0, double *x1, double *y1,
		  unsigned int num_segments, unsigned long pixel_value) );
STATIC_FUNCTION (flag get_colour,
		 (X11Canvas x11canvas, CONST char *colourname,
		  unsigned long *pixel_value, unsigned short *red,
		  unsigned short *green, unsigned short *blue) );
STATIC_FUNCTION (XFontStruct *load_font,
		 (X11Canvas x11canvas, CONST char *fontname) );
STATIC_FUNCTION (flag get_string_size,
		 (XFontStruct *x11font, CONST char *string, va_list argp) );
STATIC_FUNCTION (void set_font, (X11Canvas x11canvas, XFontStruct *x11font) );
STATIC_FUNCTION (flag query_colourmap,
		 (X11Canvas canvas, unsigned long *pixels,
		  unsigned short *reds, unsigned short *greens,
		  unsigned short *blues, unsigned int num_colours) );
/*  Private functions  */
STATIC_FUNCTION (void set_pixel_in_gc,
		 (X11Canvas x11canvas, unsigned long pixel_value) );
STATIC_FUNCTION (XVisualInfo *get_visinfo_for_visual,
		 (Display *dpy, Visual *visual) );
STATIC_FUNCTION (KPixCanvasImageCache size_cache,
		 (X11Canvas x11canvas, KPixCanvasImageCache *cache_ptr,
		  unsigned int width, unsigned int height) );
STATIC_FUNCTION (void alloc_points, (X11Canvas x11canvas, int num_points) );
STATIC_FUNCTION (void alloc_arcs, (X11Canvas x11canvas, int num_arcs) );
STATIC_FUNCTION (void alloc_segments, (X11Canvas x11canvas,int num_segments) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KPixCanvas kwin_create_x (Display *display, Window window, GC gc,
			  int xoff, int yoff, int width, int height)
/*  [PURPOSE] This routine will create a pixel canvas, ready for drawing, from
    an X window. Note that the origin of a KPixCanvas is the upper-left corner.
    [NOTE] This routine is only available with the X window system.
    <display> The X display.
    <window> The window ID in which the canvas lies.
    <gc> The graphics context.
    <xoff> The horizontal offset of the canvas origin (upper-left corner)
    relative to the parent window.
    <yoff> The vertical offset of the canvas origin (upper-left corner)
    relative to the parent window.
    <width> The width (vertical extent) of the canvas.
    <height> The height (horizontal extent) of the canvas.
    [RETURNS] A pixel canvas on success, else NULL.
*/
{
    KPixCanvas canvas;
    KPixCanvasFont font;
    X11Canvas x11canvas;
    uaddr red_offset, green_offset, blue_offset;
    XWindowAttributes window_attributes;
    int im_byte_order;
    unsigned int visual;
    unsigned long im_red_mask, im_green_mask, im_blue_mask;
    unsigned char *ch_ptr;
    XVisualInfo *vinfo;
    static char function_name[] = "kwin_create_x";

    if (BitmapUnit (display) != 32)
    {
	(void) fprintf (stderr, "BitmapUnit must be 32\n");
	return (NULL);
    }
    im_byte_order = ImageByteOrder (display);
    /*  Get X colourmap  */
    XGetWindowAttributes (display, window, &window_attributes);
    if (window_attributes.colormap == None)
    {
	(void) fprintf (stderr, "Error: window has no colourmap\n");
	a_prog_bug (function_name);
    }
    if ( ( x11canvas = (X11Canvas) m_alloc (sizeof *x11canvas) ) == NULL )
    {
	m_error_notify (function_name, "X pixel canvas");
	return (NULL);
    }
    m_clear ( (char *) x11canvas, sizeof *x11canvas );
    x11canvas->display = display;
    x11canvas->window = window;
    x11canvas->cmap = window_attributes.colormap;
    x11canvas->gc = gc;
    /*  Get GCValues  */
    if (XGetGCValues (display, gc,
		      GCFunction | GCPlaneMask |
		      GCForeground | GCBackground | GCLineWidth | GCLineStyle |
		      GCCapStyle | GCJoinStyle | GCFillStyle | GCFillRule |
		      GCTile | GCStipple | GCTileStipXOrigin |
		      GCTileStipYOrigin | GCFont | GCSubwindowMode |
		      GCGraphicsExposures | GCClipXOrigin | GCClipYOrigin |
		      GCDashOffset | GCArcMode,
		      &x11canvas->gcvalues) == 0)
    {
	(void) fprintf (stderr, "Error getting GC values\n");
	m_free ( (char *) x11canvas );
	return (NULL);
    }
    x11canvas->shm_available = xi_check_shared_images_available (display);
    x11canvas->max_request_size = XMaxRequestSize (display);
    x11canvas->common_ximage = NULL;
    x11canvas->colours = NULL;
    vinfo = get_visinfo_for_visual (display, window_attributes.visual);
    if (window_attributes.depth != vinfo->depth)
    {
	(void) fprintf (stderr, "Window depth: %d is not visual depth: %d\n",
			window_attributes.depth, vinfo->depth);
	a_prog_bug (function_name);
    }
    m_copy ( (char *) &x11canvas->vinfo, (char *) vinfo, sizeof *vinfo );
    XFree ( (void *) vinfo );
    switch (x11canvas->vinfo.class)
    {
      case DirectColor:
      case TrueColor:
	if (x11canvas->vinfo.colormap_size != 256)
	{
	    (void) fprintf (stderr,
			    "Colourmap size: %d for %s visual is not 256\n",
			    x11canvas->vinfo.colormap_size,
			    (x11canvas->vinfo.class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) x11canvas );
	    return (NULL);
	}
	if (window_attributes.depth != 24)
	{
	    (void) fprintf (stderr,
			    "Depth: %u for %s visual is not 24\n",
			    window_attributes.depth,
			    (x11canvas->vinfo.class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) x11canvas );
	    return (NULL);
	}
	/*  Compute the byte offsets in a word required for drawing images
	    (images are stored in server native format for efficiency).
	    Note that the masks in the vinfo structure are in client order.  */
	switch (x11canvas->vinfo.red_mask)
	{
	  case 0xff:
	    red_offset = (im_byte_order == MSBFirst) ? 3 : 0;
	    break;
	  case 0xff00:
	    red_offset = (im_byte_order == MSBFirst) ? 2 : 1;
	    break;
	  case 0xff0000:
	    red_offset = (im_byte_order == MSBFirst) ? 1 : 2;
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Red mask: 0x%x for %s visual is not valid\n",
			    x11canvas->vinfo.red_mask,
			    (x11canvas->vinfo.class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) x11canvas );
	    return (NULL);
/*
	    break;
*/
	}
	switch (x11canvas->vinfo.green_mask)
	{
	  case 0xff:
	    green_offset = (im_byte_order == MSBFirst) ? 3 : 0;
	    break;
	  case 0xff00:
	    green_offset = (im_byte_order == MSBFirst) ? 2 : 1;
	    break;
	  case 0xff0000:
	    green_offset = (im_byte_order == MSBFirst) ? 1 : 2;
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Green mask: 0x%x for %s visual is not valid\n",
			    x11canvas->vinfo.green_mask,
			    (x11canvas->vinfo.class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) x11canvas );
	    return (NULL);
/*
	    break;
*/
	}
	switch (x11canvas->vinfo.blue_mask)
	{
	  case 0xff:
	    blue_offset = (im_byte_order == MSBFirst) ? 3 : 0;
	    break;
	  case 0xff00:
	    blue_offset = (im_byte_order == MSBFirst) ? 2 : 1;
	    break;
	  case 0xff0000:
	    blue_offset = (im_byte_order == MSBFirst) ? 1 : 2;
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Blue mask: 0x%x for %s visual is not valid\n",
			    x11canvas->vinfo.blue_mask,
			    (x11canvas->vinfo.class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) x11canvas );
	    return (NULL);
/*
	    break;
*/
	}
	/*  Compute the masks required for drawing images (images are stored in
	    server native format for efficiency).  */
	im_red_mask = 0;
	ch_ptr = (unsigned char *) &im_red_mask;
	ch_ptr[red_offset] = 0xff;
	im_green_mask = 0;
	ch_ptr = (unsigned char *) &im_green_mask;
	ch_ptr[green_offset] = 0xff;
	im_blue_mask = 0;
	ch_ptr = (unsigned char *) &im_blue_mask;
	ch_ptr[blue_offset] = 0xff;
	break;
      default:
	break;
    }
    switch (x11canvas->vinfo.class)
    {
      case PseudoColor:
	visual = KWIN_VISUAL_PSEUDOCOLOUR;
	break;
      case DirectColor:
	visual = KWIN_VISUAL_DIRECTCOLOUR;
	break;
      case TrueColor:
	visual = KWIN_VISUAL_TRUECOLOUR;
	break;
      case GrayScale:
	visual = KWIN_VISUAL_GRAYSCALE;
	break;
      case StaticGray:
	visual = KWIN_VISUAL_STATICGRAY;
	break;
      case StaticColor:
	visual = KWIN_VISUAL_STATICCOLOUR;
	break;
      default:
	(void) fprintf (stderr, "Illegal visual class: %u\n",
			x11canvas->vinfo.class);
	a_prog_bug (function_name);
	break;
    }
    if ( (visual == KWIN_VISUAL_DIRECTCOLOUR) ||
	(visual == KWIN_VISUAL_TRUECOLOUR ) )
    {
	canvas = ( kwin_create_generic
		  (x11canvas, xoff, yoff, width, height,
		   window_attributes.depth, visual, TRUE, draw_point,
		   create_child, clear_area,
		   KWIN_ATT_PIX_RED_MASK, x11canvas->vinfo.red_mask,
		   KWIN_ATT_PIX_GREEN_MASK, x11canvas->vinfo.green_mask,
		   KWIN_ATT_PIX_BLUE_MASK, x11canvas->vinfo.blue_mask,
		   KWIN_ATT_IM_RED_MASK, im_red_mask,
		   KWIN_ATT_IM_GREEN_MASK, im_green_mask,
		   KWIN_ATT_IM_BLUE_MASK, im_blue_mask,
		   KWIN_ATT_IM_RED_OFFSET, red_offset,
		   KWIN_ATT_IM_GREEN_OFFSET, green_offset,
		   KWIN_ATT_IM_BLUE_OFFSET, blue_offset,
		   KWIN_FUNC_DRAW_PC_IMAGE, draw_pc_image,
		   KWIN_FUNC_DRAW_RGB_IMAGE, draw_rgb_image,
		   KWIN_FUNC_DRAW_CACHED_IMAGE, draw_cached_image,
		   KWIN_FUNC_FREE_CACHE_DATA, free_cache_data,
		   KWIN_FUNC_DRAW_LINE, draw_line,
		   KWIN_FUNC_DRAW_ARC, draw_arc,
		   KWIN_FUNC_DRAW_POLYGON, draw_polygon,
		   KWIN_FUNC_DRAW_STRING, draw_string,
		   KWIN_FUNC_DRAW_RECTANGLE, draw_rectangle,
		   KWIN_FUNC_DRAW_LINES, draw_lines,
		   KWIN_FUNC_DRAW_ARCS, draw_arcs,
		   KWIN_FUNC_DRAW_SEGMENTS, draw_segments,
		   KWIN_FUNC_GET_COLOUR, get_colour,
		   KWIN_FUNC_LOAD_FONT, load_font,
		   KWIN_FUNC_GET_STRING_SIZE, get_string_size,
		   KWIN_FUNC_SET_FONT, set_font,
		   KWIN_FUNC_QUERY_COLOURMAP, query_colourmap,
		   KWIN_ATT_END) );
    }
    else
    {
	canvas = ( kwin_create_generic
		  (x11canvas, xoff, yoff, width, height,
		   window_attributes.depth,
		   visual, TRUE, draw_point, create_child, clear_area,
		   KWIN_FUNC_DRAW_PC_IMAGE, draw_pc_image,
		   KWIN_FUNC_DRAW_RGB_IMAGE, draw_rgb_image,
		   KWIN_FUNC_DRAW_CACHED_IMAGE, draw_cached_image,
		   KWIN_FUNC_FREE_CACHE_DATA, free_cache_data,
		   KWIN_FUNC_DRAW_LINE, draw_line,
		   KWIN_FUNC_DRAW_ARC, draw_arc,
		   KWIN_FUNC_DRAW_POLYGON, draw_polygon,
		   KWIN_FUNC_DRAW_STRING, draw_string,
		   KWIN_FUNC_DRAW_RECTANGLE, draw_rectangle,
		   KWIN_FUNC_DRAW_LINES, draw_lines,
		   KWIN_FUNC_DRAW_ARCS, draw_arcs,
		   KWIN_FUNC_DRAW_SEGMENTS, draw_segments,
		   KWIN_FUNC_GET_COLOUR, get_colour,
		   KWIN_FUNC_LOAD_FONT, load_font,
		   KWIN_FUNC_GET_STRING_SIZE, get_string_size,
		   KWIN_FUNC_SET_FONT, set_font,
		   KWIN_FUNC_QUERY_COLOURMAP, query_colourmap,
		   KWIN_ATT_END) );
    }
    if (canvas == NULL) m_abort (function_name, "pixel canvas");
    x11canvas->magic_number = CANVAS_MAGIC_NUMBER;
    x11canvas->pixcanvas = canvas;
    if ( ( font = kwin_load_font (canvas, "fixed") ) == NULL )
    {
	m_abort (function_name, "font");
    }
    kwin_set_attributes (canvas,
			 KWIN_ATT_FONT, font,
			 KWIN_ATT_END);
    return (canvas);
}   /*  End Function kwin_create_x  */

/*PUBLIC_FUNCTION*/
void kwin_set_gc_x (KPixCanvas canvas, GC gc)
/*  [PURPOSE] This routine will register a new Graphics Context to be used
    when drawing into the pixel canvas. Subsequent drawing operations will use
    the new Graphics Context.
    [NOTE] This routine is only available with the X window system.
    <canvas> The canvas.
    <gc> The Graphics Context.
    [RETURNS] Nothing.
*/
{
    X11Canvas x11canvas;
    static char function_name[] = "kwin_set_gc_x";

    kwin_get_attributes (canvas,
			 KWIN_ATT_LOWER_HANDLE, &x11canvas,
			 KWIN_ATT_END);
    VERIFY_CANVAS (x11canvas);
    x11canvas->gc = gc;
    /*  Get GCValues  */
    if (XGetGCValues (x11canvas->display, x11canvas->gc,
		      GCFunction | GCPlaneMask |
		      GCForeground | GCBackground | GCLineWidth | GCLineStyle |
		      GCCapStyle | GCJoinStyle | GCFillStyle | GCFillRule |
		      GCTile | GCStipple | GCTileStipXOrigin |
		      GCTileStipYOrigin | GCFont | GCSubwindowMode |
		      GCGraphicsExposures | GCClipXOrigin | GCClipYOrigin |
		      GCDashOffset | GCArcMode,
		      &x11canvas->gcvalues) == 0)
    {
	(void) fprintf (stderr, "Error getting GC values\n");
	return;
    }
}   /*  End Function kwin_set_gc_x  */

/*PUBLIC_FUNCTION*/
GC kwin_get_gc_x (KPixCanvas canvas)
/*  [PURPOSE] This routine will get the Graphics Context used when drawing into
    the pixel canvas. If the Graphics Context is modified, it should be
    registered prior to drawing on the canvas by calling <<kwin_set_gc_x>>.
    [NOTE] This routine is only available with the X window system.
    <canvas> The canvas.
    [RETURNS] The Graphics Context.
*/
{
    X11Canvas x11canvas;
    static char function_name[] = "kwin_get_gc_x";

    kwin_get_attributes (canvas,
			 KWIN_ATT_LOWER_HANDLE, &x11canvas,
			 KWIN_ATT_END);
    VERIFY_CANVAS (x11canvas);
    return (x11canvas->gc);
}   /*  End Function kwin_get_gc_x  */


/*  Hook functions follow  */

static flag draw_point (void *v_canvas, double x, double y,
			unsigned long pixel_value)
/*  [PURPOSE] This routine will draw a point onto an X11 canvas.
    <v_canvas> The X11 canvas.
    <x> The horizontal offset of the point.
    <y> The vertical offset of the point.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    X11Canvas x11canvas = (X11Canvas) v_canvas;
    static char function_name[] = "__kwin_X11_draw_point";

    VERIFY_CANVAS (x11canvas);
    set_pixel_in_gc (x11canvas, pixel_value);
    XDrawPoint (x11canvas->display, x11canvas->window, x11canvas->gc,
		(int) x, (int) y);
    return (TRUE);
}   /*  End Function draw_point  */

static void *create_child (void *v_parent, KPixCanvas child)
/*  [PURPOSE] This routine will create a child X11 canvas.
    <v_parent> The parent X11 canvas.
    [RETURNS] The child X11 canvas on success, else NULL.
*/
{
    X11Canvas x11child;
    X11Canvas x11parent = (X11Canvas) v_parent;
    static char function_name[] = "__kwin_X11_create_child";

    VERIFY_CANVAS (x11parent);
    if ( ( x11child = (X11Canvas) m_alloc (sizeof *x11child) ) == NULL )
    {
	m_error_notify (function_name, "X pixel canvas");
	return (NULL);
    }
    m_copy ( (char *) x11child, (char *) x11parent, sizeof *x11child );
    x11child->pixcanvas = child;
    /*  Create new Graphics Context  */
    x11child->gc = XCreateGC (x11child->display, x11child->window,
			      GCFunction | GCPlaneMask |
			      GCForeground | GCBackground | GCLineWidth |
			      GCLineStyle | GCCapStyle | GCJoinStyle |
			      GCFillStyle | GCFillRule |
			      GCTileStipXOrigin | GCTileStipYOrigin | GCFont |
			      GCSubwindowMode | GCGraphicsExposures |
			      GCClipXOrigin | GCClipYOrigin | GCDashOffset |
			      GCArcMode,
			      &x11child->gcvalues);
    return ( (void *) x11child );
}   /*  End Function create_child  */

static flag clear_area (void *v_canvas, int x, int y,
			int width, int height)
/*  [PURPOSE] This routine will clear an area in an X11 canvas.
    <v_canvas> The X11 canvas.
    <x> The horizontal offset of the area.
    <y> The vertical offset of the area.
    <width> The width of the area.
    <height> The height of the area.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    X11Canvas x11canvas = (X11Canvas) v_canvas;
    static char function_name[] = "__kwin_X11_clear_area";

    VERIFY_CANVAS (x11canvas);
    XClearArea (x11canvas->display, x11canvas->window,
		x, y, (unsigned int) width, (unsigned int) height, False);
    return (TRUE);
}   /*  End Function clear_area  */

static flag draw_pc_image (X11Canvas x11canvas, int x_off, int y_off,
			   int x_pixels, int y_pixels, CONST char *slice,
			   CONST uaddr *hoffsets, CONST uaddr *voffsets,
			   unsigned int width, unsigned int height,
			   unsigned int type, unsigned int conv_type,
			   unsigned int num_pixels,unsigned long *pixel_values,
			   unsigned long blank_pixel,
			   unsigned long min_sat_pixel,
			   unsigned long max_sat_pixel,
			   double i_min, double i_max,
			   flag (*iscale_func) (), void *iscale_info,
			   KPixCanvasImageCache *cache_ptr)
/*  [PURPOSE] This routine will draw a 2-dimensional slice of a Karma array
    onto an X11 canvas. This slice may be tiled. The slice is a PseudoColour
    image.
    <x11canvas> The X11 canvas.
    <x_off> The horizontal offset, relative to the top-left corner of the
    canvas.
    <y_off> The vertical offset, relative to the top-left corner of the canvas.
    <x_pixels> The number of horizontal pixels to draw.
    <y_pixels> The number of vertical pixels to draw.
    <slice> The start of the slice image data.
    <hoffsets> The array of horizontal byte offsets.
    <voffsets> The array of vertical byte offsets.
    <width> The width of the input image (in values).
    <height> The height of the input image (in values).
    <type> The type of the slice image data.
    <conv_type> The input conversion type (when the input is complex).
    <num_pixels> The number of pixels in the pixel array.
    <pixel_values> The array of pixel values.
    <blank_pixel> The pixel value to be used when the intensity value is an
    undefined value.
    <min_sat_pixel> The pixel value to be used when the intensity value is
    below the minimum value.
    <max_sat_pixel> The pixel value to be used when the intensity value is
    above the maximum value.
    <i_min> The minimum intensity value.
    <i_max> The maximum intensity value.
    <iscale_func> The function to be called when non-linear intensity scaling
    is required. If NULL, linear intensity scaling is used. The interface to
    this function is as follows:
    [<pre>]
    flag iscale_func (double *out, unsigned int out_stride,
                      double *inp, unsigned int inp_stride,
		      unsigned int num_values, double i_min, double i_max,
		      void *info)
    *   [PURPOSE] This routine will perform an arbitrary intensity scaling on
        an array of values. This routine may be called many times to scale an
	image.
        <out> The output array.
	<out_stride> The stride (in doubles) of the output array.
	<inp> The input array.
	<inp_stride> The stride (in doubles) of the input array.
	<num_values> The number of values to scale.
	<i_min> The minimum intensity value.
	<i_max> The maximum intensity value.
	<info> A pointer to arbitrary information.
	[RETURNS] TRUE on success, else FALSE.
    *
    [</pre>]
    <iscale_info> A pointer to arbitrary information for <<iscale_func>>.
    <cache_ptr> Cache data is written here. The routine may produce cache data
    which will vastly increase the speed of subsequent operations on this data.
    Prior to process exit, a call MUST be made to <<kwin_free_cache_data>>,
    otherwise shared memory segments could remain after the process exits.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    KPixCanvasImageCache cache;
    unsigned int count;
    uaddr im_red_offset, im_green_offset, im_blue_offset;
    unsigned char *ub_ptr;
    unsigned char ub_pixels[256];
    XImage *ximage;
    static char function_name[] = "__kwin_X11_draw_pc_image";

    long compute_time, dump_time;
    struct timeval start_time;
    struct timeval stop_time;
    static struct timezone tz = {0, 0};

    VERIFY_CANVAS (x11canvas);
    if ( ( cache = size_cache (x11canvas, cache_ptr, x_pixels, y_pixels) )
	== NULL )
    {
	return (FALSE);
    }
    ximage = (cache->ximage ==NULL) ? x11canvas->common_ximage : cache->ximage;
    if (ximage->depth == 24)
    {
	if (gettimeofday (&start_time, &tz) != 0)
	{
	    (void) fprintf (stderr, "Error getting time of day\n");
	    (void) exit (1);
	}
    }
    /*  Now have an XImage big enough  */
    if (num_pixels <= 256)
    {
	/*  Convert pixel values  */
	for (count = 0; count < num_pixels; ++count)
	{
	    if (pixel_values[count] > 255)
	    {
		(void) fprintf (stderr, "Pixel value: %lu  over 255\n",
				pixel_values[count]);
		return (FALSE);
	    }
	    ub_pixels[count] = pixel_values[count];
	}
    }
    if ( (width == x_pixels) && (height == y_pixels) && (ximage->depth == 8) )
    {
	/*  New drawing routines  */
	/*  Note the casts from (uaddr *) to (iaddr *) for the offset
	    arrays. This is dodgy, but it should work.  */
	if ( !imw_to8_oi ( (unsigned char *) ximage->data, 1,
			  ximage->bytes_per_line,
			  x_pixels, y_pixels,
			  slice, (iaddr *) hoffsets, (iaddr *) voffsets,
			  type, conv_type,
			  num_pixels, ub_pixels,
			  blank_pixel, min_sat_pixel, max_sat_pixel,
			  i_min, i_max, iscale_func, iscale_info ) )
	{
	    (void) fprintf (stderr, "Error drawing image into X11Canvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
    }
    else if ( (width == x_pixels) && (height == y_pixels) &&
	     (ximage->depth == 24) && (num_pixels == 65536) &&
	     (type == K_USHORT) && (0 == (int) i_min) &&
	     (65535 == (int) i_max) )
    {
	ub_ptr = (unsigned char *) pixel_values;
	kwin_get_attributes (x11canvas->pixcanvas,
			     KWIN_ATT_IM_RED_OFFSET, &im_red_offset,
			     KWIN_ATT_IM_GREEN_OFFSET, &im_green_offset,
			     KWIN_ATT_IM_BLUE_OFFSET, &im_blue_offset,
			     KWIN_ATT_END);
	if ( !imw_scmap_16to24_o ( (unsigned char *) ximage->data + im_red_offset,
				  (unsigned char *) ximage->data + im_green_offset,
				  (unsigned char *) ximage->data + im_blue_offset,
				  FALSE, 4, ximage->bytes_per_line,
				  x_pixels, y_pixels,
				  (CONST unsigned short *) slice,
				  (iaddr *) hoffsets, (iaddr *) voffsets,
				  ub_ptr + im_red_offset,
				  ub_ptr + im_green_offset,
				  ub_ptr + im_blue_offset,
				  sizeof *pixel_values) )
	{
	    (void) fprintf (stderr, "Error drawing image into X11Canvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
    }
    else if (ximage->depth == 8)
    {
	if ( !imw_to8_lossy ( (unsigned char *) ximage->data, 1,
			     ximage->bytes_per_line,
			     x_pixels, y_pixels,
			     slice, (iaddr *) hoffsets, (iaddr *) voffsets,
			     (int) width, (int) height, type, conv_type,
			     num_pixels, ub_pixels,
			     blank_pixel, min_sat_pixel, max_sat_pixel,
			     i_min, i_max, iscale_func, iscale_info ) )
	{
	    (void) fprintf (stderr, "Error drawing image into X11Canvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
    }
    else
    {
	(void) fprintf (stderr,
			"%s: general drawing request not implemented\n",
			function_name);
	kwin_free_cache_data (cache);
	if (cache_ptr != NULL) *cache_ptr = NULL;
	return (FALSE);
    }
    if (ximage->depth == 24)
    {
	if (gettimeofday (&stop_time, &tz) != 0)
	{
	    (void) fprintf (stderr, "Error getting time of day");
	    (void) exit (1);
	}
	compute_time = 1000 * (stop_time.tv_sec - start_time.tv_sec);
	compute_time += (stop_time.tv_usec - start_time.tv_usec) / 1000;
	m_copy ( (char *) &start_time, (char *) &stop_time,sizeof start_time );
    }
    cache->width = x_pixels;
    cache->height = y_pixels;
    if (cache->pixmap == (Pixmap) NULL)
    {
	/*  No pixmap copy: just dump the image  */
	xi_put_image (x11canvas->display, x11canvas->window, x11canvas->gc,
		      ximage, 0, 0, x_off, y_off, cache->width, cache->height,
		      cache->shared, TRUE);
    }
    else
    {
	/*  First copy to pixmap and then copy from pixmap to screen. Copying
	    to screen then to pixmap is bad if the screen is obscured.  */
	xi_put_image (x11canvas->display, cache->pixmap, x11canvas->gc,
		      ximage, 0, 0, 0, 0, cache->width, cache->height,
		      cache->shared, TRUE);
	XCopyArea (x11canvas->display, cache->pixmap, x11canvas->window,
		   x11canvas->gc,
		   0, 0, cache->width, cache->height, x_off, y_off);
    }
    if (ximage->depth == 24)
    {
	if (gettimeofday (&stop_time, &tz) != 0)
	{
	    (void) fprintf (stderr, "Error getting time of day\n");
	    (void) exit (1);
	}
	dump_time = 1000 * (stop_time.tv_sec - start_time.tv_sec);
	dump_time += (stop_time.tv_usec - start_time.tv_usec) / 1000;
	(void) fprintf(stderr,
		       "image compute time(ms): %d\timage dump time(ms): %d\n",
		       compute_time, dump_time);
    }
/*
    XBell (canvas->display, 100);
*/
    if (cache_ptr == NULL) kwin_free_cache_data (cache);
    else *cache_ptr = cache;
    return (TRUE);
}   /*  End Function draw_pc_image  */

static flag draw_rgb_image (X11Canvas x11canvas,
			    int x_off, int y_off, int x_pixels, int y_pixels,
			    CONST unsigned char *red_slice,
			    CONST unsigned char *green_slice,
			    CONST unsigned char *blue_slice,
			    CONST uaddr *hoffsets, CONST uaddr *voffsets,
			    unsigned int width, unsigned int height,
			    KPixCanvasImageCache *cache_ptr)
/*  [PURPOSE] This routine will draw a 2-dimensional slice of a Karma array
    onto an X11 canvas. This slice may be tiled. The slice is a RGB image.
    <x11canvas> The X11 canvas.
    <x_off> The horizontal offset, relative to the top-left corner of the
    canvas.
    <y_off> The vertical offset, relative to the top-left corner of the canvas.
    <x_pixels> The number of horizontal pixels to draw.
    <y_pixels> The number of vertical pixels to draw.
    <red_slice> The start of the red slice data.
    <green_slice> The start of the green slice data.
    <blue_slice> The start of the blue slice data.
    [NOTE] The 3 colour components must be of type  K_UBYTE  .
    <hoffsets> The array of horizontal byte offsets.
    <voffsets> The array of vertical byte offsets.
    <width> The width of the input image (in values).
    <height> The height of the input image (in values).
    <cache_ptr> Cache data is written here. The routine may produce cache data
    which will vastly increase the speed of subsequent operations on this data.
    Prior to process exit, a call MUST be made to <<kwin_free_cache_data>>,
    otherwise shared memory segments could remain after the process exits.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    KPixCanvasImageCache cache;
    iaddr pstride;
    uaddr im_red_offset, im_green_offset, im_blue_offset;
    unsigned int count;
    unsigned char *out_image;
    XImage *ximage;
    unsigned char pixels[256];
    static char function_name[] = "__kwin_X11_draw_rgb_image";

    VERIFY_CANVAS (x11canvas);
    if ( ( cache = size_cache (x11canvas, cache_ptr, x_pixels, y_pixels) )
	== NULL )
    {
	return (FALSE);
    }
    kwin_get_attributes (x11canvas->pixcanvas,
			 KWIN_ATT_IM_RED_OFFSET, &im_red_offset,
			 KWIN_ATT_IM_GREEN_OFFSET, &im_green_offset,
			 KWIN_ATT_IM_BLUE_OFFSET, &im_blue_offset,
			 KWIN_ATT_END);
    ximage = (cache->ximage ==NULL) ? x11canvas->common_ximage : cache->ximage;
    /*  Now have an XImage big enough  */
    pstride = ximage->bits_per_pixel / 8;  /*  Typically 4  */
    if (pstride != 4)
    {
	(void) fprintf (stderr, "%s: pstride: %ld\n", function_name, pstride);
    }
    out_image = (unsigned char *) ximage->data;
    /*  Set up pixel value array  */
    for (count = 0; count < 256; ++count) pixels[count] = count;
    /*  Note the casts from (uaddr *) to (iaddr *) for the offset
	arrays. This is dodgy, but it should work.  */
    if ( (x_pixels == width) && (y_pixels == height) )
    {
	/*  This is what I like to see  */
	if ( !imw_to8_o (out_image + im_red_offset, pstride,
			 ximage->bytes_per_line,
			 x_pixels, y_pixels, (CONST char *) red_slice,
			 (iaddr *) hoffsets, (iaddr *) voffsets,
			 K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			 256, pixels, 0, 0, 255, 0.0, 255.0) )
	{
	    (void) fprintf (stderr,
			    "Error drawing red image into X11Canvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
	if ( !imw_to8_o (out_image + im_green_offset, pstride,
			 ximage->bytes_per_line,
			 x_pixels, y_pixels, (CONST char *) green_slice,
			 (iaddr *) hoffsets, (iaddr *) voffsets,
			 K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			 256, pixels, 0, 0, 255, 0.0, 255.0) )
	{
	    (void) fprintf (stderr,
			    "Error drawing green image into X11Canvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
	if ( !imw_to8_o (out_image + im_blue_offset, pstride,
			 ximage->bytes_per_line,
			 x_pixels, y_pixels, (CONST char *) blue_slice,
			 (iaddr *) hoffsets, (iaddr *) voffsets,
			 K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			 256, pixels, 0, 0, 255, 0.0, 255.0) )
	{
	    (void) fprintf (stderr,
			    "Error drawing blue image into X11Canvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
    }
    else
    {
	/*  But we can cope with differing input/output image sizes  */
	if ( !imw_to8_lossy (out_image + im_red_offset, pstride,
			     ximage->bytes_per_line,
			     x_pixels, y_pixels, (CONST char *) red_slice,
			     (iaddr *) hoffsets, (iaddr *) voffsets,
			     width, height,
			     K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			     256, pixels, 0, 0, 255, 0.0, 255.0,
			     ( flag (*) () ) NULL, NULL) )
	{
	    (void) fprintf (stderr,
			    "Error drawing red image into X11Canvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
	if ( !imw_to8_lossy (out_image + im_green_offset, pstride,
			     ximage->bytes_per_line,
			     x_pixels, y_pixels, (CONST char *) green_slice,
			     (iaddr *) hoffsets, (iaddr *) voffsets,
			     width, height,
			     K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			     256, pixels, 0, 0, 255, 0.0, 255.0,
			     ( flag (*) () ) NULL, NULL) )
	{
	    (void) fprintf (stderr,
			    "Error drawing green image into X11Canvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
	if ( !imw_to8_lossy (out_image + im_blue_offset, pstride,
			     ximage->bytes_per_line,
			     x_pixels, y_pixels, (CONST char *) blue_slice,
			     (iaddr *) hoffsets, (iaddr *) voffsets,
			     width, height,
			     K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			     256, pixels, 0, 0, 255, 0.0, 255.0,
			     ( flag (*) () ) NULL, NULL) )
	{
	    (void) fprintf (stderr,
			    "Error drawing blue image into X11Canvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
    }
    cache->width = x_pixels;
    cache->height = y_pixels;
    if (cache->pixmap == (Pixmap) NULL)
    {
	/*  No pixmap copy: just dump the image  */
	xi_put_image (x11canvas->display, x11canvas->window, x11canvas->gc,
		      ximage, 0, 0, x_off, y_off, cache->width, cache->height,
		      cache->shared, TRUE);
    }
    else
    {
	/*  First copy to pixmap and then copy from pixmap to screen. Copying
	    to screen then to pixmap is bad if the screen is obscured.  */
	xi_put_image (x11canvas->display, cache->pixmap, x11canvas->gc,
		      ximage, 0, 0, 0, 0, cache->width, cache->height,
		      cache->shared, TRUE);
	XCopyArea (x11canvas->display, cache->pixmap, x11canvas->window,
		   x11canvas->gc,
		   0, 0, cache->width, cache->height, x_off, y_off);
    }
/*
    XBell (canvas->display, 100);
*/
    if (cache_ptr == NULL) kwin_free_cache_data (cache);
    else *cache_ptr = cache;
    return (TRUE);
}   /*  End Function draw_rgb_image  */

static flag draw_cached_image (KPixCanvasImageCache cache, int x_off,int y_off)
/*  [PURPOSE] This routine will draw a previously computed image cache data
    (computed by <<kwin_draw_pc_image>>) onto the canvas which the original
    image was drawn.
    <cache> The cache data.
    <x_off> The horizontal offset, relative to the top-left corner of the
    canvas.
    <y_off> The vertical offset, relative to the top-left corner of the canvas.
    [RETURNS] TRUE on success if there is valid cache data, else FALSE
    indicating that the image must be recomputed and drawn using
    <<kwin_draw_pc_image>>.
*/
{
    X11Canvas x11canvas;
    static char function_name[] = "__kwin_X11_draw_cached_image";

    if (cache == NULL) return (FALSE);
    if (cache->magic_number != CACHE_DATA_MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Invalid cache data\n");
	a_prog_bug (function_name);
    }
    x11canvas = cache->x11canvas;
    if ( (cache->ximage != NULL) && cache->shared )
    {
	xi_put_image (x11canvas->display, x11canvas->window, x11canvas->gc,
		      cache->ximage,
		      0, 0, x_off, y_off, cache->width, cache->height,
		      cache->shared, TRUE);
	return (TRUE);
    }
    if (cache->pixmap != (Pixmap) NULL)
    {
	XCopyArea (x11canvas->display, cache->pixmap, x11canvas->window,
		   x11canvas->gc,
		   0, 0, cache->width, cache->height, x_off, y_off);
	return (TRUE);
    }
    if (cache->ximage == NULL) return (FALSE);
    /*  Call it again with non-pixmap, non-shared XImage  */
    xi_put_image (x11canvas->display, x11canvas->window, x11canvas->gc,
		  cache->ximage,
		  0, 0, x_off, y_off, cache->width, cache->height,
		  cache->shared, TRUE);
/*
    XBell (canvas->display, 100);
*/
    return (TRUE);
}   /*  End Function draw_cached_image  */

static void free_cache_data (KPixCanvasImageCache cache)
/*  [PURPOSE] This routine will free some cache data allocated by
    <<kwin_draw_pc_image>>.
    <cache> The cache data.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "__kwin_X11_free_cache_data";

    if (cache == NULL) return;
    if (cache->magic_number != CACHE_DATA_MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Invalid cache data\n");
	a_prog_bug (function_name);
    }
    if (cache->ximage != NULL)
    {
	xi_destroy_image (cache->x11canvas->display, cache->ximage,
			  cache->shared);
    }
    if (cache->pixmap != (Pixmap) NULL)
    {
	XFreePixmap (cache->x11canvas->display, cache->pixmap);
    }
    cache->magic_number = 0;
    m_free ( (char *) cache );
}   /*  End Function free_cache_data  */

static flag draw_line (X11Canvas x11canvas,
		       double x0, double y0, double x1, double y1,
		       unsigned long pixel_value)
/*  [PURPOSE] This routine will draw a line on an X11 canvas.
    <x11canvas> The X11 canvas.
    <x0> The horizontal offset of the first point.
    <y0> The vertical offset of the first point.
    <x1> The horizontal offset of the second point.
    <y1> The vertical offset of the second point.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "__kwin_X11_draw_line";

    VERIFY_CANVAS (x11canvas);
    set_pixel_in_gc (x11canvas, pixel_value);
    XDrawLine (x11canvas->display, x11canvas->window, x11canvas->gc,
	       (int) x0, (int) y0, (int) x1, (int) y1);
}   /*  End Function draw_line  */

static flag draw_arc (X11Canvas x11canvas,
		      double x, double y, double width, double height,
		      int angle1, int angle2, unsigned long pixel_value,
		      flag fill)
/*  [PURPOSE] This routine will draw an arc on an X11 canvas.
    <x11canvas> The X11 canvas.
    <x> The horizontal co-orinate of the arc.
    <y> The vertical co-ordinate of the arc.
    <width> The width of the arc.
    <height> The height of the arc.
    <angle1> See <<XDrawArc>>.
    <angle2> See <<XDrawArc>>.
    <pixel_value> The pixel value to use.
    <fill> If TRUE, the arc is filled, else only the outside is drawn.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "__kwin_X11_draw_arc";

    VERIFY_CANVAS (x11canvas);
    FLAG_VERIFY (fill);
    set_pixel_in_gc (x11canvas, pixel_value);
    if (fill)
    {
	XFillArc (x11canvas->display, x11canvas->window, x11canvas->gc,
		  (int) x, (int) y, (int) width, (int) height, angle1, angle2);
    }
    else
    {
	XDrawArc (x11canvas->display, x11canvas->window, x11canvas->gc,
		  (int) x, (int) y, (int) width, (int) height, angle1, angle2);
    }
    return (TRUE);
}   /*  End Function draw_arc  */

static flag draw_polygon (X11Canvas x11canvas, double *x_arr, double *y_arr,
			  unsigned int num_vertices, unsigned long pixel_value,
			  flag convex, flag fill)
/*  [PURPOSE] This routine will draw a polygon onto an X11 canvas.
    <x11canvas> The X11 canvas.
    <point_x> The array of x co-ordinates of vertices of the polygon.
    <point_y> The array of y co-ordinates of vertices of the polygon.
    <num_vertices> The number of vertices in the polygon.
    <pixel_value> The pixel value to use.
    <convex> If TRUE, then the points must form a convex polygon.
    <fill> If TRUE, the polygon will be filled.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int coord_count;
    static unsigned int num_points_allocated = 0;
    static XPoint *points = NULL;
    static char function_name[] = "__kwin_X11_draw_polygon";

    VERIFY_CANVAS (x11canvas);
    FLAG_VERIFY (convex);
    FLAG_VERIFY (fill);
    if (!fill)
    {
	(void) fprintf (stderr, "%s: unfilled polygons not supported\n",
			function_name);
	return (FALSE);
    }
    if (num_vertices > num_points_allocated)
    {
	if (points != NULL)
	{
	    m_free ( (char *) points );
	    points = NULL;
	}
	num_points_allocated = 0;
	if ( ( points = (XPoint *)
	      m_alloc (sizeof *points * num_vertices) ) == NULL )
	{
	    m_error_notify (function_name, "array of XPoint structures");
	    return (FALSE);
	}
	num_points_allocated = num_vertices;
    }
    set_pixel_in_gc (x11canvas, pixel_value);
    /*  Convert world co-ordinates to pixel co-ordinates  */
    for (coord_count = 0; coord_count < num_vertices; ++coord_count)
    {
	points[coord_count].x = (int) x_arr[coord_count];
	points[coord_count].y = (int) y_arr[coord_count];
    }
    XFillPolygon (x11canvas->display, x11canvas->window, x11canvas->gc,
		  points, (int) num_vertices, convex ? Convex : Complex,
		  CoordModeOrigin);
    return (TRUE);
}   /*  End Function draw_polygon  */

static flag draw_string (X11Canvas x11canvas, double x, double y,
			 CONST char *string, unsigned long pixel_value,
			 flag clear_under)
/*  [PURPOSE] This routine will draw a NULL terminated string onto an X11
    canvas, using the default font for the canvas.
    <canvas> The X11 canvas.
    <x> The horizontal offset of the string origin.
    <y> The vertical offset of the string origin.
    <string> The string.
    <pixel_value> The pixel value to use.
    <clear_under> If TRUE, then the routine will draw both the foreground and
    background of the characters.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int length;
    static char function_name[] = "__kwin_X11_draw_string";

    VERIFY_CANVAS (x11canvas);
    FLAG_VERIFY (clear_under);
    set_pixel_in_gc (x11canvas, pixel_value);
    length = strlen (string);
    if (clear_under)
    {
	XDrawImageString (x11canvas->display, x11canvas->window, x11canvas->gc,
			  (int) x, (int) y, string, length);
    }
    else
    {
	XDrawString (x11canvas->display, x11canvas->window, x11canvas->gc,
		     (int) x, (int) y, string, length);
    }
    return (TRUE);
}   /*  End Function draw_string  */

static flag draw_rectangle (X11Canvas x11canvas,
			    double x, double y, double width, double height,
			    unsigned long pixel_value, flag fill)
/*  [PURPOSE] This routine will draw a single rectangle onto an X11 canvas.
    <x11canvas> The X11 canvas.
    <x> The horizontal offset of the rectangle.
    <y> The vertical offset of the rectangle.
    <width> The width of the rectangle.
    <height> The height of the rectangle.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "__kwin_X11_draw_rectangle";

    VERIFY_CANVAS (x11canvas);
    FLAG_VERIFY (fill);
    set_pixel_in_gc (x11canvas, pixel_value);
    if (fill)
    {
	XFillRectangle (x11canvas->display, x11canvas->window, x11canvas->gc,
			(int) x, (int) y,
			(unsigned int) width + 1, (unsigned int) height + 1);
    }
    else
    {
	XDrawRectangle (x11canvas->display, x11canvas->window, x11canvas->gc,
			(int) x, (int) y,
			(unsigned int) width, (unsigned int) height);
    }
    return (TRUE);
}   /*  End Function draw_rectangle  */

static flag draw_lines (X11Canvas x11canvas, double *x_arr, double *y_arr,
			unsigned int num_points, unsigned long pixel_value)
/*  [PURPOSE] This routine will draw multiple connected lines onto an X11
    canvas.
    <x11canvas> The X11 canvas.
    <x_array> The horizontal co-ordinates of the points.
    <y_array> The vetical co-ordinates of the points.
    <num_points> The number of points. The number of lines drawn is 1 less than
    this value.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int ipoint_count, xpoint_count;
    extern int num_xpoints_allocated;
    extern XPoint *xpoints;
    static char function_name[] = "__kwin_X11_draw_lines";

    VERIFY_CANVAS (x11canvas);
    set_pixel_in_gc (x11canvas, pixel_value);
    alloc_points (x11canvas, num_points);
    for (ipoint_count = 0, xpoint_count = 0; ipoint_count < num_points;
	 ++ipoint_count, ++xpoint_count)
    {
	if (xpoint_count >= num_xpoints_allocated)
	{
	    /*  Send some points now  */
	    XDrawLines (x11canvas->display, x11canvas->window, x11canvas->gc,
			xpoints, xpoint_count, CoordModeOrigin);
	    xpoints[0].x = (int) x_arr[ipoint_count - 1];
	    xpoints[0].y = (int) y_arr[ipoint_count - 1];
	    xpoint_count = 1;
	}
	xpoints[xpoint_count].x = (int) x_arr[ipoint_count];
	xpoints[xpoint_count].y = (int) y_arr[ipoint_count];
    }
    /*  Draw remaining points  */
    if (xpoint_count > 0)
    {
	XDrawLines (x11canvas->display, x11canvas->window, x11canvas->gc,
		    xpoints, xpoint_count, CoordModeOrigin);
    }
    return (TRUE);
}   /*  End Function draw_lines  */

static flag draw_arcs (X11Canvas x11canvas,
		       double *x, double *y, double *width, double *height,
		       int *angle1, int *angle2, unsigned int num_ellipses,
		       unsigned long pixel_value, flag fill)
/*  [PURPOSE] This routine will draw multiple arcs onto an X11 canvas.
    <x11canvas> The X11 canvas.
    <x> The array of horizontal co-orinates of the arcs.
    <y> The array of vertical co-ordinates of the arcs.
    <width> The array of widths of the arcs.
    <height> The array of heights of the arcs.
    <angle1> Another array. See <<XDrawArc>>.
    <angle2> Another array. See <<XDrawArc>>.
    <num_ellipses> The number of ellipses.
    <pixel_value> The pixel value to use.
    <fill> If TRUE, the arcs are filled, else only the outside is drawn.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int iarc_count, xarc_count;
    short angle = 64 * 360;
    extern int num_xarcs_allocated;
    extern XArc *xarcs;
    static char function_name[] = "__kwin_X11_draw_arcs";

    VERIFY_CANVAS (x11canvas);
    FLAG_VERIFY (fill);
    set_pixel_in_gc (x11canvas, pixel_value);
    alloc_arcs (x11canvas, num_ellipses);
    for (iarc_count = 0, xarc_count = 0; iarc_count < num_ellipses;
	 ++iarc_count, ++xarc_count)
    {
	if (xarc_count >= num_xarcs_allocated)
	{
	    /*  Send some arcs now  */
	    if (fill) XFillArcs (x11canvas->display, x11canvas->window,
				 x11canvas->gc, xarcs, xarc_count);
	    else XDrawArcs (x11canvas->display, x11canvas->window,
			    x11canvas->gc, xarcs, xarc_count);
	    xarc_count = 0;
	}
	xarcs[xarc_count].x = (short) x[iarc_count];
	xarcs[xarc_count].y = (short) y[iarc_count];
	xarcs[xarc_count].width = (unsigned short) width[iarc_count];
	xarcs[xarc_count].height = (unsigned short) height[iarc_count];
	xarcs[xarc_count].angle1 = (short) angle1[iarc_count];
	xarcs[xarc_count].angle2 = (short) angle2[iarc_count];
    }
    /*  Draw remaining arcs  */
    if (xarc_count > 0)
    {
	if (fill) XFillArcs (x11canvas->display, x11canvas->window,
			     x11canvas->gc, xarcs, xarc_count);
	else XDrawArcs (x11canvas->display, x11canvas->window, x11canvas->gc,
			xarcs, xarc_count);
    }
    return (TRUE);
}   /*  End Function draw_arcs  */

static flag draw_segments (X11Canvas x11canvas,
			   double *x0, double *y0, double *x1, double *y1,
			   unsigned int num_segments,
			   unsigned long pixel_value)
/*  [PURPOSE] This routine will draw multiple disjoint lines onto an X11
    canvas.
    <x11canvas> The X11 canvas.
    <x0> The horizontal start co-ordinates of the segments.
    <y0> The vetical start co-ordinates of the segments.
    <x1> The horizontal end co-ordinates of the segments.
    <y1> The vetical end co-ordinates of the segments.
    <num_segments> The number of segments.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int isegment_count, xsegment_count;
    extern int num_xsegments_allocated;
    extern XSegment *xsegments;
    static char function_name[] = "__kwin_X11_draw_segments";

    VERIFY_CANVAS (x11canvas);
    set_pixel_in_gc (x11canvas, pixel_value);
    alloc_segments (x11canvas, num_segments);
    for (isegment_count = 0, xsegment_count = 0;
	 isegment_count < num_segments;
	 ++isegment_count, ++xsegment_count)
    {
	if (xsegment_count >= num_xsegments_allocated)
	{
	    /*  Send some segments now  */
	    XDrawSegments (x11canvas->display, x11canvas->window,x11canvas->gc,
			   xsegments, xsegment_count);
	    xsegment_count = 0;
	}
	xsegments[xsegment_count].x1 = x0[isegment_count];
	xsegments[xsegment_count].y1 = y0[isegment_count];
	xsegments[xsegment_count].x2 = x1[isegment_count];
	xsegments[xsegment_count].y2 = y1[isegment_count];
    }
    /*  Draw remaining segments  */
    if (xsegment_count > 0)
    {
	XDrawSegments (x11canvas->display, x11canvas->window, x11canvas->gc,
		       xsegments, xsegment_count);
    }
    return (TRUE);
}   /*  End Function draw_segments  */

static flag get_colour (X11Canvas x11canvas, CONST char *colourname,
			unsigned long *pixel_value, unsigned short *red,
			unsigned short *green, unsigned short *blue)
/*  [PURPOSE] This routine will allocate a colour for an X11 canvas.
    <x11canvas> The X11 canvas.
    <colourname> The name of the colour.
    <pixel_value> The pixel value will be written here.
    [NOTE] The pixel value is valid ONLY for this canvas. ALSO: the pixel value
    becomes invalid EVERY time the canvas is refreshed/ resized. If the canvas
    is refreshed/ resized, this routine MUST be called again.
    <red> The red intensity in the hardware colourmap for the pixel will be
    written here. If this is NULL, nothing is written here.
    <green> The green intensity in the hardware colourmap for the pixel will be
    written here. If this is NULL, nothing is written here.
    <blue> The blue intensity in the hardware colourmap for the pixel will be
    written here. If this is NULL, nothing is written here.
    [RETURNS] TRUE if the colourcell was allocated, else FALSE.
*/
{
    XColor colourcell_def;
    XColor rgb_db_def;
    struct colourcell_type *colour;
    static char function_name[] = "__kwin_X11_get_colour";

    VERIFY_CANVAS (x11canvas);
    if (colourname == NULL)
    {
	(void) fprintf (stderr, "NULL colourname pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Search canvas for existing allocation  */
    for (colour = x11canvas->colours; colour != NULL; colour = colour->next)
    {
	if (strcmp (colourname, colour->name) == 0)
	{
	    *pixel_value = colour->pixel_value;
	    if (red != NULL) *red = colour->red;
	    if (green != NULL) *green = colour->green;
	    if (blue != NULL) *blue = colour->blue;
	    return (TRUE);
	}
    }
    /*  Need to allocate a new colour  */
    if ( ( colour = (struct colourcell_type *) m_alloc (sizeof *colour) )
	== NULL )
    {
	m_abort (function_name, "colourcell structure");
    }
    if ( ( colour->name = st_dup (colourname) ) == NULL )
    {
	m_abort (function_name, "colour name");
    }
    if (XAllocNamedColor (x11canvas->display, x11canvas->cmap, colourname,
			  &colourcell_def, &rgb_db_def) == 0)
    {
	(void) fprintf (stderr, "Error allocating colour: \"%s\"\n",
			colourname);
	m_free ( colour->name );
	m_free ( (char *) colour );
	return (FALSE);
    }
    colour->pixel_value = colourcell_def.pixel;
    *pixel_value = colour->pixel_value;
    if (red != NULL) *red = colourcell_def.red;
    if (green != NULL) *green = colourcell_def.green;
    if (blue != NULL) *blue = colourcell_def.blue;
    colour->next = x11canvas->colours;
    x11canvas->colours = colour;
    return (TRUE);
}   /*  End Function get_colour  */

static XFontStruct *load_font (X11Canvas x11canvas, CONST char *fontname)
/*  [PURPOSE] This routine will load an X11 font.
    <x11canvas> The X11 canvas for which the font is valid.
    <fontname> The fontname.
    [RETURNS] A pointer to the X11 font structure.
*/
{
    XFontStruct *x11font;
    static char function_name[] = "__kwin_X11_load_font";

    VERIFY_CANVAS (x11canvas);
    if ( ( x11font = XLoadQueryFont (x11canvas->display, fontname) ) == NULL )
    {
	(void) fprintf (stderr, "Error loading font: \"%s\"\n", fontname);
	return (NULL);
    }
    return (x11font);
}   /*  End Function load_font  */

static flag get_string_size (XFontStruct *x11font, CONST char *string,
			     va_list argp)
/*  [PURPOSE] This routine will get the size of a string.
    <x11font> The pointer to the X11 font structure.
    <string> The string.
    [VARARGS] The optional list of parameter attribute-key attribute-value
    pairs must follow. See the header file for details on defined attributes.
    This list must be terminated with the value  KWIN_STRING_END  .
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int length;
    unsigned int att_key;
    static char function_name[] = "__kwin_X11_get_string_size";

    length = strlen (string);
    while ( ( att_key = va_arg (argp, unsigned int) ) != KWIN_STRING_END )
    {
	switch (att_key)
	{
	  case KWIN_STRING_WIDTH:
	    *( va_arg (argp, int *) ) = XTextWidth (x11font, string, length);
	    break;
	  case KWIN_STRING_HEIGHT:
	    *( va_arg (argp, int *) ) = x11font->ascent + x11font->descent;
	    break;
	  case KWIN_STRING_ASCENT:
	    *( va_arg (argp, int *) ) = x11font->ascent;
	    break;
	  case KWIN_STRING_DESCENT:
	    *( va_arg (argp, int *) ) = x11font->descent;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    return (TRUE);
}   /*  End Function get_string_size  */

static void set_font (X11Canvas x11canvas, XFontStruct *x11font)
/*  [PURPOSE] This routine will set the font for an X11 canvas.
    <x11canvas> The X11 canvas.
    <x11font> The X11 font.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "__kwin_X11_set_font";

    VERIFY_CANVAS (x11canvas);
    if (x11canvas->gcvalues.font != x11font->fid)
    {
	x11canvas->gcvalues.font = x11font->fid;
	XChangeGC (x11canvas->display, x11canvas->gc, GCFont,
		   &x11canvas->gcvalues);
    }
}   /*  End Function set_x11font  */

static flag query_colourmap (X11Canvas x11canvas, unsigned long *pixels,
			     unsigned short *reds, unsigned short *greens,
			     unsigned short *blues, unsigned int num_colours)
/*  [PURPOSE] This routine will determine the RGB components of an array of
    colourmap entries.
    <canvas> The X11  canvas.
    <pixels> The array of colourmap indices.
    <reds> The red components will be written to this array.
    <greens> The green components will be written to this array.
    <blues> The blue components will be written to this array.
    <num_colours> The number of colours in the arrays.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int count;
    XColor *xcolours;
    static char function_name[] = "__kwin_X11_query_colourmap";

    VERIFY_CANVAS (x11canvas);
    if ( ( xcolours = (XColor *) m_alloc (num_colours * sizeof *xcolours) )
	== NULL )
    {
	m_error_notify (function_name, "XColor array");
	return (FALSE);
    }
    for (count = 0; count < num_colours; ++count)
    {
	xcolours[count].pixel = pixels[count];
    }
    XQueryColors (x11canvas->display, x11canvas->cmap, xcolours,
		  (int) num_colours);
    for (count = 0; count < num_colours; ++count)
    {
	reds[count] = xcolours[count].red;
	greens[count] = xcolours[count].green;
	blues[count] = xcolours[count].blue;
    }
    m_free ( (char *) xcolours );
    return (TRUE);
}   /*  End Function query_colourmap  */


/*  Private functions follow  */

static void set_pixel_in_gc (X11Canvas x11canvas, unsigned long pixel_value)
/*  [PURPOSE] This routine will set the foreground pixel value in the Graphics
    Context for an X11 canvas.
    <x11canvas> The X11 canvas.
    <pixel_value> The pixel value.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "__kwin_X11_set_pixel_in_gc";

    VERIFY_CANVAS (x11canvas);
    if (pixel_value != x11canvas->gcvalues.foreground)
    {
	x11canvas->gcvalues.foreground = pixel_value;
	XChangeGC (x11canvas->display, x11canvas->gc, GCForeground,
		   &x11canvas->gcvalues);
    }
}   /*  End Function set_pixel_in_gc  */

static XVisualInfo *get_visinfo_for_visual (Display *dpy, Visual *visual)
/*  [PURPOSE] This routine will get the visual information structure for a
    visual.
    <dpy> The X display.
    <visual> The visual.
    [RETURNS] A pointer to an XVisualInfo structure on succes, else NULL. The
    XVisualInfo structure must be freed by XFree()
*/
{
    int num_vinfos;
    XVisualInfo vinfo_template;
    XVisualInfo *vinfos;
    static char function_name[] = "__kwin_X11_get_visinfo_for_visual";

    vinfo_template.visualid = XVisualIDFromVisual (visual);
    vinfos = XGetVisualInfo (dpy, VisualIDMask, &vinfo_template, &num_vinfos);
    if (num_vinfos < 1)
    {
	(void) fprintf (stderr, "Error getting visual info for visual: %p\n",
			visual);
	a_prog_bug (function_name);
    }
    if (num_vinfos > 1)
    {
	(void)fprintf(stderr,
		      "%s: WARNING: number of visuals for visual: %p is: %d\n",
		      function_name, visual, num_vinfos);
    }
    return (vinfos);
}   /*  End Function get_visinfo_for_visual  */

static KPixCanvasImageCache size_cache (X11Canvas x11canvas,
					KPixCanvasImageCache *cache_ptr,
					unsigned int width,
					unsigned int height)
/*  [PURPOSE] This routine will allocate image cache data.
    <x11canvas> The X11 canvas.
    <cache_ptr> Cache data is written here. The routine may produce cache data
    which will vastly increase the speed of subsequent operations on this data.
    Prior to process exit, a call MUST be made to <<kwin_free_cache_data>>,
    otherwise shared memory segments could remain after the process exits.
    [RETURNS] A pointer to cache data on success, else NULL.
*/
{
    KPixCanvasImageCache cache;
    flag allocate = FALSE;
    static flag first_time = TRUE;
    static flag enable_pixmaps = TRUE;
    static char function_name[] = "__kwin_X11_size_cache";

    VERIFY_CANVAS (x11canvas);
    if (first_time)
    {
	first_time = FALSE;
	if (r_getenv ("KWIN_DISABLE_PIXMAPS") != NULL)
	{
	    enable_pixmaps = FALSE;
	    (void)fprintf(stderr,
			  "KWIN_DISABLE_PIXMAPS environment variable found\n");
	    (void) fprintf (stderr, "No pixmap image caching used.\n");
	}
    }
    if ( (cache_ptr == NULL) || (*cache_ptr == NULL) )
    {
	if ( ( cache = (KPixCanvasImageCache) m_alloc (sizeof *cache) )
	    == NULL )
	{
	    m_error_notify (function_name, "cache data structure");
	    return (NULL);
	}
	cache->pixcanvas = x11canvas->pixcanvas;
	cache->x11canvas = x11canvas;
	cache->magic_number = CACHE_DATA_MAGIC_NUMBER;
	cache->ximage = NULL;
	cache->pixmap = (Pixmap) NULL;
	cache->shared = FALSE;
	cache->width = 0;
	cache->height = 0;
	cache->p_width = 0;
	cache->p_height = 0;
    }
    else
    {
	cache = *cache_ptr;
    }
    if (cache->magic_number != CACHE_DATA_MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Invalid cache data\n");
	a_prog_bug (function_name);
    }
    /*  Check if existing XImage is big enough  */
    /*  Here we assume that if an XImage is big enough, everything is fine.
	This is of course always true for SHM XImages. For ordinary XImages,
	this is true if we make the assumption that there is no Pixmap.  */
    if (cache->ximage != NULL)
    {
	if ( (width <= cache->ximage->width) &&
	    (height <= cache->ximage->height) )
	{
	    /*  XImage is big enough: great!  */
	    if (cache->pixmap != (Pixmap) NULL)
	    {
		(void) fprintf (stderr, "Cache has XImage and Pixmap!\n");
		a_prog_bug (function_name);
	    }
	    return (cache);
	}
	/*  XImage is too small  */
	xi_destroy_image (x11canvas->display, cache->ximage, cache->shared);
	cache->ximage = NULL;
	cache->shared = FALSE;
    }
    /*  No appropriate XImage  */
    if (x11canvas->shm_available)
    {
	/*  No XImage but SHM XImages supported: give it a go  */
	if ( ( cache->ximage = xi_create_shm_image (x11canvas->display,
						    x11canvas->vinfo.visual,
						    x11canvas->vinfo.depth,
						    width, height, FALSE) )
	    != NULL )
	{
	    /*  Created a SHM XImage  */
	    cache->shared = TRUE;
	    if (cache->pixmap != (Pixmap) NULL)
	    {
		XFreePixmap (x11canvas->display, cache->pixmap);
		cache->pixmap = (Pixmap) NULL;
	    }
	    return (cache);
	}
    }
    /*  Have to rely on Pixmaps or ordinary XImages  */
    if (cache->pixmap != (Pixmap) NULL)
    {
	/*  We have an existing Pixmap  */
	if ( (width > cache->p_width) || (height > cache->p_height) )
	{
	    /*  Pixmap is too small  */
	    XFreePixmap (x11canvas->display, cache->pixmap);
	    cache->pixmap = (Pixmap) NULL;
	}
    }
    /*  No appropriate Pixmap already: can we make one?  */
    if ( (cache->pixmap == (Pixmap) NULL) && enable_pixmaps )
    {
	cache->p_width = width;
	cache->p_height = height;
	cache->pixmap = XCreatePixmap (x11canvas->display, x11canvas->window,
				       width, height, x11canvas->vinfo.depth);
    }
    if (cache->pixmap != (Pixmap) NULL)
    {
	/*  Have a good Pixmap: is common XImage big enough?  */
	if ( (x11canvas->common_ximage != NULL) &&
	    ( (x11canvas->common_ximage->width < width) ||
	     (x11canvas->common_ximage->height < height) ) )
	{
	    xi_destroy_image (x11canvas->display, x11canvas->common_ximage, FALSE);
	    x11canvas->common_ximage = NULL;
	}
	if (x11canvas->common_ximage == NULL)
	{
	    cache->shared = FALSE;
	    if ( ( x11canvas->common_ximage =
		  xi_create_image (x11canvas->display, x11canvas->window,
				   width, height, &cache->shared) ) == NULL )
	    {
		kwin_free_cache_data (cache);
		if (cache_ptr != NULL) *cache_ptr = NULL;
		return (NULL);
	    }
	}
	/*  Have Pixmap and common XImage big enough  */
	return (cache);
    }
    /*  No Pixmap and no XImage  */
    cache->shared = FALSE;
    if ( ( cache->ximage = xi_create_image (x11canvas->display,
					    x11canvas->window, width, height,
					    &cache->shared) ) == NULL )
    {
	kwin_free_cache_data (cache);
	if (cache_ptr != NULL) *cache_ptr = NULL;
	return (NULL);
    }
    return (cache);
}   /*  End Function size_cache  */

static void alloc_points (X11Canvas x11canvas, int num_points)
/*  This routine will (re)allocate the array of XPoint structures.
    The canvas must be given by  canvas  .The  max_request_size  field is used.
    The number of points requested must be given by  num_points  .There is no
    guarantee that the number of points requested will be allocated.
    The routine returns nothing.
*/
{
    int max_points;
    extern int num_xpoints_allocated;
    extern XPoint *xpoints;
    static char function_name[] = "alloc_points";

    max_points = (x11canvas->max_request_size - 3) / 2 + 1;
    if (max_points > MAX_POINTS) max_points = MAX_POINTS;
    if (num_points > max_points) num_points = max_points;
    /*  Determine if XPoint array is big enough  */
    if (num_points > num_xpoints_allocated)
    {
	if (xpoints != NULL)
	{
	    m_free ( (char *) xpoints );
	    xpoints = NULL;
	}
	num_xpoints_allocated = 0;
	if ( ( xpoints = (XPoint *) m_alloc (sizeof *xpoints * num_points) )
	    == NULL )
	{
	    m_abort (function_name, "array of XPoint structures");
	}
	num_xpoints_allocated = num_points;
    }
}   /*  End Function alloc_points  */

static void alloc_arcs (X11Canvas x11canvas, int num_arcs)
/*  This routine will (re)allocate the array of XArc structures.
    The canvas must be given by  canvas  .The  max_request_size  field is used.
    The number of arcs requested must be given by  num_arcs  .There is no
    guarantee that the number of arcs requested will be allocated.
    The routine returns nothing.
*/
{
    int max_arcs;
    extern int num_xarcs_allocated;
    extern XArc *xarcs;
    static char function_name[] = "alloc_arcs";

    max_arcs = (x11canvas->max_request_size - 3) / 3;
    if (max_arcs > MAX_ARCS) max_arcs = MAX_ARCS;
    if (num_arcs > max_arcs) num_arcs = max_arcs;
    /*  Determine if XArc array is big enough  */
    if (num_arcs > num_xarcs_allocated)
    {
	if (xarcs != NULL)
	{
	    m_free ( (char *) xarcs );
	    xarcs = NULL;
	}
	num_xarcs_allocated = 0;
	if ( ( xarcs = (XArc *) m_alloc (sizeof *xarcs * num_arcs) ) == NULL )
	{
	    m_abort (function_name, "array of XArc structures");
	}
	num_xarcs_allocated = num_arcs;
    }
}   /*  End Function alloc_arcs  */

static void alloc_segments (X11Canvas x11canvas, int num_segments)
/*  This routine will (re)allocate the array of XSegment structures.
    The canvas must be given by  canvas  .The  max_request_size  field is used.
    The number of segments requested must be given by  num_segments  .There is
    no guarantee that the number of segments requested will be allocated.
    The routine returns nothing.
*/
{
    int max_segments;
    extern int num_xsegments_allocated;
    extern XSegment *xsegments;
    static char function_name[] = "alloc_segments";

    max_segments = (x11canvas->max_request_size - 3) / 2;
    if (max_segments > MAX_SEGMENTS) max_segments = MAX_SEGMENTS;
    if (num_segments > max_segments) num_segments = max_segments;
    /*  Determine if XSegment array is big enough  */
    if (num_segments > num_xsegments_allocated)
    {
	if (xsegments != NULL)
	{
	    m_free ( (char *) xsegments );
	    xsegments = NULL;
	}
	num_xsegments_allocated = 0;
	if ( ( xsegments = (XSegment *) m_alloc (sizeof *xsegments *
						 num_segments) ) == NULL )
	{
	    m_abort (function_name, "array of XSegment structures");
	}
	num_xsegments_allocated = num_segments;
    }
}   /*  End Function alloc_segments  */

#endif  /*  X11  */

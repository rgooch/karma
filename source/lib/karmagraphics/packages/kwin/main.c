/*LINTLIBRARY*/
/*  main.c

    This code provides KPixCanvas objects.

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

/*  This file contains all routines needed for manipulating a simple pixel
    canvas (window) independent of the graphics system in use.


    Written by      Richard Gooch   15-APR-1993

    Updated by      Richard Gooch   27-APR-1993

    Updated by      Richard Gooch   15-OCT-1993: Added
  kwin_convert_from_canvas_coord  routine.

    Updated by      Richard Gooch   20-NOV-1993: Added  kwin_draw_string  .

    Updated by      Richard Gooch   21-NOV-1993: Added  kwin_draw_rectangle
  and  kwin_fill_rectangle  .

    Updated by      Richard Gooch   23-NOV-1993: Fixed bug in
  kwin_free_cache_data  which passed NULL pointer to  xi_destroy_image  on
  failure to allocate XImage in  kwin_draw_image  .

    Updated by      Richard Gooch   26-NOV-1993: Added  kwin_draw_lines  and
  kwin_draw_arc  .

    Updated by      Richard Gooch   29-NOV-1993: Added colourcell caching
  support and  kwin_draw_*_c  routines.

    Updated by      Richard Gooch   2-DEC-1993: Removed  kwin_draw_*_c
  routines and added  kwin_get_colour  routine instead.

    Updated by      Richard Gooch   5-DEC-1993: Added tests for re-entry into
  kwin_resize  and  kwin_process_position_event  .

    Updated by      Richard Gooch   7-DEC-1993: Added  kwin_draw_string_font  .

    Updated by      Richard Gooch   11-DEC-1993: Added  kwin_draw_ellipses  and
  kwin_fill_ellipses  .

    Updated by      Richard Gooch   12-DEC-1993: Added  kwin_draw_segments  .

    Updated by      Richard Gooch   13-DEC-1993: Added test for tiled images in
  kwin_draw_image  to disable image display. Once tiling is fully supported,
  this test will be removed.

    Updated by      Richard Gooch   20-FEB-1994: Added support for VX display
  (create, resize, draw point, draw line and fill rectangle only).

    Updated by      Richard Gooch   21-FEB-1994: Fixed bug in
  kwin_draw_string_font  .

    Updated by      Richard Gooch   22-FEB-1994: Made call to assembler code
  for  kwin_fill_rectangle  .

    Updated by      Richard Gooch   27-FEB-1994: Added  kwin_clear  .

    Updated by      Richard Gooch   15-APR-1994: Added support for VX stereo
  display.

    Updated by      Richard Gooch   19-MAY-1994: Modified  kwin_get_colour  to
  also give RGB colour values.

    Updated by      Richard Gooch   21-MAY-1994: Created  kwin_write_ps  .

    Updated by       Richard Gooch   6-JUN-1994: Fixed bug in
  kwin_fill_rectangle  introduced with PostScript support.

    Updated by      Richard Gooch   26-JUL-1994: Added support for
  non-pseudocolour canvases and created  kwin_get_attributes  .

    Updated by      Richard Gooch   31-JUL-1994: Created  kwin_create_child  .

    Updated by      Richard Gooch   8-AUG-1994: Created  kwin_draw_rgb_image  .

    Updated by      Richard Gooch   9-AUG-1994: Created  kwin_set_attributes  .

    Updated by      Richard Gooch   11-AUG-1994: Fixed mask finding code in
  kwin_create_x  .

    Updated by      Richard Gooch   7-OCT-1994: Added extra parameters to
  kwin_clear  .

    Updated by      Richard Gooch   13-OCT-1994: Added pixmap caching of images
  for non-RGB images when running under X.

    Updated by      Richard Gooch   29-OCT-1994: Reduced colour query calls to
  X server in  kwin_draw_lines  .

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   20-NOV-1994: Created  kwin_load_font  and
  removed  kwin_draw_string_font  routine.

    Updated by      Richard Gooch   22-NOV-1994: Created
  kwin_unregister_position_event_func  .

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/kwin/main.c

    Updated by      Richard Gooch   28-NOV-1994: Made use of  c_  package.

    Updated by      Richard Gooch   15-DEC-1994: Fixed bug in  size_cache
  which sometimes resulted in pixmaps too small not being replaced.

    Updated by      Richard Gooch   2-JAN-1995: Took account of change to  imw_
  package.

    Last updated by Richard Gooch   20-JAN-1995: Uncommented call to XCopyArea
  in <kwin_draw_rgb_image>.


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <varargs.h>
#include <karma.h>
#define KWIN_INTERNAL
#include <karma_kwin.h>
#ifdef X11
#  include <karma_drw.h>
#  include <karma_xi.h>
#  include <X11/Xutil.h>
#endif
#ifdef OS_VXMVX
#  include <karma_psw_def.h>
#else
#  include <karma_psw.h>
#endif
#include <karma_imw.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_r.h>
#include <karma_c.h>


#define CANVAS_MAGIC_NUMBER (unsigned int) 245724438
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

/*  X specific defines follow  */
#ifdef X11
#  define MAX_POINTS 10000
#  define MAX_ARCS 10000
#  define MAX_SEGMENTS 10000
#  define MAX_INTENSITY 65535
#endif


/*  Structure declarations  */

struct pixcanvas_type
{
    unsigned int magic_number;
    int xoff;
    int yoff;
    int width;
    int height;
    KCallbackList refresh_list;
    KCallbackList position_list;
    struct colourcell_type *colours;
    unsigned int visual;
    unsigned int depth;
    flag visible;
    uaddr red_offset;        /*  These 3 are only for 24 bit visuals   */
    uaddr green_offset;      /*  Also, with X: only for image display  */
    uaddr blue_offset;
#ifdef X11
    Display *display;
    Window window;
    Colormap cmap;
    flag shm_available;
    GC gc;
    XGCValues gcvalues;
    long max_request_size;
#endif
#ifdef _VXMVX
    unsigned int vxvisual;
    unsigned int *frame_buffer;  /*  Pointer to start of frame buffer  */
    int screen_width;
    int screen_height;
    unsigned char *origin;       /*  Pointer to top-left of window     */
    unsigned long background;
#endif
    PostScriptPage pspage;
    KPixCanvasFont font;
};

struct cache_data_type
{
    unsigned int magic_number;
    KPixCanvas canvas;
#ifdef X11
    Display *display;
    unsigned int width;
    unsigned int height;
    XImage *ximage;
    flag shared;
    Pixmap pixmap;
    unsigned int p_width;
    unsigned int p_height;
#endif
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

struct child_type
{
    KPixCanvas child;
    flag absorb_events;
};

struct pixfont_type
{
    unsigned int magic_number;
#ifdef X11
    Display *display;
    XFontStruct *xfont;
#endif
};

struct position_struct
{
    int x;
    int y;
    unsigned int event_code;
    void *e_info;
};

/*  X specific structures  */
#ifdef X11
#endif

/*  X specific private data  */
#ifdef X11
static int num_xpoints_allocated = 0;
static XPoint *xpoints = NULL;
static int num_xarcs_allocated = 0;
static XArc *xarcs = NULL;
static int num_xsegments_allocated = 0;
static XSegment *xsegments = NULL;
#endif


/*  Local functions  */
void kwin_get_attributes ();
void kwin_set_attributes ();


/*  Private functions  */
STATIC_FUNCTION (KPixCanvas alloc_canvas, () );
STATIC_FUNCTION (KPixCanvasImageCache alloc_cache_data_struct,
		 (KPixCanvas canvas) );
STATIC_FUNCTION (flag child_position_event_func,
		 (KPixCanvas parent, int x, int y, unsigned int event_code,
		  void *event_info, void **f_info) );
STATIC_FUNCTION (KPixCanvasImageCache size_cache,
		 (KPixCanvas canvas, KPixCanvasImageCache *cache_ptr,
		  unsigned int width, unsigned int height) );
STATIC_FUNCTION (void not_implemented, (char *s) );
STATIC_FUNCTION (flag refresh_event_func,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );
STATIC_FUNCTION (flag position_event_func,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );


/*  X specific local functions  */
#ifdef X11
STATIC_FUNCTION (void alloc_points, (KPixCanvas canvas, int num_points) );
STATIC_FUNCTION (void alloc_arcs, (KPixCanvas canvas, int num_arcs) );
STATIC_FUNCTION (void alloc_segments, (KPixCanvas canvas, int num_segments) );
STATIC_FUNCTION (void get_colour_from_pixel, (KPixCanvas canvas,
					      XColor *colour_def) );
STATIC_FUNCTION (void get_colours_from_pixels,
		 (KPixCanvas canvas, XColor *colour_def,
		  unsigned int num_pixels) );
STATIC_FUNCTION (XVisualInfo *get_visinfo_for_visual,
		 (Display *dpy, Visual *visual) );
#endif


/*  VX specific assembler functions  */
#ifdef OS_VXMVX
EXTERN_FUNCTION (void asm__kwin_fill_rectangle,
		 (flag depth, unsigned char *origin, int width, int height,
		  unsigned int pixel_value, int pixel_stride,
		  int line_stride) );
#endif


/*  Public functions follow  */

/*  X specific public functions follow  */
#ifdef X11

/*PUBLIC_FUNCTION*/
KPixCanvas kwin_create_x (display, window, gc, xoff, yoff, width, height)
/*  This routine will create a pixel canvas, ready for drawing, from an X
    window. Note that the origin of a KPixCanvas is the upper-left corner.
    NOTE: this routine is only available with the X window system.
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
    XWindowAttributes window_attributes;
    int im_byte_order;
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
    if ( ( canvas = alloc_canvas () ) == NULL )
    {
	m_error_notify (function_name, "X pixel canvas");
	return (NULL);
    }
    canvas->display = display;
    canvas->window = window;
    canvas->cmap = window_attributes.colormap;
    canvas->gc = gc;
    /*  Get GCValues  */
    if (XGetGCValues (display, gc,
		      GCFunction | GCPlaneMask |
		      GCForeground | GCBackground | GCLineWidth | GCLineStyle |
		      GCCapStyle | GCJoinStyle | GCFillStyle | GCFillRule |
		      GCTile | GCStipple | GCTileStipXOrigin |
		      GCTileStipYOrigin | GCFont | GCSubwindowMode |
		      GCGraphicsExposures | GCClipXOrigin | GCClipYOrigin |
		      GCDashOffset | GCArcMode,
		      &canvas->gcvalues) == 0)
    {
	(void) fprintf (stderr, "Error getting GC values\n");
	m_free ( (char *) canvas );
	return (NULL);
    }
    canvas->xoff = xoff;
    canvas->yoff = yoff;
    canvas->width = width;
    canvas->height = height;
    canvas->shm_available = xi_check_shared_images_available (display);
    canvas->max_request_size = XMaxRequestSize (display);
    canvas->colours = NULL;
    vinfo = get_visinfo_for_visual (display, window_attributes.visual);
    switch (vinfo->class)
    {
	case DirectColor:
	case TrueColor:
	if (vinfo->colormap_size != 256)
	{
	    (void) fprintf (stderr,
			    "Colourmap size: %d for %s visual is not 256\n",
			    vinfo->colormap_size,
			    (vinfo->class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) canvas );
	    return (NULL);
	}
	if (window_attributes.depth != 24)
	{
	    (void) fprintf (stderr,
			    "Depth: %u for %s visual is not 24\n",
			    window_attributes.depth,
			    (vinfo->class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) canvas );
	    return (NULL);
	}
	switch (vinfo->red_mask)
	{
	  case 0xff:
	    canvas->red_offset = (im_byte_order == MSBFirst) ? 3 : 0;
	    break;
	  case 0xff00:
	    canvas->red_offset = (im_byte_order == MSBFirst) ? 2 : 1;
	    break;
	  case 0xff0000:
	    canvas->red_offset = (im_byte_order == MSBFirst) ? 1 : 2;
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Red mask: 0x%x for %s visual is not valid\n",
			    vinfo->red_mask,
			    (vinfo->class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) canvas );
	    return (NULL);
/*
	    break;
*/
	}
	switch (vinfo->green_mask)
	{
	  case 0xff:
	    canvas->green_offset = (im_byte_order == MSBFirst) ? 3 : 0;
	    break;
	  case 0xff00:
	    canvas->green_offset = (im_byte_order == MSBFirst) ? 2 : 1;
	    break;
	  case 0xff0000:
	    canvas->green_offset = (im_byte_order == MSBFirst) ? 1 : 2;
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Green mask: 0x%x for %s visual is not valid\n",
			    vinfo->green_mask,
			    (vinfo->class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) canvas );
	    return (NULL);
/*
	    break;
*/
	}
	switch (vinfo->blue_mask)
	{
	  case 0xff:
	    canvas->blue_offset = (im_byte_order == MSBFirst) ? 3 : 0;
	    break;
	  case 0xff00:
	    canvas->blue_offset = (im_byte_order == MSBFirst) ? 2 : 1;
	    break;
	  case 0xff0000:
	    canvas->blue_offset = (im_byte_order == MSBFirst) ? 1 : 2;
	    break;
	  default:
	    (void) fprintf (stderr,
			    "Blue mask: 0x%x for %s visual is not valid\n",
			    vinfo->blue_mask,
			    (vinfo->class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) canvas );
	    return (NULL);
/*
	    break;
*/
	}
	break;
      default:
	break;
    }
    switch (vinfo->class)
    {
      case PseudoColor:
	canvas->visual = KWIN_VISUAL_PSEUDOCOLOUR;
	break;
      case DirectColor:
	canvas->visual = KWIN_VISUAL_DIRECTCOLOUR;
	break;
      case TrueColor:
	canvas->visual = KWIN_VISUAL_TRUECOLOUR;
	break;
      case GrayScale:
	canvas->visual = KWIN_VISUAL_GRAYSCALE;
	break;
      case StaticGray:
	canvas->visual = KWIN_VISUAL_STATICGRAY;
	break;
      case StaticColor:
	canvas->visual = KWIN_VISUAL_STATICCOLOUR;
	break;
      default:
	(void) fprintf (stderr, "Illegal visual class: %u\n", vinfo->class);
	a_prog_bug (function_name);
	break;
    }
    XFree (vinfo);
    canvas->depth = window_attributes.depth;
    if ( ( canvas->font = kwin_load_font (canvas, "fixed") ) == NULL )
    {
	m_free ( (char *) canvas );
	return (NULL);
    }
    return (canvas);
}   /*  End Function kwin_create_x  */

/*PUBLIC_FUNCTION*/
void kwin_set_gc_x (canvas, gc)
/*  This routine will register a new Graphics Context to be used when drawing
    into the pixel canvas. Subsequent drawing operations will use the new
    Graphics Context.
    NOTE: this routine is only available with the X window system.
    The canvas must be given by  canvas  .
    The Graphics Context must be given by  gc  .
    The routine returns nothing.
*/
KPixCanvas canvas;
GC gc;
{
    static char function_name[] = "kwin_set_gc_x";

    VERIFY_CANVAS (canvas);
    canvas->gc = gc;
    /*  Get GCValues  */
    if (XGetGCValues (canvas->display, canvas->gc,
		      GCFunction | GCPlaneMask |
		      GCForeground | GCBackground | GCLineWidth | GCLineStyle |
		      GCCapStyle | GCJoinStyle | GCFillStyle | GCFillRule |
		      GCTile | GCStipple | GCTileStipXOrigin |
		      GCTileStipYOrigin | GCFont | GCSubwindowMode |
		      GCGraphicsExposures | GCClipXOrigin | GCClipYOrigin |
		      GCDashOffset | GCArcMode,
		      &canvas->gcvalues) == 0)
    {
	(void) fprintf (stderr, "Error getting GC values\n");
	return;
    }
}   /*  End Function kwin_set_gc_x  */

/*PUBLIC_FUNCTION*/
GC kwin_get_gc_x (canvas)
/*  This routine will get the Graphics Context used when drawing into the
    pixel canvas. If the Graphics Context is modified, it should be registered
    prior to drawing on the canvas by calling  kwin_set_gc_x  .
    NOTE: this routine is only available with the X window system.
    The canvas must be given by  canvas  .
    The routine returns the Graphics Context.
*/
KPixCanvas canvas;
{
    static char function_name[] = "kwin_get_gc_x";

    VERIFY_CANVAS (canvas);
    return (canvas->gc);
}   /*  End Function kwin_get_gc_x  */


/*  X specific private functions follow  */

static void alloc_points (canvas, num_points)
/*  This routine will (re)allocate the array of XPoint structures.
    The canvas must be given by  canvas  .The  max_request_size  field is used.
    The number of points requested must be given by  num_points  .There is no
    guarantee that the number of points requested will be allocated.
    The routine returns nothing.
*/
KPixCanvas canvas;
int num_points;
{
    int max_points;
    extern int num_xpoints_allocated;
    extern XPoint *xpoints;
    static char function_name[] = "alloc_points";

    max_points = (canvas->max_request_size - 3) / 2 + 1;
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

static void alloc_arcs (canvas, num_arcs)
/*  This routine will (re)allocate the array of XArc structures.
    The canvas must be given by  canvas  .The  max_request_size  field is used.
    The number of arcs requested must be given by  num_arcs  .There is no
    guarantee that the number of arcs requested will be allocated.
    The routine returns nothing.
*/
KPixCanvas canvas;
int num_arcs;
{
    int max_arcs;
    extern int num_xarcs_allocated;
    extern XArc *xarcs;
    static char function_name[] = "alloc_arcs";

    max_arcs = (canvas->max_request_size - 3) / 3;
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

static void alloc_segments (canvas, num_segments)
/*  This routine will (re)allocate the array of XSegment structures.
    The canvas must be given by  canvas  .The  max_request_size  field is used.
    The number of segments requested must be given by  num_segments  .There is
    no guarantee that the number of segments requested will be allocated.
    The routine returns nothing.
*/
KPixCanvas canvas;
int num_segments;
{
    int max_segments;
    extern int num_xsegments_allocated;
    extern XSegment *xsegments;
    static char function_name[] = "alloc_segments";

    max_segments = (canvas->max_request_size - 3) / 2;
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

static void get_colour_from_pixel (canvas, colour_def)
/*  This routine will get the colour associated with a pixel value.
    The canvas must be given by  canvas  .
    The pixel value must be in the  pixel  field in  colour_def  .
    The colour information is written to the storage pointed to by  colour_def
    The routine returns nothing.
*/
KPixCanvas canvas;
XColor *colour_def;
{
    static char function_name[] = "get_colour_from_pixel";

    XQueryColor (canvas->display, canvas->cmap, colour_def);
}   /*  End Function get_colour_from_pixel  */

static void get_colours_from_pixels (canvas, colour_def, num_pixels)
/*  This routine will get the colour associated with a pixel value.
    The canvas must be given by  canvas  .
    The pixel value must be in the  pixel  field in  colour_def  .
    The colour information is written to the storage pointed to by  colour_def
    The number of pixels must be given by  num_pixels  .
    The routine returns nothing.
*/
KPixCanvas canvas;
XColor *colour_def;
unsigned int num_pixels;
{
    static char function_name[] = "get_colour_from_pixels";

    XQueryColors (canvas->display, canvas->cmap, colour_def, (int) num_pixels);
}   /*  End Function get_colours_from_pixels  */

static XVisualInfo *get_visinfo_for_visual (dpy, visual)
/*  This routine will get the visual information structure for a visual.
    The X display must be given by  dpy  .
    The visual must be given by  visual  .
    The routine returns a pointer to an XVisualInfo structure on succes, else
    it returns NULL. The XVisualInfo structure must be freed by XFree()
*/
Display *dpy;
Visual *visual;
{
    int num_vinfos;
    XVisualInfo vinfo_template;
    XVisualInfo *vinfos;
    static char function_name[] = "__kwin_get_visinfo_for_visual";

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
	(void) fprintf (stderr,
			"%s: WARNING: number of visuals for visual: %p is: %d\n",
			function_name, visual, num_vinfos);
    }
    return (vinfos);
}   /*  End Function get_visinfo_for_visual  */


#endif  /*  X11  */

/*  VX specific functions follow  */
#ifdef OS_VXMVX

/*PUBLIC_FUNCTION*/
KPixCanvas kwin_create_vx (visual, right_buf, xoff, yoff, width, height,
			   background)
/*  This routine will create a pixel canvas, ready for drawing, on a VX screen.
    Note that the origin of a KPixCanvas is the upper-left corner.
    NOTE: this routine is only available when running on a VX.
    The visual type for the canvas must be given by  visual  .Legal values are:
        VC_VX_VISUAL_PSEUDOCOLOUR0    8  bits deep (alpha channel)
        VC_VX_VISUAL_PSEUDOCOLOUR1    8  bits deep (blue channel)
        VC_VX_VISUAL_PSEUDOCOLOUR2    8  bits deep (green channel)
        VC_VX_VISUAL_PSEUDOCOLOUR3    8  bits deep (red channel)
        VC_VX_VISUAL_DIRECTCOLOUR     24 bits deep
    Note that PseudoColour channels 1, 2 and 3 occupy the same area of screen
    memory as the DirectColour channel. When drawing to a PseudoColour canvas,
    the upper 24 bits are discarded. When drawing to a DirectColour canvas,
    the upper 8 bits are discarded, also, bits 0-7, 8-15 and 16-23 contain the
    red, green and blue components, respectively.
    NOTE: when drawing onto the canvas, changes will not be visible unless
    vc_set_visual  has been called with the appropriate parameters.
    If the VX display is operating in stereo mode, the canvas will be mapped to
    the right-eye frame buffer if  right_buf  is TRUE.
    The horizontal offset of the canvas origin (upper-left corner) relative to
    the parent window must be given by  xoff  .
    The vertical offset of the canvas origin (upper-left corner) relative to
    the parent window must be given by  yoff  .
    The width (vertical extent) of the canvas must be given by  width  .
    The height (horizontal extent) of the canvas must be given by  height  .
    The background colour must be given by  background  .
    The routine returns a pixel canvas on success, else it returns NULL.
*/
unsigned int visual;
flag right_buf;
int xoff;
int yoff;
int width;
int height;
unsigned long background;
{
    KPixCanvas canvas;
    unsigned int vis_type, depth;
    unsigned int byte_off;
    unsigned int *ptr;
    static char function_name[] = "kwin_create_vx";

    switch (visual)
    {
      case VC_VX_VISUAL_PSEUDOCOLOUR0:
	byte_off = 0;
	vis_type = KWIN_VISUAL_PSEUDOCOLOUR;
	depth = 8;
	break;
      case VC_VX_VISUAL_PSEUDOCOLOUR1:
	byte_off = 1;
	vis_type = KWIN_VISUAL_PSEUDOCOLOUR;
	depth = 8;
	break;
      case VC_VX_VISUAL_PSEUDOCOLOUR2:
	byte_off = 2;
	vis_type = KWIN_VISUAL_PSEUDOCOLOUR;
	depth = 8;
	break;
      case VC_VX_VISUAL_PSEUDOCOLOUR3:
	byte_off = 3;
	vis_type = KWIN_VISUAL_PSEUDOCOLOUR;
	depth = 8;
	break;
      case VC_VX_VISUAL_DIRECTCOLOUR:
	byte_off = 1;
	vis_type = KWIN_VISUAL_DIRECTCOLOUR;
	depth = 24;
	break;
      default:
	(void) fprintf (stderr, "Illegal visual type: %u\n", visual);
	a_prog_bug (function_name);
	break;
    }
    FLAG_VERIFY (right_buf);
    if ( ( canvas = alloc_canvas () ) == NULL )
    {
	m_error_notify (function_name, "VX pixel canvas");
	return (NULL);
    }
    vx_videoformat (NULL, &canvas->screen_width, &canvas->screen_height);
    canvas->xoff = xoff;
    canvas->yoff = yoff;
    canvas->width = width;
    canvas->height = height;
    canvas->colours = NULL;
    canvas->visual = vis_type;
    canvas->depth = depth;
    /*  These only apply for DirectColour  */
    canvas->red_offset = 2;
    canvas->green_offset = 1;
    canvas->blue_offset = 0;
    canvas->vxvisual = visual;
    ptr = (unsigned int *) 0x80000000;
    if (right_buf) ptr += canvas->screen_width * canvas->screen_height;
    canvas->frame_buffer = ptr;
    ptr += xoff + canvas->screen_width * yoff;
    canvas->origin = (unsigned char *) ptr + byte_off;
    canvas->background = background;
    kwin_fill_rectangle (canvas, 0, 0, canvas->width, canvas->height,
			 canvas->background);
    return (canvas);
}   /*  End Function kwin_create_vx  */

#endif  /*  OS_VXMVX  */

/*  Generic functions follow  */

#ifndef OS_VXMVX
/*PUBLIC_FUNCTION*/
KPixCanvas kwin_create_child (parent, xoff, yoff, width, height, absorb_events)
/*  This routine will create a pixel canvas, ready for drawing, within an
    existing pixel canvas. The new pixel canvas is a child of the parent pixel
    canvas. The child *does not* receive refresh events from the parent, it
    *does* receive position events from the parent, provided these events lie
    within the child's boundaries. For X-based graphics, a copy of the
    Graphics Context is made for the child.
    Note that the origin of a KPixCanvas is the upper-left corner.
    The parent pixel canvas must be given by  parent  .
    The horizontal offset of the canvas origin (upper-left corner) relative to
    the parent window must be given by  xoff  .
    The vertical offset of the canvas origin (upper-left corner) relative to
    the parent window must be given by  yoff  .
    The width (vertical extent) of the canvas must be given by  width  .
    The height (horizontal extent) of the canvas must be given by  height  .
    If the value of  absorb_events  is TRUE, any events passed from the parent
    canvas to the child canvas and consumed by the registered event functions
    for the child canvas are consumed from the parent canvas. If the value of
    absort_events  is FALSE, the child canvas does not consume events from the
    parent. Note that the parent events are still *passed* to the child canvas.
    The routine returns a pixel canvas on success, else it returns NULL.
*/
KPixCanvas parent;
int xoff;
int yoff;
int width;
int height;
flag absorb_events;
{
    KPixCanvas child;
    struct child_type *handle;
    static char function_name[] = "kwin_create_child";

    VERIFY_CANVAS (parent);
    if ( ( handle = (struct child_type *) m_alloc (sizeof *handle) ) == NULL )
    {
	m_error_notify (function_name, "child type structure");
	return (NULL);
    }
    if ( ( child = alloc_canvas () ) == NULL )
    {
	m_error_notify (function_name, "X pixel canvas");
	m_free ( (char *) handle );
	return (NULL);
    }
    m_copy ( (char *) child, (char *) parent, sizeof *child );
    child->xoff = xoff;
    child->yoff = yoff;
    child->width = width;
    child->height = height;
    child->colours = NULL;
    child->refresh_list = NULL;
    child->position_list = NULL;
    child->pspage = NULL;
#ifdef X11
    /*  Create new Graphics Context  */
    child->gc = XCreateGC (child->display, child->window,
			   GCFunction | GCPlaneMask |
			   GCForeground | GCBackground | GCLineWidth |
			   GCLineStyle | GCCapStyle | GCJoinStyle |
			   GCFillStyle | GCFillRule |
			   GCTileStipXOrigin | GCTileStipYOrigin | GCFont |
			   GCSubwindowMode | GCGraphicsExposures |
			   GCClipXOrigin | GCClipYOrigin | GCDashOffset |
			   GCArcMode,
			   &child->gcvalues);
#endif
    handle->child = child;
    handle->absorb_events = absorb_events;
    kwin_register_position_event_func (parent, child_position_event_func,
				       (void *) handle);
    return (child);
}   /*  End Function kwin_create_child  */
#endif  /*  OS_VXMVX  */

/*PUBLIC_FUNCTION*/
KCallbackFunc kwin_register_refresh_func (KPixCanvas canvas,
					  void (*refresh_func) (), void *info)
/*  This routine will register a refresh function for a pixel canvas. The
    refresh function will be called whenever the contents of the canvas need to
    be redrawn. Many refresh functions may be registered per canvas. The first
    function registered is the first function called upon refresh.
    The canvas must be given by  canvas  .
    The function that is called when the canvas is to be refreshed must be
    pointed to by  refresh_func  .
    The interface to this routine is as follows:

    void refresh_func (canvas, width, height, info, pspage)
    *   This routine will process a refresh event for a pixel canvas.
        The canvas is given by  canvas  .
	The width of the canvas in pixels is given by  width  .
	The height of the canvas in pixels is given by  height  .
	The arbitrary canvas information pointer is pointed to by  info  .
	The PostScriptPage object will be given by  pspage  .If this is NULL,
	the refresh is *not* destined for a PostScript page.
	The routine returns nothing.
    *
    KPixCanvas canvas;
    int width;
    int height;
    void **info;
    PostScriptPage pspage;
    

    The initial arbitrary canvas information pointer must be given by  info  .
    The routine returns a KCallbackFunc object.
*/
{
    static char function_name[] = "kwin_register_refresh_func";

    VERIFY_CANVAS (canvas);
    return ( c_register_callback (&canvas->refresh_list,
				  refresh_event_func,
				  canvas,
				  info, TRUE, (void *) refresh_func, FALSE,
				  FALSE) );
}   /*  End Function kwin_register_refresh_func  */

/*PUBLIC_FUNCTION*/
KCallbackFunc kwin_register_position_event_func (KPixCanvas canvas,
						 flag (*func) (), void *f_info)
/*  This routine will register a position event function for a pixel canvas.
    The position event function will be called whenever a position event on the
    canvas has not been consumed. Many position event functions may be
    registered per canvas. The first function registered is the first function
    called upon a position event.
    The canvas must be given by  canvas  .
    The function that is called when a position event occurs must be pointed to
    by  func  .
    The interface to this routine is as follows:

    flag func (canvas, x, y, event_code, e_info, f_info)
    *   This routine is a position event consumer for a pixel canvas.
        The canvas is given by  canvas  .
	The horizontal position of the event, relative to the canvas origin,
	will be given by  x  .
	The vertical position of the event, relative to the canvas origin,
	will be given by  y  .
	The arbitrary event code is given by  event_code  .
	The arbitrary event information is pointed to by  e_info  .
	The arbitrary function information pointer is pointed to by  f_info  .
	The routine returns TRUE if the event was consumed, else it returns
	FALSE indicating that the event is still to be processed.
    *
    KPixCanvas canvas;
    int x;
    int y;
    unsigned int event_code;
    void *e_info;
    void **f_info;

    The initial arbitrary function information pointer must be given by  f_info
    The routine returns a KCallbackFunc object.
*/
{
    static char function_name[] = "kwin_register_position_func";

    VERIFY_CANVAS (canvas);
    return ( c_register_callback (&canvas->position_list,
				  position_event_func,
				  canvas, f_info, TRUE, (void *) func, FALSE,
				  TRUE) );
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
#ifdef OS_VXMVX
    int byte_off;
    unsigned int *ui_ptr;
#endif
    static char function_name[] = "kwin_resize";

    VERIFY_CANVAS (canvas);
    FLAG_VERIFY (clear);
    canvas->pspage = NULL;
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
	canvas->width = width;
	canvas->height = height;
	canvas->xoff = xoff;
	canvas->yoff = yoff;
#ifdef OS_VXMVX
	switch (canvas->vxvisual)
	{
	  case VC_VX_VISUAL_PSEUDOCOLOUR0:
	    byte_off = 0;
	    break;
	  case VC_VX_VISUAL_PSEUDOCOLOUR1:
	    byte_off = 1;
	    break;
	  case VC_VX_VISUAL_PSEUDOCOLOUR2:
	    byte_off = 2;
	    break;
	  case VC_VX_VISUAL_PSEUDOCOLOUR3:
	    byte_off = 3;
	    break;
	  case VC_VX_VISUAL_DIRECTCOLOUR:
	    byte_off = 1;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal visual type: %u\n",
			    canvas->vxvisual);
	    a_prog_bug (function_name);
	    break;
	}
	ui_ptr = canvas->frame_buffer;
	ui_ptr += xoff + canvas->screen_width * yoff;
	canvas->origin = (unsigned char *) ui_ptr + byte_off;
#endif
    }
    if (clear) kwin_clear (canvas, 0, 0, -1, -1);
    /*  Call refresh functions  */
    (void) c_call_callbacks (canvas->refresh_list, NULL);
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
    struct position_struct data;
    static char function_name[] = "kwin_process_position_event";

    VERIFY_CANVAS (canvas);
    if (canvas->pspage != NULL)
    {
	(void) fprintf (stderr, "Previous PostScriptPage still active\n");
	a_prog_bug (function_name);
    }
    if (clip)
    {
	/*  Clip x  */
	if (x < canvas->xoff)
	{
	    x = 0;
	}
	else if (x >= canvas->xoff + canvas->width)
	{
	    x = canvas->width - 1;
	}
	else
	{
	    x -= canvas->xoff;
	}
	/*  Clip y  */
	if (y < canvas->yoff)
	{
	    y = 0;
	}
	else if (y >= canvas->yoff + canvas->height)
	{
	    y = canvas->height - 1;
	}
	else
	{
	    y -= canvas->yoff;
	}
    }
    else
    {
	/*  Discard if outside  */
	if ( (x < canvas->xoff) || (x >= canvas->xoff + canvas->width) ||
	    (y < canvas->yoff) || (y >= canvas->yoff + canvas->height) )
	{
	    return (FALSE);
	}
	x -= canvas->xoff;
	y -= canvas->yoff;
    }
    /*  Have event in canvas co-ordinates: call event consumer functions  */
    data.x = x;
    data.y = y;
    data.event_code = event_code;
    data.e_info = event_info;
    return ( c_call_callbacks (canvas->position_list, &data) );
}   /*  End Function kwin_process_position_event  */

#ifndef OS_VXMVX
/*PUBLIC_FUNCTION*/
flag kwin_write_ps (KPixCanvas canvas, PostScriptPage pspage)
/*  This routine will refresh a pixel canvas, redirecting output to a
    PostScriptPage object.
    The pixel canvas must be given by  canvas  .
    The PostScriptPage object must be given by  pspage  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    static char function_name[] = "kwin_write_ps";

    VERIFY_CANVAS (canvas);
    if (pspage == NULL)
    {
	(void) fprintf (stderr, "NULL PostScriptPage object passed\n");
	a_prog_bug (function_name);
    }
    if (canvas->pspage != NULL)
    {
	(void) fprintf (stderr, "Previous PostScriptPage still active\n");
	a_prog_bug (function_name);
    }
    canvas->pspage = pspage;
    /*  Call refresh functions  */
    (void) c_call_callbacks (canvas->refresh_list, NULL);
    canvas->pspage = NULL;
    return (TRUE);
}   /*  End Function kwin_write_ps  */

/*PUBLIC_FUNCTION*/
void kwin_get_attributes (canvas, va_alist)
/*  This routine will get the attributes for a pixel canvas.
    The pixel canvas must be given by  canvas  .
    The list of parameter attribute-key attribute-value pairs must follow. See
    the header file for details on defined attributes.
    This list must be terminated with the value  KWIN_ATT_END  .
    The routine returns nothing.
*/
KPixCanvas canvas;
va_dcl
{
    va_list arg_pointer;
    unsigned int att_key;
    static char function_name[] = "kwin_get_attributes";

    va_start (arg_pointer);
    VERIFY_CANVAS (canvas);
    while ( ( att_key = va_arg (arg_pointer, unsigned int) ) != KWIN_ATT_END )
    {
	switch (att_key)
	{
	  case KWIN_ATT_VISUAL:
	    *( va_arg (arg_pointer, unsigned int *) ) = canvas->visual;
	    break;
	  case KWIN_ATT_DEPTH:
	    *( va_arg (arg_pointer, unsigned int *) ) = canvas->depth;
	    break;
	  case KWIN_ATT_VISIBLE:
	    *( va_arg (arg_pointer, flag *) ) = canvas->visible;
	    break;
	  case KWIN_ATT_FONT:
	    *( va_arg (arg_pointer, KPixCanvasFont *) ) = canvas->font;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (arg_pointer);
}   /*  End Function kwin_get_attributes  */

/*PUBLIC_FUNCTION*/
void kwin_set_attributes (canvas, va_alist)
/*  This routine will set the attributes for a pixel canvas.
    The pixel canvas must be given by  canvas  .
    The list of parameter attribute-key attribute-value pairs must follow. See
    the header file for details on defined attributes.
    This list must be terminated with the value  KWIN_ATT_END  .
    The routine returns nothing.
*/
KPixCanvas canvas;
va_dcl
{
    va_list arg_pointer;
    KPixCanvasFont font;
    flag bool;
    unsigned int att_key;
    static char function_name[] = "kwin_set_attributes";

    va_start (arg_pointer);
    VERIFY_CANVAS (canvas);
    while ( ( att_key = va_arg (arg_pointer, unsigned int) ) != KWIN_ATT_END )
    {
	switch (att_key)
	{
	  case KWIN_ATT_VISUAL:
	    (void) fprintf (stderr, "Cannot set the visual after creation\n");
	    a_prog_bug (function_name);
	    break;
	  case KWIN_ATT_DEPTH:
	    (void) fprintf (stderr, "Cannot set the depth after creation\n");
	    a_prog_bug (function_name);
	    break;
	  case KWIN_ATT_VISIBLE:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    canvas->visible = bool;
	    break;
	  case KWIN_ATT_FONT:
	    font = va_arg (arg_pointer, KPixCanvasFont);
	    VERIFY_FONT (font);
#ifdef X11
	    if (font->display != canvas->display)
	    {
		(void) fprintf (stderr,
				"Font not associated with canvas display!\n");
		a_prog_bug (function_name);
	    }
	    if (canvas->gcvalues.font != font->xfont->fid)
	    {
		canvas->gcvalues.font = font->xfont->fid;
		XChangeGC (canvas->display, canvas->gc, GCFont,
			   &canvas->gcvalues);
	    }
#endif
	    canvas->font = font;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (arg_pointer);
}   /*  End Function kwin_set_attributes  */
#endif  /*  OS_VXMVX  */


/*  Drawing routines follow  */

/*PUBLIC_FUNCTION*/
void kwin_clear (KPixCanvas canvas, int x, int y, int width, int height)
/*  This routine will clear a rectangular portion of a pixel canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the rectangle must be given by  x  .
    The vertical offset of the rectangle must be given by  y  .
    The width of the rectangle must be given by  width  .If this is less than
    0 the pixel canvas is cleared to its right edge.
    The height of the rectangle must be given by  height  .If this is less than
    0 the pixel canvas is cleared to its bottom edge.
    NOTE: to clear the entire canvas, the following should be used:
        kwin_clear (canvas, 0, 0, -1, -1);
    The routine returns nothing.
*/
{
    static char function_name[] = "kwin_clear";

    VERIFY_CANVAS (canvas);
    if (canvas->pspage != NULL)
    {
	(void) fprintf (stderr, "Cannot clear a PostScriptPage object\n");
	return;
    }
    if (x < 0)
    {
	width = (width < 0) ? canvas->width : width + x;
	x = 0;
    }
    if (y < 0)
    {
	height = (height < 0) ? canvas->height : height + y;
	y = 0;
    }
    if (width < 0) width = canvas->width - x;
    if (height < 0) height = canvas->height - y;
    if ( (width < 1) || (height < 1) ) return;

#undef IMPLEMENTED

#ifdef X11
#define IMPLEMENTED
    XClearArea (canvas->display, canvas->window,
		canvas->xoff + x, canvas->yoff + y,
		(unsigned int) width, (unsigned int) height, False);
#endif

#ifdef OS_VXMVX
#define IMPLEMENTED
    kwin_fill_rectangle (canvas, x, y, width, height, canvas->background);
#endif

#ifndef IMPLEMENTED
    not_implemented (function_name);
#endif
}   /*  End Function kwin_clear  */

#ifndef OS_VXMVX
/*PUBLIC_FUNCTION*/
flag kwin_draw_image (KPixCanvas canvas, array_desc *arr_desc, char *slice,
		      unsigned int hdim, unsigned int vdim,
		      unsigned int elem_index, unsigned int num_pixels,
		      unsigned long *pixel_values,
		      struct win_scale_type *win_scale,
		      KPixCanvasImageCache *cache_ptr)
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
{
    int y0, y1;
    unsigned int x, y;
    unsigned int count;
    unsigned int dim_count;
    unsigned int hstride, vstride;
    unsigned int type;
    unsigned int abs_start_coord;
    unsigned int abs_end_coord;
    unsigned int ord_start_coord;
    unsigned int ord_end_coord;
    unsigned int num_abs_coords;
    unsigned int num_ord_coords;
    unsigned char *ubimage;
    double val;
    unsigned char *ub_pixels;
    uaddr *hoffsets, *voffsets;
    unsigned char imap_red[256], imap_green[256], imap_blue[256];
    dim_desc *hdim_desc, *vdim_desc;
#ifdef X11
    XColor xcolours[256];
#endif
    KPixCanvasImageCache cache;
    static char function_name[] = "kwin_draw_image";

    VERIFY_CANVAS (canvas);
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
    slice += ds_get_element_offset (arr_desc->packet, elem_index);
    type = arr_desc->packet->element_types[elem_index];
    if ( !ds_element_is_atomic (type) )
    {
	(void) fprintf (stderr, "Element must be atomic\n");
	a_prog_bug (function_name);
    }
    /*  Determine start and stop co-ordinates along each dimension  */
    abs_start_coord = ds_get_coord_num (hdim_desc, win_scale->x_min,
					SEARCH_BIAS_CLOSEST);
    abs_end_coord = ds_get_coord_num (hdim_desc, win_scale->x_max,
				      SEARCH_BIAS_CLOSEST);
    num_abs_coords = abs_end_coord - abs_start_coord + 1;
    ord_start_coord = ds_get_coord_num (vdim_desc, win_scale->y_min,
					SEARCH_BIAS_CLOSEST);
    ord_end_coord = ds_get_coord_num (vdim_desc, win_scale->y_max,
				      SEARCH_BIAS_CLOSEST);
    num_ord_coords = ord_end_coord - ord_start_coord + 1;
    if (canvas->pspage != NULL)
    {
	if (win_scale->z_scale != K_INTENSITY_SCALE_LINEAR)
	{
	    (void) fprintf (stderr,
			    "Non-linear intensity scale not supported yet\n");
	    return (FALSE);
	}
	if (num_pixels > 256)
	{
	    (void) fprintf (stderr, "Number of pixels is greater than 256\n");
	    a_prog_bug (function_name);
	}
	for (count = 0; count < num_pixels; ++count)
	{
	    xcolours[count].pixel = pixel_values[count];
	}
	get_colours_from_pixels (canvas, xcolours, num_pixels);
	for (count = 0; count < num_pixels; ++count)
	{
	    imap_red[count] = (int) xcolours[count].red >> 8;
	    imap_green[count] = (int) xcolours[count].green >> 8;
	    imap_blue[count] = (int) xcolours[count].blue >> 8;
	}
	/*  Allocate temporary image  */
	if ( ( ubimage = (unsigned char *)
	      m_alloc (sizeof *ubimage * num_abs_coords * num_ord_coords) )
	    == NULL )
	{
	    m_error_notify (function_name, "ubarray");
	    return (FALSE);
	}
	/*  Convert image  */
	for (y = ord_start_coord; y <= ord_end_coord; ++y)
	{
	    for (x = abs_start_coord; x <= abs_end_coord; ++x)
	    {
		val = ds_convert_atomic (slice + hoffsets[x] + voffsets[y],
					 type, (double *) NULL,
					 (double *) NULL);
		val -= win_scale->z_min;
		val /= win_scale->z_max - win_scale->z_min;
		val *= (double) (num_pixels - 1);
		ubimage[(y - ord_start_coord) * num_abs_coords +
			(x - abs_start_coord)] = (int) val;
	    }
	}
	/*  Flip  */
	y0 = canvas->height - 1 - win_scale->y_offset;
	y1 = canvas->height - 1 - (win_scale->y_offset + win_scale->y_pixels);
	if ( !psw_pseudocolour_image
	    ( canvas->pspage, ubimage,
	     num_abs_coords, num_ord_coords,
	     (uaddr *) NULL, (uaddr *) NULL,
	     imap_red, imap_green, imap_blue,
	     (double) win_scale->x_offset / (double) (canvas->width - 1),
	     (double) y1 / (double) (canvas->height - 1),
	     (double) (win_scale->x_offset + win_scale->x_pixels) /
	     (double) (canvas->width - 1),
	     (double) y0 / (double) (canvas->height - 1) ) )
	{
	    m_free ( (char *) ubimage );
	    return (FALSE);
	}
	m_free ( (char *) ubimage );
	return (TRUE);
    }
    if (!canvas->visible) return (TRUE);
    if (arr_desc->num_levels > 0)
    {
	(void) fprintf (stderr, "Tiling not supported.\n");
	return (FALSE);
    }
    if ( ( cache = size_cache (canvas, cache_ptr,
			       win_scale->x_pixels, win_scale->y_pixels) )
	== NULL )
    {
	return (FALSE);
    }
#ifdef X11
    /*  Now have an XImage big enough  */
    hstride = ds_get_packet_size (arr_desc->packet);
    for (dim_count = arr_desc->num_dimensions - 1; dim_count > hdim;
	 --dim_count)
    {
	hstride *= (* arr_desc->dimensions[dim_count] ).length;
    }
    vstride = ds_get_packet_size (arr_desc->packet);
    for (dim_count = arr_desc->num_dimensions - 1; dim_count > vdim;
	 --dim_count)
    {
	vstride *= (* arr_desc->dimensions[dim_count] ).length;
    }
    if ( (num_abs_coords == win_scale->x_pixels) &&
	(num_ord_coords == win_scale->y_pixels) &&
	(cache->ximage->depth == 8) &&
	(win_scale->z_scale == K_INTENSITY_SCALE_LINEAR) )
    {
	/*  New drawing routines  */
	/*  Convert pixel values  */
	if ( ( ub_pixels = (unsigned char *)
	      m_alloc (num_pixels * sizeof *ub_pixels) ) == NULL )
	{
	    m_error_notify (function_name, "unsigned byte pixel array");
	    return (FALSE);
	}
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
	/*  Note the casts from (uaddr *) to (iaddr *) for the offset
	    arrays. This is dodgy, but it should work.  */
	if ( !imw_to8_o ( (unsigned char *) cache->ximage->data, 1,
			 cache->ximage->bytes_per_line,
			 win_scale->x_pixels, win_scale->y_pixels,
			 slice + hoffsets[abs_start_coord] +
			 voffsets[ord_start_coord],
			 (iaddr *) hoffsets, (iaddr *) voffsets,
			 type, win_scale->conv_type,
			 num_pixels, ub_pixels,
			 win_scale->blank_pixel,
			 win_scale->min_sat_pixel,
			 win_scale->max_sat_pixel,
			 win_scale->z_min, win_scale->z_max ) )
	{
	    (void) fprintf (stderr, "Error drawing image into KPixCanvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL)
	    {
		*cache_ptr = NULL;
	    }
	    m_free ( (char *) ub_pixels );
	    return (FALSE);
	}
	m_free ( (char *) ub_pixels );
    }
    else
    {
	if (drw_single_plane (cache->ximage, num_pixels, pixel_values,
			      slice,
			      arr_desc->packet->element_types[elem_index],
			      win_scale->conv_type,
			      arr_desc->dimensions[hdim], hstride,
			      arr_desc->dimensions[vdim], vstride,win_scale)
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
    }
    cache->width = win_scale->x_pixels;
    cache->height = win_scale->y_pixels;
    if (cache->pixmap == (Pixmap) NULL)
    {
	/*  No pixmap copy: just dump the image  */
	xi_put_image (canvas->display, canvas->window, canvas->gc,
		      cache->ximage,
		      0, 0,
		      win_scale->x_offset + canvas->xoff,
		      win_scale->y_offset + canvas->yoff,
		      cache->width, cache->height,
		      cache->shared, TRUE);
    }
    else
    {
	/*  First copy to pixmap and then copy from pixmap to screen. Copying
	    to screen then to pixmap is bad if the screen is obscured.  */
	xi_put_image (canvas->display, cache->pixmap, canvas->gc,
		      cache->ximage,
		      0, 0,
		      0, 0,
		      cache->width, cache->height,
		      cache->shared, TRUE);
	XCopyArea (canvas->display, cache->pixmap, canvas->window,
		   canvas->gc,
		   0, 0,
		   cache->width, cache->height,
		   win_scale->x_offset + canvas->xoff,
		   win_scale->y_offset + canvas->yoff);
    }
/*
    XBell (canvas->display, 100);
*/
#endif  /*  X11  */
    if (cache_ptr == NULL)
    {
	kwin_free_cache_data (cache);
    }
    else
    {
	*cache_ptr = cache;
    }
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    return (TRUE);
#endif

#ifndef IMPLEMENTED
    not_implemented (function_name);
    return (FALSE);
#endif
}   /*  End Function kwin_draw_image  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_rgb_image (KPixCanvas canvas, int x_off, int y_off,
			  int x_pixels, int y_pixels,
			  CONST unsigned char *red_slice,
			  CONST unsigned char *green_slice,
			  CONST unsigned char *blue_slice,
			  CONST uaddr *hoffsets, CONST uaddr *voffsets,
			  unsigned int width, unsigned int height,
			  KPixCanvasImageCache *cache_ptr)
/*  This routine will draw a 2-dimensional slice of a Karma array onto a pixel
    canvas. This slice may be tiled. The slice is a RGB image.
    The canvas must be given by  canvas  .
    The horizontal offset, relative to the top-left corner of the canvas, must
    be given by  x_off  .
    The vertical offset, relative to the top-left corner of the canvas, must
    be given by  y_off  .
    The number of horizontal pixels to draw must be given by  x_pixels  .
    The number of vertical pixels to draw must be given by  y_pixels  .
    The start of the red slice data must be pointed to by  red_slice  .
    The start of the green slice data must be pointed to by  green_slice  .
    The start of the blue slice data must be pointed to by  blue_slice  .
    NOTE: the 3 colour components must be of type  K_UBYTE  .
    The array of horizontal byte offsets must be pointed to by  hoffsets  .
    If this is NULL, the stride between successive horizontal values (in bytes)
    must be given by  hstride  .
    The array of vertical byte offsets must be pointed to by  voffsets  .
    If this is NULL, the stride between successive vertical values (in bytes)
    must be given by  vstride  .
    The width of the input image (in values) must be given by  width  .
    The height of the input image (in values) must be given by  height  .
    The routine may produce cache data which will vastly increase the speed of
    subsequent operations on this data. The routine will write a pointer to
    this data to the storage pointed to by  cache_ptr  .Prior to process
    exit, a call MUST be made to  kwin_free_cache_data  ,otherwise shared
    memory segments could remain after the process exits.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    int y0, y1;
    unsigned int count;
    unsigned char *out_image;
#ifdef X11
    iaddr pstride;
#endif
    unsigned char pixels[256];
    KPixCanvasImageCache cache;
    static char function_name[] = "kwin_draw_rgb_image";

    VERIFY_CANVAS (canvas);
    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
	(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
    {
	(void) fprintf (stderr,
			"Canvas visual type: %u illegal for RGB images\n",
			canvas->visual);
	a_prog_bug (function_name);
    }
    if (canvas->depth != 24)
    {
	(void) fprintf(stderr,
		       "Canvas depth: %u illegal for RGB images. Must be 24\n",
		       canvas->depth);
	a_prog_bug (function_name);
    }
    if (canvas->pspage != NULL)
    {
	/*  Flip  */
	y0 = canvas->height - 1 - y_off;
	y1 = canvas->height - 1 - (y_off + y_pixels);
	if ( !psw_rgb_image
	    ( canvas->pspage, red_slice, green_slice, blue_slice,
	     width, height,
	     hoffsets, voffsets, hoffsets, voffsets, hoffsets, voffsets, 0,
	     (double) x_off / (double) (canvas->width - 1),
	     (double) y1 / (double) (canvas->height - 1),
	     (double) (x_off + x_pixels) / (double) (canvas->width - 1),
	     (double) y0 / (double) (canvas->height - 1) ) )
	{
	    return (FALSE);
	}
	return (TRUE);
    }
    if (!canvas->visible) return (TRUE);
    if ( (x_pixels != width) || (y_pixels != height) )
    {
	(void) fprintf (stderr, "Non 1:1 RGB display not implemented yet\n");
	(void) fprintf (stderr,
			"x_pix: %d  y_pix: %d    width: %d  height: %d\n",
			x_pixels, y_pixels, width, height);
	return (FALSE);
    }
    if ( ( cache = size_cache (canvas, cache_ptr, x_pixels, y_pixels) )
	== NULL )
    {
	return (FALSE);
    }
#ifdef X11
    /*  Now have an XImage big enough  */
    pstride = cache->ximage->bits_per_pixel / 8;
    out_image = (unsigned char *) cache->ximage->data;
    /*  Set up pixel value array  */
    for (count = 0; count < 256; ++count) pixels[count] = count;
    /*  Note the casts from (uaddr *) to (iaddr *) for the offset
	arrays. This is dodgy, but it should work.  */
    if ( !imw_to8_o (out_image + canvas->red_offset, pstride,
		     cache->ximage->bytes_per_line,
		     x_pixels, y_pixels, (CONST char *) red_slice,
		     (iaddr *) hoffsets, (iaddr *) voffsets,
		     K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
		     256, pixels, 0, 0, 255, 0.0, 255.0) )
    {
	(void) fprintf (stderr, "Error drawing red image into KPixCanvas\n");
	kwin_free_cache_data (cache);
	if (cache_ptr != NULL)
	{
	    *cache_ptr = NULL;
	}
	return (FALSE);
    }
    if ( !imw_to8_o (out_image + canvas->green_offset, pstride,
		     cache->ximage->bytes_per_line,
		     x_pixels, y_pixels, (CONST char *) green_slice,
		     (iaddr *) hoffsets, (iaddr *) voffsets,
		     K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
		     256, pixels, 0, 0, 255, 0.0, 255.0) )
    {
	(void) fprintf (stderr, "Error drawing green image into KPixCanvas\n");
	kwin_free_cache_data (cache);
	if (cache_ptr != NULL)
	{
	    *cache_ptr = NULL;
	}
	return (FALSE);
    }
    if ( !imw_to8_o (out_image + canvas->blue_offset, pstride,
		     cache->ximage->bytes_per_line,
		     x_pixels, y_pixels, (CONST char *) blue_slice,
		     (iaddr *) hoffsets, (iaddr *) voffsets,
		     K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
		     256, pixels, 0, 0, 255, 0.0, 255.0) )
    {
	(void) fprintf (stderr, "Error drawing blue image into KPixCanvas\n");
	kwin_free_cache_data (cache);
	if (cache_ptr != NULL)
	{
	    *cache_ptr = NULL;
	}
	return (FALSE);
    }
    cache->width = x_pixels;
    cache->height = y_pixels;
    if (cache->pixmap == (Pixmap) NULL)
    {
	/*  No pixmap copy: just dump the image  */
	xi_put_image (canvas->display, canvas->window, canvas->gc,
		      cache->ximage,
		      0, 0,
		      x_off + canvas->xoff, y_off + canvas->yoff,
		      cache->width, cache->height,
		      cache->shared, TRUE);
    }
    else
    {
	/*  First copy to pixmap and then copy from pixmap to screen. Copying
	    to screen then to pixmap is bad if the screen is obscured.  */
	xi_put_image (canvas->display, cache->pixmap, canvas->gc,
		      cache->ximage,
		      0, 0,
		      0, 0,
		      cache->width, cache->height,
		      cache->shared, TRUE);
	XCopyArea (canvas->display, cache->pixmap, canvas->window,
		   canvas->gc,
		   0, 0,
		   cache->width, cache->height,
		   x_off + canvas->xoff, y_off + canvas->yoff);
    }
/*
    XBell (canvas->display, 100);
*/
#endif  /*  X11  */
    if (cache_ptr == NULL)
    {
	kwin_free_cache_data (cache);
    }
    else
    {
	*cache_ptr = cache;
    }
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    return (TRUE);
#endif

#ifndef IMPLEMENTED
    not_implemented (function_name);
    return (FALSE);
#endif
}   /*  End Function kwin_draw_rgb_image  */
#endif  /*  OS_VXMVX  */

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
    if (cache->magic_number != CACHE_DATA_MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Invalid cache data\n");
	a_prog_bug (function_name);
    }
    canvas = cache->canvas;
    /*  It's a shame to have to throw away the cache validity  */
    if (canvas->pspage != NULL) return (FALSE);
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    if ( (cache->ximage != NULL) && cache->shared )
    {
	xi_put_image (canvas->display, canvas->window, canvas->gc,
		      cache->ximage,
		      0, 0, x_off + canvas->xoff, y_off + canvas->yoff,
		      cache->width, cache->height,
		      cache->shared, TRUE);
	return (TRUE);
    }
    if (cache->pixmap != (Pixmap) NULL)
    {
	XCopyArea (canvas->display, cache->pixmap, canvas->window,
		   canvas->gc,
		   0, 0, cache->width, cache->height,
		   x_off + canvas->xoff, y_off + canvas->yoff);
	return (TRUE);
    }
    if (cache->ximage == NULL) return (FALSE);
    /*  Call it again with non-pixmap, non-shared XImage  */
    xi_put_image (canvas->display, canvas->window, canvas->gc,
		  cache->ximage,
		  0, 0, x_off + canvas->xoff, y_off + canvas->yoff,
		  cache->width, cache->height,
		  cache->shared, TRUE);
/*
    XBell (canvas->display, 100);
*/
    return (TRUE);
#endif  /*  X11  */

#ifndef IMPLEMENTED
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
#ifdef OS_VXMVX
    unsigned char *b_ptr;
#endif
    static char function_name[] = "kwin_draw_point";

    VERIFY_CANVAS (canvas);
    if (canvas->pspage != NULL)
    {
	a_func_abort (function_name, "PostScript output not yet supported");
	return;
    }
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    if (pixel_value != canvas->gcvalues.foreground)
    {
	canvas->gcvalues.foreground = pixel_value;
	XChangeGC (canvas->display, canvas->gc, GCForeground,
		   &canvas->gcvalues);
    }
    XDrawPoint (canvas->display, canvas->window, canvas->gc,
		canvas->xoff + x, canvas->yoff + y);
    return;
#endif

#ifdef OS_VXMVX
#define IMPLEMENTED
    b_ptr = canvas->origin + (x + y * canvas->screen_width) * 4;
    if (canvas->vxvisual == VC_VX_VISUAL_DIRECTCOLOUR)
    {
	b_ptr[canvas->red_offset] = pixel_value;
	b_ptr[canvas->green_offset] = pixel_value >> 8;
	b_ptr[canvas->blue_offset] = pixel_value >> 16;
    }
    else
    {
	b_ptr[0] = pixel_value;
    }
    return;
#endif

#ifndef IMPLEMENTED
    not_implemented (function_name);
#endif
}   /*  End Function kwin_draw_point  */

/*PUBLIC_FUNCTION*/
void kwin_draw_line (canvas, x0, y0, x1, y1, pixel_value)
/*  This routine will draw a single line onto a pixel canvas.
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
#ifdef OS_VXMVX
    int sum;
    int inc;
    int x, y;
    int tmp;
    int delta_x, delta_y;
#endif
#ifdef X11
    XColor rgb_def;
#endif
    static char function_name[] = "kwin_draw_line";

    VERIFY_CANVAS (canvas);
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    if (canvas->pspage == NULL)
    {
	if (pixel_value != canvas->gcvalues.foreground)
	{
	    canvas->gcvalues.foreground = pixel_value;
	    XChangeGC (canvas->display, canvas->gc, GCForeground,
		       &canvas->gcvalues);
	}
	XDrawLine (canvas->display, canvas->window, canvas->gc,
		   canvas->xoff + x0, canvas->yoff + y0,
		   canvas->xoff + x1, canvas->yoff + y1);
	return;
    }
    /*  Do a PostScript draw  */
    rgb_def.pixel = pixel_value;
    get_colour_from_pixel (canvas, &rgb_def);
    /*  Flip vertical  */
    y0 = canvas->height - 1 - y0;
    y1 = canvas->height - 1 - y1;
    (void) psw_rgb_line ( canvas->pspage,
			 (double) rgb_def.red / MAX_INTENSITY,
			 (double) rgb_def.green / MAX_INTENSITY,
			 (double) rgb_def.blue / MAX_INTENSITY,
			 (double) x0 / (double) (canvas->width - 1),
			 (double) y0 / (double) (canvas->height - 1),
			 (double) x1 / (double) (canvas->width - 1),
			 (double) y1 / (double) (canvas->height - 1) );
    return;
#endif

#ifdef OS_VXMVX
#define IMPLEMENTED
    if (canvas->pspage != NULL)
    {
	a_func_abort (function_name, "PostScript output not yet supported");
	return;
    }
    if ( (x1 == x0) && (y1 == y0) )
    {
	kwin_draw_point (canvas, x0, y0, pixel_value);
	return;
    }
    /*  First do horizontal draw  */
    if (x1 < x0)
    {
	tmp = x1;  x1 = x0;  x0 = tmp;
	tmp = y1;  y1 = y0;  y0 = tmp;
    }
    delta_x = x1 - x0;
    if (y1 < y0)
    {
	/*  Line goes up  */
	delta_y = y0 - y1;
	inc = -1;
    }
    else
    {
	/*  Line goes down  */
	delta_y = y1 - y0;
	inc = 1;
    }
    x = x0;
    y = y0;
    sum = delta_x / 2;
    kwin_draw_point (canvas, x, y, pixel_value);
    while (x < x1)
    {
	++x;
	sum += delta_y;
	while (sum >= delta_x)
	{
	    y += inc;
	    sum -= delta_x;
	}
	kwin_draw_point (canvas, x, y, pixel_value);
    }
    /*  Now do vertical draw  */
    if (y1 < y0)
    {
	tmp = y1;  y1 = y0;  y0 = tmp;
	tmp = x1;  x1 = x0;  x0 = tmp;
    }
    delta_y = y1 - y0;
    if (x1 < x0)
    {
	/*  Line goes left  */
	delta_x = x0 - x1;
	inc = -1;
    }
    else
    {
	/*  Line goes right  */
	delta_x = x1 - x0;
	inc = 1;
    }
    y = y0;
    x = x0;
    sum = delta_y / 2;
    kwin_draw_point (canvas, x, y, pixel_value);
    while (y < y1)
    {
	++y;
	sum += delta_x;
	while (sum >= delta_y)
	{
	    x += inc;
	    sum -= delta_y;
	}
	kwin_draw_point (canvas, x, y, pixel_value);
    }
    return;
#endif

#ifndef IMPLEMENTED
    not_implemented (function_name);
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
#ifdef X11
    XColor rgb_def;
#endif
    static char function_name[] = "kwin_fill_ellipse";

    VERIFY_CANVAS (canvas);
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    if (canvas->pspage == NULL)
    {
	if (pixel_value != canvas->gcvalues.foreground)
	{
	    canvas->gcvalues.foreground = pixel_value;
	    XChangeGC (canvas->display, canvas->gc, GCForeground,
		       &canvas->gcvalues);
	}
	XFillArc (canvas->display, canvas->window, canvas->gc,
		  canvas->xoff + cx - rx, canvas->yoff + cy - ry,
		  2 * rx, 2 * ry,
		  64 * 360, 64 * 360);
	return;
    }
    /*  Do a PostScript draw  */
    rgb_def.pixel = pixel_value;
    get_colour_from_pixel (canvas, &rgb_def);
    /*  Flip vertical  */
    cy = canvas->height - 1 - cy;
    (void) psw_rgb_ellipse (canvas->pspage,
			    (double) rgb_def.red / MAX_INTENSITY,
			    (double) rgb_def.green / MAX_INTENSITY,
			    (double) rgb_def.blue / MAX_INTENSITY,
			    (double) cx / (double) (canvas->width - 1),
			    (double) cy / (double) (canvas->height - 1),
			    (double) rx / (double) (canvas->width - 1),
			    (double) ry / (double) (canvas->height - 1),
			    TRUE);
    return;
#endif

#ifndef IMPLEMENTED
    not_implemented (function_name);
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
    flag retval;
    int tmp;
    unsigned int coord_count;
    double xscale, yscale;
    double *x_arr, *y_arr;
#ifdef X11
    XColor rgb_def;
    static unsigned int num_points_allocated = 0;
    static XPoint *points = NULL;
#endif
    static char function_name[] = "kwin_fill_polygon";

    VERIFY_CANVAS (canvas);
    FLAG_VERIFY (convex);
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    if (canvas->pspage == NULL)
    {
	if (pixel_value != canvas->gcvalues.foreground)
	{
	    canvas->gcvalues.foreground = pixel_value;
	    XChangeGC (canvas->display, canvas->gc, GCForeground,
		       &canvas->gcvalues);
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
	/*  Convert world co-ordinates to pixel co-ordinates  */
	for (coord_count = 0; coord_count < num_vertices; ++coord_count)
	{
	    points[coord_count].x = canvas->xoff + point_x[coord_count];
	    points[coord_count].y = canvas->yoff + point_y[coord_count];
	}
	XFillPolygon (canvas->display, canvas->window, canvas->gc,
		      points, (int) num_vertices, convex ? Convex : Complex,
		      CoordModeOrigin);
	return (TRUE);
    }
    /*  Do a PostScript draw  */
    rgb_def.pixel = pixel_value;
    get_colour_from_pixel (canvas, &rgb_def);
    if ( ( x_arr = (double *) m_alloc (num_vertices * sizeof *x_arr) )
	== NULL )
    {
	m_error_notify (function_name, "x array");
	return (FALSE);
    }
    if ( ( y_arr = (double *) m_alloc (num_vertices * sizeof *y_arr) )
	== NULL )
    {
	m_error_notify (function_name, "y array");
	m_free ( (char *) x_arr );
	return (FALSE);
    }
    xscale = 1.0 / (double) (canvas->width - 1);
    yscale = 1.0 / (double) (canvas->height - 1);
    for (coord_count = 0; coord_count < num_vertices; ++coord_count)
    {
	x_arr[coord_count] = (double) point_x[coord_count] * xscale;
	/*  Flip vertical  */
	tmp = canvas->height - 1 - point_y[coord_count];
	y_arr[coord_count] = (double) tmp * yscale;
    }
    retval = psw_rgb_polygon (canvas->pspage,
			      (double) rgb_def.red / MAX_INTENSITY,
			      (double) rgb_def.green / MAX_INTENSITY,
			      (double) rgb_def.blue / MAX_INTENSITY,
			      x_arr, y_arr, num_vertices, TRUE);
    m_free ( (char *) x_arr );
    m_free ( (char *) y_arr );
    return (retval);
#endif

#ifndef IMPLEMENTED
    not_implemented (function_name);
    return (FALSE);
#endif
}   /*  End Function kwin_fill_polygon  */

/*PUBLIC_FUNCTION*/
void kwin_draw_string (KPixCanvas canvas, int x, int y, char *string,
		       unsigned long pixel_value, flag clear_under)
/*  This routine will draw a NULL terminated string onto a pixel canvas, using
    the default font for the canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the point must be given by  x  .
    The vertical offset of the point must be given by  y  .
    The string must be pointed to by  string  .
    The pixel value to use must be given by  pixel_value  .
    If the value of  clear_under  is TRUE, then the routine will draw both the
    foreground and background of the characters.
    The routine returns nothing.
*/
{
#ifdef X11
    int length;
#endif
    static char function_name[] = "kwin_draw_string";

    VERIFY_CANVAS (canvas);
    if (canvas->pspage != NULL)
    {
	a_func_abort (function_name, "PostScript output not yet supported");
	return;
    }
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    if (pixel_value != canvas->gcvalues.foreground)
    {
	canvas->gcvalues.foreground = pixel_value;
	XChangeGC (canvas->display, canvas->gc, GCForeground,
		   &canvas->gcvalues);
    }
    length = strlen (string);
    if (clear_under)
    {
	XDrawImageString (canvas->display, canvas->window, canvas->gc,
			  canvas->xoff + x, canvas->yoff + y,
			  string, length);
    }
    else
    {
	XDrawString (canvas->display, canvas->window, canvas->gc,
		     canvas->xoff + x, canvas->yoff + y, string, length);
    }
    return;
#endif

#ifndef IMPLEMENTED
    not_implemented (function_name);
#endif
}   /*  End Function kwin_draw_string  */

/*PUBLIC_FUNCTION*/
void kwin_draw_rectangle (canvas, x, y, width, height, pixel_value)
/*  This routine will draw a single rectangle onto a pixel canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the rectangle must be given by  x  .
    The vertical offset of the rectangle must be given by  y  .
    The width of the rectangle must be given by  width  .
    The height of the rectangle must be given by  height  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KPixCanvas canvas;
int x;
int y;
int width;
int height;
unsigned long pixel_value;
{
    int x1, y1;
    static char function_name[] = "kwin_draw_rectangle";

    VERIFY_CANVAS (canvas);
#ifdef X11
    if (canvas->pspage == NULL)
    {
	if (pixel_value != canvas->gcvalues.foreground)
	{
	    canvas->gcvalues.foreground = pixel_value;
	    XChangeGC (canvas->display, canvas->gc, GCForeground,
		       &canvas->gcvalues);
	}
	XDrawRectangle (canvas->display, canvas->window, canvas->gc,
			canvas->xoff + x, canvas->yoff + y,
			(unsigned int) width, (unsigned int) height);
	return;
    }
#endif
    /*  Do it the slow way  */
    x1 = x + width - 1;
    y1 = y + height - 1;
    kwin_draw_line (canvas, x, y, x1, y, pixel_value);
    kwin_draw_line (canvas, x, y1, x1, y1, pixel_value);
    kwin_draw_line (canvas, x, y, x, y1, pixel_value);
    kwin_draw_line (canvas, x1, y, x1, y1, pixel_value);
}   /*  End Function kwin_draw_rectangle  */

/*PUBLIC_FUNCTION*/
void kwin_fill_rectangle (canvas, x, y, width, height, pixel_value)
/*  This routine will fill a single rectangle onto a pixel canvas.
    The canvas must be given by  canvas  .
    The horizontal offset of the rectangle must be given by  x  .
    The vertical offset of the rectangle must be given by  y  .
    The width of the rectangle must be given by  width  .
    The height of the rectangle must be given by  height  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KPixCanvas canvas;
int x;
int y;
int width;
int height;
unsigned long pixel_value;
{
    int px[4], py[4];
#ifdef OS_VXMVX
    int depth;
    unsigned char *line_ptr;
#endif
    static char function_name[] = "kwin_fill_rectangle";

    VERIFY_CANVAS (canvas);
    if ( (width < 1) || (height < 1) ) return;
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    if (canvas->pspage == NULL)
    {
	if (pixel_value != canvas->gcvalues.foreground)
	{
	    canvas->gcvalues.foreground = pixel_value;
	    XChangeGC (canvas->display, canvas->gc, GCForeground,
		       &canvas->gcvalues);
	}
	XFillRectangle (canvas->display, canvas->window, canvas->gc,
			canvas->xoff + x, canvas->yoff + y,
			(unsigned int) width + 1, (unsigned int) height + 1);
	return;
    }
#endif

#ifdef OS_VXMVX
#define IMPLEMENTED
    /*  Compute origin of rectangle  */
    line_ptr = canvas->origin + (x + y * canvas->screen_width) * 4;
    depth = (canvas->vxvisual == VC_VX_VISUAL_DIRECTCOLOUR) ? 3 : 1;
    asm__kwin_fill_rectangle (depth, line_ptr, width, height, pixel_value,
			      4, canvas->screen_width * 4);
#ifdef dummy
    for (y = 0; y < height; ++y, line_ptr += canvas->screen_width * 4)
    {
	if (canvas->vxvisual == VC_VX_VISUAL_DIRECTCOLOUR)
	{
	    for (x = 0, b_ptr = line_ptr; x < width; ++x, b_ptr += 4)
	    {
		b_ptr[canvas->red_offset] = pixel_value;
		b_ptr[canvas->green_offset] = pixel_value >> 8;
		b_ptr[canvas->blue_offset] = pixel_value >> 16;
	    }
	}
	else
	{
	    for (x = 0, b_ptr = line_ptr; x < width;
		 ++x, b_ptr += 4) *b_ptr = pixel_value;
	}
    }
#endif
    return;
#endif

#ifndef IMPLEMENTED
    /*  Do it the slow way  */
    px[0] = x;
    py[0] = y;
    px[1] = x + width - 1;
    py[1] = py[0];
    px[2] = px[1];
    py[2] = y + height - 1;
    px[3] = x;
    py[3] = py[2];
    (void) kwin_fill_polygon (canvas, px, py, 4, pixel_value, TRUE);
#endif
}   /*  End Function kwin_fill_rectangle  */

/*PUBLIC_FUNCTION*/
void kwin_draw_lines (canvas, x_array, y_array, num_points, pixel_value)
/*  This routine will draw multiple connected lines onto a pixel canvas.
    The canvas must be given by  canvas  .
    The horizontal co-ordinates of the points must be pointed to by  x_array  .
    The vetical co-ordinates of the points must be pointed to by  y_array  .
    The number of points must be given by  num_points  .The number of lines
    draw is 1 less than this value.
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KPixCanvas canvas;
int *x_array;
int *y_array;
int num_points;
unsigned long pixel_value;
{
    int count;
#ifdef X11
    int x0, x1, y0, y1;
    int ipoint_count, xpoint_count;
    XColor rgb_def;
    extern int num_xpoints_allocated;
    extern XPoint *xpoints;
#endif
    static char function_name[] = "kwin_draw_lines";

    VERIFY_CANVAS (canvas);
#ifdef X11
    if (canvas->pspage == NULL)
    {
	alloc_points (canvas, num_points);
	if (pixel_value != canvas->gcvalues.foreground)
	{
	    canvas->gcvalues.foreground = pixel_value;
	    XChangeGC (canvas->display, canvas->gc, GCForeground,
		       &canvas->gcvalues);
	}
	for (ipoint_count = 0, xpoint_count = 0; ipoint_count < num_points;
	     ++ipoint_count, ++xpoint_count)
	{
	    if (xpoint_count >= num_xpoints_allocated)
	    {
		/*  Send some points now  */
		XDrawLines (canvas->display, canvas->window, canvas->gc,
			    xpoints, xpoint_count, CoordModeOrigin);
		xpoint_count = 0;
	    }
	    xpoints[xpoint_count].x = canvas->xoff + x_array[ipoint_count];
	    xpoints[xpoint_count].y = canvas->yoff + y_array[ipoint_count];
	}
	/*  Draw remaining points  */
	if (xpoint_count > 0)
	{
	    XDrawLines (canvas->display, canvas->window, canvas->gc,
			xpoints, xpoint_count, CoordModeOrigin);
	}
	return;
    }
    /*  Do a PostScript draw  */
    rgb_def.pixel = pixel_value;
    get_colour_from_pixel (canvas, &rgb_def);
    for (count = 0; count < num_points - 1; ++count)
    {
	x0 = x_array[count];
	x1 = x_array[count + 1];
	/*  Flip vertical  */
	y0 = canvas->height - 1 - y_array[count];
	y1 = canvas->height - 1 - y_array[count + 1];
	(void) psw_rgb_line ( canvas->pspage,
			     (double) rgb_def.red / MAX_INTENSITY,
			     (double) rgb_def.green / MAX_INTENSITY,
			     (double) rgb_def.blue / MAX_INTENSITY,
			     (double) x0 / (double) (canvas->width - 1),
			     (double) y0 / (double) (canvas->height - 1),
			     (double) x1 / (double) (canvas->width - 1),
			     (double) y1 / (double) (canvas->height - 1) );
    }
    return;
#else  /*  !X11  */
    /*  Do it the slow way  */
    for (count = 0; count < num_points - 1; ++count)
    {
	kwin_draw_line (canvas, x_array[count], y_array[count],
			x_array[count + 1], y_array[count + 1], pixel_value);
    }
#endif  /*  X11  */
}   /*  End Function kwin_draw_lines  */

/*PUBLIC_FUNCTION*/
void kwin_draw_ellipse (canvas, cx, cy, rx, ry, pixel_value)
/*  This routine will draw an ellipse onto a pixel canvas.
    The canvas must be given by  canvas  .
    The co-ordinates of the centre of the ellipse must be given by  cx  and  cy
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
#ifdef X11
    XColor rgb_def;
#endif
    static char function_name[] = "kwin_draw_ellipse";

    VERIFY_CANVAS (canvas);
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    if (canvas->pspage == NULL)
    {
	if (pixel_value != canvas->gcvalues.foreground)
	{
	    canvas->gcvalues.foreground = pixel_value;
	    XChangeGC (canvas->display, canvas->gc, GCForeground,
		       &canvas->gcvalues);
	}
	XDrawArc (canvas->display, canvas->window, canvas->gc,
		  canvas->xoff + cx - rx, canvas->yoff + cy - ry,
		  2 * rx, 2 * ry,
		  64 * 360, 64 * 360);
	return;
    }
    /*  Do a PostScript draw  */
    rgb_def.pixel = pixel_value;
    get_colour_from_pixel (canvas, &rgb_def);
    /*  Flip vertical  */
    cy = canvas->height - 1 - cy;
    (void) psw_rgb_ellipse (canvas->pspage,
			    (double) rgb_def.red / MAX_INTENSITY,
			    (double) rgb_def.green / MAX_INTENSITY,
			    (double) rgb_def.blue / MAX_INTENSITY,
			    (double) cx / (double) (canvas->width - 1),
			    (double) cy / (double) (canvas->height - 1),
			    (double) rx / (double) (canvas->width - 1),
			    (double) ry / (double) (canvas->height - 1),
			    FALSE);
    return;
#endif

#ifndef IMPLEMENTED
    not_implemented (function_name);
#endif
}   /*  End Function kwin_draw_ellipse  */

/*PUBLIC_FUNCTION*/
void kwin_draw_ellipses (canvas, cx, cy, rx, ry, num_ellipses, pixel_value)
/*  This routine will draw multiple ellipses onto a pixel canvas.
    The canvas must be given by  canvas  .
    The co-ordinates of the centres of the ellipses must be pointed to by  cx
    and  cy  .
    The radii must be pointed to by  rx  and  ry  .
    The number of ellipses must be given by  num_ellipses  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KPixCanvas canvas;
int *cx;
int *cy;
int *rx;
int *ry;
int num_ellipses;
unsigned long pixel_value;
{
    int count;
#ifdef X11
    int iarc_count, xarc_count;
    short angle = 64 * 360;
    extern int num_xarcs_allocated;
    extern XArc *xarcs;
#endif
    static char function_name[] = "kwin_draw_ellipses";

    VERIFY_CANVAS (canvas);
#ifdef X11
    if (canvas->pspage == NULL)
    {
	alloc_arcs (canvas, num_ellipses);
	if (pixel_value != canvas->gcvalues.foreground)
	{
	    canvas->gcvalues.foreground = pixel_value;
	    XChangeGC (canvas->display, canvas->gc, GCForeground,
		       &canvas->gcvalues);
	}
	for (iarc_count = 0, xarc_count = 0; iarc_count < num_ellipses;
	     ++iarc_count, ++xarc_count)
	{
	    if (xarc_count >= num_xarcs_allocated)
	    {
		/*  Send some arcs now  */
		XDrawArcs (canvas->display, canvas->window, canvas->gc,
			   xarcs, xarc_count);
		xarc_count = 0;
	    }
	    xarcs[xarc_count].x = canvas->xoff+cx[iarc_count]-rx[iarc_count];
	    xarcs[xarc_count].y = canvas->yoff+cy[iarc_count]-ry[iarc_count];
	    xarcs[xarc_count].width = 2 * rx[iarc_count];
	    xarcs[xarc_count].height = 2 * ry[iarc_count];
	    xarcs[xarc_count].angle1 = angle;
	    xarcs[xarc_count].angle2 = angle;
	}
	/*  Draw remaining arcs  */
	if (xarc_count > 0)
	{
	    XDrawArcs (canvas->display, canvas->window, canvas->gc,
		       xarcs, xarc_count);
	}
	return;
    }
#endif  /*  X11  */
    /*  Do it the slow way  */
    for (count = 0; count < num_ellipses; ++count)
    {
	kwin_draw_ellipse (canvas, cx[count], cy[count], rx[count], ry[count],
			   pixel_value);
    }
}   /*  End Function kwin_draw_ellipses  */

/*PUBLIC_FUNCTION*/
void kwin_fill_ellipses (canvas, cx, cy, rx, ry, num_ellipses, pixel_value)
/*  This routine will draw multiple filled ellipses onto a pixel canvas.
    The canvas must be given by  canvas  .
    The co-ordinates of the centres of the ellipses must be pointed to by  cx
    and  cy  .
    The radii must be pointed to by  rx  and  ry  .
    The number of ellipses must be given by  num_ellipses  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KPixCanvas canvas;
int *cx;
int *cy;
int *rx;
int *ry;
int num_ellipses;
unsigned long pixel_value;
{
    int count;
#ifdef X11
    int iarc_count, xarc_count;
    short angle = 64 * 360;
    extern int num_xarcs_allocated;
    extern XArc *xarcs;
#endif
    static char function_name[] = "kwin_fill_ellipses";

    VERIFY_CANVAS (canvas);
#ifdef X11
    if (canvas->pspage == NULL)
    {
	alloc_arcs (canvas, num_ellipses);
	if (pixel_value != canvas->gcvalues.foreground)
	{
	    canvas->gcvalues.foreground = pixel_value;
	    XChangeGC (canvas->display, canvas->gc, GCForeground,
		       &canvas->gcvalues);
	}
	for (iarc_count = 0, xarc_count = 0; iarc_count < num_ellipses;
	     ++iarc_count, ++xarc_count)
	{
	    if (xarc_count >= num_xarcs_allocated)
	    {
		/*  Send some arcs now  */
		XFillArcs (canvas->display, canvas->window, canvas->gc,
			   xarcs, xarc_count);
		xarc_count = 0;
	    }
	    xarcs[xarc_count].x = canvas->xoff+cx[iarc_count]-rx[iarc_count];
	    xarcs[xarc_count].y = canvas->yoff+cy[iarc_count]-ry[iarc_count];
	    xarcs[xarc_count].width = 2 * rx[iarc_count];
	    xarcs[xarc_count].height = 2 * ry[iarc_count];
	    xarcs[xarc_count].angle1 = angle;
	    xarcs[xarc_count].angle2 = angle;
	}
	/*  Fill remaining arcs  */
	if (xarc_count > 0)
	{
	    XFillArcs (canvas->display, canvas->window, canvas->gc,
		       xarcs, xarc_count);
	}
	return;
    }
#endif  /*  X11  */
    /*  Do it the slow way  */
    for (count = 0; count < num_ellipses; ++count)
    {
	kwin_fill_ellipse (canvas, cx[count], cy[count], rx[count], ry[count],
			   pixel_value);
    }
}   /*  End Function kwin_fill_ellipses  */

/*PUBLIC_FUNCTION*/
void kwin_draw_segments (canvas, x0, y0, x1, y1, num_segments, pixel_value)
/*  This routine will draw multiple disjoint lines onto a pixel canvas.
    The canvas must be given by  canvas  .
    The co-ordinates must be pointed to by  x0  ,  y0  ,  x1  and  y1  .
    The number of lines must be given by  num_segments  .
    The pixel value to use must be given by  pixel_value  .
    The routine returns nothing.
*/
KPixCanvas canvas;
int *x0;
int *y0;
int *x1;
int *y1;
int num_segments;
unsigned long pixel_value;
{
    int count;
#ifdef X11
    int isegment_count, xsegment_count;
    extern int num_xsegments_allocated;
    extern XSegment *xsegments;
#endif
    static char function_name[] = "kwin_draw_segments";

    VERIFY_CANVAS (canvas);
#ifdef X11
    if (canvas->pspage == NULL)
    {
	alloc_segments (canvas, num_segments);
	if (pixel_value != canvas->gcvalues.foreground)
	{
	    canvas->gcvalues.foreground = pixel_value;
	    XChangeGC (canvas->display, canvas->gc, GCForeground,
		       &canvas->gcvalues);
	}
	for (isegment_count = 0, xsegment_count = 0;
	     isegment_count < num_segments;
	     ++isegment_count, ++xsegment_count)
	{
	    if (xsegment_count >= num_xsegments_allocated)
	    {
		/*  Send some segments now  */
		XDrawSegments (canvas->display, canvas->window, canvas->gc,
			       xsegments, xsegment_count);
		xsegment_count = 0;
	    }
	    xsegments[xsegment_count].x1 = canvas->xoff + x0[isegment_count];
	    xsegments[xsegment_count].y1 = canvas->yoff + y0[isegment_count];
	    xsegments[xsegment_count].x2 = canvas->xoff + x1[isegment_count];
	    xsegments[xsegment_count].y2 = canvas->yoff + y1[isegment_count];
	}
	/*  Draw remaining segments  */
	if (xsegment_count > 0)
	{
	    XDrawSegments (canvas->display, canvas->window, canvas->gc,
			   xsegments, xsegment_count);
	}
	return;
    }
#endif  /*  X11  */
    /*  Do it the slow way  */
    for (count = 0; count < num_segments; ++count)
    {
	kwin_draw_line (canvas, x0[count], y0[count], x1[count], y1[count],
			pixel_value);
    }
}   /*  End Function kwin_draw_segments  */


/*  Other public routines follow  */

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
    *width = canvas->width;
    *height = canvas->height;
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
    if (cache->magic_number != CACHE_DATA_MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Invalid cache data\n");
	a_prog_bug (function_name);
    }
#ifdef X11
    if (cache->ximage != NULL)
    {
	xi_destroy_image (cache->display, cache->ximage,cache->shared);
    }
    if (cache->pixmap != (Pixmap) NULL)
    {
	XFreePixmap (cache->display, cache->pixmap);
    }
#endif
    cache->magic_number = 0;
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
    *xout = xin - canvas->xoff;
    *yout = yin - canvas->yoff;
    if ( (*xout < 0) || (*xout >= canvas->width) ||
	(*yout < 0) || (*yout >= canvas->height) ) return (FALSE);
    return (TRUE);
}   /*  End Function kwin_convert_to_canvas_coord  */

/*PUBLIC_FUNCTION*/
flag kwin_convert_from_canvas_coord (canvas, xin, yin, xout, yout)
/*  This routine will convert co-ordinates in a pixel canvas to co-ordinates in
    a lower level object (parent, ie. X window).
    The canvas must be given by  canvas  .
    The horizontal canvas co-ordinate must be given by  xin  .
    The vertical canvas co-ordinate must be given by  yin  .
    The lower level horizontal co-ordinate will be written to the storage
    pointed to by  xout  .
    The lower level vertical co-ordinate will be written to the storage pointed
    to by  xout  .
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
    static char function_name[] = "kwin_convert_from_canvas_coord";

    VERIFY_CANVAS (canvas);
    *xout = xin + canvas->xoff;
    *yout = yin + canvas->yoff;
    if ( (xin < 0) || (xin >= canvas->width) ||
	(yin < 0) || (yin >= canvas->height) ) return (FALSE);
    return (TRUE);
}   /*  End Function kwin_convert_from_canvas_coord  */

/*PUBLIC_FUNCTION*/
flag kwin_get_colour (canvas, colourname, pixel_value, red, green, blue)
/*  This routine will get (possibly allocating) a colourcell for a canvas.
    The canvas must be given by  canvas  .
    The name of the colour to get must be pointed to by  colourname  .
    The pixel value will be written to the storage pointed to by  pixel_value
    NOTE: the pixel value is valid ONLY for this canvas. ALSO: the pixel value
    becomes invalid EVERY time the canvas is refreshed/ resized. If the canvas
    is refreshed/ resized, this routine MUST be called again.
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
KPixCanvas canvas;
char *colourname;
unsigned long *pixel_value;
unsigned short *red;
unsigned short *green;
unsigned short *blue;
{
#ifdef X11
    XColor colourcell_def;
    XColor rgb_db_def;
#endif
    struct colourcell_type *colour;
    static char function_name[] = "kwin_get_colour";

    VERIFY_CANVAS (canvas);
    if (colourname == NULL)
    {
	(void) fprintf (stderr, "NULL colourname pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Search canvas for existing allocation  */
    for (colour = canvas->colours; colour != NULL; colour = colour->next)
    {
	if (strcmp (colourname, colour->name) == 0)
	{
	    *pixel_value = colour->pixel_value;
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
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    if (XAllocNamedColor (canvas->display, canvas->cmap, colourname,
			  &colourcell_def, &rgb_db_def) == 0)
    {
	(void) fprintf (stderr, "Error allocating colour: \"%s\"\n",
			colourname);
	m_free ( colour->name );
	m_free ( (char *) colour );
	return (FALSE);
    }
    colour->pixel_value = colourcell_def.pixel;
    if (red != NULL) *red = colourcell_def.red;
    if (green != NULL) *green = colourcell_def.green;
    if (blue != NULL) *blue = colourcell_def.blue;
#endif  /*  X11  */

#ifndef IMPLEMENTED
    (void) fprintf (stderr, "No colourcell support\n");
    a_prog_bug (function_name);
#endif

    /*  Support for colours exists: finish up  */
    colour->next = canvas->colours;
    canvas->colours = colour;
    *pixel_value = colour->pixel_value;
    return (TRUE);
}   /*  End Function kwin_get_colour  */

/*PUBLIC_FUNCTION*/
KPixCanvasFont kwin_load_font (KPixCanvas canvas, char *fontname)
/*  This routine will load a font which may be then used to draw text onto a
    pixel canvas.
    The handle to the display system must be specified by the pixel canvas
    given by  canvas  .Note that the font is valid for all canvases associated
    with the same display system (in X parlance, the same server).
    The name of the font must be given by  fontname  .
    The routine returns a pixel font on success, else it returns NULL.
*/
{
    KPixCanvasFont font;
#undef IMPLEMENTED
#ifdef X11
#define IMPLEMENTED
    XFontStruct *xfont;
#endif
    static char function_name[] = "kwin_load_font";

    VERIFY_CANVAS (canvas);
    if (fontname == NULL)
    {
	(void) fprintf (stderr, "NULL fontname pointer passed\n");
	a_prog_bug (function_name);
    }
#ifndef IMPLEMENTED
    (void) fprintf (stderr, "No font support\n");
    a_prog_bug (function_name);
#endif
#ifdef X11
    if ( ( xfont = XLoadQueryFont (canvas->display, fontname) ) == NULL )
    {
	(void) fprintf (stderr, "Error loading font: \"%s\"\n", fontname);
	return (NULL);
    }
#endif
    if ( ( font = (KPixCanvasFont) m_alloc (sizeof *font) ) == NULL )
    {
	m_error_notify (function_name, "font");
#ifdef X11
	XUnloadFont (canvas->display, xfont->fid);
#endif
	return (NULL);
    }
    font->magic_number = FONT_MAGIC_NUMBER;
#ifdef X11
    font->display = canvas->display;
    font->xfont = xfont;
#endif
    return (font);
}   /*  End Function kwin_load_font  */

/*PUBLIC_FUNCTION*/
void kwin_get_string_size (font, string, va_alist)
/*  This routine will determine the size of a string.
    The font must be given by  font  .
    The string must be pointed to by  string  .
    The list of parameter attribute-key attribute-value pairs must follow. See
    the header file for details on defined attributes.
    This list must be terminated with the value  KWIN_STRING_END  .
    The routine returns nothing.
*/
KPixCanvasFont font;
char *string;
va_dcl
{
    va_list arg_ptr;
    int length, itmp;
    unsigned int att_key;
#ifdef X11
    XFontStruct *xfont;
#endif
    static char function_name[] = "kwin_get_string_size";

    va_start (arg_ptr);
    VERIFY_FONT (font);
    if (string == NULL)
    {
	(void) fprintf (stderr, "NULL string pointer passed\n");
	a_prog_bug (function_name);
    }
#ifndef X11
    (void) fprintf (stderr, "No font support\n");
    a_prog_bug (function_name);
#endif
    length = strlen (string);
#ifdef X11
    xfont = font->xfont;
    while ( ( att_key = va_arg (arg_ptr, unsigned int) ) != KWIN_STRING_END )
    {
	switch (att_key)
	{
	  case KWIN_STRING_WIDTH:
	    *( va_arg (arg_ptr, int *) ) = XTextWidth (xfont, string, length);
	    break;
	  case KWIN_STRING_HEIGHT:
	    *( va_arg (arg_ptr, int *) ) = xfont->ascent + xfont->descent;
	    break;
	  case KWIN_STRING_ASCENT:
	    *( va_arg (arg_ptr, int *) ) = xfont->ascent;
	    break;
	  case KWIN_STRING_DESCENT:
	    *( va_arg (arg_ptr, int *) ) = xfont->descent;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
#endif  /*  X11  */
    va_end (arg_ptr);
}   /*  End Function kwin_get_string_size  */


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
    canvas->magic_number = CANVAS_MAGIC_NUMBER;
    canvas->xoff = 0;
    canvas->yoff = 0;
    canvas->width = 0;
    canvas->height = 0;
    canvas->refresh_list = NULL;
    canvas->position_list = NULL;
    canvas->visible = TRUE;
#ifdef X11
    canvas->display = NULL;
    canvas->window = 0;
    canvas->shm_available = FALSE;
    canvas->gc = 0;
#endif
#ifdef OS_VXMVX
    canvas->vxvisual = VC_VX_VISUAL_UNDEFINED;
    canvas->frame_buffer = NULL;
    canvas->screen_width = -1;
    canvas->screen_height = -1;
    canvas->origin = NULL;
    canvas->background = 0;
#endif
    canvas->pspage = NULL;
    canvas->font = NULL;
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
    if ( ( cache = (KPixCanvasImageCache) m_alloc (sizeof *cache) ) == NULL )
    {
	m_error_notify (function_name, "cache data structure");
	return (NULL);
    }
    cache->magic_number = CACHE_DATA_MAGIC_NUMBER;
    cache->canvas = canvas;
#ifdef X11
    cache->display = canvas->display;
    cache->ximage = NULL;
    cache->pixmap = (Pixmap) NULL;
    cache->shared = FALSE;
    cache->width = 0;
    cache->height = 0;
    cache->p_width = 0;
    cache->p_height = 0;
#endif
    return (cache);
}   /*  End Function alloc_cache_data_struct  */

#ifndef OS_VXMVX
static flag child_position_event_func (parent, x, y, event_code,
				       event_info, f_info)
/*  This routine is a position event consumer for a pixel canvas.
    The parent pixel canvas is given by  parent  .
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
KPixCanvas parent;
int x;
int y;
unsigned int event_code;
void *event_info;
void **f_info;
{
    KPixCanvas child;
    flag consumed;
    struct child_type *handle;
    static char function_name[] = "child_position_event_func";

    VERIFY_CANVAS (parent);
    if ( (handle = (struct child_type *) *f_info) == NULL )
    {
	(void) fprintf (stderr, "NULL child_type structure pointer\n");
	a_prog_bug (function_name);
    }
    child = handle->child;
    consumed = kwin_process_position_event (child, x, y, FALSE,
					    event_code, event_info);
    if (consumed && handle->absorb_events) return (TRUE);
    return (FALSE);
}   /*  End Function child_position_event_func  */
#endif /*  OS_VXMVX  */

static KPixCanvasImageCache size_cache (KPixCanvas canvas,
					KPixCanvasImageCache *cache_ptr,
					unsigned int width,
					unsigned int height)
/*  This routine will allocate image cache data.
    The canvas must be given by  canvas  .
    The routine will produce cache data which will vastly increase the speed of
    subsequent operations on this data. The routine will write a pointer to
    this data to the storage pointed to by  cache_ptr  .Prior to process
    exit, a call MUST be made to  kwin_free_cache_data  ,otherwise shared
    memory segments could remain after the process exits.
    The routine returns the cache data on success, else it returns NULL.
*/
{
    KPixCanvasImageCache cache;
    flag allocate = FALSE;
    static flag first_time = TRUE;
    static flag enable_pixmaps = TRUE;
    static char function_name[] = "cache_size";

    VERIFY_CANVAS (canvas);
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
	if ( ( cache = alloc_cache_data_struct (canvas) ) == NULL )
	{
	    m_error_notify (function_name, "cache data structure");
	    return (NULL);
	}
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
#ifdef X11
    /*  Check if existing XImage is big enough  */
    if (cache->ximage == NULL)
    {
	allocate = TRUE;
    }
    else
    {
	if ( (width > cache->ximage->width) ||
	    (height > cache->ximage->height) )
	{
	    /*  XImage is too small  */
	    xi_destroy_image (cache->display, cache->ximage, cache->shared);
	    cache->ximage = NULL;
	    cache->shared = FALSE;
	    allocate = TRUE;
	}
    }
    if (allocate)
    {
	/*  Allocate XImage  */
	if (canvas->shm_available)
	{
	    cache->shared = TRUE;
	}
	if ( ( cache->ximage = xi_create_image (canvas->display,
						canvas->window, width, height,
						&cache->shared) )
	    == NULL )
	{
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL)
	    {
		*cache_ptr = NULL;
	    }
	    return (NULL);
	}
    }
    if (cache->shared)
    {
	/*  Shared memory XImage: no need for pixmap  */
	if (cache->pixmap) XFreePixmap (canvas->display,
					    cache->pixmap);
	cache->pixmap = (Pixmap) NULL;
    }
    else
    {
	if (cache->pixmap == (Pixmap) NULL)
	{
	    /*  Create pixmap for the first time  */
	    if (enable_pixmaps)
	    {
		cache->p_width = width;
		cache->p_height = height;
		cache->pixmap = XCreatePixmap (canvas->display,
						 canvas->window,
						 width, height,
						 canvas->depth);
	    }
	}
	else
	{
	    /*  Have existing pixmap  */
	    if ( (width > cache->p_width) || (height > cache->p_height) )
	    {
		/*  Existing pixmap is too small: free old and create new  */
		XFreePixmap (canvas->display, cache->pixmap);
	    	/*  Create the pixmap  */
		cache->p_width = width;
		cache->p_height = height;
		cache->pixmap = XCreatePixmap (canvas->display, canvas->window,
					       width, height, canvas->depth);
	    }
	}
    }
#endif  /*  X11  */
    return (cache);
}   /*  End Function size_cache  */

static void not_implemented (char *s)
{
    (void) fprintf (stderr, "%s: not implemented yet\n", s);
}   /*  End Function not_implemented  */

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
    KPixCanvas canvas;
    void (*func) ();
    static char function_name[] = "refresh_event_func";

    canvas = (KPixCanvas) object;
    VERIFY_CANVAS (canvas);
    func = ( void (*) () ) client2_data;
    (*func) (canvas, canvas->width, canvas->height, client1_data,
	     canvas->pspage);
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
    KPixCanvas canvas;
    flag (*func) ();
    struct position_struct *data;
    static char function_name[] = "position_event_func";

    canvas = (KPixCanvas) object;
    VERIFY_CANVAS (canvas);
    data = (struct position_struct *) call_data;
    func = ( flag (*) () ) client2_data;
    return ( (*func) (canvas, data->x, data->y, data->event_code,
		      data->e_info, client1_data) );
}   /*  End Function position_event_func  */

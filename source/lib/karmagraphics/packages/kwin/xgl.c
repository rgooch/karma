/*LINTLIBRARY*/
/*  xgl.c

    This code provides KPixCanvas objects.

    Copyright (C) 1995-1996  Richard Gooch

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
    contains the XGL code.


    Written by      Richard Gooch   22-SEP-1995

    Updated by      Richard Gooch   22-SEP-1995

    Updated by      Richard Gooch   20-OCT-1995: Created <resize> function.

    Updated by      Richard Gooch   28-JAN-1996: Added test for
  KWIN_XGL_FORCE_STEREO environment variable.

    Updated by      Richard Gooch   1-FEB-1996: Created <convert_colour>
  routine to do colour conversion right.

    Updated by      Richard Gooch   4-FEB-1996: Worked around bug in XGL in
  <size_cache>.

    Updated by      Richard Gooch   19-FEB-1996: Modified interface for
  <clear_area> to support width and height parameters.

    Updated by      Richard Gooch   21-FEB-1996: Pass <resize> func when
  creating generic pixel canvas: forgot to do this when <resize> created.

    Updated by      Richard Gooch   23-FEB-1996: Clip (sub)image to cache size
  in <draw_cached_image> and added <<wait>> parameter.

    Updated by      Richard Gooch   13-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   26-APR-1996: Moved stub functions in.


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_kwin.h>
#include <karma_kwin_hooks.h>
#include <karma_xi.h>
#ifdef X11
#  include <X11/Xutil.h>
#endif
#include <karma_imw.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_r.h>


#if defined(X11) && defined(HAS_XGL)
#include <xgl/xgl.h>


#define CANVAS_MAGIC_NUMBER (unsigned int) 249832902
#define CACHE_DATA_MAGIC_NUMBER (unsigned int) 38276323

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

/*  Structure declarations  */

typedef struct
{
    Display *display;
    Window window;
    Colormap cmap;
    XVisualInfo vinfo;
    Xgl_win_ras win_ras;
    Xgl_object render_context;
    int xoff;
    int yoff;
    int width;
    int height;
    int xwin_width;
    int xwin_height;
    struct colourcell_type *colours;
    struct Xglcanvas_type *xgl_active;
    struct Xglcanvas_type *xgl_mono;
    struct Xglcanvas_type *xgl_left;
    struct Xglcanvas_type *xgl_right;
} *SharedCanvas;

typedef struct Xglcanvas_type
{
    unsigned int magic_number;
    KPixCanvas pixcanvas;
    SharedCanvas shared_canvas;
} *XglCanvas;

struct cache_data_type
{
    KPixCanvas pixcanvas;
    /*  Specific routines define bigger strucures  */
    XglCanvas xglcanvas;
    unsigned int magic_number;
    unsigned int width;          /*  The width of the image in the cache   */
    unsigned int height;         /*  The height of the image in the cache  */
    unsigned int ras_width;      /*  The width of the XGL memory raster    */
    unsigned int ras_height;     /*  The height of the XGL memory raster   */
    Xgl_ras raster;
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
static Xgl_sys_state xgl_system_state = NULL;

/*  Mandatory functions  */
STATIC_FUNCTION (flag draw_point, (void *v_canvas, double x, double y,
				   unsigned long pixel_value) );
STATIC_FUNCTION (void *create_child, (void *v_parent, KPixCanvas child) );
STATIC_FUNCTION (flag clear_area, (void *v_canvas, int x, int y,
				   int width, int height) );
/*  Optional hook functions  */
STATIC_FUNCTION (flag draw_pc_image,
		 (XglCanvas xglcanvas,
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
		 (XglCanvas xglcanvas,
		  int x_off, int y_off, int x_pixels, int y_pixels,
		  CONST unsigned char *red_slice,
		  CONST unsigned char *green_slice,
		  CONST unsigned char *blue_slice,
		  CONST uaddr *hoffsets, CONST uaddr *voffsets,
		  unsigned int width, unsigned int height,
		  KPixCanvasImageCache *cache_ptr) );
STATIC_FUNCTION (flag draw_cached_image,
		 (KPixCanvasImageCache cache, flag wait,
		  int parent_x_off, int parent_y_off,
		  int image_width, int image_height,
		  int image_x_off, int image_y_off /*,
		  int canvas_x_off, int canvas_y_off,
		  int canvas_width, int canvas_height */) );
STATIC_FUNCTION (void free_cache_data, (KPixCanvasImageCache cache) );
STATIC_FUNCTION (flag draw_line, (XglCanvas xglcanvas,
				  double x0, double y0, double x1, double y1,
				  unsigned long pixel_value) );
STATIC_FUNCTION (flag draw_arc,
		 (XglCanvas xglcanvas,
		  double x, double y, double width, double height,
		  int angle1, int angle2, unsigned long pixel_value,
		  flag fill) );
STATIC_FUNCTION (flag draw_polygon,
		 (XglCanvas xglcanvas, double *x_arr, double *y_arr,
		  unsigned int num_vertices, unsigned long pixel_value,
		  flag convex, flag fill) );
STATIC_FUNCTION (flag draw_string,
		 (XglCanvas xglcanvas, double x, double y, CONST char *string,
		  unsigned long pixel_value, flag clear_under) );
STATIC_FUNCTION (flag draw_rectangle,
		 (XglCanvas xglcanvas,
		  double x, double y, double width, double height,
		  unsigned long pixel_value, flag fill) );
STATIC_FUNCTION (flag draw_lines,
		 (XglCanvas xglcanvas, double *x_arr, double *y_arr,
		  unsigned int num_points, unsigned long pixel_value) );
STATIC_FUNCTION (flag draw_arcs,
		 (XglCanvas xglcanvas,
		  double *x, double *y, double *width, double *height,
		  int *angle1, int *angle2, unsigned int num_ellipses,
		  unsigned long pixel_value, flag fill) );
STATIC_FUNCTION (flag draw_segments,
		 (XglCanvas xglcanvas,
		  double *x0, double *y0, double *x1, double *y1,
		  unsigned int num_segments, unsigned long pixel_value) );
STATIC_FUNCTION (flag get_colour,
		 (XglCanvas xglcanvas, CONST char *colourname,
		  unsigned long *pixel_value, unsigned short *red,
		  unsigned short *green, unsigned short *blue) );
STATIC_FUNCTION (XFontStruct *load_font,
		 (XglCanvas xglcanvas, CONST char *fontname) );
STATIC_FUNCTION (flag get_string_size,
		 (XFontStruct *x11font, CONST char *string, va_list argp) );
STATIC_FUNCTION (void set_font, (XglCanvas xglcanvas, XFontStruct *x11font) );
STATIC_FUNCTION (flag query_colourmap,
		 (XglCanvas canvas, unsigned long *pixels,
		  unsigned short *reds, unsigned short *greens,
		  unsigned short *blues, unsigned int num_colours) );
STATIC_FUNCTION (flag resize,
		 (XglCanvas xglcanvas, int xoff, int yoff,
		  int width, int height) );
/*  Private functions  */
STATIC_FUNCTION (void initialise, () );
STATIC_FUNCTION (void set_pixel_in_gc,
		 (XglCanvas xglcanvas, unsigned long pixel_value) );
STATIC_FUNCTION (XVisualInfo *get_visinfo_for_visual,
		 (Display *dpy, Visual *visual) );
STATIC_FUNCTION (KPixCanvasImageCache size_cache,
		 (XglCanvas xglcanvas, KPixCanvasImageCache *cache_ptr,
		  unsigned int width, unsigned int height) );
STATIC_FUNCTION (void alloc_points, (XglCanvas xglcanvas, int num_points) );
STATIC_FUNCTION (void alloc_arcs, (XglCanvas xglcanvas, int num_arcs) );
STATIC_FUNCTION (void alloc_segments, (XglCanvas xglcanvas,int num_segments) );
STATIC_FUNCTION (void set_active_canvas, (XglCanvas xglcanvas) );
STATIC_FUNCTION (void convert_colour,
		 (XglCanvas xglcanvas, Xgl_color *xgl_colour,
		  unsigned long pixel_value) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
unsigned int kwin_xgl_test_stereo (Display *display, Window window)
/*  [SUMMARY] Test if stereo supported using XGL.
    [PURPOSE] This routine will determine if stereoscopic display is supported
    using XGL on a particular display.
    [NOTE] This routine is only available with the X window system.
    <display> The X display.
    <window> The window ID of a window of the same class as the desired stereo
    window.
    [RETURNS] A status code. See [<KWIN_XGL_STATUS>] for a list of values.
*/
{
    Xgl_obj_desc win_desc;
    Xgl_X_window xgl_x_win;
    Xgl_inquire *win_caps;
    extern Xgl_sys_state xgl_system_state;
    static flag first_time = TRUE;
    static flag force_stereo = FALSE;
    static char function_name[] = "kwin_xgl_test_stereo";

    if (first_time)
    {
	initialise ();
	if ( ( r_getenv ("KWIN_XGL_FORCE_STEREO") ) != NULL )
	
	{
	    force_stereo = FALSE;
	    (void) fprintf (stderr,
			    "KWIN_XGL_FORCE_STEREO environment variable exists\n");
	    (void) fprintf (stderr, "Forcing XGL to create stereo windows\n");
	}
	first_time = FALSE;
    }
    xgl_x_win.X_display = display;
    xgl_x_win.X_window = window;
    xgl_x_win.X_screen = DefaultScreen (display);
    win_desc.win_ras.type = XGL_WIN_X | XGL_WIN_X_PROTO_DEFAULT;
    win_desc.win_ras.desc = &xgl_x_win;
    if ( ( win_caps = xgl_inquire (xgl_system_state, &win_desc) ) == NULL )
    {
	(void) fprintf (stderr, "%s: cannot xgl_inquire\n", function_name);
	return (KWIN_XGL_STEREO_NOT_AVAILABLE);
    }
    if (win_caps->stereo)
    {
	free ( (char *) win_caps );
	return (KWIN_XGL_STEREO_AVAILABLE);
    }
    free ( (char *) win_caps );
    if (force_stereo) return (KWIN_XGL_STEREO_AVAILABLE);
    return (KWIN_XGL_STEREO_NOT_AVAILABLE);
}   /*  End Function kwin_xgl_test_stereo  */

/*PUBLIC_FUNCTION*/
flag kwin_xgl_create_stereo (Display *display, Window window,
			     int xoff, int yoff, int width, int height,
			     KPixCanvas *mono,
			     KPixCanvas *left, KPixCanvas *right)
/*  [SUMMARY] Create a stereo pixel canvas using XGL.
    [PURPOSE] This routine will create a stereo pixel canvas, ready for
    drawing, from an X window, using the XGL library. Note that the origin of
    a KPixCanvas is the upper-left corner.
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
    [RETURNS] TRUE on success, else FALSE.
*/
{
    SharedCanvas shared_canvas;
    Xgl_obj_desc win_desc;
    Xgl_X_window xgl_x_win;
    Xgl_color colour;
    Xgl_cmap xgl_cmap;
    Xgl_win_ras win_ras;
    XWindowAttributes window_attributes;
    int im_byte_order, count;
    unsigned int visual;
    unsigned int red_offset, green_offset, blue_offset;
    unsigned long im_red_mask, im_green_mask, im_blue_mask;
    unsigned char *ch_ptr;
    XVisualInfo *vinfo;
    extern Xgl_sys_state xgl_system_state;
    static Xgl_usgn32 pixel_values[256];
    static char function_name[] = "kwin_xgl_create_stereo";

    initialise ();
    for (count = 0; count < 256; ++count) pixel_values[count] = count;
    *mono = NULL;
    *left = NULL;
    *right = NULL;
    if (BitmapUnit (display) != 32)
    {
	(void) fprintf (stderr, "BitmapUnit must be 32\n");
	return (FALSE);
    }
    im_byte_order = ImageByteOrder (display);
    /*  Get X colourmap  */
    XGetWindowAttributes (display, window, &window_attributes);
    if (window_attributes.colormap == None)
    {
	(void) fprintf (stderr, "Error: window has no colourmap\n");
	a_prog_bug (function_name);
    }
    if ( ( shared_canvas = (SharedCanvas) m_alloc (sizeof *shared_canvas) )
	== NULL )
    {
	m_error_notify (function_name, "XGL shared canvas");
	return (FALSE);
    }
    m_clear ( (char *) shared_canvas, sizeof *shared_canvas );
    shared_canvas->display = display;
    shared_canvas->window = window;
    shared_canvas->cmap = window_attributes.colormap;
    shared_canvas->xoff = xoff;
    shared_canvas->yoff = yoff;
    shared_canvas->width = width;
    shared_canvas->height = height;
    shared_canvas->xwin_width = window_attributes.width;
    shared_canvas->xwin_height = window_attributes.height;
    vinfo = get_visinfo_for_visual (display, window_attributes.visual);
    if (window_attributes.depth != vinfo->depth)
    {
	(void) fprintf (stderr, "Window depth: %d is not visual depth: %d\n",
			window_attributes.depth, vinfo->depth);
	a_prog_bug (function_name);
    }
    m_copy ( (char *) &shared_canvas->vinfo, (char *) vinfo, sizeof *vinfo );
    XFree ( (void *) vinfo );
    switch (shared_canvas->vinfo.class)
    {
      case DirectColor:
      case TrueColor:
	if (shared_canvas->vinfo.colormap_size != 256)
	{
	    (void) fprintf (stderr,
			    "Colourmap size: %d for %s visual is not 256\n",
			    shared_canvas->vinfo.colormap_size,
			    (shared_canvas->vinfo.class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) shared_canvas );
	    return (FALSE);
	}
	if (window_attributes.depth != 24)
	{
	    (void) fprintf (stderr,
			    "Depth: %u for %s visual is not 24\n",
			    window_attributes.depth,
			    (shared_canvas->vinfo.class ==
			     DirectColor) ? "DirectColour" : "TrueColour");
	    m_free ( (char *) shared_canvas );
	    return (FALSE);
	}
	/*  Compute the byte offsets in a word required for drawing images
	    (images are stored in server native format for efficiency).
	    Note that the masks in the vinfo structure are in client order.  */
	red_offset = (im_byte_order == MSBFirst) ? 3 : 0;
	green_offset = (im_byte_order == MSBFirst) ? 2 : 1;
	blue_offset = (im_byte_order == MSBFirst) ? 1 : 2;
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
    switch (shared_canvas->vinfo.class)
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
			shared_canvas->vinfo.class);
	a_prog_bug (function_name);
	break;
    }
    if ( ( shared_canvas->xgl_mono = (XglCanvas)
	  m_alloc (sizeof *shared_canvas->xgl_mono) ) == NULL )
    {
	m_abort (function_name, "mono XglCanvas");
    }
    if ( ( shared_canvas->xgl_left = (XglCanvas)
	  m_alloc (sizeof *shared_canvas->xgl_left) ) == NULL )
    {
	m_abort (function_name, "left XglCanvas");
    }
    if ( ( shared_canvas->xgl_right = (XglCanvas)
	  m_alloc (sizeof *shared_canvas->xgl_right) ) == NULL )
    {
	m_abort (function_name, "right XglCanvas");
    }
    if ( (visual == KWIN_VISUAL_DIRECTCOLOUR) ||
	(visual == KWIN_VISUAL_TRUECOLOUR ) )
    {
	*mono = ( kwin_create_generic
		 (shared_canvas->xgl_mono, xoff, yoff, width, height,
		  window_attributes.depth, visual, TRUE, draw_point,
		  create_child, clear_area,
		  KWIN_ATT_PIX_RED_MASK, 0xff,
		  KWIN_ATT_PIX_GREEN_MASK, 0xff00,
		  KWIN_ATT_PIX_BLUE_MASK, 0xff0000,
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
		  KWIN_FUNC_GET_COLOUR, get_colour,
		  KWIN_FUNC_RESIZE, resize,
		  KWIN_ATT_END) );
	*left = ( kwin_create_generic
		 (shared_canvas->xgl_left, xoff, yoff, width, height,
		  window_attributes.depth, visual, TRUE, draw_point,
		  create_child, clear_area,
		  KWIN_ATT_PIX_RED_MASK, 0xff,
		  KWIN_ATT_PIX_GREEN_MASK, 0xff00,
		  KWIN_ATT_PIX_BLUE_MASK, 0xff0000,
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
		  KWIN_FUNC_GET_COLOUR, get_colour,
		  KWIN_FUNC_RESIZE, resize,
		  KWIN_ATT_END) );
	*right = ( kwin_create_generic
		  (shared_canvas->xgl_right, xoff, yoff, width, height,
		   window_attributes.depth, visual, TRUE, draw_point,
		   create_child, clear_area,
		   KWIN_ATT_PIX_RED_MASK, 0xff,
		   KWIN_ATT_PIX_GREEN_MASK, 0xff00,
		   KWIN_ATT_PIX_BLUE_MASK, 0xff0000,
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
		   KWIN_FUNC_GET_COLOUR, get_colour,
		   KWIN_FUNC_RESIZE, resize,
		   KWIN_ATT_END) );
	xgl_cmap = NULL;
    }
    else
    {
	/*  PseudoColour  */
	*mono = ( kwin_create_generic
		 (shared_canvas->xgl_mono, xoff, yoff, width, height,
		  window_attributes.depth,
		  visual, TRUE, draw_point, create_child, clear_area,
		  KWIN_FUNC_DRAW_PC_IMAGE, draw_pc_image,
		  KWIN_FUNC_DRAW_RGB_IMAGE, draw_rgb_image,
		  KWIN_FUNC_DRAW_CACHED_IMAGE, draw_cached_image,
		  KWIN_FUNC_FREE_CACHE_DATA, free_cache_data,
		  KWIN_FUNC_DRAW_LINE, draw_line,
		  KWIN_FUNC_GET_COLOUR, get_colour,
		  KWIN_FUNC_RESIZE, resize,
		  KWIN_ATT_END) );
	*left = ( kwin_create_generic
		 (shared_canvas->xgl_left, xoff, yoff, width, height,
		  window_attributes.depth,
		  visual, TRUE, draw_point, create_child, clear_area,
		  KWIN_FUNC_DRAW_PC_IMAGE, draw_pc_image,
		  KWIN_FUNC_DRAW_RGB_IMAGE, draw_rgb_image,
		  KWIN_FUNC_DRAW_CACHED_IMAGE, draw_cached_image,
		  KWIN_FUNC_FREE_CACHE_DATA, free_cache_data,
		  KWIN_FUNC_DRAW_LINE, draw_line,
		  KWIN_FUNC_GET_COLOUR, get_colour,
		  KWIN_FUNC_RESIZE, resize,
		  KWIN_ATT_END) );
	*right = ( kwin_create_generic
		  (shared_canvas->xgl_right, xoff, yoff, width, height,
		   window_attributes.depth,
		   visual, TRUE, draw_point, create_child, clear_area,
		   KWIN_FUNC_DRAW_PC_IMAGE, draw_pc_image,
		   KWIN_FUNC_DRAW_RGB_IMAGE, draw_rgb_image,
		   KWIN_FUNC_DRAW_CACHED_IMAGE, draw_cached_image,
		   KWIN_FUNC_FREE_CACHE_DATA, free_cache_data,
		   KWIN_FUNC_DRAW_LINE, draw_line,
		   KWIN_FUNC_GET_COLOUR, get_colour,
		   KWIN_FUNC_RESIZE, resize,
		   KWIN_ATT_END) );
	xgl_cmap = xgl_object_create (xgl_system_state, XGL_CMAP, NULL, NULL);
	xgl_object_set (xgl_cmap,
			XGL_CMAP_NAME, window_attributes.colormap,
			XGL_CMAP_COLOR_TABLE_SIZE, 256,
			NULL);
    }
    if ( (*mono == NULL) || (*left == NULL) || (*right == NULL) )
    {
	m_abort (function_name, "XglCanvas");
    }
    shared_canvas->xgl_mono->magic_number = CANVAS_MAGIC_NUMBER;
    shared_canvas->xgl_mono->pixcanvas = *mono;
    shared_canvas->xgl_mono->shared_canvas = shared_canvas;
    shared_canvas->xgl_left->magic_number = CANVAS_MAGIC_NUMBER;
    shared_canvas->xgl_left->pixcanvas = *left;
    shared_canvas->xgl_left->shared_canvas = shared_canvas;
    shared_canvas->xgl_right->magic_number = CANVAS_MAGIC_NUMBER;
    shared_canvas->xgl_right->pixcanvas = *right;
    shared_canvas->xgl_right->shared_canvas = shared_canvas;
    xgl_x_win.X_display = display;
    xgl_x_win.X_window = window;
    xgl_x_win.X_screen = DefaultScreen (display);
    win_desc.win_ras.type = XGL_WIN_X | XGL_WIN_X_PROTO_DEFAULT;
    win_desc.win_ras.desc = &xgl_x_win;
    if (xgl_cmap == NULL)
    {
	win_ras = xgl_object_create (xgl_system_state,
				     XGL_WIN_RAS, &win_desc,
				     XGL_WIN_RAS_BACKING_STORE, FALSE,
				     NULL);
    }
    else
    {
	win_ras = xgl_object_create (xgl_system_state, XGL_WIN_RAS, &win_desc,
				     XGL_DEV_COLOR_MAP, xgl_cmap,
				     XGL_WIN_RAS_PIXEL_MAPPING, pixel_values,
				     XGL_WIN_RAS_BACKING_STORE, FALSE,
				     NULL);
    }
    if ( (shared_canvas->win_ras = win_ras) == NULL )
    {
	(void) fprintf (stderr, "%s: cannot create Xgl_win_ras\n",
			function_name);
	return (FALSE);
    }
    if (visual == KWIN_VISUAL_PSEUDOCOLOUR)
    {
	colour.index = 200;
    }
    else
    {
	colour.rgb.r = 0.0;
	colour.rgb.g = 0.0;
	colour.rgb.b = 0.0;
    }
    if ( ( shared_canvas->render_context =
	  xgl_object_create (xgl_system_state, XGL_3D_CTX, NULL,
			     XGL_CTX_DEVICE, shared_canvas->win_ras,
			     XGL_CTX_DEFERRAL_MODE, XGL_DEFER_ASAP,
			     XGL_CTX_VDC_MAP, XGL_VDC_MAP_DEVICE,
			     XGL_CTX_BACKGROUND_COLOR, &colour,
			     XGL_CTX_NEW_FRAME_ACTION, XGL_CTX_NEW_FRAME_CLEAR,
			     NULL) ) == NULL )
    {
	(void) fprintf (stderr, "%s: cannot create XGL_3D_CTX\n",
			function_name);
	return (FALSE);
    }
    xgl_context_new_frame (shared_canvas->render_context);
    shared_canvas->xgl_active = shared_canvas->xgl_mono;
    return (TRUE);
}   /*  End Function kwin_xgl_create_stereo  */


/*  Mandatory hook functions follow  */

static flag draw_point (void *v_canvas, double x, double y,
			unsigned long pixel_value)
/*  [PURPOSE] This routine will draw a point onto an XGL canvas.
    <v_canvas> The XGL canvas.
    <x> The horizontal offset of the point.
    <y> The vertical offset of the point.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    XglCanvas xglcanvas = (XglCanvas) v_canvas;
    Xgl_pt_list point_list;
    Xgl_pt_color_f2d point;
    static char function_name[] = "__kwin_xgl_draw_point";

    VERIFY_CANVAS (xglcanvas);
    set_active_canvas (xglcanvas);
    point_list.pt_type = XGL_PT_COLOR_F2D;
    point_list.bbox = NULL;
    point_list.num_pts = 1;
    point_list.pts = &point;
    /*  Have to set the point colour. Pity XGL has such a cumbersome scheme  */
    convert_colour (xglcanvas, &point.color, pixel_value);
    point.x = x;
    point.y = y;
    xgl_multimarker (xglcanvas->shared_canvas->render_context, &point_list);
    return (TRUE);
}   /*  End Function draw_point  */

static void *create_child (void *v_parent, KPixCanvas child)
/*  [PURPOSE] This routine will create a child XGL canvas.
    <v_parent> The parent XGL canvas.
    [RETURNS] The child X11 canvas on success, else NULL.
*/
{
    XglCanvas xglparent = (XglCanvas) v_parent;
    static char function_name[] = "__kwin_xgl_create_child";

    VERIFY_CANVAS (xglparent);
    (void) fprintf (stderr, "%s: child creation not supported yet\n",
		    function_name);
    return (NULL);
}   /*  End Function create_child  */

static flag clear_area (void *v_canvas, int x, int y, int width, int height)
/*  [PURPOSE] This routine will clear an area in an XGL canvas.
    <v_canvas> The XGL canvas.
    <x> The horizontal offset of the area.
    <y> The vertical offset of the area.
    <width> The width of the area.
    <height> The height of the area.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    XglCanvas xglcanvas = (XglCanvas) v_canvas;
    SharedCanvas sc;
    static char function_name[] = "__kwin_xgl_clear_area";

    VERIFY_CANVAS (xglcanvas);
    sc = xglcanvas->shared_canvas;
    set_active_canvas (xglcanvas);
/*
    (void) fprintf (stderr, "%s...\n", function_name);
*/
    if ( (x == 0) && (y == 0) && (width == sc->xwin_width) &&
	 (height == sc->xwin_height) )
    {
	(void) fprintf (stderr, "%s clear all...\n", function_name);
	xgl_context_new_frame (sc->render_context);
    }
    return (TRUE);
}   /*  End Function clear_area  */


/*  Optional hook functions follow  */

static flag draw_pc_image (XglCanvas xglcanvas, int x_off, int y_off,
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
    onto a XGL canvas. This slice may be tiled. The slice is a PseudoColour
    image.
    <xglcanvas> The XGL canvas.
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
    unsigned int count, depth;
    uaddr im_red_offset, im_green_offset, im_blue_offset;
    unsigned char *ub_ptr, *raster_base;
    unsigned char ub_pixels[256];
    static char function_name[] = "__kwin_xgl_draw_pc_image";

    long compute_time, dump_time;
    struct timeval start_time;
    struct timeval stop_time;
    static struct timezone tz = {0, 0};

    VERIFY_CANVAS (xglcanvas);
/*
    (void) fprintf (stderr, "%s: xoff: %d  yoff: %d  x_pix: %d  y_pix: %d\n",
		    function_name, x_off, y_off, x_pixels, y_pixels);
    (void) fprintf (stderr, "width: %d  height: %d\n", width, height);
*/
    if ( ( cache = size_cache (xglcanvas, cache_ptr, x_pixels, y_pixels) )
	== NULL )
    {
	return (FALSE);
    }
    xgl_object_get (cache->raster, XGL_MEM_RAS_IMAGE_BUFFER_ADDR,&raster_base);
    depth = xglcanvas->shared_canvas->vinfo.depth;
    if (depth == 24)
    {
	if (gettimeofday (&start_time, &tz) != 0)
	{
	    (void) fprintf (stderr, "Error getting time of day\n");
	    (void) exit (1);
	}
    }
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
    if ( (width == x_pixels) && (height == y_pixels) && (depth == 8) )
    {
	/*  New drawing routines  */
	/*  Note the casts from (uaddr *) to (iaddr *) for the offset
	    arrays. This is dodgy, but it should work.  */
	if ( !imw_to8_oi (raster_base, 1, cache->ras_width,
			  x_pixels, y_pixels,
			  slice, (iaddr *) hoffsets, (iaddr *) voffsets,
			  type, conv_type,
			  num_pixels, ub_pixels,
			  blank_pixel, min_sat_pixel, max_sat_pixel,
			  i_min, i_max, iscale_func, iscale_info) )
	{
	    (void) fprintf (stderr, "Error drawing image into XglCanvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
    }
    else if ( (width == x_pixels) && (height == y_pixels) &&
	     (depth == 24) && (num_pixels == 65536) &&
	     (type == K_USHORT) && (0 == (int) i_min) &&
	     (65535 == (int) i_max) )
    {
	ub_ptr = (unsigned char *) pixel_values;
	kwin_get_attributes (xglcanvas->pixcanvas,
			     KWIN_ATT_IM_RED_OFFSET, &im_red_offset,
			     KWIN_ATT_IM_GREEN_OFFSET, &im_green_offset,
			     KWIN_ATT_IM_BLUE_OFFSET, &im_blue_offset,
			     KWIN_ATT_END);
	if ( !imw_scmap_16to24_o (raster_base + im_red_offset,
				  raster_base + im_green_offset,
				  raster_base + im_blue_offset,
				  FALSE, 4, cache->ras_width * 4,
				  x_pixels, y_pixels,
				  (CONST unsigned short *) slice,
				  (iaddr *) hoffsets, (iaddr *) voffsets,
				  ub_ptr + im_red_offset,
				  ub_ptr + im_green_offset,
				  ub_ptr + im_blue_offset,
				  sizeof *pixel_values) )
	{
	    (void) fprintf (stderr, "Error drawing image into XglCanvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
    }
    else if (depth == 8)
    {
	if ( !imw_to8_lossy (raster_base, 1, cache->ras_width,
			     x_pixels, y_pixels,
			     slice, (iaddr *) hoffsets, (iaddr *) voffsets,
			     width, height, type, conv_type,
			     num_pixels, ub_pixels,
			     blank_pixel, min_sat_pixel, max_sat_pixel,
			     i_min, i_max, iscale_func, iscale_info ) )
	{
	    (void) fprintf (stderr, "Error drawing image into XglCanvas\n");
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
    if (depth == 24)
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
    if ( !draw_cached_image (cache, TRUE, x_off, y_off, 0, 0, 0, 0) )
    {
	(void) fprintf (stderr, "%s: error drawing image\n", function_name);
	kwin_free_cache_data (cache);
	if (cache_ptr != NULL) *cache_ptr = NULL;
	return (FALSE);
    }
    if (depth == 24)
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

static flag draw_rgb_image (XglCanvas xglcanvas,
			    int x_off, int y_off, int x_pixels, int y_pixels,
			    CONST unsigned char *red_slice,
			    CONST unsigned char *green_slice,
			    CONST unsigned char *blue_slice,
			    CONST uaddr *hoffsets, CONST uaddr *voffsets,
			    unsigned int width, unsigned int height,
			    KPixCanvasImageCache *cache_ptr)
/*  [PURPOSE] This routine will draw a 2-dimensional slice of a Karma array
    onto an XGL canvas. This slice may be tiled. The slice is a RGB image.
    <xglcanvas> The XGL canvas.
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
    unsigned int count, depth;
    unsigned char *raster_base;
    unsigned char pixels[256];
    static char function_name[] = "__kwin_xgl_draw_rgb_image";

    VERIFY_CANVAS (xglcanvas);
    if ( ( cache = size_cache (xglcanvas, cache_ptr, x_pixels, y_pixels) )
	== NULL )
    {
	return (FALSE);
    }
    xgl_object_get (cache->raster, XGL_MEM_RAS_IMAGE_BUFFER_ADDR,&raster_base);
    depth = xglcanvas->shared_canvas->vinfo.depth;
    kwin_get_attributes (xglcanvas->pixcanvas,
			 KWIN_ATT_IM_RED_OFFSET, &im_red_offset,
			 KWIN_ATT_IM_GREEN_OFFSET, &im_green_offset,
			 KWIN_ATT_IM_BLUE_OFFSET, &im_blue_offset,
			 KWIN_ATT_END);
    /*  Set up pixel value array  */
    for (count = 0; count < 256; ++count) pixels[count] = count;
    /*  Note the casts from (uaddr *) to (iaddr *) for the offset
	arrays. This is dodgy, but it should work.  */
    pstride = 4;
    if ( (x_pixels == width) && (y_pixels == height) )
    {
	/*  This is what I like to see  */
	if ( !imw_to8_o (raster_base + im_red_offset,
			 pstride, cache->ras_width * pstride,
			 x_pixels, y_pixels, (CONST char *) red_slice,
			 (iaddr *) hoffsets, (iaddr *) voffsets,
			 K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			 256, pixels, 0, 0, 255, 0.0, 255.0) )
	{
	    (void) fprintf (stderr,"Error drawing red image into XglCanvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
	if ( !imw_to8_o (raster_base + im_green_offset,
			 pstride, cache->ras_width * pstride,
			 x_pixels, y_pixels, (CONST char *) green_slice,
			 (iaddr *) hoffsets, (iaddr *) voffsets,
			 K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			 256, pixels, 0, 0, 255, 0.0, 255.0) )
	{
	    (void) fprintf (stderr,
			    "Error drawing green image into XglCanvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
	if ( !imw_to8_o (raster_base + im_blue_offset,
			 pstride, cache->ras_width * pstride,
			 x_pixels, y_pixels, (CONST char *) blue_slice,
			 (iaddr *) hoffsets, (iaddr *) voffsets,
			 K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			 256, pixels, 0, 0, 255, 0.0, 255.0) )
	{
	    (void) fprintf (stderr,
			    "Error drawing blue image into XglCanvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
    }
    else
    {
	/*  But we can cope with differing input/output image sizes  */
	if ( !imw_to8_lossy (raster_base + im_red_offset,
			     pstride, cache->ras_width * pstride,
			     x_pixels, y_pixels, (CONST char *) red_slice,
			     (iaddr *) hoffsets, (iaddr *) voffsets,
			     width, height,
			     K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			     256, pixels, 0, 0, 255, 0.0, 255.0,
			     ( flag (*) () ) NULL, NULL) )
	{
	    (void) fprintf (stderr,
			    "Error drawing red image into XglCanvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
	if ( !imw_to8_lossy (raster_base + im_green_offset,
			     pstride, cache->ras_width * pstride,
			     x_pixels, y_pixels, (CONST char *) green_slice,
			     (iaddr *) hoffsets, (iaddr *) voffsets,
			     width, height,
			     K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			     256, pixels, 0, 0, 255, 0.0, 255.0,
			     ( flag (*) () ) NULL, NULL) )
	{
	    (void) fprintf (stderr,
			    "Error drawing green image into XglCanvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
	if ( !imw_to8_lossy (raster_base + im_blue_offset,
			     pstride, cache->ras_width * pstride,
			     x_pixels, y_pixels, (CONST char *) blue_slice,
			     (iaddr *) hoffsets, (iaddr *) voffsets,
			     width, height,
			     K_UBYTE, KIMAGE_COMPLEX_CONV_REAL,
			     256, pixels, 0, 0, 255, 0.0, 255.0,
			     ( flag (*) () ) NULL, NULL) )
	{
	    (void) fprintf (stderr,
			    "Error drawing blue image into XglCanvas\n");
	    kwin_free_cache_data (cache);
	    if (cache_ptr != NULL) *cache_ptr = NULL;
	    return (FALSE);
	}
    }
    cache->width = x_pixels;
    cache->height = y_pixels;
    if ( !draw_cached_image (cache, TRUE, x_off, y_off, 0, 0, 0, 0) )
    {
	(void) fprintf (stderr, "%s: error drawing image\n", function_name);
	kwin_free_cache_data (cache);
	if (cache_ptr != NULL) *cache_ptr = NULL;
	return (FALSE);
    }
/*
    XBell (canvas->display, 100);
*/
    if (cache_ptr == NULL) kwin_free_cache_data (cache);
    else *cache_ptr = cache;
    return (TRUE);
}   /*  End Function draw_rgb_image  */

static flag draw_cached_image (KPixCanvasImageCache cache, flag wait,
			       int parent_x_off, int parent_y_off,
			       int image_width, int image_height,
			       int image_x_off, int image_y_off /*,
			       int canvas_x_off, int canvas_y_off,
			       int canvas_width, int canvas_height */)
/*  [PURPOSE] This routine will draw a previously computed image cache data
    (computed by <<kwin_draw_pc_image>>) onto the canvas which the original
    image was drawn.
    <cache> The cache data.
    <wait> If TRUE and the cache data is accessable by the graphics system, the
    routine waits for the drawing request to complete.
    <parent_x_off> The horizontal offset, relative to the top-left corner of
    the parent object, where the image will be drawn.
    <parent_y_off> The vertical offset, relative to the top-left corner of the
    parent object, where the image will be drawn.
    <image_width> The width of the image segment to draw. If this is less than
    1 the entire image is drawn.
    <image_height> The height of the image segment to draw. If this is less
    than 1 the entire image is drawn.
    <image_x_off> The horizontal offset, relative to the top-left corner of the
    image.
    <image_y_off> The vertical offset, relative to the top-left corner of the
    image.
    <canvas_x_off> The horizontal offset, relative to the top-left corner of
    the parent object, of the pixel canvas. Used for clipping purposes.
    <canvas_y_off> The vertical offset, relative to the top-left corner of
    the parent object, of the pixel canvas. Used for clipping purposes.
    <canvas_width> The width of the pixel canvas. Used for clipping purposes.
    <canvas_height> The height of the pixel canvas. Used for clipping purposes.
    [RETURNS] TRUE on success if there is valid cache data, else FALSE
    indicating that the image must be recomputed and drawn using
    <<kwin_draw_pc_image>>.
*/
{
    XglCanvas xglcanvas;
    Xgl_bounds_i2d rect;
    Xgl_pt_i2d pos;
    static char function_name[] = "__kwin_xgl_draw_cached_image";

    if (cache == NULL) return (FALSE);
    if (cache->magic_number != CACHE_DATA_MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Invalid cache data\n");
	a_prog_bug (function_name);
    }
    xglcanvas = cache->xglcanvas;
    if (cache->raster == NULL) return (FALSE);
    set_active_canvas (xglcanvas);
/*
    (void) fprintf (stderr,
		    "%s: ca_xoff: %d  ca_yoff: %d  width: %d  height: %d\n",
		    function_name,
		    parent_x_off, parent_y_off, width, height);
    (void) fprintf (stderr, "im_xoff: %d  im_y_off: %d\n",
		    image_x_off, image_y_off);
*/
    if ( (image_width < 1) || (image_height < 1) )
    {
	image_width = cache->width;
	image_height = cache->height;
	image_x_off = 0;
	image_y_off = 0;
    }
    /*  Trim image to cache size  */
    if (image_x_off + image_width > cache->width)
	image_width = cache->width - image_x_off;
    if (image_y_off + image_height > cache->height)
    {
	image_height = cache->height - image_y_off;
    }
    rect.xmin = image_x_off;
    rect.xmax = image_x_off + image_width - 1;
    rect.ymin = image_y_off;
    rect.ymax = image_y_off + image_height - 1;
    pos.x = parent_x_off;
    pos.y = parent_y_off;
    xgl_context_copy_buffer (xglcanvas->shared_canvas->render_context, &rect,
			     &pos, cache->raster);
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
    static char function_name[] = "__kwin_xgl_free_cache_data";

    if (cache == NULL) return;
    if (cache->magic_number != CACHE_DATA_MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Invalid cache data\n");
	a_prog_bug (function_name);
    }
    if (cache->raster != NULL)
    {
	xgl_object_destroy (cache->raster);
    }
    cache->magic_number = 0;
    m_free ( (char *) cache );
}   /*  End Function free_cache_data  */

static flag draw_line (XglCanvas xglcanvas,
		       double x0, double y0, double x1, double y1,
		       unsigned long pixel_value)
/*  [PURPOSE] This routine will draw a line on an X11 canvas.
    <xglcanvas> The XGL canvas.
    <x0> The horizontal offset of the first point.
    <y0> The vertical offset of the first point.
    <x1> The horizontal offset of the second point.
    <y1> The vertical offset of the second point.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Xgl_pt_list point_list;
    Xgl_color colour;
    Xgl_pt_f3d points[2];
    static char function_name[] = "__kwin_xgl_draw_line";

    VERIFY_CANVAS (xglcanvas);
    set_active_canvas (xglcanvas);
    point_list.pt_type = XGL_PT_F3D;
    point_list.bbox = NULL;
    point_list.num_pts = 2;
    point_list.pts = points;
    /*  Have to set the point colour. Pity XGL has such a cumbersome scheme  */
    convert_colour (xglcanvas, &colour, pixel_value);
    xgl_object_set (xglcanvas->shared_canvas->render_context,
		    XGL_CTX_LINE_COLOR, &colour,
		    NULL);
    points[0].x = x0;
    points[0].y = y0;
    points[0].z = 0;
    points[1].x = x1;
    points[1].y = y1;
    points[1].z = 0;
    xgl_multipolyline (xglcanvas->shared_canvas->render_context, NULL,
		       1, &point_list);
    return (TRUE);
}   /*  End Function draw_line  */

static flag get_colour (XglCanvas xglcanvas, CONST char *colourname,
			unsigned long *pixel_value, unsigned short *red,
			unsigned short *green, unsigned short *blue)
/*  [PURPOSE] This routine will allocate a colour for an XGL canvas.
    <xglcanvas> The XGL canvas.
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
    static char function_name[] = "__kwin_xgl_get_colour";

    VERIFY_CANVAS (xglcanvas);
    if (colourname == NULL)
    {
	(void) fprintf (stderr, "NULL colourname pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Search canvas for existing allocation  */
    for (colour = xglcanvas->shared_canvas->colours; colour != NULL;
	 colour = colour->next)
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
    if (XAllocNamedColor (xglcanvas->shared_canvas->display,
			  xglcanvas->shared_canvas->cmap, colourname,
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
    colour->next = xglcanvas->shared_canvas->colours;
    xglcanvas->shared_canvas->colours = colour;
    return (TRUE);
}   /*  End Function get_colour  */

static flag resize (XglCanvas xglcanvas, int xoff, int yoff,
		    int width, int height)
/*  [PURPOSE] This routine will resize an XGL canvas.
    <xglcanvas> The XGL canvas.
    <xoff> The horizontal offset.
    <yoff> The vertical offset.
    <width> The width of the canvas.
    <height> The height of the canvas.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    SharedCanvas sc;
    XWindowAttributes window_attributes;
    static char function_name[] = "__kwin_xgl_resize";

    VERIFY_CANVAS (xglcanvas);
    sc = xglcanvas->shared_canvas;
    if ( (xoff == sc->xoff) && (yoff == sc->yoff) &&
	(width == sc->width) && (height == sc->height) ) return (TRUE);
/*
    (void) fprintf (stderr,
		    "%s: xoff: %d  yoff: %d   width: %d  height: %d...\n",
		    function_name, xoff, yoff, width, height);
*/
    XSync (sc->display, False);
    XGetWindowAttributes (sc->display, sc->window, &window_attributes);
    sc->xwin_width = window_attributes.width;
    sc->xwin_height = window_attributes.height;
    xgl_window_raster_resize (sc->win_ras);
    xgl_context_new_frame (sc->render_context);
    sc->xoff = xoff;
    sc->yoff = yoff;
    sc->width = width;
    sc->height = height;
    return (TRUE);
}   /*  End Function resize  */


/*  Private functions follow  */

static void initialise ()
/*  [PURPOSE] This routine will inialise the XGL support.
    [RETURNS] Nothing.
*/
{
    extern Xgl_sys_state xgl_system_state;

    if (xgl_system_state == NULL)
    {
	if ( ( xgl_system_state = xgl_open (0) ) == NULL )
	{
	    (void) fprintf (stderr, "Error openening XGL\n");
	    return;
	}
    }
    xgl_object_set (xgl_system_state,
		    XGL_SYS_ST_ERROR_DETECTION, TRUE,
		    NULL);
}   /*  End Function initialise  */

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
    static char function_name[] = "__kwin_xgl_get_visinfo_for_visual";

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

static KPixCanvasImageCache size_cache (XglCanvas xglcanvas,
					KPixCanvasImageCache *cache_ptr,
					unsigned int width,
					unsigned int height)
/*  [PURPOSE] This routine will allocate image cache data.
    <xglcanvas> The XGL canvas.
    <cache_ptr> Cache data is written here. The routine may produce cache data
    which will vastly increase the speed of subsequent operations on this data.
    Prior to process exit, a call MUST be made to <<kwin_free_cache_data>>,
    otherwise shared memory segments could remain after the process exits.
    [RETURNS] A pointer to cache data on success, else NULL.
*/
{
    KPixCanvasImageCache cache;
    unsigned int depth, ctype;
    extern Xgl_sys_state xgl_system_state;
    static char function_name[] = "__kwin_xgl_size_cache";

    VERIFY_CANVAS (xglcanvas);
    depth = (xglcanvas->shared_canvas->vinfo.depth == 24) ? 32 : 8;
    ctype = (depth == 8) ? XGL_COLOR_INDEX : XGL_COLOR_RGB;
    if ( (cache_ptr == NULL) || (*cache_ptr == NULL) )
    {
	if ( ( cache = (KPixCanvasImageCache) m_alloc (sizeof *cache) )
	    == NULL )
	{
	    m_error_notify (function_name, "cache data structure");
	    return (NULL);
	}
	/*  Apparently with 8 bit memory rasters XGL cannot cope with copying
	    to a window raster if the width of the memory raster is odd. This
	    may also apply to the height (I haven't checked this). There
	    doesn't seem to be a problem with 32 bit rasters. Until XGL is
	    fixed, we need to do the following hack.  */
	if (depth == 8)
	{
	    if (width % 2 != 0) ++width;
	    if (height % 2 != 0) ++height;
	}
	if ( ( cache->raster =
	      xgl_object_create (xgl_system_state,
				 XGL_MEM_RAS, NULL,
				 XGL_RAS_DEPTH, depth,
				 XGL_RAS_WIDTH, width,
				 XGL_RAS_HEIGHT, height,
/*
				 XGL_DEV_COLOR_TYPE, ctype,
				 XGL_DEV_REAL_COLOR_TYPE, ctype,
*/
				 NULL) ) == NULL )
	{
	    (void) fprintf (stderr, "Error creating XGL memory raster\n");
	    m_free ( (char *) cache );
	    return (NULL);
	}
	cache->pixcanvas = xglcanvas->pixcanvas;
	cache->xglcanvas = xglcanvas;
	cache->magic_number = CACHE_DATA_MAGIC_NUMBER;
	cache->width = width;
	cache->height = height;
	cache->ras_width = width;
	cache->ras_height = height;
	return (cache);
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
    /*  Check if existing XGL memory raster is big enough  */
    if ( (width <= cache->ras_width) && (height <= cache->ras_height) )
    {
	/*  Raster is big enough: great!  */
	return (cache);
    }
    /*  Raster is too small  */
    xgl_object_set (cache->raster,
		    XGL_RAS_WIDTH, width,
		    XGL_RAS_HEIGHT, height,
		    NULL);
    cache->ras_width = width;
    cache->ras_height = height;
    return (cache);
}   /*  End Function size_cache  */

static void set_active_canvas (XglCanvas xglcanvas)
/*  [PURPOSE] This routine will make an XGL canvas active.
    <xglcanvas> The XGL canvas.
    [RETURNS] Nothing.
    */
{
    SharedCanvas shared_canvas;
    static char function_name[] = "__kwin_xgl_set_active_canvas";

    VERIFY_CANVAS (xglcanvas);
    shared_canvas = xglcanvas->shared_canvas;
    if (xglcanvas == shared_canvas->xgl_active) return;
    if (xglcanvas == shared_canvas->xgl_mono)
    {
	(void) fprintf (stderr, "mono canvas active!\n");
	xgl_object_set ( (Xgl_object) shared_canvas->win_ras,
			XGL_WIN_RAS_STEREO_MODE, XGL_STEREO_NONE,
			NULL );
    }
    else if (xglcanvas == shared_canvas->xgl_left)
    {
	xgl_object_set ( (Xgl_object) shared_canvas->win_ras,
			XGL_WIN_RAS_STEREO_MODE, XGL_STEREO_LEFT,
			NULL );
    }
    else if (xglcanvas == shared_canvas->xgl_right)
    {
	xgl_object_set ( (Xgl_object) shared_canvas->win_ras,
			XGL_WIN_RAS_STEREO_MODE, XGL_STEREO_RIGHT,
			NULL );
    }
    else
    {
	(void) fprintf (stderr, "Unknown XglCanvas: %p\n", xglcanvas);
	a_prog_bug (function_name);
    }
    shared_canvas->xgl_active = xglcanvas;
}   /*  End Function set_active_canvas  */

static void convert_colour (XglCanvas xglcanvas, Xgl_color *xgl_colour,
			    unsigned long pixel_value)
/*  [PURPOSE] This routine will convert a pixel value to an XGL colour.
    <xglcanvas> The XGL canvas.
    <xgl_colour> The XGL colour will be written here.
    <pixel_value> The pixel value.
    [RETURNS] Nothing.
*/
{
    unsigned long red_mask, green_mask, blue_mask;
    float red, green, blue;

    if (xglcanvas->shared_canvas->vinfo.class == PseudoColor)
    {
	xgl_colour->index = pixel_value;
	return;
    }
    kwin_get_attributes (xglcanvas->pixcanvas,
			 KWIN_ATT_PIX_RED_MASK, &red_mask,
			 KWIN_ATT_PIX_GREEN_MASK, &green_mask,
			 KWIN_ATT_PIX_BLUE_MASK, &blue_mask,
			 NULL);
    xgl_colour->rgb.r = (float) (pixel_value &red_mask) / (float) red_mask;
    xgl_colour->rgb.g = (float) (pixel_value &green_mask) / (float) green_mask;
    xgl_colour->rgb.b = (float) (pixel_value & blue_mask) / (float) blue_mask;
}   /*  End Function convert_colour  */

#else  /*  !(X11 & HAS_XGL)  */
#  ifdef X11

/*  The stub functions are kept with the real functions now because when I
    started using the -pedantic-errors option to gcc under Linux, it complained
    about empty source files. Moving them into the one file solves this,
    provided X11 is defined. I'll cross that bridge if I come to it.
*/

/*STUB_FUNCTION*/
unsigned int kwin_xgl_test_stereo (Display *display, Window window)
{
    return (KWIN_XGL_NOT_AVAILABLE);
}   /*  End Function kwin_test_xgl_stereo  */

/*STUB_FUNCTION*/
flag kwin_xgl_create_stereo (Display *display, Window window,
			     int xoff, int yoff, int width, int height,
			     KPixCanvas *mono,
			     KPixCanvas *left, KPixCanvas *right)
{
    static char function_name[] = "kwin_create_xgl_stereo";

    (void) fprintf (stderr, "%s: XGL not available!\n", function_name);
    return (FALSE);
}   /*  End Function kwin_create_xgl_stereo  */

#  endif  /*  !X11  */
#endif  /*  X11 & HAS_XGL  */

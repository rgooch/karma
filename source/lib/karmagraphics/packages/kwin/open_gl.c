/*LINTLIBRARY*/
/*  open_gl.c

    This code provides KPixCanvas objects.

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

/*  This file contains all routines needed for manipulating a simple pixel
    canvas (window) independent of the graphics system in use. This file
    contains the OpenGL code.


    Written by      Richard Gooch   9-MAY-1996

    Last updated by Richard Gooch   9-MAY-1996

    Last updated by Richard Gooch   26-MAY-1996: Moved stub functions in.


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_kwin.h>
#include <karma_kwin_hooks.h>
#include <karma_drw.h>
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


#if defined(X11) && defined(HAS_OpenGL)
#include <GL/gl.h>
#include <GL/glx.h>


#define CANVAS_MAGIC_NUMBER (unsigned int) 342879432
#define CACHE_DATA_MAGIC_NUMBER (unsigned int) 1834532176

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
    GLXContext context;
    int xoff;
    int yoff;
    int width;
    int height;
    int xwin_width;
    int xwin_height;
    struct colourcell_type *colours;
    struct OpenGLcanvas_type *gl_active;
    struct OpenGLcanvas_type *gl_left;
    struct OpenGLcanvas_type *gl_right;
} *SharedCanvas;

typedef struct OpenGLcanvas_type
{
    unsigned int magic_number;
    KPixCanvas pixcanvas;
    SharedCanvas shared_canvas;
    flag first_time;
} *OpenGLCanvas;

struct cache_data_type
{
    KPixCanvas pixcanvas;
    /*  Specific routines define bigger strucures  */
    OpenGLCanvas opengl_canvas;
    unsigned int magic_number;
    unsigned int width;          /*  The width of the image in the cache   */
    unsigned int height;         /*  The height of the image in the cache  */
    unsigned int ras_width;      /*  The width of the GL memory raster    */
    unsigned int ras_height;     /*  The height of the GL memory raster   */
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
static GLXContext current_context = NULL;


/*  Mandatory functions  */
STATIC_FUNCTION (flag draw_point, (void *v_canvas, double x, double y,
				   unsigned long pixel_value) );
STATIC_FUNCTION (void *create_child, (void *v_parent, KPixCanvas child) );
STATIC_FUNCTION (flag clear_area, (void *v_canvas, int x, int y,
				   int width, int height) );
/*  Optional hook functions  */
STATIC_FUNCTION (flag draw_pc_image,
		 (OpenGLCanvas opengl_canvas,
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
		 (OpenGLCanvas opengl_canvas,
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
		  int image_x_off, int image_y_off,
		  int canvas_x_off, int canvas_y_off,
		  int canvas_width, int canvas_height) );
STATIC_FUNCTION (void free_cache_data, (KPixCanvasImageCache cache) );
STATIC_FUNCTION (flag draw_line, (OpenGLCanvas opengl_canvas,
				  double x0, double y0, double x1, double y1,
				  unsigned long pixel_value) );
STATIC_FUNCTION (flag draw_arc,
		 (OpenGLCanvas opengl_canvas,
		  double x, double y, double width, double height,
		  int angle1, int angle2, unsigned long pixel_value,
		  flag fill) );
STATIC_FUNCTION (flag draw_polygon,
		 (OpenGLCanvas opengl_canvas, double *x_arr, double *y_arr,
		  unsigned int num_vertices, unsigned long pixel_value,
		  flag convex, flag fill) );
STATIC_FUNCTION (flag draw_string,
		 (OpenGLCanvas opengl_canvas, double x, double y,
		  CONST char *string, unsigned long pixel_value,
		  flag clear_under) );
STATIC_FUNCTION (flag draw_rectangle,
		 (OpenGLCanvas opengl_canvas,
		  double x, double y, double width, double height,
		  unsigned long pixel_value, flag fill) );
STATIC_FUNCTION (flag draw_lines,
		 (OpenGLCanvas opengl_canvas, double *x_arr, double *y_arr,
		  unsigned int num_points, unsigned long pixel_value) );
STATIC_FUNCTION (flag draw_arcs,
		 (OpenGLCanvas opengl_canvas,
		  double *x, double *y, double *width, double *height,
		  int *angle1, int *angle2, unsigned int num_ellipses,
		  unsigned long pixel_value, flag fill) );
STATIC_FUNCTION (flag draw_segments,
		 (OpenGLCanvas opengl_canvas,
		  double *x0, double *y0, double *x1, double *y1,
		  unsigned int num_segments, unsigned long pixel_value) );
STATIC_FUNCTION (flag get_colour,
		 (OpenGLCanvas opengl_canvas, CONST char *colourname,
		  unsigned long *pixel_value, unsigned short *red,
		  unsigned short *green, unsigned short *blue) );
STATIC_FUNCTION (XFontStruct *load_font,
		 (OpenGLCanvas opengl_canvas, CONST char *fontname) );
STATIC_FUNCTION (flag get_string_size,
		 (XFontStruct *x11font, CONST char *string, va_list argp) );
STATIC_FUNCTION (void set_font,
		 (OpenGLCanvas opengl_canvas, XFontStruct *x11font) );
STATIC_FUNCTION (flag query_colourmap,
		 (OpenGLCanvas canvas, unsigned long *pixels,
		  unsigned short *reds, unsigned short *greens,
		  unsigned short *blues, unsigned int num_colours) );
STATIC_FUNCTION (flag resize,
		 (OpenGLCanvas OpenGLcanvas, int xoff, int yoff,
		  int width, int height) );
/*  Private functions  */
STATIC_FUNCTION (void initialise, () );
STATIC_FUNCTION (void set_pixel_in_gc,
		 (OpenGLCanvas opengl_canvas, unsigned long pixel_value) );
STATIC_FUNCTION (XVisualInfo *get_visinfo_for_visual,
		 (Display *dpy, Visual *visual) );
STATIC_FUNCTION (KPixCanvasImageCache size_cache,
		 (OpenGLCanvas opengl_canvas, KPixCanvasImageCache *cache_ptr,
		  unsigned int width, unsigned int height) );
STATIC_FUNCTION (void alloc_points, (OpenGLCanvas opengl_canvas, int num_points) );
STATIC_FUNCTION (void alloc_arcs, (OpenGLCanvas opengl_canvas, int num_arcs) );
STATIC_FUNCTION (void alloc_segments,
		 (OpenGLCanvas opengl_canvas, int num_segments) );
STATIC_FUNCTION (void set_active_canvas, (OpenGLCanvas opengl_canvas) );
STATIC_FUNCTION (void set_pixel,
		 (OpenGLCanvas opengl_canvas, unsigned long pixel_value) );


/*  Public functions follow  */

/*EXPERIMENTAL_FUNCTION*/
flag kwin_open_gl_test_available (Display *display)
/*  [SUMMARY] Test if the GLX extension is supported for a display.
    <display> The X11 display handle.
    [RETURNS] TRUE if the GLX extension is supported, else FALSE.
*/
{
    /*  TEMPORARY until I get stereo with OpenGL working  */
    return (FALSE);
    if ( glXQueryExtension (display, NULL, NULL) )
    {
	/*  DEBUG  */
	(void) fprintf (stderr, "GLX extension available\n");
	return (TRUE);
    }
    /*  DEBUG  */
    (void) fprintf (stderr, "GLX extension not available\n");
    return (FALSE);
}   /*  End Function kwin_open_gl_test_available  */

/*EXPERIMENTAL_FUNCTION*/
flag kwin_open_gl_test_stereo (Display *display, XVisualInfo *visinfo)
/*  [SUMMARY] Test if a particular visual can display stereo.
    <display> The X11 display handle.
    <visinfo> The XVisualInfo structure.
    [RETURNS] TRUE if stereo is available, else FALSE.
*/
{
    int value;
    XVisualInfo *tmp_vinfo;
    static int attrib_list[] = {GLX_STEREO, None};
    static char function_name[] = "kwin_open_gl_test_stereo";

    /*  HACK  */
    if ( ( tmp_vinfo = glXChooseVisual (display, 0, attrib_list) ) == NULL )
    {
	(void) fprintf (stderr, "No PseudoColour stereo visuals available\n");
    }
    else
    {
	(void) fprintf (stderr, "Found PseudoColour stereo visual\n");
    }
    /*  END HACK  */
    if (glXGetConfig (display, visinfo, GLX_USE_GL, &value) != 0)
    {
	(void) fprintf (stderr, "%s: error testing GLX_USE_GL attribute\n",
			function_name);
	return (FALSE);
    }
    if (!value)
    {
	/*  DEBUG  */
	(void) fprintf (stderr,
			"OpenGL rendering not supported on this visual\n");
	return (FALSE);
    }
    /*  DEBUG  */
    if (glXGetConfig (display, visinfo, GLX_RGBA, &value) != 0)
    {
	(void) fprintf (stderr, "%s: error testing GLX_RGBA attribute\n",
			function_name);
	return (FALSE);
    }
    if (value) (void) fprintf (stderr, "%s: RGBA visual\n", function_name);
    else (void) fprintf (stderr, "%s: colour index visual\n", function_name);
    /*  END DEBUG  */
    if (glXGetConfig (display, visinfo, GLX_STEREO, &value) != 0)
    {
	(void) fprintf (stderr, "%s: error testing GLX_STEREO attribute\n",
			function_name);
	return (FALSE);
    }
    if (!value)
    {
	/*  DEBUG  */
	(void) fprintf (stderr,
			"OpenGL stereo not supported on this visual\n");
/*
	(void) fprintf (stderr, "WARNING: forcing use of stereo anyway\n");
*/
	return (FALSE);
    }
    else (void) fprintf (stderr, "OpenGL stereo supported on this visual\n");
    return (TRUE);
}   /*  End Function kwin_open_gl_test_stereo  */

/*EXPERIMENTAL_FUNCTION*/
flag kwin_open_gl_create_stereo (Display *display, Window window,
				 int xoff, int yoff, int width, int height,
				 KPixCanvas *left, KPixCanvas *right)
{
    SharedCanvas shared_canvas;
    XWindowAttributes window_attributes;
    int im_byte_order, count;
    unsigned int visual;
    unsigned int red_offset, green_offset, blue_offset;
    unsigned long im_red_mask, im_green_mask, im_blue_mask;
    unsigned char *ch_ptr;
    XVisualInfo *vinfo;
    extern GLXContext shared_context;
    static char function_name[] = "kwin_create_open_gl_stereo";

    /*  DEBUG  */
    (void) fprintf (stderr, "%s...\n", function_name);
    /*  Do some sanity tests  */
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
    /*  Create and initialise the shared canvas  */
    if ( ( shared_canvas = (SharedCanvas) m_alloc (sizeof *shared_canvas) )
	== NULL )
    {
	m_error_notify (function_name, "OpenGL shared canvas");
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
    /*  Process the window's visual information  */
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
    /*  Create the OpenGL context  */
    if ( ( shared_canvas->context =
	   glXCreateContext (display, &shared_canvas->vinfo, NULL, True) )
	 == NULL )
    {
	(void) fprintf (stderr, "Error creating GLXContext object\n");
	m_free ( (char *) shared_canvas );
	return (FALSE);
    }
    if ( ( shared_canvas->gl_left = (OpenGLCanvas)
	  m_alloc (sizeof *shared_canvas->gl_left) ) == NULL )
    {
	m_abort (function_name, "left OpenGLCanvas");
    }
    shared_canvas->gl_left->first_time = TRUE;
    if ( ( shared_canvas->gl_right = (OpenGLCanvas)
	  m_alloc (sizeof *shared_canvas->gl_right) ) == NULL )
    {
	m_abort (function_name, "right OpenGLCanvas");
    }
    shared_canvas->gl_right->first_time = TRUE;
    if ( (visual == KWIN_VISUAL_DIRECTCOLOUR) ||
	(visual == KWIN_VISUAL_TRUECOLOUR ) )
    {
	*left = ( kwin_create_generic
		 (shared_canvas->gl_left, xoff, yoff, width, height,
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
/*
		  KWIN_FUNC_DRAW_PC_IMAGE, draw_pc_image,
		  KWIN_FUNC_DRAW_RGB_IMAGE, draw_rgb_image,
		  KWIN_FUNC_DRAW_CACHED_IMAGE, draw_cached_image,
		  KWIN_FUNC_FREE_CACHE_DATA, free_cache_data,
		  KWIN_FUNC_DRAW_LINE, draw_line,
		  KWIN_FUNC_GET_COLOUR, get_colour,
*/
		  KWIN_FUNC_RESIZE, resize,
		  KWIN_ATT_END) );
	*right = ( kwin_create_generic
		  (shared_canvas->gl_right, xoff, yoff, width, height,
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
/*
		   KWIN_FUNC_DRAW_PC_IMAGE, draw_pc_image,
		   KWIN_FUNC_DRAW_RGB_IMAGE, draw_rgb_image,
		   KWIN_FUNC_DRAW_CACHED_IMAGE, draw_cached_image,
		   KWIN_FUNC_FREE_CACHE_DATA, free_cache_data,
		   KWIN_FUNC_DRAW_LINE, draw_line,
		   KWIN_FUNC_GET_COLOUR, get_colour,
*/
		   KWIN_FUNC_RESIZE, resize,
		   KWIN_ATT_END) );
    }
    else
    {
	/*  PseudoColour  */
	*left = ( kwin_create_generic
		 (shared_canvas->gl_left, xoff, yoff, width, height,
		  window_attributes.depth,
		  visual, TRUE, draw_point, create_child, clear_area,
/*
		  KWIN_FUNC_DRAW_PC_IMAGE, draw_pc_image,
		  KWIN_FUNC_DRAW_RGB_IMAGE, draw_rgb_image,
		  KWIN_FUNC_DRAW_CACHED_IMAGE, draw_cached_image,
		  KWIN_FUNC_FREE_CACHE_DATA, free_cache_data,
		  KWIN_FUNC_DRAW_LINE, draw_line,
		  KWIN_FUNC_GET_COLOUR, get_colour,
*/
		  KWIN_FUNC_RESIZE, resize,
		  KWIN_ATT_END) );
	*right = ( kwin_create_generic
		  (shared_canvas->gl_right, xoff, yoff, width, height,
		   window_attributes.depth,
		   visual, TRUE, draw_point, create_child, clear_area,
/*
		   KWIN_FUNC_DRAW_PC_IMAGE, draw_pc_image,
		   KWIN_FUNC_DRAW_RGB_IMAGE, draw_rgb_image,
		   KWIN_FUNC_DRAW_CACHED_IMAGE, draw_cached_image,
		   KWIN_FUNC_FREE_CACHE_DATA, free_cache_data,
		   KWIN_FUNC_DRAW_LINE, draw_line,
		   KWIN_FUNC_GET_COLOUR, get_colour,
*/
		   KWIN_FUNC_RESIZE, resize,
		   KWIN_ATT_END) );
    }
    if ( (*left == NULL) || (*right == NULL) )
    {
	m_abort (function_name, "OpenGLCanvas");
    }
    shared_canvas->gl_left->magic_number = CANVAS_MAGIC_NUMBER;
    shared_canvas->gl_left->pixcanvas = *left;
    shared_canvas->gl_left->shared_canvas = shared_canvas;
    shared_canvas->gl_right->magic_number = CANVAS_MAGIC_NUMBER;
    shared_canvas->gl_right->pixcanvas = *right;
    shared_canvas->gl_right->shared_canvas = shared_canvas;
    if ( !glXMakeCurrent (display, window, shared_canvas->context) )
    {
	(void) fprintf (stderr, "Error making GLXContext current\n");
	exit (RV_UNDEF_ERROR);
    }
    current_context = shared_canvas->context;
    fprintf (stderr, "%s: OpenGL stereo canvases created\n",
	     function_name);
/*
    glClearIndex (0);
    */
    glOrtho (0.0, width, 0.0, height, -1.0, 1.0);

    glDrawBuffer (GL_FRONT_RIGHT);
    glClearColor (1.0, 0.0, 0.0, 0.0);
    glColor3f (0.0, 0.0, 1.0);

    glDrawBuffer (GL_FRONT_LEFT);
    glClearColor (0.0, 1.0, 0.0, 0.0);
    glColor3f (0.0, 0.0, 1.0);
    return (TRUE);
}   /*  End Function kwin_create_open_gl_stereo  */


/*  Mandatory hook functions follow  */

static flag draw_point (void *v_canvas, double x, double y,
			unsigned long pixel_value)
/*  [PURPOSE] This routine will draw a point onto an OpenGL canvas.
    <v_canvas> The OpenGL canvas.
    <x> The horizontal offset of the point.
    <y> The vertical offset of the point.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    OpenGLCanvas opengl_canvas = (OpenGLCanvas) v_canvas;
    SharedCanvas sc;
    int gl_x, gl_y;
    static char function_name[] = "__kwin_open_gl_draw_point";

    VERIFY_CANVAS (opengl_canvas);
    set_active_canvas (opengl_canvas);
    glColor3f (0.0, 0.0, 1.0);
    if (opengl_canvas->first_time)
    {
	glClear (GL_COLOR_BUFFER_BIT);
	opengl_canvas->first_time = FALSE;
    }
    /*  Have to set the point colour  */
    set_pixel (opengl_canvas, pixel_value);
    sc = opengl_canvas->shared_canvas;
    glBegin (GL_POINTS);
    gl_x = x + sc->xoff;
    gl_y = sc->xwin_height - (y + sc->yoff) - 1;
    glVertex2i (gl_x, gl_y);
    glEnd ();
    return (TRUE);
}   /*  End Function draw_point  */

static void *create_child (void *v_parent, KPixCanvas child)
/*  [PURPOSE] This routine will create a child OpenGL canvas.
    <v_parent> The parent OpenGL canvas.
    [RETURNS] The child X11 canvas on success, else NULL.
*/
{
    OpenGLCanvas opengl_parent = (OpenGLCanvas) v_parent;
    static char function_name[] = "__kwin_open_gl_create_child";

    VERIFY_CANVAS (opengl_parent);
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
    OpenGLCanvas opengl_canvas = (OpenGLCanvas) v_canvas;
    SharedCanvas sc;
    static char function_name[] = "__kwin_open_gl_clear_area";

    VERIFY_CANVAS (opengl_canvas);
    sc = opengl_canvas->shared_canvas;
    set_active_canvas (opengl_canvas);
/*
    (void) fprintf (stderr, "%s...\n", function_name);
*/
    if ( (x == 0) && (y == 0) && (width == sc->xwin_width) &&
	 (height == sc->xwin_height) )
    {
	(void) fprintf (stderr, "%s clear all...\n", function_name);
	glClear (GL_COLOR_BUFFER_BIT);
    }
    return (TRUE);
}   /*  End Function clear_area  */


/*  Optional hook functions follow  */

static flag resize (OpenGLCanvas opengl_canvas, int xoff, int yoff,
		    int width, int height)
/*  [PURPOSE] This routine will resize an OpenGL canvas.
    <opengl_canvas> The OpenGL canvas.
    <xoff> The horizontal offset.
    <yoff> The vertical offset.
    <width> The width of the canvas.
    <height> The height of the canvas.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    SharedCanvas sc;
    XWindowAttributes window_attributes;
    static char function_name[] = "__kwin_open_gl__resize";

    VERIFY_CANVAS (opengl_canvas);
    sc = opengl_canvas->shared_canvas;
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
    sc->xoff = xoff;
    sc->yoff = yoff;
    sc->width = width;
    sc->height = height;
    return (TRUE);
}   /*  End Function resize  */


/*  Private functions follow  */

static void set_active_canvas (OpenGLCanvas opengl_canvas)
/*  [PURPOSE] This routine will make an OpenGL canvas active.
    <opengl_canvas> The OpenGL canvas.
    [RETURNS] Nothing.
    */
{
    SharedCanvas shared_canvas;
    extern GLXContext current_context;
    static char function_name[] = "__kwin_open_gl_set_active_canvas";

    VERIFY_CANVAS (opengl_canvas);
    shared_canvas = opengl_canvas->shared_canvas;
    if (current_context != shared_canvas->context)
    {
	if ( !glXMakeCurrent (shared_canvas->display, shared_canvas->window,
			      shared_canvas->context) )
	{
	    (void) fprintf (stderr, "Error making GLXContext current\n");
	    exit (RV_UNDEF_ERROR);
	}
	current_context = shared_canvas->context;
    }
    if (opengl_canvas == shared_canvas->gl_active) return;
    if (opengl_canvas == shared_canvas->gl_left)
    {
	(void) fprintf (stderr, "Setting left buffer...\n");
	glDrawBuffer (GL_FRONT_LEFT);
    }
    else if (opengl_canvas == shared_canvas->gl_right)
    {
	(void) fprintf (stderr, "Setting right buffer...\n");
	glDrawBuffer (GL_FRONT_RIGHT);
    }
    else
    {
	(void) fprintf (stderr, "Unknown OpenGLCanvas: %p\n", opengl_canvas);
	a_prog_bug (function_name);
    }
    shared_canvas->gl_active = opengl_canvas;
}   /*  End Function set_active_canvas  */

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
    static char function_name[] = "__kwin_open_gl_get_visinfo_for_visual";

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

static void set_pixel (OpenGLCanvas opengl_canvas, unsigned long pixel_value)
/*  [PURPOSE] This routine set the current pixel value.
    <opengl_canvas> The OpenGL canvas.
    <pixel_value> The pixel value.
    [RETURNS] Nothing.
*/
{
    unsigned long red_mask, green_mask, blue_mask;

    return;
    if (opengl_canvas->shared_canvas->vinfo.class == PseudoColor)
    {
	glIndexi (pixel_value);
	return;
    }
    kwin_get_attributes (opengl_canvas->pixcanvas,
			 KWIN_ATT_PIX_RED_MASK, &red_mask,
			 KWIN_ATT_PIX_GREEN_MASK, &green_mask,
			 KWIN_ATT_PIX_BLUE_MASK, &blue_mask,
			 NULL);
    glColor3f ( (float) (pixel_value & red_mask) / (float) red_mask,
		(float) (pixel_value & green_mask) / (float) green_mask,
		(float) (pixel_value & blue_mask) / (float) blue_mask );
}   /*  End Function set_pixel  */

#else  /*  !(X11 & HAS_OpenGL)  */
#  ifdef X11

/*  The stub functions are kept with the real functions now because when I
    started using the -pedantic-errors option to gcc under Linux, it complained
    about empty source files. Moving them into the one file solves this,
    provided X11 is defined. I'll cross that bridge if I come to it.
*/

/*STUB_FUNCTION*/
flag kwin_open_gl_test_available (Display *display)
/*  [SUMMARY] Test if the GLX extension is supported for a display.
    <display> The X11 display handle.
    [RETURNS] TRUE if the GLX extension is supported, else FALSE.
*/
{
    return (FALSE);
}   /*  End Function kwin_open_gl_test_available  */

/*STUB_FUNCTION*/
flag kwin_open_gl_test_stereo (Display *display, XVisualInfo *visinfo)
/*  [SUMMARY] Test if a particular visual can display stereo.
    <display> The X11 display handle.
    <visinfo> The XVisualInfo structure.
    [RETURNS] TRUE if stereo is available, else FALSE.
*/
{
    return (FALSE);
}   /*  End Function kwin_open_gl_test_stereo  */

/*STUB_FUNCTION*/
flag kwin_open_gl_create_stereo (Display *display, Window window,
				 int xoff, int yoff, int width, int height,
				 KPixCanvas *left, KPixCanvas *right)
{
    static char function_name[] = "kwin_create_open_gl_stereo";

    (void) fprintf (stderr, "%s: OpenGL not available!\n", function_name);
    return (FALSE);
}   /*  End Function kwin_create_open_gl_stereo  */

#  endif  /*  !X11  */
#endif  /*  X11 & HAS_OpenGL  */

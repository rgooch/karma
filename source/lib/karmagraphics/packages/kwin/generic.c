/*LINTLIBRARY*/
/*  generic.c

    This code provides KPixCanvas objects.

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

/*  This file contains all routines needed for manipulating a simple pixel
    canvas (window) independent of the graphics system in use. This file
    contains the generic code.


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

    Updated by      Richard Gooch   20-JAN-1995: Uncommented call to XCopyArea
  in <kwin_draw_rgb_image>.

    Updated by      Richard Gooch   15-MAR-1995: Allowed drawing TrueColour
  images to PostScript page even if canvas is not TrueColour in
  <kwin_draw_rgb_image>.

    Updated by      Richard Gooch   1-APR-1995: Fixed bug in PostScript
  generation with out of bound values in <kwin_draw_image>.

    Updated by      Richard Gooch   9-APR-1995: Fixed bug in PostScript
  generation with negative values in <kwin_draw_image>.

    Updated by      Richard Gooch   30-APR-1995: Added support for intensity
  scaling function.

    Updated by      Richard Gooch   26-JUL-1995: Added support for non 1:1 RGB
  image display by using <imw_to8_lossy> in <kwin_draw_rgb_image>.

    Updated by      Richard Gooch   5-SEP-1995: Added support for drawing 16bit
  PseudoColour images onto a 24bit TrueColour or DirectColour canvas  in
  <kwin_draw_rgb_image>.

    Updated by      Richard Gooch   6-SEP-1995: Created <kwin_draw_pc_image>.

    Updated by      Richard Gooch   13-SEP-1995: Fixed bug in <size_cache>
  which did not properly test if a Pixmap is big enough.

    Updated by      Richard Gooch   15-SEP-1995: Increased precision of
  <kwin_draw_point> and <kwin_draw_line>.

    Updated by      Richard Gooch   18-SEP-1995: Copied code history from
  main.c  and created this file.

    Updated by      Richard Gooch   20-OCT-1995: Created KWIN_FUNC_RESIZE.

    Updated by      Richard Gooch   22-OCT-1995: Do not draw cached image if
  canvas is not visible.

    Last updated by Richard Gooch   6-JAN-1996: Fixed bug with image placement
  when drawing to a PostScriptPage.


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_kwin.h>
#include <karma_psw.h>
#include <karma_imw.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_c.h>
#include <sys/time.h>
#include <sys/resource.h>


#define CANVAS_MAGIC_NUMBER (unsigned int) 145624539
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

#define MAX_INTENSITY 65535.0

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
    PostScriptPage pspage;
    KPixCanvasFont font;
    KPixCanvas parent;
    /*  The following are only used for TrueColour and DirectColour visuals  */
    unsigned long pix_red_mask;
    unsigned long pix_green_mask;
    unsigned long pix_blue_mask;
    unsigned long im_red_mask;
    unsigned long im_green_mask;
    unsigned long im_blue_mask;
    uaddr red_offset;      /*  With X: only for image display  */
    uaddr green_offset;
    uaddr blue_offset;
    /*  Mandatory graphics system specific hooks  */
    void *info;
    flag (*draw_point) (void *info, double x, double y,
			unsigned long pixel_value);
    void *(*create_child) (void *parent, KPixCanvas child);
    flag (*clear_area) (void *info, int x, int y, int width, int height);
    /*  Optional graphics system specific hooks  */
    flag (*draw_pc_image) (void *info, int x_off, int y_off,
			   int x_pixels, int y_pixels,
			   CONST char *slice,
			   CONST uaddr *hoffsets, CONST uaddr *voffsets,
			   unsigned int width, unsigned int height,
			   unsigned int type, unsigned int conv_type,
			   unsigned int num_pixels,unsigned long *pixel_values,
			   unsigned long blank_pixel,
			   unsigned long min_sat_pixel,
			   unsigned long max_sat_pixel,
			   double i_min, double i_max,
			   flag (*iscale_func) (), void *iscale_info,
			   KPixCanvasImageCache *cache_ptr);
    flag (*draw_rgb_image) (void *info, int x_off, int y_off,
			    int x_pixels, int y_pixels,
			    CONST unsigned char *red_slice,
			    CONST unsigned char *green_slice,
			    CONST unsigned char *blue_slice,
			    CONST uaddr *hoffsets, CONST uaddr *voffsets,
			    unsigned int width, unsigned int height,
			    KPixCanvasImageCache *cache_ptr);
    flag (*draw_cached_image) (KPixCanvasImageCache cache,int x_off,int y_off);
    void (*free_cache_data) (KPixCanvasImageCache cache);
    flag (*draw_line) (void *info, double x0, double y0, double x1, double y1,
		       unsigned long pixel_value);
    flag (*draw_arc) (void *info, double x, double y,
		      double width, double height, int angle1, int angle2,
		      unsigned long pixel_value, flag fill);
    flag (*draw_polygon) (void *info, double *x_arr, double *y_arr,
			  unsigned int num_vertices, unsigned long pixel_value,
			  flag convex, flag fill);
    flag (*draw_string) (void *info, double x, double y, CONST char *string,
			 unsigned long pixel_value, flag clear_under);
    flag (*draw_rectangle) (void *info,
			    double x, double y, double width, double height,
			    unsigned long pixel_value, flag fill);
    flag (*draw_lines) (void *info, double *x_arr, double *y_arr,
			unsigned int num_points, unsigned long pixel_value);
    flag (*draw_arcs) (void *info,
		       double *x, double *y, double *width, double *height,
		       int *angle1, int *angle2, unsigned int num_ellipses,
		       unsigned long pixel_value, flag fill);
    flag (*draw_segments) (void *info,
			   double *x0, double *y0, double *x1, double *y1,
			   unsigned int num_segments,
			   unsigned long pixel_value);
    flag (*get_colour) (void *info, CONST char *colourname,
			unsigned long *pixel_value, unsigned short *red,
			unsigned short *green, unsigned short *blue);
    void *(*load_font) (void *info, CONST char *fontname);
    flag (*get_string_size) (void *info, CONST char *string, va_list argp);
    void (*set_font) (void *canvas_info, void *font_info);
    flag (*query_colourmap) (void *info, unsigned long *pixels,
			     unsigned short *reds, unsigned short *greens,
			     unsigned short *blues, unsigned int num_colours);
    flag (*resize) (void *info, int xoff, int yoff, int width, int height);
};

struct cache_data_type
{
    KPixCanvas pixcanvas;
    /*  Specific routines define bigger strucures  */
};

struct child_type
{
    KPixCanvas child;
    flag absorb_events;
};

struct pixfont_type
{
    unsigned int magic_number;
    KPixCanvas canvas;
    void *info;
};

struct position_struct
{
    int x;
    int y;
    unsigned int event_code;
    void *e_info;
};

/*  Private functions  */
STATIC_FUNCTION (KPixCanvas alloc_canvas, () );
STATIC_FUNCTION (flag child_position_event_func,
		 (KPixCanvas parent, int x, int y, unsigned int event_code,
		  void *event_info, void **f_info) );
STATIC_FUNCTION (void not_implemented, (char *s) );
STATIC_FUNCTION (flag refresh_event_func,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );
STATIC_FUNCTION (flag position_event_func,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );
STATIC_FUNCTION (flag get_colours_from_pixels,
		 (KPixCanvas canvas, unsigned long *pixels,
		  unsigned short *reds, unsigned short *greens,
		  unsigned short *blues, unsigned int num_colours) );


/*  Public functions follow  */

/*EXPERIMENTAL_FUNCTION*/
KPixCanvas kwin_create_generic (void *info, int xoff, int yoff,
				int width, int height, unsigned int depth,
				unsigned int visual, flag visible,
				flag (*draw_point) (),
				void *(*create_child) (),
				flag (*clear_area) (), ...)
/*  [PURPOSE] This routine will create a pixel canvas. The interface to this
    routine is generic. Note that the origin of a KPixCanvas is the upper-left
    corner.
    <info> A pointer to canvas specific data.
    <xoff> The horizontal offset of the canvas origin (upper-left corner)
    relative to the parent object.
    <yoff> The vertical offset of the canvas origin (upper-left corner)
    relative to the parent object.
    <width> The width (vertical extent) of the canvas.
    <height> The height (horizontal extent) of the canvas.
    <depth> The depth (in bits) of the canvas.
    <visual> The visual type of the canvas.
    <visible> TRUE if the canvas is visible, else FALSE.
    <draw_point> The function that is called when a point is to be drawn to the
    pixel canvas. The interface to this routine is as follows:
    [<pre>]
    flag draw_point (void *info, double x, double y, unsigned long pixel_value)
    *   [PURPOSE] This routine will draw a point onto a pixel canvas.
        <info> A pointer to the canvas specific data.
	<x> The horizontal offset of the point.
	<y> The vertical offset of the point.
	<pixel_value> The pixel value to use.
	[RETURNS] TRUE on success, else FALSE.
    *
    [</pre>]
    <create_child> The function that is called when a child canvas is created.
    The interface to this routine is as follows:
    [<pre>]
    void *create_child (void *parent, KPixCanvas child)
    *   [PURPOSE] This routine will create the graphics system specific child
        canvas.
	<parent> A pointer to the parent canvas specific data.
	<child> The newly created child pixel canvas.
	[RETURNS] A pointer to canvas specific data for the child on success,
	else NULL.
    *
    [</pre>]
    <clear_area> The function that is called when the canvas is to be cleared.
    The interface to this routine is as follows:
    [<pre>]
    flag clear_area (void *info, int x, int y, int width, int height)
    *   [PURPOSE] This routine will clear an area in a canvas.
        <info> A pointer to the canvas specific data.
	<x> The horizontal offset of the area.
	<y> The vertical offset of the area.
	<width> The width of the area.
	<height> The height of the area.
	[RETURNS] TRUE on success, else FALSE.
    *
    [</pre>]
    [VARARGS] The optinal list of parameter attribute-key attribute-value
    pairs must follow. See the header file for details on defined attributes.
    This list must be terminated with the value  KWIN_ATT_END  .
    [RETURNS] A pixel canvas on success, else NULL.
*/
{
    va_list argp;
    KPixCanvas canvas;
    KPixCanvasFont font;
    flag bool;
    unsigned int att_key;
    void **ptr;
    static char function_name[] = "kwin_create_generic";

    va_start (argp, clear_area);
    FLAG_VERIFY (visible);
    if ( ( canvas = alloc_canvas () ) == NULL )
    {
	m_error_notify (function_name, "pixel canvas");
	return (NULL);
    }
    canvas->info = info;
    canvas->xoff = xoff;
    canvas->yoff = yoff;
    canvas->width = width;
    canvas->height = height;
    canvas->refresh_list = NULL;
    canvas->position_list = NULL;
    canvas->visible = visible;
    canvas->depth = depth;
    canvas->visual = visual;
    canvas->pspage = NULL;
    canvas->font = NULL;
    canvas->parent = NULL;
    canvas->draw_point = draw_point;
    canvas->create_child = create_child;
    canvas->clear_area = clear_area;
    while ( ( att_key = va_arg (argp, unsigned int) ) != KWIN_ATT_END )
    {
	switch (att_key)
	{
	  case KWIN_ATT_PIX_RED_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr, "Cannot set red mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    canvas->pix_red_mask = va_arg (argp, unsigned long);
	    break;
	  case KWIN_ATT_PIX_GREEN_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf(stderr,"Cannot set green mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    canvas->pix_green_mask = va_arg (argp, unsigned long);
	    break;
	  case KWIN_ATT_PIX_BLUE_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr,"Cannot set blue mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    canvas->pix_blue_mask = va_arg (argp, unsigned long);
	    break;
	  case KWIN_ATT_IM_RED_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr, "Cannot set red mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    canvas->im_red_mask = va_arg (argp, unsigned long);
	    break;
	  case KWIN_ATT_IM_GREEN_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf(stderr,"Cannot set green mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    canvas->im_green_mask = va_arg (argp, unsigned long);
	    break;
	  case KWIN_ATT_IM_BLUE_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr,"Cannot set blue mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    canvas->im_blue_mask = va_arg (argp, unsigned long);
	    break;
	  case KWIN_ATT_IM_RED_OFFSET:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr,
				"Cannot set red offset for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    canvas->red_offset = va_arg (argp, uaddr);
	    break;
	  case KWIN_ATT_IM_GREEN_OFFSET:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr,
				"Cannot set green offset for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    canvas->green_offset = va_arg (argp, uaddr);
	    break;
	  case KWIN_ATT_IM_BLUE_OFFSET:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr,
				"Cannot set blue offset for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    canvas->blue_offset = va_arg (argp, uaddr);
	    break;
	  case KWIN_FUNC_DRAW_PC_IMAGE:
	    ptr = (void **) &canvas->draw_pc_image;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_DRAW_RGB_IMAGE:
	    ptr = (void **) &canvas->draw_rgb_image;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_DRAW_CACHED_IMAGE:
	    ptr = (void **) &canvas->draw_cached_image;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_FREE_CACHE_DATA:
	    ptr = (void **) &canvas->free_cache_data;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_DRAW_LINE:
	    ptr = (void **) &canvas->draw_line;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_DRAW_ARC:
	    ptr = (void **) &canvas->draw_arc;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_DRAW_POLYGON:
	    ptr = (void **) &canvas->draw_polygon;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_DRAW_STRING:
	    ptr = (void **) &canvas->draw_string;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_DRAW_RECTANGLE:
	    ptr = (void **) &canvas->draw_rectangle;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_DRAW_LINES:
	    ptr = (void **) &canvas->draw_lines;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_DRAW_ARCS:
	    ptr = (void **) &canvas->draw_arcs;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_DRAW_SEGMENTS:
	    ptr = (void **) &canvas->draw_segments;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_GET_COLOUR:
	    ptr = (void **) &canvas->get_colour;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_LOAD_FONT:
	    ptr = (void **) &canvas->load_font;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_GET_STRING_SIZE:
	    ptr = (void **) &canvas->get_string_size;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_SET_FONT:
	    ptr = (void **) &canvas->set_font;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_QUERY_COLOURMAP:
	    ptr = (void **) &canvas->query_colourmap;
	    *ptr = va_arg (argp, void *);
	    break;
	  case KWIN_FUNC_RESIZE:
	    ptr = (void **) &canvas->resize;
	    *ptr = va_arg (argp, void *);
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (argp);
    canvas->magic_number = CANVAS_MAGIC_NUMBER;
    return (canvas);
}   /*  End Function kwin_create_generic  */

/*PUBLIC_FUNCTION*/
KPixCanvas kwin_create_child (KPixCanvas parent, int xoff, int yoff,
			      int width, int height, flag absorb_events)
/*  [PURPOSE] This routine will create a pixel canvas, ready for drawing,
    within an existing pixel canvas. The new pixel canvas is a child of the
    parent pixel canvas. The child *does not* receive refresh events from the
    parent, it *does* receive position events from the parent, provided these
    events lie within the child's boundaries. For X-based graphics, a copy of
    the Graphics Context is made for the child.
    Note that the origin of a KPixCanvas is the upper-left corner.
    <parent> The parent pixel canvas.
    <xoff> The horizontal offset of the canvas origin (upper-left corner)
    relative to the parent window.
    <yoff> The vertical offset of the canvas origin (upper-left corner)
    relative to the parent window.
    <width> The width (vertical extent) of the canvas.
    <height> The height (horizontal extent) of the canvas.
    <absorb_events> If TRUE, any events passed from the parent canvas to the
    child canvas and consumed by the registered event functions for the child
    canvas are consumed from the parent canvas. If FALSE, the child canvas does
    not consume events from the parent. Note that the parent events are still
    *passed* to the child canvas.
    [RETURNS] A pixel canvas on success, else NULL.
*/
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
	m_error_notify (function_name, "child canvas");
	m_free ( (char *) handle );
	return (NULL);
    }
    m_copy ( (char *) child, (char *) parent, sizeof *child );
    if ( ( child->info = (*parent->create_child) (parent->info, child) )
	== NULL )
    {
	m_error_notify (function_name,"graphics system specific child canvas");
	m_free ( (char *) handle );
	child->magic_number = 0;
	m_free ( (char *) child );
	return (NULL);
    }
    child->xoff = parent->xoff + xoff;
    child->yoff = parent->yoff + yoff;
    child->width = width;
    child->height = height;
    child->colours = NULL;
    child->refresh_list = NULL;
    child->position_list = NULL;
    child->pspage = NULL;
    child->parent = parent;
    handle->child = child;
    handle->absorb_events = absorb_events;
    kwin_register_position_event_func (parent, child_position_event_func,
				       (void *) handle);
    return (child);
}   /*  End Function kwin_create_child  */

/*PUBLIC_FUNCTION*/
KCallbackFunc kwin_register_refresh_func (KPixCanvas canvas,
					  void (*refresh_func) (), void *info)
/*  [PURPOSE] This routine will register a refresh function for a pixel canvas.
    The refresh function will be called whenever the contents of the canvas
    need to be redrawn. Many refresh functions may be registered per canvas.
    The first function registered is the first function called upon refresh.
    <canvas> The pixel canvas.
    <func> The function that is called when the canvas is to be refreshed. The
    interface to this routine is as follows:
    [<pre>]
    void refresh_func (KPixCanvas canvas, int width, int height, void **info,
                       PostScriptPage pspage)
    *   [PURPOSE] This routine will process a refresh event for a pixel canvas.
        <canvas> The pixel canvas.
	<width> The width of the canvas in pixels.
	<height> The height of the canvas in pixels.
	<info> A pointer to the arbitrary canvas information pointer.
	<pspage> If not NULL, the PostScriptPage object the refresh is
	redirected to.
	[RETURNS] Nothing.
    *
    [</pre>]
    <info> The initial arbitrary canvas information pointer.
    [RETURNS] A KCallbackFunc object.
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
/*  [PURPOSE] This routine will register a position event function for a pixel
    canvas. The position event function will be called whenever a position
    event on the canvas has not been consumed. Many position event functions
    may be registered per canvas. The first function registered is the first
    function called upon a position event.
    <canvas> The pixel canvas.
    <func> The function that is called when a position event occurs. The
    interface to this routine is as follows:
    [<pre>]
    flag func (KPixCanvas canvas, int x, int y, unsigned int event_code,
               void *e_info, void **f_info)
    *   [PURPOSE] This routine is a position event consumer for a pixel canvas.
        <canvas> The pixel canvas on which the event occurred.
	<x> The horizontal position of the event, relative to the canvas origin
	<y> The vertical position of the event, relative to the canvas origin.
	<event_code> The arbitrary event code.
	<e_info> A pointer to arbitrary event information.
	<f_info> A pointer to an arbitrary function information pointer.
	[RETURNS] TRUE if the event was consumed, else FALSE indicating that
	the event is still to be processed.
    *
    [</pre>]

    <f_info> The initial arbitrary function information pointer.
    [RETURNS] A KCallbackFunc object.
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
	if (canvas->parent != NULL)
	{
	    canvas->xoff += canvas->parent->xoff;
	    canvas->yoff += canvas->parent->yoff;
	}
    }
    if (canvas->resize != NULL)
    {
	if ( !(*canvas->resize) (canvas->info, canvas->xoff, canvas->yoff,
				 canvas->width, canvas->height) )
	{
	    (void) fprintf (stderr, "%s: error resizing lower level canvas\n",
			    function_name);
	    return (FALSE);
	}
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
void kwin_get_attributes (KPixCanvas canvas, ...)
/*  [PURPOSE] This routine will get the attributes for a pixel canvas.
    <canvas> The pixel canvas.
    [VARARGS] The optional list of parameter attribute-key attribute-value
    pairs must follow. See the header file for details on defined attributes.
    This list must be terminated with the value  KWIN_ATT_END  .
    [RETURNS] Nothing.
*/
{
    va_list argp;
    unsigned int att_key;
    static char function_name[] = "kwin_get_attributes";

    va_start (argp, canvas);
    VERIFY_CANVAS (canvas);
    while ( ( att_key = va_arg (argp, unsigned int) ) != KWIN_ATT_END )
    {
	switch (att_key)
	{
	  case KWIN_ATT_VISUAL:
	    *( va_arg (argp, unsigned int *) ) = canvas->visual;
	    break;
	  case KWIN_ATT_DEPTH:
	    *( va_arg (argp, unsigned int *) ) = canvas->depth;
	    break;
	  case KWIN_ATT_VISIBLE:
	    *( va_arg (argp, flag *) ) = canvas->visible;
	    break;
	  case KWIN_ATT_FONT:
	    *( va_arg (argp, KPixCanvasFont *) ) = canvas->font;
	    break;
	  case KWIN_ATT_PIX_RED_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr, "Cannot get red mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, unsigned long *) ) = canvas->pix_red_mask;
	    break;
	  case KWIN_ATT_PIX_GREEN_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr,
				"Cannot get green mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, unsigned long *) ) = canvas->pix_green_mask;
	    break;
	  case KWIN_ATT_PIX_BLUE_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr, "Cannot get blue mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, unsigned long *) ) = canvas->pix_blue_mask;
	    break;
	  case KWIN_ATT_IM_RED_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr, "Cannot get red mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, unsigned long *) ) = canvas->im_red_mask;
	    break;
	  case KWIN_ATT_IM_GREEN_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr,
				"Cannot get green mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, unsigned long *) ) = canvas->im_green_mask;
	    break;
	  case KWIN_ATT_IM_BLUE_MASK:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr, "Cannot get blue mask for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, unsigned long *) ) = canvas->im_blue_mask;
	    break;
	  case KWIN_ATT_IM_RED_OFFSET:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr,
				"Cannot get red offset for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, uaddr *) ) = canvas->red_offset;
	    break;
	  case KWIN_ATT_IM_GREEN_OFFSET:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr,
				"Cannot get green offset for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, uaddr *) ) = canvas->green_offset;
	    break;
	  case KWIN_ATT_IM_BLUE_OFFSET:
	    if ( (canvas->visual != KWIN_VISUAL_DIRECTCOLOUR) &&
		(canvas->visual != KWIN_VISUAL_TRUECOLOUR) )
	    {
		(void) fprintf (stderr,
				"Cannot get blue offset for visual: %u\n",
				canvas->visual);
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, uaddr *) ) = canvas->blue_offset;
	    break;
	  case KWIN_ATT_LOWER_HANDLE:
	    *( va_arg (argp, void **) ) = canvas->info;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (argp);
}   /*  End Function kwin_get_attributes  */

/*PUBLIC_FUNCTION*/
void kwin_set_attributes (KPixCanvas canvas, ...)
/*  [PURPOSE] This routine will set the attributes for a pixel canvas.
    <canvas> The pixel canvas.
    [VARARGS] The optional list of parameter attribute-key attribute-value
    pairs must follow. See the header file for details on defined attributes.
    This list must be terminated with the value  KWIN_ATT_END  .
    [RETURNS] Nothing.
*/
{
    va_list argp;
    KPixCanvasFont font;
    flag bool;
    unsigned int att_key;
    static char function_name[] = "kwin_set_attributes";

    va_start (argp, canvas);
    VERIFY_CANVAS (canvas);
    while ( ( att_key = va_arg (argp, unsigned int) ) != KWIN_ATT_END )
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
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    canvas->visible = bool;
	    break;
	  case KWIN_ATT_FONT:
	    font = va_arg (argp, KPixCanvasFont);
	    VERIFY_FONT (font);
	    if (canvas->set_font == NULL)
	    {
		(void) fprintf (stderr, "Font setting not supported\n");
		continue;
	    }
	    if (canvas->font == font) break;
	    (*canvas->set_font) (canvas->info, font->info);
	    canvas->font = font;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (argp);
}   /*  End Function kwin_set_attributes  */


/*  Drawing routines follow  */

/*PUBLIC_FUNCTION*/
flag kwin_clear (KPixCanvas canvas, int x, int y, int width, int height)
/*  [PURPOSE] This routine will clear a rectangular portion of a pixel canvas.
    <canvas> The canvas.
    <x> The horizontal offset of the rectangle.
    <y> The vertical offset of the rectangle.
    <width> The width of the rectangle. If this is less than 0 the pixel canvas
    is cleared to its right edge.
    <height> The height of the rectangle. If this is less than 0 the pixel
    canvas is cleared to its bottom edge.
    [NOTE] To clear the entire canvas, the following should be used:
    kwin_clear (canvas, 0, 0, -1, -1);
    [RETURNS] TRUE on success, else FALSE.
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
    return ( (*canvas->clear_area) (canvas->info,
				    canvas->xoff + x, canvas->yoff + y,
				    width, height) );
}   /*  End Function kwin_clear  */

/*OBSOLETE_FUNCTION*/
flag kwin_draw_image (KPixCanvas canvas, array_desc *arr_desc, char *slice,
		      unsigned int hdim, unsigned int vdim,
		      unsigned int elem_index, unsigned int num_pixels,
		      unsigned long *pixel_values,
		      struct win_scale_type *win_scale,
		      KPixCanvasImageCache *cache_ptr)
/*  [PURPOSE] This routine will draw a 2-dimensional slice of a Karma array
    onto a pixel canvas. This slice may be tiled.
    <canvas> The pixel canvas.
    <arr_desc> The array descriptor.
    <slice> A pointer to the start of the slice data.
    <hdim> The dimension index of the horizontal dimension.
    <vdim> The dimension index of the vertical dimension.
    <elem_index> The element index in the data packets.
    <num_pixels> The number of pixels in the colourmap.
    <pixel_values> The array of colourmap pixel values. These pixel values are
    used when translating the data into pixel values.
    <win_scale> The window scaling information.
    <cache_ptr> Cache data is written here. The routine may produce cache data
    which will vastly increase the speed of subsequent operations on this data.
    Prior to process exit, a call MUST be made to <<kwin_free_cache_data>>,
    otherwise shared memory segments could remain after the process exits.
    [MT-LEVEL] Unsafe.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int type;
    unsigned int abs_start_coord;
    unsigned int abs_end_coord;
    unsigned int ord_start_coord;
    unsigned int ord_end_coord;
    unsigned int num_abs_coords;
    unsigned int num_ord_coords;
    uaddr *hoffsets, *voffsets;
    dim_desc *hdim_desc, *vdim_desc;
    static char function_name[] = "kwin_draw_image";

    (void) fprintf (stderr,
		    "Function: <%s> will be removed in Karma version 2.0\n",
		    function_name);
    (void) fprintf (stderr, "Use: <kwin_draw_pc_image> instead.\n");
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
    return ( kwin_draw_pc_image (canvas,
				 win_scale->x_offset, win_scale->y_offset,
				 win_scale->x_pixels, win_scale->y_pixels,
				 slice, hoffsets, voffsets,
				 num_abs_coords, num_ord_coords,
				 type, win_scale->conv_type,
				 num_pixels, pixel_values,
				 win_scale->blank_pixel,
				 win_scale->min_sat_pixel,
				 win_scale->max_sat_pixel,
				 win_scale->z_min, win_scale->z_max,
				 win_scale->iscale_func,win_scale->iscale_info,
				 cache_ptr) );
}   /*  End Function kwin_draw_image  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_pc_image (KPixCanvas canvas, int x_off, int y_off,
			 int x_pixels, int y_pixels,
			 CONST char *slice,
			 CONST uaddr *hoffsets, CONST uaddr *voffsets,
			 unsigned int width, unsigned int height,
			 unsigned int type, unsigned int conv_type,
			 unsigned int num_pixels, unsigned long *pixel_values,
			 unsigned long blank_pixel,unsigned long min_sat_pixel,
			 unsigned long max_sat_pixel,
			 double i_min, double i_max,
			 flag (*iscale_func) (), void *iscale_info,
			 KPixCanvasImageCache *cache_ptr)
/*  [PURPOSE] This routine will draw a 2-dimensional slice of a Karma array
    onto a pixel canvas. This slice may be tiled. The slice is a PseudoColour
    image.
    <canvas> The pixel canvas.
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
    flag complex, greyscale, retval;
    int min_sat_index = -1;
    int max_sat_index = -1;
    int blank_index = -1;
    unsigned char pixel_val, red, green, blue;
    unsigned int x, y;
    unsigned int count, num_colours;
    unsigned char *ubimage, *ub_ptr;
    double y0, y1;
    double val, factor;
    double toobig = TOOBIG;
    double *values, *val_ptr;
    unsigned char imap_red[256], imap_green[256], imap_blue[256];
    unsigned short cmap_reds[256], cmap_greens[256], cmap_blues[256];
    unsigned long cmap_pixels[256];
    static char function_name[] = "kwin_draw_pc_image";

    VERIFY_CANVAS (canvas);
#ifdef DEBUG
    if (getuid () == 465) (void) fprintf (stderr, "%s: x_off: %d  y_off: %d\n",
					  function_name, x_off, y_off);
#endif
    if ( !ds_element_is_atomic (type) )
    {
	(void) fprintf (stderr, "Element must be atomic\n");
	a_prog_bug (function_name);
    }
    if (canvas->pspage != NULL)
    {
	if (num_pixels > 256)
	{
	    (void) fprintf (stderr, "Too many pixels: %u\n", num_pixels);
	    a_prog_bug (function_name);
	}
	for (count = 0; count < num_pixels; ++count)
	{
	    cmap_pixels[count] = pixel_values[count];
	}
	if (num_pixels > 253)
	{
	    /*  Have to make sure special pixels are already in pixel array  */
	    for (count = 0; count < num_pixels; ++count)
	    {
		if ( (min_sat_index < 0) &&
		    (min_sat_pixel == pixel_values[count]) )
		{
		    min_sat_index = count;
		}
		if ( (max_sat_index < 0) &&
		    (max_sat_pixel == pixel_values[count]) )
		{
		    max_sat_index = count;
		}
		if ( (blank_index < 0) &&
		    (blank_pixel == pixel_values[count]) )
		{
		    blank_index = count;
		}
	    }
	    if ( (min_sat_index < 0) || (max_sat_index < 0) ||
		(blank_index < 0) )
	    {
		(void) fprintf (stderr,
				"Number of pixels is greater than 253\n");
		a_prog_bug (function_name);
	    }
	    num_colours = num_pixels;
	}
	else
	{
	    /*  Special pixels can be shoved at the end  */
	    min_sat_index = num_pixels;
	    max_sat_index = num_pixels + 1;
	    blank_index = num_pixels + 2;
	    cmap_pixels[min_sat_index] = min_sat_pixel;
	    cmap_pixels[max_sat_index] = max_sat_pixel;
	    cmap_pixels[blank_index] = blank_pixel;
	    num_colours = num_pixels + 3;
	}
	if ( !get_colours_from_pixels (canvas, cmap_pixels,
				       cmap_reds, cmap_greens, cmap_blues,
				       num_colours) )
	{
	    (void) fprintf (stderr, "%s: cannot find RGB values\n",
			    function_name);
	    return (FALSE);
	}
	/*  Convert 16bit RGB values to 8bit and check if GreyScale (i.e.
	    R=G=B  */
	for (count = 0, greyscale = TRUE; count < num_colours; ++count)
	{
	    red = (int) cmap_reds[count] >> 8;
	    green = (int) cmap_greens[count] >> 8;
	    blue = (int) cmap_blues[count] >> 8;
	    imap_red[count] = red;
	    imap_green[count] = green;
	    imap_blue[count] = blue;
	    if ( (red != green) || (red != blue) ) greyscale = FALSE;
	}
	/*  Allocate temporary image  */
	if ( ( ubimage = (unsigned char *)
	      m_alloc (sizeof *ubimage * width * height) )
	    == NULL )
	{
	    m_error_notify (function_name, "ubarray");
	    return (FALSE);
	}
	/*  Convert image  */
	factor = (double) (num_pixels - 1) / (i_max - i_min);
	if ( ( values = (double *) m_alloc (sizeof *values * 2 * width) )
	    == NULL )
	{
	    m_error_notify (function_name, "values array");
	    m_free ( (char *) ubimage );
	    return (FALSE);
	}
	for (y = 0; y < height; ++y)
	{
	    /*  Convert values to generic data type  */
	    if ( !ds_get_scattered_elements (slice + voffsets[y], type,
					     hoffsets, values, &complex,
					     width) )
	    {
		(void) fprintf (stderr, "Error converting data\n");
		return (FALSE);
	    }
	    if (complex) ds_complex_to_real_1D (values, 2, values, width,
						conv_type);
	    if (iscale_func != NULL)
	    {
		if ( !(*iscale_func) (values, 2, values, 2, width,
				      i_min, i_max, iscale_info) )
		{
		    (void) fprintf (stderr, "Error scaling data\n");
		    return (FALSE);
		}
	    }
	    /*  Convert to unsigned bytes  */
	    for (x = 0, val_ptr = values; x < width; ++x, val_ptr += 2)
	    {
		val = *val_ptr;
		if (val < i_min) pixel_val = min_sat_index;
		else if (val >= toobig) pixel_val = blank_index;
		else if (val > i_max) pixel_val = max_sat_index;
		else
		{
		    val = (val - i_min) * factor;
		    pixel_val = (int) val;
		}
		ubimage[y * width + x] = pixel_val;
	    }
	}
	m_free ( (char *) values );
	/*  Flip  */
	y0 = (double) (canvas->height - 1 - y_off);
	y1 = (double) ( canvas->height - 1 - (y_off + y_pixels - 1) );
	if (greyscale)
	{
	    retval = psw_mono_image
	    ( canvas->pspage, ubimage, width, height,
	     (uaddr *) NULL, (uaddr *) NULL,
	     imap_red,
	     (double) x_off / (double) (canvas->width - 1),
	     (double) y1 / (double) (canvas->height - 1),
	     (double) (x_off + x_pixels - 1) / (double) (canvas->width - 1),
	     (double) y0 / (double) (canvas->height - 1) );
	}
	else
	{
	    retval = psw_pseudocolour_image
	    ( canvas->pspage, ubimage, width, height,
	     (uaddr *) NULL, (uaddr *) NULL,
	     imap_red, imap_green, imap_blue,
	     (double) x_off / (double) (canvas->width - 1),
	     (double) y1 / (double) (canvas->height - 1),
	     (double) (x_off + x_pixels - 1) / (double) (canvas->width - 1),
	     (double) y0 / (double) (canvas->height - 1) );
	}
	m_free ( (char *) ubimage );
	return (retval);
    }
    if (!canvas->visible) return (TRUE);
    if (canvas->draw_pc_image == NULL)
    {
	(void) fprintf (stderr, "Cannot draw PseudoColour images\n");
	return (FALSE);
    }
    return ( (*canvas->draw_pc_image) (canvas->info, canvas->xoff + x_off,
				       canvas->yoff + y_off,
				       x_pixels, y_pixels,
				       slice, hoffsets, voffsets,
				       width, height, type, conv_type,
				       num_pixels, pixel_values,
				       blank_pixel,min_sat_pixel,max_sat_pixel,
				       i_min, i_max,
				       iscale_func, iscale_info, cache_ptr) );
}   /*  End Function kwin_draw_pc_image  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_rgb_image (KPixCanvas canvas, int x_off, int y_off,
			  int x_pixels, int y_pixels,
			  CONST unsigned char *red_slice,
			  CONST unsigned char *green_slice,
			  CONST unsigned char *blue_slice,
			  CONST uaddr *hoffsets, CONST uaddr *voffsets,
			  unsigned int width, unsigned int height,
			  KPixCanvasImageCache *cache_ptr)
/*  [PURPOSE] This routine will draw a 2-dimensional slice of a Karma array
    onto a pixel canvas. This slice may be tiled. The slice is a RGB image.
    <canvas> The pixel canvas.
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
    unsigned int count;
    double y0, y1;
    unsigned char *out_image;
    unsigned char pixels[256];
    static char function_name[] = "kwin_draw_rgb_image";

    VERIFY_CANVAS (canvas);
    if (canvas->pspage != NULL)
    {
	/*  Flip  */
	y0 = (double) (canvas->height - 1 - y_off);
	y1 = (double) ( canvas->height - 1 - (y_off + y_pixels - 1) );
	if ( !psw_rgb_image
	    ( canvas->pspage, red_slice, green_slice, blue_slice,
	     width, height,
	     hoffsets, voffsets, hoffsets, voffsets, hoffsets, voffsets, 0,
	     (double) x_off / (double) (canvas->width - 1),
	     (double) y1 / (double) (canvas->height - 1),
	     (double) (x_off + x_pixels - 1) / (double) (canvas->width - 1),
	     (double) y0 / (double) (canvas->height - 1) ) )
	{
	    return (FALSE);
	}
	return (TRUE);
    }
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
    if (!canvas->visible) return (TRUE);
    if (canvas->draw_rgb_image == NULL)
    {
	(void) fprintf (stderr, "Cannot draw RGB images\n");
	return (FALSE);
    }
    return ( (*canvas->draw_rgb_image) (canvas->info, canvas->xoff + x_off,
					canvas->yoff + y_off,
					x_pixels, y_pixels,
					red_slice, green_slice, blue_slice,
					hoffsets, voffsets, width, height,
					cache_ptr) );
}   /*  End Function kwin_draw_rgb_image  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_cached_image (KPixCanvasImageCache cache, int x_off, int y_off)
/*  [PURPOSE] This routine will draw a previously computed image cache data
    (computed by <<kwin_draw_image>>) onto the canvas which the original image
    was drawn.
    <cache> The cache data.
    <x_off> The horizontal offset, relative to the top-left corner of the
    canvas.
    <y_off> The vertical offset, relative to the top-left corner of the canvas.
    [RETURNS] TRUE on success if there is valid cache data, else FALSE
    indicating that the image must be recomputed and drawn using
    <<kwin_draw_image>>.
*/
{
    KPixCanvas canvas;
    static char function_name[] = "kwin_draw_cached_image";

    if (cache == NULL) return (FALSE);
    canvas = cache->pixcanvas;
    VERIFY_CANVAS (canvas);
    /*  It's a shame to have to throw away the cache validity  */
    if (canvas->pspage != NULL) return (FALSE);
    if (canvas->draw_cached_image == NULL)
    {
	(void) fprintf (stderr, "Cache created but no support for drawing!\n");
	return (FALSE);
    }
    if (!canvas->visible) return (TRUE);
    return ( (*canvas->draw_cached_image) (cache, canvas->xoff + x_off,
					   canvas->yoff + y_off) );
}   /*  End Function kwin_draw_cached_image  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_point (KPixCanvas canvas, double x, double y,
		      unsigned long pixel_value)
/*  [PURPOSE] This routine will draw a single point onto a pixel canvas.
    <canvas> The canvas.
    <x> The horizontal offset of the point.
    <y> The vertical offset of the point.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "kwin_draw_point";

    VERIFY_CANVAS (canvas);
    if (canvas->pspage != NULL)
    {
	a_func_abort (function_name, "PostScript output not yet supported");
	return;
    }
    return ( canvas->draw_point (canvas->info, (double) canvas->xoff + x,
				 (double) canvas->yoff + y, pixel_value) );
}   /*  End Function kwin_draw_point  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_line (KPixCanvas canvas, double x0, double y0,
		     double x1, double y1, unsigned long pixel_value)
/*  [PURPOSE] This routine will draw a single line onto a pixel canvas.
    <canvas> The canvas.
    <x0> The horizontal offset of the first point.
    <y0> The vertical offset of the first point.
    <x1> The horizontal offset of the second point.
    <y1> The vertical offset of the second point.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int i_x0 = x0;
    int i_y0 = y0;
    int i_x1 = x1;
    int i_y1 = y1;
    int sum;
    int inc;
    int x, y;
    int tmp;
    int delta_x, delta_y;
    unsigned short red, green, blue;
    static char function_name[] = "kwin_draw_line";

    VERIFY_CANVAS (canvas);
    if (canvas->pspage != NULL)
    {
	/*  Do a PostScript draw  */
	get_colours_from_pixels (canvas, &pixel_value, &red, &green, &blue, 1);
	/*  Flip vertical  */
	y0 = (double) canvas->height - 1.0 - y0;
	y1 = (double) canvas->height - 1.0 - y1;
	return ( psw_rgb_line ( canvas->pspage,
			       (double) red / MAX_INTENSITY,
			       (double) green / MAX_INTENSITY,
			       (double) blue / MAX_INTENSITY,
			       x0 / (double) (canvas->width - 1),
			       y0 / (double) (canvas->height - 1),
			       x1 / (double) (canvas->width - 1),
			       y1 / (double) (canvas->height - 1) ) );
    }
    if ( (i_x1 == i_x0) && (i_y1 == i_y0) )
    {
	return ( kwin_draw_point (canvas, x0, y0, pixel_value) );
    }
    if (canvas->draw_line != NULL)
    {
	return ( (*canvas->draw_line) (canvas->info,
				       (double) canvas->xoff + x0,
				       (double) canvas->yoff + y0,
				       (double) canvas->xoff + x1,
				       (double) canvas->yoff + y1,
				       pixel_value) );
    }
    /*  First do horizontal draw  */
    if (i_x1 < i_x0)
    {
	tmp = i_x1;  i_x1 = i_x0;  i_x0 = tmp;
	tmp = i_y1;  i_y1 = i_y0;  i_y0 = tmp;
    }
    delta_x = i_x1 - i_x0;
    if (i_y1 < i_y0)
    {
	/*  Line goes up  */
	delta_y = i_y0 - i_y1;
	inc = -1;
    }
    else
    {
	/*  Line goes down  */
	delta_y = i_y1 - i_y0;
	inc = 1;
    }
    x = i_x0;
    y = i_y0;
    sum = delta_x / 2;
    if ( !kwin_draw_point (canvas, x, y, pixel_value) ) return (FALSE);
    while (x < i_x1)
    {
	++x;
	sum += delta_y;
	while (sum >= delta_x)
	{
	    y += inc;
	    sum -= delta_x;
	}
	if ( !kwin_draw_point (canvas, x, y, pixel_value) ) return (FALSE);
    }
    /*  Now do vertical draw  */
    if (i_y1 < i_y0)
    {
	tmp = i_y1;  i_y1 = i_y0;  i_y0 = tmp;
	tmp = i_x1;  i_x1 = i_x0;  i_x0 = tmp;
    }
    delta_y = i_y1 - i_y0;
    if (i_x1 < i_x0)
    {
	/*  Line goes left  */
	delta_x = i_x0 - i_x1;
	inc = -1;
    }
    else
    {
	/*  Line goes right  */
	delta_x = i_x1 - i_x0;
	inc = 1;
    }
    y = i_y0;
    x = i_x0;
    sum = delta_y / 2;
    if ( !kwin_draw_point (canvas, x, y, pixel_value) ) return (FALSE);
    while (y < i_y1)
    {
	++y;
	sum += delta_x;
	while (sum >= delta_y)
	{
	    x += inc;
	    sum -= delta_y;
	}
	if ( !kwin_draw_point (canvas, x, y, pixel_value) ) return (FALSE);
    }
    return (TRUE);
}   /*  End Function kwin_draw_line  */

/*PUBLIC_FUNCTION*/
flag kwin_fill_ellipse (KPixCanvas canvas,
			double cx, double cy, double rx, double ry,
			unsigned long pixel_value)
/*  [PURPOSE] This routine will draw a filled ellipse onto a pixel canvas.
    <canvas> The canvas.
    <cx> The horizontal co-ordinate of the centre of the ellipse.
    <cy> The vertical co-ordinate of the centre of the ellipse.
    <rx> The horizontal radius.
    <ry> The vertical radius.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned short red, green, blue;
    static char function_name[] = "kwin_fill_ellipse";

    VERIFY_CANVAS (canvas);
    if (canvas->pspage != NULL)
    {
	/*  Do a PostScript draw  */
	get_colours_from_pixels (canvas, &pixel_value, &red, &green, &blue, 1);
	/*  Flip vertical  */
	cy = canvas->height - 1.0 - cy;
	return ( psw_rgb_ellipse (canvas->pspage,
				  (double) red / MAX_INTENSITY,
				  (double) green / MAX_INTENSITY,
				  (double) blue / MAX_INTENSITY,
				  cx / (double) (canvas->width - 1),
				  cy / (double) (canvas->height - 1),
				  rx / (double) (canvas->width - 1),
				  ry / (double) (canvas->height - 1),
				  TRUE) );
    }
    if (canvas->draw_arc == NULL)
    {
	(void) fprintf (stderr, "Filling arcs not supported\n");
	return (FALSE);
    }
    return ( (*canvas->draw_arc) (canvas->info,
				  (double) canvas->xoff + cx - rx,
				  (double) canvas->yoff + cy - ry,
				  2.0 * rx, 2.0 * ry,
				  64 * 360, 64 * 360, pixel_value, TRUE) );
}   /*  End Function kwin_fill_ellipse  */

/*PUBLIC_FUNCTION*/
flag kwin_fill_polygon (KPixCanvas canvas, int *point_x, int *point_y,
			unsigned int num_vertices, unsigned long pixel_value,
			flag convex)
/*  [PURPOSE] This routine will draw a filled polygon onto a pixel canvas.
    <canvas> The canvas.
    <point_x> The array of x co-ordinates of vertices of the polygon.
    <point_y> The array of y co-ordinates of vertices of the polygon.
    <num_vertices> The number of vertices in the polygon.
    <pixel_value> The pixel value to use.
    <convex> If TRUE, then the points must form a convex polygon.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag retval;
    int tmp;
    unsigned short red, green, blue;
    unsigned int coord_count;
    double xscale, yscale;
    double *x_arr, *y_arr;
    static char function_name[] = "kwin_fill_polygon";

    VERIFY_CANVAS (canvas);
    FLAG_VERIFY (convex);
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
    if (canvas->pspage != NULL)
    {
	/*  Do a PostScript draw  */
	get_colours_from_pixels (canvas, &pixel_value, &red, &green, &blue, 1);
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
				  (double) red / MAX_INTENSITY,
				  (double) green / MAX_INTENSITY,
				  (double) blue / MAX_INTENSITY,
				  x_arr, y_arr, num_vertices, TRUE);
	m_free ( (char *) x_arr );
	m_free ( (char *) y_arr );
	return (retval);
    }
    if (canvas->draw_polygon == NULL)
    {
	(void) fprintf (stderr, "Filling polygons not supported\n");
	m_free ( (char *) x_arr );
	m_free ( (char *) y_arr );
	return (FALSE);
    }
    for (coord_count = 0; coord_count < num_vertices; ++coord_count)
    {
	x_arr[coord_count] = (double) (point_x[coord_count] + canvas->xoff);
	y_arr[coord_count] = (double) (point_y[coord_count] + canvas->yoff);
    }
    retval = (*canvas->draw_polygon) (canvas->info, x_arr, y_arr, num_vertices,
				      pixel_value, convex, TRUE);
    m_free ( (char *) x_arr );
    m_free ( (char *) y_arr );
    return (retval);
}   /*  End Function kwin_fill_polygon  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_string (KPixCanvas canvas, double x, double y,
		       CONST char *string, unsigned long pixel_value,
		       flag clear_under)
/*  [PURPOSE] This routine will draw a NULL terminated string onto a pixel
    canvas, using the default font for the canvas.
    <canvas> The canvas.
    <x> The horizontal offset of the string origin.
    <y> The vertical offset of the string origin.
    <string> The string.
    <pixel_value> The pixel value to use.
    <clear_under> If TRUE, then the routine will draw both the foreground and
    background of the characters.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "kwin_draw_string";

    VERIFY_CANVAS (canvas);
    if (canvas->pspage != NULL)
    {
	a_func_abort (function_name, "PostScript output not yet supported");
	return;
    }
    if (canvas->draw_string == NULL)
    {
	(void) fprintf (stderr, "Drawing strings not supported\n");
	return (FALSE);
    }
    return ( (*canvas->draw_string) (canvas->info, (double) canvas->xoff + x,
				     (double) canvas->yoff + y,
				     string, pixel_value, clear_under) );
}   /*  End Function kwin_draw_string  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_rectangle (KPixCanvas canvas, double x, double y,
			  double width, double height,
			  unsigned long pixel_value)
/*  [PURPOSE] This routine will draw a single rectangle onto a pixel canvas.
    <canvas> The canvas.
    <x> The horizontal offset of the rectangle.
    <y> The vertical offset of the rectangle.
    <width> The width of the rectangle.
    <height> The height of the rectangle.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    double x1, y1;
    static char function_name[] = "kwin_draw_rectangle";

    VERIFY_CANVAS (canvas);
    if (canvas->draw_rectangle != NULL)
    {
	return ( (*canvas->draw_rectangle) (canvas->info,
					    (double) canvas->xoff + x,
					    (double) canvas->yoff + y,
					    width, height, pixel_value,
					    FALSE) );
    }
    /*  Do it the slow way  */
    x1 = x + width - 1.0;
    y1 = y + height - 1.0;
    if ( !kwin_draw_line (canvas, x, y, x1, y, pixel_value) ) return (FALSE);
    if ( !kwin_draw_line (canvas, x, y1, x1, y1, pixel_value) ) return (FALSE);
    if ( !kwin_draw_line (canvas, x, y, x, y1, pixel_value) ) return (FALSE);
    return ( kwin_draw_line (canvas, x1, y, x1, y1, pixel_value) );
}   /*  End Function kwin_draw_rectangle  */

/*PUBLIC_FUNCTION*/
flag kwin_fill_rectangle (KPixCanvas canvas, double x, double y,
			  double width, double height,
			  unsigned long pixel_value)
/*  [PURPOSE] This routine will fill a single rectangle onto a pixel canvas.
    <canvas> The canvas.
    <x> The horizontal offset of the rectangle.
    <y> The vertical offset of the rectangle.
    <width> The width of the rectangle.
    <height> The height of the rectangle.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int px[4], py[4];
    static char function_name[] = "kwin_fill_rectangle";

    VERIFY_CANVAS (canvas);
    if ( (width < 1.0) || (height < 1.0) ) return;
    if (canvas->draw_rectangle != NULL)
    {
	return ( (*canvas->draw_rectangle) (canvas->info,
					    (double) canvas->xoff + x,
					    (double) canvas->yoff + y,
					    width, height, pixel_value,
					    TRUE) );
    }
    /*  Do it the slow way  */
    px[0] = (int) x;
    py[0] = (int) y;
    px[1] = (int) x + width - 1;
    py[1] = py[0];
    px[2] = px[1];
    py[2] = (int) y + height - 1;
    px[3] = (int) x;
    py[3] = py[2];
    return ( kwin_fill_polygon (canvas, px, py, 4, pixel_value, TRUE) );
}   /*  End Function kwin_fill_rectangle  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_lines (KPixCanvas canvas, int *x_array, int *y_array,
		      int num_points, unsigned long pixel_value)
/*  [PURPOSE] This routine will draw multiple connected lines onto a pixel
    canvas.
    <canvas> The canvas.
    <x_array> The horizontal co-ordinates of the points.
    <y_array> The vetical co-ordinates of the points.
    <num_points> The number of points. The number of lines drawn is 1 less than
    this value.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag retval;
    int count;
    double *x_arr, *y_arr;
    static char function_name[] = "kwin_draw_lines";

    VERIFY_CANVAS (canvas);
    if ( ( x_arr = (double *) m_alloc (sizeof *x_arr * num_points) ) == NULL )
    {
	m_error_notify (function_name, "x array");
	return (FALSE);
    }
    if ( ( y_arr = (double *) m_alloc (sizeof *y_arr * num_points) ) == NULL )
    {
	m_error_notify (function_name, "y array");
	m_free ( (char *) x_arr );
	return (FALSE);
    }
    if (canvas->draw_lines != NULL)
    {
	for (count = 0; count < num_points; ++count)
	{
	    x_arr[count] = (double) (x_array[count] + canvas->xoff);
	    y_arr[count] = (double) (y_array[count] + canvas->yoff);
	}
	retval = (*canvas->draw_lines) (canvas->info, x_arr, y_arr, num_points,
					pixel_value);
	m_free ( (char *) x_arr );
	m_free ( (char *) y_arr );
	return (retval);
    }
    /*  Do this the hard way  */
    for (count = 0; count < num_points; ++count)
    {
	x_arr[count] = (double) x_array[count];
	y_arr[count] = (double) y_array[count];
    }
    for (count = 0; count < num_points - 1; ++count)
    {
	if ( !kwin_draw_line (canvas, x_arr[count], y_arr[count],
			      x_arr[count + 1], y_arr[count + 1],
			      pixel_value) )
	{
	    m_free ( (char *) x_arr );
	    m_free ( (char *) y_arr );
	    return (FALSE);
	}
    }
    m_free ( (char *) x_arr );
    m_free ( (char *) y_arr );
    return (TRUE);
}   /*  End Function kwin_draw_lines  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_ellipse (KPixCanvas canvas, double cx, double cy,
			double rx, double ry, unsigned long pixel_value)
/*  [PURPOSE] This routine will draw an ellipse onto a pixel canvas.
    <canvas> The canvas.
    <cx> The horizontal co-ordinate of the centre of the ellipse.
    <cy> The vertical co-ordinate of the centre of the ellipse.
    <rx> The horizontal radius.
    <ry> The vertical radius.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned short red, green, blue;
    static char function_name[] = "kwin_draw_ellipse";

    VERIFY_CANVAS (canvas);
    if (canvas->pspage != NULL)
    {
	/*  Do a PostScript draw  */
	get_colours_from_pixels (canvas, &pixel_value, &red, &green, &blue, 1);
	/*  Flip vertical  */
	cy = canvas->height - 1.0 - cy;
	return ( psw_rgb_ellipse (canvas->pspage,
				  (double) red / MAX_INTENSITY,
				  (double) green / MAX_INTENSITY,
				  (double) blue / MAX_INTENSITY,
				  cx / (double) (canvas->width - 1),
				  cy / (double) (canvas->height - 1),
				  rx / (double) (canvas->width - 1),
				  ry / (double) (canvas->height - 1),
				  FALSE) );
    }
    if (canvas->draw_arc == NULL)
    {
	(void) fprintf (stderr, "Drawing arcs not supported\n");
	return (FALSE);
    }
    return ( (*canvas->draw_arc) (canvas->info,
				  (double) canvas->xoff + cx - rx,
				  (double) canvas->yoff + cy - ry,
				  2.0 * rx, 2.0 * ry,
				  64 * 360, 64 * 360, pixel_value, FALSE) );
}   /*  End Function kwin_draw_ellipse  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_ellipses (KPixCanvas canvas, int *cx, int *cy, int *rx, int *ry,
			 int num_ellipses, unsigned long pixel_value)
/*  [PURPOSE] This routine will draw multiple ellipses onto a pixel canvas.
    <canvas> The canvas.
    <cx> The array of horizontal co-ordinate of the centre of the ellipse.
    <cy> The array of vertical co-ordinate of the centre of the ellipse.
    <rx> The array of horizontal radii.
    <ry> The array of vertical radii.
    <num_ellipses> The number of ellipses.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int count;
    flag retval;
    int *angle1_arr, *angle2_arr;
    double *x_arr, *y_arr, *width_arr, *height_arr;
    static char function_name[] = "kwin_draw_ellipses";

    VERIFY_CANVAS (canvas);
    if (canvas->draw_arcs != NULL)
    {
	if ( ( x_arr = (double *) m_alloc (sizeof *x_arr * num_ellipses) )
	    == NULL )
	{
	    m_error_notify (function_name, "x array");
	    return (FALSE);
	}
	if ( ( y_arr = (double *) m_alloc (sizeof *y_arr * num_ellipses) )
	    == NULL )
	{
	    m_error_notify (function_name, "y array");
	    m_free ( (char *) x_arr );
	    return (FALSE);
	}
	if ( ( width_arr = (double *) m_alloc (sizeof *width_arr *
					       num_ellipses) ) == NULL )
	{
	    m_error_notify (function_name, "width array");
	    m_free ( (char *) x_arr );
	    m_free ( (char *) y_arr );
	    return (FALSE);
	}
	if ( ( height_arr = (double *) m_alloc (sizeof *height_arr *
						num_ellipses) ) == NULL )
	{
	    m_error_notify (function_name, "height array");
	    m_free ( (char *) x_arr );
	    m_free ( (char *) y_arr );
	    m_free ( (char *) width_arr );
	    return (FALSE);
	}
	if ( ( angle1_arr = (int *) m_alloc (sizeof *angle1_arr *
					     num_ellipses) ) == NULL )
	{
	    m_error_notify (function_name, "height array");
	    m_free ( (char *) x_arr );
	    m_free ( (char *) y_arr );
	    m_free ( (char *) width_arr );
	    m_free ( (char *) height_arr );
	    return (FALSE);
	}
	if ( ( angle2_arr = (int *) m_alloc (sizeof *angle2_arr *
					     num_ellipses) ) == NULL )
	{
	    m_error_notify (function_name, "height array");
	    m_free ( (char *) x_arr );
	    m_free ( (char *) y_arr );
	    m_free ( (char *) width_arr );
	    m_free ( (char *) height_arr );
	    m_free ( (char *) angle1_arr );
	    return (FALSE);
	}
	for (count = 0; count < num_ellipses; ++count)
	{
	    x_arr[count] = (double) (canvas->xoff + cx[count] - rx[count]);
	    y_arr[count] = (double) (canvas->yoff + cy[count] - ry[count]);
	    width_arr[count] = 2.0 * (double) rx[count];
	    height_arr[count] = 2.0 * (double) ry[count];
	    angle1_arr[count] = 64 * 360;
	    angle2_arr[count] = 64 * 360;
	}
	retval = (*canvas->draw_arcs) (canvas->info, x_arr, y_arr,
				       width_arr, height_arr,
				       angle1_arr, angle2_arr,
				       num_ellipses, pixel_value, FALSE);
	m_free ( (char *) x_arr );
	m_free ( (char *) y_arr );
	m_free ( (char *) width_arr );
	m_free ( (char *) height_arr );
	m_free ( (char *) angle1_arr );
	m_free ( (char *) angle2_arr );
	return (retval);
    }
    /*  Do it the slow way  */
    for (count = 0; count < num_ellipses; ++count)
    {
	if ( !kwin_draw_ellipse (canvas, (double) cx[count],(double) cy[count],
				 (double) rx[count], (double) ry[count],
				 pixel_value) )
	{
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function kwin_draw_ellipses  */

/*PUBLIC_FUNCTION*/
flag kwin_fill_ellipses (KPixCanvas canvas, int *cx, int *cy, int *rx, int *ry,
			 int num_ellipses, unsigned long pixel_value)
/*  [PURPOSE] This routine will draw multiple filled ellipses onto a pixel
    canvas.
    <canvas> The canvas.
    <cx> The array of horizontal co-ordinate of the centre of the ellipse.
    <cy> The array of vertical co-ordinate of the centre of the ellipse.
    <rx> The array of horizontal radii.
    <ry> The array of vertical radii.
    <num_ellipses> The number of ellipses.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int count;
    flag retval;
    int *angle1_arr, *angle2_arr;
    double *x_arr, *y_arr, *width_arr, *height_arr;
    static char function_name[] = "kwin_fill_ellipses";

    VERIFY_CANVAS (canvas);
    if (canvas->draw_arcs != NULL)
    {
	if ( ( x_arr = (double *) m_alloc (sizeof *x_arr * num_ellipses) )
	    == NULL )
	{
	    m_error_notify (function_name, "x array");
	    return (FALSE);
	}
	if ( ( y_arr = (double *) m_alloc (sizeof *y_arr * num_ellipses) )
	    == NULL )
	{
	    m_error_notify (function_name, "y array");
	    m_free ( (char *) x_arr );
	    return (FALSE);
	}
	if ( ( width_arr = (double *) m_alloc (sizeof *width_arr *
					       num_ellipses) ) == NULL )
	{
	    m_error_notify (function_name, "width array");
	    m_free ( (char *) x_arr );
	    m_free ( (char *) y_arr );
	    return (FALSE);
	}
	if ( ( height_arr = (double *) m_alloc (sizeof *height_arr *
						num_ellipses) ) == NULL )
	{
	    m_error_notify (function_name, "height array");
	    m_free ( (char *) x_arr );
	    m_free ( (char *) y_arr );
	    m_free ( (char *) width_arr );
	    return (FALSE);
	}
	if ( ( angle1_arr = (int *) m_alloc (sizeof *angle1_arr *
					     num_ellipses) ) == NULL )
	{
	    m_error_notify (function_name, "height array");
	    m_free ( (char *) x_arr );
	    m_free ( (char *) y_arr );
	    m_free ( (char *) width_arr );
	    m_free ( (char *) height_arr );
	    return (FALSE);
	}
	if ( ( angle2_arr = (int *) m_alloc (sizeof *angle2_arr *
					     num_ellipses) ) == NULL )
	{
	    m_error_notify (function_name, "height array");
	    m_free ( (char *) x_arr );
	    m_free ( (char *) y_arr );
	    m_free ( (char *) width_arr );
	    m_free ( (char *) height_arr );
	    m_free ( (char *) angle1_arr );
	    return (FALSE);
	}
	for (count = 0; count < num_ellipses; ++count)
	{
	    x_arr[count] = (double) (canvas->xoff + cx[count] - rx[count]);
	    y_arr[count] = (double) (canvas->yoff + cy[count] - ry[count]);
	    width_arr[count] = 2.0 * (double) rx[count];
	    height_arr[count] = 2.0 * (double) ry[count];
	    angle1_arr[count] = 64 * 360;
	    angle2_arr[count] = 64 * 360;
	}
	retval = (*canvas->draw_arcs) (canvas->info, x_arr, y_arr,
				       width_arr, height_arr,
				       angle1_arr, angle2_arr,
				       num_ellipses, pixel_value, TRUE);
	m_free ( (char *) x_arr );
	m_free ( (char *) y_arr );
	m_free ( (char *) width_arr );
	m_free ( (char *) height_arr );
	m_free ( (char *) angle1_arr );
	m_free ( (char *) angle2_arr );
	return (retval);
    }
    /*  Do it the slow way  */
    for (count = 0; count < num_ellipses; ++count)
    {
	if ( !kwin_fill_ellipse (canvas, (double) cx[count],(double) cy[count],
				 (double) rx[count], (double) ry[count],
				 pixel_value) )
	{
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function kwin_fill_ellipses  */

/*PUBLIC_FUNCTION*/
flag kwin_draw_segments (KPixCanvas canvas, int *x0, int *y0, int *x1, int *y1,
			 int num_segments, unsigned long pixel_value)
/*  [PURPOSE] This routine will draw multiple disjoint lines onto a pixel
    canvas.
    <canvas> The canvas.
    <x0> The horizontal start co-ordinates of the segments.
    <y0> The vetical start co-ordinates of the segments.
    <x1> The horizontal end co-ordinates of the segments.
    <y1> The vetical end co-ordinates of the segments.
    <num_segments> The number of segments.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag retval;
    int count;
    double *x0_arr, *y0_arr, *x1_arr, *y1_arr;
    static char function_name[] = "kwin_draw_segments";

    VERIFY_CANVAS (canvas);
    if (canvas->draw_segments != NULL)
    {
	if ( ( x0_arr = (double *) m_alloc (sizeof *x0_arr * num_segments) )
	    == NULL )
	{
	    m_error_notify (function_name, "x0 array");
	    return (FALSE);
	}
	if ( ( y0_arr = (double *) m_alloc (sizeof *y0_arr * num_segments) )
	    == NULL )
	{
	    m_error_notify (function_name, "y0 array");
	    m_free ( (char *) x0_arr );
	    return (FALSE);
	}
	if ( ( x1_arr = (double *) m_alloc (sizeof *x1_arr * num_segments) )
	    == NULL )
	{
	    m_error_notify (function_name, "x1 array");
	    m_free ( (char *) x0_arr );
	    m_free ( (char *) y0_arr );
	    return (FALSE);
	}
	if ( ( y1_arr = (double *) m_alloc (sizeof *y1_arr * num_segments) )
	    == NULL )
	{
	    m_error_notify (function_name, "y1 array");
	    m_free ( (char *) x0_arr );
	    m_free ( (char *) y0_arr );
	    m_free ( (char *) x1_arr );
	    return (FALSE);
	}
	for (count = 0; count < num_segments; ++count)
	{
	    x0_arr[count] = (double) (x0[count] + canvas->xoff);
	    y0_arr[count] = (double) (y0[count] + canvas->yoff);
	    x1_arr[count] = (double) (x1[count] + canvas->xoff);
	    y1_arr[count] = (double) (y1[count] + canvas->yoff);
	}
	retval = (*canvas->draw_segments) (canvas->info, x0_arr, y0_arr,
					   x1_arr, y1_arr, num_segments,
					   pixel_value);
	m_free ( (char *) x0_arr );
	m_free ( (char *) y0_arr );
	m_free ( (char *) x1_arr );
	m_free ( (char *) y1_arr );
	return (retval);
    }
    /*  Do this the hard way  */
    for (count = 0; count < num_segments; ++count)
    {
	if ( !kwin_draw_line (canvas, (double) x0[count], (double) y0[count],
			      (double) x1[count], (double) y1[count],
			      pixel_value) )
	{
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function kwin_draw_segments  */


/*  Other public routines follow  */

/*PUBLIC_FUNCTION*/
void kwin_get_size (KPixCanvas canvas, int *width, int *height)
/*  [PURPOSE] This routine will get the size of a pixel canvas.
    <width> The number of horizontal pixel will be written here.
    <height> The number of vertical pixel will be written here.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "kwin_get_size";

    VERIFY_CANVAS (canvas);
    *width = canvas->width;
    *height = canvas->height;
}   /*  End Function kwin_get_size  */

/*PUBLIC_FUNCTION*/
void kwin_free_cache_data (KPixCanvasImageCache cache)
/*  [PURPOSE] This routine will free some cache data allocated by
    <<kwin_draw_image>>.
    <cache> The cache data.
    [RETURNS] Nothing.
*/
{
    KPixCanvas canvas;
    static char function_name[] = "kwin_free_cache_data";

    if (cache == NULL) return;
    canvas = cache->pixcanvas;
    VERIFY_CANVAS (canvas);
    if (canvas->free_cache_data == NULL)
    {
	(void) fprintf (stderr, "Don't know how to free cache\n");
	a_prog_bug (function_name);
    }
    (*canvas->free_cache_data) (cache);
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
flag kwin_convert_to_canvas_coord (KPixCanvas canvas, int xin, int yin,
				   int *xout, int *yout)
/*  [PURPOSE] This routine will convert co-ordinates in a lower level object
    (parent, ie. X window) to co-ordinates in a pixel canvas.
    <canvas> The canvas.
    <xin> The lower level horizontal co-ordinate.
    <yin> The lower level vertical co-ordinate.
    <xout> The horizontal canvas co-ordinate will be written here.
    <yout> The vertical canvas co-ordinate will be written here.
    [RETURNS] TRUE if the co-ordinate lies within the canvas boundaries, else
    FALSE (although a conversion is still performed).
*/
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
flag kwin_convert_from_canvas_coord (KPixCanvas canvas, int xin, int yin,
				     int *xout, int *yout)
/*  [PURPOSE] This routine will convert co-ordinates in a pixel canvas to
    co-ordinates in a lower level object (parent, ie. X window).
    <canvas> The canvas.
    <xin> The horizontal canvas co-ordinate.
    <yin> The vertical canvas co-ordinate.
    <xout> The lower level horizontal co-ordinate will be written here.
    <yout> The lower level vertical co-ordinate will be written here.
    [RETURNS] TRUE if the co-ordinate lies within the canvas boundaries, else
    FALSE (although a conversion is still performed).
*/
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
flag kwin_get_colour (KPixCanvas canvas, CONST char *colourname,
		      unsigned long *pixel_value, unsigned short *red,
		      unsigned short *green, unsigned short *blue)
/*  [PURPOSE] This routine will get (possibly allocating) a colourcell for a
    canvas.
    <canvas> The canvas.
    <colourname> The name of the colour to get.
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
    static char function_name[] = "kwin_get_colour";

    VERIFY_CANVAS (canvas);
    if (colourname == NULL)
    {
	(void) fprintf (stderr, "NULL colourname pointer passed\n");
	a_prog_bug (function_name);
    }
    if (canvas->get_colour == NULL)
    {
	(void) fprintf (stderr, "Colourname allocation not supported\n");
	return (FALSE);
    }
    return ( (*canvas->get_colour) (canvas->info, colourname, pixel_value,
				    red, green, blue) );
}   /*  End Function kwin_get_colour  */

/*PUBLIC_FUNCTION*/
KPixCanvasFont kwin_load_font (KPixCanvas canvas, CONST char *fontname)
/*  [PURPOSE] This routine will load a font which may be then used to draw text
    onto a pixel canvas.
    <canvas> The pixel canvas for which the font is valid.
    <fontname> The name of the font.
    [RETURNS] A pixel font on success, else NULL.
*/
{
    KPixCanvasFont font;
    static char function_name[] = "kwin_load_font";

    VERIFY_CANVAS (canvas);
    if (fontname == NULL)
    {
	(void) fprintf (stderr, "NULL fontname pointer passed\n");
	a_prog_bug (function_name);
    }
    if (canvas->load_font == NULL)
    {
	(void) fprintf (stderr, "Font loading not supported\n");
	return (NULL);
    }
    if ( ( font = (KPixCanvasFont) m_alloc (sizeof *font) ) == NULL )
    {
	m_abort (function_name, "font structure");
    }
    if ( ( font->info = (*canvas->load_font) (canvas->info, fontname) )
	== NULL )
    {
	m_free ( (char *) font );
	return (NULL);
    }
    font->magic_number = FONT_MAGIC_NUMBER;
    font->canvas = canvas;
    return (font);
}   /*  End Function kwin_load_font  */

/*PUBLIC_FUNCTION*/
flag kwin_get_string_size (KPixCanvasFont font, CONST char *string, ...)
/*  [PURPOSE] This routine will determine the size of a string.
    <font> The font.
    <string> The string.
    [VARARGS] The optional list of parameter attribute-key attribute-value
    pairs must follow. See the header file for details on defined attributes.
    This list must be terminated with the value  KWIN_STRING_END  .
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    KPixCanvas canvas;
    flag retval;
    static char function_name[] = "kwin_get_string_size";

    va_start (argp, string);
    VERIFY_FONT (font);
    canvas = font->canvas;
    VERIFY_CANVAS (canvas);
    if (string == NULL)
    {
	(void) fprintf (stderr, "NULL string pointer passed\n");
	a_prog_bug (function_name);
    }
    if (canvas->get_string_size == NULL)
    {
	(void) fprintf (stderr, "Font sizing not supported\n");
	return (FALSE);
    }
    retval = (*canvas->get_string_size) (font->info, string, argp);
    va_end (argp);
    return (retval);
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
    m_clear ( (char *) canvas, sizeof *canvas );
    canvas->xoff = 0;
    canvas->yoff = 0;
    canvas->width = 0;
    canvas->height = 0;
    canvas->refresh_list = NULL;
    canvas->position_list = NULL;
    canvas->visible = FALSE;
    canvas->depth = 0;
    canvas->visual = -1;
    canvas->pspage = NULL;
    canvas->font = NULL;
    canvas->parent = NULL;
    canvas->info = NULL;
    canvas->draw_point = ( flag (*) () ) NULL;
    canvas->create_child = ( void *(*) () ) NULL;
    canvas->magic_number = CANVAS_MAGIC_NUMBER;
    return (canvas);
}   /*  End Function alloc_canvas  */

static flag child_position_event_func (KPixCanvas parent, int x, int y,
				       unsigned int event_code,
				       void *event_info, void **f_info)
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

static flag get_colours_from_pixels (KPixCanvas canvas, unsigned long *pixels,
				     unsigned short *reds,
				     unsigned short *greens,
				     unsigned short *blues,
				     unsigned int num_colours)
/*  [PURPOSE] This routine will determine the RGB components of an array of
    colourmap entries.
    <canvas> The pixel canvas.
    <pixels> The array of colourmap indices.
    <reds> The red components will be written to this array.
    <greens> The green components will be written to this array.
    <blues> The blue components will be written to this array.
    <num_colours> The number of colours in the arrays.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "__kwin_get_colours_from_pixels";

    VERIFY_CANVAS (canvas);
    if (canvas->query_colourmap == NULL)
    {
	(void) fprintf (stderr, "Colourmap querying not supported.\n");
	return (FALSE);
    }
    return ( (*canvas->query_colourmap) (canvas->info, pixels,
					 reds, greens, blues, num_colours) );
}   /*  End Function get_colours_from_pixels  */

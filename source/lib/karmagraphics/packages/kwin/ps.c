/*LINTLIBRARY*/
/*  ps.c

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
    contains the PostScript code.


    Written by      Richard Gooch   25-AUG-1996: Moved embedded PostScript
  functionality in generic.c here and made use of <kwin_new_driver_refresh>.

    Updated by      Richard Gooch   14-SEP-1996: Created <draw_point> and
  <set_linewidth>.

    Updated by      Richard Gooch   10-OCT-1996: Support huge pixel arrays in
  <draw_pc_image>.

    Last updated by Richard Gooch   22-NOV-1996: Fixed bug in <draw_rgb_image>
  when drawing onto a PseudoColour canvas. Fixed bug in <draw_pc_image> when
  getting RGB values from pixel values.


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <unistd.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_kwin.h>
#include <karma_psw.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_r.h>

#define KPixHookCanvas PSCanvas

typedef struct PScanvas_type
{
    unsigned int magic_number;
    PostScriptPage pspage;
    KPixCanvas pixcanvas;
} *PSCanvas;

#include <karma_kwin_hooks.h>


#define MAX_INTENSITY 65535.0
#define CANVAS_MAGIC_NUMBER 893453289
#define VERIFY_CANVAS(canvas) if (canvas == NULL) \
{fprintf (stderr, "NULL canvas passed\n"); \
 a_prog_bug (function_name); } \
if (canvas->magic_number != CANVAS_MAGIC_NUMBER) \
{fprintf (stderr, "Invalid canvas object\n"); \
 a_prog_bug (function_name); }

/*  Structure declarations  */


/*  Private data  */


/*  Mandatory functions  */
STATIC_FUNCTION (flag draw_point, (PSCanvas pscanvas, double x, double y,
				   unsigned long pixel_value) );
/*  Optional hook functions  */
STATIC_FUNCTION (flag draw_pc_image,
		 (PSCanvas pscanvas,
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
		 (PSCanvas pscanvas,
		  int x_off, int y_off, int x_pixels, int y_pixels,
		  CONST unsigned char *red_slice,
		  CONST unsigned char *green_slice,
		  CONST unsigned char *blue_slice,
		  CONST uaddr *hoffsets, CONST uaddr *voffsets,
		  unsigned int width, unsigned int height,
		  KPixCanvasImageCache *cache_ptr) );
STATIC_FUNCTION (flag draw_line, (PSCanvas pscanvas,
				  double x0, double y0, double x1, double y1,
				  unsigned long pixel_value) );
STATIC_FUNCTION (flag draw_arc,
		 (PSCanvas pscanvas,
		  double x, double y, double width, double height,
		  int angle1, int angle2, unsigned long pixel_value,
		  flag fill) );
STATIC_FUNCTION (flag draw_polygon,
		 (PSCanvas pscanvas, double *x_arr, double *y_arr,
		  unsigned int num_vertices, unsigned long pixel_value,
		  flag convex, flag fill) );
STATIC_FUNCTION (flag set_linewidth, (PSCanvas pscanvas, double linewidth) );

/*  Private functions  */

/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag kwin_write_ps (KPixCanvas canvas, PostScriptPage pspage)
/*  [SUMMARY] Refresh a pixel canvas onto a PostScriptPage object.
    [PURPOSE] This routine will refresh a pixel canvas, redirecting output to a
    PostScriptPage object.
    <canvas> The pixel canvas.
    <pspage> The PostScriptPage object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag ok;
    int canvas_width, canvas_height;
    double linewidth, d_tmp;
    KPixDrawFuncs new_funcs;
    struct PScanvas_type pscanvas;
    static char function_name[] = "kwin_write_ps";

    if (pspage == NULL)
    {
	fprintf (stderr, "NULL PostScriptPage object passed\n");
	a_prog_bug (function_name);
    }
    m_clear ( (char *) &new_funcs, sizeof new_funcs );
    new_funcs.point = draw_point;
    new_funcs.pc_image = draw_pc_image;
    new_funcs.rgb_image = draw_rgb_image;
    new_funcs.line = draw_line;
    new_funcs.arc = draw_arc;
    new_funcs.polygon = draw_polygon;
    new_funcs.set_linewidth = set_linewidth;
    new_funcs.info = &pscanvas;
    pscanvas.magic_number = CANVAS_MAGIC_NUMBER;
    pscanvas.pspage = pspage;
    pscanvas.pixcanvas = canvas;
    kwin_get_attributes (canvas,
			 KWIN_ATT_LINEWIDTH, &linewidth,
			 KWIN_ATT_END);
    kwin_get_size (canvas, &canvas_width, &canvas_height);
    if (linewidth > 0.0)
    {
	d_tmp = (canvas_width * canvas_width) + (canvas_height *canvas_height);
	d_tmp = sqrt (d_tmp) / 1.414213562;
	psw_set_attributes (pspage,
			    PSW_ATT_LINEWIDTH_RELATIVE, linewidth / d_tmp,
			    PSW_ATT_END);
    }
    ok = kwin_new_driver_refresh (canvas, new_funcs, pspage);
    m_clear ( (char *) &pscanvas, sizeof pscanvas );
    return (ok);
}   /*  End Function kwin_write_ps  */


/*  Mandatory functions follow  */

static flag draw_point (PSCanvas pscanvas, double x, double y,
			unsigned long pixel_value)
/*  [PURPOSE] This routine will draw a point on a PostScript canvas.
    <pscanvas> The PostScript canvas.
    <x> The horizontal offset of the point.
    <y> The vertical offset of the point.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int canvas_width, canvas_height;
    unsigned short red, green, blue;
    static char function_name[] = "__kwin_ps_draw_point";

    VERIFY_CANVAS (pscanvas);
    kwin_get_size (pscanvas->pixcanvas, &canvas_width, &canvas_height);
    kwin_get_pixel_RGB_values (pscanvas->pixcanvas, &pixel_value,
			       &red, &green, &blue, 1);
    /*  Flip vertical  */
    y = canvas_height - 1.0 - y;
    return ( psw_rgb_ellipse (pscanvas->pspage,
			      (double) red / MAX_INTENSITY,
			      (double) green / MAX_INTENSITY,
			      (double) blue / MAX_INTENSITY,
			      x / (double) (canvas_width - 1),
			      y / (double) (canvas_height - 1),
			      1e-2 / (double) canvas_width,
			      1e-2 / (double) canvas_height, TRUE) );
}   /*  End Function draw_point  */


/*  Optional hook functions follow  */

static flag draw_pc_image (PSCanvas pscanvas, int x_off, int y_off,
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
    onto a PostScript canvas. This slice may be tiled. The slice is a
    PseudoColour image.
    <pscanvas> The PostScript canvas.
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
    int canvas_width, canvas_height, index;
    int pix_red_shift, pix_green_shift, pix_blue_shift;
    int im_red_shift, im_green_shift, im_blue_shift;
    unsigned int x, y;
    unsigned int count, num_colours, visual;
    unsigned long pix_red_mask, pix_green_mask, pix_blue_mask;
    unsigned long im_red_mask, im_green_mask, im_blue_mask;
    unsigned long tmp_mask, red, green, blue;
    unsigned char *ubimage;
    double y0, y1;
    double val, factor;
    double scaled_min, scaled_max;
    double toobig = TOOBIG;
    unsigned short *cmap_reds, *cmap_greens, *cmap_blues;
    unsigned long *cmap_pixels;
    double *values, *val_ptr;
    static char function_name[] = "__kwin_ps_draw_pc_image";

    VERIFY_CANVAS (pscanvas);
    if ( !ds_element_is_atomic (type) )
    {
	fprintf (stderr, "Element must be atomic\n");
	a_prog_bug (function_name);
    }
    /*  First copy pixel values into a temporary array which also includes
	the special pixels  */
    num_colours = num_pixels + 3;
    if ( ( cmap_pixels = (unsigned long *)
	   m_alloc (num_colours * sizeof *cmap_pixels) ) == NULL )
    {
	m_error_notify (function_name, "temporary pixel array");
	return (FALSE);
    }
    for (count = 0; count < num_pixels; ++count)
    {
	cmap_pixels[count] = pixel_values[count];
    }
    cmap_pixels[num_pixels] = min_sat_pixel;
    cmap_pixels[num_pixels + 1] = max_sat_pixel;
    cmap_pixels[num_pixels + 2] = blank_pixel;
    if ( ( cmap_reds = (unsigned short *)
	   m_alloc (num_colours * sizeof *cmap_reds * 3) ) == NULL )
    {
	m_free ( (char *) cmap_pixels );
	m_error_notify (function_name, "temporary colourmap");
	return (FALSE);
    }
    cmap_greens = cmap_reds + num_colours;
    cmap_blues = cmap_greens + num_colours;
    kwin_get_attributes (pscanvas->pixcanvas,
			 KWIN_ATT_VISUAL, &visual,
			 KWIN_ATT_END);
    if (visual != KWIN_VISUAL_PSEUDOCOLOUR)
    {
	/*  Note that the pixel values in the array are valid for drawing
	    images, not pixels. Because the <kwin_get_pixel_RGB_values>
	    routine expects regular pixel values (i.e. NOT image pixel values),
	    the array of pixel values needs to be fiddled  */
	kwin_get_attributes (pscanvas->pixcanvas,
			     KWIN_ATT_PIX_RED_MASK, &pix_red_mask,
			     KWIN_ATT_PIX_GREEN_MASK, &pix_green_mask,
			     KWIN_ATT_PIX_BLUE_MASK, &pix_blue_mask,
			     KWIN_ATT_IM_RED_MASK, &im_red_mask,
			     KWIN_ATT_IM_GREEN_MASK, &im_green_mask,
			     KWIN_ATT_IM_BLUE_MASK, &im_blue_mask,
			     KWIN_ATT_END);
	for (pix_red_shift = 0, tmp_mask = pix_red_mask; !(tmp_mask & 1);
	     tmp_mask = tmp_mask >> 1, ++pix_red_shift);
	if (tmp_mask != 0xff)
	{
	    fprintf (stderr, "Shifted red_mask: %lx is not 0xff\n",
		     tmp_mask);
	    a_prog_bug (function_name);
	}
	for (pix_green_shift = 0, tmp_mask = pix_green_mask; !(tmp_mask & 1);
	    tmp_mask = tmp_mask >> 1, ++pix_green_shift);
	if (tmp_mask != 0xff)
	{
	    fprintf (stderr, "Shifted green_mask: %lx is not 0xff\n",
		     tmp_mask);
	    a_prog_bug (function_name);
	}
	for (pix_blue_shift = 0, tmp_mask = pix_blue_mask; !(tmp_mask & 1);
	     tmp_mask = tmp_mask >> 1, ++pix_blue_shift);
	if (tmp_mask != 0xff)
	{
	    fprintf (stderr, "Shifted blue_mask: %lx is not 0xff\n",
		     tmp_mask);
	    a_prog_bug (function_name);
	}
	for (im_red_shift = 0, tmp_mask = im_red_mask; !(tmp_mask & 1);
	     tmp_mask = tmp_mask >> 1, ++im_red_shift);
	if (tmp_mask != 0xff)
	{
	    fprintf (stderr, "Shifted red_mask: %lx is not 0xff\n",
		     tmp_mask);
	    a_prog_bug (function_name);
	}
	for (im_green_shift = 0, tmp_mask = im_green_mask; !(tmp_mask & 1);
	    tmp_mask = tmp_mask >> 1, ++im_green_shift);
	if (tmp_mask != 0xff)
	{
	    fprintf (stderr, "Shifted green_mask: %lx is not 0xff\n",
		     tmp_mask);
	    a_prog_bug (function_name);
	}
	for (im_blue_shift = 0, tmp_mask = im_blue_mask; !(tmp_mask & 1);
	     tmp_mask = tmp_mask >> 1, ++im_blue_shift);
	if (tmp_mask != 0xff)
	{
	    fprintf (stderr, "Shifted blue_mask: %lx is not 0xff\n",
		     tmp_mask);
	    a_prog_bug (function_name);
	}
	for (count = 0; count < num_colours; ++count)
	{
	    red = cmap_pixels[count];
	    green = red;
	    blue = red;
	    red = (red & im_red_mask) >> im_red_shift << pix_red_shift;
	    green = (green & im_green_mask) >>im_green_shift <<pix_green_shift;
	    blue = (blue & im_blue_mask) >> im_blue_shift << pix_blue_shift;
	    cmap_pixels[count] = red | green | blue;
	}
    }
    if ( !kwin_get_pixel_RGB_values (pscanvas->pixcanvas, cmap_pixels,
				     cmap_reds, cmap_greens, cmap_blues,
				     num_colours) )
    {
	m_free ( (char *) cmap_pixels );
	m_free ( (char *) cmap_reds );
	fprintf (stderr, "%s: cannot find RGB values\n", function_name);
	return (FALSE);
    }
    m_free ( (char *) cmap_pixels );
    /*  Convert 16bit RGB values to 8bit and check if GreyScale (i.e.
	R=G=B  */
    for (count = 0, greyscale = TRUE; count < num_colours; ++count)
    {
	red = cmap_reds[count] >> 8;
	green = cmap_greens[count] >> 8;
	blue = cmap_blues[count] >> 8;
	cmap_reds[count] = red;
	cmap_greens[count] = green;
	cmap_blues[count] = blue;
	if ( (red != green) || (red != blue) ) greyscale = FALSE;
    }
    /*  Allocate temporary image  */
    if (greyscale)
    {
	/*  Just 1 byte per pixel is enough  */
	if ( ( ubimage = (unsigned char *)
	       m_alloc (sizeof *ubimage * width * height) )
	     == NULL )
	{
	    m_free ( (char *) cmap_reds );
	    m_error_notify (function_name, "ubarray");
	    return (FALSE);
	}
    }
    else
    {
	/*  Need 3 bytes per pixel  */
	if ( ( ubimage = (unsigned char *)
	       m_alloc (sizeof *ubimage * width * height * 3) )
	     == NULL )
	{
	    m_free ( (char *) cmap_reds );
	    m_error_notify (function_name, "ubarray");
	    return (FALSE);
	}
    }
    /*  Convert image  */
    if (iscale_func == NULL)
    {
	scaled_min = i_min;
	scaled_max = i_max;
    }
    else
    {
	if ( !(*iscale_func) (&scaled_min, 0, &i_min, 0, 1, i_min, i_max,
			      iscale_info) )
	{
	    fprintf (stderr, "%s: error scaling raw intensity minimum\n",
		     function_name);
	}
	if ( !(*iscale_func) (&scaled_max, 0, &i_max, 0, 1, i_min, i_max,
			      iscale_info) )
	{
	    fprintf (stderr, "%s: error scaling raw intensity minimum\n",
		     function_name);
	}
    }
    factor = (double) (num_pixels - 1) / (scaled_max - scaled_min);
    if ( ( values = (double *) m_alloc (sizeof *values * 2 * width) ) == NULL )
    {
	m_error_notify (function_name, "values array");
	m_free ( (char *) cmap_reds );
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
	    fprintf (stderr, "Error converting data\n");
	    m_free ( (char *) cmap_reds );
	    m_free ( (char *) ubimage );
	    return (FALSE);
	}
	if (complex) ds_complex_to_real_1D (values, 2, values, width,
					    conv_type);
	if (iscale_func != NULL)
	{
	    if ( !(*iscale_func) (values, 2, values, 2, width,
				  i_min, i_max, iscale_info) )
	    {
		fprintf (stderr, "Error scaling data\n");
		m_free ( (char *) cmap_reds );
		m_free ( (char *) ubimage );
		return (FALSE);
	    }
	}
	/*  Convert to unsigned bytes  */
	for (x = 0, val_ptr = values; x < width; ++x, val_ptr += 2)
	{
	    val = *val_ptr;
	    if (val < scaled_min) index = num_pixels;
	    else if (val >= toobig) index = num_pixels + 2;
	    else if (val > scaled_max) index = num_pixels + 1;
	    else
	    {
		val = (val - scaled_min) * factor;
		index = (int) val;
	    }
	    if (greyscale)
	    {
		ubimage[y * width + x] = cmap_reds[index];
	    }
	    else
	    {
		ubimage[(y * width + x) * 3] = cmap_reds[index];
		ubimage[(y * width + x) * 3 + 1] = cmap_greens[index];
		ubimage[(y * width + x) * 3 + 2] = cmap_blues[index];
	    }
	}
    }
    m_free ( (char *) values );
    m_free ( (char *) cmap_reds );
    kwin_get_size (pscanvas->pixcanvas, &canvas_width, &canvas_height);
    /*  Flip  */
    y0 = (double) (canvas_height - 1 - y_off);
    y1 = (double) ( canvas_height - 1 - (y_off + y_pixels - 1) );
    if (greyscale)
    {
	retval = psw_mono_image
	    ( pscanvas->pspage, ubimage, width, height, NULL, NULL, NULL,
	      (double) x_off / (double) (canvas_width - 1),
	      (double) y1 / (double) (canvas_height - 1),
	      (double) (x_off + x_pixels - 1) / (double) (canvas_width - 1),
	      (double) y0 / (double) (canvas_height - 1) );
    }
    else
    {
	retval = psw_rgb_image
	    ( pscanvas->pspage, ubimage, ubimage + 1, ubimage + 2,
	      width, height, NULL, NULL, NULL, NULL, NULL, NULL, 3,
	      (double) x_off / (double) (canvas_width - 1),
	      (double) y1 / (double) (canvas_height - 1),
	      (double) (x_off + x_pixels - 1) / (double) (canvas_width - 1),
	      (double) y0 / (double) (canvas_height - 1) );
    }
    m_free ( (char *) ubimage );
    return (retval);
}   /*  End Function draw_pc_image  */

static flag draw_rgb_image (PSCanvas pscanvas,
			    int x_off, int y_off, int x_pixels, int y_pixels,
			    CONST unsigned char *red_slice,
			    CONST unsigned char *green_slice,
			    CONST unsigned char *blue_slice,
			    CONST uaddr *hoffsets, CONST uaddr *voffsets,
			    unsigned int width, unsigned int height,
			    KPixCanvasImageCache *cache_ptr)
/*  [PURPOSE] This routine will draw a 2-dimensional slice of a Karma array
    onto a PostScript canvas. This slice may be tiled. The slice is a RGB image
    <pscanvas> The PostScript canvas.
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
    flag ok;
    int canvas_width, canvas_height;
    unsigned char red, green, blue;
    unsigned int count, visual;
    unsigned long red_mask, green_mask, blue_mask;
    float red_max, green_max, blue_max, col_scale;
    double y0, y1;
    unsigned char imap_red[256], imap_green[256], imap_blue[256];
    unsigned short cmap_reds[256], cmap_greens[256], cmap_blues[256];
    unsigned long cmap_pixels[256];
    static char function_name[] = "__kwin_ps_draw_rgb_image";

    VERIFY_CANVAS (pscanvas);
    kwin_get_size (pscanvas->pixcanvas, &canvas_width, &canvas_height);
    kwin_get_attributes (pscanvas->pixcanvas,
			 KWIN_ATT_VISUAL, &visual,
			 KWIN_ATT_END);
    /*  Flip  */
    y0 = (double) (canvas_height - 1 - y_off);
    y1 = (double) ( canvas_height - 1 - (y_off + y_pixels - 1) );
    if (visual != KWIN_VISUAL_DIRECTCOLOUR)
    {
	/*  Not DirectColour visual: no need to worry about the state of
	    the colourmap. Note that even if the visual is PseudoColour,
	    there is no need to worry about the colourmap, since in this
	    case the RGB image is not really destined for a PseudoColour
	    visual, but the <viewimg> package is drawing the RGB image
	    instead of the PseudoColour image because PostScript is being
	    generated  */
	ok = psw_rgb_image
	    ( pscanvas->pspage, red_slice, green_slice, blue_slice,
	      width, height,
	      hoffsets, voffsets, hoffsets, voffsets, hoffsets, voffsets, 0,
	      (double) x_off / (double) (canvas_width - 1),
	      (double) y1 / (double) (canvas_height - 1),
	      (double) (x_off + x_pixels - 1) / (double) (canvas_width - 1),
	      (double) y0 / (double) (canvas_height - 1) );
	return (ok);
    }
    /*  DirectColour visual: have to get colourmap values  */
    kwin_get_attributes (pscanvas->pixcanvas,
			 KWIN_ATT_PIX_RED_MASK, &red_mask,
			 KWIN_ATT_PIX_GREEN_MASK, &green_mask,
			 KWIN_ATT_PIX_BLUE_MASK, &blue_mask,
			 KWIN_ATT_END);
    red_max = red_mask;
    green_max = green_mask;
    blue_max = blue_mask;
    for (count = 0; count < 256; ++count)
    {
	/*  Have to construct pixel value from each mask  */
	col_scale = (float) count / 256.0;
	cmap_pixels[count] = ( (unsigned long) (red_max * col_scale) &
			       red_mask ) |
	    ( (unsigned long) (green_max * col_scale) & green_mask ) |
	    ( (unsigned long) (blue_max * col_scale) & blue_mask );
    }
    if ( !kwin_get_pixel_RGB_values (pscanvas->pixcanvas, cmap_pixels,
				     cmap_reds, cmap_greens, cmap_blues,
				     256) )
    {
	fprintf (stderr, "%s: cannot find RGB values\n", function_name);
	return (FALSE);
    }
    /*  Convert 16bit RGB values to 8bit  */
    for (count = 0; count < 256; ++count)
    {
	red = (int) cmap_reds[count] >> 8;
	green = (int) cmap_greens[count] >> 8;
	blue = (int) cmap_blues[count] >> 8;
	imap_red[count] = red;
	imap_green[count] = green;
	imap_blue[count] = blue;
    }
    ok = psw_directcolour_image
	( pscanvas->pspage, red_slice, green_slice, blue_slice, width, height,
	  hoffsets, voffsets, hoffsets, voffsets, hoffsets, voffsets, 0,
	  imap_red, imap_green, imap_blue,
	  (double) x_off / (double) (canvas_width - 1),
	  (double) y1 / (double) (canvas_height - 1),
	  (double) (x_off + x_pixels - 1) / (double) (canvas_width - 1),
	  (double) y0 / (double) (canvas_height - 1) );
    return (ok);
}   /*  End Function draw_rgb_image  */

static flag draw_line (PSCanvas pscanvas,
		       double x0, double y0, double x1, double y1,
		       unsigned long pixel_value)
/*  [PURPOSE] This routine will draw a line on a PostScript canvas.
    <pscanvas> The PostScript canvas.
    <x0> The horizontal offset of the first point.
    <y0> The vertical offset of the first point.
    <x1> The horizontal offset of the second point.
    <y1> The vertical offset of the second point.
    <pixel_value> The pixel value to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int canvas_width, canvas_height;
    unsigned short red, green, blue;
    static char function_name[] = "__kwin_ps_draw_line";

    VERIFY_CANVAS (pscanvas);
    kwin_get_size (pscanvas->pixcanvas, &canvas_width, &canvas_height);
    kwin_get_pixel_RGB_values (pscanvas->pixcanvas, &pixel_value,
			       &red, &green, &blue, 1);
    /*  Flip vertical  */
    y0 = (double) canvas_height - 1.0 - y0;
    y1 = (double) canvas_height - 1.0 - y1;
    return ( psw_rgb_line ( pscanvas->pspage,
			    (double) red / MAX_INTENSITY,
			    (double) green / MAX_INTENSITY,
			    (double) blue / MAX_INTENSITY,
			    x0 / (double) (canvas_width - 1),
			    y0 / (double) (canvas_height - 1),
			    x1 / (double) (canvas_width - 1),
			    y1 / (double) (canvas_height - 1) ) );
}   /*  End Function draw_line  */

static flag draw_arc (PSCanvas pscanvas,
		      double x, double y, double width, double height,
		      int angle1, int angle2, unsigned long pixel_value,
		      flag fill)
/*  [PURPOSE] This routine will draw an arc on a PostScript canvas.
    <pscanvas> The PostScript canvas.
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
    int canvas_width, canvas_height;
    unsigned short red, green, blue;
    static char function_name[] = "__kwin_ps_draw_arc";

    VERIFY_CANVAS (pscanvas);
    kwin_get_size (pscanvas->pixcanvas, &canvas_width, &canvas_height);
    kwin_get_pixel_RGB_values (pscanvas->pixcanvas, &pixel_value,
			       &red, &green, &blue, 1);
    /*  Flip vertical  */
    y = canvas_height - 1.0 - y;
    return ( psw_rgb_ellipse (pscanvas->pspage,
			      (double) red / MAX_INTENSITY,
			      (double) green / MAX_INTENSITY,
			      (double) blue / MAX_INTENSITY,
			      x / (double) (canvas_width - 1),
			      y / (double) (canvas_height - 1),
			      width / (double) (canvas_width - 1) / 2.0,
			      height / (double) (canvas_height - 1) / 2.0,
			      TRUE) );
}   /*  End Function draw_arc  */

static flag draw_polygon (PSCanvas pscanvas, double *x_arr, double *y_arr,
			  unsigned int num_vertices, unsigned long pixel_value,
			  flag convex, flag fill)
/*  [PURPOSE] This routine will draw a polygon onto a PostScript canvas.
    <pscanvas> The PostScript canvas.
    <x_arr> The array of x co-ordinates of vertices of the polygon.
    <y_arr> The array of y co-ordinates of vertices of the polygon.
    <num_vertices> The number of vertices in the polygon.
    <pixel_value> The pixel value to use.
    <convex> If TRUE, then the points must form a convex polygon.
    <fill> If TRUE, the polygon will be filled.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag retval;
    int canvas_width, canvas_height;
    int tmp;
    unsigned short red, green, blue;
    unsigned int coord_count;
    double xscale, yscale;
    double *x, *y;
    static char function_name[] = "__kwin_ps_draw_polygon";

    VERIFY_CANVAS (pscanvas);
    FLAG_VERIFY (convex);
    kwin_get_size (pscanvas->pixcanvas, &canvas_width, &canvas_height);
    if ( ( x = (double *) m_alloc (num_vertices * sizeof *x) ) == NULL )
    {
	m_error_notify (function_name, "x array");
	return (FALSE);
    }
    if ( ( y = (double *) m_alloc (num_vertices * sizeof *y) ) == NULL )
    {
	m_error_notify (function_name, "y array");
	m_free ( (char *) x );
	return (FALSE);
    }
    /*  Do a PostScript draw  */
    kwin_get_pixel_RGB_values (pscanvas->pixcanvas, &pixel_value,
			       &red, &green, &blue, 1);
    xscale = 1.0 / (double) (canvas_width - 1);
    yscale = 1.0 / (double) (canvas_height - 1);
    for (coord_count = 0; coord_count < num_vertices; ++coord_count)
    {
	x[coord_count] = (double) x_arr[coord_count] * xscale;
	/*  Flip vertical  */
	tmp = canvas_height - 1 - y_arr[coord_count];
	y[coord_count] = (double) tmp * yscale;
    }
    retval = psw_rgb_polygon (pscanvas->pspage,
			      (double) red / MAX_INTENSITY,
			      (double) green / MAX_INTENSITY,
			      (double) blue / MAX_INTENSITY,
			      x, y, num_vertices, TRUE);
    m_free ( (char *) x );
    m_free ( (char *) y );
    return (retval);
}   /*  End Function draw_polygon  */

static flag set_linewidth (PSCanvas pscanvas, double linewidth)
/*  [SUMMARY] Set the linewidth for a canvas.
    <pscanvas> The PostScript canvas.
    <linewidth> The linewidth, in pixels.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int canvas_width, canvas_height;
    double d_tmp;
    static char function_name[] = "__kwin_ps_set_linewidth";

    VERIFY_CANVAS (pscanvas);
    kwin_get_size (pscanvas->pixcanvas, &canvas_width, &canvas_height);
    d_tmp = (canvas_width * canvas_width) + (canvas_height * canvas_height);
    d_tmp = sqrt (d_tmp) / 1.414213562;
    psw_set_attributes (pscanvas->pspage,
			PSW_ATT_LINEWIDTH_RELATIVE, linewidth / d_tmp,
			PSW_ATT_END);
    return (TRUE);
}   /*  End Function set_linewidth  */


/*  Private functions follow  */

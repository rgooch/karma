/*  karma_kwin.h

    Header for  kwin_  package.

    Copyright (C) 1993  Richard Gooch

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

/*

    This include file contains all the definitions and function declarations
  needed to interface to the kwin_ routines in the Karma library.


    Written by      Richard Gooch   16-APR-1993

    Last updated by Richard Gooch   28-MAY-1993

*/

#ifndef KARMA_KWIN_H
#define KARMA_KWIN_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#include <k_win_scale.h>
#include <karma_ds_def.h>
#ifndef KWIN_GENERIC_ONLY
#  ifdef X11
#include <X11/Xlib.h>
#  endif
#endif

#ifndef KPIXCANVAS_DEFINED
#define KPIXCANVAS_DEFINED
typedef void * KPixCanvas;
#endif

#ifndef KPIXCANVASIMAGECACHE_DEFINED
#define KPIXCANVASIMAGECACHE_DEFINED
typedef void * KPixCanvasImageCache;
#endif


/*  File:   kwin.c   */

#ifndef KWIN_GENERIC_ONLY

/*  X specific routines  */
#  ifdef X11
EXTERN_FUNCTION (KPixCanvas kwin_create_x, (Display *display, Window window,
					    GC gc,
					    int xoff, int yoff,
					    int width, int height) );
EXTERN_FUNCTION (void kwin_set_gc_x, (KPixCanvas canvas, GC gc) );
#  endif  /*  X11  */

/*  VX specific routines  */
#  ifdef ARCH_VXMVX
EXTERN_FUNCTION (KPixCanvas kwin_create_vx,
		 (flag pseudo_colour, int xoff, int yoff,
		  int width, int height) );
#  endif  /*  ARCH_VXMVX  */

#endif  /*  !KWIN_GENERIC_ONLY  */

/*  Generic routines  */
EXTERN_FUNCTION (void kwin_register_refresh_func,
		 (KPixCanvas canvas, void (*refresh_func) (), void *info) );
EXTERN_FUNCTION (void kwin_register_position_event_func,
		 (KPixCanvas canvas, flag (*position_func) (), void *f_info) );
EXTERN_FUNCTION (flag kwin_resize, (KPixCanvas canvas, flag clear,
				    int xoff, int yoff,
				    int width, int height) );
EXTERN_FUNCTION (flag kwin_process_position_event, (KPixCanvas canvas,
						    int x, int y, flag clip,
						    unsigned int event_code,
						    void *event_info) );
EXTERN_FUNCTION (flag kwin_draw_image, (KPixCanvas canvas,
					array_desc *arr_desc, char *slice,
					unsigned int hdim, unsigned int vdim,
					unsigned int elem_index,
					unsigned int num_pixels,
					unsigned long *pixel_values,
					struct win_scale_type *win_scale,
					KPixCanvasImageCache *cache_ptr) );
EXTERN_FUNCTION (flag kwin_draw_cached_image, (KPixCanvasImageCache cache,
					       int x_off, int y_off) );
EXTERN_FUNCTION (void kwin_draw_point, (KPixCanvas canvas,
					int x, int y,
					unsigned long pixel_value) );
EXTERN_FUNCTION (void kwin_draw_line, (KPixCanvas canvas,
					int x0, int y0, int x1, int y1,
					unsigned long pixel_value) );
EXTERN_FUNCTION (void kwin_fill_ellipse, (KPixCanvas canvas,
					  int cx, int cy, int rx, int ry,
					  unsigned long pixel_value) );
EXTERN_FUNCTION (flag kwin_fill_polygon, (KPixCanvas canvas,
					  int *point_x, int *point_y,
					  unsigned int num_vertices,
					  unsigned long pixel_value,
					  flag convex) );
EXTERN_FUNCTION (void kwin_get_size, (KPixCanvas canvas, int *width,
				      int *height) );
EXTERN_FUNCTION (void kwin_free_cache_data, (KPixCanvasImageCache cache) );
EXTERN_FUNCTION (flag kwin_convert_to_canvas_coord, (KPixCanvas canvas,
						     int xin, int yin,
						     int *xout, int *yout) );


#endif /*  KARMA_KWIN_H  */

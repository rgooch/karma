/*  karma_canvas.h

    Header for  canvas_  package.

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
  needed to interface to the canvas_ routines in the Karma library.


    Written by      Richard Gooch   17-APR-1993

    Last updated by Richard Gooch   22-NOV-1993

*/

#ifndef KARMA_CANVAS_H
#define KARMA_CANVAS_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#include <karma_iedit.h>
#include <karma_kcmap.h>
#include <karma_kwin.h>

#ifndef KWORLDCANVAS_DEFINED
#define KWORLDCANVAS_DEFINED
typedef void * KWorldCanvas;
#endif


/*  File:  canvas.c  */
EXTERN_FUNCTION (KWorldCanvas canvas_create,
		 (KPixCanvas pixcanvas, Kcolourmap cmap,
		  struct win_scale_type *win_scale) );
EXTERN_FUNCTION (void canvas_register_refresh_func, (KWorldCanvas canvas,
						     void (*refresh_func) (),
						     void *info) );
EXTERN_FUNCTION (void canvas_register_size_control_func,
		 (KWorldCanvas canvas, void (*size_control_func) (),
		  void *info) );
EXTERN_FUNCTION (void canvas_register_position_event_func,
		 (KWorldCanvas canvas, flag (*position_func) (),
		  void *f_info) );
EXTERN_FUNCTION (flag canvas_resize, (KWorldCanvas canvas,
				      struct win_scale_type *win_scale,
				      flag always_clear) );
EXTERN_FUNCTION (void canvas_get_size, (KWorldCanvas canvas,
					int *width, int *height,
					struct win_scale_type *win_scale) );
EXTERN_FUNCTION (flag canvas_convert_to_canvas_coord, (KWorldCanvas canvas,
						       int xin, int yin,
						       double *xout,
						       double *yout) );
EXTERN_FUNCTION (flag canvas_convert_from_canvas_coord, (KWorldCanvas canvas,
							 double xin,
							 double yin,
							 int *xout,
							 int *yout) );
EXTERN_FUNCTION (void canvas_register_convert_func,
		 (KWorldCanvas canvas, flag (*coord_convert_func) (),
		  void *info) );

/*  Drawing routines  */
EXTERN_FUNCTION (flag canvas_draw_image, (KWorldCanvas canvas,
					  array_desc *arr_desc, char *slice,
					  unsigned int hdim, unsigned int vdim,
					  unsigned int elem_index,
					  KPixCanvasImageCache *cache_ptr) );
EXTERN_FUNCTION (void canvas_draw_point, (KWorldCanvas canvas,
					  double x, double y,
					  double value[2]) );
EXTERN_FUNCTION (void canvas_draw_point_p, (KWorldCanvas canvas,
					    double x, double y,
					    unsigned long pixel_value) );
EXTERN_FUNCTION (void canvas_draw_line, (KWorldCanvas canvas,
					  double x0, double y0,
					  double x1, double y1,
					  double value[2]) );
EXTERN_FUNCTION (void canvas_draw_line_p, (KWorldCanvas canvas,
					   double x0, double y0,
					   double x1, double y1,
					   unsigned long pixel_value) );
EXTERN_FUNCTION (void canvas_fill_ellipse, (KWorldCanvas canvas,
					    double centre_x, double centre_y,
					    double radius_x, double radius_y,
					    double value[2]) );
EXTERN_FUNCTION (void canvas_fill_ellipse_p, (KWorldCanvas canvas,
					      double centre_x, double centre_y,
					      double radius_x, double radius_y,
					      unsigned long pixel_value) );
EXTERN_FUNCTION (flag canvas_fill_polygon, (KWorldCanvas canvas,
					    edit_coord *coords,
					    unsigned int num_vertices,
					    double value[2], flag convex) );
EXTERN_FUNCTION (void canvas_draw_rectangle, (KWorldCanvas canvas,
					      double x, double y,
					      double width, double height,
					      double value[2]) );
EXTERN_FUNCTION (void canvas_draw_rectangle_p, (KWorldCanvas canvas,
						double x, double y,
						double width, double height,
						unsigned long pixel_value) );
EXTERN_FUNCTION (void canvas_fill_rectangle, (KWorldCanvas canvas,
					      double x, double y,
					      double width, double height,
					      double value[2]) );
EXTERN_FUNCTION (void canvas_fill_rectangle_p, (KWorldCanvas canvas,
						double x, double y,
						double width, double height,
						unsigned long pixel_value) );

/*  Image editing related functions  */
/*  File:  canvas_edit.c  */
EXTERN_FUNCTION (flag canvas_draw_edit_list, (KWorldCanvas canvas,
					      KImageEditList ilist) );
EXTERN_FUNCTION (flag canvas_draw_edit_object, (KWorldCanvas canvas,
						char *object) );

/*  File:  cnv_stroke.c  */
EXTERN_FUNCTION (flag canvas_create_stroke_instruction,
		 (KWorldCanvas canvas, double x0, double y0,
		  double x1, double y1, unsigned int brush_width,
		  double value[2], KImageEditList ilist) );


#endif /*  KARMA_CANVAS_H  */

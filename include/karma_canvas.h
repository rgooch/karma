/*  karma_canvas.h

    Header for  canvas_  package.

    Copyright (C) 1993,1994  Richard Gooch

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

    Last updated by Richard Gooch   29-NOV-1994

*/

#ifndef KARMA_CANVAS_H
#define KARMA_CANVAS_H


#include <karma_iedit.h>
#include <karma_kcmap.h>
#include <karma_kwin.h>

#ifndef KARMA_C_DEF_H
#  include <karma_c_def.h>
#endif

typedef struct worldcanvas_type * KWorldCanvas;


/*  Dressing parameters  */
#define KCD_END (unsigned int) 0
#define KCD_DISPLAY (unsigned int) 1              /*  (flag)    */
#define KCD_TOP_TICKS (unsigned int) 2            /*  (flag)    */
#define KCD_BOTTOM_TICKS (unsigned int) 3         /*  (flag)    */
#define KCD_LEFT_TICKS (unsigned int) 4           /*  (flag)    */
#define KCD_RIGHT_TICKS (unsigned int) 5          /*  (flag)    */
#define KCD_INTERNAL_TICKS (unsigned int) 6       /*  (flag)    */
#define KCD_AXES_COLOUR (unsigned int) 7          /*  (char *)  */
#define KCD_AXES_FONT (unsigned int) 8            /*  (char *)  */
#define KCD_SCALE_COLOUR (unsigned int) 9         /*  (char *)  */
#define KCD_SCALE_FONT (unsigned int) 10          /*  (char *)  */
#define KCD_HLABEL (unsigned int) 11              /*  (char *)  */
#define KCD_VLABEL (unsigned int) 12              /*  (char *)  */
#define KCD_LABEL_COLOUR (unsigned int) 13        /*  (char *)  */
#define KCD_LABEL_FONT (unsigned int) 14          /*  (char *)  */
#define KCD_TITLE (unsigned int) 15               /*  (char *)  */
#define KCD_TITLE_COLOUR (unsigned int) 16        /*  (char *)  */
#define KCD_TITLE_FONT (unsigned int) 17          /*  (char *)  */


/*  File:  canvas.c  */
EXTERN_FUNCTION (KWorldCanvas canvas_create,
		 (KPixCanvas pixcanvas, Kcolourmap cmap,
		  struct win_scale_type *win_scale) );
EXTERN_FUNCTION (KCallbackFunc canvas_register_refresh_func,
		 (KWorldCanvas canvas, void (*refresh_func) (), void *info) );
EXTERN_FUNCTION (void canvas_register_size_control_func,
		 (KWorldCanvas canvas, void (*size_control_func) (),
		  void *info) );
EXTERN_FUNCTION (KCallbackFunc canvas_register_position_event_func,
		 (KWorldCanvas canvas, flag (*position_func) (),
		  void *f_info) );
EXTERN_FUNCTION (flag canvas_resize, (KWorldCanvas canvas,
				      struct win_scale_type *win_scale,
				      flag always_clear) );
EXTERN_FUNCTION (void canvas_get_size, (KWorldCanvas canvas,
					int *width, int *height,
					struct win_scale_type *win_scale) );
EXTERN_FUNCTION (flag canvas_specify,
		 (KWorldCanvas canvas, char *xlabel, char *ylabel,
		  unsigned int num_restr, char **restr_names,
		  double *restr_values) );
EXTERN_FUNCTION (void canvas_get_specification,
		 (KWorldCanvas canvas, char **xlabel, char **ylabel,
		  unsigned int *num_restr, char ***restr_names,
		  double **restr_values) );
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
EXTERN_FUNCTION (flag canvas_coord_transform,
		 (KWorldCanvas canvas, double *x, double *y, flag to_linear) );
EXTERN_FUNCTION (void canvas_register_transform_func,
		 (KWorldCanvas canvas, void (*coord_transform_func) (),
		  void *info) );
EXTERN_FUNCTION (KPixCanvas canvas_get_pixcanvas, (KWorldCanvas canvas) );
EXTERN_FUNCTION (flag canvas_get_colour,
		 (KWorldCanvas canvas, char *colourname,
		  unsigned long *pixel_value, unsigned short *red,
		  unsigned short *green, unsigned short *blue) );
#ifndef KCANVAS_INTERNAL
EXTERN_FUNCTION (void canvas_set_dressing, (KWorldCanvas canvas, ...) );
#endif
EXTERN_FUNCTION (void canvas_sequence_dressing_refresh, (KWorldCanvas canvas));
EXTERN_FUNCTION (void canvas_draw_dressing, (KWorldCanvas canvas) );
EXTERN_FUNCTION (Kcolourmap canvas_get_cmap, (KWorldCanvas canvas) );


/*  Drawing routines  */
EXTERN_FUNCTION (flag canvas_draw_image, (KWorldCanvas canvas,
					  array_desc *arr_desc, char *slice,
					  unsigned int hdim, unsigned int vdim,
					  unsigned int elem_index,
					  KPixCanvasImageCache *cache_ptr) );
EXTERN_FUNCTION (flag canvas_draw_rgb_image,
		 (KWorldCanvas canvas, array_desc *arr_desc, CONST char *slice,
		  unsigned int hdim, unsigned int vdim,
		  unsigned int red_index, unsigned int green_index,
		  unsigned int blue_index, KPixCanvasImageCache *cache_ptr) );
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
EXTERN_FUNCTION (void canvas_draw_lines, (KWorldCanvas canvas,
					  double *x_array, double *y_array,
					  int num_points, double value[2]) );
EXTERN_FUNCTION (void canvas_draw_lines_p, (KWorldCanvas canvas,
					    double *x_array, double *y_array,
					    int num_points,
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

/*  File:  canvas_misc.c  */
EXTERN_FUNCTION (void canvas_init_win_scale,
		 (struct win_scale_type *win_scale,
		  unsigned int magic_number) );
EXTERN_FUNCTION (void canvas_use_log_scale,
		 (KWorldCanvas canvas, flag x_log, flag y_log) );


#endif /*  KARMA_CANVAS_H  */

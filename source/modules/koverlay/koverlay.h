/*  koverlay.h

    Include file for  koverlay  (X11 image+contour display tool for Karma).

    Copyright (C) 1996  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*
    This Karma module will enable on-screen display of an image overlayed with
    contours from another image/cube.
    This module runs on an X11 server.


    Written by      Richard Gooch   13-OCT-1996

    Last updated by Richard Gooch   16-OCT-1996: Renamed to <koverlay>.


*/

#include <karma.h>
#include <karma_viewimg.h>
#include <karma_wcs.h>


#define MAX_CONTOUR_LEVELS 20

#define IMAGE_MODE_LOADED             0
#define IMAGE_MODE_MOM0               1
#define IMAGE_MODE_MOM1               2
#define IMAGE_MODE_CHANNEL0           3
#define NUM_IMAGE_MODE_ALTERNATIVES   4

#ifdef NEED_IMAGE_MODE_ALTERNATIVES
static char *image_mode_alternatives[] =
{
    "loaded image",
    "0th moment",
    "1st moment",
    "0th channel",
};
#endif


/*  File: main_xt.c  */
EXTERN_FUNCTION (void load_image_and_setup, (CONST char *filename) );
EXTERN_FUNCTION (void refresh_canvas, () );

/*  File: generic.c  */
#ifndef KWIN_GENERIC_ONLY
EXTERN_FUNCTION (void setup_comms, (Display *display) );
#endif
EXTERN_FUNCTION (flag load_image,
		 (CONST char *inp_filename,
		  KWorldCanvas pseudo_canvas, KWorldCanvas rgb_canvas,
		  KWorldCanvas mag_pseudo_canvas, KWorldCanvas mag_rgb_canvas,
		  iarray *pseudo_arr,
		  ViewableImage *image, ViewableImage **movie,
		  ViewableImage *magnified_image,
		  ViewableImage **magnified_movie,
		  unsigned int *num_frames, double *min, double *max) );
EXTERN_FUNCTION (flag load_contour,
		 (CONST char *inp_filename, iarray *contour_arr,
		  KWorldCanvas pseudo_canvas, KWorldCanvas rgb_canvas,
		  double *min, double *max) );
EXTERN_FUNCTION (flag copy_header_info, (iarray out, iarray in) );
EXTERN_FUNCTION (void convert_lin_world_coords,
		 (double *xout, double *yout, KwcsAstro out_ap,
		  CONST double *xin, CONST double *yin, KwcsAstro in_ap,
		  unsigned int num_coords) );

/*  File: event.c  */
EXTERN_FUNCTION (flag endpoint_position_func,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info,
		  void **f_info, double x_lin, double y_lin) );
EXTERN_FUNCTION (track_aux_canvas_event,
		 (ViewableImage vimage, double x, double y,
		  void *value, unsigned int event_code,
		  void *e_info, void **f_info,
		  double x_lin, double y_lin, unsigned int value_type) );
EXTERN_FUNCTION (void main_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize, void **info,
		  PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );
EXTERN_FUNCTION (void trace_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize, void **info,
		  PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );

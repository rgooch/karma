/*  kshell.h

    Include file for  kshell  (X11 ellipse integrator tool for Karma).

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
    This Karma module will enable interactive integration along the
    circumference of concentric ellipses.
    This module runs on an X11 server.


    Written by      Richard Gooch   24-SEP-1996: Copied from kpvslice module.

    Last updated by Richard Gooch   1-OCT-1996


*/

#include <karma.h>
#include <karma_viewimg.h>
#include <karma_wcs.h>


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
EXTERN_FUNCTION (void load_and_setup, (CONST char *filename) );
EXTERN_FUNCTION (void print_slice_info, (CONST char *window_string) );
EXTERN_FUNCTION (flag track_aux_canvas_xtcoord,
		 (CONST char *track_txt, CONST int track_num)   );
EXTERN_FUNCTION (flag track_aux_canvas_xtmagupd, (ViewableImage vimage,
						  double x_lin,
						  double y_lin) );

/*  File: generic.c  */
#ifndef KWIN_GENERIC_ONLY
EXTERN_FUNCTION (void setup_comms, (Display *display) );
#endif
EXTERN_FUNCTION (iarray load_image,
		 (CONST char *inp_filename,
		  KWorldCanvas pseudo_canvas, KWorldCanvas mag_pseudo_canvas,
		  ViewableImage *image, ViewableImage *magnified_image) );
EXTERN_FUNCTION (iarray load_cube,
		 (CONST char *inp_filename, double *min, double *max) );
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

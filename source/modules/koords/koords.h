/*  koords.h

    Include file for  koords  (X11 co-ordinate generator tool for Karma).

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
    This Karma module will enable interactive generation of an astronomical
    co-ordinate system on a target image, using a reference image.
    This module runs on an X11 server.


    Written by      Richard Gooch   14-OCT-1996: Copied from kshell/kshell.h

    Last updated by Richard Gooch   3-NOV-1996


*/

#include <karma.h>
#include <karma_viewimg.h>
#include <karma_wcs.h>


#define MAX_PAIRS 30


/*  File: main_xt.c  */
EXTERN_FUNCTION (void ring_bell, () );
EXTERN_FUNCTION (flag track_aux_canvas_xtcoord,
		 (CONST char *track_txt, CONST int track_num)   );
EXTERN_FUNCTION (flag track_aux_canvas_xtmagupd, (ViewableImage vimage,
						  double x_lin,
						  double y_lin) );

/*  File: generic.c  */
#ifndef KWIN_GENERIC_ONLY
EXTERN_FUNCTION (void setup_comms, (Display *display) );
#endif
EXTERN_FUNCTION (flag load_image,
		 (CONST char *filename, iarray *array, KwcsAstro *ap, flag ref,
		  KWorldCanvas pseudo_canvas, KWorldCanvas mag_pseudo_canvas,
		  ViewableImage *image, ViewableImage *magnified_image,
		  double *min, double *max) );
EXTERN_FUNCTION (void convert_lin_world_coords,
		 (double *xout, double *yout, KwcsAstro out_ap,
		  CONST double *xin, CONST double *yin, KwcsAstro in_ap,
		  unsigned int num_coords) );

/*  File: event.c  */
EXTERN_FUNCTION (flag reference_event_func,
		 (ViewableImage vimage, double x, double y,
		  void *value, unsigned int event_code,
		  void *e_info, void **f_info,
		  double x_lin, double y_lin, unsigned int value_type) );
EXTERN_FUNCTION (flag target_event_func,
		 (ViewableImage vimage, double x, double y,
		  void *value, unsigned int event_code,
		  void *e_info, void **f_info,
		  double x_lin, double y_lin, unsigned int value_type) );
EXTERN_FUNCTION (void ref_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize, void **info,
		  PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );
EXTERN_FUNCTION (void tar_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize, void **info,
		  PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );
EXTERN_FUNCTION (void update_magnifier,
		 (KWorldCanvas canvas, double x, double y,
		  KWorldCanvas main_canvas) );

/*  File: compute.c  */
EXTERN_FUNCTION (void compute_and_store, () );
EXTERN_FUNCTION (void apply_coordinates, () );

/*  File: solve_im.c  */
EXTERN_FUNCTION (unsigned int compute_coords,
		 (double *crval1, double *crpix1, double *cdelt1,
		  double *crval2, double *crpix2, double *cdelt2,
		  double *crota, CONST char *projection,
		  unsigned int num_pairs,
		  CONST double *ra, CONST double *dec,
		  CONST double *x, CONST double *y,
		  unsigned int naxis1, unsigned int naxis2) );

/*  File: mat.c  */
EXTERN_FUNCTION (void mat_mult,
		 (double *c, double *a, double *b,
		  int rowsa, int colsa, int colsb) );
EXTERN_FUNCTION (void mat_add,
		 (double *c, double *a, double *b, int row, int col) );
EXTERN_FUNCTION (void mat_sub,
		 (double *c, double *a, double *b, int row, int col) );

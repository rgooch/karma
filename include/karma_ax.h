/*  karma_ax.h

    Header for  ax_  package.

    Copyright (C) 1992,1993,1994,1995  Richard Gooch

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
  needed to interface to the ax_ routines in the Karma library.


    Written by      Richard Gooch   25-SEP-1992

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#if !defined(K_WIN_SCALE_H) || defined(MAKEDEPEND)
#  include <k_win_scale.h>
#endif

#include <X11/Xlib.h>

#ifndef KARMA_AX_H
#define KARMA_AX_H


/*  File:   axes.c   */
EXTERN_FUNCTION (flag ax_plot_dressing, (Display *display, Window window,
					 GC gc, char *title_string,
					 char *abscissa_label,
					 char *ordinate_label,
					 char *title_font_name,
					 char *axes_font_name,
					 char *scale_font_name,
					 struct win_scale_type *win_scale,
					 unsigned int max_log_cycles,
					 void (*error_notify_func) () ) );
EXTERN_FUNCTION (flag ax_choose_scale, (double min, double max, flag log,
					struct scale_type *new_scale,
					unsigned int max_log_cycles,
					void (*error_notify_func) () ) );
EXTERN_FUNCTION (flag ax_draw_ordinate_scale,
		 (Display *display, Window window, GC gc,
		  XFontStruct *font_info, struct win_scale_type *win_scale,
		  unsigned int max_log_cycles,
		  void (*error_notify_func) () ) );
EXTERN_FUNCTION (flag ax_draw_abscissa_scale,
		 (Display *display, Window window, GC gc,
		  XFontStruct *font_info, int scale_offset,
		  struct win_scale_type *win_scale,
		  unsigned int max_log_cycles,
		  void (*error_notify_func) () ) );
EXTERN_FUNCTION (void ax_set_zoom_h_info, (int x1, int x2, flag *abs_zoomed,
					   struct win_scale_type *win_scale) );
EXTERN_FUNCTION (void ax_set_zoom_v_info, (int y1, int y2, flag *ord_zoomed,
					   struct win_scale_type *win_scale) );
EXTERN_FUNCTION (flag ax_unset_zoom_info, (flag *abs_zoomed,
					   flag *ord_zoomed) );
EXTERN_FUNCTION (flag ax_verify_crosshair_location,
		 (int x, int y, struct win_scale_type *win_scale) );
EXTERN_FUNCTION (double ax_choose_ord_intvl, (double input_interval) );
EXTERN_FUNCTION (double ax_pixel_to_abscissa,
		 (int x, struct win_scale_type *win_scale) );
EXTERN_FUNCTION (double ax_pixel_to_ordinate,
		 (int y, struct win_scale_type *win_scale) );


#endif /*  KARMA_AX_H  */

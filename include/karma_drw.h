/*  karma_drw.h

    Header for  drw_  package.

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
  needed to interface to the drw_ routines in the Karma library.


    Written by      Richard Gooch   3-OCT-1992

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_DS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ds_def.h>
#endif

#if !defined(K_WIN_SCALE_H) || defined(MAKEDEPEND)
#  include <k_win_scale.h>
#endif

#include <X11/Xlib.h>

#ifndef KARMA_DRW_H
#define KARMA_DRW_H


/*  File:   drw_image.c   */
EXTERN_FUNCTION (flag drw_single_plane, (XImage *ximage,
					 unsigned int num_pixels,
					 unsigned long *pixel_values,
					 char *data, unsigned int elem_type,
					 unsigned int conv_type,
					 dim_desc *abs_dim_desc,
					 unsigned int abs_dim_stride,
					 dim_desc *ord_dim_desc,
					 unsigned int ord_dim_stride,
					 struct win_scale_type *win_scale) );


#endif /*  KARMA_DRW_H  */

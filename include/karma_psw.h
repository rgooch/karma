/*  karma_psw.h

    Header for  psw_  package.

    Copyright (C) 1994,1995  Richard Gooch

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
  needed to interface to the psw_ routines in the Karma library.


    Written by      Richard Gooch   10-MAY-1994

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#if !defined(KARMA_CH_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ch_def.h>
#endif

#if !defined(KARMA_PSW_DEF_H) || defined(MAKEDEPEND)
#  include <karma_psw_def.h>
#endif

#ifndef KARMA_PSW_H
#define KARMA_PSW_H



/*  File:   ps_write.c   */
EXTERN_FUNCTION (PostScriptPage psw_create, (Channel channel,
					     double hoffset, double voffset,
					     double hsize, double vsize,
					     flag portrait) );
EXTERN_FUNCTION (flag psw_mono_image,
		 (PostScriptPage pspage, CONST unsigned char *image,
		  unsigned int xlen, unsigned int ylen,
		  CONST uaddr *xoffsets, CONST uaddr *yoffsets,
		  CONST unsigned char imap[256],
		  double xstart, double ystart, double xend, double yend) );
EXTERN_FUNCTION (flag psw_pseudocolour_image,
		 (PostScriptPage pspage, CONST unsigned char *image,
		  unsigned int xlen, unsigned int ylen,
		  CONST uaddr *xoffsets, CONST uaddr *yoffsets,
		  CONST unsigned char imap_red[256],
		  CONST unsigned char imap_green[256],
		  CONST unsigned char imap_blue[256],
		  double xstart, double ystart, double xend, double yend) );
EXTERN_FUNCTION (flag psw_rgb_image,
		 (PostScriptPage pspage, CONST unsigned char *image_reds,
		  CONST unsigned char *image_greens,
		  CONST unsigned char *image_blues,
		  unsigned int xlen, unsigned int ylen,
		  CONST uaddr *xoffsets_red, CONST uaddr *yoffsets_red,
		  CONST uaddr *xoffsets_green, CONST uaddr *yoffsets_green,
		  CONST uaddr *xoffsets_blue, CONST uaddr *yoffsets_blue,
		  uaddr stride,
		  double xstart, double ystart, double xend, double yend) );
EXTERN_FUNCTION (flag psw_finish, (PostScriptPage pspage, flag eps, flag flush,
				   flag close) );
EXTERN_FUNCTION (flag psw_rgb_line,
		 (PostScriptPage pspage, double red, double green, double blue,
		  double xstart, double ystart, double xend, double yend) );
EXTERN_FUNCTION (flag psw_rgb_polygon,
		 (PostScriptPage pspage, double red, double green, double blue,
		  CONST double *x_arr, CONST double *y_arr,
		  unsigned int num_points,
		  flag fill) );
EXTERN_FUNCTION (flag psw_rgb_ellipse,
		 (PostScriptPage pspage, double red, double green, double blue,
		  double cx, double cy, double rx, double ry, flag fill) );
EXTERN_FUNCTION (flag psw_rgb_text,
		 (PostScriptPage pspage, double red, double green, double blue,
		  CONST char *string, CONST char *fontname,
		  unsigned int fontsize, double xstart, double ystart,
		  double angle) );


#endif /*  KARMA_PSW_H  */

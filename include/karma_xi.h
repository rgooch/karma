/*  karma_xi.h

    Header for  xi_  package.

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
  needed to interface to the xi_ routines in the Karma library.


    Written by      Richard Gooch   21-SEP-1992

    Last updated by Richard Gooch   8-SEP-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif
#include <X11/Xlib.h>

#ifndef KARMA_XI_H
#define KARMA_XI_H


/*  File:   xi.c   */
EXTERN_FUNCTION (flag xi_check_shared_images_available, (Display *display) );
EXTERN_FUNCTION (XImage *xi_create_image, (Display *display, Window window,
					   unsigned int image_width,
					   unsigned int image_height,
					   flag *share) );
EXTERN_FUNCTION (XImage *xi_create_shm_image,
		 (Display *display, Visual *visual, int depth,
		  unsigned int width, unsigned int height, flag quiet) );
EXTERN_FUNCTION (void xi_destroy_image, (Display *display, XImage *ximage,
					 flag shared_memory) );
EXTERN_FUNCTION (void xi_put_image, (Display *display, Drawable drawable,
				     GC gc, XImage *ximage,
				     int src_x, int src_y,
				     int dest_x, int dest_y,
				     unsigned int width, unsigned int height,
				     flag shared_memory, flag wait) );


#endif /*  KARMA_XI_H  */

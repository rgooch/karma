/*  karma_xv.h

    Header for  xv_  package.

    Copyright (C) 1996  Richard Gooch

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
  needed to interface to the xv_ routines in the Karma library.


    Written by      Richard Gooch   3-DEC-1996

    Last updated by Richard Gooch   3-DEC-1996

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif
#include <X11/Xlib.h>
#include <X11/Xutil.h>

#ifndef KARMA_XV_H
#define KARMA_XV_H


/*  For the file: visuals.c  */
EXTERN_FUNCTION (void xv_get_vinfos,
		 (Screen *screen, XVisualInfo **pseudocolour,
		  XVisualInfo **truecolour, XVisualInfo **directcolour) );
EXTERN_FUNCTION (void xv_get_visuals,
		 (Screen *screen, Visual **pseudocolour,
		  Visual **truecolour, Visual **directcolour) );
EXTERN_FUNCTION (XVisualInfo *xv_get_visinfo_for_visual,
		 (Display *dpy, Visual *visual) );


#endif /*  KARMA_XV_H  */

/*LINTLIBRARY*/
#if defined(X11) && !defined(HAS_XGL)
/*  stub_xgl.c

    This code provides KPixCanvas objects.

    Copyright (C) 1995  Richard Gooch

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

/*  This file contains all routines needed for manipulating a simple pixel
    canvas (window) independent of the graphics system in use. This file
    contains the stub XGL code.


    Written by      Richard Gooch   22-SEP-1995

    Last updated by Richard Gooch   13-OCT-1995: Changed names.


*/
#include <stdio.h>
#include <string.h>
#include <math.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_kwin.h>


/*STUB_FUNCTION*/
unsigned int kwin_xgl_test_stereo (Display *display, Window window)
{
    return (KWIN_XGL_NOT_AVAILABLE);
}   /*  End Function kwin_test_xgl_stereo  */

/*STUB_FUNCTION*/
flag kwin_xgl_create_stereo (Display *display, Window window,
			     int xoff, int yoff, int width, int height,
			     KPixCanvas *mono,
			     KPixCanvas *left, KPixCanvas *right)
{
    static char function_name[] = "kwin_create_xgl_stereo";

    (void) fprintf (stderr, "%s: XGL not available!\n", function_name);
    return (FALSE);
}   /*  End Function kwin_create_xgl_stereo  */
#endif

/*  CanvasP.h

    Private header for  Canvas  widget class.

    Copyright (C) 1994  Richard Gooch

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

    This include file contains the private class declarations for the Canvas
  widget.


    Written by      Richard Gooch   17-JUL-1994

    Last updated by Richard Gooch   24-DEC-1994

*/

#ifndef _CanvasP_h
#define _CanvasP_h

#include <Xkw/Canvas.h>

typedef struct {
    Boolean	(*change_sensitive)(/* widget */);
} CanvasClassPart;

#define XtInheritChangeSensitive ((Boolean (*)())_XtInherit)

typedef struct _CanvasClassRec {
    CoreClassPart	core_class;
    CanvasClassPart	canvas_class;
} CanvasClassRec;

extern CanvasClassRec canvasClassRec;

typedef struct {
    /* resources */
    Pixmap	insensitive_border;

    Visual *    visual;
    KPixCanvas  pixcanvas;
    Boolean     clip;
    Boolean     silence_unconsumed_events;
    Pixel       foreground_pixel;
    Boolean     force_new_cmap;
    Boolean     retain_fgbg;
    XtCallbackList realiseCallback;
    /* private state */
    flag        cmap_installed;
    flag        cmap_installed_internally;
    flag        mapped;
    flag        partly_unobscured;
} CanvasPart;

typedef struct _CanvasRec {
    CorePart	core;
    CanvasPart	canvas;
} CanvasRec;

#endif /* _CanvasP_h */

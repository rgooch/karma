/*  Canvaskarma_conn.h

    Public header for  Canvas  widget class.

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

    This include file contains the public class declarations for the Canvas
  widget.


    Written by      Richard Gooch   17-JUL-1994

    Last updated by Richard Gooch   24-DEC-1994

*/

#ifndef _Canvas_h
#define _Canvas_h

#include <X11/Xmu/Converters.h>
#include <X11/Shell.h>
#include <karma_kwin.h>

/****************************************************************
 *
 * Canvas widgets
 *
 ****************************************************************/

/* Resources:

 Name		     Class		RepType		Default Value
 ----		     -----		-------		-------------
 background	     Background		Pixel		XtDefaultBackground
 border		     BorderColor	Pixel		XtDefaultForeground
 borderWidth	     BorderWidth	Dimension	1
 destroyCallback     Callback		Pointer		NULL
 height		     Height		Dimension	0
 insensitiveBorder   Insensitive	Pixmap		Gray
 mappedWhenManaged   MappedWhenManaged	Boolean		True
 sensitive	     Sensitive		Boolean		True
 width		     Width		Dimension	0
 x		     Position		Position	0
 y		     Position		Position	0
 visual              Visual             Pointer         CopyFromParent
 pixCanvas           PixCanvas          Pointer         NULL
 clipEvents          ClipEvents         Boolean         FALSE
 silenceUnconsumed   SilenceUnconsumed  Boolean         FALSE
 foreground	     Foreground		Pixel		XtDefaultForeground
 forceNewCmap	     ForceNewCmap	Boolean		False
 retainFgBg	     RetainFgBg		Boolean		False
 realiseCallback     Callback           Callback        NULL

*/

#define XtNinsensitiveBorder "insensitiveBorder"
#define XkwNpixCanvas "pixCanvas"
#define XkwNclipEvents "clipEvents"
#define XkwNsilenceUnconsumed "silenceUnconsumed"
#define XkwNforceNewCmap "forceNewCmap"
#define XkwNretainFgBg "retainFgBg"
#define XkwNrealiseCallback "realiseCallback"

#define XtCInsensitive "Insensitive"
#define XkwCPixCanvas "PixCanvas"
#define XkwCClipEvents "ClipEvents"
#define XkwCSilenceUnconsumed "SilenceUnconsumed"
#define XkwCForceNewCmap "ForceNewCmap"
#define XkwCRetainFgBg "RetainFgBg"

typedef struct _CanvasClassRec	*CanvasWidgetClass;
typedef struct _CanvasRec	*CanvasWidget;

extern WidgetClass canvasWidgetClass;

#endif /* _Canvas_h */

/*  Canvas.h

    Public header for  Canvas  widget class.

    Copyright (C) 1994-1996  Richard Gooch

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

    Last updated by Richard Gooch   4-MAY-1996

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

 Name                  Class             RepType         Default Value
 ----                  -----             -------         -------------
 XtNbackground         Background        Pixel           XtDefaultBackground
 XtNborder             BorderColor       Pixel           XtDefaultForeground
 XtNborderWidth        BorderWidth       Dimension       1
 XtNdestroyCallback    Callback          Pointer         NULL
 XtNheight             Height            Dimension       0
 XtNmappedWhenManaged  MappedWhenManaged Boolean         True
 XtNsensitive          Sensitive         Boolean         True
 XtNwidth              Width             Dimension       0
 XtNx                  Position          Position        0
 XtNy                  Position          Position        0
 XtNvisual             Visual            Pointer         CopyFromParent
 XtNcolormap           Colormap          Colormap        dynamic
 XkwNstereoMode        StereoMode        Int             XkwSTEREO_MODE_MONO
 XkwNverticalSplit     VerticalSplit     Bool            False
 XkwNsplitSeparation   SplitSeparation   Int             0
 XkwNmonoPixCanvas     PixCanvas         Pointer         NULL
 XkwNleftPixCanvas     PixCanvas         Pointer         NULL
 XkwNrightPixCanvas    PixCanvas         Pointer         NULL
 XkwNclipEvents        ClipEvents        Bool            FALSE
 XkwNsilenceUnconsumed SilenceUnconsumed Bool            FALSE
 XtNforeground         Foreground        Pixel           XtDefaultForeground
 XkwNforceNewCmap      ForceNewCmap      Bool            False
 XkwNretainFgBg        RetainFgBg        Bool            False
 XkwNrealiseCallback   Callback          Callback        NULL
 XtNfont               Font              String          NULL
 XkwNverbose           Verbose           Bool            False

*/

#define XkwNstereoMode "stereoMode"
#define XkwNverticalSplit "verticalSplit"
#define XkwNsplitSeparation "splitSeparation"
#define XkwNmonoPixCanvas "monoPixCanvas"
#define XkwNleftPixCanvas "leftPixCanvas"
#define XkwNrightPixCanvas "rightPixCanvas"
#define XkwNpixCanvas XkwNmonoPixCanvas  /*  For backwards compatibility  */
#define XkwNclipEvents "clipEvents"
#define XkwNsilenceUnconsumed "silenceUnconsumed"
#define XkwNforceNewCmap "forceNewCmap"
#define XkwNretainFgBg "retainFgBg"
#define XkwNrealiseCallback "realiseCallback"
#define XkwNverbose "verbose"

#define XkwCStereoMode "StereoMode"
#define XkwCVerticalSplit "VerticalSplit"
#define XkwCSplitSeparation "SplitSeparation"
#define XkwCPixCanvas "PixCanvas"
#define XkwCClipEvents "ClipEvents"
#define XkwCSilenceUnconsumed "SilenceUnconsumed"
#define XkwCForceNewCmap "ForceNewCmap"
#define XkwCRetainFgBg "RetainFgBg"
#define XkwCVerbose "Verbose"

#define XkwSTEREO_MODE_MONO      0
#define XkwSTEREO_MODE_XMBUF     1
#define XkwSTEREO_MODE_XGL       2
#define XkwSTEREO_MODE_OpenGL    3
#define XkwSTEREO_MODE_SPLIT     4

typedef struct _CanvasClassRec  *CanvasWidgetClass;
typedef struct _CanvasRec       *CanvasWidget;

extern WidgetClass canvasWidgetClass;

#endif /* _Canvas_h */

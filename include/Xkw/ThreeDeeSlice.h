/*  ThreeDeeSliceP.h

    Public header for  ThreeDeeSlice  widget class.

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

    This include file contains the public class declarations for the
  ThreeDeeSlice widget, a simple three-dimensional slicer widget for Xt.


    Written by      Richard Gooch   30-JUL-1995

    Last updated by Richard Gooch   9-NOV-1996

*/

/*----------------------------------------------------------------------*/
/* This code provides a 3D slicer widget for Xt. */
/**/
/*
 Name		         Class		   RepType         Default Value
 ----		         -----		   -------         -------------
 iarray                  Iarray            Pointer         NULL
 karmaColourmap          KarmaColourmap    Pointer         NULL
 minPtr                  MinPtr            Pointer         NULL
 maxPtr                  MaxPtr            Pointer         NULL
 canvasVisual            Visual            Pointer         CopyFromParent
 verbose                 Verbose           Bool            False
 XkwNcursorCallback      Callback          Callback        NULL

*/    
/*----------------------------------------------------------------------*/

#ifndef THREEDEESLICE__H
#define THREEDEESLICE__H

#if !defined(KARMA_IARRAY_DEF_H) || defined(MAKEDEPEND)
#  include <karma_iarray_def.h>
#endif

#if !defined(KARMA_VRENDER_DEF_H) || defined(MAKEDEPEND)
#  include <karma_vrender_def.h>
#endif

#include <X11/Xmu/Converters.h>

extern WidgetClass threeDeeSliceWidgetClass;
typedef struct _ThreeDeeSliceClassRec *ThreeDeeSliceWidgetClass;
typedef struct _ThreeDeeSliceRec *ThreeDeeSliceWidget;

#define XtIsThreeDeeSlice(w) XtIsSubclass((w), threeDeeSliceWidgetClass)

#define XkwNiarray "iarray"
#define XkwNkarmaColourmap "karmaColourmap"
#define XkwNminPtr "minPtr"
#define XkwNmaxPtr "maxPtr"
#define XkwNcanvasVisual "canvasVisual"
#define XkwNverbose "verbose"
#define XkwNcursorCallback "cursorCallback"

#define XkwCIarray "Iarray"
#define XkwCKarmaColourmap "KarmaColourmap"
#define XkwCMinPtr "MinPtr"
#define XkwCMaxPtr "MaxPtr"
#define XkwCVerbose "Verbose"

typedef struct XkwThreeDeeSliceCursor
{
    Kdcoord_3d world;   /*  This is in proper world co-ordinates */
    Kdcoord_3d linear;  /*  This is in linear co-ordinates       */
    Kcoord_3d pixel;    /*  This is in data pixel co-ordinates   */
} *XkwThreeDeeSliceCallbackData;

#define XkwThreeDeeSlicePixelCursor 0
#define XkwThreeDeeSliceLinearCursor 1
#define XkwThreeDeeSliceWorldCursor 2

EXTERN_FUNCTION (void XkwThreeDeeSlicePrecompute,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
EXTERN_FUNCTION (void XkwThreeDeeSliceSetCursor,
		 (Widget W, unsigned int type, double x, double y, double z) );


#endif

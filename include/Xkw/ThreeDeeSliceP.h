/*  ThreeDeeSliceP.h

    Private header for  ThreeDeeSlice  widget class.

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

    This include file contains the private class declarations for the
  ThreeDeeSlice widget, a simple three-dimensional slicer widget for Xt.


    Written by      Richard Gooch   30-JUL-1995

    Last updated by Richard Gooch   9-NOV-1996

*/

#ifndef ThreeDeeSliceP__H
#define ThreeDeeSliceP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/FormP.h>
#include <X11/StringDefs.h>

#include <Xkw/ThreeDeeSlice.h>
#include <karma_canvas.h>

typedef struct _ThreeDeeSlicePart
{
    /*  Public resources  */
    iarray         cube;
    Kcolourmap     karmaCmap;
    double         *minPtr;
    double         *maxPtr;
    Visual         *canvasVisual;
    Bool           verbose;
    XtCallbackList cursorCallback;
    /*  Private resources  */
    Widget        track_label0;
    Widget        track_label1;
    KPixCanvas    parent_pixcanvas;
    KPixCanvas    xy_pixcanvas;
    KPixCanvas    xz_pixcanvas;
    KPixCanvas    zy_pixcanvas;
    KPixCanvas    last_event_canvas;
    KWorldCanvas  xy_worldcanvas;
    KWorldCanvas  xz_worldcanvas;
    KWorldCanvas  zy_worldcanvas;
    KwcsAstro     ap;
    ViewableImage *xy_frames;
    ViewableImage *xz_frames;
    ViewableImage *zy_frames;
    int           x_mag;
    int           y_mag;
    int           z_mag;
    KCallbackFunc iarr_destroy_func;
    struct XkwThreeDeeSliceCursor cursor;
} ThreeDeeSlicePart, *ThreeDeeSlicePartPtr;

typedef struct _ThreeDeeSliceRec
{
    CorePart core;
    CompositePart composite;
    ConstraintPart constraint;
    FormPart form;
    ThreeDeeSlicePart threeDeeSlice;
} ThreeDeeSliceRec, *ThreeDeeSlicePtr;

typedef struct _ThreeDeeSliceClassPart
{
    int empty;
} ThreeDeeSliceClassPart;

typedef struct _ThreeDeeSliceClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ConstraintClassPart constraint_class;
    FormClassPart form_class;
    ThreeDeeSliceClassPart threeDeeSlice_class;
} ThreeDeeSliceClassRec, *ThreeDeeSliceClassPtr;

extern ThreeDeeSliceClassRec threeDeeSliceClassRec;

typedef struct {int empty;} ThreeDeeSliceConstraintsPart;

typedef struct _ThreeDeeSliceConstraintsRec
{
    FormConstraintsPart	  form;
    ThreeDeeSliceConstraintsPart ThreeDeeSlice;
} ThreeDeeSliceConstraintsRec, *ThreeDeeSliceConstraints;

#endif

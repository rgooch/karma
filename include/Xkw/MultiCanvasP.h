/*  CanvasP.h

    Private header for  MultiCanvas  widget class.

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

    This include file contains the private class declarations for the
  MultiCanvas widget.


    Written by      Richard Gooch   17-JUL-1994

    Last updated by Richard Gooch   28-DEC-1995

*/

#ifndef MultiCanvasP__H
#define MultiCanvasP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/FormP.h>
#include <X11/StringDefs.h>

#include <Xkw/MultiCanvas.h>

typedef struct _MultiCanvasPart
{
    /*  Public resources  */
    int requestList;
    Bool splitStereo;
    Bool verbose;
    /*  Private resources  */
} MultiCanvasPart, *MultiCanvasPartPtr;

typedef struct _MultiCanvasRec
{
    CorePart core;
    CompositePart composite;
    ConstraintPart constraint;
    FormPart form;
    MultiCanvasPart multiCanvas;
} MultiCanvasRec, *MultiCanvasPtr;

typedef struct _MultiCanvasClassPart
{
    int empty;
} MultiCanvasClassPart;

typedef struct _MultiCanvasClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ConstraintClassPart constraint_class;
    FormClassPart form_class;
    MultiCanvasClassPart multiCanvas_class;
} MultiCanvasClassRec, *MultiCanvasClassPtr;

extern MultiCanvasClassRec multiCanvasClassRec;

typedef struct {int empty;} MultiCanvasConstraintsPart;

typedef struct _MultiCanvasConstraintsRec {
    FormConstraintsPart	  form;
    MultiCanvasConstraintsPart MultiCanvas;
} MultiCanvasConstraintsRec, *MultiCanvasConstraints;

#endif

/*  MomentGenerator.h

    Private header for  MomentGenerator  widget class.

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
  MomentGenerator widget, a simple moment generator widget for Xt.


    Written by      Richard Gooch   11-SEP-1996

    Last updated by Richard Gooch   14-NOV-1996

*/

/*----------------------------------------------------------------------
   This widget implements a moment generator control

 Name		         Class            RepType         Default Value
 ----		         -----            -------         -------------
 XkwNverbose             Verbose          Bool            FALSE
 XkwNmom0Array           Array            Pointer         NULL
 XkwNmom1Array           Array            Pointer         NULL
 XkwNmomentCallback      Callback         Callback        NULL
------------------------------------------------------------------------*/

#ifndef MOMENTGENERATOR__H
#define MOMENTGENERATOR__H

#include <karma.h>
#include <karma_iarray.h>

extern WidgetClass momentGeneratorWidgetClass;
typedef struct _MomentGeneratorClassRec *MomentGeneratorWidgetClass;
typedef struct _MomentGeneratorRec *MomentGeneratorWidget;

#define XtIsMomentGenerator(w) XtIsSubclass((w), momentGeneratorWidgetClass)

#define XkwNverbose "verbose"
#define XkwNmom0Array "mom0Array"
#define XkwNmom1Array "mom1Array"
#define XkwNmomentCallback "momentCallback"

#define XkwCVerbose "Verbose"
#define XkwCArray "Array"

EXTERN_FUNCTION (void XkwMomentGeneratorNewArray,
		 (Widget W, iarray array, double min, double max) );

#endif

/*  TracePopupP.h

    Public header for  TracePopup  widget class.

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
  TracePopup widget, a simple trace display widget for Xt.


    Written by      Richard Gooch   15-SEP-1996

    Last updated by Richard Gooch   9-OCT-1996

*/

/*----------------------------------------------------------------------
   This widget implements a trace display popup

 Name		         Class            RepType         Default Value
 ----		         -----            -------         -------------
 XkwNiarray              Iarray           Pointer         NULL
 XkwNverbose             Verbose          Bool            FALSE
 XkwNcanvasVisual        Visual           Pointer         CopyFromParent
 XkwNworldCanvas         WorldCanvas      Pointer         NULL
 XkwNrealiseCallback     Callback         Callback        NULL
 XkwNkarmaColourmap      KarmaColourmap   Pointer         NULL
------------------------------------------------------------------------*/

#ifndef TRACEPOPUP__H
#define TRACEPOPUP__H

#include <karma_iarray.h>

extern WidgetClass tracePopupWidgetClass;
typedef struct _TracePopupClassRec *TracePopupWidgetClass;
typedef struct _TracePopupRec *TracePopupWidget;

#define XtIsTracePopup(w) XtIsSubclass((w), tracePopupWidgetClass)

#define XkwNiarray "iarray"
#define XkwNverbose "verbose"
#define XkwNcanvasVisual "canvasVisual"
#define XkwNworldCanvas "worldCanvas"
#define XkwNrealiseCallback "realiseCallback"
#define XkwNkarmaColourmap "karmaColourmap"

#define XkwCIarray "Iarray"
#define XkwCVerbose "Verbose"
#define XkwCWorldCanvas "WorldCanvas"
#define XkwCKarmaColourmap "KarmaColourmap"

EXTERN_FUNCTION (void XkwTracePopupNewArray,
		 (Widget W, iarray array, double min, double max) );
EXTERN_FUNCTION (void XkwTracePopupShowTrace,
		 (Widget W, unsigned int *dim_indices, uaddr *coords) );

#endif

/*  TracePopupP.h

    Private header for  TracePopup  widget class.

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
  TracePopup widget, a simple trace display widget for Xt.


    Written by      Richard Gooch   15-SEP-1996

    Last updated by Richard Gooch   9-OCT-1996

*/

#ifndef TracePopupP__H
#define TracePopupP__H

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <karma_canvas.h>

#include <Xkw/TracePopup.h>

typedef struct _TracePopupPart
{
    /*  Public resources  */
    iarray           array;
    Bool             verbose;
    Visual           *canvasVisual; 
    KWorldCanvas     worldCanvas;
    XtCallbackList   realiseCallback;
    Kcolourmap       karmaColourmap;
    /*  Private resources  */
    KCallbackFunc    iarr_destroy_callback;
    double           array_minimum;
    double           array_maximum;
    Pixel            canvas_foreground;
    double           *y_arr;
    unsigned int     buf_len;
    Widget           pswinpopup;
    unsigned int     trace_dim;
    CONST char       *trace;
} TracePopupPart, *TracePopupPartPtr;

typedef struct _TracePopupRec
{
    CorePart core;
    CompositePart composite;
    ShellPart shell;
    WMShellPart wm;
    VendorShellPart vendor;
    TopLevelShellPart topLevel;
    TracePopupPart tracePopup;
} TracePopupRec, *TracePopupPtr;

typedef struct _TracePopupClassPart
{
    int empty;
} TracePopupClassPart;

typedef struct _TracePopupClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ShellClassPart shell_class;
    WMShellClassPart wm_shell_class;
    VendorShellClassPart vendor_shell_class;
    TopLevelShellClassPart top_level_shell_class;
    TracePopupClassPart tracePopup_class;
} TracePopupClassRec, *TracePopupClassPtr;

extern TracePopupClassRec tracePopupClassRec;

#endif

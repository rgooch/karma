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

#ifndef MomentGeneratorP__H
#define MomentGeneratorP__H

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <karma_canvas.h>
#include <karma_wcs.h>

#include <Xkw/MomentGenerator.h>

typedef struct _MomentGeneratorPart
{
    /*  Public resources  */
    Bool            verbose;
    XtCallbackList  momentCallback;
    iarray          mom0Array;
    iarray          mom1Array;
    /*  Private resources  */
    iarray          cube_arr;
    KwcsAstro       cube_ap;
    Widget          cube_min_label;
    Widget          cube_max_label;
    Widget          sum_min_label;
    Widget          sum_max_label;
    Widget          lower_clip_dlg;
    Widget          sum_clip_dlg;
    KCallbackFunc   iarr_destroy_callback;
    unsigned int    mom1_algorithm;
} MomentGeneratorPart, *MomentGeneratorPartPtr;

typedef struct _MomentGeneratorRec
{
    CorePart core;
    CompositePart composite;
    ShellPart shell;
    WMShellPart wm;
    VendorShellPart vendor;
    TopLevelShellPart topLevel;
    MomentGeneratorPart momentGenerator;
} MomentGeneratorRec, *MomentGeneratorPtr;

typedef struct _MomentGeneratorClassPart
{
    int empty;
} MomentGeneratorClassPart;

typedef struct _MomentGeneratorClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ShellClassPart shell_class;
    WMShellClassPart wm_shell_class;
    VendorShellClassPart vendor_shell_class;
    TopLevelShellClassPart top_level_shell_class;
    MomentGeneratorClassPart momentGenerator_class;
} MomentGeneratorClassRec, *MomentGeneratorClassPtr;

extern MomentGeneratorClassRec momentGeneratorClassRec;

#endif

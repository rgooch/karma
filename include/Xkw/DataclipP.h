/*  DataclipP.h

    Private header for  Dataclip  widget class.

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

    This include file contains the private class declarations for the
  Dataclip widget, a graphical widget to select data regions for Xt.


    Written by      Richard Gooch   23-OCT-1994

    Last updated by Richard Gooch   2-OCT-1996

*/


#ifndef DataclipP__H
#define DataclipP__H

#include <X11/IntrinsicP.h>
#include <X11/ShellP.h>
#include <X11/StringDefs.h>

#include <karma_canvas.h>

#include <Xkw/Dataclip.h>

typedef struct _DataclipPart
{
    /*  Public resources  */
    iarray          array;
    XtCallbackList  regionCallback;
    int             maxRegions;
    int             pad;
    Bool            verbose;
    Bool            showBlankControl;
    Cardinal        fixedOutputType;
    Bool            autoPopdown;
    /*  Private resources: must start on a  double  boundary  */
    double data_min;
    double data_max;
    double left_pos;
    double *minima;
    double *maxima;
    unsigned int num_regions;
    KPixCanvas pixcanvas;
    KWorldCanvas worldcanvas;
    KCallbackFunc iarr_destroy_callback;
    double *histogram_array;
    unsigned hist_arr_length;
    unsigned hist_buf_length;
    flag immediate_apply;
    flag last_was_left;
    flag blank_data;
    Widget min_label;
    Widget max_label;
    Widget lower_label;
    Widget upper_label;
    Widget cursor_label;
    Widget dialog;
    Widget cnv;
    flag popped_up;
    unsigned int output_type;
} DataclipPart, *DataclipPartPtr;

typedef struct _DataclipRec
{
    CorePart core;
    CompositePart composite;
    ShellPart shell;
    WMShellPart wm;
    VendorShellPart vendor;
    TopLevelShellPart topLevel;
    DataclipPart dataclip;
} DataclipRec, *DataclipPtr;

typedef struct _DataclipClassPart
{
    unsigned int type_index_to_type[NUMTYPES];
    char *type_names[NUMTYPES];
    unsigned int num_types;
} DataclipClassPart;

typedef struct _DataclipClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ShellClassPart shell_class;
    WMShellClassPart wm_shell_class;
    VendorShellClassPart vendor_shell_class;
    TopLevelShellClassPart top_level_shell_class;
    DataclipClassPart dataclip_class;
} DataclipClassRec, *DataclipClassPtr;

extern DataclipClassRec dataclipClassRec;

#endif

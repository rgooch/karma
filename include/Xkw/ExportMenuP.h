/*  ExportMenuP.h

    Private header for  ExportMenu  widget class.

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
  ExportMenu widget, a simple export (non-exclusive) menu widget for Xt.


    Written by      Richard Gooch   1-OCT-1996

    Last updated by Richard Gooch   1-OCT-1996

*/

#ifndef ExportMenuP__H
#define ExportMenuP__H

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/FormP.h>
#include <Xkw/ExportMenu.h>

typedef struct
{
    /*  Public resources  */
    iarray array;
    KWorldCanvas worldCanvas;
    /*  Private resources  */
    unsigned int  export_type;
    Widget        save_dialog_popup;
    Widget        pswinpopup;
} ExportMenuPart, *ExportMenuPartPtr;

typedef struct _ExportMenuRec
{
    CorePart core;
    CompositePart composite;
    ConstraintPart constraint;
    FormPart form;
    ExportMenuPart exportMenu;
} ExportMenuRec, *ExportMenuPtr;

typedef struct _ExportMenuClassPart
{
    int empty;
} ExportMenuClassPart;

typedef struct _ExportMenuClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ConstraintClassPart constraint_class;
    FormClassPart form_class;
    ExportMenuClassPart exportMenu_class;
} ExportMenuClassRec, *ExportMenuClassPtr;

extern ExportMenuClassRec exportMenuClassRec;

typedef struct
{
    int empty;
} ExportMenuConstraintsPart;

typedef struct _ExportMenuConstraintsRec
{
    FormConstraintsPart	  form;
    ExportMenuConstraintsPart ExportMenu;
} ExportMenuConstraintsRec, *ExportMenuConstraints;

#endif

/*  ExclusiveMenuP.h

    Private header for  ExclusiveMenu  widget class.

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
  ExclusiveMenu widget, a simple exclusive menu widget for Xt.


    Written by      Richard Gooch   10-DEC-1994

    Last updated by Richard Gooch   1-OCT-1996

*/

#ifndef ExclusiveMenuP__H
#define ExclusiveMenuP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/MenuButtoP.h>
#include <X11/StringDefs.h>
#include <Xkw/ExclusiveMenu.h>

typedef struct
{
    /*  Public resources  */
    String choiceName;
    XtPointer itemStrings;
    int numItems;
    int *valuePtr;
    XtCallbackList selectCallback;
    /*  Private resources  */
    struct item_data_type *item_data;
} ExclusiveMenuPart, *ExclusiveMenuPartPtr;

typedef struct _ExclusiveMenuRec
{
    CorePart            core;
    SimplePart	        simple;
    LabelPart	        label;
    CommandPart	        command;
    MenuButtonPart      menuButton;
    ExclusiveMenuPart   exclusiveMenu;
} ExclusiveMenuRec, *ExclusiveMenuPtr;

typedef struct _ExclusiveMenuClassPart
{
  int empty;
} ExclusiveMenuClassPart;

typedef struct _ExclusiveMenuClassRec
{
    CoreClassPart	    core_class;
    SimpleClassPart	    simple_class;
    LabelClassPart	    label_class;
    CommandClassPart	    command_class;
    MenuButtonClassPart     menuButton_class;
    ExclusiveMenuClassPart  exclusiveMenu_class;
} ExclusiveMenuClassRec, *ExclusiveMenuClassPtr;

extern ExclusiveMenuClassRec exclusiveMenuClassRec;

#endif

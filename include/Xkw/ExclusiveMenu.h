/*  ExclusiveMenu.h

    Public header for  ExclusiveMenu  widget class.

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

    This include file contains the public class declarations for the
  ExclusiveMenu widget, a simple exclusive menu widget for Xt.


    Written by      Richard Gooch   10-DEC-1994

    Last updated by Richard Gooch   1-OCT-1996

*/

/*
   Resources:               Type:                Defaults:
     XkwNchoiceName           String               NULL
     XkwNselectCallback       Callback             NULL
     XkwNitemStrings          Pointer              NULL
     XkwNnumItems             Int                  0
     XkwNvaluePtr             Pointer              NULL

*/    
/*----------------------------------------------------------------------*/

#ifndef EXCLUSIVEMENU__H
#define EXCLUSIVEMENU__H

#include <X11/Xaw/MenuButton.h>

extern WidgetClass exclusiveMenuWidgetClass;
typedef struct _ExclusiveMenuClassRec * ExclusiveMenuWidgetClass;
typedef struct _ExclusiveMenuRec * ExclusiveMenuWidget;

#define XtIsExclusiveMenu(w) XtIsSubclass((w), exclusiveMenuWidgetClass)

#define XkwNchoiceName "choiceName"
#define XkwNitemStrings "itemStrings"
#define XkwNselectCallback "selectCallback"
#define XkwNnumItems "numItems"
#define XkwNvaluePtr "valuePtr"

#define XkwCChoiceName "ChoiceName"
#define XkwCItemStrings "ItemStrings"
#define XkwCNumItems "NumItems"
#define XkwCValuePtr "ValuePtr"

#endif

/*  Cmapwin.h

    Public header for  Cmapwin  widget class.

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

    This include file contains the public class declarations for the Cmapwin
  widget.


    Written by      Richard Gooch   28-JAN-1994

    Last updated by Richard Gooch   4-DEC-1996

*/

/*----------------------------------------------------------------------
   This widget implements a PseudoColour colourmap control box

 Name		          Class                RepType         Default Value
 ----		          -----                -------         -------------
 XkwNcolourbarVisual      Visual               Pointer         CopyFromParent
 XkwNkarmaColourmap       KarmaColourmap       Pointer         NULL
 XkwNcolourCallback       Callback             Callback        NULL
 XkwNregenerateColourmap  RegenerateColourmap  Bool            False
 XkwNsimpleColourbar      SimpleColourbar      Bool            False
 XkwNdisableScaleSliders  DisableScaleSliders  Bool            FALSE

------------------------------------------------------------------------*/

#ifndef CMAPWIN__H
#define CMAPWIN__H

#include <X11/Shell.h>
#include <karma_kcmap.h>

extern WidgetClass cmapwinWidgetClass;
typedef struct _CmapwinClassRec *CmapwinWidgetClass;
typedef struct _CmapwinRec *CmapwinWidget;

#define XtIsCmapwin(w) XtIsSubclass((w), cmapwinWidgetClass)

#define XkwNcolourbarVisual "colourbarVisual"
#define XkwNkarmaColourmap "karmaColourmap"
#define XkwNcolourCallback "colourCallback"
#define XkwNregenerateColourmap "regenerateColourmap"
#define XkwNsimpleColourbar "simpleColourbar"
#define XkwNdisableScaleSliders "disableScaleSliders"

#define XkwCKarmaColourmap "KarmaColourmap"
#define XkwCRegenerateColourmap "RegenerateColourmap"
#define XkwCSimpleColourbar "SimpleColourbar"
#define XkwCDisableScaleSliders "DisableScaleSliders"

void XkwCmapwinSetColourmap (Widget w, char *new_cmap_name);

#endif

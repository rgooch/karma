/*  Cmapwinpopup.h

    Public header for  Cmapwinpopup  widget class.

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
    Cmapwinpopup widget.


    Written by      Richard Gooch   17-DEC-1994

    Last updated by Richard Gooch   4-DEC-1996

*/

/*----------------------------------------------------------------------
   This widget implements a colourmap control popup

 Name		         Class            RepType         Default Value
 ----		         -----            -------         -------------
 XkwNcolourbarVisual     Visual           Pointer         CopyFromParent
 XkwNkarmaColourmap      KarmaColourmap   Pointer         NULL
 XkwNsimpleColourbar     SimpleColourbar  Bool            False

------------------------------------------------------------------------*/

#ifndef CMAPWINPOPUP__H
#define CMAPWINPOPUP__H

#include <Xkw/Cmapwin.h>

extern WidgetClass cmapwinpopupWidgetClass;
typedef struct _CmapwinpopupClassRec *CmapwinpopupWidgetClass;
typedef struct _CmapwinpopupRec *CmapwinpopupWidget;

#define XtIsCmapwinpopup(w) XtIsSubclass((w), cmapwinpopupWidgetClass)

#define XkwNcolourbarVisual "colourbarVisual"
#define XkwNkarmaColourmap "karmaColourmap"
#define XkwNsimpleColourbar "simpleColourbar"

#define XkwCKarmaColourmap "KarmaColourmap"
#define XkwCSimpleColourbar "SimpleColourbar"

#endif

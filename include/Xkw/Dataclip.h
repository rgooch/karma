/*  Dataclip.h

    Public header for  Dataclip  widget class.

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
  Dataclip widget, a graphical widget to select data regions for Xt.


    Written by      Richard Gooch   23-OCT-1994

    Last updated by Richard Gooch   28-OCT-1996

*/


/*----------------------------------------------------------------------
   This widget implements a data clip control

 Name		         Class            RepType         Default Value
 ----		         -----            -------         -------------
 XkwNiarray              Iarray           Pointer         NULL
 XkwNmaxDataRegions      MaxDataRegions   int             1
 XkwNregionCallback      Callback         Callback        NULL
 XkwNshowBlankControl    ShowBlankControl Bool            FALSE
 XkwNfixedOutputType     FixedOutputType  Cardinal        NONE
 XkwNautoPopdown         AutoPopdown      Bool            FALSE
 XkwNverbose             Verbose          Bool            FALSE
------------------------------------------------------------------------*/

#ifndef DATACLIP__H
#define DATACLIP__H

#include <karma_iarray.h>

extern WidgetClass dataclipWidgetClass;
typedef struct _DataclipClassRec *DataclipWidgetClass;
typedef struct _DataclipRec *DataclipWidget;

typedef struct
{
    flag blank_data_outside_regions;
    unsigned int num_regions;
    double *minima;
    double *maxima;
} DataclipRegions;

#define XtIsDataclip(w) XtIsSubclass((w), dataclipWidgetClass)

#define XkwNiarray "iarray"
#define XkwNmaxDataRegions "maxDataRegions"
#define XkwNregionCallback "regionCallback"
#define XkwNshowBlankControl "showBlankControl"
#define XkwNfixedOutputType "fixedOutputType"
#define XkwNautoPopdown "autoPopdown"
#define XkwNverbose "verbose"

#define XkwCIarray "Iarray"
#define XkwCMaxDataRegions "MaxDataRegions"
#define XkwCShowBlankControl "ShowBlankControl"
#define XkwCFixedOutputType "FixedOutputType"
#define XkwCAutoPopdown "AutoPopdown"
#define XkwCVerbose "Verbose"

#endif

/*  SimpleColourbarP.h

    Private header for  SimpleColourbar  widget class.

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
  SimpleColourbar widget.


    Written by      Richard Gooch   21-APR-1996

    Last updated by Richard Gooch   28-APR-1996

*/

#ifndef SimpleColourbarP__H
#define SimpleColourbarP__H

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <Xkw/SimpleColourbar.h>
#include <Xkw/CanvasP.h>

typedef struct _SimpleColourbarPart
{
    /*  Public resources  */
    Visual     *visual;
    Kcolourmap karmaCmap;
    Bool       maskRed;
    Bool       maskGreen;
    Bool       maskBlue;
    /*  Private resources  */
    KPixCanvas canvas;
    KPixCanvasImageCache row_cache;
    flag valid_cache;
    int cache_width;
} SimpleColourbarPart, *SimpleColourbarPartPtr;

typedef struct _SimpleColourbarRec
{
    CorePart core;
    CompositePart composite;
    SimpleColourbarPart simpleColourbar;
} SimpleColourbarRec, *SimpleColourbarPtr;

typedef struct _SimpleColourbarClassPart
{
    int empty;
} SimpleColourbarClassPart;

typedef struct _SimpleColourbarClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    SimpleColourbarClassPart simpleColourbar_class;
} SimpleColourbarClassRec, *SimpleColourbarClassPtr;

extern SimpleColourbarClassRec simpleColourbarClassRec;

#endif

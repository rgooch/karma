/*  SimpleSliderP.h

    Private header for  SimpleSlider  widget class.

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
  SimpleSlider widget.


    Written by      Richard Gooch   4-MAY-1996

    Last updated by Richard Gooch   6-MAY-1996

*/

#ifndef _SimpleSliderP_h
#define _SimpleSliderP_h

#include <X11/extensions/multibuf.h>
#include <X11/Xaw/SimpleP.h>
#include <Xkw/CanvasP.h>
#include <Xkw/SimpleSlider.h>

typedef struct
{
    int empty;
} SimpleSliderClassPart;

typedef struct _SimpleSliderClassRec {
    CoreClassPart	   core_class;
    SimpleClassPart        simple_class;
    CanvasClassPart        canvas_class;
    SimpleSliderClassPart  slider_class;
} SimpleSliderClassRec;

extern SimpleSliderClassRec simpleSliderClassRec;

typedef struct {
    /* resources */
    XtCallbackList  valueChangeCallback;
    String          label;
    int             minimum;
    int             maximum;
    Bool            wrap;
    int             value;
    int             *valuePtr;
    int             modifier;
    XtOrientation   layout;
    Bool            showRange;
    Bool            showValue;
    Bool            valueBesideLabel;
    Bool            callbackOnDrag;
    int             initialDelay;
    int             repeatDelay;
    int             minimumDelay;
    int             decay;
    /* private state */
    XtIntervalId    timer;
    flag            clicked_thumb;
    flag            hyperspace;
    int             last_x;
    int             last_y;
    int             last_event_code;
    flag            allow_timer;
    int             next_delay;
} SimpleSliderPart;

typedef struct _SimpleSliderRec {
    CorePart	core;
    SimplePart  simple;
    CanvasPart  canvas;
    SimpleSliderPart	slider;
} SimpleSliderRec;

#define SSW_DEF_DECAY 5                 /* milliseconds */
#define SSW_DEF_INITIAL_DELAY 200       /* milliseconds */
#define SSW_DEF_MINIMUM_DELAY 10        /* milliseconds */
#define SSW_DEF_REPEAT_DELAY 50         /* milliseconds */

#endif /* _SimpleSliderP_h */

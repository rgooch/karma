/*
 * Copyright 1992 John L. Cwikla
 * 
 * Permission to use, copy, modify, distribute, and sell this software
 * and its documentation for any purpose is hereby granted without fee,
 * provided that the above copyright notice appears in all copies and that
 * both that copyright notice and this permission notice appear in
 * supporting documentation, and that the name of John L. Cwikla or
 * University of Illinois not be used in advertising or publicity
 * pertaining to distribution of the software without specific, written
 * prior permission.  John L. Cwikla and University of Illinois make no
 * representations about the suitability of this software for any
 * purpose.  It is provided "as is" without express or implied warranty.
 *
 * John L. Cwikla and University of Illinois disclaim all warranties with
 * regard to this software, including all implied warranties of
 * merchantability and fitness, in no event shall John L. Cwikla or
 * University of Illinois be liable for any special, indirect or
 * consequential damages or any damages whatsoever resulting from loss of
 * use, data or profits, whether in an action of contract, negligence or
 * other tortious action, arising out of or in connection with the use or
 * performance of this software.
 *
 * Author:
 * 	John L. Cwikla
 * 	Materials Research Laboratory Center for Computation
 * 	University Of Illinois at Urbana-Champaign
 *	104 S. Goodwin
 * 	Urbana, IL 61801
 * 
 * 	cwikla@uimrl7.mrl.uiuc.edu
 */

/* Dial widget, based roughly on the dial widget by Douglas A. Young */

/* Default Translations:
 * <key>+: Increment(1)
 * Shift<key>+: Increment(100)
 * <key>-: Decrement(1)
 * Shift<key>-: Decrement(100)
 * <Btn1Down>: set()
 * <Btn1Motion>: set() drag()
*/

/* Resources:               Type:                Defaults:
 * XtNforeground          : pixel    : XtNDefaultForeground
 * XtNlabelForeground          : pixel    : XtNDefaultForeground
 * XtNminimum             : int      : 0
 * XtNmaximum             : int      : 65535
 * XtNvalue               : int      : 0
 * XtNfont                : XFontStruct   : XtNDefaultFont
 * XtNmargin              : int      : 5
 * XtNimcrementCallback   : callback : NULL
 * XtNdecrementCallbacki  : callback : NULL
 * XtNvalueChangeCallback : callback : NULL
*/


#ifndef HDIALP__H
#define HDIALP__H

#include <X11/IntrinsicP.h>
#include <X11/CoreP.h>
#include <Xkw/Hdial.h>

typedef struct _HdialPart
{
  Pixel foreground;
  Pixel labelForeground;
  XFontStruct *font;
  char label[11];	/* Label for number */
  int minimum; 		/* value */
  int maximum;		/* value */
  int value;
  int margin;
  XPoint center;
  XPoint top; 
  XPoint labelPos;
  Dimension labelHeight;
  Dimension labelWidth;
  Dimension lineWidth;
  double length;
  GC labelGC;
  GC gc;
  GC eraseGC;
  XtCallbackList incrementCallback;
  XtCallbackList decrementCallback;
  XtCallbackList valueChangeCallback;
} HdialPart, *HdialPartPtr;

typedef struct _HdialRec
{
  CorePart core;
  HdialPart hdial;
} HdialRec, *HdialPtr;

typedef struct _HdialClassPart
{
  int empty;
} HdialClassPart;

typedef struct _HdialClassRec
{
  CoreClassPart core_class;	
  HdialClassPart hdial_class;
} HdialClassRec, *HdialClassPtr;

extern HdialClassRec hdialClassRec;

#endif /* HDIALP__H */

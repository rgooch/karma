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

/* Basically a simple widget roughly based on the one by Doug Young,
 * except that the dial is a semi-circle, and the indicator has an
 * arrow. 
*/

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

#ifndef HDIAL__H
#define HDIAL__H

/*
** Hdial Widget
*/

extern WidgetClass hdialWidgetClass;
typedef struct _HdialClassRec *HdialWidgetClass;
typedef struct _HdialRec *HdialWidget;

#ifndef XtIsHdial
#define XtIsHdial(w) XtIsSubclass((w), hdialWidgetClass)
#endif

#define XtNlabelForeground "labelForeground"
#define XtNminimum "minimum"
#define XtNmaximum "maximum"
#define XtNincrementCallback "incrementCallback"
#define XtNdecrementCallback "decrementCallback"
#define XtNvalueChangeCallback "valueChangeCallback"
#define XtNmargin "margin"

#define XtCLabelForeground "LabelForeground"
#define XtCMinimum "Minimum"
#define XtCMaximum "Maximum"
#ifndef XtCMargin
#define XtCMargin "Margin"
#endif

/* Reasons */
#define HDIAL_INCREMENT 1
#define HDIAL_DECREMENT 2
#define HDIAL_SET 3

typedef struct _HdialCallbackStruct
{
  int reason;
  XEvent *event;
  int value;
} HdialCallbackStruct, *HdialCallbackPtr;

#endif /* HDIAL__H */

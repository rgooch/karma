/*  SimpleSlider.h

    Public header for  SimpleSlider  widget class.

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

    This include file contains the public class declarations for the
  SimpleSlider widget.


    Written by      Richard Gooch   4-MAY-1996

    Last updated by Richard Gooch   5-MAY-1996

*/

#ifndef _SimpleSlider_h
#define _SimpleSlider_h

#include <X11/Xmu/Converters.h>
#include <X11/Shell.h>
#include <karma_kwin.h>

/****************************************************************
 *
 * SimpleSlider widgets
 *
 ****************************************************************/

/* Resources:

 Name		           Class		RepType		Default Value
 ----		           -----		-------		-------------
 XkwNvalueChangeCallback   Callback             Pointer         NULL
 XtNlabel                  Label                String          NULL
 XkwNminimum               Minimum              Int             0
 XkwNmaximum               Maximum              Int             0
 XkwNwrap                  Wrap                 Bool            False
 XtNvalue                  Value                Int             0
 XkwNvaluePtr              ValuePtr             Pointer         NULL
 XkwNmodifier              Modifier             Int             0
 XtNorientation            Orientation                       XtorientHorizontal
 XkwNshowRange             ShowRange            Bool            False
 XkwNshowValue             ShowValue            Bool            True
 XkwNvalueBesideLabel      ValueBesideLabel     Bool            True
 XkwNcallbackOnDrag        CallbackOnDrag       Bool            True

*/

#define XkwNvalueChangeCallback "valueChangeCallback"
#define XkwNminimum "minimum"
#define XkwNmaximum "maximum"
#define XkwNwrap "wrap"
#define XkwNvaluePtr "valuePtr"
#define XkwNmodifier "modifier"
#define XkwNshowRange "showRange"
#define XkwNshowValue "showValue"
#define XkwNvalueBesideLabel "valueBesideLabel"
#define XkwNcallbackOnDrag "callbackOnDrag"
#define XtNdecay "decay"
#define XtNinitialDelay "initialDelay"
#define XtNminimumDelay "minimumDelay"
#define XtNrepeatDelay "repeatDelay"

#define XkwCMinimum "Minimum"
#define XkwCMaximum "Maximum"
#define XkwCWrap "Wrap"
#define XkwCValuePtr "ValuePtr"
#define XkwCModifier "Modifier"
#define XkwCShowRange "ShowRange"
#define XkwCShowValue "ShowValue"
#define XkwCValueBesideLabel "ValueBesideLabel"
#define XkwCCallbackOnDrag "CallbackOnDrag"
#define XtCDecay "Decay"
#define XtCDelay "Delay"
#define XtCMinimumDelay "MinimumDelay"

typedef struct _SimpleSliderClassRec	*SimpleSliderWidgetClass;
typedef struct _SimpleSliderRec	*SimpleSliderWidget;

extern WidgetClass simpleSliderWidgetClass;

#endif /* _SimpleSlider_h */

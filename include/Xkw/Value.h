
/*----------------------------------------------------------------------*/
/* This code provides a simple slider widget for Xt. */
/**/
/*
   Resources:               Type:                Defaults:
     XkwNvalueChangeCallback  Callback             NULL
     XtNlabel                 String               NULL
     XkwNminimum              Int                  0
     XkwNmaximum              Int                  0
     XtNvalue                 Int                  0
     XkwNmodifier             Int                  0
     XkwNwrap                 Boolean              False
     XtNorientation           Orientation          XtorientHorizontal

*/    
/*----------------------------------------------------------------------*/

#ifndef VALUE__H
#define VALUE__H

#include <X11/Xmu/Converters.h>

extern WidgetClass valueWidgetClass;
typedef struct _ValueClassRec *ValueWidgetClass;
typedef struct _ValueRec *ValueWidget;

#define XtIsValue(w) XtIsSubclass((w), valueWidgetClass)

#define XkwNminimum "minimum"
#define XkwNmaximum "maximum"
#define XkwNmodifier "modifier"
#define XkwNvalueChangeCallback "valueChangeCallback"
#define XkwNwrap "wrap"

#define XkwCMinimum "Minimum"
#define XkwCMaximum "Maximum"
#define XkwCModifier "Modifier"
#define XkwCValueChangeCallback "ValueChangeCallback"
#define XkwCWrap "Wrap"

#endif

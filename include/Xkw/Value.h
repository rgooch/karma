
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
     XkwNwrap                 Bool                 False
     XtNorientation           Orientation          XtorientHorizontal
     XkwNvaluePtr             Pointer              NULL

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
#define XkwNvaluePtr "valuePtr"

#define XkwCMinimum "Minimum"
#define XkwCMaximum "Maximum"
#define XkwCModifier "Modifier"
#define XkwCWrap "Wrap"
#define XkwCValuePtr "ValuePtr"

#endif


/*----------------------------------------------------------------------*/
/* This widget implements a plus/minus resize widget for picturebox     */
/*                                                                      */
/* Resources:               Type:                Defaults:              */
/*----------------------------------------------------------------------*/

#ifndef Increment__h
#define Increment__h

extern WidgetClass incrementWidgetClass;
typedef struct _IncrementClassRec *IncrementWidgetClass;
typedef struct _IncrementRec *IncrementWidget;

#define XtIsIncrement(w) XtIsSubclass((w), incrementWidgetClass)

#define XkwNlist "list"
#define XkwNindex "index"
#define XkwNvalueChangeCallback "valueChangeCallback"
#define XkwNlabel "label"

#define XkwCList "List"
#define XkwCIndex "Index"
#define XkwCLabel "Label"

#endif

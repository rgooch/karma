
/*----------------------------------------------------------------------*/
/* This code provides a simple exclusive menu widget for Xt.            */
/**/
/*
   Resources:               Type:                Defaults:
     XkwNchoiceName           String               NULL
     XkwNselectCallback       Callback             NULL
     XkwNitemStrings          Pointer              NULL
     XkwNnumItems             Int                  0

*/    
/*----------------------------------------------------------------------*/

#ifndef EXCLUSIVEMENU__H
#define EXCLUSIVEMENU__H

#include <X11/Xaw/MenuButton.h>

extern WidgetClass exclusiveMenuWidgetClass;
typedef struct _ExclusiveMenuClassRec * ExclusiveMenuWidgetClass;
typedef struct _ExclusiveMenuRec * ExclusiveMenuWidget;

#define XtIsExclusiveMenu(w) XtIsSubclass((w), exclusiveMenuWidgetClass)

#define XkwNchoiceName "choiceName"
#define XkwNitemStrings "itemStrings"
#define XkwNselectCallback "selectCallback"
#define XkwNnumItems "numItems"

#define XkwCChoiceName "ChoiceName"
#define XkwCItemStrings "ItemStrings"
#define XkwCNumItems "NumItems"

#endif

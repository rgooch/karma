
/*------------------------------------------------------------------------*/
/* This code provides a simple choice (non-exclusive) menu widget for Xt. */

/*
   Resources:               Type:                Defaults:
     XkwNmenuTitle            String               NULL
     XkwNselectCallback       Callback             NULL
     XkwNitemStrings          Pointer              NULL
     XkwNnumItems             Int                  0

*/    
/*------------------------------------------------------------------------*/

#ifndef CHOICEMENU__H
#define CHOICEMENU__H

#include <X11/Xaw/MenuButton.h>

extern WidgetClass choiceMenuWidgetClass;
typedef struct _ChoiceMenuClassRec * ChoiceMenuWidgetClass;
typedef struct _ChoiceMenuRec * ChoiceMenuWidget;

#define XtIsChoiceMenu(w) XtIsSubclass((w), choiceMenuWidgetClass)

#define XkwNmenuTitle "menuTitle"
#define XkwNitemStrings "itemStrings"
#define XkwNselectCallback "selectCallback"
#define XkwNnumItems "numItems"

#define XkwCMenuTitle "MenuTitle"
#define XkwCItemStrings "ItemStrings"
#define XkwCNumItems "NumItems"

#endif

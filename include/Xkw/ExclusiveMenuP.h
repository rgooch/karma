
#ifndef ExclusiveMenuP__H
#define ExclusiveMenuP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/MenuButtoP.h>
#include <X11/StringDefs.h>
#include <Xkw/ExclusiveMenu.h>

typedef struct
{
    /*  Public resources  */
    String choiceName;
    XtPointer itemStrings;
    int numItems;
    XtCallbackList selectCallback;
    /*  Private resources  */
    struct item_data_type *item_data;
} ExclusiveMenuPart, *ExclusiveMenuPartPtr;

typedef struct _ExclusiveMenuRec
{
    CorePart            core;
    SimplePart	        simple;
    LabelPart	        label;
    CommandPart	        command;
    MenuButtonPart      menuButton;
    ExclusiveMenuPart   exclusiveMenu;
} ExclusiveMenuRec, *ExclusiveMenuPtr;

typedef struct _ExclusiveMenuClassPart
{
  int empty;
} ExclusiveMenuClassPart;

typedef struct _ExclusiveMenuClassRec
{
    CoreClassPart	    core_class;
    SimpleClassPart	    simple_class;
    LabelClassPart	    label_class;
    CommandClassPart	    command_class;
    MenuButtonClassPart     menuButton_class;
    ExclusiveMenuClassPart  exclusiveMenu_class;
} ExclusiveMenuClassRec, *ExclusiveMenuClassPtr;

extern ExclusiveMenuClassRec exclusiveMenuClassRec;

#endif

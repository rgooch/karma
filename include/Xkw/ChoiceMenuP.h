
#ifndef ChoiceMenuP__H
#define ChoiceMenuP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/MenuButtoP.h>
#include <X11/StringDefs.h>
#include <Xkw/ChoiceMenu.h>

typedef struct
{
    /*  Public resources  */
    String menuTitle;
    XtPointer itemStrings;
    int numItems;
    XtCallbackList selectCallback;
    /*  Private resources  */
    struct item_data_type *item_data;
} ChoiceMenuPart, *ChoiceMenuPartPtr;

typedef struct _ChoiceMenuRec
{
    CorePart         core;
    SimplePart	     simple;
    LabelPart	     label;
    CommandPart	     command;
    MenuButtonPart   menuButton;
    ChoiceMenuPart   choiceMenu;
} ChoiceMenuRec, *ChoiceMenuPtr;

typedef struct _ChoiceMenuClassPart
{
  int empty;
} ChoiceMenuClassPart;

typedef struct _ChoiceMenuClassRec
{
    CoreClassPart	 core_class;
    SimpleClassPart	 simple_class;
    LabelClassPart	 label_class;
    CommandClassPart	 command_class;
    MenuButtonClassPart  menuButton_class;
    ChoiceMenuClassPart  choiceMenu_class;
} ChoiceMenuClassRec, *ChoiceMenuClassPtr;

extern ChoiceMenuClassRec choiceMenuClassRec;

#endif

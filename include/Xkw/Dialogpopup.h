
/*----------------------------------------------------------------------*/
/* This widget implements a file selector */
/*
 * Resources:               Type:                Defaults:
   XkwNdefaultName          String               "fred"
   XkwNextension            String               ".kf"
   XkwNshowAutoIncrement    ShowAutoIncrement    TRUE
 *----------------------------------------------------------------------*/

#ifndef DIALOGPOPUP__H
#define DIALOGPOPUP__H

extern WidgetClass dialogpopupWidgetClass;
typedef struct _DialogpopupClassRec *DialogpopupWidgetClass;
typedef struct _DialogpopupRec *DialogpopupWidget;

#define XtIsDialogpopup(w) XtIsSubclass((w), dialogpopupWidgetClass)

#define XkwNdefaultName "defaultName"
#define XkwNextension "extension"
#define XkwNshowAutoIncrement "showAutoIncrement"

#define XkwCDefaultName "DefaultName"
#define XkwCExtension "Extension"
#define XkwCShowAutoIncrement "ShowAutoIncrement"

/*----------------------------------------------------------------------
 * Functions
 *----------------------------------------------------------------------*/

#endif

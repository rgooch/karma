
/*----------------------------------------------------------------------*/
/* This widget implements a file selector */
/**/
/*
 Name		       Class		   RepType         Default Value
 ----		       -----		   -------         -------------
 autoPopdown           AutoPopdown         Bool            False
 fileSelectCallback    Callback            Callback        NULL
 filenameTester        Callback            Pointer         NULL
*/    
/*----------------------------------------------------------------------*/

#ifndef FILEPOPUP__H
#define FILEPOPUP__H

#include <X11/Shell.h>
#if !defined(KARMA_DIR_H) || defined(MAKEDEPEND)
#  include <karma_dir.h>
#endif

extern WidgetClass filepopupWidgetClass;
typedef struct _FilepopupClassRec *FilepopupWidgetClass;
typedef struct _FilepopupRec *FilepopupWidget;

#define XtIsFilepopup(w) XtIsSubclass((w), filepopupWidgetClass)

#define XkwNautoPopdown "autoPopdown"
#define XkwNfileSelectCallback "fileSelectCallback"
#define XkwNfilenameTester "filenameTester"

#define XkwCAutoPopdown "AutoPopdown"

/*----------------------------------------------------------------------*/
/* Functions
 *----------------------------------------------------------------------*/

void XkwFilepopupRescan(Widget w,XtPointer client_data,XtPointer call_data);

#endif

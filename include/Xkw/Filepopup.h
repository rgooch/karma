
/*----------------------------------------------------------------------*/
/* This widget implements a file selector */
/**/
/*
 Name		       Class		   RepType         Default Value
 ----		       -----		   -------         -------------
 autoPopdown           AutoPopdown         Boolean         False
*/    
/*----------------------------------------------------------------------*/

#ifndef FILEPOPUP__H
#define FILEPOPUP__H

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
/*----------------------------------------------------------------------*/

void XkwFilepopupRescan(Widget w,XtPointer client_data,XtPointer call_data);

#endif

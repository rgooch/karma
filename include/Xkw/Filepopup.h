
/*----------------------------------------------------------------------*/
/* This widget implements a file selector */
/**/
/* Resources:               Type:                Defaults:*/
/*----------------------------------------------------------------------*/

#ifndef FILEPOPUP__H
#define FILEPOPUP__H

extern WidgetClass filepopupWidgetClass;
typedef struct _FilepopupClassRec *FilepopupWidgetClass;
typedef struct _FilepopupRec *FilepopupWidget;

#define XtIsFilepopup(w) XtIsSubclass((w), filepopupWidgetClass)

#define XkwNfileSelectCallback "fileSelectCallback"
#define XkwNfilenameTester "filenameTester"

/*----------------------------------------------------------------------*/
/* Functions
/*----------------------------------------------------------------------*/

void XkwFilepopupRescan(Widget w,XtPointer client_data,XtPointer call_data);

#endif

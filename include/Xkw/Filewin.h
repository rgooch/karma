
/*----------------------------------------------------------------------*/
/* This widget implements a file selector */
/**/
/* Resources:               Type:                Defaults:*/
/*----------------------------------------------------------------------*/

#ifndef FILEWIN__H
#define FILEWIN__H

extern WidgetClass filewinWidgetClass;
typedef struct _FilewinClassRec *FilewinWidgetClass;
typedef struct _FilewinRec *FilewinWidget;

#define XtIsFilewin(w) XtIsSubclass((w), filewinWidgetClass)

#define XkwNfileSelectCallback "fileSelectCallback"
#define XkwNfilenameTester "filenameTester"

/*----------------------------------------------------------------------*/
/* Functions
/*----------------------------------------------------------------------*/

void XkwFilewinRescan(Widget w,XtPointer client_data,XtPointer call_data);
char *XkwFilewinCurrentDirectory(Widget W);

#endif

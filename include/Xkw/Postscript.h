
/*----------------------------------------------------------------------*/
/* This widget implements a page setup control */
/*
 Name		         Class		       RepType         Default Value
 ----		         -----		       -------         -------------
 portraitOrientation     PortraitOrientation   Bool            True
 pageHorizontalOffset    PageHorizontalOffset  Int             10
 pageVerticalOffset      PageVerticalOffset    Int             10
 pageHorizontalSize      PageHorizontalSize    Int             180
 pageVerticalSize        PageVerticalSize      Int             180
 XkwNautoIncrement       AutoIncrement         Bool            FALSE
*/
/*----------------------------------------------------------------------*/

#ifndef POSTSCRIPT__H
#define POSTSCRIPT__H

#include <karma_kwin.h>

extern WidgetClass postscriptWidgetClass;
typedef struct _PostscriptClassRec *PostscriptWidgetClass;
typedef struct _PostscriptRec *PostscriptWidget;

#define XkwNportraitOrientation "portraitOrientation"
#define XkwNpageHorizontalOffset "pageHorizontalOffset"
#define XkwNpageVerticalOffset "pageVerticalOffset"
#define XkwNpageHorizontalSize "pageHorizontalSize"
#define XkwNpageVerticalSize "pageVerticalSize"
#define XkwNautoIncrement "autoIncrement"

#define XkwCPortraitOrientation "PortraitOrientation"
#define XkwCPageHorizontalOffset "PageHorizontalOffset"
#define XkwCPageVerticalOffset "PageVerticalOffset"
#define XkwCPageHorizontalSize "PageHorizontalSize"
#define XkwCPageVerticalSize "PageVerticalSize"
#define XkwCAutoIncrement "AutoIncrement"

#define XtIsPostscript(w) XtIsSubclass((w), postscriptWidgetClass)

EXTERN_FUNCTION (void XkwPostscriptRegisterImageAndName,
		 (Widget w, KPixCanvas image, char *name) );

#endif

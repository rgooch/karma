
/*----------------------------------------------------------------------*/
/* This widget implements a page setup control */
/**/
/* Resources:               Type:                Defaults:*/
/*----------------------------------------------------------------------*/

#ifndef POSTSCRIPT__H
#define POSTSCRIPT__H

#include <karma_kwin.h>

extern WidgetClass postscriptWidgetClass;
typedef struct _PostscriptClassRec *PostscriptWidgetClass;
typedef struct _PostscriptRec *PostscriptWidget;

#define XtIsPostscript(w) XtIsSubclass((w), postscriptWidgetClass)

EXTERN_FUNCTION (void XkwPostscriptRegisterImageAndName,
		 (Widget w, KPixCanvas image, char *name) );

#endif

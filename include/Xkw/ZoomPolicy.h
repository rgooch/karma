
/*----------------------------------------------------------------------*/
/* This code provides an zoom policy control widget for Xt. */
/**/
/*
 Name		         Class		    RepType         Default Value
 ----		         -----		    -------         -------------
 XkwNcanvases            Canvases           Pointer         NULL
 XkwNautoIntensityScale  AutoIntensityScale Bool            True
*/    
/*----------------------------------------------------------------------*/

#ifndef ZOOMPOLICY__H
#define ZOOMPOLICY__H

#include <X11/Xmu/Converters.h>
#include <karma_canvas.h>

extern WidgetClass zoomPolicyWidgetClass;
typedef struct _ZoomPolicyClassRec *ZoomPolicyWidgetClass;
typedef struct _ZoomPolicyRec *ZoomPolicyWidget;

#define XtIsZoomPolicy(w) XtIsSubclass((w), zoomPolicyWidgetClass)

#define XkwNcanvases "canvases"
#define XkwNautoIntensityScale "autoIntensityScale"

#define XkwCCanvases "Canvases"
#define XkwCAutoIntensityScale "AutoIntensityScale"


#endif


/*----------------------------------------------------------------------*/
/* This code provides a multi canvas widget for Xt. */
/**/
/*
 Name		       Class		   RepType         Default Value
 ----		       -----		   -------         -------------
 canvasTypes           CanvasTypes         Int             0
*/    
/*----------------------------------------------------------------------*/

#ifndef MULTICANVAS__H
#define MULTICANVAS__H

#include <X11/Xmu/Converters.h>
#include <Xkw/Canvas.h>

#define XkwCanvasTypePseudoColour 0x01
#define XkwCanvasTypeDirectColour 0x02
#define XkwCanvasTypeTrueColour 0x04
#define XkwCanvasTypeStereo 0x08


extern WidgetClass multiCanvasWidgetClass;
typedef struct _MultiCanvasClassRec *MultiCanvasWidgetClass;
typedef struct _MultiCanvasRec *MultiCanvasWidget;

#define XtIsMultiCanvas(w) XtIsSubclass((w), multiCanvasWidgetClass)

#define XkwNcanvasTypes "canvasTypes"

#define XkwCCanvasTypes "CanvasTypes"

#endif


/*----------------------------------------------------------------------*/
/* This widget implements a slider control choosing an element from*/
/* a karma kcolourmap. The elements of the colourmap are displayed to*/
/* the left of or above (depending on orientation) the slider control.*/
/**/
/* Default Translations:*/
/* <Btn1Down>: set()*/
/* <Btn1Motion>: set() drag()*/
/**/
/* Resources:               Type:                Defaults:*/
/* XtNforeground          : pixel      : XtNDefaultForeground*/
/* XtNvalue               : float      : 0.0*/
/* XkNminimum             : float      : 0.0*/
/* XkNmaximum             : float      : 1.0*/
/* XkNvalueChangeCallback : callback   : NULL*/
/* XkNkarmaColourmap      : kcolourmap : NULL*/
/*----------------------------------------------------------------------*/

#ifndef PALETTE__H
#define PALETTE__H

#include <c_varieties.h>

extern WidgetClass paletteWidgetClass;
typedef struct _PaletteClassRec *PaletteWidgetClass;
typedef struct _PaletteRec *PaletteWidget;

#define XtIsPalette(w) XtIsSubclass((w), paletteWidgetClass)

#define XkwNvalueChangeCallback "valueChangeCallback"
#define XkwCValueChangeCallback "ValueChangeCallback"

typedef enum {
  XkwVertical = 1,
  XkwHorizontal
} XkwOrientation;

#define XkwNorientation	"orientation"
#define XkwCOrientation	"Orientation"
#define XkwROrientation	"XkwOrientation"
#define XkwEvertical	"vertical"
#define XkwEhorizontal	"horizontal"

#define XkwNminimum "minimum"
#define XkwNmaximum "maximum"

#define XkwCMinimum "Minimum"
#define XkwCMaximum "Maximum"


#define XkwNkarmaColourmap "karmaColourmap"
#define XkwCKarmaColourmap "KarmaColourmap"


EXTERN_FUNCTION (void CvtStringToOrient, (XrmValuePtr args, Cardinal *num_args,
					  XrmValuePtr fromVal,
					  XrmValuePtr toVal) );



typedef struct _PaletteCallbackStruct
{
  float value;
} PaletteCallbackStruct, *PaletteCallbackPtr;

#endif

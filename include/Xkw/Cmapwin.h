
/*----------------------------------------------------------------------
   This widget implements a PseudoColour colourmap control box

 Name		         Class                RepType         Default Value
 ----		         -----                -------         -------------
 colourbarVisual         Visual               Pointer         CopyFromParent
 karmaColourmap          KarmaColourmap       Pointer         NULL
 colourCallback          Callback             Callback        NULL
 regenerateColourmap     RegenerateColourmap  Bool            False
 simpleColourbar         SimpleColourbar      Bool            False
 XkwNdisableScaleSliders DisableScaleSliders  Bool            FALSE

------------------------------------------------------------------------*/

#ifndef CMAPWIN__H
#define CMAPWIN__H

#include <X11/Shell.h>
#include <karma_kcmap.h>

extern WidgetClass cmapwinWidgetClass;
typedef struct _CmapwinClassRec *CmapwinWidgetClass;
typedef struct _CmapwinRec *CmapwinWidget;

#define XtIsCmapwin(w) XtIsSubclass((w), cmapwinWidgetClass)

#define XkwNcolourbarVisual "colourbarVisual"
#define XkwNkarmaColourmap "karmaColourmap"
#define XkwNcolourCallback "colourCallback"
#define XkwNregenerateColourmap "regenerateColourmap"
#define XkwNsimpleColourbar "simpleColourbar"
#define XkwNdisableScaleSliders "disableScaleSliders"

#define XkwCKarmaColourmap "KarmaColourmap"
#define XkwCRegenerateColourmap "RegenerateColourmap"
#define XkwCSimpleColourbar "SimpleColourbar"
#define XkwCDisableScaleSliders "DisableScaleSliders"

void XkwCmapwinSetColourmap (Widget w, char *new_cmap_name);

#endif


/*----------------------------------------------------------------------
   This widget implements a colourmap control popup

 Name		         Class            RepType         Default Value
 ----		         -----            -------         -------------
 colourbarVisual         Visual           Pointer         CopyFromParent
 karmaColourmap          KarmaColourmap   Pointer         NULL
 simpleColourbar         SimpleColourbar  Bool            False

------------------------------------------------------------------------*/

#ifndef CMAPWINPOPUP__H
#define CMAPWINPOPUP__H

#include <Xkw/Cmapwin.h>

extern WidgetClass cmapwinpopupWidgetClass;
typedef struct _CmapwinpopupClassRec *CmapwinpopupWidgetClass;
typedef struct _CmapwinpopupRec *CmapwinpopupWidget;

#define XtIsCmapwinpopup(w) XtIsSubclass((w), cmapwinpopupWidgetClass)

#define XkwNcolourbarVisual "colourbarVisual"
#define XkwNkarmaColourmap "karmaColourmap"
#define XkwNsimpleColourbar "simpleColourbar"

#define XkwCKarmaColourmap "KarmaColourmap"
#define XkwCSimpleColourbar "SimpleColourbar"

#endif


/*----------------------------------------------------------------------
   This widget implements a colourmap control popup

 Name		         Class            RepType         Default Value
 ----		         -----            -------         -------------
 karmaColourmap          KarmaColourmap   Pointer         NULL

------------------------------------------------------------------------*/

#ifndef CMAPWINPOPUP__H
#define CMAPWINPOPUP__H

#include <Xkw/Cmapwin.h>

extern WidgetClass cmapwinpopupWidgetClass;
typedef struct _CmapwinpopupClassRec *CmapwinpopupWidgetClass;
typedef struct _CmapwinpopupRec *CmapwinpopupWidget;

#define XtIsCmapwinpopup(w) XtIsSubclass((w), cmapwinpopupWidgetClass)

#define XkwNkarmaColourmap "karmaColourmap"

#define XkwCKarmaColourmap "KarmaColourmap"

#endif

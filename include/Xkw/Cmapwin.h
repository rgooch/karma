
/*----------------------------------------------------------------------
   This widget implements a colourmap control box

 Name		         Class                RepType         Default Value
 ----		         -----                -------         -------------
 karmaColourmap          KarmaColourmap       Pointer         NULL
 colourCallback          Callback             Callback        NULL
 regenerateColourmap     RegenerateColourmap  Bool            False

------------------------------------------------------------------------*/

#ifndef CMAPWIN__H
#define CMAPWIN__H

#include <karma_kcmap.h>

extern WidgetClass cmapwinWidgetClass;
typedef struct _CmapwinClassRec *CmapwinWidgetClass;
typedef struct _CmapwinRec *CmapwinWidget;

#define XtIsCmapwin(w) XtIsSubclass((w), cmapwinWidgetClass)

#define XkwNkarmaColourmap "karmaColourmap"
#define XkwNcolourCallback "colourCallback"
#define XkwNregenerateColourmap "regenerateColourmap"

#define XkwCKarmaColourmap "KarmaColourmap"
#define XkwCRegenerateColourmap "RegenerateColourmap"

void XkwCmapwinSetColourmap(Widget w,char *new_cmap_name);

#endif


/*----------------------------------------------------------------------*/
/* This code provides a simple colourbar widget for Xt. */
/**/
/*
 Name		       Class		   RepType         Default Value
 ----		       -----		   -------         -------------
 visual                Visual              Pointer         CopyFromParent
 karmaColourmap        KarmaColourmap      Pointer         NULL
 maskRed               MaskColour          Bool            False
 maskGreen             MaskColour          Bool            False
 maskBlue              MaskColour          Bool            False
*/    
/*----------------------------------------------------------------------*/

#ifndef SIMPLECOLOURBAR__H
#define SIMPLECOLOURBAR__H

#include <karma_kcmap.h>

extern WidgetClass simpleColourbarWidgetClass;
typedef struct _SimpleColourbarClassRec *SimpleColourbarWidgetClass;
typedef struct _SimpleColourbarRec *SimpleColourbarWidget;

#define XtIsSimpleColourbar(w) XtIsSubclass((w), simpleColourbarWidgetClass)

#define XkwNkarmaColourmap "karmaColourmap"
#define XkwNmaskRed "maskRed"
#define XkwNmaskGreen "maskGreen"
#define XkwNmaskBlue "maskBlue"

#define XkwCKarmaColourmap "KarmaColourmap"
#define XkwCMaskColour "MaskColour"

#endif

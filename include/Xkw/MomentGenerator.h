
/*----------------------------------------------------------------------
   This widget implements a moment generator control

 Name		         Class            RepType         Default Value
 ----		         -----            -------         -------------
 XkwNverbose             Verbose          Bool            FALSE
 XkwNmom0Array           Array            Pointer         NULL
 XkwNmom1Array           Array            Pointer         NULL
 XkwNmomentCallback      Callback         Callback        NULL
------------------------------------------------------------------------*/

#ifndef MOMENTGENERATOR__H
#define MOMENTGENERATOR__H

#include <karma.h>
#include <karma_iarray.h>

extern WidgetClass momentGeneratorWidgetClass;
typedef struct _MomentGeneratorClassRec *MomentGeneratorWidgetClass;
typedef struct _MomentGeneratorRec *MomentGeneratorWidget;

#define XtIsMomentGenerator(w) XtIsSubclass((w), momentGeneratorWidgetClass)

#define XkwNverbose "verbose"
#define XkwNmom0Array "mom0Array"
#define XkwNmom1Array "mom1Array"
#define XkwNmomentCallback "momentCallback"

#define XkwCVerbose "Verbose"
#define XkwCArray "Array"

EXTERN_FUNCTION (void XkwMomentGeneratorNewArray,
		 (Widget W, iarray array, double min, double max) );

#endif

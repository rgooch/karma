
/*----------------------------------------------------------------------*/
/* This code provides a 3D slicer widget for Xt. */
/**/
/*
 Name		         Class		   RepType         Default Value
 ----		         -----		   -------         -------------
 iarray                  Iarray            Pointer         NULL
 karmaColourmap          KarmaColourmap    Pointer         NULL
 minPtr                  MinPtr            Pointer         NULL
 maxPtr                  MaxPtr            Pointer         NULL
 canvasVisual            Visual            Pointer         CopyFromParent
 verbose                 Verbose           Bool            False
 XkwNcursorCallback      Callback          Callback        NULL

*/    
/*----------------------------------------------------------------------*/

#ifndef THREEDEESLICE__H
#define THREEDEESLICE__H

#if !defined(KARMA_IARRAY_DEF_H) || defined(MAKEDEPEND)
#  include <karma_iarray_def.h>
#endif

#if !defined(KARMA_VRENDER_DEF_H) || defined(MAKEDEPEND)
#  include <karma_vrender_def.h>
#endif

#include <X11/Xmu/Converters.h>

extern WidgetClass threeDeeSliceWidgetClass;
typedef struct _ThreeDeeSliceClassRec *ThreeDeeSliceWidgetClass;
typedef struct _ThreeDeeSliceRec *ThreeDeeSliceWidget;

#define XtIsThreeDeeSlice(w) XtIsSubclass((w), threeDeeSliceWidgetClass)

#define XkwNiarray "iarray"
#define XkwNkarmaColourmap "karmaColourmap"
#define XkwNminPtr "minPtr"
#define XkwNmaxPtr "maxPtr"
#define XkwNcanvasVisual "canvasVisual"
#define XkwNverbose "verbose"
#define XkwNcursorCallback "cursorCallback"

#define XkwCIarray "Iarray"
#define XkwCKarmaColourmap "KarmaColourmap"
#define XkwCMinPtr "MinPtr"
#define XkwCMaxPtr "MaxPtr"
#define XkwCVerbose "Verbose"

EXTERN_FUNCTION (void XkwThreeDeeSlicePrecompute,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
#endif


#ifndef CMAPWINP__H
#define CMAPWINP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/FormP.h>
#include <X11/StringDefs.h>

#include <Xkw/Cmapwin.h>

typedef struct _CmapwinPart
{
    /*  Public resources  */
    Visual          *colourbarVisual;
    Kcolourmap      dcm;
    XtCallbackList  colourCallback;
    Bool            regenerateColourmap;
    Bool            simpleColourbar;
    Bool            disableScaleSliders;
    /*  Private resources  */
    Widget          selector;
    Widget          palette;
    Widget          twodthing;
    Widget          thecolour;
    CONST char      **list;
    double          cmap_x;
    double          cmap_y;
    int             listcount;
} CmapwinPart, *CmapwinPartPtr;

typedef struct _CmapwinRec
{
    CorePart core;
    CompositePart composite;
    ConstraintPart constraint;
    FormPart form;
    CmapwinPart cmapwin;
} CmapwinRec, *CmapwinPtr;

typedef struct _CmapwinClassPart
{
    int empty;
} CmapwinClassPart;

typedef struct _CmapwinClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ConstraintClassPart constraint_class;
    FormClassPart form_class;
    CmapwinClassPart cmapwin_class;
} CmapwinClassRec, *CmapwinClassPtr;

extern CmapwinClassRec cmapwinClassRec;

typedef struct {int empty;} CmapwinConstraintsPart;

typedef struct _CmapwinConstraintsRec {
    FormConstraintsPart	  form;
    CmapwinConstraintsPart Cmapwin;
} CmapwinConstraintsRec, *CmapwinConstraints;

#endif

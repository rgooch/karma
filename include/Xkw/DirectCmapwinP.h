
#ifndef DIRECTCMAPWINP__H
#define DIRECTCMAPWINP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/FormP.h>
#include <X11/StringDefs.h>

#include <Xkw/DirectCmapwin.h>

typedef struct _DirectCmapwinPart
{
    /*  Public resources  */
    Visual          *colourbarVisual;
    Kcolourmap      karmaCmap;
    XtCallbackList  colourCallback;
    Bool            regenerateColourmap;
    Bool            simpleColourbar;
    /*  Private resources  */
    Widget          selector;
    Widget          red_twodthing;
    Widget          green_twodthing;
    Widget          blue_twodthing;
    CONST char      **list;
    double          red_x;
    double          red_y;
    double          green_x;
    double          green_y;
    double          blue_x;
    double          blue_y;
    int             listcount;
} DirectCmapwinPart, *DirectCmapwinPartPtr;

typedef struct _DirectCmapwinRec
{
    CorePart core;
    CompositePart composite;
    ConstraintPart constraint;
    FormPart form;
    DirectCmapwinPart directCmapwin;
} DirectCmapwinRec, *DirectCmapwinPtr;

typedef struct _DirectCmapwinClassPart
{
    int empty;
} DirectCmapwinClassPart;

typedef struct _DirectCmapwinClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ConstraintClassPart constraint_class;
    FormClassPart form_class;
    DirectCmapwinClassPart directCmapwin_class;
} DirectCmapwinClassRec, *DirectCmapwinClassPtr;

extern DirectCmapwinClassRec directCmapwinClassRec;

typedef struct {int empty;} DirectCmapwinConstraintsPart;

typedef struct _DirectCmapwinConstraintsRec {
    FormConstraintsPart	  form;
    DirectCmapwinConstraintsPart DirectCmapwin;
} DirectCmapwinConstraintsRec, *DirectCmapwinConstraints;

#endif

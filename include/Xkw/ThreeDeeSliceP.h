
#ifndef ThreeDeeSliceP__H
#define ThreeDeeSliceP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/FormP.h>
#include <X11/StringDefs.h>

#include <Xkw/ThreeDeeSlice.h>
#include <karma_canvas.h>

typedef struct _ThreeDeeSlicePart
{
    /*  Public resources  */
    iarray         cube;
    Kcolourmap     karmaCmap;
    double         *minPtr;
    double         *maxPtr;
    Visual         *canvasVisual;
    Bool           verbose;
    XtCallbackList cursorCallback;
    Kcoord_3d      cursorPosition;
    /*  Private resources  */
    KPixCanvas    parent_pixcanvas;
    KPixCanvas    xy_pixcanvas;
    KPixCanvas    xz_pixcanvas;
    KPixCanvas    zy_pixcanvas;
    KPixCanvas    last_event_canvas;
    KWorldCanvas  xy_worldcanvas;
    KWorldCanvas  xz_worldcanvas;
    KWorldCanvas  zy_worldcanvas;
    ViewableImage *xy_frames;
    ViewableImage *xz_frames;
    ViewableImage *zy_frames;
    int           x_mag;
    int           y_mag;
    int           z_mag;
    KCallbackFunc iarr_destroy_func;
} ThreeDeeSlicePart, *ThreeDeeSlicePartPtr;

typedef struct _ThreeDeeSliceRec
{
    CorePart core;
    CompositePart composite;
    ConstraintPart constraint;
    FormPart form;
    ThreeDeeSlicePart threeDeeSlice;
} ThreeDeeSliceRec, *ThreeDeeSlicePtr;

typedef struct _ThreeDeeSliceClassPart
{
    int empty;
} ThreeDeeSliceClassPart;

typedef struct _ThreeDeeSliceClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ConstraintClassPart constraint_class;
    FormClassPart form_class;
    ThreeDeeSliceClassPart threeDeeSlice_class;
} ThreeDeeSliceClassRec, *ThreeDeeSliceClassPtr;

extern ThreeDeeSliceClassRec threeDeeSliceClassRec;

typedef struct {int empty;} ThreeDeeSliceConstraintsPart;

typedef struct _ThreeDeeSliceConstraintsRec
{
    FormConstraintsPart	  form;
    ThreeDeeSliceConstraintsPart ThreeDeeSlice;
} ThreeDeeSliceConstraintsRec, *ThreeDeeSliceConstraints;

#endif

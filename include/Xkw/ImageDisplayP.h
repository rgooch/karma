
#ifndef ImageDisplayP__H
#define ImageDisplayP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/FormP.h>
#include <X11/StringDefs.h>

#include <Xkw/ImageDisplay.h>

struct crosshair_type
{
    double x;
    double y;
};

struct dual_crosshair_type
{
    struct crosshair_type first;
    struct crosshair_type second;
    unsigned int num_crosshairs;
};

typedef struct _ImageDisplayPart
{
    /*  Public resources  */
    KWorldCanvas  pseudoCanvas;
    KWorldCanvas  directCanvas;
    KWorldCanvas  trueCanvas;
    KWorldCanvas  pseudoCanvasLeft;
    KWorldCanvas  pseudoCanvasRight;
    KWorldCanvas  directCanvasLeft;
    KWorldCanvas  directCanvasRight;
    KWorldCanvas  trueCanvasLeft;
    KWorldCanvas  trueCanvasRight;
    KWorldCanvas  visibleCanvas;
    String        imageName;
    Bool          enableAnimation;
    Bool          showAnimateButton;
    Bool          showQuitButton;
    Bool          fullscreen;
    int           cmapSize;
    Bool          verbose;
    /*  Private resources  */
    Widget filepopup;
    Widget trackLabel;
    Widget izoomwinpopup;
    Widget animatepopup;
    Widget pswinpopup;
    Widget multi_canvas;
    Widget override_shell;
    GC pseudo_main_gc;
    GC pseudo_crosshair_gc;
    GC direct_main_gc;
    GC direct_crosshair_gc;
    GC true_main_gc;
    GC true_crosshair_gc;
    Widget cmap_btn;
    struct dual_crosshair_type crosshairs;
    Kcolourmap pseudo_cmap;
} ImageDisplayPart, *ImageDisplayPartPtr;

typedef struct _ImageDisplayRec
{
    CorePart core;
    CompositePart composite;
    ConstraintPart constraint;
    FormPart form;
    ImageDisplayPart imageDisplay;
} ImageDisplayRec, *ImageDisplayPtr;

typedef struct _ImageDisplayClassPart
{
    int empty;
} ImageDisplayClassPart;

typedef struct _ImageDisplayClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ConstraintClassPart constraint_class;
    FormClassPart form_class;
    ImageDisplayClassPart imageDisplay_class;
} ImageDisplayClassRec, *ImageDisplayClassPtr;

extern ImageDisplayClassRec imageDisplayClassRec;

typedef struct {int empty;} ImageDisplayConstraintsPart;

typedef struct _ImageDisplayConstraintsRec
{
    FormConstraintsPart	  form;
    ImageDisplayConstraintsPart ImageDisplay;
} ImageDisplayConstraintsRec, *ImageDisplayConstraints;

#endif


#ifndef ImageDisplayP__H
#define ImageDisplayP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/FormP.h>
#include <X11/StringDefs.h>

#include <Xkw/ImageDisplay.h>

typedef struct _ImageDisplayPart
{
    /*  Public resources  */
    /*  The main window  */
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
    /*  The magnifier window  */
    KWorldCanvas  magnifierPseudoCanvas;
    KWorldCanvas  magnifierDirectCanvas;
    KWorldCanvas  magnifierTrueCanvas;
    KWorldCanvas  magnifierVisibleCanvas;
    /*  Other public resources  */
    String        imageName;
    Bool          enableAnimation;
    Bool          showAnimateButton;
    Bool          showQuitButton;
    Bool          fullscreen;
    int           cmapSize;
    Bool          autoIntensityScale;
    Bool          verbose;
    Cardinal      numTrackLabels;
    /*  Private resources  */
    Widget        filepopup;
    Widget        izoomwinpopup;
    Widget        animatepopup;
    Widget        pswinpopup;
    Widget        multi_canvas;
    Widget        override_shell;
    Widget        cmapwinpopup_psuedo;
    Widget        cmapwinpopup_direct;
    Widget        cmap_btn;
    Widget        zoom_policy_popup;
    flag          set_canvases;
    Kcolourmap    pseudo_cmap;
    Kcolourmap    direct_cmap;
    Widget        magnifier_pseudo_canvas;
    Widget        magnifier_direct_canvas;
    Widget        magnifier_true_canvas;
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

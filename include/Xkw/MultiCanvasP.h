
#ifndef MultiCanvasP__H
#define MultiCanvasP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/FormP.h>
#include <X11/StringDefs.h>

#include <Xkw/MultiCanvas.h>

typedef struct _MultiCanvasPart
{
    /*  Public resources  */
    int requestList;
    /*  Private resources  */
    Widget pseudoCanvas;
    Widget directCanvas;
    Widget trueCanvas;
    Widget pseudoCanvasLeft;
    Widget directCanvasLeft;
    Widget trueCanvasLeft;
    Widget pseudoCanvasRight;
    Widget directCanvasRight;
    Widget trueCanvasRight;
} MultiCanvasPart, *MultiCanvasPartPtr;

typedef struct _MultiCanvasRec
{
    CorePart core;
    CompositePart composite;
    ConstraintPart constraint;
    FormPart form;
    MultiCanvasPart multiCanvas;
} MultiCanvasRec, *MultiCanvasPtr;

typedef struct _MultiCanvasClassPart
{
    int empty;
} MultiCanvasClassPart;

typedef struct _MultiCanvasClassRec
{
    CoreClassPart core_class;	
    CompositeClassPart composite_class;
    ConstraintClassPart constraint_class;
    FormClassPart form_class;
    MultiCanvasClassPart multiCanvas_class;
} MultiCanvasClassRec, *MultiCanvasClassPtr;

extern MultiCanvasClassRec multiCanvasClassRec;

typedef struct {int empty;} MultiCanvasConstraintsPart;

typedef struct _MultiCanvasConstraintsRec {
    FormConstraintsPart	  form;
    MultiCanvasConstraintsPart MultiCanvas;
} MultiCanvasConstraintsRec, *MultiCanvasConstraints;

#endif

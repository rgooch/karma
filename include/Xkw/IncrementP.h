
#ifndef _IncrementP_h
#define _IncrementP_h

/*---------------------------------------------------------------------- */
/* Include Files */
/*---------------------------------------------------------------------- */

#include <X11/IntrinsicP.h>
#include <X11/Xaw/FormP.h>
#include <X11/StringDefs.h>
#include <Xkw/Increment.h>

/*---------------------------------------------------------------------- */
/* Class structures */
/*---------------------------------------------------------------------- */

typedef struct _IncrementClassPart{
  char  dummy;  /* some compilers dont like empty structures */
} IncrementClassPart;

typedef struct _IncrementClassRec {
  CoreClassPart core_class;
  CompositeClassPart composite_class;
  ConstraintClassPart constraint_class;
  FormClassPart form_class;
  IncrementClassPart increment_class;
} IncrementClassRec, *IncrementClassPtr;

extern IncrementClassRec incrementClassRec;

/*---------------------------------------------------------------------- */
/* Instance structures */
/*---------------------------------------------------------------------- */

typedef struct _IncrementPart
{
    /*  Public resources  */
    XtCallbackList valueChangeCallback;
    char **list;
    int index;
    char *label;
    /*  Private resources  */
    Widget valueWidget;
} IncrementPart;

typedef struct _IncrementRec {
  CorePart              core;
  CompositePart         composite;
  ConstraintPart        constraint;
  FormPart              form;
  IncrementPart         increment;
} IncrementRec;


/*---------------------------------------------------------------------- */
/* Constraints part  */
/*---------------------------------------------------------------------- */

typedef struct {int empty;} IncrementConstraintsPart;

typedef struct _IncrementConstraintsRec {
  FormConstraintsPart form;
  IncrementConstraintsPart increment;
} IncrementConstraintsRec, *IncrementConstraints;

/*---------------------------------------------------------------------- */
/* Nothing should come after the following endif */
/*---------------------------------------------------------------------- */

#endif

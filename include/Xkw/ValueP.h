
#ifndef ValueP__H
#define ValueP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/FormP.h>
#include <X11/StringDefs.h>

#include <Xkw/Value.h>

typedef struct _ValuePart
{
    /*  Public resources  */
    int value;
    int modifier;
    int minimum;
    int maximum;
    char *label;
    XtCallbackList valueChangeCallback;
    Bool wrap;
    int *valuePtr;
    XtOrientation layout;
    /*  Private resources  */
    Widget vallabel;
    Widget labelwidget;
} ValuePart, *ValuePartPtr;

typedef struct _ValueRec
{
  CorePart core;
  CompositePart composite;
  ConstraintPart constraint;
  FormPart form;
  ValuePart value;
} ValueRec, *ValuePtr;

typedef struct _ValueClassPart
{
  int empty;
} ValueClassPart;

typedef struct _ValueClassRec
{
  CoreClassPart core_class;	
  CompositeClassPart composite_class;
  ConstraintClassPart constraint_class;
  FormClassPart form_class;
  ValueClassPart value_class;
} ValueClassRec, *ValueClassPtr;

extern ValueClassRec valueClassRec;

typedef struct {int empty;} ValueConstraintsPart;

typedef struct _ValueConstraintsRec {
    FormConstraintsPart	  form;
    ValueConstraintsPart Value;
} ValueConstraintsRec, *ValueConstraints;

#endif

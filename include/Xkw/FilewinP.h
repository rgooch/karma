
#ifndef FILEWINP__H
#define FILEWINP__H

#include <X11/IntrinsicP.h>
#include <X11/Xaw/FormP.h>
#include <X11/StringDefs.h>

#include <karma_dir.h>

#include <Xkw/Filewin.h>

typedef struct _FilewinPart
{
    /*  Public resources  */
    XtCallbackList fileSelectCallback;
    flag (*accept_file) ();
    Pixel foreground;
    /*  Private resources  */
    Widget listwidget;
    char **list;
    char curdir[500];
    int listcount;
    int listmax;
    KCallbackList dir_callbacks;
} FilewinPart, *FilewinPartPtr;

typedef struct _FilewinRec
{
  CorePart core;
  CompositePart composite;
  ConstraintPart constraint;
  FormPart form;
  FilewinPart filewin;
} FilewinRec, *FilewinPtr;

typedef struct _FilewinClassPart
{
  int empty;
} FilewinClassPart;

typedef struct _FilewinClassRec
{
  CoreClassPart core_class;	
  CompositeClassPart composite_class;
  ConstraintClassPart constraint_class;
  FormClassPart form_class;
  FilewinClassPart filewin_class;
} FilewinClassRec, *FilewinClassPtr;

extern FilewinClassRec filewinClassRec;

typedef struct {int empty;} FilewinConstraintsPart;

typedef struct _FilewinConstraintsRec {
    FormConstraintsPart	  form;
    FilewinConstraintsPart filewin;
} FilewinConstraintsRec, *FilewinConstraints;

#endif

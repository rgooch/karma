/*LINTLIBRARY*/
/*  Increment.c

    This code provides an increment widget for Xt.

    Copyright (C) 1993-1996  Catherine Rastello
    Incorporated into Karma by permission.

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Catherine Rastello may be reached by email at  rastello@inf.enst.fr
    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    Patrick Jordan may be reached by email at  pjordan@rp.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains all routines needed for manipulating an increment
    widget for Xt.


    Written by      Catherine Rastello 12-OCT-1993: Submitted by Patrick Jordan

    Updated by      Richard Gooch      12-OCT-1993

    Updated by      Patrick Jordan     18-OCT-1993: Removed some print
  statements.

    Last updated by Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/

/*----------------------------------------------------------------------*/
/* Include Files*/
/*----------------------------------------------------------------------*/

#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <X11/Xos.h>

#include <stdio.h>

#include <Xkw/IncrementP.h>

/*----------------------------------------------------------------------*/
/* Function Declarations*/
/*----------------------------------------------------------------------*/

/* methods */

static void Initialise(Widget Req,Widget New,ArgList args,Cardinal *num_args);
static Boolean SetValues(IncrementWidget Current,IncrementWidget Request,
			 IncrementWidget New,ArgList args,Cardinal *nargs);
static void ConstraintInitialise(Widget request,Widget new);

/*----------------------------------------------------------------------*/
/* Resources*/
/*----------------------------------------------------------------------*/

#define offset(field) XtOffset(IncrementWidget, increment.field)

static XtResource resources[] = 
{
  {XkwNvalueChangeCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
   offset(valueChangeCallback), XtRCallback, (caddr_t)NULL},
  {XkwNlist, XkwCList, XtRPointer, sizeof(char **),
   offset(list), XtRString, NULL},
  {XkwNindex, XkwCIndex, XtRInt, sizeof(int),
   offset(index), XtRInt, NULL},
  {XkwNlabel, XkwCLabel, XtRString, sizeof(String),
   offset(label), XtRString, NULL},
};

#undef offset

/*---------------------------------------------------------------------- */
/* Class initialisation */
/*---------------------------------------------------------------------- */

IncrementClassRec incrementClassRec =
{
  {
        /* CoreClassPart */
    (WidgetClass) &formClassRec,  /* superclass */
    "Increment",                  /* class_name */
    sizeof(IncrementRec),         /* widget_size */
    NULL,                         /* class_initialize */
    NULL,                         /* class_part_initialize */
    False,                        /* class_inited */
    (XtInitProc)Initialise,       /* initialize */
    NULL,                         /* initialize_hook */
    XtInheritRealize,             /* realize */
    NULL,                         /* actions */
    0,                            /* num_actions */
    resources,                    /* resources */
    XtNumber(resources),          /* num_resources */
    NULLQUARK,                    /* xrm_class */
    True,                         /* compress_motion */
    True,                         /* compress_exposure */
    True,                         /* compress_enterleave */
    True,                         /* visible_interest */
    NULL,                         /* destroy */
    NULL,                         /* resize */
    NULL,                         /* expose */
    (XtSetValuesFunc)SetValues,   /* set_values */
    NULL,                         /* set_values_hook */
    XtInheritSetValuesAlmost,     /* set_values_almost */
    NULL,                         /* get_values_hook */
    NULL,                         /* accept_focus */
    XtVersion,                    /* version */
    NULL,                         /* callback_private */
    NULL,                         /* tm_table */
    NULL,                         /* query_geometry */
    NULL,                         /* display_accelerator */
    NULL                          /* extension */
  },
  {  
    /* CompositeClassPart */
    XtInheritGeometryManager,     /* geometry manager */
    XtInheritChangeManaged,       /* change_manager */
    XtInheritInsertChild,         /* insert_child */
    XtInheritDeleteChild,         /* delete_child */
    NULL                          /* extension */
  },
  {
    /* Constraint */
    NULL,                         /* subresources */
    0,                            /* subresources_count */
    sizeof(IncrementConstraintsRec), /* constraint_size */
    (XtInitProc)ConstraintInitialise, /* initialise */
    NULL,                         /* destroy */
    NULL,                         /* set_values */
    NULL                          /* extension */
  },
  {
    /* FormClassPart */
    XtInheritLayout               /* layout */
  },
  {
    /* IncrementClassPart */
    0    /* empty */
  }
};

WidgetClass incrementWidgetClass = (WidgetClass) &incrementClassRec;


/*---------------------------------------------------------------------- */
/* Change value callback */
/*---------------------------------------------------------------------- */

static void valueChange_cbk
(Widget w,XtPointer client_data,XtPointer call_data)
{
  IncrementWidget iw = (IncrementWidget)client_data;
  int index = iw->increment.index;
  int nbind;

  sscanf(iw->increment.list[0],"%d",&nbind);

  if (strcmp(XtName(w),"plus")==0 && index<nbind)  {
    index = ++iw->increment.index;
    iw->increment.label = iw->increment.list[index];
    XtVaSetValues(iw->increment.valueWidget,
		  XtNlabel,iw->increment.label,
		  NULL);
    XtCallCallbacks((Widget)iw,XkwNvalueChangeCallback,&index);
  }
  else if (strcmp(XtName(w),"minus")==0 && index>1)  {
    index = --iw->increment.index;
    iw->increment.label = iw->increment.list[index];
    XtVaSetValues(iw->increment.valueWidget,
		  XtNlabel,iw->increment.label,
		  NULL);
    XtCallCallbacks((Widget)iw,XkwNvalueChangeCallback,&index);
  }
}


/*---------------------------------------------------------------------- */
/* Constraint Initialise method */
/*---------------------------------------------------------------------- */

static void ConstraintInitialise(Widget req, Widget new)
{
/*nothing to be done here really, but apparently needed  */
}


/*---------------------------------------------------------------------- */
/* Initialise method */
/*---------------------------------------------------------------------- */

static void Initialise(Widget Req,Widget New,ArgList args,Cardinal *num_args)
{
  IncrementWidget iw = (IncrementWidget) New;
  Widget minus,plus;

  iw->core.height=50;
  iw->core.width=75;

  minus = XtVaCreateManagedWidget("minus",
				  commandWidgetClass,New,
				  XtNlabel,"-",
				  XtNwidth,30,
				  XtNheight,15,
				  NULL);
  XtAddCallback(minus,XtNcallback,valueChange_cbk,New);

  plus = XtVaCreateManagedWidget("plus",
				 commandWidgetClass,New,
				 XtNlabel,"+",
				 XtNwidth,30,
				 XtNheight,15,
				 XtNfromHoriz,minus,
				 XtNhorizDistance,5,
				 NULL);

  iw->increment.valueWidget = 
    XtVaCreateManagedWidget("value",
			    labelWidgetClass,New,
			    XtNfromVert,minus,
			    XtNvertDistance,5,
			    XtNlabel,iw->increment.label,
			    NULL);

  XtAddCallback(plus,XtNcallback,valueChange_cbk,New);
}


/*---------------------------------------------------------------------- */
/* SetValues method */
/*---------------------------------------------------------------------- */

static Boolean SetValues(IncrementWidget Current,IncrementWidget Request,
			 IncrementWidget New,ArgList args,Cardinal *nargs)
{
    IncrementWidget current = (IncrementWidget) Current;
    IncrementWidget new = (IncrementWidget) New;

    if (current->increment.label != new->increment.label)
	XtVaSetValues(new->increment.valueWidget,
		      XtNlabel,new->increment.label,
		      NULL);
    return (FALSE);
}


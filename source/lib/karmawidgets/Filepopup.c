/*LINTLIBRARY*/
/*  Filepopup.c

    This code provides a popup file selector widget with close and
    rescan buttons, for Xt.

    Copyright (C) 1993-1996  Patrick Jordan
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

    Patrick Jordan may be reached by email at  pjordan@rp.csiro.au
    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains all routines needed for manipulating a popup file
    selector widget with close and rescan buttons, for Xt.


    Written by      Partick Jordan  9-JUL-1993

    Updated by      Richard Gooch   29-SEP-1993

    Updated by      Richard Gooch   8-DEC-1993: Added more patches from Patrick

    Updated by      Richard Gooch   17-JAN-1994: Added more patches from
  Patrick

    Updated by      Richard Gooch   23-OCT-1994: Changed some resource names.

    Updated by      Richard Gooch   14-NOV-1994: Changed some resource names.

    Updated by      Richard Gooch   18-NOV-1994: Added XkwNautoPopdown resource

    Updated by      Richard Gooch   24-DEC-1994: Propagated change of filename
  tester resource.

    Updated by      Richard Gooch   17-APR-1995: Fixed mix of XtRBool and
  sizeof Boolean in resource fields. Use Bool because it is of int size.

    Updated by      Richard Gooch   25-FEB-1996: Made default position top-left
  corner.

    Last updated by Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#include <Xkw/FilepopupP.h>

#include <Xkw/Filewin.h>

#include <X11/Xos.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Command.h>    
#include <X11/Xaw/List.h>
#include <X11/Xaw/Viewport.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <varargs.h>

#include <karma_dir.h>
#include <karma_st.h>

/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

/* Methods*/

static void Initialize (Widget request, Widget new);
static void Destroy (Widget w);
static Boolean SetValues (Widget current, Widget request, Widget new);

/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define TheOffset(field) XtOffset(FilepopupWidget, filepopup.field)

static XtResource FilepopupResources[] = 
{
  {XkwNautoPopdown, XkwCAutoPopdown, XtRBool, sizeof (Bool),
   TheOffset (autoPopdown), XtRImmediate, False},
  {XkwNfileSelectCallback, XtCCallback, XtRCallback, sizeof (caddr_t),
   TheOffset (fileSelectCallback), XtRCallback, (caddr_t) NULL},
  {XkwNfilenameTester, XtCCallback, XtRPointer, sizeof (caddr_t),
   TheOffset (accept_file), XtRPointer, (caddr_t) NULL},
  {XtNx, XtCPosition, XtRPosition, sizeof (Position),
   XtOffset (FilepopupWidget, core.x), XtRPosition, 0},
  {XtNy, XtCPosition, XtRPosition, sizeof (Position),
   XtOffset (FilepopupWidget, core.y), XtRPosition, 0},
};

#undef TheOffset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

FilepopupClassRec filepopupClassRec = 
{
  {        /* CoreClassPart */
    (WidgetClass)&topLevelShellClassRec,  /* superclass */
    "Filepopup",                     /* class_name */
    sizeof(FilepopupRec),            /* widget_size */
    NULL,                          /* class_initialize */
    NULL,                          /* class_part_initialize */
    FALSE,                         /* class_init */
    (XtInitProc)Initialize,        /* initialize */
    NULL,                          /* initialize_hook */
    XtInheritRealize,              /* realize */
    NULL,                          /* actions */
    0,                             /* num_actions */
    FilepopupResources,              /* resources */
    XtNumber(FilepopupResources),    /* num_resources */
    NULLQUARK,                     /* xrm_class */
    TRUE,                          /* compress_motion */
    TRUE,                          /* compress_exposure */
    TRUE,                          /* compress_enterleave */
    TRUE,                          /* visible_interest */
    Destroy,                       /* destroy */
    NULL,                          /* resize */
    NULL,                          /* expose */
    (XtSetValuesFunc)SetValues,    /* set_values */
    NULL,                          /* set_values_hook */
    XtInheritSetValuesAlmost,      /* set_values_almost */
    NULL,                          /* get_values_hook */
    NULL,                          /* accept_focus */
    XtVersion,                     /* version */
    NULL,                          /* callback_private */
    NULL,                          /* tm_translations */
    NULL,                          /* query_geometry  */
    NULL,
    NULL,
  },
  {     /* CompositeClassPart */
    /* geometry_manager	  */	XtInheritGeometryManager,
    /* change_managed	  */	XtInheritChangeManaged,
    /* insert_child	  */	XtInheritInsertChild,
    /* delete_child	  */	XtInheritDeleteChild,
    /* extension	  */	NULL
  },{ /* Shell */
    /* extension	  */	NULL
  },{ /* WMShell */
    /* extension	  */	NULL
  },{ /* VendorShell */
    /* extension	  */	NULL
  },{ /* TopLevelShell */
    /* extension	  */	NULL
  },
  {  /* FilepopupClassPart */
    0 /* empty */
  }
};

WidgetClass filepopupWidgetClass = (WidgetClass) &filepopupClassRec;

/*----------------------------------------------------------------------*/
/* Callback for file selection*/
/* This just passes the info on to the callback for the popup widget */
/*----------------------------------------------------------------------*/

static void filesel_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    Widget parent = (Widget) client_data;
    FilepopupWidget fw = (FilepopupWidget) parent;

    if ( (*fw).filepopup.autoPopdown )
    {
	XtPopdown (parent);
	XSync (XtDisplay (parent), False);
    }
    XtCallCallbacks ( (Widget) fw, XkwNfileSelectCallback, call_data);
}

/*----------------------------------------------------------------------*/
/* Callback for close button*/
/*----------------------------------------------------------------------*/

static void close_cbk(Widget w,XtPointer client_data,XtPointer call_data)
{
  FilepopupWidget fw = (FilepopupWidget)client_data;
  XtPopdown((Widget)fw);
}

/*----------------------------------------------------------------------*/
/* Initialisation method*/
/*----------------------------------------------------------------------*/

static void Initialize(Widget Request,Widget New)
{
    /*FilepopupWidget request = (FilepopupWidget) Request;*/
    FilepopupWidget new = (FilepopupWidget) New;
    Widget closebtn,form,rescanbtn;

  form=XtVaCreateManagedWidget
    ("form",formWidgetClass,(Widget)new,
     NULL);

  new->filepopup.selector=XtVaCreateManagedWidget
    ("selector",filewinWidgetClass,form,
     XkwNfilenameTester,new->filepopup.accept_file,
     XtNdefaultDistance,0,
     XtNhorizDistance,5,
     XtNvertDistance,5,
     NULL);
  XtAddCallback(new->filepopup.selector,XkwNfileSelectCallback,filesel_cbk,new);

  closebtn=XtVaCreateManagedWidget
    ("closeButton",commandWidgetClass,form,
     XtNlabel,"close",
     XtNhorizDistance,5,
     XtNvertDistance,5,
     XtNfromVert,new->filepopup.selector,
/*
     XtNwidth,52,
*/
     XtNheight,48,
     NULL);
  XtAddCallback(closebtn,XtNcallback,close_cbk,new);

  rescanbtn=XtVaCreateManagedWidget
    ("rescanButton",commandWidgetClass,form,
     XtNlabel,"rescan",
     XtNhorizDistance,5,
     XtNvertDistance,5,
     XtNfromVert,new->filepopup.selector,
     XtNfromHoriz,closebtn,
  /*
   XtNwidth,52,
*/
     XtNheight,48,
     NULL);
  XtAddCallback(rescanbtn,XtNcallback,XkwFilewinRescan,new->filepopup.selector);
  XtAddCallback(New,XtNpopupCallback,XkwFilewinRescan,new->filepopup.selector);
}

/*----------------------------------------------------------------------*/
/* Destroy method*/
/*----------------------------------------------------------------------*/

static void Destroy(Widget W)
{
  XtRemoveAllCallbacks(W, XkwNfileSelectCallback);
}

/*----------------------------------------------------------------------*/
/* SetVaues method*/
/*----------------------------------------------------------------------*/

static Boolean SetValues(Widget Current,Widget Request,Widget New)
{
    FilepopupWidget current = (FilepopupWidget) Current;
    /*FilepopupWidget request = (FilepopupWidget) Request;*/
    FilepopupWidget new = (FilepopupWidget) New;

    if (new->filepopup.accept_file != current->filepopup.accept_file)
    {
	XtVaSetValues (new->filepopup.selector,
		       XkwNfilenameTester, new->filepopup.accept_file,
		       NULL);
    }
    return False;
}

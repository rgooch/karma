/*LINTLIBRARY*/
/*  Dialogpopup.c

    This code provides a popup dialog widget, for Xt.

    Copyright (C) 1993  Patrick Jordan  Incorporated into Karma by permission.

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

/*  This file contains all routines needed for manipulating a popup
    dialog widget with cancel and ok buttons, for Xt.


    Written by      Patrick Jordan  19-AUG-1994

    Updated by      Patrick Jordan  19-AUG-1994

    Last updated by Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#include <Xkw/DialogpopupP.h>

#include <X11/Xos.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Command.h>    
#include <X11/Xaw/Dialog.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <varargs.h>

/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

/* Methods*/

static void Initialize(Widget request,Widget new);
static void Destroy(Widget w);
static Boolean SetValues(Widget current,Widget request,Widget new);

/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define TheOffset(field) XtOffset(DialogpopupWidget, dialogpopup.field)

static XtResource DialogpopupResources[] = 
{
  {XtNcallback, XtCCallback, XtRCallback, sizeof(caddr_t),
   TheOffset(callback), XtRCallback, (caddr_t)NULL},
  {XtNlabel, XtCLabel, XtRString, sizeof(char *),
   TheOffset(label), XtRString, (char *)NULL},
};

#undef TheOffset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

DialogpopupClassRec dialogpopupClassRec = 
{
  {        /* CoreClassPart */
    (WidgetClass)&topLevelShellClassRec,  /* superclass */
    "Dialogpopup",                     /* class_name */
    sizeof(DialogpopupRec),            /* widget_size */
    NULL,                          /* class_initialize */
    NULL,                          /* class_part_initialize */
    FALSE,                         /* class_init */
    (XtInitProc)Initialize,        /* initialize */
    NULL,                          /* initialize_hook */
    XtInheritRealize,              /* realize */
    NULL,                          /* actions */
    0,                             /* num_actions */
    DialogpopupResources,              /* resources */
    XtNumber(DialogpopupResources),    /* num_resources */
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
    NULL,
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
  {  /* DialogpopupClassPart */
    0 /* empty */
  }
};

WidgetClass dialogpopupWidgetClass = (WidgetClass) &dialogpopupClassRec;

/*----------------------------------------------------------------------*/
/* Callback for ok button*/
/*----------------------------------------------------------------------*/

static void ok_cbk(Widget w,XtPointer client_data,XtPointer call_data)
{
  DialogpopupWidget dw = (DialogpopupWidget)client_data;
  char ans[100];

  sprintf(ans,"%s",XawDialogGetValueString(dw->dialogpopup.dialog));

  XtCallCallbacks((Widget)dw,XtNcallback,ans);
  XtPopdown((Widget)dw);
  XtVaSetValues(dw->dialogpopup.dialog,XtNvalue,"",NULL);
}

/*----------------------------------------------------------------------*/
/* Callback for cancel button*/
/*----------------------------------------------------------------------*/

static void cancel_cbk(Widget w,XtPointer client_data,XtPointer call_data)
{
  DialogpopupWidget dw = (DialogpopupWidget)client_data;
  XtPopdown((Widget)dw);
}

/*----------------------------------------------------------------------*/
/* Initialisation method*/
/*----------------------------------------------------------------------*/

static void Initialize(Widget Request,Widget New)
{
    /*DialogpopupWidget request = (DialogpopupWidget) Request;*/
    DialogpopupWidget new = (DialogpopupWidget) New;

    new->dialogpopup.dialog = XtVaCreateManagedWidget
	("dialog", dialogWidgetClass, (Widget)new,
	 XtNlabel, new->dialogpopup.label,
	 XtNvalue, "",
	 NULL);

    XawDialogAddButton (new->dialogpopup.dialog, "cancel", cancel_cbk, new);
    XawDialogAddButton (new->dialogpopup.dialog, "ok", ok_cbk, new);
}

/*----------------------------------------------------------------------*/
/* Destroy method*/
/*----------------------------------------------------------------------*/

static void Destroy(Widget W)
{
  XtRemoveAllCallbacks(W, XtNcallback);
}

/*----------------------------------------------------------------------*/
/* SetVaues method*/
/*----------------------------------------------------------------------*/

static Boolean SetValues(Widget Current,Widget Request,Widget New)
{
    /*DialogpopupWidget current = (DialogpopupWidget) Current;
      DialogpopupWidget request = (DialogpopupWidget) Request;*/
    DialogpopupWidget new = (DialogpopupWidget) New;

    XtVaSetValues (new->dialogpopup.dialog,
		   XtNlabel, new->dialogpopup.label,
		   NULL);
  return True;
}


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

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   15-AUG-1996: Added overwrite toggle.

    Updated by      Richard Gooch   23-SEP-1996: Added XkwNshowAutoIncrement
  resource.

    Last updated by Richard Gooch   30-OCT-1996: Added #include <sys/types.h>


*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <errno.h>
#include <varargs.h>
#include <X11/Xos.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>    
#include <X11/Xaw/Dialog.h>
#ifndef X11
#  define X11
#endif
#include <karma.h>
#include <Xkw/DialogpopupP.h>
#include <Xkw/Ktoggle.h>


STATIC_FUNCTION (void Dialogpopup__Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (void Dialogpopup__Destroy, (Widget w) );
STATIC_FUNCTION (Boolean Dialogpopup__SetValues,
		 (Widget current, Widget request,Widget new) );
STATIC_FUNCTION (void auto_increment_cbk, (Widget w, XtPointer client_data,
					   XtPointer call_data) );

/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define TheOffset(field) XtOffset(DialogpopupWidget, dialogpopup.field)

static XtResource DialogpopupResources[] = 
{
    {XtNcallback, XtCCallback, XtRCallback, sizeof (caddr_t),
     TheOffset (callback), XtRCallback, (caddr_t) NULL},
    {XtNlabel, XtCLabel, XtRString, sizeof (String),
     TheOffset (label), XtRString, (char *) NULL},
    {XkwNdefaultName, XkwCDefaultName, XtRString, sizeof (String),
     TheOffset (defaultName), XtRString, "fred"},
    {XkwNextension, XkwCExtension, XtRString, sizeof (String),
     TheOffset (extension), XtRString, ".kf"},
    {XkwNshowAutoIncrement, XkwCShowAutoIncrement, XtRBool, sizeof (Bool),
     TheOffset (showAutoIncrement), XtRImmediate, (XtPointer) TRUE},
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
    NULL,                          /* class_initialise */
    NULL,                          /* class_part_initialise */
    FALSE,                         /* class_init */
    (XtInitProc)Dialogpopup__Initialise,        /* initialise */
    NULL,                          /* initialise_hook */
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
    Dialogpopup__Destroy,          /* destroy */
    NULL,                          /* resize */
    NULL,                          /* expose */
    (XtSetValuesFunc)Dialogpopup__SetValues,    /* set_values */
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

static void ok_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    flag check = TRUE;
    unsigned int sequence_number = 0;
    struct stat statbuf;
    DialogpopupWidget dw = (DialogpopupWidget) client_data;
    char filename[STRING_LENGTH];
    char basefile[STRING_LENGTH];
    String dialog_name, extension;

    XtVaGetValues (dw->dialogpopup.dialog,
		   XtNvalue, &dialog_name,
		   NULL);
    if ( (dialog_name == NULL) || (*dialog_name == '\n') )
    {
	(void) strcpy (basefile, "fred");
    }
    else (void) strcpy (basefile, dialog_name);
    extension = dw->dialogpopup.extension;
    while (check)
    {
	if ( dw->dialogpopup.auto_increment && (sequence_number > 0) )
	{
	    sprintf (filename, "%s.%u%s",
		     basefile, sequence_number, extension);
	}
	else
	{
	    sprintf (filename, "%s%s", basefile, extension);
	}
	if (dw->dialogpopup.auto_increment)
	{
	    ++sequence_number;
	    if (stat (filename, &statbuf) != 0) check = FALSE;
	}
	else check = FALSE;
    }
    XtCallCallbacks ( (Widget) dw, XtNcallback, filename);
    XtPopdown ( (Widget) dw );
}   /* End Function ok_cbk  */

/*----------------------------------------------------------------------*/
/* Callback for cancel button*/
/*----------------------------------------------------------------------*/

static void cancel_cbk (Widget w,XtPointer client_data,XtPointer call_data)
{
  DialogpopupWidget dw = (DialogpopupWidget)client_data;
  XtPopdown((Widget)dw);
}

/*----------------------------------------------------------------------*/
/* Initialisation method*/
/*----------------------------------------------------------------------*/

static void Dialogpopup__Initialise (Widget Request, Widget New)
{
    /*DialogpopupWidget request = (DialogpopupWidget) Request;*/
    DialogpopupWidget new = (DialogpopupWidget) New;
    Widget form;
    Widget w = NULL;  /*  Initialised to keep compiler happy  */
    char txt[STRING_LENGTH];

    new->dialogpopup.auto_increment = TRUE;
    form = XtVaCreateManagedWidget ("form", formWidgetClass, New,
				    XtNborderWidth, 0,
				    NULL);
    if (new->dialogpopup.showAutoIncrement)
    {
	w = XtVaCreateManagedWidget ("overwriteToggle", ktoggleWidgetClass,
				     form,
				     XtNlabel, "Auto Increment",
				     XtNstate, new->dialogpopup.auto_increment,
				     XkwNcrosses, False,
				     NULL);
	XtAddCallback (w, XtNcallback, auto_increment_cbk, New);
	sprintf (txt, "Extension: %s", new->dialogpopup.extension);
	w = XtVaCreateManagedWidget ("extensionLabel", labelWidgetClass, form,
				     XtNborderWidth, 0,
				     XtNhorizDistance, 0,
				     /*XtNvertDistance, 0,*/
				     XtNlabel, txt,
				     XtNfromVert, w,
				     NULL);
	new->dialogpopup.extension_label = w;
    }
    w = XtVaCreateManagedWidget ("dialog", dialogWidgetClass, form,
				 XtNborderWidth, 0,
				 XtNhorizDistance, 0,
				 XtNvertDistance, 0,
				 XtNlabel, new->dialogpopup.label,
				 XtNvalue, new->dialogpopup.defaultName,
				 /*  Below is a devious cheat: don't put
				     anything below it other than NULL  */
				 new->dialogpopup.showAutoIncrement ?
				 XtNfromVert : NULL, w,
				 NULL);
    new->dialogpopup.dialog = w;
    XawDialogAddButton (new->dialogpopup.dialog, "cancel", cancel_cbk, new);
    XawDialogAddButton (new->dialogpopup.dialog, "ok", ok_cbk, new);
}   /*  End Function Initialise  */

/*----------------------------------------------------------------------*/
/* Destroy method*/
/*----------------------------------------------------------------------*/

static void Dialogpopup__Destroy (Widget W)
{
    XtRemoveAllCallbacks (W, XtNcallback);
}   /*  End Function Destroy  */

/*----------------------------------------------------------------------*/
/* SetVaues method*/
/*----------------------------------------------------------------------*/

static Boolean Dialogpopup__SetValues (Widget Current, Widget Request,
				       Widget New)
{
    /*DialogpopupWidget current = (DialogpopupWidget) Current;
      DialogpopupWidget request = (DialogpopupWidget) Request;*/
    DialogpopupWidget new = (DialogpopupWidget) New;
    char txt[STRING_LENGTH];

    XtVaSetValues (new->dialogpopup.dialog,
		   XtNlabel, new->dialogpopup.label,
		   NULL);
    XtVaSetValues (new->dialogpopup.dialog,
		   XtNvalue, new->dialogpopup.defaultName,
		   NULL);
    sprintf (txt, "Extension: %s", new->dialogpopup.extension);
    XtVaSetValues (new->dialogpopup.extension_label,
		   XtNlabel, txt,
		   NULL);
    return FALSE;
}   /*  End Function SetValues  */

static void auto_increment_cbk (Widget w, XtPointer client_data,
				XtPointer call_data)
{
    flag bool = (uaddr) call_data;
    DialogpopupWidget top = (DialogpopupWidget) client_data;

    top->dialogpopup.auto_increment = bool;
}   /*  End Function auto_increment_cbk  */

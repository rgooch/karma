/*LINTLIBRARY*/
/*  Cmapwinpopup.c

    This code provides a colourmap popup control widget for Xt.

    Copyright (C) 1994-1996  Richard Gooch

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

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains all routines needed for a colourmap control widget for
  Xt.


    Written by      Richard Gooch   17-DEC-1994

    Updated by      Richard Gooch   17-DEC-1994

    Updated by      Richard Gooch   3-JAN-1995: Added reverse and invert
  toggles and save and load buttons.

    Updated by      Richard Gooch   17-MAR-1996: Made use of <xtmisc_popup_cbk>

    Updated by      Richard Gooch   21-APR-1996: Added XtNvisual and
  XkwNsimpleColourbar resources.

    Updated by      Richard Gooch   28-APR-1996: Added support for colourmaps
  with DirectColour visual types.

    Updated by      Richard Gooch   3-MAY-1996: Switched to KtoggleWidget.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   15-AUG-1996: Made use of XkwNdefaultName
  resource for Dialogpopup widget.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#define X11
#include <Xkw/CmapwinpopupP.h>

#include <X11/Xos.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <varargs.h>

#include <sys/wait.h>
#include <signal.h>
#include <k_event_codes.h>
#include <karma_xtmisc.h>
#include <karma_dsxfr.h>
#include <karma_dir.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>
#include <Xkw/Filepopup.h>
#include <Xkw/Dialogpopup.h>
#include <Xkw/Filewin.h>
#include <Xkw/Cmapwin.h>
#include <Xkw/DirectCmapwin.h>
#include <Xkw/Ktoggle.h>


/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

STATIC_FUNCTION (void Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (void reverse_cbk, (Widget w, XtPointer client_data,
				    XtPointer call_data) );
STATIC_FUNCTION (void invert_cbk, (Widget w, XtPointer client_data,
				   XtPointer call_data) );
STATIC_FUNCTION (flag accept_file, (KFileInfo finfo) );
STATIC_FUNCTION (void cmap_got_one,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void save_dialog_cbk,
		 (Widget w,XtPointer client_data,XtPointer call_data) );
STATIC_FUNCTION (Widget create_button,
		 (char *name, WidgetClass type, char *label, Widget parent,
		  Widget left, flag small) );


/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define offset(field) XtOffset(CmapwinpopupWidget, cmapwinpopup.field)

static XtResource CmapwinpopupResources[] = 
{
    {XkwNcolourbarVisual, XtCVisual, XtRVisual, sizeof (Visual *),
     offset (colourbarVisual), XtRImmediate, CopyFromParent},
    {XkwNkarmaColourmap, XkwCKarmaColourmap, XtRPointer, sizeof (XtPointer),
     offset (cmap), XtRPointer, (XtPointer) NULL},
    {XkwNsimpleColourbar, XkwCSimpleColourbar, XtRBool,
     sizeof (Bool), offset (simpleColourbar), XtRImmediate,
     (XtPointer) False},
};
#undef offset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

CmapwinpopupClassRec cmapwinpopupClassRec = 
{
  {
      /* CoreClassPart */
    (WidgetClass)&topLevelShellClassRec,  /* superclass */
    "Cmapwinpopup",                /* class_name */
    sizeof (CmapwinpopupRec),      /* widget_size */
    NULL,                          /* class_initialise */
    NULL,                          /* class_part_initialise */
    FALSE,                         /* class_init */
    (XtInitProc) Initialise,       /* initialise */
    NULL,                          /* initialise_hook */
    XtInheritRealize,              /* realise */
    NULL,                          /* actions */
    0,                             /* num_actions */
    CmapwinpopupResources,         /* resources */
    XtNumber (CmapwinpopupResources),   /* num_resources */
    NULLQUARK,                     /* xrm_class */
    TRUE,                          /* compress_motion */
    TRUE,                          /* compress_exposure */
    TRUE,                          /* compress_enterleave */
    TRUE,                          /* visible_interest */
    NULL,                          /* destroy */
    XtInheritResize,               /* resize */
    NULL,                          /* expose */
    NULL,                          /* set_values */
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
  {  /* CmapwinpopupClassPart */
    0 /* empty */
  }
};

WidgetClass cmapwinpopupWidgetClass = (WidgetClass) &cmapwinpopupClassRec;

/*----------------------------------------------------------------------*/
/* Initialise */
/*----------------------------------------------------------------------*/

static void Initialise (Widget Request, Widget New)
{
    flag direct_visual_cmap_type;
    /*CmapwinpopupWidget request = (CmapwinpopupWidget) Request;*/
    CmapwinpopupWidget new = (CmapwinpopupWidget) New;
    Widget form, close_btn, reverse_tgl, invert_tgl, colourmapwin;
    Widget save_dialog, filepopup;
    Widget save_btn, load_btn;
    /*static char function_name[] = "CmapwinpopupWidget::Initialise";*/

    form = XtVaCreateManagedWidget ("form", formWidgetClass, New,
				    XtNborderWidth, 0,
				    NULL);
    close_btn = create_button ("closeButton", commandWidgetClass, "Close",
			       form, NULL, False);
    reverse_tgl = create_button ("reverseToggle", ktoggleWidgetClass, "Reverse",
				 form, close_btn, False);
    invert_tgl = create_button ("invertToggle", ktoggleWidgetClass, "Invert",
				form, reverse_tgl, True);
    save_btn = create_button ("saveButton", commandWidgetClass, "Save",
			      form, invert_tgl, False);
    load_btn = create_button ("loadButton", commandWidgetClass, "Load",
			      form, save_btn, False);

    XtAddCallback (close_btn, XtNcallback, xtmisc_popdown_cbk, New);
    XtAddCallback (reverse_tgl, XtNcallback, reverse_cbk, (XtPointer) New);
    XtAddCallback (invert_tgl, XtNcallback, invert_cbk, (XtPointer) New);
    kcmap_get_attributes (new->cmapwinpopup.cmap,
			  KCMAP_ATT_DIRECT_VISUAL, &direct_visual_cmap_type,
			  KCMAP_ATT_END);
    if (direct_visual_cmap_type)
    {
	colourmapwin = XtVaCreateManagedWidget
	    ("cmapwin", directCmapwinWidgetClass, form,
	     XkwNcolourbarVisual, new->cmapwinpopup.colourbarVisual,
	     XkwNkarmaColourmap, new->cmapwinpopup.cmap,
	     XtNborderWidth, 0,
	     XtNfromVert, close_btn,
	     NULL);
    }
    else
    {
	colourmapwin = XtVaCreateManagedWidget
	    ("cmapwin", cmapwinWidgetClass, form,
	     XkwNcolourbarVisual, new->cmapwinpopup.colourbarVisual,
	     XkwNkarmaColourmap, new->cmapwinpopup.cmap,
	     XkwNsimpleColourbar, new->cmapwinpopup.simpleColourbar,
	     XtNborderWidth, 0,
	     XtNfromVert, close_btn,
	     NULL);
    }
    new->cmapwinpopup.cmapwin = colourmapwin;
    save_dialog = XtVaCreatePopupShell ("save_dialog", dialogpopupWidgetClass,
					New,
					XtNlabel, "Save Filename:",
					XkwNdefaultName, "cmap",
					NULL);
    new->cmapwinpopup.save_dialog = save_dialog;
    XtAddCallback (save_dialog, XtNcallback, save_dialog_cbk, New);
    XtAddCallback (save_btn, XtNcallback, xtmisc_popup_cbk, save_dialog);
    filepopup = XtVaCreatePopupShell ("filewin", filepopupWidgetClass, New,
				      XkwNfilenameTester, accept_file,
				      XtNtitle, "Colourmap File Selector",
				      XkwNautoPopdown, True,
				      NULL);
    new->cmapwinpopup.filepopup = filepopup;
    XtAddCallback (filepopup, XkwNfileSelectCallback, cmap_got_one, New);
    XtAddCallback (load_btn, XtNcallback, xtmisc_popup_cbk, filepopup);
}   /*  End Function Initialise  */

static void reverse_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the reverse toggle callback.
*/
{
    flag bool = (flag) call_data;
    CmapwinpopupWidget top = (CmapwinpopupWidget) client_data;

    kcmap_set_attributes (top->cmapwinpopup.cmap,
			  KCMAP_ATT_REVERSE, bool,
			  KCMAP_ATT_END);
    XtVaSetValues (top->cmapwinpopup.cmapwin,
		   XkwNregenerateColourmap, True,
		   NULL);
}   /*  End Function reverse_cbk  */

static void invert_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the invert toggle callback.
*/
{
    flag bool = (flag) call_data;
    CmapwinpopupWidget top = (CmapwinpopupWidget) client_data;

    kcmap_set_attributes (top->cmapwinpopup.cmap,
			  KCMAP_ATT_INVERT, bool,
			  KCMAP_ATT_END);
    XtVaSetValues (top->cmapwinpopup.cmapwin,
		   XkwNregenerateColourmap, True,
		   NULL);
}   /*  End Function invert_cbk  */

static flag accept_file (KFileInfo finfo)
{
    if (finfo.type == KFILETYPE_DIRECTORY) return TRUE;
    if (strcmp (finfo.filename + strlen (finfo.filename) - 3, ".kf") == 0)
    {
	return TRUE;
    }
    /*  Reject everything else  */
    return FALSE;
}   /*  End Function accept_file  */

static void cmap_got_one (Widget w, XtPointer client_data, XtPointer call_data)
{
    char *fname = (char *) call_data;
    multi_array *multi_desc;
    CmapwinpopupWidget top = (CmapwinpopupWidget) client_data;

    if ( ( multi_desc = dsxfr_get_multi (fname, FALSE, K_CH_MAP_NEVER, FALSE) )
	== NULL )
    {
	XBell (XtDisplay (w), 100);
	return;
    }
    kcmap_copy_from_struct (top->cmapwinpopup.cmap, multi_desc->headers[0],
			    multi_desc->data[0]);
    ds_dealloc_multi (multi_desc);
}   /*  End Function cmap_got_one  */

static void save_dialog_cbk(Widget w,XtPointer client_data,XtPointer call_data)
{
    char fname[500];
    char *cwd;
    char *filename = (char *) call_data;
    multi_array *multi_desc;
    CmapwinpopupWidget top = (CmapwinpopupWidget) client_data;
    Widget filewin;
    static char function_name[] = "CmapwinpopupWidget::save_dialog_cbk";

    filewin = XtNameToWidget ( (Widget) top, "filewin*selector" );
    if ( !XtIsFilewin (filewin) )
    {
	(void) fprintf (stderr, "Could not find filewin widget\n");
	a_prog_bug (function_name);
    }
    if ( ( multi_desc = ds_alloc_multi (1) ) == NULL )
    {
	m_abort (function_name, "multi_array descriptor");
    }
    kcmap_copy_to_struct (top->cmapwinpopup.cmap, multi_desc->headers,
			  multi_desc->data);
    cwd = XkwFilewinCurrentDirectory (filewin);
    (void) sprintf (fname, "%s/%s", cwd, filename);
    dsxfr_put_multi (fname, multi_desc);
    ds_dealloc_multi (multi_desc);
}   /*  End Function save_dialog_cbk  */

static Widget create_button (char *name, WidgetClass type, char *label,
			     Widget parent, Widget left, flag small)
{
    int width = 73;

    if (small) width -= 2;

    return XtVaCreateManagedWidget (name, type, parent,
				    XtNlabel, label,
				    XkwNcrosses, False,
				    XtNheight, 20,
				    XtNwidth, width,
				    XtNfromHoriz, left,
				    XtNtop, XtChainTop,
				    XtNbottom, XtChainTop,
				    NULL);
}   /*  End Function create_button  */

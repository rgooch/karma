/*LINTLIBRARY*/
/*  ZoomPolicy.c

    This code provides a data clip control widget for Xt.

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

/*  This file contains all routines needed for a data clip control widget for
  Xt.


    Written by      Richard Gooch   12-MAY-1996

    Last updated by Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/

#include <stdio.h>
#include <math.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <karma.h>
#include <k_event_codes.h>
#include <karma_viewimg.h>
#include <karma_xtmisc.h>
#include <karma_m.h>
#include <karma_a.h>
#include <Xkw/ZoomPolicyP.h>
#include <Xkw/Ktoggle.h>


/*  Private functions  */

STATIC_FUNCTION (void ZoomPolicy__Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (Boolean ZoomPolicy__SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void apply_cbk, (Widget w, XtPointer client_data,
				  XtPointer call_data) );
STATIC_FUNCTION (void set_bool,
		 (ZoomPolicyWidget top, unsigned int att, flag val) );
STATIC_FUNCTION (void auto_refresh_cbk, (Widget w, XtPointer client_data,
					 XtPointer call_data) );
STATIC_FUNCTION (void auto_intensity_cbk, (Widget w, XtPointer client_data,
					   XtPointer call_data) );
STATIC_FUNCTION (void aspect_cbk, (Widget w, XtPointer client_data,
				   XtPointer call_data) );
STATIC_FUNCTION (void trunc_cbk, (Widget w, XtPointer client_data,
				  XtPointer call_data) );
STATIC_FUNCTION (void int_x_cbk, (Widget w, XtPointer client_data,
				  XtPointer call_data) );
STATIC_FUNCTION (void int_y_cbk, (Widget w, XtPointer client_data,
				  XtPointer call_data) );


/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define offset(field) XtOffset(ZoomPolicyWidget, zoomPolicy.field)

static XtResource ZoomPolicyResources[] = 
{
    {XkwNcanvases, XkwCCanvases, XtRPointer, sizeof (XtPointer),
     offset (canvases), XtRImmediate, NULL},
    {XkwNautoIntensityScale, XkwCAutoIntensityScale, XtRBool, sizeof (Bool),
     offset (autoIntensityScale), XtRImmediate, (XtPointer) True},
};

#undef offset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

ZoomPolicyClassRec zoomPolicyClassRec = 
{
  {
      /* CoreClassPart */
    (WidgetClass)&topLevelShellClassRec,     /* superclass */
    "ZoomPolicy",                            /* class_name */
    sizeof (ZoomPolicyRec),                  /* widget_size */
    NULL,                                    /* class_initialise */
    NULL,                                    /* class_part_initialise */
    FALSE,                                   /* class_init */
    (XtInitProc) ZoomPolicy__Initialise,     /* initialise */
    NULL,                                    /* initialise_hook */
    XtInheritRealize,                        /* realise */
    NULL,                                    /* actions */
    0,                                       /* num_actions */
    ZoomPolicyResources,                     /* resources */
    XtNumber (ZoomPolicyResources),          /* num_resources */
    NULLQUARK,                               /* xrm_class */
    TRUE,                                    /* compress_motion */
    TRUE,                                    /* compress_exposure */
    TRUE,                                    /* compress_enterleave */
    TRUE,                                    /* visible_interest */
    NULL,                                    /* destroy */
    XtInheritResize,                         /* resize */
    NULL,                                    /* expose */
    (XtSetValuesFunc) ZoomPolicy__SetValues, /* set_values */
    NULL,                                    /* set_values_hook */
    XtInheritSetValuesAlmost,                /* set_values_almost */
    NULL,                                    /* get_values_hook */
    NULL,                                    /* accept_focus */
    XtVersion,                               /* version */
    NULL,                                    /* callback_private */
    NULL,                                    /* tm_translations */
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
  {  /* ZoomPolicyClassPart */
    0 /* empty */
  }
};

WidgetClass zoomPolicyWidgetClass = (WidgetClass) &zoomPolicyClassRec;

static void ZoomPolicy__Initialise (Widget Request, Widget New)
{
    /*ZoomPolicyWidget request = (ZoomPolicyWidget) Request;*/
    ZoomPolicyWidget new = (ZoomPolicyWidget) New;
    Widget form;
    Widget w, upper_widget;
    /*static char function_name[] = "ZoomPolicyWidget::Initialise";*/

    new->zoomPolicy.auto_refresh = TRUE;
    form = XtVaCreateManagedWidget ("form", formWidgetClass, New,
				    XtNborderWidth, 0,
				    NULL);

    w = XtVaCreateManagedWidget ("closeButton", commandWidgetClass, form,
				 XtNlabel, "Close",
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNleft, XtChainLeft,
				 XtNheight, 20,
				 NULL);
    XtAddCallback (w, XtNcallback, xtmisc_popdown_cbk, New);
    w = XtVaCreateManagedWidget ("apply", commandWidgetClass, form,
				 XtNlabel, "Apply",
				 XtNfromHoriz, w,
				 NULL);
    XtAddCallback (w, XtNcallback, apply_cbk, New);
    w = XtVaCreateManagedWidget ("toggle", ktoggleWidgetClass, form,
				 XtNlabel, "Auto Refresh",
				 XtNstate, True,
				 XkwNcrosses, False,
				 XtNfromHoriz, w,
				 NULL);
    upper_widget = w;
    XtAddCallback (w, XtNcallback, auto_refresh_cbk, New);
    w = XtVaCreateManagedWidget ("toggle", ktoggleWidgetClass, form,
				 XtNlabel, "Auto Intensity Scale",
				 XtNfromVert, upper_widget,
				 XtNstate, new->zoomPolicy.autoIntensityScale,
				 XkwNcrosses, False,
				 NULL);
    upper_widget = w;
    XtAddCallback (w, XtNcallback, auto_intensity_cbk, New);
    w = XtVaCreateManagedWidget ("toggle", ktoggleWidgetClass, form,
				 XtNlabel, "Fix Aspect",
				 XtNfromVert, upper_widget,
				 XtNstate, True,
				 XkwNcrosses, False,
				 NULL);
    XtAddCallback (w, XtNcallback, aspect_cbk, New);
    w = XtVaCreateManagedWidget ("toggle", ktoggleWidgetClass, form,
				 XtNlabel, "Allow Truncation",
				 XtNfromHoriz, w,
				 XtNfromVert, upper_widget,
				 XtNstate, True,
				 XkwNcrosses, False,
				 NULL);
    upper_widget = w;
    XtAddCallback (w, XtNcallback, trunc_cbk, New);
    w = XtVaCreateManagedWidget ("toggle", ktoggleWidgetClass, form,
				 XtNlabel, "Integer X Zoom",
				 XtNfromVert, upper_widget,
				 XtNstate, True,
				 XkwNcrosses, False,
				 NULL);
    XtAddCallback (w, XtNcallback, int_x_cbk, New);
    w = XtVaCreateManagedWidget ("toggle", ktoggleWidgetClass, form,
				 XtNlabel, "Integer Y Zoom",
				 XtNfromHoriz, w,
				 XtNfromVert, upper_widget,
				 XtNstate, True,
				 XkwNcrosses, False,
				 NULL);
    upper_widget = w;
    XtAddCallback (w, XtNcallback, int_y_cbk, New);
}   /*  End Function Initialise  */

static Boolean ZoomPolicy__SetValues (Widget Current, Widget Request,
				      Widget New)
{
    return False;
}   /*  End Function SetValues  */

static void apply_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the apply callback.
*/
{
    KPixCanvas pixcanvas;
    flag visible;
    ZoomPolicyWidget top = (ZoomPolicyWidget) client_data;
    KWorldCanvas *canvas;

    for (canvas = top->zoomPolicy.canvases; *canvas != NULL; ++canvas)
    {
	pixcanvas = canvas_get_pixcanvas (*canvas);
	kwin_get_attributes (pixcanvas,
			     KWIN_ATT_VISIBLE, &visible,
			     KWIN_ATT_END);
	if (!visible) continue;
	/*  Resize the canvas  */
	if ( !canvas_resize (*canvas, NULL, FALSE) )
	{
	    (void) fprintf (stderr, "Error resizing canvas\n");
	}
    }
}   /*  End Function apply_cbk   */

static void set_bool (ZoomPolicyWidget top, unsigned int att, flag val)
{
    KPixCanvas pixcanvas;
    flag visible;
    KWorldCanvas *canvas;

    if (top->zoomPolicy.canvases == NULL) return;
    for (canvas = top->zoomPolicy.canvases; *canvas != NULL; ++canvas)
    {
	viewimg_set_canvas_attributes (*canvas,
				       att, val,
				       VIEWIMG_ATT_END);
	if (!top->zoomPolicy.auto_refresh) continue;
	pixcanvas = canvas_get_pixcanvas (*canvas);
	kwin_get_attributes (pixcanvas,
			     KWIN_ATT_VISIBLE, &visible,
			     KWIN_ATT_END);
	if (!visible) continue;
	/*  Resize the canvas  */
	if ( !canvas_resize (*canvas, NULL, FALSE) )
	{
	    (void) fprintf (stderr, "Error resizing canvas\n");
	}
    }
}   /*  End Function set_bool   */

static void auto_refresh_cbk (Widget w, XtPointer client_data,
			      XtPointer call_data)
/*  This is the auto refresh callback.
*/
{
    ZoomPolicyWidget top = (ZoomPolicyWidget) client_data;

    top->zoomPolicy.auto_refresh = (uaddr) call_data;
}   /*  End Function auto_refresh_cbk   */

static void auto_intensity_cbk (Widget w, XtPointer client_data,
				XtPointer call_data)
/*  This is the auto intensity callback.
*/
{
    flag bool = (uaddr) call_data;
    ZoomPolicyWidget top = (ZoomPolicyWidget) client_data;

    set_bool (top, VIEWIMG_ATT_AUTO_V, bool);
}   /*  End Function auto_intensity_cbk   */

static void aspect_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the aspect callback.
*/
{
    flag bool = (uaddr) call_data;
    ZoomPolicyWidget top = (ZoomPolicyWidget) client_data;

    set_bool (top, VIEWIMG_ATT_MAINTAIN_ASPECT, bool);
}   /*  End Function aspect_cbk   */

static void trunc_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the trunc callback.
*/
{
    flag bool = (uaddr) call_data;
    ZoomPolicyWidget top = (ZoomPolicyWidget) client_data;

    set_bool (top, VIEWIMG_ATT_ALLOW_TRUNCATION, bool);
}   /*  End Function trunc_cbk   */

static void int_x_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the int_x callback.
*/
{
    flag bool = (uaddr) call_data;
    ZoomPolicyWidget top = (ZoomPolicyWidget) client_data;

    set_bool (top, VIEWIMG_ATT_INT_X, bool);
}   /*  End Function int_x_cbk   */

static void int_y_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the int_y callback.
*/
{
    flag bool = (uaddr) call_data;
    ZoomPolicyWidget top = (ZoomPolicyWidget) client_data;

    set_bool (top, VIEWIMG_ATT_INT_Y, bool);
}   /*  End Function int_y_cbk   */

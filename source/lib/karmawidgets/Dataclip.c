/*LINTLIBRARY*/
/*  Dataclip.c

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


    Written by      Richard Gooch   23-OCT-1994

    Updated by      Richard Gooch   26-NOV-1994

    Updated by      Richard Gooch   25-DEC-1994: Added autoValueScale resource.

    Uupdated by     Richard Gooch   27-DEC-1994: Delay computing histogram
  until widget popped up.

    Updated by      Richard Gooch   6-JAN-1995: Changed from passing flag
  by value to by reference for intensityScaleCallback.

    Updated by      Richard Gooch   9-APR-1995: Added initialisation of
  conv_type  field in  win_scale  structure.

    Updated by      Richard Gooch   17-APR-1995: Fixed mix of XtRBool and
  sizeof Boolean in resource fields. Use Bool because it is of int size.

    Updated by      Richard Gooch   21-APR-1995: Added Full Range button.

    Updated by      Richard Gooch   30-JUL-1995: Added destroy callback for
  Intelligent Array.

    Updated by      Richard Gooch   28-DEC-1995: Added verbose resource.

    Updated by      Richard Gooch   22-FEB-1996: Allow dragging of end-points.

    Updated by      Richard Gooch   18-MAR-1996: Display clip limits.

    Updated by      Richard Gooch   4-MAY-1996: Switched to KtoggleWidget and
  added showBlankControl resource.

    Updated by      Richard Gooch   24-MAY-1996: Completed save functionality
  and set XtNresizable resource to TRUE for the save dialog widget so that
  longer names may be typed and viewed.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   4-JUN-1996: Switched to left-right
  bottom-top co-ordinate specification instead of min-max x and y.

    Updated by      Richard Gooch   15-JUN-1996: Added scaling information when
  writing integer types.

    Updated by      Richard Gooch   16-JUN-1996: Read input scaling information
  and combine with computed scaling when writing integer types.

    Updated by      Richard Gooch   20-JUN-1996: Fixed bug in <SetValues> where
  destroy callback was not being correctly cleared and set.

    Updated by      Richard Gooch   21-JUN-1996: Fixed bug in
  <iarr_destroy_callback> where destroy callback was not being cleared.

    Updated by      Richard Gooch   15-AUG-1996: Added autoPopdown resource.

    Updated by      Richard Gooch   2-OCT-1996: Added <get_colour> routine to
  make widget more robust when insufficient colours are available. Added
  cursor position label. Apply cube scaling attachments to all value displays.

    Last updated by Richard Gooch   13-OCT-1996: Added Zoom and Unzoom buttons.


*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <sys/wait.h>
#include <signal.h>
#include <X11/Xos.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Dialog.h>
#ifndef X11
#  define X11
#endif
#include <karma.h>
#define NEW_WIN_SCALE
#include <k_event_codes.h>
#include <karma_iarray.h>
#include <karma_c.h>
#include <karma_xtmisc.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>
#include <Xkw/DataclipP.h>
#include <Xkw/Canvas.h>
#include <Xkw/Ktoggle.h>
#include <Xkw/ExclusiveMenu.h>


/*  Private functions  */

STATIC_FUNCTION (void Dataclip__ClassInititialise, () );
STATIC_FUNCTION (void Dataclip__Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (void Dataclip__cnv_realise_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (Boolean Dataclip__SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void immediate_apply_cbk, (Widget w, XtPointer client_data,
					    XtPointer call_data) );
STATIC_FUNCTION (void apply_cbk, (Widget w, XtPointer client_data,
				  XtPointer call_data) );
STATIC_FUNCTION (void histogram_canvas_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale, Kcolourmap cmap,
		  flag cmap_resize, void **info) );
STATIC_FUNCTION (flag histogram_canvas_position_func,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info, void **f_info,
		  double x_lin, double y_lin) );
STATIC_FUNCTION (void size_control_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  void **info, flag *boundary_clear) );
STATIC_FUNCTION (void pop_cbk, (Widget w, XtPointer client_data,
				XtPointer call_data) );
STATIC_FUNCTION (void full_range_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (void iarr_destroy_callback, (iarray arr,DataclipWidget top) );
STATIC_FUNCTION (void blank_data_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (void type_menu_cbk, (Widget w, XtPointer client_data,
				      XtPointer call_data) );
STATIC_FUNCTION (void save_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );
STATIC_FUNCTION (unsigned long get_colour,
		 (KWorldCanvas canvas, CONST char *colourname) );
STATIC_FUNCTION (void zoom_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );
STATIC_FUNCTION (void unzoom_cbk, (Widget w, XtPointer client_data,
				   XtPointer call_data) );


/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define offset(field) XtOffset(DataclipWidget, dataclip.field)

static XtResource DataclipResources[] = 
{
    {XkwNiarray, XkwCIarray, XtRPointer, sizeof (XtPointer),
     offset (array), XtRImmediate, NULL},
    {XkwNmaxDataRegions, XkwCMaxDataRegions, XtRInt, sizeof (int),
     offset (maxRegions), XtRImmediate, (XtPointer) 1},
    {XkwNregionCallback, XtCCallback, XtRCallback, sizeof (caddr_t),
     offset (regionCallback), XtRCallback, (caddr_t) NULL},
    {XkwNshowBlankControl, XkwCShowBlankControl, XtRBool, sizeof (Bool),
     offset (showBlankControl), XtRImmediate, (XtPointer) False},
    {XkwNfixedOutputType, XkwCFixedOutputType, XtRCardinal, sizeof (Cardinal),
     offset (fixedOutputType), XtRImmediate, (XtPointer) NONE},
    {XkwNverbose, XkwCVerbose, XtRBool, sizeof (Bool),
     offset (verbose), XtRImmediate, False},
    {XkwNautoPopdown, XkwCAutoPopdown, XtRBool, sizeof (Bool),
     offset (autoPopdown), XtRImmediate, (XtPointer) FALSE},
};

#undef offset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

DataclipClassRec dataclipClassRec = 
{
  {
      /* CoreClassPart */
    (WidgetClass)&topLevelShellClassRec,  /* superclass */
    "Dataclip",                    /* class_name */
    sizeof (DataclipRec),          /* widget_size */
    Dataclip__ClassInititialise,   /* class_initialise */
    NULL,                          /* class_part_initialise */
    FALSE,                         /* class_init */
    (XtInitProc) Dataclip__Initialise,  /* initialise */
    NULL,                          /* initialise_hook */
    XtInheritRealize,              /* realise */
    NULL,                          /* actions */
    0,                             /* num_actions */
    DataclipResources,             /* resources */
    XtNumber (DataclipResources),  /* num_resources */
    NULLQUARK,                     /* xrm_class */
    TRUE,                          /* compress_motion */
    TRUE,                          /* compress_exposure */
    TRUE,                          /* compress_enterleave */
    TRUE,                          /* visible_interest */
    NULL,                          /* destroy */
    XtInheritResize,               /* resize */
    NULL,                          /* expose */
    (XtSetValuesFunc) Dataclip__SetValues,   /* set_values */
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
      /* geometry_manager */	XtInheritGeometryManager,
      /* change_managed	  */	XtInheritChangeManaged,
      /* insert_child	  */	XtInheritInsertChild,
      /* delete_child	  */	XtInheritDeleteChild,
      /* extension	  */	NULL
  },
  { /* Shell */
      /* extension	  */	NULL
  },
  { /* WMShell */
      /* extension	  */	NULL
  },
  { /* VendorShell */
      /* extension	  */	NULL
  },
  { /* TopLevelShell */
      /* extension	  */	NULL
  },
  {  /* DataClipClassPart */
      {0}, {NULL}, 0      /* empty */
  }
};

WidgetClass dataclipWidgetClass = (WidgetClass) &dataclipClassRec;

static void Dataclip__ClassInititialise ()
{
    unsigned int count, index;
    extern DataclipClassRec dataclipClassRec;
    extern char *data_type_names[NUMTYPES];

    for (count = 0, index = 0; count < NUMTYPES; ++count)
    {
	if ( !ds_element_is_legal (count) ) continue;
	if ( !ds_element_is_atomic (count) ) continue;
	if ( ds_element_is_complex (count) ) continue;
	dataclipClassRec.dataclip_class.type_index_to_type[index] = count;
	dataclipClassRec.dataclip_class.type_names[index] =
	    data_type_names[count];
	++index;
    }
    dataclipClassRec.dataclip_class.num_types = index;
}   /*  End Function ClassInitialise  */

static void Dataclip__Initialise (Widget Request, Widget New)
{
    /*DataclipWidget request = (DataclipWidget) Request;*/
    DataclipWidget new = (DataclipWidget) New;
    Widget form;
    Widget close_btn, save_btn;
    Widget full_range_btn;
    Widget cnv;
    Widget w;
    extern DataclipClassRec dataclipClassRec;
    static char function_name[] = "DataclipWidget::Initialise";

    new->dataclip.pixcanvas = NULL;
    new->dataclip.worldcanvas = NULL;
    new->dataclip.iarr_destroy_callback = NULL;
    new->dataclip.immediate_apply = FALSE;
    new->dataclip.last_was_left = FALSE;
    new->dataclip.left_pos = TOOBIG;
    new->dataclip.popped_up = FALSE;
    new->dataclip.blank_data = FALSE;
    XtAddCallback (New, XtNpopupCallback, pop_cbk, (XtPointer) TRUE);
    XtAddCallback (New, XtNpopdownCallback, pop_cbk, (XtPointer) FALSE);
    if (new->dataclip.maxRegions < 1)
    {
	fprintf (stderr, "maxRegions: %u  must be greater than 0\n",
		 new->dataclip.maxRegions);
	a_prog_bug (function_name);
    }
    if ( ( new->dataclip.minima = (double *) m_alloc
	  (sizeof *new->dataclip.minima * new->dataclip.maxRegions) )
	== NULL )
    {
	m_abort (function_name, "array of minima");
    }
    if ( ( new->dataclip.maxima = (double *) m_alloc
	  (sizeof *new->dataclip.maxima * new->dataclip.maxRegions) )
	== NULL )
    {
	m_abort (function_name, "array of maxima");
    }
    new->dataclip.num_regions = 0;
    new->dataclip.histogram_array = NULL;
    new->dataclip.hist_arr_length = 0;
    new->dataclip.hist_buf_length = 0;
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
    close_btn = w;
    XtAddCallback (close_btn, XtNcallback, xtmisc_popdown_cbk, New);
    if (new->dataclip.showBlankControl)
    {
	w = XtVaCreateManagedWidget ("blankToggle", ktoggleWidgetClass, form,
				     XtNlabel, "Blank",
				     XtNfromHoriz, w,
				     XtNtop, XtChainTop,
				     XtNbottom, XtChainTop,
				     XtNheight, 20,
				     XtNstate, new->dataclip.blank_data,
				     XkwNcrosses, False,
				     NULL);
	XtAddCallback (w, XtNcallback, blank_data_cbk, (XtPointer) New);
    }
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
				 XtNlabel, "Zoom",
				 XtNfromHoriz, w,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNleft, XtChainLeft,
				 XtNheight, 20,
				 NULL);
    XtAddCallback (w, XtNcallback, zoom_cbk, New);
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
				 XtNlabel, "Unzoom",
				 XtNfromHoriz, w,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNleft, XtChainLeft,
				 XtNheight, 20,
				 NULL);
    XtAddCallback (w, XtNcallback, unzoom_cbk, New);
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
				 XtNlabel, "Apply",
				 XtNfromVert, close_btn,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNleft, XtChainLeft,
				 XtNheight, 20,
				 NULL);
    XtAddCallback (w, XtNcallback, apply_cbk, New);
    if (new->dataclip.maxRegions == 1)
    {
	w = XtVaCreateManagedWidget ("toggle", ktoggleWidgetClass, form,
				     XtNlabel, "Auto Apply",
				     XtNfromHoriz, w,
				     XtNfromVert, close_btn,
				     XtNtop, XtChainTop,
				     XtNbottom, XtChainTop,
				     XtNheight, 20,
				     XtNstate, False,
				     XkwNcrosses, False,
				     NULL);
	XtAddCallback (w, XtNcallback, immediate_apply_cbk, (XtPointer) New);
    }
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
				 XtNlabel, "Full Range",
				 XtNfromHoriz, w,
				 XtNfromVert, close_btn,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNheight, 20,
				 NULL);
    full_range_btn = w;
    XtAddCallback (full_range_btn, XtNcallback, full_range_cbk,
		   (XtPointer) New);
    w = XtVaCreateManagedWidget ("label", labelWidgetClass, form,
				 XtNlabel, "Data Minimum:              ",
				 XtNborderWidth, 0,
				 XtNfromVert, w,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNleft, XtChainLeft,
				 XtNheight, 20,
				 XtNresizable, True,
				 NULL);
    new->dataclip.min_label = w;
    w = XtVaCreateManagedWidget ("label", labelWidgetClass, form,
				 XtNlabel, "Data Maximum:              ",
				 XtNborderWidth, 0,
				 XtNfromVert, w,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNleft, XtChainLeft,
				 XtNheight, 20,
				 XtNresizable, True,
				 NULL);
    new->dataclip.max_label = w;
    w = XtVaCreateManagedWidget ("label", labelWidgetClass, form,
				 XtNlabel, "Lower Clip:                ",
				 XtNborderWidth, 0,
				 XtNfromVert, w,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNleft, XtChainLeft,
				 XtNheight, 20,
				 XtNresizable, True,
				 NULL);
    new->dataclip.lower_label = w;
    w = XtVaCreateManagedWidget ("label", labelWidgetClass, form,
				 XtNlabel, "Upper Clip:                ",
				 XtNborderWidth, 0,
				 XtNfromVert, w,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNleft, XtChainLeft,
				 XtNheight, 20,
				 XtNresizable, True,
				 NULL);
    new->dataclip.upper_label = w;
    w = XtVaCreateManagedWidget ("label", labelWidgetClass, form,
				 XtNlabel, "Cursor Position:                 ",
				 XtNborderWidth, 0,
				 XtNfromVert, w,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNleft, XtChainLeft,
				 XtNheight, 20,
				 XtNresizable, True,
				 NULL);
    new->dataclip.cursor_label = w;
    save_btn = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
					XtNlabel, "Save",
					XtNfromVert, w,
					NULL);
    if (new->dataclip.fixedOutputType == NONE)
    {
	w = XtVaCreateManagedWidget ("menuButton", exclusiveMenuWidgetClass,
				     form,
				     XtNmenuName, "typeMenu",
				     XkwNchoiceName, "Output Type",
				     XtNfromVert, w,
				     XtNfromHoriz, save_btn,
				     XtNleft, XtChainLeft,
				     XtNright, XtChainRight,
				     XkwNnumItems,
				     dataclipClassRec.dataclip_class.num_types,
				     XkwNitemStrings,
				    dataclipClassRec.dataclip_class.type_names,
				     NULL);
	XtAddCallback (w, XkwNselectCallback, type_menu_cbk, New);
	new->dataclip.output_type =
	    dataclipClassRec.dataclip_class.type_index_to_type[0];
    }
    else new->dataclip.output_type = new->dataclip.fixedOutputType;
    w = save_btn;
    w = XtVaCreateManagedWidget ("filename", dialogWidgetClass, form,
				 XtNlabel, "Filename: ",
				 XtNborderWidth, 0,
				 XtNfromVert, w,
				 XtNvalue, "scaled",
				 XtNresizable, TRUE,
				 NULL);
    new->dataclip.dialog = w;
    XtAddCallback (save_btn, XtNcallback, save_cbk, New);
    cnv = XtVaCreateManagedWidget ("pseudoColourCanvas", canvasWidgetClass,
				   form,
				   XtNfromVert, w,
				   XtNleft, XtChainLeft,
				   XtNright, XtChainRight,
				   XtNwidth, 300,
				   XtNheight, 200,
				   XkwNsilenceUnconsumed, True,
				   XtNresizable, True,
				   XtNtop, XtChainTop,
				   XtNbottom, XtChainBottom,
				   NULL);
    new->dataclip.cnv = cnv;
    XtAddCallback (cnv, XkwNrealiseCallback, Dataclip__cnv_realise_cbk,
		   (XtPointer) New);
}   /*  End Function Initialise  */

static void Dataclip__cnv_realise_cbk (Widget w, XtPointer client_data,
				       XtPointer call_data)
/*  This is the canvas realise callback.
*/
{
    KPixCanvas pixcanvas = (KPixCanvas) call_data;
    KWorldCanvas wc;
    GC gc;
    XGCValues gcvalues;
    struct win_scale_type win_scale;
    DataclipWidget top = (DataclipWidget) client_data;
    /*static char function_name[] = "DataclipWidget::cnv_realise_cbk";*/

    top->dataclip.pixcanvas = pixcanvas;
    /*  Get the Graphics Context of the pixel canvas  */
    gc = kwin_get_gc_x (pixcanvas);
    if (XGetGCValues (XtDisplay (w), gc, GCForeground | GCBackground,
		      &gcvalues) == 0)
    {
	fprintf (stderr, "Error getting GC values\n");
	exit (RV_UNDEF_ERROR);
    }
    /*  Create the offset control world canvas  */
    canvas_init_win_scale (&win_scale, K_WIN_SCALE_MAGIC_NUMBER);
    /*  No histogram data yet  */
    win_scale.min_sat_pixel = gcvalues.background;
    win_scale.max_sat_pixel = gcvalues.foreground;
    win_scale.conv_type = CONV_CtoR_REAL;
    if ( ( wc = canvas_create (pixcanvas, NULL, &win_scale) ) == NULL )
    {
	exit (RV_UNDEF_ERROR);
    }
    top->dataclip.worldcanvas = wc;
    canvas_use_log_scale (wc, FALSE, TRUE);
    canvas_register_refresh_func (wc, histogram_canvas_refresh_func,
				  (void *) top);
    canvas_register_position_event_func (wc,
						histogram_canvas_position_func,
						(void *) top);
    canvas_register_size_control_func (wc, size_control_func, (void *) top);
}   /*  End Function cnv_realise_cbk   */

static Boolean Dataclip__SetValues (Widget Current, Widget Request, Widget New)
{
    DataclipWidget current = (DataclipWidget) Current;
    /*DataclipWidget request = (DataclipWidget) Request;*/
    DataclipWidget new = (DataclipWidget) New;
    static char function_name[] = "DataclipWidget::SetValues";

    if (new->dataclip.maxRegions != current->dataclip.maxRegions)
    {
	fprintf (stderr, "Cannot change number of regions\n");
	a_prog_bug (function_name);
    }
    if (new->dataclip.array != current->dataclip.array)
    {
	/*  Change in array  */
	if (new->dataclip.iarr_destroy_callback != NULL)
	{
	    c_unregister_callback (new->dataclip.iarr_destroy_callback);
	    new->dataclip.iarr_destroy_callback = NULL;
	}
	new->dataclip.data_min = TOOBIG;
	new->dataclip.data_max = -TOOBIG;
	new->dataclip.hist_arr_length = 0;
	/*  Force computation and display of histogram if new data available,
	    else clear canvas  */
	if (new->dataclip.popped_up)
	{
	    kwin_resize (new->dataclip.pixcanvas, TRUE, 0, 0, 0, 0);
	}
	if (new->dataclip.array != NULL)
	{
	    /*  New array  */
	    new->dataclip.iarr_destroy_callback = 
		iarray_register_destroy_func (new->dataclip.array,
					      ( flag (*) () ) iarr_destroy_callback,
					      new);
	}
    }
    return False;
}   /*  End Function SetValues  */

static void immediate_apply_cbk (Widget w, XtPointer client_data,
				 XtPointer call_data)
{
    flag bool = (uaddr) call_data;
    DataclipWidget top = (DataclipWidget) client_data;

    top->dataclip.immediate_apply = bool;
}   /*  End Function immediate_apply_cbk  */

static void apply_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    double scale, offset;
    DataclipWidget top = (DataclipWidget) client_data;
    DataclipRegions regions;
    char txt[STRING_LENGTH];

    if (top->dataclip.array == NULL) return;
    regions.blank_data_outside_regions = top->dataclip.blank_data;
    regions.num_regions = top->dataclip.num_regions;
    if ( ( regions.minima = (double *) m_dup ( (char *) top->dataclip.minima,
					      sizeof *top->dataclip.minima *
					      regions.num_regions ) )
	== NULL ) return;
    if ( ( regions.maxima = (double *) m_dup ( (char *) top->dataclip.maxima,
					      sizeof *top->dataclip.maxima *
					      regions.num_regions ) )
	== NULL )
    {
	m_free ( (char *) regions.minima );
	return;
    }
    iarray_get_data_scaling (top->dataclip.array, &scale, &offset);
    sprintf (txt, "Lower Clip: %e", regions.minima[0] * scale + offset);
    XtVaSetValues (top->dataclip.lower_label,
		   XtNlabel, txt,
		   NULL);
    sprintf (txt, "Upper Clip: %e",
	     regions.maxima[regions.num_regions - 1] * scale + offset);
    XtVaSetValues (top->dataclip.upper_label,
		   XtNlabel, txt,
		   NULL);
    XtCallCallbacks ( (Widget) top, XkwNregionCallback, (XtPointer) &regions );
    m_free ( (char *) regions.minima );
    m_free ( (char *) regions.maxima );
}   /*  End Function apply_cbk  */

static void histogram_canvas_refresh_func (canvas, width, height, win_scale,
					   cmap, cmap_resize, info)
/*  This routine is a refresh event consumer for a world canvas.
    The canvas is given by  canvas  .
    The width of the canvas in pixels is given by  width  .
    The height of the canvas in pixels is given by  height  .
    The window scaling information is pointed to by  win_scale  .
    The colourmap associated with the canvas is given by  cmap  .
    If the refresh function was called as a result of a colourmap resize
    the value of  cmap_resize  will be TRUE.
    The arbitrary canvas information pointer is pointed to by  info  .
    The routine returns nothing.
*/
KWorldCanvas canvas;
int width;
int height;
struct win_scale_type *win_scale;
Kcolourmap cmap;
flag cmap_resize;
void **info;
{
    int count;
    unsigned long pixel_value;
    DataclipWidget w = (DataclipWidget) *info;
    double x0, x1;
    /*static char function_name[] = "histogram_canvas_refresh_func";*/

    if (w->dataclip.array == NULL) return;
    canvas_draw_lines_p (canvas, (double *) NULL,
			 w->dataclip.histogram_array,
			 w->dataclip.hist_arr_length,
			 win_scale->max_sat_pixel);
    pixel_value = get_colour (canvas, "yellow4");
    if (w->dataclip.left_pos < TOOBIG)
    {
	x0 = w->dataclip.left_pos;
	canvas_draw_line_p (canvas,
			    x0, win_scale->bottom_y, x0, win_scale->top_y,
			    pixel_value);
    }
    if (w->dataclip.num_regions < 1) return;
    for (count = 0; count < w->dataclip.num_regions; ++count)
    {
	x0 = w->dataclip.minima[count];
	x1 = w->dataclip.maxima[count];
	canvas_draw_line_p (canvas,
			    x0, win_scale->bottom_y, x0, win_scale->top_y,
			    pixel_value);
	canvas_draw_line_p (canvas,
			    x1, win_scale->bottom_y, x1, win_scale->top_y,
			    pixel_value);
	canvas_draw_line_p (canvas,
			    x0, win_scale->bottom_y, x1, win_scale->top_y,
			    pixel_value);
	canvas_draw_line_p (canvas,
			    x1, win_scale->bottom_y, x0, win_scale->top_y,
			    pixel_value);
    }
}   /*  End Function histogram_canvas_refresh_func  */

static flag histogram_canvas_position_func (KWorldCanvas canvas,
					    double x, double y,
					    unsigned int event_code,
					    void *e_info, void **f_info,
					    double x_lin, double y_lin)
/*  This routine is a position event consumer for a world canvas.
    The canvas is given by  canvas  .
    The horizontal world co-ordinate of the event will be given by  x  .
    The vertical world co-ordinate of the event will be given by  y  .
    The arbitrary event code is given by  event_code  .
    The arbitrary event information is pointed to by  e_info  .
    The arbitrary function information pointer is pointed to by  f_info  .
    The routine returns TRUE if the event was consumed, else it returns
    FALSE indicating that the event is still to be processed.
*/
{
    flag apply = FALSE;
    double scale, offset;
    DataclipWidget w = (DataclipWidget) *f_info;
    char txt[STRING_LENGTH];

    switch (event_code)
    {
      case K_CANVAS_EVENT_POINTER_MOVE:
	if (w->dataclip.array == NULL) return (FALSE);
	iarray_get_data_scaling (w->dataclip.array, &scale, &offset);
	sprintf (txt, "Cursor Position: %e\n", x * scale + offset);
	XtVaSetValues (w->dataclip.cursor_label,
		       XtNlabel, txt,
		       NULL);
	return (TRUE);
	/*break;*/
      case K_CANVAS_EVENT_LEFT_MOUSE_CLICK:
      case K_CANVAS_EVENT_LEFT_MOUSE_DRAG:
	if (w->dataclip.last_was_left)
	{
	    XBell (XtDisplay (w), 100);
	    return (TRUE);
	}
	if ( (w->dataclip.maxRegions == 1) &&
	    (w->dataclip.num_regions == 1) )
	{
	    if (x >= w->dataclip.maxima[0])
	    {
		XBell (XtDisplay (w), 100);
		return (TRUE);
	    }
	    w->dataclip.minima[0] = x;
	    if (w->dataclip.immediate_apply) apply = TRUE;
	}
	else if (w->dataclip.num_regions >= w->dataclip.maxRegions)
	{
	    XBell (XtDisplay (w), 100);
	    return (TRUE);
	}
	else
	{
	    w->dataclip.left_pos = x;
	    w->dataclip.last_was_left = TRUE;
	}
	break;
      case K_CANVAS_EVENT_RIGHT_MOUSE_CLICK:
      case K_CANVAS_EVENT_RIGHT_MOUSE_DRAG:
	if ( (w->dataclip.maxRegions == 1) &&
	    (w->dataclip.num_regions == 1) )
	{
	    if (x <= w->dataclip.minima[0])
	    {
		XBell (XtDisplay (w), 100);
		return (TRUE);
	    }
	    w->dataclip.maxima[0] = x;
	}
	else if (!w->dataclip.last_was_left)
	{
	    XBell (XtDisplay (w), 100);
	    return (TRUE);
	}
	else if (w->dataclip.num_regions >= w->dataclip.maxRegions)
	{
	    XBell (XtDisplay (w), 100);
	    return (TRUE);
	}
	else if (x <= w->dataclip.left_pos)
	{
	    XBell (XtDisplay (w), 100);
	    return (TRUE);
	}
	else
	{
	    w->dataclip.minima[w->dataclip.num_regions] = w->dataclip.left_pos;
	    w->dataclip.maxima[w->dataclip.num_regions++] = x;
	    w->dataclip.last_was_left = FALSE;
	    w->dataclip.left_pos = TOOBIG;
	}
	if (w->dataclip.immediate_apply) apply = TRUE;
	break;
      default:
	return (FALSE);
	/*break;*/
    }
    canvas_resize (canvas, NULL, TRUE);
    if (apply)
    {
	XSync (XtDisplay (w), False);
	apply_cbk ( (Widget) NULL, (XtPointer) w, NULL );
    }
    return (TRUE);
#ifdef not_implemented
    sprintf (txt, "Lower Clip: %e", regions.minima[0]);
    XtVaSetValues (top->dataclip.lower_label,
		   XtNlabel, txt,
		   NULL);
    sprintf (txt, "Upper Clip: %e",
		    regions.maxima[regions.num_regions - 1]);
    XtVaSetValues (top->dataclip.upper_label,
		   XtNlabel, txt,
		   NULL);
#endif
}   /*  End Function histogram_canvas_position_func  */

static void size_control_func (KWorldCanvas canvas, int width, int height,
			       struct win_scale_type *win_scale,
			       void **info, flag *boundary_clear)
/*  This routine will modify the window scaling information for a world
    canvas. While this routine is running, colourmap resize events are
    ignored. Hence this routine may safely cause the associated Kcolourmap
    object to be resized.
    The canvas is given by  canvas  .
    The width of the canvas in pixels is given by  width  .
    The height of the canvas in pixels is given by  height  .
    The window scaling information is pointed to by  win_scale  .The data
    herein may be modified.
    The arbitrary canvas information pointer is pointed to by  info  .
    If the value TRUE is written to the storage pointed to by
    boundary_clear  then the  canvas_resize  routine will attempt to clear
    only the boundary between the pixel canvas and the world canvas. If
    the value FALSE is written here or nothing is written here, the
    canvas_resize  routine will clear the entire pixel canvas as
    appropriate.
    The routine should return nothing.
*/
{
    flag verbose;
    int array_len = width / 2;
    unsigned int count;
    unsigned long hpeak, hmode;
    double frac;
    double min, max, scale, offset;
    DataclipWidget w = (DataclipWidget) *info;
    char txt[STRING_LENGTH];
    unsigned long *hist_array;
    static char function_name[] = "DataclipWidget::size_control_func";

    verbose = w->dataclip.verbose;
    if (w->dataclip.array == NULL) return;
    if (w->dataclip.data_min >= TOOBIG)
    {
	/*  Recompute the minimum and maximum  */
	if ( !iarray_min_max (w->dataclip.array, CONV1_REAL, &min, &max) )
	{
	    fprintf (stderr, "Error getting image range\n");
	    return;
	}
	if (min >= max)
	{
	    w->dataclip.array = NULL;
	    return;
	}
	w->dataclip.data_min = min;
	w->dataclip.data_max = max;
	iarray_get_data_scaling (w->dataclip.array, &scale, &offset);
	sprintf (txt, "Data Minimum: %e", min * scale + offset);
	XtVaSetValues (w->dataclip.min_label,
		       XtNlabel, txt,
		       NULL);
	sprintf (txt, "Data Maximum: %e", max * scale + offset);
	XtVaSetValues (w->dataclip.max_label,
		       XtNlabel, txt,
		       NULL);
	win_scale->left_x = w->dataclip.data_min;
	win_scale->right_x = w->dataclip.data_max;
    }
    if (array_len == w->dataclip.hist_arr_length) return;
    if ( ( hist_array = (unsigned long *)
	   m_alloc_scratch (sizeof *hist_array * array_len, function_name) )
	 == NULL )
    {
	m_abort (function_name, "temporary array");
    }
    m_clear ( (char *) hist_array, sizeof *hist_array * array_len );
    if (array_len > w->dataclip.hist_buf_length)
    {
	/*  Increase histogram array size  */
	if (w->dataclip.histogram_array != NULL)
	{
	    m_free ( (char *) w->dataclip.histogram_array );
	    w->dataclip.hist_buf_length = 0;
	}
	if ( ( w->dataclip.histogram_array = (double *)
	       m_alloc (sizeof *w->dataclip.histogram_array * array_len) )
	     == NULL )
	{
	    m_abort (function_name, "histogram array");
	}
	w->dataclip.hist_buf_length = array_len;
    }
    m_clear ( (char *) w->dataclip.histogram_array,
	      sizeof *w->dataclip.histogram_array * array_len);
    w->dataclip.hist_arr_length = array_len;
    hpeak = 0;
    hmode = 0;
    if ( !iarray_compute_histogram (w->dataclip.array,
				    win_scale->conv_type,
				    win_scale->left_x, win_scale->right_x,
				    array_len, hist_array, &hpeak, &hmode) )
    {
	fprintf (stderr, "Error computing histogram\n");
	a_prog_bug (function_name);
    }
    if (verbose) fprintf (stderr, "histogram peak: %lu\n", hpeak);
    for (count = 0; count < array_len; ++count)
    {
	w->dataclip.histogram_array[count] = hist_array[count];
    }
    m_free_scratch ();
    /*  Have histogram data: setup logarithmic scale  */
    win_scale->top_y = hpeak;
    frac = log10 (win_scale->top_y);
    if (frac > 6.0) frac = 6.0;
    win_scale->bottom_y = win_scale->top_y / pow (10.0, frac);
    if (win_scale->bottom_y < 1.0) win_scale->bottom_y = 1.0;
    win_scale->x_offset = 0;
    win_scale->y_offset = 0;
    win_scale->x_pixels = width;
    win_scale->y_pixels = height;
}   /*  End Function size_control_func  */

static void pop_cbk (w, client_data, call_data)
/*  This is the generic popup/popdown callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    DataclipWidget top = (DataclipWidget) w;

    top->dataclip.popped_up = (iaddr) client_data;
}   /*  End Function pop_cbk   */

static void full_range_cbk (w, client_data, call_data)
/*  This is the generic popup/popdown callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    DataclipWidget top = (DataclipWidget) client_data;

    if (top->dataclip.array == NULL) return;
    top->dataclip.num_regions = 1;
    top->dataclip.minima[0] = top->dataclip.data_min;
    top->dataclip.maxima[0] = top->dataclip.data_max;
    kwin_resize (top->dataclip.pixcanvas, TRUE, 0, 0, 0, 0);
    XSync (XtDisplay (w), False);
    apply_cbk ( (Widget) NULL, (XtPointer) top, NULL );
}   /*  End Function full_range_cbk   */

static void iarr_destroy_callback (iarray arr, DataclipWidget top)
/*  [PURPOSE] This routine will register the destruction of the Intelligent
    Array.
    <arr> The Intelligent Array.
    <top> The Dataclip widget.
    [RETURNS] Nothing.
*/
{
    top->dataclip.array = NULL;
    top->dataclip.iarr_destroy_callback = NULL;
    if (top->dataclip.autoPopdown) XtPopdown ( (Widget) top );
}   /*  End Function iarr_destroy_callback  */

static void blank_data_cbk (Widget w, XtPointer client_data,
				 XtPointer call_data)
{
    flag bool = (uaddr) call_data;
    DataclipWidget top = (DataclipWidget) client_data;

    top->dataclip.blank_data = bool;
    apply_cbk ( (Widget) NULL, (XtPointer) top, NULL );
}   /*  End Function blank_data_cbk  */

static void type_menu_cbk (Widget w, XtPointer client_data,XtPointer call_data)
{
    unsigned int type;
    unsigned int index = *(int *) call_data;
    DataclipWidget top = (DataclipWidget) client_data;

    type = dataclipClassRec.dataclip_class.type_index_to_type[index];
    top->dataclip.output_type = type;
}   /*  End Function type_menu_cbk  */

static void save_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    iarray new, old;
    flag rescale = TRUE;
    unsigned int count, num_dim;
    Widget dialog;
    DataclipWidget top = (DataclipWidget) client_data;
    double inp_scale, inp_offset, min, max, scale, offset;
    char *arrayfile_name;
    unsigned long *dim_lengths;
    CONST char **dim_names;
    extern DataclipClassRec dataclipClassRec;
    static char function_name[] = "DataclipWidget::save_cbk";

    dialog = top->dataclip.dialog;
    if (top->dataclip.num_regions < 1)
    {
	XBell (XtDisplay (w), 100);
	return;
    }
    if (top->dataclip.num_regions > 1)
    {
	fprintf (stderr, "Multiple regions not supported yet\n");
	return;
    }
    arrayfile_name = XawDialogGetValueString (dialog);
    if (arrayfile_name == NULL) arrayfile_name = "scaled";
    if (strlen (arrayfile_name) < 1) arrayfile_name = "scaled";
    old = top->dataclip.array;
    num_dim = iarray_num_dim (old);
    if ( ( dim_names = (CONST char **) m_alloc (num_dim * sizeof *dim_names) )
	 == NULL )
    {
	m_abort (function_name, "array of dimension names pointers");
    }
    if ( ( dim_lengths = (unsigned long *)
	   m_alloc (num_dim * sizeof *dim_lengths) ) == NULL )
    {
	m_abort (function_name, "array of dimension lengths");
    }
    for (count = 0; count < num_dim; ++count)
    {
	dim_names[count] = iarray_dim_name (old, count);
	dim_lengths[count] = iarray_dim_length (old, count);
    }
    if ( ( new = iarray_create (top->dataclip.output_type, num_dim, dim_names,
				dim_lengths, iarray_value_name (old), old) )
	 == NULL )
    {
	m_error_notify (function_name, "output Intelligent Array");
	m_free ( (char *) dim_names );
	m_free ( (char *) dim_lengths );
	return;
    }
    m_free ( (char *) dim_names );
    m_free ( (char *) dim_lengths );
    for (count = 0; count < num_dim; ++count)
    {
	iarray_get_world_coords (old, count, &min, &max);
	iarray_set_world_coords (new, count, min, max);
    }
    /*  Determine desired new minimum and maximum range  */
    iarray_get_data_scaling (old, &inp_scale, &inp_offset);
    switch (top->dataclip.output_type)
    {
      case K_BYTE:
	min = -127.0;
	max = 127.0;
	break;
      case K_UBYTE:
	min = 0.0;
	max = 255.0;
	break;
      case K_SHORT:
	min = -32767.0;
	max = 32767.0;
	break;
      case K_USHORT:
	min = 0.0;
	max = 65535.0;
	break;
      case K_INT:
	min = -2147483647.0;
	max = 2147483647.0;
	break;
      case K_UINT:
	min = 0.0;
	max = 4294967296.0;
	break;
      default:
	rescale = FALSE;
	break;
    }
    if (rescale)
    {
	/*  Must fit the defined range of data into a range  */
	scale = (max - min) /
	    (top->dataclip.maxima[0] - top->dataclip.minima[0]);
	offset = min - scale * top->dataclip.minima[0];
	iarray_set_data_scaling (new, inp_scale / scale,
				 inp_offset - offset *inp_scale /scale);
    }
    else
    {
	scale = 1.0;
	offset = 0.0;
    }
    iarray_clip_scale_and_offset (new, old, scale, offset,
				  top->dataclip.minima[0],
				  top->dataclip.maxima[0],
				  top->dataclip.blank_data);
    if ( !iarray_write (new, arrayfile_name) )
    {
	iarray_dealloc (new);
	return;
    }
    iarray_dealloc (new);
    XBell (XtDisplay (w), -90);
}   /*  End Function save_cbk  */

static unsigned long get_colour (KWorldCanvas canvas, CONST char *colourname)
/*  [SUMMARY] Get colour.
    <canvas> The world canvas.
    <colourname> The name of the colour.
    [RETURNS] The specified colour if available, else a default colour.
*/
{
    KPixCanvas pixcanvas;
    unsigned long pixel_value;

    pixcanvas = canvas_get_pixcanvas (canvas);
    if ( kwin_get_colour (pixcanvas, colourname, &pixel_value,
			  NULL, NULL, NULL) )
    {
	return (pixel_value);
    }
    if ( kwin_get_colour (pixcanvas, "white", &pixel_value, NULL, NULL, NULL) )
    {
	fprintf (stderr,
		 "Error allocating colour: \"%s\" defaulting to white\n",
		 colourname);
	return (pixel_value);
    }
    fprintf (stderr, "Error allocating colour: \"%s\" defaulting to 0\n",
	     colourname);
    return (0);
}   /*  End Function get_colour  */

static void zoom_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [SUMMARY] Zoom callback.
    <w> The widget.
    <client_data> Client data pointer.
    <call_data> Call-specific data pointer.
    [RETURNS] Nothing.
*/
{
    double min, max, left, right;
    DataclipWidget top = (DataclipWidget) client_data;

    if (top->dataclip.array == NULL) return;
    if (top->dataclip.data_min >= TOOBIG) return;
    if (top->dataclip.num_regions < 1) return;
    canvas_get_attributes (top->dataclip.worldcanvas,
			   CANVAS_ATT_LEFT_X, &left,
			   CANVAS_ATT_RIGHT_X, &right,
			   CANVAS_ATT_END);
    min = top->dataclip.minima[0];
    max = top->dataclip.maxima[top->dataclip.num_regions - 1];
    if ( (min == left) && (max == right) ) return;
    canvas_set_attributes (top->dataclip.worldcanvas,
			   CANVAS_ATT_LEFT_X, min,
			   CANVAS_ATT_RIGHT_X, max,
			   CANVAS_ATT_END);
    /*  Force histogram to be recomputed  */
    top->dataclip.hist_arr_length = 0;
    kwin_resize (top->dataclip.pixcanvas, TRUE, 0, 0, 0, 0);
}   /*  End Function zoom_cbk   */

static void unzoom_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [SUMMARY] Unzoom callback.
    <w> The widget.
    <client_data> Client data pointer.
    <call_data> Call-specific data pointer.
    [RETURNS] Nothing.
*/
{
    double min, max, left, right;
    DataclipWidget top = (DataclipWidget) client_data;

    if (top->dataclip.array == NULL) return;
    if (top->dataclip.data_min >= TOOBIG) return;
    canvas_get_attributes (top->dataclip.worldcanvas,
			   CANVAS_ATT_LEFT_X, &left,
			   CANVAS_ATT_RIGHT_X, &right,
			   CANVAS_ATT_END);
    min = top->dataclip.data_min;
    max = top->dataclip.data_max;
    if ( (min == left) && (max == right) ) return;
    canvas_set_attributes (top->dataclip.worldcanvas,
			   CANVAS_ATT_LEFT_X, min,
			   CANVAS_ATT_RIGHT_X, max,
			   CANVAS_ATT_END);
    /*  Force histogram to be recomputed  */
    top->dataclip.hist_arr_length = 0;
    kwin_resize (top->dataclip.pixcanvas, TRUE, 0, 0, 0, 0);
}   /*  End Function unzoom_cbk   */

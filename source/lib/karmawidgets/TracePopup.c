/*LINTLIBRARY*/
/*  TracePopup.c

    This code provides a trace display widget for Xt.

    Copyright (C) 1996  Richard Gooch

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

/*  This file contains all routines needed for a trace display widget for Xt.


    Written by      Richard Gooch   15-SEP-1996

    Updated by      Richard Gooch   16-SEP-1996

    Updated by      Richard Gooch   19-SEP-1996: Changed to special function
  call interface for setting new array.

    Last updated by Richard Gooch   9-OCT-1996: Store trace details and use
  refresh function. Added print button.


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
#include <karma_iarray.h>
#include <karma_c.h>
#include <karma_xtmisc.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_m.h>
#include <karma_a.h>
#include <Xkw/TracePopupP.h>
#include <Xkw/Canvas.h>
#include <Xkw/ExclusiveMenu.h>
#include <Xkw/Postscript.h>


/*  Private functions  */

STATIC_FUNCTION (void TracePopup__Initialise,
		 (Widget request, Widget new) );
STATIC_FUNCTION (Boolean TracePopup__SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void iarr_destroy_callback,
		 (iarray arr,TracePopupWidget top) );
STATIC_FUNCTION (void TracePopup__canvas_realise_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize, void **info,
		  PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );
STATIC_FUNCTION (void postscript_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );


/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define offset(field) XtOffset(TracePopupWidget, tracePopup.field)
#define XkwRDouble "Double"

static XtResource TracePopupResources[] = 
{
    {XkwNiarray, XkwCIarray, XtRPointer, sizeof (XtPointer),
     offset (array), XtRImmediate, NULL},
    {XkwNverbose, XkwCVerbose, XtRBool, sizeof (Bool),
     offset (verbose), XtRImmediate, FALSE},
    {XkwNcanvasVisual, XtCVisual, XtRVisual, sizeof (Visual *),
     offset (canvasVisual), XtRImmediate, CopyFromParent},
    {XkwNworldCanvas, XkwCWorldCanvas, XtRPointer, sizeof (XtPointer),
     offset (worldCanvas), XtRImmediate, NULL},
    {XkwNrealiseCallback, XtCCallback, XtRCallback, sizeof (caddr_t),
     offset (realiseCallback), XtRCallback, (caddr_t) NULL},
    {XkwNkarmaColourmap, XkwCKarmaColourmap, XtRPointer, sizeof (XtPointer),
     offset (karmaColourmap), XtRImmediate, (caddr_t) NULL},
};

#undef offset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

TracePopupClassRec tracePopupClassRec = 
{
  {
      /* CoreClassPart */
    (WidgetClass)&topLevelShellClassRec,      /* superclass */
    "TracePopup",                             /* class_name */
    sizeof (TracePopupRec),                   /* widget_size */
    NULL,                                     /* class_initialise */
    NULL,                                     /* class_part_initialise */
    FALSE,                                    /* class_init */
    (XtInitProc) TracePopup__Initialise,      /* initialise */
    NULL,                                     /* initialise_hook */
    XtInheritRealize,                         /* realise */
    NULL,                                     /* actions */
    0,                                        /* num_actions */
    TracePopupResources,                      /* resources */
    XtNumber (TracePopupResources),           /* num_resources */
    NULLQUARK,                                /* xrm_class */
    TRUE,                                     /* compress_motion */
    TRUE,                                     /* compress_exposure */
    TRUE,                                     /* compress_enterleave */
    TRUE,                                     /* visible_interest */
    NULL,                                     /* destroy */
    XtInheritResize,                          /* resize */
    NULL,                                     /* expose */
    (XtSetValuesFunc) TracePopup__SetValues,  /* set_values */
    NULL,                                     /* set_values_hook */
    XtInheritSetValuesAlmost,                 /* set_values_almost */
    NULL,                                     /* get_values_hook */
    NULL,                                     /* accept_focus */
    XtVersion,                                /* version */
    NULL,                                     /* callback_private */
    NULL,                                     /* tm_translations */
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
  {  /* TracePopupClassPart */
      0                   /* empty */
  }
};

WidgetClass tracePopupWidgetClass = (WidgetClass) &tracePopupClassRec;


static void TracePopup__Initialise (Widget Request, Widget New)
{
    /*TracePopupWidget request = (TracePopupWidget) Request;*/
    TracePopupWidget new = (TracePopupWidget) New;
    Widget form, w, print_btn, cnv;
    /*static char function_name[] = "TracePopupWidget::Initialise";*/

    new->tracePopup.iarr_destroy_callback = NULL;
    new->tracePopup.y_arr = NULL;
    new->tracePopup.buf_len = 0;
    new->tracePopup.trace = NULL;
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
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
				 XtNlabel, "Print",
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNfromHoriz, w,
				 XtNheight, 20,
				 NULL);
    print_btn = w;
    w = XtVaCreateManagedWidget ("pseudoColourCanvas", canvasWidgetClass, form,
				 XtNfromVert, w,
				 XtNleft, XtChainLeft,
				 XtNright, XtChainRight,
				 XtNwidth, 400,
				 XtNheight, 300,
				 XtNvisual, new->tracePopup.canvasVisual,
				 XkwNsilenceUnconsumed, True,
				 XtNresizable, True,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainBottom,
				 NULL);
    cnv = w;
    XtAddCallback (cnv, XkwNrealiseCallback,
		   TracePopup__canvas_realise_cbk, (XtPointer) New);
    w = XtVaCreatePopupShell ("postscriptwinpopup", postscriptWidgetClass, New,
			      XtNtitle, "Postscript Window",
			      XkwNautoIncrement, TRUE,
			      NULL);
    new->tracePopup.pswinpopup = w;
    XtAddCallback (w, XtNcallback, postscript_cbk, New);
    XtAddCallback (print_btn, XtNcallback, xtmisc_popup_cbk, w);
}   /*  End Function Initialise  */

static Boolean TracePopup__SetValues (Widget Current, Widget Request,
				      Widget New)
{
    /*static char function_name[] = "TracePopupWidget::SetValues";*/

    return False;
}   /*  End Function SetValues  */

static void iarr_destroy_callback (iarray arr, TracePopupWidget top)
/*  [PURPOSE] This routine will register the destruction of the Intelligent
    Array.
    <arr> The Intelligent Array.
    <top> The TracePopup widget.
    [RETURNS] Nothing.
*/
{
    top->tracePopup.array = NULL;
    top->tracePopup.iarr_destroy_callback = NULL;
    top->tracePopup.trace = NULL;
}   /*  End Function iarr_destroy_callback  */

static void TracePopup__canvas_realise_cbk (Widget w, XtPointer client_data,
					    XtPointer call_data)
/*  [SUMMARY] Canvas realise callback.
    [PURPOSE] This routine is called when a Canvas widget is realised. A
    Kcolourmap object is created if required, then the KWorldCanvas is created.
    <w> The Canvas widget that has just been realised.
    <client_data> The TracePopup widget.
    <call_data> The pixel canvas(es).
    [RETURNS] Nothing.
*/
{
    KPixCanvas pixcanvas = (KPixCanvas) call_data;
    TracePopupWidget top = (TracePopupWidget) client_data;
    /*static char function_name[] = "TracePopupWidget::canvas_realise_cbk";*/

    top->tracePopup.worldCanvas = canvas_create (pixcanvas, NULL, NULL);
    XtVaGetValues (w,
		   XtNforeground, &top->tracePopup.canvas_foreground,
		   NULL);
    if (top->tracePopup.array != NULL)
    {
	canvas_set_attributes(top->tracePopup.worldCanvas,
			      CANVAS_ATT_BOTTOM_Y,
			      top->tracePopup.array_minimum,
			      CANVAS_ATT_TOP_Y, top->tracePopup.array_maximum,
			      CANVAS_ATT_END);
    }
    canvas_register_refresh_func (top->tracePopup.worldCanvas, refresh_func,
				  top);
    XtCallCallbacks ( (Widget) top, XkwNrealiseCallback,
		      (XtPointer) top->tracePopup.worldCanvas );
}   /*  End Function canvas_realise_cbk   */


/*  Public functions follow  */

void XkwTracePopupNewArray (Widget W, iarray array, double min, double max)
/*  [SUMMARY] Register new array.
    <W> The TracePopup widget.
    <array> The new array. This may be NULL.
    <min> The minimum data value in the array.
    <max> The maximum data value in the array. If this is less than <<min>>
    then the minimum and maximum values are computed.
    [RETURNS] Nothing.
*/
{
    TracePopupWidget w = (TracePopupWidget) W;
    static char function_name[] = "XkwTracePopupNewArray";

    /*  Clean up and old array  */
    if (w->tracePopup.iarr_destroy_callback != NULL)
    {
	c_unregister_callback (w->tracePopup.iarr_destroy_callback);
	w->tracePopup.iarr_destroy_callback = NULL;
	w->tracePopup.trace = NULL;
    }
    if (w->tracePopup.worldCanvas != NULL)
    {
	kwin_resize (canvas_get_pixcanvas (w->tracePopup.worldCanvas),
		     TRUE, 0, 0, 0, 0);
    }
    /*  Process new array  */
    if (array == NULL) return;
    w->tracePopup.array = array;
    if (iarray_type (w->tracePopup.array) != K_FLOAT)
    {
	fprintf (stderr, "Only floating point data supported\n");
	a_prog_bug (function_name);
    }
    if (max < min)
    {
	/*  Compute the minimum and maximum  */
	iarray_min_max (w->tracePopup.array, CONV_CtoR_REAL,
			&w->tracePopup.array_minimum,
			&w->tracePopup.array_maximum);
    }
    else
    {
	w->tracePopup.array_minimum = min;
	w->tracePopup.array_maximum = max;
    }
    w->tracePopup.iarr_destroy_callback = 
	iarray_register_destroy_func (w->tracePopup.array,
				      ( flag (*) () )iarr_destroy_callback,
				      w);
    if (w->tracePopup.worldCanvas != NULL)
    {
	canvas_set_attributes(w->tracePopup.worldCanvas,
			      CANVAS_ATT_BOTTOM_Y, w->tracePopup.array_minimum,
			      CANVAS_ATT_TOP_Y, w->tracePopup.array_maximum,
			      CANVAS_ATT_END);
    }
}   /*  End Function XkwTracePopupNewArray  */

void XkwTracePopupShowTrace (Widget W, unsigned int *dim_indices,uaddr *coords)
/*  [SUMMARY] Show a trace.
    <w> The widget.
    <dim_indices> The dimension indicies. The length of this array must be one
    less than the number of dimensions in the array.
    <coords> The coordinates of the trace. The length of this array must be one
    less than the number of dimensions in the array.
    [RETURNS] Nothing.
*/
{
    int trace_dim = -1;
    unsigned int count, dim_count, num_dim;
    TracePopupWidget w = (TracePopupWidget) W;
    CONST char *trace;
    static char function_name[] = "XkwTracePopupShowTrace";

    if (w->tracePopup.worldCanvas == NULL) return;
    if (w->tracePopup.array == NULL) return;
    num_dim = iarray_num_dim (w->tracePopup.array);
    /*  Compute offset to trace  */
    trace = w->tracePopup.array->data;
    for (count = 0; count < num_dim - 1; ++count)
    {
	dim_count = dim_indices[count];
	if (coords[count] >= w->tracePopup.array->lengths[dim_count])
	{
	    fprintf(stderr,
		    "Axis: %u  coordinate: %lu is not less than length: %lu\n",
		    dim_count, coords[count],
		    w->tracePopup.array->lengths[dim_count]);
	    return;
	}
	trace += w->tracePopup.array->offsets[dim_count][ coords[count] ];
    }
    w->tracePopup.trace = trace;
    /*  Find trace dimension  */
    for (dim_count = 0; (dim_count < num_dim) && (trace_dim < 0); ++dim_count)
    {
	trace_dim = dim_count;
	for (count = 0; count < num_dim - 1; ++count)
	{
	    if (dim_indices[count] == dim_count) trace_dim = -1;
	}
    }
    if (trace_dim < 0)
    {
	fprintf (stderr, "No trace dimension!\n");
	a_prog_bug (function_name);
    }
    w->tracePopup.trace_dim = trace_dim;
    kwin_resize (canvas_get_pixcanvas (w->tracePopup.worldCanvas),
		 TRUE, 0, 0, 0, 0);
}   /*  End Function XkwTracePopupShowTrace  */

static void refresh_func (KWorldCanvas canvas, int width, int height,
			  struct win_scale_type *win_scale,
			  Kcolourmap cmap, flag cmap_resize, void **info,
			  PostScriptPage pspage,
			  unsigned int num_areas,
			  KPixCanvasRefreshArea *areas,
			  flag *honoured_areas)
/*  [SUMMARY] Refresh event callback.
    [PURPOSE] This routine is a refresh event consumer for a world canvas.
    <canvas> The world canvas being refreshed.
    <width> The width of the canvas in pixels.
    <height> The height of the canvas in pixels.
    <win_scale> A pointer to the window scaling information.
    <cmap> The colourmap associated with the canvas.
    <cmap_resize> TRUE if the refresh function was called as a result of a
    colourmap resize, else FALSE.
    <info> A pointer to the arbitrary canvas information pointer.
    <pspage> If not NULL, the PostScriptPage object the refresh is
    redirected to.
    <num_areas> The number of areas that need to be refreshed. If this is
    0 then the entire pixel canvas needs to be refreshed.
    <areas> The list of areas that need to be refreshed. Note that these
    areas are given in pixel co-ordinates.
    <honoured_areas> If the value TRUE is written here it is assumed the
    routine honoured the list of refresh areas and did not write outside
    these areas and hence the list of areas will be passed to subsequent
    registered refresh routines. If FALSE is written here (or nothing is
    written here), implying the routine refreshed the entire canvas,
    subsequent refresh routines will be told to refresh the entire canvas.
    [RETURNS] Nothing.
*/
{
    unsigned int count, length, num_pixels;
    unsigned long pixel_value, *pixel_values;
    double x0, y0, x1, y1, left_x, right_x, xscale;
    TracePopupWidget w = (TracePopupWidget) *info;
    uaddr *offsets;
    static char function_name[] = "TracePopupWidget::refresh_func";

    if (w->tracePopup.worldCanvas == NULL) return;
    if (w->tracePopup.array == NULL) return;
    if (w->tracePopup.trace == NULL) return;
    /*  Draw zero line  */
    if ( !canvas_get_colour (w->tracePopup.worldCanvas,
			     (pspage == NULL) ? "yellow" : "black",
			     &pixel_value, NULL, NULL, NULL) )
    {
	pixel_value = w->tracePopup.canvas_foreground;
    }
    canvas_draw_line_p (w->tracePopup.worldCanvas, 0.0, 0.0, 1.0, 0.0,
			pixel_value);
    length = iarray_dim_length (w->tracePopup.array, w->tracePopup.trace_dim);
    offsets = w->tracePopup.array->offsets[w->tracePopup.trace_dim];
    if ( (w->tracePopup.karmaColourmap != NULL) && (pspage == NULL) )
    {
	/*  Use pixels from supplied colourmap  */
	num_pixels = kcmap_get_pixels (w->tracePopup.karmaColourmap,
				       &pixel_values);
	canvas_get_attributes (w->tracePopup.worldCanvas,
			       CANVAS_ATT_LEFT_X, &left_x,
			       CANVAS_ATT_RIGHT_X, &right_x,
			       CANVAS_ATT_END);
	xscale = (right_x - left_x) / (double) (length - 1);
	/*  Now draw each line  */
	for (count = 0; count < length - 1; ++count)
	{
	    y0 = *(float *) (w->tracePopup.trace + offsets[count]);
	    y1 = *(float *) (w->tracePopup.trace + offsets[count + 1]);
	    if ( (y0 >= TOOBIG) || (y1 >= TOOBIG) ) continue;
	    x0 = left_x + (double) count * xscale;
	    x1 = left_x + (double) (count + 1) * xscale;
	    pixel_value = pixel_values[count * (num_pixels - 1) / (length -1)];
	    canvas_draw_line_p (w->tracePopup.worldCanvas, x0, y0, x1, y1,
				pixel_value);
	}
	return;
    }
    /*  Ensure buffer is big enough  */
    if (w->tracePopup.buf_len < length)
    {
	if (w->tracePopup.y_arr != NULL) m_free ( (char *)w->tracePopup.y_arr);
	if ( ( w->tracePopup.y_arr = (double *)
	       m_alloc (length * sizeof *w->tracePopup.y_arr) ) == NULL )
	{
	    m_abort (function_name, "y array");
	}
	w->tracePopup.buf_len = length;
    }
    /*  Copy into buffer  */
    for (count = 0; count < length; ++count)
    {
	w->tracePopup.y_arr[count] = *(float *) (w->tracePopup.trace +
						 offsets[count]);
    }
    if (pspage == NULL) pixel_value = w->tracePopup.canvas_foreground;
    else canvas_get_colour (w->tracePopup.worldCanvas, "black",
			    &pixel_value, NULL, NULL, NULL);
    canvas_draw_lines_p (w->tracePopup.worldCanvas, NULL, w->tracePopup.y_arr,
			 length, pixel_value);
}   /*  End Function refresh_func  */

static void postscript_cbk (Widget w, XtPointer client_data,
			    XtPointer call_data)
/*  [SUMMARY] PostScript callback.
    [PURPOSE] This is the PostScript callback. It is called when the PostScript
    widget needs to know the pixel canvas and image name.
*/
{
    KPixCanvas pixcanvas;
    TracePopupWidget top = (TracePopupWidget) client_data;

    pixcanvas = canvas_get_pixcanvas (top->tracePopup.worldCanvas);
    XkwPostscriptRegisterImageAndName (w, pixcanvas, NULL);
}   /*  End Function postscript_cbk   */

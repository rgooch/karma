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

    Updated by      Richard Gooch   9-OCT-1996: Store trace details and use
  refresh function. Added print button.

    Updated by      Richard Gooch   5-NOV-1996: Added track display.

    Updated by      Richard Gooch   6-NOV-1996: Added horizontal pixel
  co-ordinate in track display and put both track dislays on their own line.

    Last updated by Richard Gooch   11-NOV-1996: Only refresh canvas if visible


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
#include <k_event_codes.h>
#include <karma_iarray.h>
#include <karma_xtmisc.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_m.h>
#include <karma_a.h>
#include <karma_c.h>
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
STATIC_FUNCTION (flag position_func,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info,
		  void **f_info, double x_lin, double y_lin) );


/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define offset(field) XtOffset(TracePopupWidget, tracePopup.field)
#define XkwRDouble "Double"

static XtResource TracePopupResources[] = 
{
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
    new->tracePopup.ap = NULL;
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
    w = XtVaCreateManagedWidget ("trackLabel0", labelWidgetClass, form,
				 XtNlabel,
				 "Track Output                               ",
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNfromVert,  w,
				 XtNheight, 20,
				 NULL);
    new->tracePopup.track_label0 = w;
    w = XtVaCreateManagedWidget ("trackLabel1", labelWidgetClass, form,
				 XtNlabel,
				 "Track Output                               ",
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNfromVert,  w,
				 XtNheight, 20,
				 NULL);
    new->tracePopup.track_label1 = w;
    w = XtVaCreateManagedWidget ("pseudoColourCanvas", canvasWidgetClass, form,
				 XtNfromVert, w,
				 XtNleft, XtChainLeft,
				 XtNright, XtChainRight,
				 XtNwidth, 400,
				 XtNheight, 280,
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
    if (top->tracePopup.ap != NULL) wcs_astro_destroy (top->tracePopup.ap);
    top->tracePopup.ap = NULL;
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
    canvas_use_astro_transform (top->tracePopup.worldCanvas,
				&top->tracePopup.ap);
    XtVaGetValues (w,
		   XtNforeground, &top->tracePopup.canvas_foreground,
		   NULL);
    if (top->tracePopup.array != NULL)
    {
	canvas_set_attributes (top->tracePopup.worldCanvas,
			       CANVAS_ATT_BOTTOM_Y,
			       top->tracePopup.array_minimum,
			       CANVAS_ATT_TOP_Y, top->tracePopup.array_maximum,
			       CANVAS_ATT_END);
    }
    canvas_register_refresh_func (top->tracePopup.worldCanvas, refresh_func,
				  top);
    canvas_register_position_event_func (top->tracePopup.worldCanvas,
					 position_func, top);
    XtCallCallbacks ( (Widget) top, XkwNrealiseCallback,
		      (XtPointer) top->tracePopup.worldCanvas );
}   /*  End Function canvas_realise_cbk   */

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
    double wlx, wrx, dummy;
    TracePopupWidget w = (TracePopupWidget) *info;
    uaddr *offsets;
    static char function_name[] = "TracePopupWidget::refresh_func";

    if (w->tracePopup.worldCanvas == NULL) return;
    if (w->tracePopup.array == NULL) return;
    if (w->tracePopup.trace == NULL) return;
    canvas_get_attributes (w->tracePopup.worldCanvas,
			   CANVAS_ATT_LEFT_X, &left_x,
			   CANVAS_ATT_RIGHT_X, &right_x,
			   CANVAS_ATT_END);
    /*  Convert left&right x values to non-linear co-ordinates  */
    wlx = left_x;
    dummy = 0.0;
    canvas_coords_transform (w->tracePopup.worldCanvas, 1,
			     &wlx, FALSE, &dummy, FALSE);
    wrx = right_x;
    dummy = 0.0;
    canvas_coords_transform (w->tracePopup.worldCanvas, 1,
			     &wrx, FALSE, &dummy, FALSE);
    /*  Draw zero line  */
    if ( !canvas_get_colour (w->tracePopup.worldCanvas,
			     (pspage == NULL) ? "yellow" : "black",
			     &pixel_value, NULL, NULL, NULL) )
    {
	pixel_value = w->tracePopup.canvas_foreground;
    }
    canvas_draw_line_p (w->tracePopup.worldCanvas, wlx, 0.0, wrx, 0.0,
			pixel_value);
    length = iarray_dim_length (w->tracePopup.array, w->tracePopup.trace_dim);
    offsets = w->tracePopup.array->offsets[w->tracePopup.trace_dim];
    if ( (w->tracePopup.karmaColourmap != NULL) && (pspage == NULL) )
    {
	/*  Use pixels from supplied colourmap  */
	num_pixels = kcmap_get_pixels (w->tracePopup.karmaColourmap,
				       &pixel_values);
	xscale = (wrx - wlx) / (double) (length - 1);
	/*  Now draw each line  */
	for (count = 0; count < length - 1; ++count)
	{
	    y0 = *(float *) (w->tracePopup.trace + offsets[count]);
	    y1 = *(float *) (w->tracePopup.trace + offsets[count + 1]);
	    if ( (y0 >= TOOBIG) || (y1 >= TOOBIG) ) continue;
	    x0 = wlx + (double) count * xscale;
	    x1 = wlx + (double) (count + 1) * xscale;
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

static flag position_func (KWorldCanvas canvas, double x, double y,
			   unsigned int event_code, void *e_info,
			   void **f_info, double x_lin, double y_lin)
/*  [SUMMARY] Position event callback.
    [PURPOSE] This routine is a position event consumer for a world canvas.
    <canvas> The canvas on which the event occurred.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    <event_code> The arbitrary event code.
    <e_info> A pointer to arbitrary event information.
    <f_info> A pointer to an arbitrary function information pointer.
    <x_lin> The horizontal linear world co-ordinate prior to the transform
    function being called.
    <y_lin> The vertical linear world co-ordinate prior to the transform
    function being called.
    [NOTE] The world co-ordinate values will have been transformed by the
    registered transform function (see [<canvas_register_transform_func>]).
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    unsigned long px;
    double unit_scale;
    TracePopupWidget w = (TracePopupWidget) *f_info;
    dim_desc *dim;
    char txt[STRING_LENGTH], unit_string[STRING_LENGTH];

    if (event_code != K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    if (w->tracePopup.trace == NULL) return (FALSE);
    if (w->tracePopup.array == NULL) return (FALSE);
    /*  First display the x co-ordinate information  */
    dim = iarray_get_dim_desc (w->tracePopup.array, w->tracePopup.trace_dim);
    /*  Convert linear world co-ordinates to array indices and display  */
    px = ds_get_coord_num (dim, x_lin, SEARCH_BIAS_CLOSEST);
    if (w->tracePopup.ap == NULL)
    {
	ds_format_unit (unit_string, &unit_scale, dim->name);
	sprintf (txt, "x: %lu pixel  %e %s", px, x * unit_scale, unit_string);
    }
    else
    {
	wcs_astro_format (w->tracePopup.ap, dim->name, unit_string, x);
	sprintf (txt, "x: %lu pixel  %s", px, unit_string);
    }
    XtVaSetValues (w->tracePopup.track_label0,
		   XtNlabel, txt,
		   NULL);
    /*  Now display the y co-ordinate information  */
    iarray_format_value (w->tracePopup.array, txt, y, TOOBIG, TOOBIG);
    XtVaSetValues (w->tracePopup.track_label1,
		   XtNlabel, txt,
		   NULL);
    return (TRUE);
}   /*  End Function position_func  */


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

    /*  Clean up any old array  */
    if (w->tracePopup.iarr_destroy_callback != NULL)
    {
	c_unregister_callback (w->tracePopup.iarr_destroy_callback);
	w->tracePopup.iarr_destroy_callback = NULL;
	w->tracePopup.trace = NULL;
    }
    if (w->tracePopup.ap != NULL) wcs_astro_destroy (w->tracePopup.ap);
    w->tracePopup.ap = NULL;
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
    w->tracePopup.ap = wcs_astro_setup (array->top_pack_desc,
					*array->top_packet);
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
    <W> The widget.
    <dim_indices> The dimension indicies. The length of this array must be one
    less than the number of dimensions in the array.
    <coords> The coordinates of the trace. The length of this array must be one
    less than the number of dimensions in the array.
    [RETURNS] Nothing.
*/
{
    KPixCanvas pixcanvas;
    flag visible;
    int trace_dim = -1;
    unsigned int count, dim_count, num_dim;
    TracePopupWidget w = (TracePopupWidget) W;
    dim_desc *dim;
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
    dim = iarray_get_dim_desc (w->tracePopup.array, trace_dim);
    if (w->tracePopup.worldCanvas != NULL)
    {
	canvas_set_attributes (w->tracePopup.worldCanvas,
			       CANVAS_ATT_LEFT_X, dim->first_coord,
			       CANVAS_ATT_RIGHT_X, dim->last_coord,
			       CANVAS_ATT_END);
    }
    canvas_specify (w->tracePopup.worldCanvas, dim->name,
		    iarray_value_name (w->tracePopup.array), 0, NULL, NULL);
    pixcanvas = canvas_get_pixcanvas (w->tracePopup.worldCanvas);
    kwin_get_attributes (pixcanvas,
			 KWIN_ATT_VISIBLE, &visible,
			 KWIN_ATT_END);
    if (visible) kwin_resize (pixcanvas, TRUE, 0, 0, 0, 0);
}   /*  End Function XkwTracePopupShowTrace  */

/*LINTLIBRARY*/
/*  SimpleColourbar.c

    This code provides a simple colourbar widget for Xt.

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

/*

    This file contains all routines needed for a simple colourbar widget for Xt


    Written by      Richard Gooch   21-APR-1996

    Updated by      Richard Gooch   28-APR-1996: Added mask* resources.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   6-NOV-1996: Cosmetic changes.


*/

#include <stdio.h>
#include <math.h>
#define X11
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Toggle.h>
#include <karma.h>
#include <k_event_codes.h>
#include <karma_xc.h>
#include <karma_m.h>
#include <Xkw/SimpleColourbarP.h>
#include <Xkw/Canvas.h>
#include <Xkw/Value.h>

STATIC_FUNCTION (void SimpleColourbar__Initialise,
		 (Widget request, Widget new) );
STATIC_FUNCTION (Boolean SimpleColourbar__SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void canvas_realise_cbk, (Widget w, XtPointer client_data,
					   XtPointer call_data) );
STATIC_FUNCTION (void canvas_refresh_func,
		 (KPixCanvas canvas, int width, int height,
		  void **info, PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );
STATIC_FUNCTION (void cmap_resize_func, (Kcolourmap cmap, void **info) );


#define offset(field) XtOffsetOf(SimpleColourbarRec, simpleColourbar.field)

static XtResource resources[] =
{
    {XtNvisual, XtCVisual, XtRVisual, sizeof (Visual *),
     offset (visual), XtRImmediate, CopyFromParent},
    {XkwNkarmaColourmap, XkwCKarmaColourmap, XtRPointer, sizeof (XtPointer),
     offset (karmaCmap), XtRImmediate, NULL},
    {XkwNmaskRed, XkwCMaskColour, XtRPointer, sizeof (Bool),
     offset (maskRed), XtRImmediate, False},
    {XkwNmaskGreen, XkwCMaskColour, XtRPointer, sizeof (Bool),
     offset (maskGreen), XtRImmediate, False},
    {XkwNmaskBlue, XkwCMaskColour, XtRPointer, sizeof (Bool),
     offset (maskBlue), XtRImmediate, False},
#undef offset
};


SimpleColourbarClassRec simpleColourbarClassRec =
{
    {
	/* core fields  */
	(WidgetClass) &compositeClassRec,    /*  superclass             */
	"SimpleColourbar",              /*  class_name             */
	sizeof (SimpleColourbarRec),    /*  widget_size            */
	NULL,                           /*  class_initialise       */
	NULL,                           /*  class_part_initialise  */
	FALSE,                          /*  class_inited           */
	(XtInitProc) SimpleColourbar__Initialise, /*  initialise             */
	NULL,                           /*  initialise_hook        */
	XtInheritRealize,               /*  realise                */
	NULL,                           /*  actions                */
	0,                              /*  num_actions            */
	resources,                      /*  resources              */
	XtNumber (resources),           /*  num_resources          */
	NULLQUARK,                      /*  xrm_class              */
	TRUE,                           /*  compress_motion        */
	TRUE,                           /*  compress_exposure      */
	TRUE,                           /*  compress_enterleave    */
	TRUE,                           /*  visible_interest       */
	NULL,                           /*  destroy                */
	XtInheritResize,                /*  resize                 */
	NULL,                           /*  expose                 */
	(XtSetValuesFunc) SimpleColourbar__SetValues,/*  set_values*/
	NULL,                           /*  set_values_hook        */
	XtInheritSetValuesAlmost,       /*  set_values_almost      */
	NULL,                           /*  get_values_hook        */
	NULL,                           /*  accept_focus           */
	XtVersion,                      /*  version                */
	NULL,                           /*  callback_private       */
	NULL,                           /*  tm_table               */
	XtInheritQueryGeometry,         /*  query_geometry         */
	XtInheritDisplayAccelerator,    /*  display_accelerator    */
	NULL                            /*  extension              */
    },
    {     /* CompositeClassPart */
	XtInheritGeometryManager        /* geometry_manager        */,
	XtInheritChangeManaged          /* change_managed          */,
	XtInheritInsertChild            /* insert_child            */,
	XtInheritDeleteChild            /* delete_child            */,
	NULL                            /* extension               */
    },
    {
	/*  simpleColourbar fields */
	0                               /*  ignore                 */
    }
};

WidgetClass simpleColourbarWidgetClass = (WidgetClass)&simpleColourbarClassRec;

static void SimpleColourbar__Initialise (Widget Request, Widget New)
{
    Kdisplay dpy_handle;
    Colormap xcmap;
    /*SimpleColourbarWidget request = (SimpleColourbarWidget) Request;*/
    SimpleColourbarWidget new = (SimpleColourbarWidget) New;
    Widget canvas;
    /*static char function_name[] = "SimpleColourbarWidget::Initialise";*/

    new->simpleColourbar.valid_cache = FALSE;
    new->simpleColourbar.cache_width = 0;
    new->simpleColourbar.row_cache = NULL;
    kcmap_get_attributes (new->simpleColourbar.karmaCmap,
			  KCMAP_ATT_DPY_HANDLE, &dpy_handle,
			  NULL);
    xcmap = xc_get_cmap (dpy_handle);
    canvas = XtVaCreateManagedWidget ("canvas", canvasWidgetClass, New,
				      XtNbackground, 0,
				      XtNwidth, new->core.width,
				      XtNheight, new->core.height,
				      XtNborderWidth, 0,
				      XtNvisual, new->simpleColourbar.visual,
				      XtNcolormap, xcmap,
				      XkwNsilenceUnconsumed, TRUE,
				      NULL);
    XtAddCallback (canvas, XkwNrealiseCallback, canvas_realise_cbk,
		   (XtPointer) New);
}   /*  End Function Initialise  */

static Boolean SimpleColourbar__SetValues (Widget Current, Widget Request,
					   Widget New)
{
    /*SimpleColourbarWidget current = (SimpleColourbarWidget) Current;
      SimpleColourbarWidget new = (SimpleColourbarWidget) New;*/
    /*static char function_name[] = "SimpleColourbarWidget::SetValues";*/

    return False;
}   /*  End Function SetValues  */

static void canvas_realise_cbk (w, client_data, call_data)
/*  This is the canvas realise callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    KPixCanvas canvas;
    SimpleColourbarWidget top = (SimpleColourbarWidget) client_data;
   /*static char function_name[]="SimpleColourbarWidget::canvas_realise_cbk";*/

    XtVaGetValues (w,
		   XkwNmonoPixCanvas, &canvas,
		   NULL);
    top->simpleColourbar.canvas = canvas;
    kwin_register_refresh_func (canvas, canvas_refresh_func, top);
    kcmap_register_resize_func (top->simpleColourbar.karmaCmap,
				cmap_resize_func, top);
}   /*  End Function canvas_realise_cbk   */

static void canvas_refresh_func (KPixCanvas canvas, int width, int height,
				 void **info, PostScriptPage pspage,
				 unsigned int num_areas,
				 KPixCanvasRefreshArea *areas,
				 flag *honoured_areas)
/*  [SUMMARY] Process a refresh event for a pixel canvas.
    <canvas> The pixel canvas.
    <width> The width of the canvas in pixels.
    <height> The height of the canvas in pixels.
    <info> A pointer to the arbitrary canvas information pointer.
    <pspage> If not NULL, the PostScriptPage object the refresh is
    redirected to.
    <num_areas> The number of areas that need to be refreshed. If this is
    0 then the entire pixel canvas needs to be refreshed.
    <areas> The list of areas that need to be refreshed.
    <honoured_areas> If the value TRUE is written here it is assumed the
    routine honoured the list of refresh areas and did not write outside
    these areas and hence the list of areas will be passed to subsequent
    registered refresh routines. If FALSE is written here (or nothing is
    written here), implying the routine refreshed the entire pixel canvas,
    subsequent refresh routines will be told to refresh the entire canvas.
    [RETURNS] Nothing.
*/
{
    int count, index;
    unsigned int num_pixels, visual_type;
    unsigned long red_mask, green_mask, blue_mask, pixel_mask;
    uaddr voffset = 0;
    unsigned long *pixel_values;
    SimpleColourbarWidget top = (SimpleColourbarWidget) *info;
    static int buf_size = 0;
    static int *array = NULL;
    static uaddr *hoffsets = NULL;
    static char function_name[] = "SimpleColourbarWidget::canvas_refresh_func";

    kwin_get_attributes (canvas,
			 KWIN_ATT_VISUAL, &visual_type,
			 KWIN_ATT_END);
    pixel_mask = 0;
    if (visual_type != KWIN_VISUAL_PSEUDOCOLOUR)
    {
	kwin_get_attributes (canvas,
			     KWIN_ATT_PIX_RED_MASK, &red_mask,
			     KWIN_ATT_PIX_GREEN_MASK, &green_mask,
			     KWIN_ATT_PIX_BLUE_MASK, &blue_mask,
			     KWIN_ATT_END);
	if (top->simpleColourbar.maskRed) pixel_mask |= red_mask;
	if (top->simpleColourbar.maskGreen) pixel_mask |= green_mask;
	if (top->simpleColourbar.maskBlue) pixel_mask |= blue_mask;
    }
    if ( (visual_type != KWIN_VISUAL_PSEUDOCOLOUR) || (pixel_mask != 0) )
    {
	/*  Cannot use image dump facility for drawing pixels  */
	pixel_mask = ~pixel_mask;
	num_pixels = kcmap_get_pixels (top->simpleColourbar.karmaCmap,
				       &pixel_values);
	for (count = 0; count < width; ++count)
	{
	    index = count * (int) (num_pixels - 1) / (width - 1);
	    kwin_draw_line (canvas, count, 0, count, height - 1,
				   pixel_values[index] & pixel_mask);
	}
	return;
    }
    if (buf_size < width)
    {
	/*  Need to expand buffers  */
	if (array != NULL) m_free ( (char *) array );
	if (hoffsets != NULL) m_free ( (char *) hoffsets );
	if ( ( array = (int *) m_alloc (width * sizeof *array) ) == NULL )
	{
	    m_abort (function_name, "values array");
	}
	if ( ( hoffsets = (uaddr *) m_alloc (width * sizeof *hoffsets) )
	     == NULL )
	{
	    m_abort (function_name, "hoffsets array");
	}
	buf_size = width;
	top->simpleColourbar.valid_cache = FALSE;
    }
    if (width != top->simpleColourbar.cache_width)
    {
	top->simpleColourbar.valid_cache = FALSE;
    }
    if (top->simpleColourbar.valid_cache)
    {
	if ( !kwin_draw_cached_image (top->simpleColourbar.row_cache, 0, 0) )
	{
	    fprintf (stderr, "Error drawing cached first row of colourbar\n");
	}
    }
    else
    {
	/*  Need to rebuild row cache  */
	num_pixels = kcmap_get_pixels (top->simpleColourbar.karmaCmap,
				       &pixel_values);
	for (count = 0; count < width; ++count)
	{
	    index = count * (int) (num_pixels - 1) / (width - 1);
	    array[count] = index;
	    hoffsets[count] = count * sizeof *array;
	}
	if ( !kwin_draw_pc_image (canvas, 0, 0, width, 1, (CONST char *) array,
				  hoffsets, &voffset, width, 1,
				  K_INT, CONV_CtoR_REAL,
				  num_pixels, pixel_values,
				  0, 0, 0,
				  (double) 0.0, (double) (num_pixels - 1),
				  ( flag (*) () ) NULL, NULL,
				  &top->simpleColourbar.row_cache) )
	{
	    fprintf (stderr, "Error drawing top row of colourbar\n");
	}
	top->simpleColourbar.valid_cache = TRUE;
	top->simpleColourbar.cache_width = width;
    }
    /*  Draw rest of colourbar using cached image  */
    for (count = 1; count < height; ++count)
    {
	if ( !kwin_draw_cached_image (top->simpleColourbar.row_cache,
				      0, count) )
	{
	    fprintf (stderr, "Error drawing cached row of colourbar\n");
	}
    }
}   /*  End Function canvas_refresh_func  */

static void cmap_resize_func (Kcolourmap cmap, void **info)
/*  [SUMMARY] This routine registers a change in the size of a colourmap.
    <cmap> The colourmap.
    <info> A pointer to the arbitrary colourmap information pointer.
    [RETURNS] Nothing.
*/
{
    SimpleColourbarWidget top = (SimpleColourbarWidget) *info;

    top->simpleColourbar.valid_cache = FALSE;
    kwin_resize (top->simpleColourbar.canvas, FALSE, 0, 0, 0, 0);
}  /*  End Function cmap_resize_func  */

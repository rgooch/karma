/*LINTLIBRARY*/
/*  ThreeDeeSlice.c

    This code provides a 3D slicer widget for Xt.

    Copyright (C) 1995-1996  Richard Gooch

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

    This file contains all routines needed for a 3D slicer widget for Xt.


    Written by      Richard Gooch   30-JUL-1995

    Updated by      Richard Gooch   31-JUL-1995: Improved event trapping when
  no cube or ViewableImages exist.

    Updated by      Richard Gooch   22-AUG-1995: Fixed bug in <Initialise> and
  <SetValues> which did not initialise <iarr_destroy_func> member properly.

    Updated by      Richard Gooch   31-AUG-1995: Fixed bug in <Initialise>
  which did not initialise <x_pos>, <y_pos> and <z_pos> members properly.

    Updated by      Richard Gooch   5-SEP-1995: Fixed bug with small windows in
  <register_new_cube> which did not sequence operations nicely.

    Updated by      Richard Gooch   26-SEP-1995: Fixed bug in <zy_pos_consumer>
  which flipped the computed Z value, resulting in the wrong XY-plane to be
  selected when left clicking in the ZX-plane.

    Updated by      Richard Gooch   8-OCT-1995: Fixed bug in
  <iarr_destroy_func> which did not clear <iarr_destroy_func> member.

    Updated by      Richard Gooch   28-OCT-1995: Extract X11 colourmap from
  Kcolourmap and pass to canvas. Also pass visual to canvas widget.

    Updated by      Richard Gooch   10-DEC-1995: Fixed bug in
  <iarr_destroy_func> which set array to NULL before freeing other data.

    Updated by      Richard Gooch   28-DEC-1995: Added verbose resource.

    Updated by      Richard Gooch   7-JAN-1996: Added cursorCallback resource.

    Updated by      Richard Gooch   22-FEB-1996: Perform partial refreshes
  where possible so as to reduce refresh times.

    Updated by      Richard Gooch   23-FEB-1996: Fixed bug in partial refreshes
  where area went past world canvas boundary, and hence past image boundary.

    Updated by      Richard Gooch   29-FEB-1996: Created <move_cursor> routine
  and added XkwNcursorPosition resource.

    Updated by      Richard Gooch   21-APR-1996: Swapped ZY co-ordinate system
  so that -Z is left and +Z is right: previously the co-ordinates were such
  that -Z was right and +Z was left, but of course the images themselves were
  not swapped.

    Updated by      Richard Gooch   29-APR-1996: Adapted canvas size depending
  on height of screen.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   4-JUN-1996: Switched to left-right
  bottom-top co-ordinate specification instead of min-max x and y.

    Updated by      Richard Gooch   21-JUL-1996: Changed to
  <canvas_convert_from_canvas_coords>.

    Updated by      Richard Gooch   9-NOV-1996: Added track labels and
  supported astronomical co-ordinates. Removed XkwNcursorPosition resource.
  Changed call_data for XkwNcursorCallback callback. Created
  <XkwThreeDeeSliceSetCursor>.

    Last updated by Richard Gooch   10-NOV-1996: Switched to
  <viewimg_set_array_attributes>


*/

#include <stdio.h>
#include <string.h>
#include <math.h>
#define X11
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Toggle.h>
#include <karma.h>
#define NEW_WIN_SCALE
#include <k_event_codes.h>
#include <karma_viewimg.h>
#include <karma_iarray.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_xc.h>
#include <karma_m.h>
#include <karma_a.h>
#include <Xkw/ThreeDeeSliceP.h>
#include <Xkw/Canvas.h>
#include <Xkw/Value.h>


STATIC_FUNCTION (void ThreeDeeSlice__Initialise,
		 (Widget request, Widget new) );
STATIC_FUNCTION (Boolean ThreeDeeSlice__SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void realise_cbk, (Widget w, XtPointer client_data,
				    XtPointer call_data) );
STATIC_FUNCTION (void redisplay_cbk, (Widget w, XtPointer client_data,
				      XtPointer call_data) );
STATIC_FUNCTION (void parent_refresh_func,
		 (KPixCanvas canvas, int width, int height,
		  void **info, PostScriptPage pspage) );
STATIC_FUNCTION (void xy_canvas_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize, void **info) );
STATIC_FUNCTION (void xz_canvas_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize, void **info) );
STATIC_FUNCTION (void zy_canvas_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize, void **info) );
STATIC_FUNCTION (void move_cursor,
		 (ThreeDeeSliceWidget top,
		  struct XkwThreeDeeSliceCursor new_cursor) );
STATIC_FUNCTION (void refresh_canvas,
		 (KWorldCanvas canvas, double lx, double ly) );
STATIC_FUNCTION (flag xy_pos_consumer,
		 (ViewableImage vimage, double x, double y,
		  void *value, unsigned int event_code,
		  void *e_info, void **f_info, double x_lin, double y_lin,
		  unsigned int value_type) );
STATIC_FUNCTION (flag xz_pos_consumer,
		 (ViewableImage vimage, double x, double y,
		  void *value, unsigned int event_code,
		  void *e_info, void **f_info, double x_lin, double y_lin,
		  unsigned int value_type) );
STATIC_FUNCTION (flag zy_pos_consumer,
		 (ViewableImage vimage, double x, double y,
		  void *value, unsigned int event_code,
		  void *e_info, void **f_info, double x_lin, double y_lin,
		  unsigned int value_type) );
STATIC_FUNCTION (void fillin_cursor,
		 (ThreeDeeSliceWidget top,
		  XkwThreeDeeSliceCallbackData cursor,
		  unsigned int type, double x, double y, double z) );
STATIC_FUNCTION (void register_new_cube, (ThreeDeeSliceWidget top, iarray cube,
					  double min, double max) );
STATIC_FUNCTION (void iarr_destroy_func,
		 (iarray arr, ThreeDeeSliceWidget top) );
STATIC_FUNCTION (flag dummy_pos_consumer,
		 (KPixCanvas canvas, int x, int y,
		  unsigned int event_code, void *e_info, void **f_info) );
STATIC_FUNCTION (void free_data, (ThreeDeeSliceWidget top) );


#define offset(field) XtOffsetOf(ThreeDeeSliceRec, threeDeeSlice.field)
#define XkwRKcoord_3d "Kcoord_3d"

static XtResource resources[] =
{
    {XkwNiarray, XkwCIarray, XtRPointer, sizeof (XtPointer),
     offset (cube), XtRImmediate, NULL},
    {XkwNkarmaColourmap, XkwCKarmaColourmap, XtRPointer, sizeof (XtPointer),
     offset (karmaCmap), XtRImmediate, NULL},
    {XkwNminPtr, XkwCMinPtr, XtRPointer, sizeof (XtPointer),
     offset (minPtr), XtRImmediate, NULL},
    {XkwNmaxPtr, XkwCMaxPtr, XtRPointer, sizeof (XtPointer),
     offset (maxPtr), XtRImmediate, NULL},
    {XkwNcanvasVisual, XtCVisual, XtRVisual, sizeof (Visual *),
     offset (canvasVisual), XtRImmediate, CopyFromParent},
    {XkwNverbose, XkwCVerbose, XtRBool, sizeof (Bool),
     offset (verbose), XtRImmediate, False},
    {XkwNcursorCallback, XtCCallback, XtRCallback, sizeof (XtPointer),
     offset (cursorCallback), XtRCallback, (XtPointer) NULL},
#undef offset
};


ThreeDeeSliceClassRec threeDeeSliceClassRec =
{
    {
	/* core fields  */
	(WidgetClass) &formClassRec,    /*  superclass             */
	"ThreeDeeSlice",                /*  class_name             */
	sizeof (ThreeDeeSliceRec),      /*  widget_size            */
	NULL,                           /*  class_initialise       */
	NULL,                           /*  class_part_initialise  */
	FALSE,                          /*  class_inited           */
	(XtInitProc) ThreeDeeSlice__Initialise, /*  initialise             */
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
	(XtSetValuesFunc) ThreeDeeSlice__SetValues, /*  set_values */
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
    { /* Constraint */
	NULL                            /* subresourses       */,
	0                               /* subresource_count  */,
	sizeof (ThreeDeeSliceConstraintsRec) /* constraint_size    */,
	NULL                            /* initialise         */,
	NULL                            /* destroy            */,
	NULL                            /* set_values         */,
	NULL                            /* extension          */ 
    },
    { /* Form */
	XtInheritLayout                 /* layout             */
    },
    {
	/*  threeDeeSlice fields */
	0                               /*  ignore                 */
    }
};

WidgetClass threeDeeSliceWidgetClass = (WidgetClass) &threeDeeSliceClassRec;

static void ThreeDeeSlice__Initialise (Widget Request, Widget New)
{
    Kdisplay dpy_handle;
    int canvas_size = 512;
    Colormap xcmap;
    /*ThreeDeeSliceWidget request = (ThreeDeeSliceWidget) Request;*/
    ThreeDeeSliceWidget new = (ThreeDeeSliceWidget) New;
    Widget w, canvas;
    /*static char function_name[] = "ThreeDeeSliceWidget::Initialise";*/

    kcmap_get_attributes (new->threeDeeSlice.karmaCmap,
			  KCMAP_ATT_DPY_HANDLE, &dpy_handle,
			  NULL);
    xcmap = xc_get_cmap (dpy_handle);
    m_clear ( (char *) &new->threeDeeSlice.cursor,
	      sizeof new->threeDeeSlice.cursor );
    new->threeDeeSlice.xy_pixcanvas = NULL;
    new->threeDeeSlice.xz_pixcanvas = NULL;
    new->threeDeeSlice.zy_pixcanvas = NULL;
    new->threeDeeSlice.last_event_canvas = NULL;
    new->threeDeeSlice.xy_worldcanvas = NULL;
    new->threeDeeSlice.xz_worldcanvas = NULL;
    new->threeDeeSlice.zy_worldcanvas = NULL;
    new->threeDeeSlice.xy_frames = NULL;
    new->threeDeeSlice.xz_frames = NULL;
    new->threeDeeSlice.zy_frames = NULL;
    new->threeDeeSlice.x_mag = 1;
    new->threeDeeSlice.y_mag = 1;
    new->threeDeeSlice.z_mag = 1;
    new->threeDeeSlice.iarr_destroy_func = NULL;
    new->threeDeeSlice.ap = NULL;
    w = XtVaCreateManagedWidget ("slider", valueWidgetClass, New,
				 XtNlabel, "X Magnification",
				 XtNorientation, XtorientHorizontal,
				 XkwNminimum, 1,
				 XkwNmaximum, 10,
				 XkwNmodifier, 1,
				 XtNvalue, 1,
				 XkwNvaluePtr, &new->threeDeeSlice.x_mag,
				 NULL);
    XtAddCallback (w, XkwNvalueChangeCallback, redisplay_cbk,
		   (XtPointer) new);
    w = XtVaCreateManagedWidget ("slider", valueWidgetClass, New,
				 XtNlabel, "Y Magnification",
				 XtNfromVert, w,
				 XtNorientation, XtorientHorizontal,
				 XkwNminimum, 1,
				 XkwNmaximum, 10,
				 XkwNmodifier, 1,
				 XtNvalue, 1,
				 XkwNvaluePtr, &new->threeDeeSlice.y_mag,
				 NULL);
    XtAddCallback (w, XkwNvalueChangeCallback, redisplay_cbk,
		   (XtPointer) new);
    w = XtVaCreateManagedWidget ("slider", valueWidgetClass, New,
				 XtNlabel, "Z Magnification",
				 XtNfromVert, w,
				 XtNorientation, XtorientHorizontal,
				 XkwNminimum, 1,
				 XkwNmaximum, 10,
				 XkwNmodifier, 1,
				 XtNvalue, 1,
				 XkwNvaluePtr, &new->threeDeeSlice.z_mag,
				 NULL);
    XtAddCallback (w, XkwNvalueChangeCallback, redisplay_cbk,
		   (XtPointer) new);
    w = XtVaCreateManagedWidget ("trackLabel0", labelWidgetClass, New,
				 XtNlabel,
				 "Track Output                                                   ",
				 XtNfromVert,  w,
				 XtNheight, 20,
				 NULL);
    new->threeDeeSlice.track_label0 = w;
    w = XtVaCreateManagedWidget ("trackLabel1", labelWidgetClass, New,
				 XtNlabel,
				 "Track Output                                                   ",
				 XtNfromVert,  w,
				 XtNheight, 20,
				 NULL);
    new->threeDeeSlice.track_label1 = w;
    /*  If screen is less than 700 pixels high, the normal canvas size of
	512*512 will not fit, so reduce the canvas size a bit. If the virtual
	screen size is large, but the physical screen size is small, this test
	will fail, but on the other hand at least the user will be able to
	navigate the view screen over the entire canvas. And of course, there
	is always the option of the user resizing the window...
	*/
    if (HeightOfScreen ( XtScreen (New) ) < 700) canvas_size = 320;
    canvas = XtVaCreateManagedWidget ("canvas", canvasWidgetClass, New,
				      XtNfromVert, w,
				      XtNwidth, canvas_size,
				      XtNheight, canvas_size,
				      XtNmappedWhenManaged, True,
				      XkwNsilenceUnconsumed, True,
				      XkwNretainFgBg, True,
				      XtNvisual,
				      new->threeDeeSlice.canvasVisual,
				      XtNcolormap, xcmap,
				      NULL);
    XtAddCallback (canvas, XkwNrealiseCallback, realise_cbk, (XtPointer) new);
}   /*  End Function Initialise  */

static Boolean ThreeDeeSlice__SetValues (Widget Current, Widget Request,
					 Widget New)
{
    flag verbose;
    double min, max;
    ThreeDeeSliceWidget current = (ThreeDeeSliceWidget) Current;
    ThreeDeeSliceWidget new = (ThreeDeeSliceWidget) New;
    static char function_name[] = "ThreeDeeSliceWidget::SetValues";

    verbose = new->threeDeeSlice.verbose;
    if (new->threeDeeSlice.cube != current->threeDeeSlice.cube)
    {
	/*  Change in cube data  */
	if (new->threeDeeSlice.iarr_destroy_func != NULL)
	{
	    c_unregister_callback (new->threeDeeSlice.iarr_destroy_func);
	}
	new->threeDeeSlice.iarr_destroy_func = NULL;
	if (new->threeDeeSlice.ap != NULL)
	    wcs_astro_destroy (new->threeDeeSlice.ap);
	new->threeDeeSlice.ap = NULL;
	free_data (new);
	/*  New cube data  */
	if (new->threeDeeSlice.cube != NULL)
	{
	    new->threeDeeSlice.iarr_destroy_func =
	    iarray_register_destroy_func (new->threeDeeSlice.cube,
					  ( flag (*) () ) iarr_destroy_func,
					  new);
	    if ( (new->threeDeeSlice.minPtr == NULL) ||
		(new->threeDeeSlice.maxPtr == NULL) )
	    {
		/*  Must compute minumum and maximum  */
		if ( !iarray_min_max (new->threeDeeSlice.cube, CONV1_REAL,
				      &min, &max) )
		{
		    fprintf (stderr, "Error computing min-max\n");
		}
	    }
	    else
	    {
		min = *new->threeDeeSlice.minPtr;
		max = *new->threeDeeSlice.maxPtr;
	    }
	    if (verbose) fprintf (stderr, "%s: min: %e  max: %e\n",
				  function_name, min, max);
	    new->threeDeeSlice.ap =
		wcs_astro_setup (new->threeDeeSlice.cube->top_pack_desc,
				 *new->threeDeeSlice.cube->top_packet);
	    register_new_cube (new, new->threeDeeSlice.cube, min, max);
	}
    }
    new->threeDeeSlice.minPtr = NULL;
    new->threeDeeSlice.maxPtr = NULL;
    return False;
}   /*  End Function SetValues  */

static void realise_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the canvas realise callback.
*/
{
    KPixCanvas parent = (KPixCanvas) call_data;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) client_data;
    XGCValues gcvalues;
    XColor scrn_def, exact_def;
    Colormap xcmap;
    struct win_scale_type win_scale;
    /*static char function_name[] = "ThreeDeeSliceWidget::realise_cbk";*/

    top->threeDeeSlice.parent_pixcanvas = parent;
    XtVaGetValues (w,
		   XtNcolormap, &xcmap,
		   XtNbackground, &gcvalues.background,
		   NULL);
    kwin_register_refresh_func (parent, parent_refresh_func, top);
    /*  Create the 3 child pixel canvases (for the XY, XZ, and ZY planes)  */
    if ( ( top->threeDeeSlice.xy_pixcanvas =
	   kwin_create_child (parent, 0, 0, 3, 3, TRUE) ) == NULL )
    {
	exit (RV_UNDEF_ERROR);
    }
    kwin_register_position_event_func (top->threeDeeSlice.xy_pixcanvas,
				       dummy_pos_consumer, top);
    if ( ( top->threeDeeSlice.xz_pixcanvas =
	   kwin_create_child (parent, 0, 0, 3, 3, TRUE) ) == NULL )
    {
	exit (RV_UNDEF_ERROR);
    }
    kwin_register_position_event_func (top->threeDeeSlice.xz_pixcanvas,
				       dummy_pos_consumer, top);
    if ( ( top->threeDeeSlice.zy_pixcanvas =
	   kwin_create_child (parent, 0, 0, 3, 3, TRUE) ) == NULL )
    {
	exit (RV_UNDEF_ERROR);
    }
    kwin_register_position_event_func (top->threeDeeSlice.zy_pixcanvas,
				       dummy_pos_consumer, top);
    canvas_init_win_scale (&win_scale, K_WIN_SCALE_MAGIC_NUMBER);
    /*  Get saturation pixels  */
    if (XAllocNamedColor (XtDisplay (w), xcmap, "Black", &scrn_def,
			  &exact_def) == 0)
    {
	fprintf (stderr, "Error allocating black in colourmap\n");
	exit (RV_UNDEF_ERROR);
    }
    win_scale.min_sat_pixel = scrn_def.pixel;
    if (XAllocNamedColor (XtDisplay (w), xcmap, "White", &scrn_def,
			  &exact_def) == 0)
    {
	fprintf (stderr, "Error allocating white in colourmap\n");
	exit (RV_UNDEF_ERROR);
    }
    win_scale.max_sat_pixel = scrn_def.pixel;
    win_scale.blank_pixel = gcvalues.background;
    /*  Create the main world canvas (with dummy world min and max)  */
    win_scale.z_min = 0.0;
    win_scale.z_max = 255.0;
    /*  XY canvas  */
    if ( ( top->threeDeeSlice.xy_worldcanvas =
	   canvas_create (top->threeDeeSlice.xy_pixcanvas,
			  top->threeDeeSlice.karmaCmap, &win_scale) )
	 == NULL )
    {
	exit (RV_UNDEF_ERROR);
    }
    canvas_use_astro_transform (top->threeDeeSlice.xy_worldcanvas,
				&top->threeDeeSlice.ap);
    canvas_set_dressing (top->threeDeeSlice.xy_worldcanvas,
			 KCD_DISPLAY, TRUE,
			 KCD_END);
    viewimg_init (top->threeDeeSlice.xy_worldcanvas);
    viewimg_register_position_event_func (top->threeDeeSlice.xy_worldcanvas,
					  xy_pos_consumer, top);
#ifdef dummy
    viewimg_register_position_event_func (xy_worldcanvas,xy_track_canvas_event,
					  (void *) xy_worldcanvas);
#endif
    viewimg_set_canvas_attributes (top->threeDeeSlice.xy_worldcanvas,
				   VIEWIMG_ATT_AUTO_V, FALSE,
				   VIEWIMG_ATT_INT_X, FALSE,
				   VIEWIMG_ATT_INT_Y, FALSE,
				   VIEWIMG_ATT_END);
    canvas_register_refresh_func (top->threeDeeSlice.xy_worldcanvas,
				  xy_canvas_refresh_func, top);
    /*  XZ canvas  */
    if ( ( top->threeDeeSlice.xz_worldcanvas =
	   canvas_create (top->threeDeeSlice.xz_pixcanvas,
			  top->threeDeeSlice.karmaCmap, &win_scale) )
	 == NULL )
    {
	exit (RV_UNDEF_ERROR);
    }
    canvas_use_astro_transform (top->threeDeeSlice.xz_worldcanvas,
				&top->threeDeeSlice.ap);
    canvas_set_dressing (top->threeDeeSlice.xz_worldcanvas,
			 KCD_DISPLAY, TRUE,
			 KCD_END);
    viewimg_init (top->threeDeeSlice.xz_worldcanvas);
    viewimg_register_position_event_func (top->threeDeeSlice.xz_worldcanvas,
					  xz_pos_consumer, top);
#ifdef dummy
    viewimg_register_position_event_func (xz_worldcanvas,xz_track_canvas_event,
					  (void *) xz_worldcanvas);
#endif
    viewimg_set_canvas_attributes (top->threeDeeSlice.xz_worldcanvas,
				   VIEWIMG_ATT_AUTO_V, FALSE,
				   VIEWIMG_ATT_INT_X, FALSE,
				   VIEWIMG_ATT_INT_Y, FALSE,
				   VIEWIMG_ATT_END);
    canvas_register_refresh_func (top->threeDeeSlice.xz_worldcanvas,
				  xz_canvas_refresh_func, top);
    /*  ZY canvas  */
    if ( ( top->threeDeeSlice.zy_worldcanvas =
	   canvas_create (top->threeDeeSlice.zy_pixcanvas,
			  top->threeDeeSlice.karmaCmap, &win_scale) )
	 == NULL )
    {
	exit (RV_UNDEF_ERROR);
    }
    canvas_use_astro_transform (top->threeDeeSlice.zy_worldcanvas,
				&top->threeDeeSlice.ap);
    canvas_set_dressing (top->threeDeeSlice.zy_worldcanvas,
			 KCD_DISPLAY, TRUE,
			 KCD_END);
    viewimg_init (top->threeDeeSlice.zy_worldcanvas);
    viewimg_register_position_event_func (top->threeDeeSlice.zy_worldcanvas,
					  zy_pos_consumer, top);
#ifdef dummy
    viewimg_register_position_event_func (zy_worldcanvas,zy_track_canvas_event,
					  (void *) zy_worldcanvas);
#endif
    viewimg_set_canvas_attributes (top->threeDeeSlice.zy_worldcanvas,
				   VIEWIMG_ATT_AUTO_V, FALSE,
				   VIEWIMG_ATT_INT_X, FALSE,
				   VIEWIMG_ATT_INT_Y, FALSE,
				   VIEWIMG_ATT_END);
    canvas_register_refresh_func (top->threeDeeSlice.zy_worldcanvas,
				  zy_canvas_refresh_func, top);
}   /*  End Function realise_cbk   */

static void redisplay_cbk (Widget w, XtPointer client_data,XtPointer call_data)
/*  This is the redisplay callback.
*/
{
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) client_data;

    kwin_resize (top->threeDeeSlice.parent_pixcanvas, TRUE, 0, 0, 0, 0);
}   /*  End Function redisplay_cbk   */

static void parent_refresh_func (KPixCanvas canvas, int width, int height,
				 void **info, PostScriptPage pspage)
/*  [PURPOSE] This routine will process a refresh event for a pixel canvas.
    <canvas> The pixel canvas.
    <width> The width of the canvas in pixels.
    <height> The height of the canvas in pixels.
    <info> A pointer to the arbitrary canvas information pointer.
    <pspage> If not NULL, the PostScriptPage object the refresh is
    redirected to.
    [RETURNS] Nothing.
*/
{
    int x_off, y_off, xlen, ylen, zlen, xpix, ypix, zpix;
    float factor, tmp;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *info;
    static char function_name[] = "ThreeDeeSlice::parent_refresh_func";

    if (top->threeDeeSlice.cube == NULL) return;
    /*  Get lengths of each dimension  */
    xlen = iarray_dim_length (top->threeDeeSlice.cube, 2);
    ylen = iarray_dim_length (top->threeDeeSlice.cube, 1);
    zlen = iarray_dim_length (top->threeDeeSlice.cube, 0);
    if ( (xlen + zlen + 3 > width) || (ylen + zlen + 3 > height) )
    {
	/*  Even when unmagnified the images are too big: scale down  */
	fprintf (stderr, "%s: images too big for window: subsampling data\n",
		 function_name);
	/*  Determine smallest scale factor  */
	factor = (float) (width - 3) / (float) (xlen + zlen);
	tmp = (float) (height - 3) / (float) (ylen + zlen);
	factor = (tmp < factor) ? tmp : factor;
	xpix = factor * (float) xlen;
	ypix = factor * (float) ylen;
	zpix = factor * (float) zlen;
    }
    else
    {
	/*  Apply the magnifications  */
	xpix = xlen * top->threeDeeSlice.x_mag;
	ypix = ylen * top->threeDeeSlice.y_mag;
	zpix = zlen * top->threeDeeSlice.z_mag;
    }
    if ( (xpix + zpix + 3 > width) || (ypix + zpix + 3 > height) )
    {
	fprintf (stderr,
		 "%s: images too big for window: reduce magnification\n",
		 function_name);
	return;
    }
    kwin_set_attributes (top->threeDeeSlice.xy_pixcanvas,
			 KWIN_ATT_VISIBLE, TRUE,
			 KWIN_ATT_END);
    kwin_set_attributes (top->threeDeeSlice.xz_pixcanvas,
			 KWIN_ATT_VISIBLE, TRUE,
			 KWIN_ATT_END);
    kwin_set_attributes (top->threeDeeSlice.zy_pixcanvas,
			 KWIN_ATT_VISIBLE, TRUE,
			 KWIN_ATT_END);
    x_off = ( width - (xpix + zpix + 3) ) / 2;
    y_off = ( height - (ypix + zpix + 3) ) / 2;
    kwin_resize (top->threeDeeSlice.xy_pixcanvas, FALSE, x_off, y_off,
		 xpix + 2, ypix + 2);
    kwin_resize (top->threeDeeSlice.xz_pixcanvas, FALSE,
		 x_off, y_off + ypix + 1,
		 xpix + 2, zpix + 2);
    kwin_resize (top->threeDeeSlice.zy_pixcanvas, FALSE,
		 x_off + xpix + 1, y_off,
		 zpix + 2, ypix + 2);
}   /*  End Function parent_refresh_func  */


/*  Routines which draw the crosshairs follow  */

static void xy_canvas_refresh_func (KWorldCanvas canvas, int width, int height,
				    struct win_scale_type *win_scale,
				    Kcolourmap cmap, flag cmap_resize,
				    void **info)
/*  [PURPOSE] This routine is a refresh event consumer for a world canvas.
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
    [RETURNS] Nothing.
*/
{
    KPixCanvas pixcanvas;
    int x_offset, y_offset, x_pixels, y_pixels;
    unsigned long pixel_value;
    double px, py;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *info;

    if ( !canvas_get_colour (canvas, "red", &pixel_value,
			     (unsigned short *) NULL, (unsigned short *) NULL,
			     (unsigned short *) NULL) ) return;
    px = top->threeDeeSlice.cursor.linear.x;
    py = top->threeDeeSlice.cursor.linear.y;
    canvas_convert_from_canvas_coords (canvas, FALSE, TRUE, 1,
				       &px, &py, &px, &py);
    pixcanvas = canvas_get_pixcanvas (canvas);
    canvas_get_attributes (canvas,
			   CANVAS_ATT_X_OFFSET, &x_offset,
			   CANVAS_ATT_Y_OFFSET, &y_offset,
			   CANVAS_ATT_X_PIXELS, &x_pixels,
			   CANVAS_ATT_Y_PIXELS, &y_pixels,
			   CANVAS_ATT_END);
    kwin_draw_line (pixcanvas, x_offset, py, x_offset + x_pixels - 1, py,
		    pixel_value);
    kwin_draw_line (pixcanvas, px, y_offset, px, y_offset + y_pixels - 1,
		    pixel_value);
}   /*  End Function xy_canvas_refresh_func  */

static void xz_canvas_refresh_func (KWorldCanvas canvas, int width, int height,
				    struct win_scale_type *win_scale,
				    Kcolourmap cmap, flag cmap_resize,
				    void **info)
/*  [PURPOSE] This routine is a refresh event consumer for a world canvas.
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
    [RETURNS] Nothing.
*/
{
    KPixCanvas pixcanvas;
    int x_offset, y_offset, x_pixels, y_pixels;
    unsigned long pixel_value;
    double px, py;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *info;

    if ( !canvas_get_colour (canvas, "red", &pixel_value,
			     (unsigned short *) NULL, (unsigned short *) NULL,
			     (unsigned short *) NULL) ) return;
    px = top->threeDeeSlice.cursor.linear.x;
    py = top->threeDeeSlice.cursor.linear.z;
    canvas_convert_from_canvas_coords (canvas, FALSE, TRUE, 1,
				       &px, &py, &px, &py);
    pixcanvas = canvas_get_pixcanvas (canvas);
    canvas_get_attributes (canvas,
			   CANVAS_ATT_X_OFFSET, &x_offset,
			   CANVAS_ATT_Y_OFFSET, &y_offset,
			   CANVAS_ATT_X_PIXELS, &x_pixels,
			   CANVAS_ATT_Y_PIXELS, &y_pixels,
			   CANVAS_ATT_END);
    kwin_draw_line (pixcanvas, x_offset, py, x_offset + x_pixels - 1, py,
		    pixel_value);
    kwin_draw_line (pixcanvas, px, y_offset, px, y_offset + y_pixels - 1,
		    pixel_value);
}   /*  End Function xz_canvas_refresh_func  */

static void zy_canvas_refresh_func (KWorldCanvas canvas, int width, int height,
				    struct win_scale_type *win_scale,
				    Kcolourmap cmap, flag cmap_resize,
				    void **info)
/*  [PURPOSE] This routine is a refresh event consumer for a world canvas.
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
    [RETURNS] Nothing.
*/
{
    KPixCanvas pixcanvas;
    int x_offset, y_offset, x_pixels, y_pixels;
    unsigned long pixel_value;
    double px, py;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *info;

    if ( !canvas_get_colour (canvas, "red", &pixel_value,
			     (unsigned short *) NULL, (unsigned short *) NULL,
			     (unsigned short *) NULL) ) return;
    px = top->threeDeeSlice.cursor.linear.z;
    py = top->threeDeeSlice.cursor.linear.y;
    canvas_convert_from_canvas_coords (canvas, FALSE, TRUE, 1,
				       &px, &py, &px, &py);
    pixcanvas = canvas_get_pixcanvas (canvas);
    canvas_get_attributes (canvas,
			   CANVAS_ATT_X_OFFSET, &x_offset,
			   CANVAS_ATT_Y_OFFSET, &y_offset,
			   CANVAS_ATT_X_PIXELS, &x_pixels,
			   CANVAS_ATT_Y_PIXELS, &y_pixels,
			   CANVAS_ATT_END);
    kwin_draw_line (pixcanvas, x_offset, py, x_offset + x_pixels - 1, py,
		    pixel_value);
    kwin_draw_line (pixcanvas, px, y_offset, px, y_offset + y_pixels - 1,
		    pixel_value);
}   /*  End Function zy_canvas_refresh_func  */


/*  Functions which process mouse events follow  */

static void move_cursor (ThreeDeeSliceWidget top,
			 struct XkwThreeDeeSliceCursor new_cursor)
/*  [PURPOSE] This routine will move the cursor to a new position, refreshing
    any canvases that require it.
    <top> The ThreeDeeSlice widget.
    <new_cursor> The new cursor position. All values must be defined.
    [RETURNS] Nothing.
*/
{
    struct XkwThreeDeeSliceCursor old_cursor;

    if (top->threeDeeSlice.cube == NULL) return;
    if (top->threeDeeSlice.xy_frames == NULL) return;
    if (top->threeDeeSlice.xz_frames == NULL) return;
    if (top->threeDeeSlice.zy_frames == NULL) return;
    old_cursor = top->threeDeeSlice.cursor;
    /*  A quick test if new cursor position is the same. Unlikely,
	considering the vagaries of floating point roundoffs  */
    if ( (old_cursor.pixel.x == new_cursor.pixel.x) &&
	 (old_cursor.pixel.y == new_cursor.pixel.y) &&
	 (old_cursor.pixel.z == new_cursor.pixel.z) ) return;
    /*  Update active cursor position so any refresh will have the right cursor
	position available.  */
    top->threeDeeSlice.cursor = new_cursor;
    /*  Refresh parts of the canvases if possible to "undraw" the old crosshair
	I prefer to do this rather than refreshing the entire canvas because
	it is much faster to redraw subimages a few pixels thick. This is
	especially true when the image is not a SHM XImage or cached in a
	Pixmap  */
    if ( (long) old_cursor.pixel.x == (long) new_cursor.pixel.x )
    {
	/*  No change in X index, which means ZY canvas has the same image, but
	    the cursor may have moved, so a partial refresh is needed  */
	refresh_canvas (top->threeDeeSlice.zy_worldcanvas,
			old_cursor.linear.z, old_cursor.linear.y);
    }
    else
    {
	/*  X index has changed, so we need a new ZY image  */
	if ( !viewimg_make_active
	     (top->threeDeeSlice.zy_frames[(long) new_cursor.pixel.x]) )
	{
	    fprintf (stderr, "Error making ViewableImage active\n");
	}
    }
    if ( (long) old_cursor.pixel.y == (long) new_cursor.pixel.y )
    {
	/*  No change in Y index, which means XZ canvas has the same image, but
	    the cursor may have moved, so a partial refresh is needed  */
	refresh_canvas (top->threeDeeSlice.xz_worldcanvas,
			old_cursor.linear.x, old_cursor.linear.z);
    }
    else
    {
	/*  Y index has changed, so we need a new XZ image  */
	if ( !viewimg_make_active
	     (top->threeDeeSlice.xz_frames[(long) new_cursor.pixel.y]) )
	{
	    fprintf (stderr, "Error making ViewableImage active\n");
	}
    }
    if ( (long) old_cursor.pixel.z == (long) new_cursor.pixel.z )
    {
	/*  No change in Z index, which means XY canvas has the same image, but
	    the cursor may have moved, so a partial refresh is needed  */
	refresh_canvas (top->threeDeeSlice.xy_worldcanvas,
			old_cursor.linear.x, old_cursor.linear.y);
    }
    else
    {
	/*  Z index has changed, so we need a new XY image  */
	if ( !viewimg_make_active
	     (top->threeDeeSlice.xy_frames[(long) new_cursor.pixel.z]) )
	{
	    fprintf (stderr, "Error making ViewableImage active\n");
	}
    }
}   /*  End Function move_cursor  */

static void refresh_canvas (KWorldCanvas canvas, double lx, double ly)
/*  [PURPOSE] This routine will refresh a canvas by "undrawing" the old cursor
    by the use of partial refresh events.
    <canvas> The canvas.
    <wx> The old horizontal component of the cursor position in linear
    co-ordinates.
    <wy> The old vertical component of the cursor position in linear
    co-ordinates.
    [RETURNS] Nothing.
*/
{
    int px, py, xoff, yoff, width, height;
    double x, y;
    KPixCanvasRefreshArea areas[2];

    /*  Compute area  */
    canvas_get_attributes (canvas,
			   CANVAS_ATT_X_OFFSET, &xoff,
			   CANVAS_ATT_Y_OFFSET, &yoff,
			   CANVAS_ATT_X_PIXELS, &width,
			   CANVAS_ATT_Y_PIXELS, &height,
			   CANVAS_ATT_END);
    canvas_convert_from_canvas_coords (canvas, TRUE, TRUE, 1,
				       &lx, &ly, &x, &y);
    px = x;
    py = y;
    /*  Area for the horizontal line  */
    areas[0].startx = xoff;
    areas[0].endx = xoff + width - 1;
    areas[0].starty = py - 1;
    if (areas[0].starty < 0) areas[0].starty = 0;
    areas[0].endy = py + 1;
    if (areas[0].endy >= yoff + height) areas[0].endy = yoff + height - 1;
    areas[0].clear = TRUE;
    /*  Area for the vertical line  */
    areas[1].startx = px - 1;
    if (areas[1].startx < 0) areas[1].startx = 0;
    areas[1].endx = px + 1;
    if (areas[1].endx >= xoff + width) areas[1].endx = xoff + width - 1;
    areas[1].starty = yoff;
    areas[1].endy = yoff + height - 1;
    areas[1].clear = TRUE;
    viewimg_partial_refresh (canvas, 2, areas);
}   /*  End Function refresh_canvas  */

static flag xy_pos_consumer (ViewableImage vimage, double x, double y,
			     void *value, unsigned int event_code,
			     void *e_info, void **f_info,
			     double x_lin, double y_lin,
			     unsigned int value_type)
/*  [SUMMARY] Position event callback.
    [PURPOSE] This routine is a position event consumer for a world canvas
    which has a number of ViewableImage objects associated with it.
    <viewimg> The active viewable image.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    These values will have been transformed by the registered transform
    function (see [<canvas_register_transform_func>]) for the associated
    world canvas.
    <value> A pointer to the data value in the viewable image corresponding
    to the event co-ordinates.
    <event_code> The arbitrary event code.
    <e_info> The arbitrary event information.
    <f_info> The arbitrary function information pointer.
    <x_lin> The linear horizontal world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <y_lin> The linear vertical world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <value_type> The type of the data value. This may be K_DCOMPLEX or
    K_UB_RGB.
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    struct XkwThreeDeeSliceCursor cursor;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *f_info;
    char *ptr;
    char pix_string[STRING_LENGTH], world_string[STRING_LENGTH];
    char extra_string[STRING_LENGTH];
    static char function_name[] = "ThreeDeeSliceWiget::xy_pos_consumer";

    if (event_code == K_CANVAS_EVENT_POINTER_MOVE)
    {
	viewimg_track_compute (vimage, value, value_type, x, y, x_lin, y_lin,
			       top->threeDeeSlice.ap, pix_string,
			       world_string, extra_string, NULL, NULL);
	/*  Need to fiddle the string  */
	if ( ( ptr = strchr (pix_string, 'z') ) == NULL )
	{
	    fprintf (stderr, "%s: error searching for 'z'\n", function_name);
	    return (TRUE);
	}
	ptr[0] = ' ';
	ptr[1] = 'z';
	XtVaSetValues (top->threeDeeSlice.track_label0,
		       XtNlabel, pix_string,
		       NULL);
	XtVaSetValues (top->threeDeeSlice.track_label1,
		       XtNlabel, world_string,
		       NULL);
	return (TRUE);
    }
    if ( (event_code != K_CANVAS_EVENT_LEFT_MOUSE_CLICK) &&
	 (event_code != K_CANVAS_EVENT_LEFT_MOUSE_DRAG) ) return (FALSE);
    fillin_cursor (top, &cursor, XkwThreeDeeSlicePixelCursor,
		   x_lin, y_lin, top->threeDeeSlice.cursor.linear.z);
    move_cursor (top, cursor);
    XtCallCallbacks ( (Widget) top, XkwNcursorCallback, (XtPointer) &cursor );
    return (TRUE);
}   /*  End Function xy_pos_consumer  */

static flag xz_pos_consumer (ViewableImage vimage, double x, double y,
			     void *value, unsigned int event_code,
			     void *e_info, void **f_info,
			     double x_lin, double y_lin,
			     unsigned int value_type)
/*  [SUMMARY] Position event callback.
    [PURPOSE] This routine is a position event consumer for a world canvas
    which has a number of ViewableImage objects associated with it.
    <viewimg> The active viewable image.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    These values will have been transformed by the registered transform
    function (see [<canvas_register_transform_func>]) for the associated
    world canvas.
    <value> A pointer to the data value in the viewable image corresponding
    to the event co-ordinates.
    <event_code> The arbitrary event code.
    <e_info> The arbitrary event information.
    <f_info> The arbitrary function information pointer.
    <x_lin> The linear horizontal world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <y_lin> The linear vertical world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <value_type> The type of the data value. This may be K_DCOMPLEX or
    K_UB_RGB.
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    struct XkwThreeDeeSliceCursor cursor;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *f_info;
    char *ptr1, *ptr2;
    char pix_string[STRING_LENGTH], world_string[STRING_LENGTH];
    char extra_string[STRING_LENGTH];
    static char function_name[] = "ThreeDeeSliceWiget::xz_pos_consumer";

    if (event_code == K_CANVAS_EVENT_POINTER_MOVE)
    {
	viewimg_track_compute (vimage, value, value_type, x, y, x_lin, y_lin,
			       top->threeDeeSlice.ap, pix_string,
			       world_string, extra_string, NULL, NULL);
	/*  Need to fiddle the string  */
	if ( ( ptr1 = strchr (pix_string, 'y') ) == NULL )
	{
	    fprintf (stderr, "%s: error searching for 'y'\n", function_name);
	    return (TRUE);
	}
	if ( ( ptr2 = strchr (pix_string, 'z') ) == NULL )
	{
	    fprintf (stderr, "%s: error searching for 'z'\n", function_name);
	    return (TRUE);
	}
	*ptr1 = 'z';
	ptr2[0] = ' ';
	ptr2[1] = 'y';
	XtVaSetValues (top->threeDeeSlice.track_label0,
		       XtNlabel, pix_string,
		       NULL);
	XtVaSetValues (top->threeDeeSlice.track_label1,
		       XtNlabel, world_string,
		       NULL);
	return (TRUE);
    }
    if ( (event_code != K_CANVAS_EVENT_LEFT_MOUSE_CLICK) &&
	 (event_code != K_CANVAS_EVENT_LEFT_MOUSE_DRAG) ) return (FALSE);
    fillin_cursor (top, &cursor, XkwThreeDeeSlicePixelCursor,
		   top->threeDeeSlice.cursor.linear.x,
		   top->threeDeeSlice.cursor.linear.y, y_lin);
    move_cursor (top, cursor);
    XtCallCallbacks ( (Widget) top, XkwNcursorCallback, (XtPointer) &cursor );
    return (TRUE);
}   /*  End Function xz_pos_consumer  */

static flag zy_pos_consumer (ViewableImage vimage, double x, double y,
			     void *value, unsigned int event_code,
			     void *e_info, void **f_info,
			     double x_lin, double y_lin,
			     unsigned int value_type)
/*  [SUMMARY] Position event callback.
    [PURPOSE] This routine is a position event consumer for a world canvas
    which has a number of ViewableImage objects associated with it.
    <viewimg> The active viewable image.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    These values will have been transformed by the registered transform
    function (see [<canvas_register_transform_func>]) for the associated
    world canvas.
    <value> A pointer to the data value in the viewable image corresponding
    to the event co-ordinates.
    <event_code> The arbitrary event code.
    <e_info> The arbitrary event information.
    <f_info> The arbitrary function information pointer.
    <x_lin> The linear horizontal world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <y_lin> The linear vertical world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <value_type> The type of the data value. This may be K_DCOMPLEX or
    K_UB_RGB.
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    struct XkwThreeDeeSliceCursor cursor;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *f_info;
    char *ptr1, *ptr2;
    char pix_string[STRING_LENGTH], world_string[STRING_LENGTH];
    char extra_string[STRING_LENGTH];
    static char function_name[] = "ThreeDeeSliceWiget::zy_pos_consumer";

    if (event_code == K_CANVAS_EVENT_POINTER_MOVE)
    {
	viewimg_track_compute (vimage, value, value_type, x, y, x_lin, y_lin,
			       top->threeDeeSlice.ap, pix_string,
			       world_string, extra_string, NULL, NULL);
	/*  Need to fiddle the string  */
	if ( ( ptr1 = strchr (pix_string, 'x') ) == NULL )
	{
	    fprintf (stderr, "%s: error searching for 'x'\n", function_name);
	    return (TRUE);
	}
	if ( ( ptr2 = strchr (pix_string, 'z') ) == NULL )
	{
	    fprintf (stderr, "%s: error searching for 'z'\n", function_name);
	    return (TRUE);
	}
	*ptr1 = 'z';
	ptr2[0] = ' ';
	ptr2[1] = 'x';
	XtVaSetValues (top->threeDeeSlice.track_label0,
		       XtNlabel, pix_string,
		       NULL);
	XtVaSetValues (top->threeDeeSlice.track_label1,
		       XtNlabel, world_string,
		       NULL);
	return (TRUE);
    }
    if ( (event_code != K_CANVAS_EVENT_LEFT_MOUSE_CLICK) &&
	 (event_code != K_CANVAS_EVENT_LEFT_MOUSE_DRAG) ) return (FALSE);
    fillin_cursor (top, &cursor, XkwThreeDeeSlicePixelCursor,
		   top->threeDeeSlice.cursor.linear.x,
		   top->threeDeeSlice.cursor.linear.y, x_lin);
    move_cursor (top, cursor);
    XtCallCallbacks ( (Widget) top, XkwNcursorCallback, (XtPointer) &cursor );
    return (TRUE);
}   /*  End Function zy_pos_consumer  */


/*  Other functions  */

static void fillin_cursor (ThreeDeeSliceWidget top,
			   XkwThreeDeeSliceCallbackData cursor,
			   unsigned int type, double x, double y, double z)
/*  [SUMMARY] Set the 3D cursor position.
    <top> The ThreeDeeSlice widget.
    <cursor> The cursor to fill in.
    <type> The type of the supplied co-ordinates.
    <x> The x co-ordinate.
    <y> The y co-ordinate.
    <z> The z co-ordinate.
    [RETURNS] Nothing.
*/
{
    dim_desc *xdim, *ydim, *zdim;
    static char function_name[] = "ThreeDeeSliceWidget::fillin_cursor";

    xdim = iarray_get_dim_desc (top->threeDeeSlice.cube, 2);
    ydim = iarray_get_dim_desc (top->threeDeeSlice.cube, 1);
    zdim = iarray_get_dim_desc (top->threeDeeSlice.cube, 0);
    switch (type)
    {
      case XkwThreeDeeSlicePixelCursor:
	cursor->pixel.x = x;
	cursor->pixel.y = y;
	cursor->pixel.z = z;
	/*  Convert to linear co-ordinates  */
	cursor->linear.x = ds_get_coordinate (xdim, cursor->pixel.x);
	cursor->linear.y = ds_get_coordinate (ydim, cursor->pixel.y);
	cursor->linear.z = ds_get_coordinate (zdim, cursor->pixel.z);
	/*  Convert to world co-ordinates  */
	cursor->world = cursor->linear;
	if (top->threeDeeSlice.ap != NULL)
	{
	    wcs_astro_transform3 (top->threeDeeSlice.ap, 1,
				  xdim->name, &cursor->world.x, FALSE,
				  ydim->name, &cursor->world.y, FALSE,
				  zdim->name, &cursor->world.z, FALSE,
				  0, NULL, NULL);
	}
	break;
      case XkwThreeDeeSliceLinearCursor:
	cursor->linear.x = x;
	cursor->linear.y = y;
	cursor->linear.z = z;
	/*  Convert to data pixel indices  */
	cursor->pixel.x = ds_get_coord_num (xdim, cursor->linear.x,
					    SEARCH_BIAS_CLOSEST);
	cursor->pixel.y = ds_get_coord_num (ydim, cursor->linear.y,
					    SEARCH_BIAS_CLOSEST);
	cursor->pixel.z = ds_get_coord_num (zdim, cursor->linear.z,
					    SEARCH_BIAS_CLOSEST);
	/*  Convert to world co-ordinates  */
	cursor->world = cursor->linear;
	if (top->threeDeeSlice.ap != NULL)
	{
	    wcs_astro_transform3 (top->threeDeeSlice.ap, 1,
				  xdim->name, &cursor->world.x, FALSE,
				  ydim->name, &cursor->world.y, FALSE,
				  zdim->name, &cursor->world.z, FALSE,
				  0, NULL, NULL);
	}
	break;
      case XkwThreeDeeSliceWorldCursor:
	cursor->world.x = x;
	cursor->world.y = y;
	cursor->world.z = z;
	/*  Convert to linear co-ordinates  */
	cursor->linear = cursor->world;
	if (top->threeDeeSlice.ap != NULL)
	{
	    wcs_astro_transform3 (top->threeDeeSlice.ap, 1,
				  xdim->name, &cursor->linear.x, TRUE,
				  ydim->name, &cursor->linear.y, TRUE,
				  zdim->name, &cursor->linear.z, TRUE,
				  0, NULL, NULL);
	}
	/*  Convert to data pixel indices  */
	cursor->pixel.x = ds_get_coord_num (xdim, cursor->linear.x,
					    SEARCH_BIAS_CLOSEST);
	cursor->pixel.y = ds_get_coord_num (ydim, cursor->linear.y,
					    SEARCH_BIAS_CLOSEST);
	cursor->pixel.z = ds_get_coord_num (zdim, cursor->linear.z,
					    SEARCH_BIAS_CLOSEST);
	break;
      default:
	fprintf (stderr, "Illegal cursor type: %u\n", type);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function fillin_cursor  */

static void register_new_cube (ThreeDeeSliceWidget top, iarray cube,
			       double min, double max)
/*  [PURPOSE] This routine will register a new cube.
    <top> The ThreeDeeSlice widget.
    <cube> The cube data.
    <min> The cube minimum.
    <max> The cube maximum.
    [RETURNS] Nothing.
*/
{
    double scale, offset;

    canvas_set_attributes (top->threeDeeSlice.xy_worldcanvas,
			   CANVAS_ATT_VALUE_MIN, min,
			   CANVAS_ATT_VALUE_MAX, max,
			   CANVAS_ATT_END);
    canvas_set_attributes (top->threeDeeSlice.xz_worldcanvas,
			   CANVAS_ATT_VALUE_MIN, min,
			   CANVAS_ATT_VALUE_MAX, max,
			   CANVAS_ATT_END);
    canvas_set_attributes (top->threeDeeSlice.zy_worldcanvas,
			   CANVAS_ATT_VALUE_MIN, min,
			   CANVAS_ATT_VALUE_MAX, max,
			   CANVAS_ATT_END);
    iarray_get_data_scaling (cube, &scale, &offset);
    /*  Create new ViewableImage objects  */
    if ( ( top->threeDeeSlice.xy_frames =
	  viewimg_create_sequence (top->threeDeeSlice.xy_worldcanvas,
				   cube->multi_desc, cube->arr_desc,
				   cube->data, 2, 1, 0, cube->elem_index) )
	== NULL )
    {
	fprintf (stderr, "Error creating ViewableImage sequence\n");
	free_data (top);
	return;
    }
    viewimg_set_array_attributes (top->threeDeeSlice.xy_frames,
				  iarray_dim_length (cube, 0),
				  VIEWIMG_VATT_DATA_SCALE, scale,
				  VIEWIMG_VATT_DATA_OFFSET, offset,
				  VIEWIMG_VATT_END);
    if ( ( top->threeDeeSlice.xz_frames =
	  viewimg_create_sequence (top->threeDeeSlice.xz_worldcanvas,
				   cube->multi_desc, cube->arr_desc,
				   cube->data, 2, 0, 1, cube->elem_index) )
	== NULL )
    {
	fprintf (stderr, "Error creating ViewableImage sequence\n");
	free_data (top);
	return;
    }
    viewimg_set_array_attributes (top->threeDeeSlice.xz_frames,
				  iarray_dim_length (cube, 1),
				  VIEWIMG_VATT_DATA_SCALE, scale,
				  VIEWIMG_VATT_DATA_OFFSET, offset,
				  VIEWIMG_VATT_END);
    if ( ( top->threeDeeSlice.zy_frames =
	  viewimg_create_sequence (top->threeDeeSlice.zy_worldcanvas,
				   cube->multi_desc, cube->arr_desc,
				   cube->data, 0, 1, 2, cube->elem_index) )
	== NULL )
    {
	fprintf (stderr, "Error creating ViewableImage sequence\n");
	free_data (top);
	return;
    }
    viewimg_set_array_attributes (top->threeDeeSlice.zy_frames,
				  iarray_dim_length (cube, 2),
				  VIEWIMG_VATT_DATA_SCALE, scale,
				  VIEWIMG_VATT_DATA_OFFSET, offset,
				  VIEWIMG_VATT_END);
    fillin_cursor (top, &top->threeDeeSlice.cursor,XkwThreeDeeSlicePixelCursor,
		   0.0, 0.0, 0.0);
    if ( !viewimg_set_active (top->threeDeeSlice.xy_frames[0], FALSE) )
    {
	fprintf (stderr, "Error making ViewableImage active\n");
	return;
    }
    if ( !viewimg_set_active (top->threeDeeSlice.xz_frames[0], FALSE) )
    {
	fprintf (stderr, "Error making ViewableImage active\n");
	return;
    }
    if ( !viewimg_set_active (top->threeDeeSlice.zy_frames[0], FALSE) )
    {
	fprintf (stderr, "Error making ViewableImage active\n");
	return;
    }
    /*  Now draw the lot  */
    kwin_resize (top->threeDeeSlice.parent_pixcanvas, TRUE, 0, 0, 0, 0);
}   /*  End Function register_new_cube  */

static void iarr_destroy_func (iarray arr, ThreeDeeSliceWidget top)
/*  [PURPOSE] This routine will register the destruction of the Intelligent
    Array.
    <arr> The Intelligent Array.
    <top> The ThreeDeeSlice widget.
    [RETURNS] Nothing.
*/
{
    flag verbose;

    verbose = top->threeDeeSlice.verbose;
    if (verbose) fprintf (stderr, "ThreeDeeSliceWidget::iarray destroyed\n");
    /*  Must free the other data first else we will be left with a bunch of
	ViewableImages floating around.  */
    free_data (top);
    top->threeDeeSlice.cube = NULL;
    top->threeDeeSlice.iarr_destroy_func = NULL;
    if (top->threeDeeSlice.ap != NULL)
	wcs_astro_destroy (top->threeDeeSlice.ap);
    top->threeDeeSlice.ap = NULL;
}   /*  End Function iarr_destroy_func  */

static flag dummy_pos_consumer (KPixCanvas canvas, int x, int y,
				unsigned int event_code, void *e_info,
				void **f_info)
/*  [PURPOSE] This routine is a position event consumer for a world canvas.
    <canvas> The canvas on which the event occurred.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    [NOTE] These values will have been transformed by the registered
    transform function (see <anvas_register_transform_func>>).
    <event_code> The arbitrary event code.
    <e_info> A pointer to arbitrary event information.
    <f_info> A pointer to an arbitrary function information pointer.
    <x_lin> The horizontal linear world co-ordinate prior to the transform
    function being called.
    <y_lin> The vertical linear world co-ordinate prior to the transform
    function being called.
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *f_info;

    if (top->threeDeeSlice.cube == NULL) return (TRUE);
    if (top->threeDeeSlice.last_event_canvas == NULL)
    {
	top->threeDeeSlice.last_event_canvas = canvas;
	return (FALSE);
    }
    if (event_code != K_CANVAS_EVENT_LEFT_MOUSE_DRAG)
    {
	top->threeDeeSlice.last_event_canvas = canvas;
	return (FALSE);
    }
    /*  Check if drag event previously came from different canvas  */
    /*  Absorb event if so  */
    if (canvas != top->threeDeeSlice.last_event_canvas)
    {
	return (TRUE);
    }
    top->threeDeeSlice.last_event_canvas = canvas;
    /*  Pass on event  */
    return (FALSE);
}   /*  End Function dummy_pos_consumer  */

static void free_data (ThreeDeeSliceWidget top)
/*  [PURPOSE] This routine will free all ViewableImage objects.
    <top> The ThreeDeeSlice widget.
    [RETURNS] Nothing.
*/
{
    int count;

    /*  There is no point trying to free other data if the cube does not exist,
	since there is no way of knowing the dimension lengths and hence the
	number of ViewableImages for each sequence.  */
    if (top->threeDeeSlice.cube == NULL) return;
    /*  Deallocate old data  */
    if (top->threeDeeSlice.xy_frames != NULL)
    {
	for (count = 0;
	     count < iarray_dim_length (top->threeDeeSlice.cube, 0);
	     ++count)
	{
	    if (top->threeDeeSlice.xy_frames[count] != NULL)
	    {
		viewimg_destroy (top->threeDeeSlice.xy_frames[count]);
		top->threeDeeSlice.xy_frames[count] = NULL;
	    }
	}
	m_free ( (char *) top->threeDeeSlice.xy_frames );
	top->threeDeeSlice.xy_frames = NULL;
    }
    if (top->threeDeeSlice.xz_frames != NULL)
    {
	for (count = 0;
	     count < iarray_dim_length (top->threeDeeSlice.cube, 1);
	     ++count)
	{
	    if (top->threeDeeSlice.xz_frames[count] != NULL)
	    {
		viewimg_destroy (top->threeDeeSlice.xz_frames[count]);
		top->threeDeeSlice.xz_frames[count] = NULL;
	    }
	}
	m_free ( (char *) top->threeDeeSlice.xz_frames );
	top->threeDeeSlice.xz_frames = NULL;
    }
    if (top->threeDeeSlice.zy_frames != NULL)
    {
	for (count = 0;
	     count < iarray_dim_length (top->threeDeeSlice.cube, 2);
	     ++count)
	{
	    if (top->threeDeeSlice.zy_frames[count] != NULL)
	    {
		viewimg_destroy (top->threeDeeSlice.zy_frames[count]);
		top->threeDeeSlice.zy_frames[count] = NULL;
	    }
	}
	m_free ( (char *) top->threeDeeSlice.zy_frames );
	top->threeDeeSlice.zy_frames = NULL;
    }
}   /*  End Function free_data  */


/*  Public functions follow  */

void XkwThreeDeeSlicePrecompute (Widget w, XtPointer client_data,
				 XtPointer call_data)
/*  [PURPOSE] This routine will precompute all ViewableImages. This routine may
    be used as a regular callback with the ThreeDeeSlice widget passed as the
    client data.
    <w> Ignored.
    <client_data> The ThreeDeeSlice widget.
    <call_data> Ignored.
    [RETURNS] Nothing.
*/
{
    int count;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) client_data;

    if (top->threeDeeSlice.cube == NULL) return;
    if (top->threeDeeSlice.xy_frames == NULL) return;
    if (top->threeDeeSlice.xz_frames == NULL) return;
    if (top->threeDeeSlice.zy_frames == NULL) return;
    fprintf (stderr, "Precomputing XY frames...\n");
    for (count = iarray_dim_length (top->threeDeeSlice.cube, 0) - 1;
	 count >= 0; --count)
    {
	if ( !viewimg_make_active (top->threeDeeSlice.xy_frames[count]) )
	{
	    fprintf (stderr, "Error making ViewableImage active\n");
	}
    }
    fprintf (stderr, "Precomputing XZ frames...\n");
    for (count = iarray_dim_length (top->threeDeeSlice.cube, 1) - 1;
	 count >= 0; --count)
    {
	if ( !viewimg_make_active (top->threeDeeSlice.xz_frames[count]) )
	{
	    fprintf (stderr, "Error making ViewableImage active\n");
	}
    }
    fprintf (stderr, "Precomputing ZY frames...\n");
    for (count = iarray_dim_length (top->threeDeeSlice.cube, 2) - 1;
	 count >= 0; --count)
    {
	if ( !viewimg_make_active (top->threeDeeSlice.zy_frames[count]) )
	{
	    fprintf (stderr, "Error making ViewableImage active\n");
	}
    }
}   /*  End Function XkwThreeDeeSlicePrecompute  */

void XkwThreeDeeSliceSetCursor (Widget W, unsigned int type,
				double x, double y, double z)
/*  [SUMMARY] Set the 3D cursor position.
    <W> The ThreeDeeSlice widget.
    <type> The type of the supplied co-ordinates.
    <x> The x co-ordinate.
    <y> The y co-ordinate.
    <z> The z co-ordinate.
    [RETURNS] Nothing.
*/
{
    struct XkwThreeDeeSliceCursor cursor;
    ThreeDeeSliceWidget w = (ThreeDeeSliceWidget) W;
    static char function_name[] = "XkwThreeDeeSliceSetCursor";

    if ( !XtIsThreeDeeSlice (W) )
    {
	fprintf (stderr, "Bad widget passed\n");
	a_prog_bug (function_name);
    }
    fillin_cursor (w, &cursor, type, x, y, z);
    move_cursor (w, cursor);
}   /*  End Function XkwThreeDeeSliceSetCursor  */

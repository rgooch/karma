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

    Last updated by Richard Gooch   7-JAN-1996: Added cursorCallback resource.


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
#include <karma_viewimg.h>
#include <karma_xc.h>
#include <karma_m.h>
#include <Xkw/ThreeDeeSliceP.h>
#include <Xkw/Canvas.h>
#include <Xkw/Value.h>

STATIC_FUNCTION (void Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (void Realise, (Widget w, Mask *valueMask,
				XSetWindowAttributes *attributes) );
STATIC_FUNCTION (Boolean SetValues,
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
STATIC_FUNCTION (flag xy_pos_consumer,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info, void **f_info,
		  double x_lin, double y_lin) );
STATIC_FUNCTION (flag xz_pos_consumer,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info, void **f_info,
		  double x_lin, double y_lin) );
STATIC_FUNCTION (flag zy_pos_consumer,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info, void **f_info,
		  double x_lin, double y_lin) );
STATIC_FUNCTION (void register_new_cube, (ThreeDeeSliceWidget top, iarray cube,
					  double min, double max) );
STATIC_FUNCTION (void iarr_destroy_func,
		 (iarray arr, ThreeDeeSliceWidget top) );
STATIC_FUNCTION (flag dummy_pos_consumer,
		 (KPixCanvas canvas, int x, int y,
		  unsigned int event_code, void *e_info, void **f_info) );
STATIC_FUNCTION (void free_data, (ThreeDeeSliceWidget top) );


#define offset(field) XtOffsetOf(ThreeDeeSliceRec, threeDeeSlice.field)

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
	(XtInitProc) Initialise,        /*  initialise             */
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
	(XtSetValuesFunc) SetValues,    /*  set_values             */
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

static void Initialise (Widget Request, Widget New)
{
    Kdisplay dpy_handle;
    Colormap xcmap;
    ThreeDeeSliceWidget request = (ThreeDeeSliceWidget) Request;
    ThreeDeeSliceWidget new = (ThreeDeeSliceWidget) New;
    int canvas_types;
    Widget canvas;
    Widget x_mag_sld, y_mag_sld, z_mag_sld;
    struct dual_crosshair_type *crosshairs;
    static char function_name[] = "ThreeDeeSliceWidget::Initialise";

    kcmap_get_attributes (new->threeDeeSlice.karmaCmap,
			  KCMAP_ATT_DPY_HANDLE, &dpy_handle,
			  NULL);
    xcmap = xc_get_cmap (dpy_handle);
    new->threeDeeSlice.x_pos = 0.0;
    new->threeDeeSlice.y_pos = 0.0;
    new->threeDeeSlice.z_pos = 0.0;
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
    x_mag_sld = XtVaCreateManagedWidget ("slider", valueWidgetClass, New,
					 XtNlabel, "X Magnification",
					 XtNorientation, XtorientHorizontal,
					 XkwNminimum, 1,
					 XkwNmaximum, 10,
					 XkwNmodifier, 1,
					 XtNvalue, 1,
					 XkwNvaluePtr,
					 &new->threeDeeSlice.x_mag,
					 NULL);
    XtAddCallback (x_mag_sld, XkwNvalueChangeCallback, redisplay_cbk,
		   (XtPointer) new);
    y_mag_sld = XtVaCreateManagedWidget ("slider", valueWidgetClass, New,
					 XtNlabel, "Y Magnification",
					 XtNfromVert, x_mag_sld,
					 XtNorientation, XtorientHorizontal,
					 XkwNminimum, 1,
					 XkwNmaximum, 10,
					 XkwNmodifier, 1,
					 XtNvalue, 1,
					 XkwNvaluePtr,
					 &new->threeDeeSlice.y_mag,
					 NULL);
    XtAddCallback (y_mag_sld, XkwNvalueChangeCallback, redisplay_cbk,
		   (XtPointer) new);
    z_mag_sld = XtVaCreateManagedWidget ("slider", valueWidgetClass, New,
					 XtNlabel, "Z Magnification",
					 XtNfromVert, y_mag_sld,
					 XtNorientation, XtorientHorizontal,
					 XkwNminimum, 1,
					 XkwNmaximum, 10,
					 XkwNmodifier, 1,
					 XtNvalue, 1,
					 XkwNvaluePtr,
					 &new->threeDeeSlice.z_mag,
					 NULL);
    XtAddCallback (z_mag_sld, XkwNvalueChangeCallback, redisplay_cbk,
		   (XtPointer) new);
    canvas = XtVaCreateManagedWidget ("canvas", canvasWidgetClass, New,
				      XtNfromVert, z_mag_sld,
				      XtNwidth, 512,
				      XtNheight, 512,
				      XtNmappedWhenManaged, True,
				      XkwNsilenceUnconsumed, True,
				      XkwNretainFgBg, True,
				      XtNvisual,
				      new->threeDeeSlice.canvasVisual,
				      XtNcolormap, xcmap,
				      NULL);
    XtAddCallback (canvas, XkwNrealiseCallback, realise_cbk, (XtPointer) new);
}   /*  End Function Initialise  */

static Boolean SetValues (Widget Current, Widget Request, Widget New)
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
		    (void) fprintf (stderr, "Error computing min-max\n");
		}
	    }
	    else
	    {
		min = *new->threeDeeSlice.minPtr;
		max = *new->threeDeeSlice.maxPtr;
	    }
	    if (verbose) (void) fprintf (stderr, "%s: min: %e  max: %e\n",
					 function_name, min, max);
	    register_new_cube (new, new->threeDeeSlice.cube, min, max);
	}
    }
    new->threeDeeSlice.minPtr = NULL;
    new->threeDeeSlice.maxPtr = NULL;
    return False;
}   /*  End Function SetValues  */

static void realise_cbk (w, client_data, call_data)
/*  This is the canvas realise callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    KPixCanvas parent = (KPixCanvas) call_data;
    KPixCanvas xy_pixcanvas, xz_pixcanvas, zy_pixcanvas;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) client_data;
    XGCValues gcvalues;
    XColor scrn_def, exact_def;
    Colormap xcmap;
    Display *dpy;
    struct win_scale_type win_scale;
    static char function_name[] = "ThreeDeeSliceWidget::realise_cbk";

    top->threeDeeSlice.parent_pixcanvas = parent;
    XtVaGetValues (w,
		   XtNcolormap, &xcmap,
		   XtNbackground, &gcvalues.background,
		   NULL);
    kwin_register_refresh_func (parent, parent_refresh_func, top);
    /*  Create the pixel canvases  */
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
	(void) fprintf (stderr,
			"Error allocating black in colourmap\n");
	exit (RV_UNDEF_ERROR);
    }
    win_scale.min_sat_pixel = scrn_def.pixel;
    if (XAllocNamedColor (XtDisplay (w), xcmap, "White", &scrn_def,
			  &exact_def) == 0)
    {
	(void) fprintf (stderr,
			"Error allocating white in colourmap\n");
	exit (RV_UNDEF_ERROR);
    }
    win_scale.max_sat_pixel = scrn_def.pixel;
    win_scale.blank_pixel = gcvalues.background;
    /*  Create the main world canvas (with dummy world min and max)  */
/*
    win_scale.iscale_func = log_iscale_func;
*/
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
    canvas_set_dressing (top->threeDeeSlice.xy_worldcanvas,
			 KCD_DISPLAY, TRUE,
			 KCD_END);
    canvas_register_position_event_func (top->threeDeeSlice.xy_worldcanvas,
					 xy_pos_consumer, top);
    viewimg_init (top->threeDeeSlice.xy_worldcanvas);
#ifdef dummy
    viewimg_register_position_event_func (xy_worldcanvas,xy_track_canvas_event,
					  (void *) xy_worldcanvas);
#endif
    viewimg_set_canvas_attributes (top->threeDeeSlice.xy_worldcanvas,
				   VIEWIMG_ATT_AUTO_V, FALSE,
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
    canvas_set_dressing (top->threeDeeSlice.xz_worldcanvas,
			 KCD_DISPLAY, TRUE,
			 KCD_END);
    canvas_register_position_event_func (top->threeDeeSlice.xz_worldcanvas,
					 xz_pos_consumer, top);
    viewimg_init (top->threeDeeSlice.xz_worldcanvas);
#ifdef dummy
    viewimg_register_position_event_func (xz_worldcanvas,xz_track_canvas_event,
					  (void *) xz_worldcanvas);
#endif
    viewimg_set_canvas_attributes (top->threeDeeSlice.xz_worldcanvas,
				   VIEWIMG_ATT_AUTO_V, FALSE,
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
    canvas_set_dressing (top->threeDeeSlice.zy_worldcanvas,
			 KCD_DISPLAY, TRUE,
			 KCD_END);
    canvas_register_position_event_func (top->threeDeeSlice.zy_worldcanvas,
					 zy_pos_consumer, top);
    viewimg_init (top->threeDeeSlice.zy_worldcanvas);
#ifdef dummy
    viewimg_register_position_event_func (zy_worldcanvas,zy_track_canvas_event,
					  (void *) zy_worldcanvas);
#endif
    viewimg_set_canvas_attributes (top->threeDeeSlice.zy_worldcanvas,
				   VIEWIMG_ATT_AUTO_V, FALSE,
				   VIEWIMG_ATT_END);
    canvas_register_refresh_func (top->threeDeeSlice.zy_worldcanvas,
				  zy_canvas_refresh_func, top);
}   /*  End Function realise_cbk   */

static void redisplay_cbk (w, client_data, call_data)
/*  This is the redisplay callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
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
    int x_off, y_off;
    int lengths[3];
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *info;

    if (top->threeDeeSlice.cube == NULL) return;
    lengths[0] = iarray_dim_length (top->threeDeeSlice.cube,
				    0) * top->threeDeeSlice.z_mag;
    lengths[1] = iarray_dim_length (top->threeDeeSlice.cube,
				    1) * top->threeDeeSlice.y_mag;
    lengths[2] = iarray_dim_length (top->threeDeeSlice.cube,
				    2) * top->threeDeeSlice.x_mag;
    if ( (lengths[2] + lengths[0] + 3 > width) ||
	(lengths[1] + lengths[0] + 3 > height) ) return;
    kwin_set_attributes (top->threeDeeSlice.xy_pixcanvas,
			 KWIN_ATT_VISIBLE, TRUE,
			 KWIN_ATT_END);
    kwin_set_attributes (top->threeDeeSlice.xz_pixcanvas,
			 KWIN_ATT_VISIBLE, TRUE,
			 KWIN_ATT_END);
    kwin_set_attributes (top->threeDeeSlice.zy_pixcanvas,
			 KWIN_ATT_VISIBLE, TRUE,
			 KWIN_ATT_END);
    x_off = ( width - (lengths[2] + lengths[0] + 3) ) / 2;
    y_off = ( height - (lengths[1] + lengths[0] + 3) ) / 2;
    kwin_resize (top->threeDeeSlice.xy_pixcanvas, FALSE, x_off, y_off,
		 lengths[2] + 2, lengths[1] + 2);
    kwin_resize (top->threeDeeSlice.xz_pixcanvas, FALSE,
		 x_off, y_off + lengths[1] + 1,
		 lengths[2] + 2, lengths[0] + 2);
    kwin_resize (top->threeDeeSlice.zy_pixcanvas, FALSE,
		 x_off + lengths[2] + 1, y_off,
		 lengths[0] + 2, lengths[1] + 2);
    
}   /*  End Function parent_refresh_func  */

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
    unsigned long pixel_value;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *info;

    if ( !canvas_get_colour (canvas, "red", &pixel_value,
			     (unsigned short *) NULL, (unsigned short *) NULL,
			     (unsigned short *) NULL) ) return;
    canvas_draw_line_p (canvas,
			win_scale->x_min, top->threeDeeSlice.y_pos,
			win_scale->x_max, top->threeDeeSlice.y_pos,
			pixel_value);
    canvas_draw_line_p (canvas,
			top->threeDeeSlice.x_pos, win_scale->y_min,
			top->threeDeeSlice.x_pos, win_scale->y_max,
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
    unsigned long pixel_value;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *info;

    if ( !canvas_get_colour (canvas, "red", &pixel_value,
			     (unsigned short *) NULL, (unsigned short *) NULL,
			     (unsigned short *) NULL) ) return;
    canvas_draw_line_p (canvas,
			win_scale->x_min, top->threeDeeSlice.z_pos,
			win_scale->x_max, top->threeDeeSlice.z_pos,
			pixel_value);
    canvas_draw_line_p (canvas,
			top->threeDeeSlice.x_pos, win_scale->y_min,
			top->threeDeeSlice.x_pos, win_scale->y_max,
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
    unsigned long pixel_value;
    double x;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *info;

    if ( !canvas_get_colour (canvas, "red", &pixel_value,
			     (unsigned short *) NULL, (unsigned short *) NULL,
			     (unsigned short *) NULL) ) return;
    /*  Convert from 3-D Z to 2-D X  */
    x = win_scale->x_max - top->threeDeeSlice.z_pos + win_scale->x_min;
    if (x < win_scale->x_min) x = win_scale->x_min;
    if (x > win_scale->x_max) x = win_scale->x_max;
    canvas_draw_line_p (canvas, x, win_scale->y_min, x, win_scale->y_max,
			pixel_value);
    canvas_draw_line_p (canvas,
			win_scale->x_min, top->threeDeeSlice.y_pos,
			win_scale->x_max, top->threeDeeSlice.y_pos,
			pixel_value);
}   /*  End Function zy_canvas_refresh_func  */

static flag xy_pos_consumer (KWorldCanvas canvas, double x, double y,
			     unsigned int event_code, void *e_info,
			     void **f_info, double x_lin, double y_lin)
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
    Kcoord_3d cursor;
    unsigned int pos;
    dim_desc *dim;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *f_info;

    if (event_code == K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK)
    {
	(void) fprintf (stderr, "X: %e  Y: %e  Z: %e\n",
			x, y, top->threeDeeSlice.z_pos);
	return (TRUE);
    }
    if ( (event_code != K_CANVAS_EVENT_LEFT_MOUSE_CLICK) &&
	(event_code != K_CANVAS_EVENT_LEFT_MOUSE_DRAG) ) return (FALSE);
    if (top->threeDeeSlice.cube == NULL) return (TRUE);
    if (top->threeDeeSlice.zy_frames == NULL) return (TRUE);
    if (top->threeDeeSlice.xz_frames == NULL) return (TRUE);
    top->threeDeeSlice.x_pos = x;
    top->threeDeeSlice.y_pos = y;
    dim = iarray_get_dim_desc (top->threeDeeSlice.cube, 2);
    pos = ds_get_coord_num (dim, x, SEARCH_BIAS_CLOSEST);
    if ( !viewimg_set_active (top->threeDeeSlice.zy_frames[pos], TRUE) )
    {
	(void) fprintf (stderr, "Error making ViewableImage active\n");
    }
    dim = iarray_get_dim_desc (top->threeDeeSlice.cube, 1);
    pos = ds_get_coord_num (dim, y, SEARCH_BIAS_CLOSEST);
    if ( !viewimg_set_active (top->threeDeeSlice.xz_frames[pos], TRUE) )
    {
	(void) fprintf (stderr, "Error making ViewableImage active\n");
    }
    kwin_resize (top->threeDeeSlice.xy_pixcanvas, FALSE, 0, 0, 0, 0);
    cursor.x = top->threeDeeSlice.x_pos;
    cursor.y = top->threeDeeSlice.y_pos;
    cursor.z = top->threeDeeSlice.z_pos;
    XtCallCallbacks ( (Widget) top, XkwNcursorCallback, (XtPointer) &cursor );
    return (TRUE);
}   /*  End Function xy_pos_consumer  */

static flag xz_pos_consumer (KWorldCanvas canvas, double x, double y,
			     unsigned int event_code, void *e_info,
			     void **f_info, double x_lin, double y_lin)
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
    Kcoord_3d cursor;
    unsigned int pos;
    dim_desc *dim;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *f_info;

    if (event_code == K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK)
    {
	(void) fprintf (stderr, "X: %e  Y: %e  Z: %e\n",
			x, top->threeDeeSlice.y_pos, y);
	return (TRUE);
    }
    if ( (event_code != K_CANVAS_EVENT_LEFT_MOUSE_CLICK) &&
	(event_code != K_CANVAS_EVENT_LEFT_MOUSE_DRAG) ) return (FALSE);
    if (top->threeDeeSlice.cube == NULL) return (TRUE);
    if (top->threeDeeSlice.xy_frames == NULL) return (TRUE);
    top->threeDeeSlice.z_pos = y;
    dim = iarray_get_dim_desc (top->threeDeeSlice.cube, 0);
    pos = ds_get_coord_num (dim, y, SEARCH_BIAS_CLOSEST);
    if ( !viewimg_make_active (top->threeDeeSlice.xy_frames[pos]) )
    {
	(void) fprintf (stderr, "Error making ViewableImage active\n");
    }
    kwin_resize (top->threeDeeSlice.xz_pixcanvas, FALSE, 0, 0, 0, 0);
    kwin_resize (top->threeDeeSlice.zy_pixcanvas, FALSE, 0, 0, 0, 0);
    cursor.x = top->threeDeeSlice.x_pos;
    cursor.y = top->threeDeeSlice.y_pos;
    cursor.z = top->threeDeeSlice.z_pos;
    XtCallCallbacks ( (Widget) top, XkwNcursorCallback, (XtPointer) &cursor );
    return (TRUE);
}   /*  End Function xz_pos_consumer  */

static flag zy_pos_consumer (KWorldCanvas canvas, double x, double y,
			     unsigned int event_code, void *e_info,
			     void **f_info, double x_lin, double y_lin)
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
    Kcoord_3d cursor;
    unsigned int pos;
    double z, x_min, x_max;
    dim_desc *dim;
    ThreeDeeSliceWidget top = (ThreeDeeSliceWidget) *f_info;

    dim = iarray_get_dim_desc (top->threeDeeSlice.cube, 0);
    /*  Convert 2-D X to 3-D Z  */
    canvas_get_attributes (canvas,
			   CANVAS_ATT_X_MIN, &x_min,
			   CANVAS_ATT_X_MAX, &x_max,
			   CANVAS_ATT_END);
    z = x_max - x + x_min;
    if (z < x_min) z = x_min;
    if (z > x_max) z = x_max;
    if (event_code == K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK)
    {
	(void) fprintf (stderr, "X: %e  Y: %e  Z: %e\n",
			top->threeDeeSlice.x_pos, y, z);
	return (TRUE);
    }
    if ( (event_code != K_CANVAS_EVENT_LEFT_MOUSE_CLICK) &&
	(event_code != K_CANVAS_EVENT_LEFT_MOUSE_DRAG) ) return (FALSE);
    if (top->threeDeeSlice.cube == NULL) return (TRUE);
    if (top->threeDeeSlice.xy_frames == NULL) return (TRUE);
    top->threeDeeSlice.z_pos = z;
    pos = ds_get_coord_num (dim, z, SEARCH_BIAS_CLOSEST);
    if ( !viewimg_make_active (top->threeDeeSlice.xy_frames[pos]) )
    {
	(void) fprintf (stderr, "Error making ViewableImage active\n");
    }
    kwin_resize (top->threeDeeSlice.xz_pixcanvas, FALSE, 0, 0, 0, 0);
    kwin_resize (top->threeDeeSlice.zy_pixcanvas, FALSE, 0, 0, 0, 0);
    cursor.x = top->threeDeeSlice.x_pos;
    cursor.y = top->threeDeeSlice.y_pos;
    cursor.z = top->threeDeeSlice.z_pos;
    XtCallCallbacks ( (Widget) top, XkwNcursorCallback, (XtPointer) &cursor );
    return (TRUE);
}   /*  End Function zy_pos_consumer  */

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
    double dummy;

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
    /*  Create new ViewableImage objects  */
    if ( ( top->threeDeeSlice.xy_frames =
	  viewimg_create_sequence (top->threeDeeSlice.xy_worldcanvas,
				   cube->multi_desc, cube->arr_desc,
				   cube->data, 2, 1, 0, cube->elem_index) )
	== NULL )
    {
	(void) fprintf (stderr, "Error creating ViewableImage sequence\n");
	free_data (top);
	return;
    }
    if ( ( top->threeDeeSlice.xz_frames =
	  viewimg_create_sequence (top->threeDeeSlice.xz_worldcanvas,
				   cube->multi_desc, cube->arr_desc,
				   cube->data, 2, 0, 1, cube->elem_index) )
	== NULL )
    {
	(void) fprintf (stderr, "Error creating ViewableImage sequence\n");
	free_data (top);
	return;
    }
    if ( ( top->threeDeeSlice.zy_frames =
	  viewimg_create_sequence (top->threeDeeSlice.zy_worldcanvas,
				   cube->multi_desc, cube->arr_desc,
				   cube->data, 0, 1, 2, cube->elem_index) )
	== NULL )
    {
	(void) fprintf (stderr, "Error creating ViewableImage sequence\n");
	free_data (top);
	return;
    }
    iarray_get_world_coords (cube, 0, &top->threeDeeSlice.z_pos, &dummy);
    iarray_get_world_coords (cube, 1, &top->threeDeeSlice.y_pos, &dummy);
    iarray_get_world_coords (cube, 2, &top->threeDeeSlice.x_pos, &dummy);
    if ( !viewimg_set_active (top->threeDeeSlice.xy_frames[0], FALSE) )
    {
	(void) fprintf (stderr, "Error making ViewableImage active\n");
	return;
    }
    if ( !viewimg_set_active (top->threeDeeSlice.xz_frames[0], FALSE) )
    {
	(void) fprintf (stderr, "Error making ViewableImage active\n");
	return;
    }
    if ( !viewimg_set_active (top->threeDeeSlice.zy_frames[0], FALSE) )
    {
	(void) fprintf (stderr, "Error making ViewableImage active\n");
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
    if (verbose) (void) fprintf (stderr,
				 "ThreeDeeSliceWidget::iarray destroyed\n");
    /*  Must free the other data first else we will be left with a bunch of
	ViewableImages floating around.  */
    free_data (top);
    top->threeDeeSlice.cube = NULL;
    top->threeDeeSlice.iarr_destroy_func = NULL;
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
    (void) fprintf (stderr, "Precomputing XY frames...\n");
    for (count = iarray_dim_length (top->threeDeeSlice.cube, 0) - 1;
	 count >= 0; --count)
    {
	if ( !viewimg_make_active (top->threeDeeSlice.xy_frames[count]) )
	{
	    (void) fprintf (stderr, "Error making ViewableImage active\n");
	}
    }
    (void) fprintf (stderr, "Precomputing XZ frames...\n");
    for (count = iarray_dim_length (top->threeDeeSlice.cube, 1) - 1;
	 count >= 0; --count)
    {
	if ( !viewimg_make_active (top->threeDeeSlice.xz_frames[count]) )
	{
	    (void) fprintf (stderr, "Error making ViewableImage active\n");
	}
    }
    (void) fprintf (stderr, "Precomputing ZY frames...\n");
    for (count = iarray_dim_length (top->threeDeeSlice.cube, 2) - 1;
	 count >= 0; --count)
    {
	if ( !viewimg_make_active (top->threeDeeSlice.zy_frames[count]) )
	{
	    (void) fprintf (stderr, "Error making ViewableImage active\n");
	}
    }
}   /*  End Function XkwThreeDeeSlicePrecompute  */

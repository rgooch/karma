/*LINTLIBRARY*/
/*  Dataclip.c

    This code provides a data clip control widget for Xt.

    Copyright (C) 1994,1995  Richard Gooch

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

    Last updated by Richard Gooch   6-JAN-1995: Changed from passing flag
  by value to by reference for intensityScaleCallback.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#define X11
#include <Xkw/DataclipP.h>

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

#include <Xkw/Value.h>
#include <Xkw/Canvas.h>

#include <sys/wait.h>
#include <signal.h>
#include <k_event_codes.h>
#include <karma_m.h>
#include <karma_a.h>


/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

/* Methods*/

STATIC_FUNCTION (void Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (void Destroy, (Widget w) );
STATIC_FUNCTION (Boolean SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void close_cbk, (Widget w, XtPointer client_data,
				  XtPointer call_data) );
STATIC_FUNCTION (void intensity_scale_cbk, (Widget w, XtPointer client_data,
					    XtPointer call_data) );
STATIC_FUNCTION (void immediate_apply_cbk, (Widget w, XtPointer client_data,
					    XtPointer call_data) );
STATIC_FUNCTION (void apply_cbk, (Widget w, XtPointer client_data,
				  XtPointer call_data) );
STATIC_FUNCTION (KWorldCanvas setup_canvas,
		 (KPixCanvas parent, Display *dpy, DataclipWidget w) );
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


/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define offset(field) XtOffset(DataclipWidget, dataclip.field)

static XtResource DataclipResources[] = 
{
    {XkwNiarray, XkwCIarray, XtRPointer, sizeof (XtPointer),
     offset (array), XtRImmediate, NULL},
    {XkwNintensityScaleCallback, XtCCallback, XtRCallback, sizeof (caddr_t),
     offset (intensityScaleCallback), XtRCallback, (caddr_t) NULL},
    {XkwNmaxDataRegions, XkwCMaxDataRegions, XtRInt, sizeof (int),
     offset (max_regions), XtRImmediate, (XtPointer) 1},
    {XkwNshowIscaleButton, XkwCShowIscaleButton, XtRBool, sizeof (Boolean),
     offset (show_intensity_scale_button), XtRImmediate, (XtPointer) False},
    {XkwNregionCallback, XtCCallback, XtRCallback, sizeof (caddr_t),
     offset (regionCallback), XtRCallback, (caddr_t) NULL},
    {XkwNautoValueScale, XkwCAutoValueScale, XtRBoolean, sizeof (Boolean),
     offset (auto_v), XtRImmediate, (XtPointer) True},
};

#undef TheOffset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

DataclipClassRec dataclipClassRec = 
{
  {
      /* CoreClassPart */
    (WidgetClass)&topLevelShellClassRec,  /* superclass */
    "Dataclip",                    /* class_name */
    sizeof (DataclipRec),           /* widget_size */
    NULL,                          /* class_initialise */
    NULL,                          /* class_part_initialise */
    FALSE,                         /* class_init */
    (XtInitProc) Initialise,       /* initialise */
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
    Destroy,                       /* destroy */
    XtInheritResize,               /* resize */
    NULL,                          /* expose */
    (XtSetValuesFunc) SetValues,   /* set_values */
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
  {  /* DataClipClassPart */
    0 /* empty */
  }
};

WidgetClass dataclipWidgetClass = (WidgetClass) &dataclipClassRec;

/*----------------------------------------------------------------------*/
/* Initialisation method                                                */
/*----------------------------------------------------------------------*/

static void Initialise (Widget Request, Widget New)
{
    DataclipWidget request = (DataclipWidget) Request;
    DataclipWidget new = (DataclipWidget) New;
    Widget form;
    Widget close_btn, scale_btn, immediate_apply_btn, apply_btn;
    Widget cnv;
    Widget w;
    char *txt;
    static char function_name[] = "DataclipWidget::Initialise";

    (*new).dataclip.pixcanvas = NULL;
    (*new).dataclip.worldcanvas = NULL;
    (*new).dataclip.array = NULL;
    (*new).dataclip.immediate_apply = FALSE;
    (*new).dataclip.last_was_left = FALSE;
    (*new).dataclip.left_pos = TOOBIG;
    (*new).dataclip.popped_up = FALSE;
    XtAddCallback (New, XtNpopupCallback, pop_cbk, (XtPointer) TRUE);
    XtAddCallback (New, XtNpopdownCallback, pop_cbk, (XtPointer) FALSE);
    if ( (*new).dataclip.max_regions < 1 )
    {
	(void) fprintf (stderr, "maxDataRegions: %u  must be greater than 0\n",
			(*new).dataclip.max_regions );
	a_prog_bug (function_name);
    }
    if ( ( (*new).dataclip.minima = (double *) m_alloc
	  (sizeof *(*new).dataclip.minima * (*new).dataclip.max_regions) )
	== NULL )
    {
	m_abort (function_name, "array of minima");
    }
    if ( ( (*new).dataclip.maxima = (double *) m_alloc
	  (sizeof *(*new).dataclip.maxima * (*new).dataclip.max_regions) )
	== NULL )
    {
	m_abort (function_name, "array of maxima");
    }
    (*new).dataclip.num_regions = 0;
    (*new).dataclip.histogram_array = NULL;
    (*new).dataclip.hist_arr_length = 0;
    (*new).dataclip.hist_buf_length = 0;
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
    XtAddCallback (close_btn, XtNcallback, close_cbk, New);
    if ( (*new).dataclip.show_intensity_scale_button )
    {
	if ( (*new).dataclip.auto_v )
	{
	    txt = "Auto Intensity Scale  ";
	}
	else
	{
	    txt = "Manual Intensity Scale";
	}
	w = XtVaCreateManagedWidget ("iscaleToggle", commandWidgetClass, form,
				     XtNlabel, txt,
				     XtNfromHoriz, close_btn,
				     XtNtop, XtChainTop,
				     XtNbottom, XtChainTop,
				     XtNleft, XtChainLeft,
				     XtNheight, 20,
				     NULL);
	scale_btn = w;
	XtAddCallback (scale_btn, XtNcallback, intensity_scale_cbk,
		       (XtPointer) New);
    }
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
				 XtNlabel, "Apply",
				 XtNfromVert, close_btn,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNleft, XtChainLeft,
				 XtNheight, 20,
				 NULL);
    apply_btn = w;
    XtAddCallback (apply_btn, XtNcallback, apply_cbk, New);
    if ( (*new).dataclip.max_regions == 1 )
    {
	w = XtVaCreateManagedWidget ("toggle", commandWidgetClass, form,
				     XtNlabel, "Manual Apply",
				     XtNfromHoriz, w,
				     XtNfromVert, close_btn,
				     XtNtop, XtChainTop,
				     XtNbottom, XtChainTop,
				     XtNleft, XtChainLeft,
				     XtNheight, 20,
				     NULL);
	immediate_apply_btn = w;
	XtAddCallback (immediate_apply_btn, XtNcallback, immediate_apply_cbk,
		       (XtPointer) New);
    }
    w = XtVaCreateManagedWidget ("label", labelWidgetClass, form,
				 XtNlabel, "Data Minimum:              ",
				 XtNborderWidth, 0,
				 XtNfromVert, w,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNheight, 20,
				 XtNresizable, True,
				 NULL);
    (*new).dataclip.min_label = w;
    w = XtVaCreateManagedWidget ("label", labelWidgetClass, form,
				 XtNlabel, "Data Maximum:              ",
				 XtNborderWidth, 0,
				 XtNfromVert, w,
				 XtNtop, XtChainTop,
				 XtNbottom, XtChainTop,
				 XtNheight, 20,
				 XtNresizable, True,
				 NULL);
    (*new).dataclip.max_label = w;
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
    (*new).dataclip.cnv = cnv;
}

/*----------------------------------------------------------------------*/
/* Destroy method                                                       */
/*----------------------------------------------------------------------*/

static void Destroy(Widget W)
{
  /* Nothing to destroy */
}

/*----------------------------------------------------------------------*/
/* SetValues method                                                     */
/*----------------------------------------------------------------------*/

static Boolean SetValues(Widget Current, Widget Request, Widget New)
{
    KWorldCanvas worldcanvas;
    DataclipWidget current = (DataclipWidget) Current;
    DataclipWidget request = (DataclipWidget) Request;
    DataclipWidget new = (DataclipWidget) New;
    static char function_name[] = "DataclipWidget::SetValues";

    if ( (*new).dataclip.max_regions != (*current).dataclip.max_regions )
    {
	(void) fprintf (stderr, "Cannot change number of regions\n");
	a_prog_bug (function_name);
    }
    if ( (*new).dataclip.pixcanvas == NULL )
    {
	XtVaGetValues ( (*new).dataclip.cnv,
		       XkwNpixCanvas, &(*new).dataclip.pixcanvas,
		       NULL );
	if ( (*new).dataclip.pixcanvas == NULL )
	{
	    (void) fprintf (stderr, "Canvas not realised yet!\n");
	    a_prog_bug (function_name);
	}
	worldcanvas = setup_canvas ( (*new).dataclip.pixcanvas,
				    XtDisplay (New), new );
	(*new).dataclip.worldcanvas = worldcanvas;
    }
    if ( ( (*new).dataclip.array != NULL ) &&
	( (*new).dataclip.array != (*current).dataclip.array ) )
    {
	(*new).dataclip.data_min = TOOBIG;
	(*new).dataclip.data_max = -TOOBIG;
	/*  Force computation of histogram  */
	(*new).dataclip.hist_arr_length = 0;
	if ( (*new).dataclip.popped_up )
	{
	    kwin_resize ( (*new).dataclip.pixcanvas, TRUE, 0, 0, 0, 0 );
	}
    }
    return False;
}   /*  End Function SetValues  */

static void close_cbk (w, client_data, call_data)
/*  This is the generic close button callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    Widget popup = (Widget) client_data;

    XtPopdown (popup);
}   /*  End Function close_cbk   */

/*----------------------------------------------------------------------*/
/* Callback for Auto/Manual button                                      */
/*----------------------------------------------------------------------*/

static void intensity_scale_cbk (Widget w, XtPointer client_data,
				 XtPointer call_data)
{
    flag bool;
    DataclipWidget top = (DataclipWidget) client_data;
    char label[STRING_LENGTH];

    if ( (*top).dataclip.auto_v )
    {
	(void) sprintf (label, "Manual Intensity Scale");
	(*top).dataclip.auto_v = FALSE;
    }
    else
    {
	(void) sprintf (label, "Auto Intensity Scale");
	(*top).dataclip.auto_v = TRUE;
    }
    bool = (*top).dataclip.auto_v;
    XtVaSetValues (w, XtNlabel, label, NULL);
    XtCallCallbacks ( (Widget) top, XkwNintensityScaleCallback,
		     (XtPointer) &bool );
}

/*----------------------------------------------------------------------*/
/* Callback for Immediate/Manual apply button                           */
/*----------------------------------------------------------------------*/

static void immediate_apply_cbk (Widget w, XtPointer client_data,
				 XtPointer call_data)
{
    DataclipWidget top = (DataclipWidget) client_data;
    char label[STRING_LENGTH];

    if ( (*top).dataclip.immediate_apply )
    {
	(void) sprintf (label, "Manual Apply");
	(*top).dataclip.immediate_apply = FALSE;
    }
    else
    {
	(void) sprintf (label, "Auto Apply");
	(*top).dataclip.immediate_apply = TRUE;
    }
    XtVaSetValues (w, XtNlabel, label, NULL);
}

/*----------------------------------------------------------------------*/
/* Callback for apply button                                            */
/*----------------------------------------------------------------------*/

static void apply_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    DataclipWidget top = (DataclipWidget) client_data;
    DataclipRegions regions;

    regions.num_regions = (*top).dataclip.num_regions;
    if ( ( regions.minima = (double *) m_dup ( (char *) (*top).dataclip.minima,
					      sizeof *(*top).dataclip.minima *
					      regions.num_regions ) )
	== NULL ) return;
    if ( ( regions.maxima = (double *) m_dup ( (char *) (*top).dataclip.maxima,
					      sizeof *(*top).dataclip.maxima *
					      regions.num_regions ) )
	== NULL )
    {
	m_free ( (char *) regions.minima );
	return;
    }
    XtCallCallbacks ( (Widget) top, XkwNregionCallback, (XtPointer) &regions );
    m_free ( (char *) regions.minima );
    m_free ( (char *) regions.maxima );
}

static KWorldCanvas setup_canvas (KPixCanvas pixcanvas, Display *display,
				  DataclipWidget w)
/*  This routine will create the offset control canvas.
    The pixel canvas must be given by  pixcanvas  .
    The X display must be pointed to by  display  .
    The routine returns the KWorldCanvas object.
*/
{
    KWorldCanvas histogram_worldcanvas;
    double frac;
    GC gc;
    XGCValues gcvalues;
    struct win_scale_type win_scale;
    static char function_name[] = "setup_canvas";

    /*  Get the Graphics Context of the pixel canvas  */
    gc = kwin_get_gc_x (pixcanvas);
    if (XGetGCValues (display, gc, GCForeground | GCBackground, &gcvalues)
	== 0)
    {
	(void) fprintf (stderr, "Error getting GC values\n");
	exit (RV_UNDEF_ERROR);
    }
    /*  Create the offset control world canvas  */
    canvas_init_win_scale (&win_scale, K_WIN_SCALE_MAGIC_NUMBER);
    /*  No histogram data yet  */
    win_scale.min_sat_pixel = gcvalues.background;
    win_scale.max_sat_pixel = gcvalues.foreground;
    if ( ( histogram_worldcanvas = canvas_create (pixcanvas, NULL,&win_scale) )
	== NULL )
    {
	exit (RV_UNDEF_ERROR);
    }
    canvas_use_log_scale (histogram_worldcanvas, FALSE, TRUE);
    canvas_register_refresh_func (histogram_worldcanvas,
				  histogram_canvas_refresh_func, (void *) w);
    (void) canvas_register_position_event_func (histogram_worldcanvas,
						histogram_canvas_position_func,
						(void *) w);
    canvas_register_size_control_func (histogram_worldcanvas,
				       size_control_func, (void *) w);
    return (histogram_worldcanvas);
}   /*  End Function setup_canvas  */

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
    static char function_name[] = "histogram_canvas_refresh_func";

    if ( (*w).dataclip.array == NULL ) return;
    canvas_draw_lines_p (canvas, (double *) NULL,
			 (*w).dataclip.histogram_array,
			 (*w).dataclip.hist_arr_length,
			 (*win_scale).max_sat_pixel);
    if (canvas_get_colour (canvas, "yellow4", &pixel_value,
			   (unsigned short *) NULL, (unsigned short *) NULL,
			   (unsigned short *) NULL) != TRUE) return;
    if ( (*w).dataclip.left_pos < TOOBIG )
    {
	x0 = (*w).dataclip.left_pos;
	canvas_draw_line_p (canvas,
			    x0, (*win_scale).y_min, x0, (*win_scale).y_max,
			    pixel_value);
    }
    if ( (*w).dataclip.num_regions < 1 ) return;
    for (count = 0; count < (*w).dataclip.num_regions; ++count)
    {
	x0 = (*w).dataclip.minima[count];
	x1 = (*w).dataclip.maxima[count];
	canvas_draw_line_p (canvas,
			    x0, (*win_scale).y_min, x0, (*win_scale).y_max,
			    pixel_value);
	canvas_draw_line_p (canvas,
			    x1, (*win_scale).y_min, x1, (*win_scale).y_max,
			    pixel_value);
	canvas_draw_line_p (canvas,
			    x0, (*win_scale).y_min, x1, (*win_scale).y_max,
			    pixel_value);
	canvas_draw_line_p (canvas,
			    x1, (*win_scale).y_min, x0, (*win_scale).y_max,
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
    DataclipWidget w = (DataclipWidget) *f_info;

    switch (event_code)
    {
      case K_CANVAS_EVENT_LEFT_MOUSE_CLICK:
	if ( (*w).dataclip.last_was_left )
	{
	    XBell (XtDisplay (w), 100);
	    return (TRUE);
	}
	if ( ( (*w).dataclip.max_regions == 1 ) &&
	    ( (*w).dataclip.num_regions == 1 ) )
	{
	    if (x >= (*w).dataclip.maxima[0])
	    {
		XBell (XtDisplay (w), 100);
		return (TRUE);
	    }
	    (*w).dataclip.minima[0] = x;
	    if ( (*w).dataclip.immediate_apply )
	    {
		/*  This can only happen when  max_regions == 1  */
		apply_cbk ( (Widget) NULL, (XtPointer) w, NULL );
	    }
	}
	else if ( (*w).dataclip.num_regions >= (*w).dataclip.max_regions )
	{
	    XBell (XtDisplay (w), 100);
	    return (TRUE);
	}
	else
	{
	    (*w).dataclip.left_pos = x;
	    (*w).dataclip.last_was_left = TRUE;
	}
	break;
      case K_CANVAS_EVENT_RIGHT_MOUSE_CLICK:
	if ( ( (*w).dataclip.max_regions == 1 ) &&
	    ( (*w).dataclip.num_regions == 1 ) )
	{
	    if (x <= (*w).dataclip.minima[0])
	    {
		XBell (XtDisplay (w), 100);
		return (TRUE);
	    }
	    (*w).dataclip.maxima[0] = x;
	}
	else if ( !(*w).dataclip.last_was_left )
	{
	    XBell (XtDisplay (w), 100);
	    return (TRUE);
	}
	else if ( (*w).dataclip.num_regions >= (*w).dataclip.max_regions )
	{
	    XBell (XtDisplay (w), 100);
	    return (TRUE);
	}
	else if (x <= (*w).dataclip.left_pos)
	{
	    XBell (XtDisplay (w), 100);
	    return (TRUE);
	}
	else
	{
	    (*w).dataclip.minima[(*w).dataclip.num_regions] = (*w).dataclip.left_pos;
	    (*w).dataclip.maxima[(*w).dataclip.num_regions++] = x;
	    (*w).dataclip.last_was_left = FALSE;
	    (*w).dataclip.left_pos = TOOBIG;
	}
	if ( (*w).dataclip.immediate_apply )
	{
	    /*  This can only happen when  max_regions == 1  */
	    apply_cbk ( (Widget) NULL, (XtPointer) w, NULL );
	}
	break;
      default:
	return (FALSE);
/*
	break;
*/
    }
    (void) canvas_resize (canvas, NULL, TRUE);
    return (TRUE);
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
    int array_len = width / 2;
    unsigned int count;
    unsigned long hpeak, hmode;
    double frac;
    double min, max;
    DataclipWidget w = (DataclipWidget) *info;
    char txt[STRING_LENGTH];
    unsigned long *hist_array;
    static char function_name[] = "DataclipWidget::size_control_func";

    if ( (*w).dataclip.array == NULL ) return;
    if ( (*w).dataclip.data_min >= TOOBIG )
    {
	/*  Recompute the minimum and maximum  */
	if ( !iarray_min_max ( (*w).dataclip.array, CONV1_REAL, &min, &max ) )
	{
	    (void) fprintf (stderr, "Error getting image range\n");
	    return;
	}
	if (min >= max)
	{
	    (*w).dataclip.array = NULL;
	    return;
	}
	(*w).dataclip.data_min = min;
	(*w).dataclip.data_max = max;
	(void) sprintf (txt, "Data Minimum: %e", min);
	XtVaSetValues ( (*w).dataclip.min_label,
		       XtNlabel, txt,
		       NULL );
	(void) sprintf (txt, "Data Maximum: %e", max);
	XtVaSetValues ( (*w).dataclip.max_label,
		       XtNlabel, txt,
		       NULL );
    }
    if (array_len == (*w).dataclip.hist_arr_length) return;
    if ( ( hist_array = (unsigned long *)
	  m_alloc_scratch (sizeof *hist_array * array_len, function_name) )
	== NULL )
    {
	m_abort (function_name, "temporary array");
    }
    m_clear ( (char *) hist_array, sizeof *hist_array * array_len );
    if (array_len > (*w).dataclip.hist_buf_length)
    {
	/*  Increase histogram array size  */
	if ( (*w).dataclip.histogram_array != NULL )
	{
	    m_free ( (char *) (*w).dataclip.histogram_array );
	    (*w).dataclip.hist_buf_length = 0;
	}
	if ( ( (*w).dataclip.histogram_array = (double *)
	      m_alloc (sizeof *(*w).dataclip.histogram_array * array_len) )
	    == NULL )
	{
	    m_abort (function_name, "histogram array");
	}
	(*w).dataclip.hist_buf_length = array_len;
    }
    m_clear ( (char *) (*w).dataclip.histogram_array,
	     sizeof *(*w).dataclip.histogram_array * array_len);
    (*w).dataclip.hist_arr_length = array_len;
    (*win_scale).x_min = (*w).dataclip.data_min;
    (*win_scale).x_max = (*w).dataclip.data_max;
    hpeak = 0;
    hmode = 0;
    if ( !iarray_compute_histogram ( (*w).dataclip.array,
				    (*win_scale).conv_type,
				    (*win_scale).x_min, (*win_scale).x_max,
				    array_len, hist_array, &hpeak, &hmode ) )
    {
	(void) fprintf (stderr, "Error computing histogram\n");
	a_prog_bug (function_name);
    }
    if (getuid () == 465)
    {
	(void) fprintf (stderr, "histogram peak: %lu\n", hpeak);
    }
    for (count = 0; count < array_len; ++count)
    {
	(*w).dataclip.histogram_array[count] = hist_array[count];
    }
    m_free_scratch ();
    /*  Have histogram data: setup logarithmic scale  */
    (*win_scale).y_max = hpeak;
    frac = log10 ( (*win_scale).y_max );
    if (frac > 6.0) frac = 6.0;
    (*win_scale).y_min = (*win_scale).y_max / pow (10.0, frac);
    if ( (*win_scale).y_min < 1.0 ) (*win_scale).y_min = 1.0;
    (*win_scale).x_offset = 0;
    (*win_scale).y_offset = 0;
    (*win_scale).x_pixels = width;
    (*win_scale).y_pixels = height;
}   /*  End Function size_control_func  */

static void pop_cbk (w, client_data, call_data)
/*  This is the generic popup/popdown callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    DataclipWidget top = (DataclipWidget) w;

    (*top).dataclip.popped_up = (iaddr) client_data;
}   /*  End Function pop_cbk   */

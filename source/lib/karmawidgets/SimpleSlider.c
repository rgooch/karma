/*LINTLIBRARY*/
/*  SimpleSlider.c

    This code provides a simple slider widget for Xt.

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

    This file contains all routines needed for a simple slider widget for Xt.


    Written by      Richard Gooch   4-MAY-1996

    Updated by      Richard Gooch   6-MAY-1996

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   2-SEP-1996: Check if pixel canvas exists
  prior to refreshing in <set_value> to cope with case where value is set prior
  to widget realisation.

    Updated by      Richard Gooch   21-NOV-1996: Added support for showing
  scaled data.

    Last updated by Richard Gooch   1-DEC-1996: Use <kwin_get_colour> rather
  then <BlackPixel> and <WhitePixel>.


*/

#include <stdio.h>
#define X11
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>
#include <X11/extensions/multibuf.h>
#include <karma.h>
#include <k_event_codes.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <Xkw/SimpleSliderP.h>
#include <os.h>


#define THUMB_WIDTH 8
#define THUMB_HEIGHT 15
#define BAR_HEIGHT 7
#define CLEARANCE 5
#define ENDBOX_WIDTH 6
#define ENDBOX_HEIGHT 11

typedef struct
{
    int label_string_x_start;
    int label_string_x_stop;
    int top_string_y_start;
    int top_string_y_stop;
    int value_string_x_start;
    int value_string_x_stop;
    int value_string_y_start;
    int bottom_string_y_start;
    int min_string_x_start;
    int min_string_x_stop;
    int max_string_x_start;
    int left_endbox_x_start;
    int left_endbox_x_stop;
    int left_endbox_y_start;
    int left_endbox_y_stop;
    int right_endbox_x_start;
    int right_endbox_x_stop;
    int right_endbox_y_start;
    int right_endbox_y_stop;
    int thumb_x_start;
    int thumb_x_stop;
    int thumb_y_start;
    int thumb_y_stop;
    int bar_x_start;
    int bar_x_stop;
    int bar_y_start;
    int bar_y_stop;
} pos_info;


#define TheOffset(field) XtOffsetOf(SimpleSliderRec, slider.field)

static XtResource resources[] =
{
    {XkwNvalueChangeCallback, XtCCallback, XtRCallback, sizeof (XtPointer),
     TheOffset (valueChangeCallback), XtRCallback, (XtPointer) NULL},
    {XtNlabel, XtCLabel, XtRString, sizeof (String),
     TheOffset (label), XtRString, NULL},
    {XkwNminimum, XkwCMinimum, XtRInt, sizeof (int),
     TheOffset (minimum), XtRImmediate, 0},
    {XkwNmaximum, XkwCMaximum, XtRInt, sizeof (int),
     TheOffset (maximum), XtRImmediate, 0},
    {XkwNwrap, XkwCWrap, XtRBool, sizeof (Bool),
     TheOffset (wrap), XtRImmediate, False},
    {XtNvalue, XtCValue, XtRInt, sizeof (int),
     TheOffset (value), XtRImmediate, 0},
    {XkwNvaluePtr, XkwCValuePtr, XtRPointer, sizeof (XtPointer),
     TheOffset (valuePtr), XtRImmediate, (XtPointer) NULL},
    {XkwNmodifier, XkwCModifier, XtRInt, sizeof (int),
     TheOffset (modifier), XtRImmediate, 0},
    {XtNorientation, XtCOrientation, XtROrientation, sizeof (XtOrientation),
     TheOffset (layout), XtRImmediate, (XtPointer) XtorientHorizontal},
    {XkwNshowRange, XkwCShowRange, XtRBool, sizeof (Bool),
     TheOffset (showRange), XtRImmediate, (XtPointer) False},
    {XkwNshowValue, XkwCShowValue, XtRBool, sizeof (Bool),
     TheOffset (showValue), XtRImmediate, (XtPointer) True},
    {XkwNvalueBesideLabel, XkwCValueBesideLabel, XtRBool, sizeof (Bool),
     TheOffset (valueBesideLabel), XtRImmediate, (XtPointer) True},
    {XkwNscaledFormat, XkwCScaledFormat, XtRString, sizeof (String),
     TheOffset (scaledFormat), XtRString, "%.3e"},
    {XkwNscaledUnit, XkwCScaledUnit, XtRString, sizeof (String),
     TheOffset (scaledUnit), XtRString, ""},
    {XkwNcallbackOnDrag, XkwCCallbackOnDrag, XtRBool, sizeof (Bool),
     TheOffset (callbackOnDrag), XtRImmediate, (XtPointer) True},
    {XtNdecay, XtCDecay, XtRInt, sizeof (int),
     TheOffset (decay), XtRImmediate, (XtPointer) SSW_DEF_DECAY},
    {XtNinitialDelay, XtCDelay, XtRInt, sizeof (int),
     TheOffset (initialDelay), XtRImmediate,
     (XtPointer) SSW_DEF_INITIAL_DELAY},
    {XtNminimumDelay, XtCMinimumDelay, XtRInt, sizeof (int),
     TheOffset (minimumDelay), XtRImmediate,
     (XtPointer) SSW_DEF_MINIMUM_DELAY},
    {XtNrepeatDelay, XtCDelay, XtRInt, sizeof (int),
     TheOffset (repeatDelay), XtRImmediate,
     (XtPointer) SSW_DEF_REPEAT_DELAY},
#undef offset
};

STATIC_FUNCTION (void SimpleSlider__Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (Boolean SimpleSlider__SetValues,
		 (Widget Current, Widget Request, Widget New) );
STATIC_FUNCTION (void canvas_realise_cbk, (Widget w, XtPointer client_data,
					   XtPointer call_data) );
STATIC_FUNCTION (void refresh_func,
		 (KPixCanvas canvas, int width, int height,
		  void **info, PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );
STATIC_FUNCTION (flag position_func,
		 (KPixCanvas canvas, int x, int y,
		  unsigned int event_code, void *e_info, void **f_info) );
STATIC_FUNCTION (void draw_shaded_box,
		 (KPixCanvas canvas, int x0, int y0, int x1, int y1,
		  unsigned long bright_pixel, unsigned long dim_pixel) );
STATIC_FUNCTION (void setup_position_parameters,
		 (SimpleSliderWidget top, int width, int height,
		  pos_info *info) );
STATIC_FUNCTION (void set_value,
		 (SimpleSliderWidget top, int new_value, flag callback,
		  flag start_timer) );
STATIC_FUNCTION (void timer_cbk, (XtPointer client_data, XtIntervalId *id) );
STATIC_FUNCTION (void compute_value_string, (SimpleSliderWidget w,
					     char string[STRING_LENGTH]) );


#define DO_CALLBACK(sw) \
    XtCallCallbackList ((Widget) sw, sw->command.callbacks, NULL)


#define ADD_TIMEOUT(sw,delay) \
  XtAppAddTimeOut (XtWidgetToApplicationContext ((Widget) sw), \
		   (unsigned long) delay, timer_cbk, (XtPointer) sw)

#define CLEAR_TIMEOUT(sw) \
{ \
  (sw)->slider.allow_timer = FALSE; \
  if ((sw)->slider.timer) \
  { \
    XtRemoveTimeOut ((sw)->slider.timer); \
    (sw)->slider.timer = 0; \
  } \
}


SimpleSliderClassRec simpleSliderClassRec =
{
    {
	/* core fields  */
	(WidgetClass) &canvasClassRec,  /*  superclass             */
	"SimpleSlider",                 /*  class_name             */
	sizeof (SimpleSliderRec),       /*  widget_size            */
	NULL,                           /*  class_initialise       */
	NULL,                           /*  class_part_initialise  */
	FALSE,                          /*  class_inited           */
	(XtInitProc) SimpleSlider__Initialise,/*  initialise             */
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
	NULL,                           /*  resize                 */
	XtInheritExpose,                /*  expose                 */
	(XtSetValuesFunc) SimpleSlider__SetValues, /*  set_values  */
	NULL,                           /*  set_values_hook        */
	XtInheritSetValuesAlmost,       /*  set_values_almost      */
	NULL,                           /*  get_values_hook        */
	NULL,                           /*  accept_focus           */
	XtVersion,                      /*  version                */
	NULL,                           /*  callback_private       */
	XtInheritTranslations,          /*  tm_table               */
	XtInheritQueryGeometry,         /*  query_geometry         */
	XtInheritDisplayAccelerator,    /*  display_accelerator    */
	NULL                            /*  extension              */
    },
    {   /* simple fields */
	XtInheritChangeSensitive        /* change_sensitive        */
    },
    {   /* canvas fields */
	0                               /* ignore                  */
    },
    {
	/*  simple slider fields */
	0                               /*  ignore                 */
    }
};

WidgetClass simpleSliderWidgetClass = (WidgetClass) &simpleSliderClassRec;

static void SimpleSlider__Initialise (Widget Request, Widget New)
{
    Window root_window;
    /*SimpleSliderWidget request = (SimpleSliderWidget) Request;*/
    SimpleSliderWidget new = (SimpleSliderWidget) New;
    Display *dpy;
    Screen *screen;
    /*static char function_name[] = "SimpleSlider::Initialise";*/

    dpy = XtDisplay (New);
    screen = XtScreen (New);
    root_window = RootWindowOfScreen (screen);
    if (new->core.width < 1) new->core.width = 174;
    if (new->core.width < 50) new->core.width = 50;
    if (new->core.height < 1) new->core.height = 40;
    new->canvas.clip = True;
    new->slider.clicked_thumb = FALSE;
    new->slider.hyperspace = FALSE;
    new->slider.timer = 0;
    new->slider.next_delay = new->slider.initialDelay;
    new->slider.show_raw = TRUE;
    new->slider.show_scaled = FALSE;
    new->slider.scale = 1.0;
    new->slider.offset = 0.0;
    XtAddCallback (New, XkwNrealiseCallback, canvas_realise_cbk,
		   (XtPointer) New);
}   /*  End Function Initialise  */

static Boolean SimpleSlider__SetValues (Widget Current, Widget Request,
					Widget New)
{
    SimpleSliderWidget current = (SimpleSliderWidget) Current;
    SimpleSliderWidget new = (SimpleSliderWidget) New;
    static char function_name[] = "SimpleSlider::SetValues";

    if (new->core.width < 50) new->core.width = 50;
    if (new->slider.minimum > new->slider.maximum)
    {
	fprintf (stderr, "Minimum: %d is greater than maximum: %d\n",
		 new->slider.minimum, new->slider.maximum);
	a_prog_bug (function_name);
    }
    if ( (new->slider.valuePtr != current->slider.valuePtr) &&
	(new->slider.valuePtr != NULL) )
    {
	if ( !IS_ALIGNED (new->slider.valuePtr, sizeof *new->slider.valuePtr) )
	{
	    fprintf (stderr, "valuePtr: %p is not aligned\n",
		     new->slider.valuePtr);
	    a_prog_bug (function_name);
	}
    }
    if ( !XtIsSensitive (New) )	CLEAR_TIMEOUT (new);
    set_value (new, new->slider.value, FALSE, FALSE);
    return False;   
}   /*  End Function SetValues  */

static void canvas_realise_cbk (Widget w, XtPointer client_data,
				XtPointer call_data)
/*  This is the canvas realise callback.
*/
{
    SimpleSliderWidget top = (SimpleSliderWidget) client_data;
    /*static char function_name[] = "SimpleSliderWidget::canvas_realise_cbk";*/

    kwin_register_refresh_func (top->canvas.monoPixCanvas, refresh_func, top);
    kwin_register_position_event_func (top->canvas.monoPixCanvas,
				       position_func, top);
}   /*  End Function canvas_realise_cbk   */

static void refresh_func (KPixCanvas canvas, int width, int height,
			  void **info, PostScriptPage pspage,
			  unsigned int num_areas, KPixCanvasRefreshArea *areas,
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
    pos_info pos;
    int screen_number;
    unsigned long black_pixel, white_pixel;
    SimpleSliderWidget top = (SimpleSliderWidget) *info;
    Display *dpy;
    Screen *screen;
    char txt[STRING_LENGTH];

    if (width < 50) return;
    dpy = XtDisplay ( (Widget) top );
    screen = XtScreen ( (Widget) top );
    screen_number = XScreenNumberOfScreen (screen);
    kwin_get_colour (canvas, "black", &black_pixel, NULL, NULL, NULL);
    kwin_get_colour (canvas, "white", &white_pixel, NULL, NULL, NULL);
    /*  Setup position parameters  */
    setup_position_parameters (top, width, height, &pos);
    if ( (top->slider.label != NULL) && (top->slider.label[0] != '\0') )
    {
	/*  Draw label  */
	kwin_draw_string (canvas,
			  pos.label_string_x_start, pos.top_string_y_start,
			  top->slider.label, black_pixel, FALSE);
    }
    if (top->slider.showValue)
    {
	compute_value_string (top, txt);
	kwin_draw_string (canvas,
			  pos.value_string_x_start, pos.value_string_y_start,
			  txt, black_pixel, FALSE);
    }
    if (top->slider.showRange)
    {
	sprintf (txt, "%d", top->slider.minimum);
	kwin_draw_string (canvas,
			  pos.min_string_x_start, pos.bottom_string_y_start,
			  txt, black_pixel, FALSE);
	sprintf (txt, "%d", top->slider.maximum);
	kwin_draw_string (canvas,
			  pos.max_string_x_start, pos.bottom_string_y_start,
			  txt, black_pixel, FALSE);
    }
    if ( XtIsSensitive ( (Widget) top ) )
    {
	/*  Draw endboxes  */
	draw_shaded_box (canvas,
			 pos.left_endbox_x_start, pos.left_endbox_y_start,
			 pos.left_endbox_x_stop - 1, pos.left_endbox_y_stop -1,
			 white_pixel, black_pixel);
	draw_shaded_box (canvas,
			 pos.right_endbox_x_start, pos.right_endbox_y_start,
			 pos.right_endbox_x_stop - 1,pos.right_endbox_y_stop-1,
			 white_pixel, black_pixel);
	/*  Draw thumb  */
	draw_shaded_box (canvas, pos.thumb_x_start, pos.thumb_y_start,
			 pos.thumb_x_stop - 1, pos.thumb_y_stop - 1,
			 white_pixel, black_pixel);
/*
	kwin_fill_rectangle (canvas,
			     pos.thumb_x_start + 1, pos.thumb_y_start + 1,
			     THUMB_WIDTH - 3, THUMB_HEIGHT - 3,
			     top->canvas.foreground_pixel);
*/
    }
    /*  Draw bar  */
    /*  Draw left bar if visible  */
    if (pos.thumb_x_start > pos.bar_x_start)
    {
	kwin_fill_rectangle (canvas, pos.bar_x_start, pos.bar_y_start,
			     pos.thumb_x_start - pos.bar_x_start - 1,
			     pos.bar_y_stop - pos.bar_y_start - 1,
			     black_pixel);
	kwin_draw_line (canvas, pos.bar_x_start + 2, pos.bar_y_start + 1,
			pos.thumb_x_start - 1, pos.bar_y_start + 1,
			top->core.background_pixel);
    }
    /*  Draw right bar if visible  */
    if (pos.thumb_x_stop < pos.bar_x_stop - 1)
    {
	kwin_fill_rectangle (canvas, pos.thumb_x_stop, pos.bar_y_start,
			     pos.bar_x_stop - pos.thumb_x_stop - 1,
			     pos.bar_y_stop - pos.bar_y_start - 2,
			     top->canvas.foreground_pixel);
	kwin_draw_line (canvas, pos.thumb_x_stop, pos.bar_y_stop - 1,
			pos.bar_x_stop - 1, pos.bar_y_stop - 1,
			white_pixel);
    }
}   /*  End Function refresh_func  */

static flag position_func (KPixCanvas canvas, int x, int y,
			   unsigned int event_code, void *e_info,
			   void **f_info)
/*  [SUMMARY] Process a position event on a pixel canvas.
    <canvas> The pixel canvas on which the event occurred.
    <x> The horizontal position of the event, relative to the canvas origin
    <y> The vertical position of the event, relative to the canvas origin.
    <event_code> The arbitrary event code.
    <e_info> A pointer to arbitrary event information.
    <f_info> A pointer to an arbitrary function information pointer.
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    pos_info pos;
    int width, height;
    int slider_pos;
    int offset = 1;
    int x_warp = 0;
    SimpleSliderWidget top = (SimpleSliderWidget) *f_info;

    top->slider.last_x = x;
    top->slider.last_y = y;
    if ( top->slider.hyperspace &&
	 (event_code == K_CANVAS_EVENT_LEFT_MOUSE_DRAG) )
    {
	/*  This was the event generated by the warp  */
	top->slider.hyperspace = FALSE;
	return (TRUE);
    }
    /*  Discard event if a mouse drag and the timer is active, otherwise
	history of which bar click event started timer is lost.  */
    if ( (top->slider.timer != 0) &&
	 ( (event_code == K_CANVAS_EVENT_LEFT_MOUSE_DRAG) ||
	   (event_code == K_CANVAS_EVENT_RIGHT_MOUSE_DRAG) ) ) return (TRUE);
    /*  Only save event code if not due to a warp pointer move, otherwise
	history of which bar click event started timer is lost.  */
    top->slider.last_event_code = event_code;
    /*  Always clear the timer when a real event is received  */
    CLEAR_TIMEOUT (top);
    /*  Set next timer delay to initial delay if any button was released  */
    if ( (event_code == K_CANVAS_EVENT_LEFT_MOUSE_RELEASE) ||
	 (event_code == K_CANVAS_EVENT_MIDDLE_MOUSE_RELEASE) ||
	 (event_code == K_CANVAS_EVENT_RIGHT_MOUSE_RELEASE) )
    {
	top->slider.next_delay = top->slider.initialDelay;
    }
    kwin_get_size (canvas, &width, &height);
    /*  Setup position parameters  */
    setup_position_parameters (top, width, height, &pos);
    /*  Check if left endbox  */
    if ( (event_code == K_CANVAS_EVENT_LEFT_MOUSE_RELEASE) &&
	 (x >= pos.left_endbox_x_start) && (x < pos.left_endbox_x_stop) &&
	 (y >= pos.left_endbox_y_start) && (y < pos.left_endbox_y_stop) )
    {
	set_value (top, top->slider.minimum, TRUE, FALSE);
	return (TRUE);
    }
    /*  Check if right endbox  */
    if ( (event_code == K_CANVAS_EVENT_LEFT_MOUSE_RELEASE) &&
	 (x >= pos.right_endbox_x_start) && (x < pos.right_endbox_x_stop) &&
	 (y >= pos.right_endbox_y_start) && (y < pos.right_endbox_y_stop) )
    {
	set_value (top, top->slider.maximum, TRUE, FALSE);
	return (TRUE);
    }
    /*  Check if thumb clicked down  */
    if ( (event_code == K_CANVAS_EVENT_LEFT_MOUSE_CLICK) &&
	 (x >= pos.thumb_x_start) && (x < pos.thumb_x_stop) &&
	 (y >= pos.thumb_y_start) && (y < pos.thumb_y_stop) )
    {
	top->slider.clicked_thumb = TRUE;
	return (TRUE);
    }
    /*  Check if thumb dragged or released  */
    if ( top->slider.clicked_thumb &&
	 ( (event_code == K_CANVAS_EVENT_LEFT_MOUSE_DRAG) ||
	   (event_code == K_CANVAS_EVENT_LEFT_MOUSE_RELEASE) ) )
    {
	/*  Check if thumb released  */
	if (event_code == K_CANVAS_EVENT_LEFT_MOUSE_RELEASE)
	{
	    top->slider.clicked_thumb = FALSE;
	}
	/*  Thumb  */
	slider_pos = x - (pos.bar_x_start + THUMB_WIDTH / 2);
	slider_pos *= top->slider.maximum - top->slider.minimum;
	slider_pos /= (pos.bar_x_stop - pos.bar_x_start - THUMB_WIDTH - 1);
	slider_pos += top->slider.minimum;
	if (event_code == K_CANVAS_EVENT_LEFT_MOUSE_DRAG)
	{
	    set_value (top, slider_pos, top->slider.callbackOnDrag, FALSE);
	    return (TRUE);
	}
	/*  Was a release event  */
	if ( !top->slider.callbackOnDrag || (slider_pos != top->slider.value) )
	{
	    /*  Slider moved or callbacks not calling callbacks on drags  */
	    set_value (top, slider_pos, TRUE, FALSE);
	}
	return (TRUE);
    }
    /*  After this the event code may be modified  */
    if (event_code == K_CANVAS_EVENT_RIGHT_MOUSE_CLICK)
    {
	offset = 10;
	event_code = K_CANVAS_EVENT_LEFT_MOUSE_CLICK;
    }
    offset *= top->slider.modifier;
    /*  Check if click on left bar  */
    if ( (event_code == K_CANVAS_EVENT_LEFT_MOUSE_CLICK) &&
	 (x >= pos.bar_x_start) && (x < pos.thumb_x_start) &&
	 (y >= pos.thumb_y_start) && (y < pos.thumb_y_stop) )
    {
	slider_pos = top->slider.value - offset;
	set_value (top, slider_pos, TRUE, TRUE);
	/*  Check if thumb has moved to the left of the pointer  */
	setup_position_parameters (top, width, height, &pos);
	if (x < pos.thumb_x_start)
	{
	    /*  Thumb is still to the right of the pointer  */
	    return (TRUE);
	}
	x_warp = pos.thumb_x_start - x - 1;
    }
    /*  Check if click on right bar  */
    else if ( (event_code == K_CANVAS_EVENT_LEFT_MOUSE_CLICK) &&
	      (x >= pos.thumb_x_stop) && (x < pos.bar_x_stop) &&
	      (y >= pos.thumb_y_start) && (y < pos.thumb_y_stop) )
    {
	slider_pos = top->slider.value + offset;
	set_value (top, slider_pos, TRUE, TRUE);
	/*  Check if thumb has moved to the right of the pointer  */
	setup_position_parameters (top, width, height, &pos);
	if (x >= pos.thumb_x_stop)
	{
	    /*  Thumb is still to the left of the pointer  */
	    return (TRUE);
	}
	x_warp = pos.thumb_x_stop - x;
    }
    if (x_warp != 0)
    {
	XWarpPointer (XtDisplay ( (Widget) top ), None, None, 0, 0, 0, 0,
		      x_warp, 0);
	XSync (XtDisplay ( (Widget) top ), False );
	/*  Disable next event which is caused by the warp  */
	top->slider.hyperspace = TRUE;
	return (TRUE);
    }
    return (TRUE);
}   /*  End Function position_func  */

static void draw_shaded_box (KPixCanvas canvas, int x0, int y0, int x1, int y1,
			     unsigned long bright_pixel,
			     unsigned long dim_pixel)
/*  [SUMMARY] Draw a shaded box.
    <canvas> The pixel canvas on which to draw.
    <x0> The horizontal offset of the top-left vertex.
    <y0> The vertical offset of the top-left vertex.
    <x1> The horizontal offset of the bottom-right vertex.
    <y1> The vertical offset of the botton-right vertex.
    <bright_pixel> The bright pixel.
    <dim_pixel> The dim pixel.
    [RETURNS] Nothing.
*/
{
    kwin_draw_line (canvas, x0, y0, x1, y0, bright_pixel);
    kwin_draw_line (canvas, x0, y0, x0, y1, bright_pixel);
    kwin_draw_line (canvas, x0, y1, x1, y1, dim_pixel);
    kwin_draw_line (canvas, x1, y0, x1, y1, dim_pixel);
}   /*  End Function draw_shaded_box  */

static void setup_position_parameters (SimpleSliderWidget top,
				       int width, int height, pos_info *info)
/*  [SUMMARY] Setup position parameters.
    <top> The simple slider widget.
    <width> The width of the canvas in pixels.
    <height> The height of the canvas in pixels.
    <info> The position parameters will be written here.
    [RETURNS] Nothing.
*/
{
    KPixCanvasFont font;
    flag has_label = FALSE;
    flag value_at_top = FALSE;
    int slider_pos;
    int slider_height;
    int label_width, value_width, min_width, max_width, ascent, descent;
    int string_height;
    char txt[STRING_LENGTH];
    static char function_name[] = "SimpleSlider::setup_position_parameters";

    if ( (top->slider.label != NULL) && (top->slider.label[0] != '\0') )
    {
	has_label = TRUE;
    }
    value_at_top = top->slider.showValue && top->slider.valueBesideLabel;
    if (has_label || top->slider.showValue || top->slider.showRange)
    {
	/*  Must draw some strings: get font and ascent and descent  */
	kwin_get_attributes (top->canvas.monoPixCanvas,
			     KWIN_ATT_FONT, &font,
			     KWIN_ATT_END);
	if ( !kwin_get_string_size (font, "dummy",
				    KWIN_STRING_HEIGHT, &string_height,
				    KWIN_STRING_ASCENT, &ascent,
				    KWIN_STRING_DESCENT, &descent,
				    KWIN_STRING_END) )
	{
	    fprintf (stderr, "Error getting string size\n");
	    a_prog_bug (function_name);
	}
    }
    else string_height = 0;
    /*  Determine vertical positioning of label and value strings  */
    if (has_label || value_at_top)
    {
	/*  Must draw label and/or value at the top  */
	info->top_string_y_start = ascent + 2;
	info->top_string_y_stop = info->top_string_y_start + descent;
	info->value_string_y_start = info->top_string_y_start;
    }
    else
    {
	info->top_string_y_start = 0;
	info->top_string_y_stop = 0;
    }
    slider_height = height - info->top_string_y_stop;
    info->bottom_string_y_start = height - 1 - (slider_height -
						string_height) / 2;
    info->bottom_string_y_start -= descent;
    if (top->slider.showValue)
    {
	if (top->slider.valueBesideLabel)
	{
	    /*  Value is in top part  */
	    info->value_string_y_start = info->top_string_y_start;
	}
	else
	{
	    /*  Value is in bottom part  */
	    info->value_string_y_start = info->bottom_string_y_start;
	}
    }
    /*  Determine width of min and max strings  */
    if (top->slider.showValue || top->slider.showRange)
    {
	sprintf (txt, "%d", top->slider.minimum);
	if ( !kwin_get_string_size (font, txt,
				    KWIN_STRING_WIDTH, &min_width,
				    KWIN_STRING_END) )
	{
	    fprintf (stderr, "Error getting string width\n");
	    a_prog_bug (function_name);
	}
	sprintf (txt, "%d", top->slider.maximum);
	if ( !kwin_get_string_size (font, txt,
				    KWIN_STRING_WIDTH, &max_width,
				    KWIN_STRING_END) )
	{
	    fprintf (stderr, "Error getting string width\n");
	    a_prog_bug (function_name);
	}
    }
    /*  Determine width of value string  */
    if (top->slider.showValue)
    {
	compute_value_string (top, txt);
	if ( !kwin_get_string_size (font, txt,
				    KWIN_STRING_WIDTH, &value_width,
				    KWIN_STRING_END) )
	{
	    fprintf (stderr, "Error getting string width\n");
	    a_prog_bug (function_name);
	}
    }
    else value_width = 0;
    /*  Determine width of label string  */
    if (has_label)
    {
	if ( !kwin_get_string_size (font, top->slider.label,
				    KWIN_STRING_WIDTH, &label_width,
				    KWIN_STRING_END) )
	{
	    fprintf (stderr, "Error getting string size\n");
	    a_prog_bug (function_name);
	}
    }
    /*  Determine horizontal positioning of label and value strings at top  */
    if (has_label && value_at_top)
    {
	/*  Both label and value string at the top  */
	info->label_string_x_start = (width - label_width - value_width -
				      CLEARANCE * 3) / 2;
	info->label_string_x_stop = info->label_string_x_start + label_width;
	info->value_string_x_start = info->label_string_x_stop + CLEARANCE * 3;
    }
    else if (has_label && !value_at_top)
    {
	/*  Only have label string at the top  */
	info->label_string_x_start = (width - label_width) / 2;
	info->label_string_x_stop = info->label_string_x_start + label_width;
    }
    else if (!has_label && value_at_top)
    {
	/*  Only have value string at the top  */
	info->value_string_x_start = (width - value_width) / 2;
    }
    /*  Determine horizontal positioning of value string at bottom  */
    if (top->slider.showValue && !top->slider.valueBesideLabel)
    {
	/*  Value string is in the bottom part  */
	info->value_string_x_start = CLEARANCE;
	info->value_string_x_stop = info->value_string_x_start + value_width;
	/*  Add extra clearance  */
	info->value_string_x_stop += CLEARANCE;
    }
    else
    {
	info->value_string_x_stop = 0;
    }
    if (top->slider.showRange)
    {
	/*  Range strings  */
	info->min_string_x_start = info->value_string_x_stop + CLEARANCE;
	info->min_string_x_stop = info->min_string_x_start + min_width;
	info->max_string_x_start = width - max_width - CLEARANCE;
    }
    else
    {
	info->min_string_x_start = info->value_string_x_start;
	info->min_string_x_stop = info->value_string_x_stop;
	info->max_string_x_start = width;
    }
    /*  Left endbox  */
    info->left_endbox_x_start = info->min_string_x_stop + CLEARANCE;
    info->left_endbox_x_stop = info->left_endbox_x_start + ENDBOX_WIDTH;
    info->left_endbox_y_start = (info->top_string_y_stop +
				 (slider_height - ENDBOX_HEIGHT) / 2);
    info->left_endbox_y_stop = info->left_endbox_y_start + ENDBOX_HEIGHT;
    /*  Right endbox  */
    info->right_endbox_x_start = (info->max_string_x_start -
				  CLEARANCE - 1 - ENDBOX_WIDTH);
    info->right_endbox_x_stop = info->right_endbox_x_start + ENDBOX_WIDTH;
    info->right_endbox_y_start = info->left_endbox_y_start;
    info->right_endbox_y_stop = info->left_endbox_y_stop;
    /*  Bar  */
    info->bar_x_start = info->left_endbox_x_stop + CLEARANCE;
    info->bar_x_stop = info->right_endbox_x_start - CLEARANCE;
    info->bar_y_start = (info->top_string_y_stop +
			 (slider_height - BAR_HEIGHT) / 2);
    info->bar_y_stop = info->bar_y_start + BAR_HEIGHT;
    /*  Thumb  */
    slider_pos = top->slider.value - top->slider.minimum;
    slider_pos *= (info->bar_x_stop - info->bar_x_start - THUMB_WIDTH - 1);
    if (top->slider.maximum > top->slider.minimum)
    {
	slider_pos /= top->slider.maximum - top->slider.minimum;
    }
    info->thumb_x_start = info->bar_x_start + slider_pos;
    if (info->thumb_x_start + THUMB_WIDTH >= info->bar_x_stop)
    {
	fprintf (stderr, "Slider ran past right limit\n");
	info->thumb_x_start = info->bar_x_stop - THUMB_WIDTH;
    }
    info->thumb_x_stop = info->thumb_x_start + THUMB_WIDTH;
    info->thumb_y_start = (info->top_string_y_stop +
			   (slider_height - THUMB_HEIGHT) / 2);
    info->thumb_y_stop = info->thumb_y_start + THUMB_HEIGHT;
}   /*  End Function setup_position_parameters  */

static void set_value (SimpleSliderWidget top, int new_value, flag callback,
		       flag start_timer)
/*  [SUMMARY] Set the value for the widget.
    <top> The simple slider widget.
    <new_value> The value to set. This is clipped to the limits.
    <callback> If TRUE the callbacks are called.
    <start_timer> If TRUE, the timer is started.
    [RETURNS] Nothing.
*/
{
    if (new_value < top->slider.minimum) new_value = top->slider.minimum;
    if (new_value > top->slider.maximum) new_value = top->slider.maximum;
    top->slider.value = new_value;
    if (top->slider.valuePtr != NULL) *top->slider.valuePtr = new_value;
    if (top->canvas.monoPixCanvas != NULL)
    {
	kwin_resize (top->canvas.monoPixCanvas, TRUE, 0, 0, 0, 0);
    }
    /*  Set allow_timer flag *before* calling callbacks, since in the
	callbacks the widget may be set insensitive, and hence the flag
	will be cleared again. If the timer is added before calling the
	callbacks one can get multiple callbacks when the callbacks take
	longer than the initial timer delay.  */
    top->slider.allow_timer = TRUE;
    if ( callback && (top->slider.valueChangeCallback != NULL) )
    {
	XtCallCallbackList ( (Widget) top, top->slider.valueChangeCallback,
			     (XtPointer) &new_value );
    }
    if (start_timer && top->slider.allow_timer)
    {
	top->slider.timer = ADD_TIMEOUT (top, top->slider.next_delay);
	if (top->slider.next_delay == top->slider.initialDelay)
	{
	    top->slider.next_delay = top->slider.repeatDelay;
	}
    }
}   /*  End Function set_value  */

static void timer_cbk (XtPointer client_data, XtIntervalId *id)
{
    SimpleSliderWidget sw = (SimpleSliderWidget) client_data;
    void *f_info = sw;
    static char function_name[] = "SimpleSlider::timer_cbk";

    if (sw->slider.timer == 0)
    {
	fprintf (stderr, "%s: no timer! Ignoring\n", function_name);
	return;
    }
    if (!sw->slider.allow_timer)
    {
	fprintf (stderr, "%s: timer disabled! Ignoring\n",
			function_name);
	return;
    }
    sw->slider.timer = 0;		/* timer is removed */
    if ( !XtIsSensitive ( (Widget) sw ) ) return;
    /*  Generate synthetic click event (based on last click event) which will
	then cause the value to be updated, the callbacks to be called and the
	timer to be started.  */
    position_func (sw->canvas.monoPixCanvas,
			  sw->slider.last_x, sw->slider.last_y,
			  sw->slider.last_event_code, NULL, &f_info);
    /* decrement delay time, but clamp */
    if (sw->slider.decay) {
	sw->slider.next_delay -= sw->slider.decay;
	if (sw->slider.next_delay < sw->slider.minimumDelay)
	  sw->slider.next_delay = sw->slider.minimumDelay;
    }
}   /*  End Function timer_cbk  */

static void compute_value_string (SimpleSliderWidget w,
				  char string[STRING_LENGTH])
/*  [SUMMARY] Compute the value string.
    <w> The SimpleSliderWidget.
    <string> The value string will be written here.
    [RETURNS] Nothing.
*/
{
    double unit_scale = 1.0;
    double value;
    char txt[STRING_LENGTH], format_string[STRING_LENGTH];

    if (w->slider.show_raw && w->slider.show_scaled)
    {
	sprintf (format_string, "r: %%d  sc: %s", w->slider.scaledFormat);
	if ( (w->slider.scaledUnit != NULL) &&
	     (w->slider.scaledUnit[0] != '\0') )
	{
	    ds_format_unit (txt, &unit_scale, w->slider.scaledUnit);
	    strcat (format_string, " ");
	    strcat (format_string, txt);
	}
	value = (double) w->slider.value * w->slider.scale + w->slider.offset;
	sprintf (string, format_string,
		 w->slider.value, value * unit_scale);
    }
    else if (w->slider.show_raw) sprintf (string, "%d", w->slider.value);
    else if (w->slider.show_scaled)
    {
	strcpy (format_string, w->slider.scaledFormat);
	if ( (w->slider.scaledUnit != NULL) &&
	     (w->slider.scaledUnit[0] != '\0') )
	{
	    ds_format_unit (txt, &unit_scale, w->slider.scaledUnit);
	    strcat (format_string, " ");
	    strcat (format_string, txt);
	}
	value = (double) w->slider.value * w->slider.scale + w->slider.offset;
	sprintf (string, format_string, value * unit_scale);
    }
}   /*  End Function compute_value_string  */


/*  Public functions follow  */

void XkwSimpleSliderSetScale (Widget W, double scale, double offset,
			      flag show_raw, flag show_scaled)
/*  [SUMMARY] Set the data scaling.
    <W> The widget.
    <scale> The scale value to be applied to data.
    <offset> The offset value to be applied after the scale value is applied.
    <show_raw> If TRUE, raw (unscaled) values are shown.
    <show_scaled> If TRUE, scaled values are shown.
    [RETURNS] Nothing.
*/
{
    SimpleSliderWidget w = (SimpleSliderWidget) W;
    static char function_name[] = "XkwSimpleSliderSetScale";

    if ( !XtIsSimpleSlider (W) )
    {
	fprintf (stderr, "Not SimpleSliderWidgetClass!\n");
	a_prog_bug (function_name);
    }
    if ( (scale == 1.0) && (offset == 0.0) )
    {
	show_raw = TRUE;
	show_scaled = FALSE;
    }
    w->slider.scale = scale;
    w->slider.offset = offset;
    w->slider.show_raw = show_raw;
    w->slider.show_scaled = show_scaled;
    if (w->canvas.monoPixCanvas != NULL)
	kwin_resize (w->canvas.monoPixCanvas, TRUE, 0, 0, 0, 0);
}   /*  End Function XkwSimpleSliderSetScale  */

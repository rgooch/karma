/*LINTLIBRARY*/
/*  AnimateControl.c

    This code provides an animation control widget for Xt.

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

/*  This file contains all routines needed for an animation control widget for
  Xt.


    Written by      Richard Gooch   8-DEC-1994

    Updated by      Richard Gooch   9-DEC-1994

    Updated by      Richard Gooch   14-DEC-1994: Fixed resource names.

    Updated by      Richard Gooch   25-DEC-1994: Removed border around number
  of frames label.

    Updated by      Richard Gooch   4-JAN-1995: Cosmetic changes.

    Updated by      Richard Gooch   18-JUL-1995: Prevented updating current
  frame slider when frame interval < 100 ms.

    Updated by      Richard Gooch   27-JUL-1995: Added canvas for prev/next
  events.

    Updated by      Richard Gooch   30-JUL-1995: Made use of XkwNvaluePtr
  resource for ValueWidgetClass.

    Updated by      Richard Gooch   17-AUG-1995: Ensured endFrame is always
  greater than startFrame.

    Last updated by Richard Gooch   31-AUG-1995: As above, but cope with
  numFrames set to 0.

    Updated by      Richard Gooch   19-MAY-1996: Changed from using window
  scale structure to using <canvas_set_attributes>.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   4-JUN-1996: Switched to left-right
  bottom-top co-ordinate specification instead of min-max x and y.


*/

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <varargs.h>
#include <sys/wait.h>
#include <signal.h>
#include <X11/Xos.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#ifndef X11
#  define X11
#endif
#include <karma.h>
#define NEW_WIN_SCALE
#include <k_event_codes.h>
#include <karma_m.h>
#include <karma_a.h>
#include <Xkw/AnimateControlP.h>
#include <Xkw/Value.h>
#include <Xkw/ExclusiveMenu.h>
#include <Xkw/Canvas.h>
#include <Xkw/Repeater.h>


/*  Private functions  */

STATIC_FUNCTION (void AnimateControl__Initialise,
		 (Widget request, Widget new) );
STATIC_FUNCTION (void cnv_realise_cbk, (Widget w, XtPointer client_data,
					XtPointer call_data) );
STATIC_FUNCTION (Boolean AnimateControl__SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void popdown_cbk, (Widget w, XtPointer client_data,
				    XtPointer call_data) );
STATIC_FUNCTION (void start_movie_cbk, (Widget w, XtPointer client_data,
					XtPointer call_data) );
STATIC_FUNCTION (void stop_movie_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (void prev_frame_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (void next_frame_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (void store_cbk, (Widget w, XtPointer client_data,
				  XtPointer call_data) );
STATIC_FUNCTION (void timer_cbk, (XtPointer client_data, XtIntervalId *id) );
STATIC_FUNCTION (void goto_frame_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (flag position_func,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info,
		  void **f_info, double x_lin, double y_lin) );
STATIC_FUNCTION (void refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale, Kcolourmap cmap,
		  flag cmap_resize, void **info, PostScriptPage pspage) );
STATIC_FUNCTION (void set_limits_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );


#define MODE_FORWARD_SPIN 0
#define MODE_REVERSE_SPIN 1
#define MODE_ROCKnROLL 2
#define NUM_MODE_CHOICES 3

static char *mode_choices[NUM_MODE_CHOICES] =
{
    "Forward Spin", "Reverse Spin", "Rock & Roll"
};


#define offset(field) XtOffset(AnimateControlWidget, animateControl.field)

static XtResource AnimateControlResources[] = 
{
    {XkwNnumFrames, XkwCNumFrames, XtRInt, sizeof (int),
     offset (numFrames), XtRImmediate, (XtPointer) 0},
    {XkwNstartFrame, XkwCStartFrame, XtRInt, sizeof (int),
     offset (startFrame), XtRImmediate, (XtPointer) 0},
    {XkwNendFrame, XkwCEndFrame, XtRInt, sizeof (int),
     offset (endFrame), XtRImmediate, (XtPointer) 0},
    {XkwNnewFrameCallback, XtCCallback, XtRCallback, sizeof (caddr_t),
     offset (newFrameCallback), XtRCallback, (caddr_t) NULL},
};

#undef TheOffset

AnimateControlClassRec animateControlClassRec = 
{
  {
      /* CoreClassPart */
      (WidgetClass)&topLevelShellClassRec, /* superclass */
      "AnimateControl",                    /* class_name */
      sizeof(AnimateControlRec),           /* widget_size */
      NULL,                                /* class_initialise */
      NULL,                                /* class_part_initialise */
      FALSE,                               /* class_init */
      (XtInitProc) AnimateControl__Initialise,/* initialise */
      NULL,                                /* initialise_hook */
      XtInheritRealize,                    /* realise */
      NULL,                                /* actions */
      0,                                   /* num_actions */
      AnimateControlResources,             /* resources */
      XtNumber(AnimateControlResources),   /* num_resources */
      NULLQUARK,                           /* xrm_class */
      TRUE,                                /* compress_motion */
      TRUE,                                /* compress_exposure */
      TRUE,                                /* compress_enterleave */
      TRUE,                                /* visible_interest */
      NULL,                                /* destroy */
      XtInheritResize,                     /* resize */
      NULL,                                /* expose */
      (XtSetValuesFunc) AnimateControl__SetValues,  /* set_values */
      NULL,                                /* set_values_hook */
      XtInheritSetValuesAlmost,            /* set_values_almost */
      NULL,                                /* get_values_hook */
      NULL,                                /* accept_focus */
      XtVersion,                           /* version */
      NULL,                                /* callback_private */
      NULL,                                /* tm_translations */
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
  {  /* PostscriptClassPart */
    0 /* empty */
  }
};

WidgetClass animateControlWidgetClass = (WidgetClass) &animateControlClassRec;

static void AnimateControl__Initialise (Widget Request, Widget New)
{
    /*AnimateControlWidget request = (AnimateControlWidget) Request;*/
    AnimateControlWidget new = (AnimateControlWidget) New;
    Widget form, w;
    Widget close_btn, start_btn, stop_btn, prev_btn, next_btn, goto_btn;
    Widget interval_sld, inc_sld;
    Widget cnv, mode_menu;
    /*static char function_name[] = "AnimateControlWidget::Initialise";*/

    new->animateControl.running_movie = FALSE;
    new->animateControl.interval_ms = 100;
    new->animateControl.inc_factor = 1;
    new->animateControl.spin_mode = MODE_FORWARD_SPIN;
    new->animateControl.direction = 1;
    new->animateControl.position_wc = NULL;
    form = XtVaCreateManagedWidget ("form", formWidgetClass, New,
				    XtNborderWidth, 0,
				    NULL);
    close_btn = XtVaCreateManagedWidget ("closeButton", commandWidgetClass,
					 form,
					 XtNlabel, "Close",
					 NULL);
    XtAddCallback (close_btn, XtNcallback, popdown_cbk, New);
    prev_btn = XtVaCreateManagedWidget ("prevButton", repeaterWidgetClass,form,
					 XtNlabel, "Previous Frame",
					 XtNfromHoriz, close_btn,
					 NULL);
    XtAddCallback (prev_btn, XtNcallback, prev_frame_cbk, (XtPointer) New);
    next_btn = XtVaCreateManagedWidget ("nextButton", repeaterWidgetClass,form,
					 XtNlabel, "Next Frame",
					 XtNfromHoriz, prev_btn,
					 NULL);
    XtAddCallback (next_btn, XtNcallback, next_frame_cbk, (XtPointer) New);
    start_btn = XtVaCreateManagedWidget ("startButton", commandWidgetClass,
					 form,
					 XtNlabel, "Start Movie",
					 XtNfromVert, close_btn,
					 NULL);
    XtAddCallback (start_btn, XtNcallback, start_movie_cbk, (XtPointer) New);
    stop_btn = XtVaCreateManagedWidget ("startButton", commandWidgetClass,
					form,
					XtNlabel, "Stop Movie",
					XtNfromVert, close_btn,
					XtNfromHoriz, start_btn,
					NULL);
    XtAddCallback (stop_btn, XtNcallback, stop_movie_cbk, (XtPointer) New);
    mode_menu = XtVaCreateManagedWidget ("menuButton",
					 exclusiveMenuWidgetClass, form,
					 XtNmenuName, "modeMenu",
					 XkwNchoiceName, "MODE",
					 XtNfromVert, stop_btn,
					 XkwNnumItems, NUM_MODE_CHOICES,
					 XkwNitemStrings, mode_choices,
					 NULL);
    XtAddCallback (mode_menu, XkwNselectCallback, store_cbk,
		   (XtPointer) &new->animateControl.spin_mode);
    interval_sld = XtVaCreateManagedWidget ("interval", valueWidgetClass, form,
					    XtNlabel, "Frame Interval (ms)",
					    XtNfromVert, mode_menu,
					    XkwNminimum, 0,
					    XkwNmaximum, 10000,
					    XkwNmodifier, 10,
					    XtNvalue,
					    new->animateControl.interval_ms,
					    XkwNvaluePtr,
					    &new->animateControl.interval_ms,
					    NULL);
    inc_sld = XtVaCreateManagedWidget ("interval", valueWidgetClass, form,
				       XtNlabel, "Skip Factor",
				       XtNfromVert, interval_sld,
				       XkwNminimum, 1,
				       XkwNmaximum, 100,
				       XkwNmodifier, 1,
				       XtNvalue,
				       new->animateControl.inc_factor,
				       XkwNvaluePtr,
				       &new->animateControl.inc_factor,
				       NULL);
    w = XtVaCreateManagedWidget ("label", labelWidgetClass, form,
				 XtNlabel, "Number of frames:     ",
				 XtNborderWidth, 0,
				 XtNfromVert, inc_sld,
				 NULL);
    new->animateControl.num_frames_lbl = w;
    goto_btn = XtVaCreateManagedWidget ("gotoButton", commandWidgetClass,form,
					XtNlabel, "Goto Frame",
					XtNfromVert, inc_sld,
					XtNfromHoriz, w,
					NULL);
    XtAddCallback (goto_btn, XtNcallback, goto_frame_cbk, (XtPointer) New);
    cnv = XtVaCreateManagedWidget ("positionCanvas", canvasWidgetClass, form,
				   XtNfromVert, w,
				   XtNwidth, 309,
				   XtNheight, 40,
				   XkwNsilenceUnconsumed, True,
				   XtNresizable, True,
				   NULL);
    XtAddCallback (cnv, XkwNrealiseCallback, cnv_realise_cbk, (XtPointer) New);
    w = XtVaCreateManagedWidget ("slider", valueWidgetClass, form,
				 XtNlabel, "Current frame",
				 XtNfromVert, cnv,
				 XkwNminimum, 0,
				 XkwNmaximum, 0,
				 XkwNmodifier, 1,
				 XtNvalue, 0,
				 XkwNwrap, True,
				 NULL);
    new->animateControl.current_frame_sld = w;
    w = XtVaCreateManagedWidget ("startpos", valueWidgetClass, form,
				 XtNlabel, "Starting Frame",
				 XtNfromVert, w,
				 XkwNminimum, 0,
				 XkwNmaximum, 0,
				 XkwNmodifier, 1,
				 XtNvalue, 0,
				 XkwNvaluePtr, &new->animateControl.startFrame,
				 NULL);
    new->animateControl.start_frame_sld = w;
    XtAddCallback (w, XkwNvalueChangeCallback, set_limits_cbk,(XtPointer) new);
    w = XtVaCreateManagedWidget ("endpos", valueWidgetClass, form,
				 XtNlabel, "End Frame",
				 XtNfromVert, w,
				 XkwNminimum, 0,
				 XkwNmaximum, 0,
				 XkwNmodifier, 1,
				 XtNvalue, 0,
				 XkwNvaluePtr, &new->animateControl.endFrame,
				 NULL);
    new->animateControl.end_frame_sld = w;
    XtAddCallback (w, XkwNvalueChangeCallback, set_limits_cbk,(XtPointer) new);
}   /*  End Function Initialise  */

static void cnv_realise_cbk (w, client_data, call_data)
/*  This is the canvas realise callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    KPixCanvas pixcanvas = (KPixCanvas) call_data;
    KWorldCanvas wc;
    double left_x, right_x;
    struct win_scale_type win_scale;
    AnimateControlWidget top = (AnimateControlWidget) client_data;
    static char function_name[] = "AnimateControlWidget::cnv_realise_cbk";

    canvas_init_win_scale (&win_scale, K_WIN_SCALE_MAGIC_NUMBER);
    left_x = top->animateControl.startFrame;
    right_x = top->animateControl.endFrame;
    if (right_x <= left_x) right_x = left_x + 1.0;
    win_scale.left_x = left_x;
    win_scale.right_x = right_x;
    if ( ( wc = canvas_create (pixcanvas, NULL, &win_scale) ) == NULL )
    {
	m_abort (function_name, "world canvas");
    }
    top->animateControl.position_wc = wc;
    (void) canvas_register_position_event_func (wc, position_func,
						(void *) top);
    (void) canvas_register_refresh_func (wc, refresh_func, (void *) top);
}   /*  End Function cnv_realise_cbk   */

static Boolean AnimateControl__SetValues (Widget Current, Widget Request,
					  Widget New)
{
    int num_frames;
    double left_x, right_x;
    char txt[STRING_LENGTH];
    AnimateControlWidget current = (AnimateControlWidget) Current;
    /*AnimateControlWidget request = (AnimateControlWidget) Request;*/
    AnimateControlWidget new = (AnimateControlWidget) New;
    /*static char function_name[] = "AnimateControlWidget::SetValues";*/

    if (new->animateControl.numFrames != current->animateControl.numFrames)
    {
	num_frames = new->animateControl.numFrames;
	new->animateControl.startFrame = 0;
	if (num_frames < 1) new->animateControl.endFrame = 0;
	else new->animateControl.endFrame = num_frames - 1;
	new->animateControl.currentFrame = 0;
	(void) sprintf (txt, "Number of frames: %d", num_frames);
	XtVaSetValues (new->animateControl.num_frames_lbl,
		       XtNlabel, txt,
		       NULL);
	XtVaSetValues (new->animateControl.current_frame_sld,
		       XkwNminimum, 0,
		       XkwNmaximum, new->animateControl.endFrame,
		       XtNvalue, 0,
		       NULL);
	XtVaSetValues (new->animateControl.start_frame_sld,
		       XkwNmaximum, num_frames - 1,
		       XtNvalue, 0,
		       NULL);
	XtVaSetValues (new->animateControl.end_frame_sld,
		       XkwNmaximum, num_frames - 1,
		       XtNvalue, num_frames - 1,
		       NULL);
	if (new->animateControl.position_wc != NULL)
	{
	    left_x = new->animateControl.startFrame;
	    right_x = new->animateControl.endFrame;
	    if (right_x <= left_x) right_x = left_x + 1.0;
	    canvas_set_attributes (new->animateControl.position_wc,
				   CANVAS_ATT_LEFT_X, left_x,
				   CANVAS_ATT_RIGHT_X, right_x,
				   CANVAS_ATT_END);
	    (void) kwin_resize (canvas_get_pixcanvas
				(new->animateControl.position_wc),
				TRUE, 0, 0, 0, 0);
	}
    }
    return False;
}   /*  End Function SetValues  */

static void popdown_cbk (w, client_data, call_data)
/*  This is the generic popdown button callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    AnimateControlWidget top = (AnimateControlWidget) client_data;

    if (top->animateControl.running_movie)
    {
	XBell (XtDisplay (top), 100);
	return;
    }
    XtPopdown ( (Widget) top );
}   /*  End Function popdown_cbk   */

static void start_movie_cbk (w, client_data, call_data)
/*  This is the start movie callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    AnimateControlWidget top = (AnimateControlWidget) client_data;
    XtAppContext app_context;

    if (top->animateControl.running_movie)
    {
	XBell (XtDisplay (w), 100);
	return;
    }
    if (top->animateControl.numFrames < 1)
    {
	XBell (XtDisplay (w), 100);
	return;
    }
    top->animateControl.running_movie = TRUE;
    app_context = XtWidgetToApplicationContext (w);
    XtAppAddTimeOut (app_context,
		     (unsigned long) top->animateControl.interval_ms + 1,
		     timer_cbk, (XtPointer) top);
}   /*  End Function start_movie_cbk   */

static void stop_movie_cbk (w, client_data, call_data)
/*  This is the stop movie callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    AnimateControlWidget top = (AnimateControlWidget) client_data;

    if (!top->animateControl.running_movie)
    {
	XBell (XtDisplay (w), 100);
	return;
    }
    top->animateControl.running_movie = FALSE;
    XtVaSetValues (top->animateControl.current_frame_sld,
		   XtNvalue, top->animateControl.currentFrame,
		   NULL);
    (void) canvas_resize (top->animateControl.position_wc,
			  (struct win_scale_type *) NULL, TRUE);
}   /*  End Function stop_movie_cbk   */

static void prev_frame_cbk (w, client_data, call_data)
/*  This is the previous frame callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    int frame_number;
    AnimateControlWidget top = (AnimateControlWidget) client_data;

    if (top->animateControl.numFrames < 1)
    {
	top->animateControl.currentFrame = 0;
	return;
    }
    if (--top->animateControl.currentFrame < top->animateControl.startFrame)
    {
	top->animateControl.currentFrame = top->animateControl.endFrame;
    }
    XtVaSetValues (top->animateControl.current_frame_sld,
		   XtNvalue, top->animateControl.currentFrame,
		   NULL);
    (void) canvas_resize (top->animateControl.position_wc,
			  (struct win_scale_type *) NULL, TRUE);
    frame_number = top->animateControl.currentFrame;
    XtCallCallbacks ( (Widget) top, XkwNnewFrameCallback,
		     (XtPointer) &frame_number );
}   /*  End Function prev_frame_cbk   */

static void next_frame_cbk (w, client_data, call_data)
/*  This is the next frame callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    int frame_number;
    AnimateControlWidget top = (AnimateControlWidget) client_data;

    if (top->animateControl.numFrames < 1)
    {
	return;
    }
    if (++top->animateControl.currentFrame > top->animateControl.endFrame)
    {
	top->animateControl.currentFrame = top->animateControl.startFrame;
    }
    XtVaSetValues (top->animateControl.current_frame_sld,
		   XtNvalue, top->animateControl.currentFrame,
		   NULL);
    (void) canvas_resize (top->animateControl.position_wc,
			  (struct win_scale_type *) NULL, TRUE);
    frame_number = top->animateControl.currentFrame;
    XtCallCallbacks ( (Widget) top, XkwNnewFrameCallback,
		     (XtPointer) &frame_number );
}   /*  End Function next_frame_cbk   */

static void store_cbk (w, client_data, call_data)
/*  This is the store callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    int *new_value = (int *) call_data;
    int *global_value = (int *) client_data;

    *global_value = *new_value;
}   /*  End Function store_cbk   */

static void timer_cbk (XtPointer client_data, XtIntervalId *id)
/*  This is the interval timer callback.
*/
{
    int frame_number, jump;
    AnimateControlWidget top = (AnimateControlWidget) client_data;
    XtAppContext app_context;

    if (!top->animateControl.running_movie) return;
    switch (top->animateControl.spin_mode)
    {
      case MODE_FORWARD_SPIN:
	top->animateControl.direction = 1;
	break;
      case MODE_REVERSE_SPIN:
	top->animateControl.direction = -1;
	break;
    }
    jump = top->animateControl.inc_factor * top->animateControl.direction;
    frame_number = top->animateControl.currentFrame + jump;
    if (frame_number < top->animateControl.startFrame)
    {
	switch (top->animateControl.spin_mode)
	{
	  case MODE_FORWARD_SPIN:
	  case MODE_REVERSE_SPIN:
	    frame_number = top->animateControl.endFrame;
	    break;
	  case MODE_ROCKnROLL:
	    frame_number = top->animateControl.startFrame;
	    top->animateControl.direction = -top->animateControl.direction;
	    break;
	}
    }
    else if (frame_number > top->animateControl.endFrame)
    {
	switch (top->animateControl.spin_mode)
	{
	  case MODE_FORWARD_SPIN:
	  case MODE_REVERSE_SPIN:
	    frame_number = top->animateControl.startFrame;
	    break;
	  case MODE_ROCKnROLL:
	    frame_number = top->animateControl.endFrame;
	    top->animateControl.direction = -top->animateControl.direction;
	    break;
	}
    }
    top->animateControl.currentFrame = frame_number;
    /*  Prevent updating the current frame at an excessive rate  */
    if (top->animateControl.interval_ms >= 100)
    {
	XtVaSetValues (top->animateControl.current_frame_sld,
		       XtNvalue, top->animateControl.currentFrame,
		       NULL);
	/*  Restore temp. value in case slider decides to change it  */
	frame_number = top->animateControl.currentFrame;
	(void) canvas_resize (top->animateControl.position_wc,
			      (struct win_scale_type *) NULL, TRUE);
    }
    XtCallCallbacks ( (Widget) top, XkwNnewFrameCallback,
		     (XtPointer) &frame_number );
    app_context = XtWidgetToApplicationContext ( (Widget) top );
    XtAppAddTimeOut (app_context,
		     (unsigned long) top->animateControl.interval_ms + 1,
		     timer_cbk, (XtPointer) top);
}   /*  End Function timer_cbk  */

static void goto_frame_cbk (w, client_data, call_data)
/*  This is the goto frame callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    flag change = FALSE;
    int frame_number;
    AnimateControlWidget top = (AnimateControlWidget) client_data;

    if (top->animateControl.running_movie)
    {
	XBell (XtDisplay (top), 100);
	return;
    }
    XtVaGetValues (top->animateControl.current_frame_sld,
		   XtNvalue, &frame_number,
		   NULL);
    if (frame_number > top->animateControl.endFrame)
    {
	frame_number = top->animateControl.endFrame;
	change = TRUE;
    }
    else if (frame_number < top->animateControl.startFrame)
    {
	frame_number = top->animateControl.startFrame;
	change = TRUE;
    }
    if (change) XtVaSetValues (top->animateControl.current_frame_sld,
			       XtNvalue, frame_number,
			       NULL);
    top->animateControl.currentFrame = frame_number;
    (void) canvas_resize (top->animateControl.position_wc,
			  (struct win_scale_type *) NULL, TRUE);
    XtCallCallbacks ( (Widget) top, XkwNnewFrameCallback,
		     (XtPointer) &frame_number );
}   /*  End Function goto_frame_cbk   */

static flag position_func (KWorldCanvas canvas, double x, double y,
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
    int frame_number;
    AnimateControlWidget top = (AnimateControlWidget) *f_info;

    switch (event_code)
    {
      case K_CANVAS_EVENT_LEFT_MOUSE_CLICK:
	prev_frame_cbk (NULL, (XtPointer) *f_info, (XtPointer) NULL);
	break;
      case K_CANVAS_EVENT_RIGHT_MOUSE_CLICK:
	next_frame_cbk (NULL, (XtPointer) *f_info, (XtPointer) NULL);
	break;
      case K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK:
      case K_CANVAS_EVENT_MIDDLE_MOUSE_DRAG:
	frame_number = x + 0.5;
	if ( (frame_number >= top->animateControl.startFrame) &&
	    (frame_number <= top->animateControl.endFrame) &&
	    (top->animateControl.currentFrame != frame_number) )
	{
	    top->animateControl.currentFrame = frame_number;
	    canvas_resize (canvas, (struct win_scale_type *) NULL, TRUE);
	    XtCallCallbacks ( (Widget) top, XkwNnewFrameCallback,
			     (XtPointer) &frame_number );
	}
	break;
      case K_CANVAS_EVENT_MIDDLE_MOUSE_RELEASE:
	XtVaSetValues (top->animateControl.current_frame_sld,
		       XtNvalue, top->animateControl.currentFrame,
		       NULL);
	break;
      default:
	break;
    }
    return (FALSE);
}   /*  End Function position_func  */

static void refresh_func (KWorldCanvas canvas, int width, int height,
			  struct win_scale_type *win_scale, Kcolourmap cmap,
			  flag cmap_resize, void **info, PostScriptPage pspage)
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
    AnimateControlWidget top = (AnimateControlWidget) *info;
    /*static char function_name[] = "AnimateControlWidget::refresh_func";*/

    if ( !canvas_get_colour (canvas, "white", &pixel_value,
			     (unsigned short *) NULL,
			     (unsigned short *) NULL,
			     (unsigned short *) NULL) ) return;
    canvas_draw_line_p (canvas, top->animateControl.currentFrame, 0.0,
			top->animateControl.currentFrame, 1.0,
			pixel_value);
}   /*  End Function refresh_func  */

static void set_limits_cbk (w, client_data, call_data)
/*  This is the limits callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    AnimateControlWidget top = (AnimateControlWidget) client_data;

    if (top->animateControl.endFrame <= top->animateControl.startFrame)
    {
	top->animateControl.endFrame = top->animateControl.startFrame + 1;
	if (top->animateControl.endFrame >= top->animateControl.numFrames)
	{
	    --top->animateControl.endFrame;
	    --top->animateControl.startFrame;
	    XtVaSetValues (top->animateControl.start_frame_sld,
			   XtNvalue, top->animateControl.startFrame,
			   NULL);
	}
	XtVaSetValues (top->animateControl.end_frame_sld,
		       XtNvalue, top->animateControl.endFrame,
		       NULL);
    }
    canvas_set_attributes (top->animateControl.position_wc,
			   CANVAS_ATT_LEFT_X,
			   (double) top->animateControl.startFrame,
			   CANVAS_ATT_RIGHT_X,
			   (double) top->animateControl.endFrame,
			   CANVAS_ATT_END);
    (void) canvas_resize (top->animateControl.position_wc, NULL, TRUE);
}   /*  End Function set_limits_cbk   */

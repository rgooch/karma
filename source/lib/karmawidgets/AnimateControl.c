/*LINTLIBRARY*/
/*  AnimateControl.c

    This code provides an animation control widget for Xt.

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

/*  This file contains all routines needed for an animation control widget for
  Xt.


    Written by      Richard Gooch   8-DEC-1994

    Updated by      Richard Gooch   9-DEC-1994

    Updated by      Richard Gooch   14-DEC-1994: Fixed resource names.

    Updated by      Richard Gooch   25-DEC-1994: Removed border around number
  of frames label.

    Last updated by Richard Gooch   4-JAN-1995: Cosmetic changes.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#define X11
#include <Xkw/AnimateControlP.h>

#include <X11/Xos.h>
#include <X11/StringDefs.h>

#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#include <X11/Xaw/Repeater.h>

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <varargs.h>

#include <Xkw/Value.h>
#include <Xkw/ExclusiveMenu.h>

#include <sys/wait.h>
#include <signal.h>
#include <karma_m.h>
#include <karma_a.h>


/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

/* Methods */

STATIC_FUNCTION (void Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (void Destroy, (Widget w) );
STATIC_FUNCTION (Boolean SetValues,
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


#define MODE_FORWARD_SPIN 0
#define MODE_REVERSE_SPIN 1
#define MODE_ROCKnROLL 2
#define NUM_MODE_CHOICES 3

static char *mode_choices[NUM_MODE_CHOICES] =
{
    "Forward Spin", "Reverse Spin", "Rock & Roll"
};


/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

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

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

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
      (XtInitProc)Initialise,              /* initialise */
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
      Destroy,                             /* destroy */
      XtInheritResize,                     /* resize */
      NULL,                                /* expose */
      (XtSetValuesFunc)SetValues,          /* set_values */
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

/*----------------------------------------------------------------------*/
/* check that a widget is actually an AnimateControlWidget              */
/*----------------------------------------------------------------------*/

static int check_type (Widget w, char *function_name)
{
  static char func_name[] = "AnimateControl.check_type";
  int fl;
  fl=XtIsSubclass(w,animateControlWidgetClass);
  if(!fl)
    fprintf(stderr,
    "ERROR: Widget passed to %s is not a AnimateControlWidget\n",
	    function_name);
  return fl;
}

/*----------------------------------------------------------------------*/
/* Initialisation method                                                */
/*----------------------------------------------------------------------*/

static void Initialise (Widget Request, Widget New)
{
    AnimateControlWidget request = (AnimateControlWidget) Request;
    AnimateControlWidget new = (AnimateControlWidget) New;
    Widget form, w;
    Widget close_btn, start_btn, stop_btn, prev_btn, next_btn, goto_btn;
    Widget interval_sld, inc_sld;
    Widget mode_menu;
    static char function_name[] = "AnimateControl::Initialise";

    new->animateControl.running_movie = FALSE;
    new->animateControl.interval_ms = 100;
    new->animateControl.inc_factor = 1;
    new->animateControl.spin_mode = MODE_FORWARD_SPIN;
    new->animateControl.direction = 1;
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
					    NULL);
    XtAddCallback (interval_sld, XkwNvalueChangeCallback, store_cbk,
		   (XtPointer) &new->animateControl.interval_ms);
    inc_sld = XtVaCreateManagedWidget ("interval", valueWidgetClass, form,
				       XtNlabel, "Skip Factor",
				       XtNfromVert, interval_sld,
				       XkwNminimum, 1,
				       XkwNmaximum, 100,
				       XkwNmodifier, 1,
				       XtNvalue,
				       new->animateControl.inc_factor,
				       NULL);
    XtAddCallback (inc_sld, XkwNvalueChangeCallback, store_cbk,
		   (XtPointer) &new->animateControl.inc_factor);
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
    w = XtVaCreateManagedWidget ("slider", valueWidgetClass, form,
				 XtNlabel, "Current frame",
				 XtNfromVert, w,
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
				 NULL);
    new->animateControl.start_frame_sld = w;
    XtAddCallback (w, XkwNvalueChangeCallback, store_cbk,
		   (XtPointer) &new->animateControl.startFrame);
    w = XtVaCreateManagedWidget ("endpos", valueWidgetClass, form,
				 XtNlabel, "End Frame",
				 XtNfromVert, w,
				 XkwNminimum, 0,
				 XkwNmaximum, 0,
				 XkwNmodifier, 1,
				 XtNvalue, 0,
				 NULL);
    new->animateControl.end_frame_sld = w;
    XtAddCallback (w, XkwNvalueChangeCallback, store_cbk,
		   (XtPointer) &new->animateControl.endFrame);
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
    int num_frames;
    char txt[STRING_LENGTH];
    AnimateControlWidget current = (AnimateControlWidget) Current;
    AnimateControlWidget request = (AnimateControlWidget) Request;
    AnimateControlWidget new = (AnimateControlWidget) New;
    static char function_name[] = "AnimateControl::SetValues";

    if (new->animateControl.numFrames !=
	current->animateControl.numFrames)
    {
	num_frames = new->animateControl.numFrames;
	new->animateControl.startFrame = 0;
	new->animateControl.endFrame = num_frames - 1;
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

/*----------------------------------------------------------------------*/
/* Callback for Auto/Manual button                                      */
/*----------------------------------------------------------------------*/

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
    if (--top->animateControl.currentFrame <
	top->animateControl.startFrame)
    {
	top->animateControl.currentFrame = top->animateControl.endFrame;
    }
    XtVaSetValues (top->animateControl.current_frame_sld,
		   XtNvalue, top->animateControl.currentFrame,
		   NULL);
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
    if (++top->animateControl.currentFrame >
	top->animateControl.endFrame)
    {
	top->animateControl.currentFrame = top->animateControl.startFrame;
    }
    XtVaSetValues (top->animateControl.current_frame_sld,
		   XtNvalue, top->animateControl.currentFrame,
		   NULL);
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
    XtVaSetValues (top->animateControl.current_frame_sld,
		   XtNvalue, top->animateControl.currentFrame,
		   NULL);
    frame_number = top->animateControl.currentFrame;
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
    top->animateControl.currentFrame = frame_number;
    XtCallCallbacks ( (Widget) top, XkwNnewFrameCallback,
		     (XtPointer) &frame_number );

}   /*  End Function goto_frame_cbk   */

/*LINTLIBRARY*/
/*  Twodpos.c

    This code provides a 2-dimensional position widget for Xt.

    Copyright (C) 1993-1996  Patrick Jordan
    Incorporated into Karma by permission.

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

    Patrick Jordan may be reached by email at  pjordan@rp.csiro.au
    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains all routines needed for manipulating a 2-dimensional
    position widget for Xt.


    Written by      Patrick Jordan  3-JUL-1993

    Updated by      Patrick Jordan  28-JAN-1994: Added centering action

    Last updated by Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/

/*----------------------------------------------------------------------*/
/* Include files*/
/*----------------------------------------------------------------------*/

#include <X11/Xos.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <math.h>
#include <varargs.h>
#include <stdio.h>

#include <Xkw/TwodposP.h>

/*----------------------------------------------------------------------*/
/* Function prototypes*/
/*----------------------------------------------------------------------*/

/* Actions*/

static void Set(Widget w, XEvent *event, String *argv, int *argc);
static void Drag(Widget w, XMotionEvent *xme);
static void Centre(Widget w);

/* Methods*/

static void Initialize(Widget request,Widget new);
static void Resize(Widget w);
static void Destroy(Widget w);
static void Redisplay(Widget w, XEvent *event, Region region);
static Boolean SetValues(Widget current,Widget request,Widget new);

/* Other*/

static void DrawSpot(Widget w, GC gc);

/*----------------------------------------------------------------------*/
/* Translations etc*/
/*----------------------------------------------------------------------*/

static char TwodposTranslations[] =
  "<Btn1Down>:    set()\n\
   <Btn3Down>:    centre()\n\
   <Btn1Motion>:  set() drag()";

static XtActionsRec TwodposActions[] = 
{
  {"set", (XtActionProc)Set},
  {"centre",(XtActionProc)Centre},
  {"drag", (XtActionProc)Drag}
};

/*----------------------------------------------------------------------*/
/* Some macros*/
/*----------------------------------------------------------------------*/

/* Default width and height */

#define TWODPOS_WIDTH 50
#define TWODPOS_HEIGHT 50

/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define TheOffset(field) XtOffset(TwodposWidget, twodpos.field)

static float zero=0.0;
static float one=1.0;
static float half=0.5;

#define XkwNminimum_x "minimum_x"
#define XkwNmaximum_x "maximum_x"
#define XkwNminimum_y "minimum_y"
#define XkwNmaximum_y "maximum_y"
#define XkwNvalue_x "value_x"
#define XkwNvalue_y "value_y"

#define XkwCMinimum_x "Minimum_x"
#define XkwCMaximum_x "Maximum_x"
#define XkwCMinimum_y "Minimum_y"
#define XkwCMaximum_y "Maximum_y"
#define XkwCValue_x "Value_x"
#define XkwCValue_y "Value_y"

static XtResource TwodposResources[] = 
{
  {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    TheOffset(foreground), XtRString, (caddr_t)XtDefaultForeground},
  {XkwNminimum_x, XkwCMinimum_x, XtRFloat, sizeof(float),
    TheOffset(minimum_x), XtRFloat, (XtPointer)&zero},
  {XkwNmaximum_x, XkwCMaximum_x, XtRFloat, sizeof(float),
    TheOffset(maximum_x), XtRFloat, (XtPointer)&one},
  {XkwNvalue_x, XkwCValue_x, XtRFloat, sizeof(float),
    TheOffset(value_x), XtRFloat, (XtPointer)&half},
  {XkwNminimum_y, XkwCMinimum_y, XtRFloat, sizeof(float),
    TheOffset(minimum_y), XtRFloat, (XtPointer)&zero},
  {XkwNmaximum_y, XkwCMaximum_y, XtRFloat, sizeof(float),
    TheOffset(maximum_y), XtRFloat, (XtPointer)&one},
  {XkwNvalue_y, XkwCValue_y, XtRFloat, sizeof(float),
    TheOffset(value_y), XtRFloat, (XtPointer)&half},
  {XkwNvalueChangeCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
    TheOffset(valueChangeCallback), XtRCallback, (caddr_t)NULL},
};

#undef TheOffset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

TwodposClassRec twodposClassRec = 
{
  {        /* CoreClassPart */
    (WidgetClass)&widgetClassRec,  /* superclass */
    "Twodpos",                     /* class_name */
    sizeof(TwodposRec),            /* widget_size */
    NULL,                          /* class_initialize */
    NULL,                          /* class_part_initialize */
    FALSE,                         /* class_init */
    (XtInitProc)Initialize,        /* initialize */
    NULL,                          /* initialize_hook */
    XtInheritRealize,              /* realize */
    TwodposActions,                /* actions */
    XtNumber(TwodposActions),      /* num_actions */
    TwodposResources,              /* resources */
    XtNumber(TwodposResources),    /* num_resources */
    NULLQUARK,                     /* xrm_class */
    TRUE,                          /* compress_motion */
    TRUE,                          /* compress_exposure */
    TRUE,                          /* compress_enterleave */
    TRUE,                          /* visible_interest */
    Destroy,                       /* destroy */
    Resize,                        /* resize */
    Redisplay,                     /* expose */
    (XtSetValuesFunc)SetValues,    /* set_values */
    NULL,                          /* set_values_hook */
    XtInheritSetValuesAlmost,      /* set_values_almost */
    NULL,                          /* get_values_hook */
    NULL,                          /* accept_focus */
    XtVersion,                     /* version */
    NULL,                          /* callback_private */
    TwodposTranslations,           /* tm_translations */
    NULL,
    NULL,
    NULL,
  },
  { 
    0 /* empty */
  }
};

WidgetClass twodposWidgetClass = (WidgetClass) &twodposClassRec;

/*----------------------------------------------------------------------*/
/* Check some values*/
/*----------------------------------------------------------------------*/

static void check_values(float *min,float *max,float *value)
{
  if (*min >= *max)  *max = *min + 1.0;
  if (*value < *min) *value = *min;
  if (*value > *max) *value = *max;
}

/*----------------------------------------------------------------------*/
/* Initialisation method*/
/*----------------------------------------------------------------------*/

static void Initialize(Widget Request,Widget New)
{
  TwodposWidget request = (TwodposWidget) Request;
  TwodposWidget new = (TwodposWidget) New;
  XGCValues gcValues;
  Display *display;

  display = XtDisplay(new);

  check_values(&new->twodpos.minimum_y,&new->twodpos.maximum_y,
	       &new->twodpos.value_y);
  check_values(&new->twodpos.minimum_x,&new->twodpos.maximum_x,
	       &new->twodpos.value_x);

  if (request->core.width == 0)
    new->core.width = TWODPOS_WIDTH;
  if (request->core.height == 0)
    new->core.height = TWODPOS_HEIGHT;

  gcValues.foreground = new->twodpos.foreground;
  gcValues.background = new->core.background_pixel;
  gcValues.line_width = 0;
  new->twodpos.gc =
    XtGetGC(New, GCForeground | GCBackground | GCLineWidth, &gcValues);
  gcValues.foreground = new->core.background_pixel;
  gcValues.background = new->core.background_pixel;
  gcValues.line_width = 0;
  new->twodpos.eraseGC =
    XtGetGC(New, GCForeground | GCBackground | GCLineWidth, &gcValues);

  Resize(New);
}

/*----------------------------------------------------------------------*/
/* Resize method*/
/*----------------------------------------------------------------------*/

static void Resize(Widget W)
{
  TwodposWidget w = (TwodposWidget) W;
  w->twodpos.scale_x=
    (w->twodpos.maximum_x-w->twodpos.minimum_x)/(float)w->core.width;
  w->twodpos.scale_y=
    (w->twodpos.maximum_y-w->twodpos.minimum_y)/(float)w->core.height;
}

/*----------------------------------------------------------------------*/
/* Destroy method*/
/*----------------------------------------------------------------------*/

static void Destroy(Widget W)
{
  TwodposWidget w = (TwodposWidget) W;
  XtReleaseGC(W, w->twodpos.gc);
  XtReleaseGC(W, w->twodpos.eraseGC);
  XtRemoveAllCallbacks(W, XkwNvalueChangeCallback);
}

/*----------------------------------------------------------------------*/
/* DrawSpot function*/
/*----------------------------------------------------------------------*/

static void DrawSpot(Widget W,GC gc)
{
  TwodposWidget w = (TwodposWidget) W;
  if (w->core.visible) {
    XFillArc(XtDisplay(w), XtWindow(w), gc,
	     (w->twodpos.value_x+w->twodpos.minimum_x)/w->twodpos.scale_x - 5,
	     (w->twodpos.value_y+w->twodpos.minimum_y)/w->twodpos.scale_y - 5,
	     10, 10, 0, 360*64);
    XDrawString(XtDisplay(w),XtWindow(w),w->twodpos.gc,
		2,15,"Centre: Btn3",12);
  }
}

/*----------------------------------------------------------------------*/
/* Redisplay method*/
/*----------------------------------------------------------------------*/

static void Redisplay(Widget W,XEvent *event,Region region)
{
  TwodposWidget w = (TwodposWidget) W;
  if (w->core.visible)  {
    XClearWindow(XtDisplay(w), XtWindow(w));
    DrawSpot(W, w->twodpos.gc);
  }
  XFlush(XtDisplay(w));
}

/*----------------------------------------------------------------------*/
/* SetVaues method*/
/*----------------------------------------------------------------------*/

static Boolean SetValues(Widget Current,Widget Request,Widget New)
{
    TwodposWidget current = (TwodposWidget) Current;
    /*TwodposWidget request = (TwodposWidget) Request;*/
    TwodposWidget new = (TwodposWidget) New;
    Boolean redisplay = FALSE;
    Boolean newErase = FALSE;
    XGCValues eraseGCValues, gcVal;
    int eraseMask = 0;

  check_values
    (&new->twodpos.minimum_y,&new->twodpos.maximum_y,&new->twodpos.value_y);
  check_values
    (&new->twodpos.minimum_x,&new->twodpos.maximum_x,&new->twodpos.value_x);

  if (new->twodpos.minimum_x != current->twodpos.minimum_x ||
      new->twodpos.maximum_x != current->twodpos.maximum_x ||
      new->twodpos.minimum_y != current->twodpos.minimum_y ||
      new->twodpos.maximum_y != current->twodpos.maximum_y)  {
    new->twodpos.scale_x=
      (new->twodpos.maximum_x-new->twodpos.minimum_x)/new->core.width;
    new->twodpos.scale_y=
      (new->twodpos.maximum_y-new->twodpos.minimum_y)/new->core.height;
  }
  
  if (new->twodpos.foreground != current->twodpos.foreground)  {
    XtReleaseGC(New, new->twodpos.gc);
    gcVal.foreground = new->twodpos.foreground;
    new->twodpos.gc = XtGetGC(New, GCForeground, &gcVal);
    redisplay = TRUE;
  }

  if (new->core.background_pixel != current->core.background_pixel)  {
    eraseGCValues.foreground = new->core.background_pixel;
    newErase = TRUE;
    eraseMask |= GCForeground;
    redisplay = TRUE;
  }

  if (newErase)  {
    XtReleaseGC(New, new->twodpos.eraseGC);
    new->twodpos.eraseGC = XtGetGC(New, eraseMask, &eraseGCValues);
  }

  if (new->twodpos.value_x != current->twodpos.value_x ||
      new->twodpos.value_y != current->twodpos.value_y )  {
    if (!redisplay)  {
      DrawSpot(Current, new->twodpos.eraseGC);
      DrawSpot(New, new->twodpos.gc);

      return FALSE;
    }
  }
  return redisplay;
}

/*----------------------------------------------------------------------*/
/* Set action*/
/*----------------------------------------------------------------------*/

static void Set(Widget W,XEvent *event,String *argv,int *argc)
{
  TwodposWidget w = (TwodposWidget) W;
  float x, y;
  TwodposCallbackStruct hdcs;

  switch (event->type)
  {
    case ButtonPress:
    case ButtonRelease:
      x = (float)((XButtonEvent *)event)->x;
      y = (float)((XButtonEvent *)event)->y;
      break;
    case KeyPress:
    case KeyRelease:
      x = (float)((XKeyEvent *)event)->x;
      y = (float)((XKeyEvent *)event)->y;
      break;
    default: return; break;
  }
  DrawSpot(W,w->twodpos.eraseGC);

  w->twodpos.value_x=(float)x*w->twodpos.scale_x+w->twodpos.minimum_x;
  w->twodpos.value_y=(float)y*w->twodpos.scale_y+w->twodpos.minimum_y;

  check_values
    (&w->twodpos.minimum_y,&w->twodpos.maximum_y,&w->twodpos.value_y);
  check_values
    (&w->twodpos.minimum_x,&w->twodpos.maximum_x,&w->twodpos.value_x);
  
  DrawSpot(W,w->twodpos.gc);

  hdcs.event = event;
  hdcs.value_x = w->twodpos.value_x;
  hdcs.value_y = w->twodpos.value_y;

  XtCallCallbacks(W, XkwNvalueChangeCallback, &hdcs);
}

/*----------------------------------------------------------------------*/
/* Drag action*/
/*----------------------------------------------------------------------*/

static void Drag (Widget W, XMotionEvent *xme)
{
    XButtonEvent xe;

    xe.type = ButtonPress;
    xe.x = xme->x;
    xe.y = xme->y;

    Set(W, (XEvent *) &xe, NULL, NULL);
}

/*----------------------------------------------------------------------*/
/* Centre action*/
/*----------------------------------------------------------------------*/

static void Centre(Widget W)
{
  TwodposWidget w = (TwodposWidget) W;
  XButtonEvent xe;

  xe.type = ButtonPress;
  xe.x = w->core.width/2.0;
  xe.y = w->core.height/2.0;

  Set(W, (XEvent *)&xe, NULL, NULL);
}


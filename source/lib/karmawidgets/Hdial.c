/*LINTLIBRARY*/
/*  Hdial.c

    This code provides a dial widget for Xt.

    Copyright (C) 1993  John L. Cwikla  Incorporated into Karma by permission.

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

    John L. Cwikla may be reached by email at  cwikla@wri.com
    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains all routines needed for manipulating a dial widget for
    Xt.


    Copied from     John L. Cwikla  21-JUL-1993

    Updated by      Richard Gooch   29-SEP-1993

    Last updated by Richard Gooch   16-DEC-1993: Added patch submitted by
  Patrick Jordan (extra  XFlush  call).


*/

/* Default Translations:
 * <key>+: increment(1)
 * Shift<key>+: increment(100)
 * <key>-: decrement(1)
 * Shift<key>-: decrement(100)
 * <Btn1Down>: set()
 * <Btn1Motion>: set() drag()
*/

/* Resources:               Type:      Defaults:
 * XtNforeground          : pixel    : XtNDefaultForeground
 * XtNlabelForeground     : pixel    : XtNDefaultForeground
 * XtNminimum             : int      : 0
 * XtNmaximum             : int      : 65535
 * XtNvalue               : int      : 0
 * XtNfont                : XFontStruct   : XtNDefaultFont
 * XtNmargin              : int      : 5
 * XtNimcrementCallback   : callback : NULL
 * XtNdecrementCallbacki  : callback : NULL
 * XtNvalueChangeCallback : callback : NULL
*/

#include <X11/Xos.h>
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>

#include <math.h>
#include <varargs.h>

#include <Xkw/HdialP.h>
#include <Xkw/Hdial.h>

#define HDIAL_MINWIDTH 50
#define HDIAL_MINHEIGHT 50
#ifndef PI
#  define PI 3.14159
#endif
#define RADIANS(a) (PI/180.0 * (double)(a))
#define DEGREES(a) (180.0 / PI * (a))
#define THETA 5.0 

/* Actions */
#if NeedFunctionProtoTypes
tdatic void Increment(HdialWidget _w, XEvent *_event, String *_argv, int *_argc);
static void Decrement(HdialWidget _w, XEvent *_event, String *_argv, int *_argc);
static void Set(HdialWidget _w, XEvent *_event, String *argv, int *_argc);
static void Drag(HdialWidget _w, XMotionEvent *_xme);
#else
static void Increment();
static void Decrement();
static void Set();
static void Drag();
#endif

/* Widget Internals */ 
#if NeedFunctionProtoTypes
static void Initialize(HdialWidget _request, HdialWidget _new);
static void Resize(HdialWidget _w);
static void Destroy(HdialWidget _w);
static void Redisplay(HdialWidget _w, XEvent *_event, Region _region);
static Boolean SetValues(HdialWidget _current, HdialWidget _request, HdialWidget _new);
#else
static void Initialize();
static void Resize();
static void Destroy();
static void Redisplay();
static Boolean SetValues();
#endif

/* Misc */
#if NeedFunctionProtoTypes
static void MoveArm(HdialWidget _w);
static void ReValue(HdialWidget _w);
static void DrawArrow(HdialWidget _w, GC _gc);
static void DrawArm(HdialWidget _w, GC _gc);
static void DrawLabel(HdialWidget _w, GC _gc);
static void MyXtWarning();
#else
static void MoveArm();
static void ReValue();
static void DrawArrow();
static void DrawArm();
static void DrawLabel();
static void MyXtWarning();
#endif

static char HdialTranslations[] =
"Shift<Key>+:   increment(100)\n\
 <Key>+:	increment(1)\n\
 Shift<Key>-:   decrement(100)\n\
 <Key>-:	decrement(1)\n\
 <Btn1Down>:    set()\n\
 <Btn1Motion>:  set() drag()";

static XtActionsRec HdialActions[] = 
{
  {"increment", Increment},
  {"decrement", Decrement},
  {"set", Set},
  {"drag", Drag}
};

#ifndef MAX
#define MAX(a,b) ((a) > (b) ? (a) : (b))
#endif
#ifndef MIN
#define MIN(a,b) ((a) < (b) ? (a) : (b))
#endif

#define TheOffset(field) XtOffset(HdialWidget, hdial.field)

static XtResource HdialResources[] = 
{
  {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    TheOffset(foreground), XtRString, (caddr_t)XtDefaultForeground},
  {XtNlabelForeground, XtCLabelForeground, XtRPixel, sizeof(Pixel),
    TheOffset(labelForeground), XtRString, (caddr_t)XtDefaultForeground},
  {XtNfont, XtCFont, XtRFontStruct, sizeof(XFontStruct),
    TheOffset(font), XtRString, (caddr_t)XtDefaultFont},
  {XtNminimum, XtCMinimum, XtRInt, sizeof(int),
    TheOffset(minimum), XtRImmediate, (caddr_t)0},
  {XtNmaximum, XtCMaximum, XtRInt, sizeof(int),
    TheOffset(maximum), XtRImmediate, (caddr_t)65535},
  {XtNvalue, XtCValue, XtRInt, sizeof(int),
    TheOffset(value), XtRImmediate, (caddr_t)0},
  {XtNincrementCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
    TheOffset(incrementCallback), XtRCallback, (caddr_t)NULL},
  {XtNdecrementCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
    TheOffset(decrementCallback), XtRCallback, (caddr_t)NULL},
  {XtNvalueChangeCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
    TheOffset(valueChangeCallback), XtRCallback, (caddr_t)NULL},
  {XtNmargin, XtCMargin, XtRInt, sizeof(int),
    TheOffset(margin), XtRImmediate, (caddr_t)5},
};

#undef TheOffset

HdialClassRec hdialClassRec = 
{
  {        /* CoreClassPart */
    (WidgetClass)&widgetClassRec,  /* superclass */
    "Hdial",                       /* class_name */
    sizeof(HdialRec),               /* widget_size */
    NULL,                          /* class_initialize */
    NULL,                          /* class_part_initialize */
    FALSE,                         /* class_init */
    Initialize,                    /* initialize */
    NULL,                          /* initialize_hook */
    XtInheritRealize,              /* realize */
    HdialActions,                   /* actions */
    XtNumber(HdialActions),         /* num_actions */
    HdialResources,                 /* resources */
    XtNumber(HdialResources),       /* num_resources */
    NULLQUARK,                     /* xrm_class */
    TRUE,                          /* compress_motion */
    TRUE,                          /* compress_exposure */
    TRUE,                          /* compress_enterleave */
    TRUE,                          /* visible_intress */
    Destroy,                       /* destroy */
    Resize,                        /* resize */
    Redisplay,                     /* expose */
    SetValues,                     /* set_values */
    NULL,                          /* set_values_hook */
    XtInheritSetValuesAlmost,      /* set_values_almost */
    NULL,                          /* get_values_hook */
    NULL,                          /* accept_focus */
    XtVersion,                     /* version */
    NULL,                          /* callback_private */
    HdialTranslations,              /* tm_translations */
    NULL,
    NULL,
    NULL,
  },
  { 
    0 /* empty */
  }
};

WidgetClass hdialWidgetClass = (WidgetClass) &hdialClassRec;

#define HDIAL _new->hdial
#define CORE _new->core

static void Initialize(_request, _new)
HdialWidget _request;
HdialWidget _new;
{
  XGCValues gcValues;
  Display *display;

  display = XtDisplay(_new);

  if (HDIAL.font == (XFontStruct *)NULL)
  {
    if ((HDIAL.font = XLoadQueryFont(display, "fixed")) == NULL) 
      if ((HDIAL.font = XLoadQueryFont(display, "9x15")) == NULL)
        MyXtWarning("HdialWidget: Fonts %s and %s not found.", "fixed", "9x15");
  }

  if (HDIAL.minimum >= HDIAL.maximum)
  {
    MyXtWarning("HdialWidget: Maximum %d is less than minimum (%d).  Maximum set to %d.", 
      HDIAL.maximum, HDIAL.maximum, HDIAL.minimum+1);
    HDIAL.maximum = HDIAL.minimum+1;
  }

  if (HDIAL.value < HDIAL.minimum)
  {
    MyXtWarning("HdialWidget: Value %d is less than minimum (%d).  Value set to %d.",
      HDIAL.value, HDIAL.minimum, HDIAL.minimum);
    HDIAL.value = HDIAL.minimum;
  }

  if (HDIAL.value > HDIAL.maximum)
  {
    MyXtWarning("HdialWidget: Value %d is more than maximum (%d).  Value set to %d.",
      HDIAL.value, HDIAL.maximum, HDIAL.maximum);
    HDIAL.value = HDIAL.maximum;
  }

  sprintf(HDIAL.label, "%d", HDIAL.value);
  HDIAL.labelHeight = HDIAL.font->ascent + HDIAL.font->descent;
  HDIAL.labelWidth = XTextWidth(HDIAL.font, HDIAL.label, strlen(HDIAL.label));
 
  if (_request->core.width == 0)
    CORE.width = HDIAL_MINWIDTH + 2 * HDIAL.margin;
  if (_request->core.height == 0)
    CORE.height = HDIAL_MINHEIGHT + 3 * HDIAL.margin + HDIAL.labelHeight;

  gcValues.foreground = HDIAL.foreground;
  gcValues.background = CORE.background_pixel;
  gcValues.line_width = 0;
  HDIAL.gc = XtGetGC((Widget)_new, GCForeground | GCBackground | GCLineWidth, &gcValues);
  gcValues.foreground = HDIAL.labelForeground;
  gcValues.font = HDIAL.font->fid;
  HDIAL.labelGC = XtGetGC((Widget)_new, GCFont | GCForeground | GCBackground, &gcValues);
  gcValues.foreground = CORE.background_pixel;
  gcValues.background = CORE.background_pixel;
  gcValues.line_width = 0;
  HDIAL.eraseGC = XtGetGC((Widget)_new, GCFont | GCForeground | GCBackground | GCLineWidth, &gcValues);

  Resize(_new);
}
#undef HDIAL
#undef CORE

#define HDIAL _w->hdial
#define CORE _w->core

static void MoveArm(_w)
HdialWidget _w;
{
  double angle;
  double rangle;
 
  angle = (HDIAL.value - HDIAL.minimum)/(double)(HDIAL.maximum - HDIAL.minimum) * 180.0; 
  rangle = RADIANS(angle);

  HDIAL.top.x = HDIAL.center.x - (int)(cos(rangle)*HDIAL.length*7.0/8.0);
  HDIAL.top.y = HDIAL.center.y - (int)(sin(rangle)*HDIAL.length*7.0/8.0);
}

static void DrawArrow(_w, _gc)
HdialWidget _w; 
GC _gc;
{
  double angle;
  double rangle;
  XPoint tri[3];
  angle = (HDIAL.value - HDIAL.minimum)/(double)(HDIAL.maximum - HDIAL.minimum) * 180.0; 
  angle+=THETA;
  rangle = RADIANS(angle);
  tri[1].x = HDIAL.center.x - (int)(cos(rangle)*HDIAL.length*6.0/8.0);
  tri[1].y = HDIAL.center.y - (int)(sin(rangle)*HDIAL.length*6.0/8.0);
  angle -= 2.0*THETA;
  rangle = RADIANS(angle);
  tri[2].x =  HDIAL.center.x - (int)(cos(rangle)*HDIAL.length*6.0/8.0);
  tri[2].y  = HDIAL.center.y - (int)(sin(rangle)*HDIAL.length*6.0/8.0);
  tri[0].x = HDIAL.top.x;
  tri[0].y = HDIAL.top.y;
  XFillPolygon(XtDisplay(_w), XtWindow(_w), _gc, tri, 3, Convex, CoordModeOrigin);
}

static void Resize(_w)
HdialWidget _w;
{
  double width, height;
  int offset;

  width = CORE.width - 2.0 * HDIAL.margin;
  height = CORE.height - 3.0 * HDIAL.margin - HDIAL.labelHeight;

  HDIAL.length = MIN(width/2.0, height);

  HDIAL.center.x = CORE.width/2;
  offset = (CORE.height - (HDIAL.length + HDIAL.margin + HDIAL.labelHeight))/2;
  HDIAL.center.y = offset + HDIAL.length; 
  HDIAL.labelPos.x = HDIAL.center.x - (HDIAL.labelWidth / 2);
  HDIAL.labelPos.y = HDIAL.center.y + HDIAL.margin + HDIAL.labelHeight;

  MoveArm(_w);
}

#undef HDIAL
#undef CORE

static void Destroy(_w)
HdialWidget _w;
{
  XtReleaseGC((Widget)_w, _w->hdial.gc);
  XtReleaseGC((Widget)_w, _w->hdial.labelGC);
  XtReleaseGC((Widget)_w, _w->hdial.eraseGC);
  XFreeFont(XtDisplay(_w),_w->hdial.font);
  XtRemoveAllCallbacks((Widget)_w, XtNincrementCallback);
  XtRemoveAllCallbacks((Widget)_w, XtNdecrementCallback);
  XtRemoveAllCallbacks((Widget)_w, XtNvalueChangeCallback);
}

static void DrawArm(_w, _gc)
HdialWidget _w; 
GC _gc;
{    
  DrawArrow(_w, _gc);

  XFillArc(XtDisplay(_w), XtWindow(_w), _w->hdial.gc,
    _w->hdial.center.x - 5,
    _w->hdial.center.y - 5,
    10, 10, 0, 180*64);

  XDrawLine(XtDisplay(_w), XtWindow(_w), _gc,
    _w->hdial.center.x,
    _w->hdial.center.y,
    _w->hdial.top.x,
    _w->hdial.top.y);

  XDrawLine(XtDisplay(_w), XtWindow(_w), _w->hdial.gc,
    _w->hdial.center.x - (int)_w->hdial.length,
    _w->hdial.center.y,
    _w->hdial.center.x + (int)_w->hdial.length,
    _w->hdial.center.y);
}

static void DrawLabel(_w, _gc)
HdialWidget _w; 
GC _gc;
{
  XDrawString(XtDisplay(_w), XtWindow(_w), _gc,
    _w->hdial.labelPos.x, _w->hdial.labelPos.y,
    _w->hdial.label, strlen(_w->hdial.label));
}

#define NUMSEGMENTS 12 

static void Redisplay(_w, _event, _region)
HdialWidget _w; 
XEvent *_event; 
Region _region;
{
  int i;
  XPoint top, bot;
  int length;
  double angle, rangle;
  int ns;

  if (_w->core.visible)
  {
    XClearWindow(XtDisplay(_w), XtWindow(_w));

    DrawArm(_w, _w->hdial.gc);

    length = (int)(_w->hdial.length) * 2;
    XDrawArc(XtDisplay(_w), XtWindow(_w), _w->hdial.gc, 
      _w->hdial.center.x - (int)_w->hdial.length,
      _w->hdial.center.y - (int)_w->hdial.length,
      length, length, 0, 180*64);

    ns = MIN(NUMSEGMENTS, _w->hdial.maximum - _w->hdial.minimum);

    for(i=0;i<ns;i++)
    {
      angle = 180.0/ns*i;
      rangle = RADIANS(angle);
      top.x = _w->hdial.center.x - (int)(cos(rangle)*_w->hdial.length);
      top.y = _w->hdial.center.y - (int)(sin(rangle)*_w->hdial.length);
      bot.x = _w->hdial.center.x - (int)(cos(rangle)*_w->hdial.length*7.2/8.0);
      bot.y = _w->hdial.center.y - (int)(sin(rangle)*_w->hdial.length*7.2/8.0);
      XDrawLine(XtDisplay(_w), XtWindow(_w), _w->hdial.gc,
        top.x, top.y, bot.x, bot.y);
    }

    DrawLabel(_w, _w->hdial.labelGC);
  }
  XFlush(XtDisplay(_w));
}

#define HDIAL _new->hdial

static Boolean SetValues(_current, _request, _new)
HdialWidget _current; 
HdialWidget _request; 
HdialWidget _new;
{
  Boolean redisplay = FALSE;
  Display *display = XtDisplay(_new);
  Boolean newErase = FALSE, newLabel = FALSE;
  XGCValues eraseGCValues, labelGCValues, gcVal;
  int eraseMask = GCFont, labelMask = GCFont;
 
  if (HDIAL.font == (XFontStruct *)NULL)
  {
    if ((HDIAL.font = XLoadQueryFont(display, "fixed")) == NULL)
    if ((HDIAL.font = XLoadQueryFont(display, "9x15")) == NULL)
      MyXtWarning("HdialWidget: Fonts %s and %s not found.", "fixed", "9x15");
    redisplay = TRUE;
  }

  if (HDIAL.minimum >= HDIAL.maximum)
  {
    MyXtWarning("HdialWidget: Maximum %d is less than minimum (%d).  Maximum set to %d.",
      HDIAL.maximum, HDIAL.maximum, HDIAL.minimum+1);
    HDIAL.maximum = HDIAL.minimum+1;
  }

  if (HDIAL.value < HDIAL.minimum)
  {
    MyXtWarning("HdialWidget: Value %d is less than minimum (%d).  Value set to %d.",
      HDIAL.value, HDIAL.minimum, HDIAL.minimum);
    HDIAL.value = HDIAL.minimum;
  }

  if (HDIAL.value > HDIAL.maximum)
  {
    MyXtWarning("HdialWidget: Value %d is more than maximum (%d).  Value set to %d.",
      HDIAL.value, HDIAL.maximum, HDIAL.maximum);
    HDIAL.value = HDIAL.maximum;
  }

  if (HDIAL.labelForeground != _current->hdial.labelForeground)
  {
    newLabel = TRUE;
    labelMask |= GCForeground;
    labelGCValues.foreground = HDIAL.labelForeground; 
    redisplay = TRUE;
  }

  if (HDIAL.foreground != _current->hdial.foreground)
  {
    XtReleaseGC((Widget)_new, HDIAL.gc);
    gcVal.foreground = HDIAL.foreground;
    HDIAL.gc = XtGetGC((Widget)_new, GCForeground, &gcVal);
    redisplay = TRUE;
  }

  if (_new->core.background_pixel != _current->core.background_pixel)
  {
    eraseGCValues.foreground = _new->core.background_pixel;
    newErase = TRUE;
    eraseMask |= GCForeground;
    redisplay = TRUE;
  }

  if (HDIAL.font->fid != _current->hdial.font->fid)
  {
    XFreeFont(display, _current->hdial.font);
    newErase = newLabel = TRUE;
    redisplay = TRUE;
  }

  eraseGCValues.font = HDIAL.font->fid;
  labelGCValues.font = HDIAL.font->fid;

  if (newErase)
  {
    XtReleaseGC((Widget)_new, _new->hdial.eraseGC);
    _new->hdial.eraseGC = XtGetGC((Widget)_new, eraseMask, &eraseGCValues);
  }

  if (newLabel)
  {
    XtReleaseGC((Widget)_new, _new->hdial.labelGC);
    _new->hdial.labelGC = XtGetGC((Widget)_new, labelMask, &labelGCValues);
  }

  if (HDIAL.value != _current->hdial.value)
  {
    if (!redisplay)
    {
      DrawArm(_current, HDIAL.eraseGC);
      DrawLabel(_current, HDIAL.eraseGC);
      ReValue(_new);
      DrawArm(_new, HDIAL.gc);
      DrawLabel(_new, HDIAL.labelGC);
      return FALSE;
    }
    ReValue(_new);
  }

  return redisplay;
}

#undef HDIAL

static void Increment(_w, _event, _argv, _argc)
HdialWidget _w; 
XEvent *_event; 
String *_argv; 
int *_argc;
{
  int i;
  int newVal;
  Arg warg;
  HdialCallbackStruct hdcs;

  if (*_argc == 0)
  {
    if ((_w->hdial.value + 1) > _w->hdial.maximum)
      return;
    XtSetArg(warg, XtNvalue, _w->hdial.value + 1);
    XtSetValues((Widget)_w, &warg, 1);
    return;
  }
  i = atoi(_argv[0]);
  if (i > 0)
  {
    newVal = ((_w->hdial.value + i) > _w->hdial.maximum) ? _w->hdial.maximum : _w->hdial.value + i;
    XtSetArg(warg, XtNvalue, newVal);
    XtSetValues((Widget)_w, &warg, 1);
  }
  hdcs.reason = HDIAL_INCREMENT;
  hdcs.event = _event;
  hdcs.value = _w->hdial.value;
  XtCallCallbacks((Widget)_w, XtNincrementCallback, &hdcs);
}

static void Decrement(_w, _event, _argv, _argc)
HdialWidget _w; 
XEvent *_event;
String *_argv;
int *_argc;
{
  int i;
  int newVal;
  Arg warg;
  HdialCallbackStruct hdcs;

  if (*_argc == 0)
  {
    if ((_w->hdial.value - 1) < _w->hdial.minimum)
      return;
    XtSetArg(warg, XtNvalue, _w->hdial.value - 1);
    XtSetValues((Widget)_w, &warg, 1);
    return;
  }
  i = atoi(_argv[0]);
  if (i > 0)
  {
    newVal = ((_w->hdial.value - i) < _w->hdial.minimum) ? _w->hdial.minimum : _w->hdial.value - i;
    XtSetArg(warg, XtNvalue, newVal);
    XtSetValues((Widget)_w, &warg, 1);
  }
  hdcs.reason = HDIAL_DECREMENT;
  hdcs.event = _event;
  hdcs.value = _w->hdial.value;
  XtCallCallbacks((Widget)_w, XtNdecrementCallback, &hdcs);
}

#define SQR(a) ((a)*(a))
#define DIST(a,b,c,d) sqrt( (double)(SQR((a)-(c)) + SQR((b)-(d))) )

static void Set(_w, _event, _argv, _argc)
HdialWidget _w; 
XEvent *_event;
String *_argv;
int *_argc;
{
  double x, y;
  double angle, rangle;
  int newVal;
  Arg warg;
  HdialCallbackStruct hdcs;

  XFlush(XtDisplay(_w));
  switch (_event->type)
  {
    case ButtonPress:
    case ButtonRelease:
      x = ((XButtonEvent *)_event)->x;
      y = ((XButtonEvent *)_event)->y;
      break;
    case KeyPress:
    case KeyRelease:
      x = (double)((XKeyEvent *)_event)->x;
      y = (double)((XKeyEvent *)_event)->y;
      break;
    default: return; break;
  }

  if (DIST(x,y,_w->hdial.center.x, _w->hdial.center.y) > _w->hdial.length)
    return;

  rangle = atan2((double)(y - _w->hdial.center.y), 
                 (double)(x - _w->hdial.center.x)); 

  angle = DEGREES(-1.0  * rangle);

  if ((angle < 0.0) || (angle > 180.0))
    return;

  if (angle < 0)
    angle += 360.0;

  newVal = _w->hdial.maximum - (_w->hdial.maximum - _w->hdial.minimum) * (angle / 180.0); 
  XtSetArg(warg, XtNvalue, newVal);
  XtSetValues((Widget)_w, &warg, 1);
  hdcs.reason = HDIAL_SET;
  hdcs.event = _event;
  hdcs.value = _w->hdial.value;
  XtCallCallbacks((Widget)_w, XtNvalueChangeCallback, &hdcs);
}

static void Drag(_w, _xme)
HdialWidget _w; 
XMotionEvent *_xme;
{
  XButtonEvent xe;
  xe.type = ButtonPress;
  xe.x = _xme->x;
  xe.y = _xme->y;

  Set(_w, (XEvent *)&xe, NULL, NULL);
}

#define HDIAL _w->hdial

static void ReValue(_w)
HdialWidget _w;
{
  sprintf(_w->hdial.label, "%d", _w->hdial.value);
  HDIAL.labelHeight = HDIAL.font->ascent + HDIAL.font->descent;
  HDIAL.labelWidth = XTextWidth(HDIAL.font, HDIAL.label, strlen(HDIAL.label));
  HDIAL.labelPos.x = HDIAL.center.x - (HDIAL.labelWidth / 2);
  HDIAL.labelPos.y = HDIAL.center.y + HDIAL.margin + HDIAL.labelHeight;

  MoveArm(_w);
}

#undef HDIAL

#define MAXSTRING 300

static void MyXtWarning(_format, va_alist)
char *_format;
va_dcl   /* stupid define already has a ; on it */
{
  va_list parms;
  char dest[MAXSTRING];

  va_start(parms);
  vsprintf(dest, _format, parms);
  va_end(parms);

  XtWarning(dest);
}

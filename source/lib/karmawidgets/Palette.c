/*LINTLIBRARY*/
/*  Palette.c

    This code provides a colour palette widget for Xt.

    Copyright (C) 1993,1994  Patrick Jordan
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

/*  This file contains all routines needed for manipulating a colour palette
    widget for Xt.


    Written by      Patrick Jordan  7-JUL-1993

    Updated by      Richard Gooch   29-SEP-1993

    Last updated by Richard Gooch   19-JUL-1994: Check if widget is realise
  in  DrawColourBar  .


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

#include <Xkw/PaletteP.h>

/*----------------------------------------------------------------------*/
/* Function Prototypes*/
/*----------------------------------------------------------------------*/

/* Actions*/

static void Set(/* Widget w, XEvent *event, String *argv, int *argc */);
static void Drag(/* Widget w, XMotionEvent *xme */);

/* Widget Internals  */

static void Initialize(/* Widget request,Widget new */);
static void Resize(/* Widget w */);
static void Destroy(/* Widget w */);
static void Redisplay(/* Widget w, XEvent *event, Region region */);
static Boolean SetValues(/* Widget current,Widget request,Widget new */);

/* Other*/

static void ClassInitialize();
static void DrawSpot(/* Widget w, GC gc */);
static void DrawColourBar(Widget W);

/*----------------------------------------------------------------------*/
/* Translations etc*/
/*----------------------------------------------------------------------*/

static char PaletteTranslations[] =
 "<Btn1Down>:    set()\n\
 <Btn1Motion>:  set() drag()";

static XtActionsRec PaletteActions[] = 
{
  {"set",(XtActionProc)Set},
  {"drag", (XtActionProc)Drag}
};

/*----------------------------------------------------------------------*/
/* Some macros*/
/*----------------------------------------------------------------------*/

#define PALETTE_MINWIDTH 30
#define PALETTE_MINHEIGHT 100

/*----------------------------------------------------------------------*/
/* Default Resources*/
/*----------------------------------------------------------------------*/

#define TheOffset(field) XtOffset(PaletteWidget, palette.field)

static float zero=0.0;
static float one=1.0;
static float half=0.5;

static XtResource PaletteResources[] = 
{
  {XtNforeground, XtCForeground, XtRPixel, sizeof(Pixel),
    TheOffset(foreground), XtRString, (caddr_t)XtDefaultForeground},
  {XkwNminimum, XkwCMinimum, XtRFloat, sizeof(float),
    TheOffset(minimum), XtRFloat, (XtPointer)&zero},
  {XkwNmaximum, XkwCMaximum, XtRFloat, sizeof(float),
    TheOffset(maximum), XtRFloat, (XtPointer)&one},
  {XtNvalue, XtCValue, XtRFloat, sizeof(float),
    TheOffset(value), XtRFloat, (XtPointer)&half},
  {XkwNkarmaColourmap, XkwCKarmaColourmap, XtRPointer, sizeof(XtPointer),
    TheOffset(dcm), XtRPointer, (XtPointer)NULL},
  {XkwNvalueChangeCallback, XtCCallback, XtRCallback, sizeof(caddr_t),
   TheOffset(valueChangeCallback), XtRCallback, (caddr_t)NULL},
  {XkwNorientation, XkwCOrientation, XkwROrientation, sizeof(XkwOrientation),
   TheOffset(orientation), XtRString, "horizontal"},
};

#undef TheOffset

/*----------------------------------------------------------------------*/
/* Core class values*/
/*----------------------------------------------------------------------*/

PaletteClassRec paletteClassRec = 
{
  {        /* CoreClassPart */
    (WidgetClass)&widgetClassRec,  /* superclass */
    "Palette",                     /* class_name */
    sizeof(PaletteRec),            /* widget_size */
    (XtProc)ClassInitialize,       /* class_initialize */
    NULL,                          /* class_part_initialize */
    FALSE,                         /* class_inited*/
    (XtInitProc)Initialize,        /* initialize */
    NULL,                          /* initialize_hook */
    XtInheritRealize,              /* realize */
    PaletteActions,                /* actions */
    XtNumber(PaletteActions),      /* num_actions */
    PaletteResources,              /* resources */
    XtNumber(PaletteResources),    /* num_resources */
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
    PaletteTranslations,           /* tm_translations */
    NULL,
    NULL,
    NULL,
  },
  { 
    0 /* empty */
  }
};

WidgetClass paletteWidgetClass = (WidgetClass) &paletteClassRec;


/*----------------------------------------------------------------------*/
/* Converts a character string to all lowercase for use in the */
/* resource string conversion routines.*/
/*----------------------------------------------------------------------*/

static void ToLower(char *source,char *dest)
{
  char ch;

  for (; (ch = *source) != 0; source++, dest++)  {
    if ('A' <= ch && ch <= 'Z')  *dest = ch - 'A' + 'a';
    else  *dest = ch;
  }
  *dest = (char)NULL;
}

/*----------------------------------------------------------------------*/
/* This macro is used by the resource string conversion routines to */
/* assign the converted type elements to the XrmValue struct.*/
/*----------------------------------------------------------------------*/

#define CvtDone(type, address) \
{ toVal->size = sizeof(type); \
  toVal->addr = (caddr_t)address; \
  return; } 


/*----------------------------------------------------------------------*/
/* This function converts resource settings in string form to the*/
/* XkwROrient representation type.*/
/*----------------------------------------------------------------------*/

static void CvtStringToOrient(XrmValuePtr args,Cardinal *num_args,
			      XrmValuePtr fromVal,XrmValuePtr toVal)
{
  static XkwOrientation orient;
  char lowerstring[100];

  /* Convert the resource string to lower case for quick comparison*/
  ToLower((char *)fromVal->addr, lowerstring);

  /* Compare resource string with valid XkwOrientation strings*/
  /* and assign to datatype.*/

  if (strcmp(lowerstring, XkwEvertical) == 0)  {
    orient = XkwVertical;
    CvtDone(XkwOrientation, &orient);
  }
  else if (strcmp(lowerstring, XkwEhorizontal) == 0) {
    orient = XkwHorizontal;
    CvtDone(XkwOrientation, &orient);
  }

  /* If the string is not valid for this resource type, print a warning*/
  /* and do not make the conversion.*/

  XtStringConversionWarning(fromVal->addr, "XkwOrientation");
  toVal->addr = NULL;
  toVal->size = 0;
}

/*----------------------------------------------------------------------*/
/* Check some values*/
/*----------------------------------------------------------------------*/

static void check_values(min,max,value)
     float *min,*max,*value;
{
  if (*min >= *max)  *max = *min+1;
  if (*value < *min) *value = *min;
  if (*value > *max) *value = *max;
}

/*----------------------------------------------------------------------*/
/* This method initializes the Slider widget class. Specifically,*/
/* it registers resource value converter functions with Xt.*/
/*----------------------------------------------------------------------*/

static void ClassInitialize()
{
   XtAddConverter(XtRString, XkwROrientation, CvtStringToOrient, NULL, 0);
}

/*----------------------------------------------------------------------*/
/* Resize cmap function. This function is called when the kcmap is resized */
/*----------------------------------------------------------------------*/

static void resize_kcmap(Kcolourmap cmp, void **info)
{
  Widget w=(Widget)*info;   /* w is the palette widget */
  DrawColourBar(w);
}

/*----------------------------------------------------------------------*/
/* Initialisation method*/
/*----------------------------------------------------------------------*/

static void Initialize(Widget Request,Widget New)
{
  PaletteWidget request = (PaletteWidget) Request;
  PaletteWidget new = (PaletteWidget) New;
  XGCValues gcValues;
  XtGCMask value,dynamic,dontcare;
  Display *display;

  display = XtDisplay(new);

  check_values(&new->palette.minimum,&new->palette.maximum,
	       &new->palette.value);

  if (request->core.width == 0)
    new->core.width = PALETTE_MINWIDTH;
  if (request->core.height == 0)
    new->core.height = PALETTE_MINHEIGHT;

  /* Gc for thumb*/
  gcValues.foreground = new->palette.foreground;
  gcValues.background = new->core.background_pixel;
  gcValues.line_width = 3;
  new->palette.gc=
    XtGetGC(New, GCForeground | GCBackground | GCLineWidth, &gcValues);

  /* Gc for erasing thumb*/
  gcValues.foreground = new->core.background_pixel;
  gcValues.background = new->core.background_pixel;
  gcValues.line_width = 3;
  new->palette.eraseGC =
    XtGetGC(New,GCForeground | GCBackground | GCLineWidth, &gcValues);

  /* GC for colourbar*/
  dynamic = GCForeground;
  dontcare = 0;
  value = GCLineStyle | GCCapStyle | GCLineWidth;
  gcValues.cap_style=CapRound;
  gcValues.line_style=LineSolid;
  gcValues.line_width=0;
  gcValues.foreground=new->core.background_pixel;
  new->palette.cbarGC=XtAllocateGC(New,0,value,&gcValues,
				   dynamic,dontcare);

  new->palette.division=2*new->core.width/3;

  new->palette.dcm=NULL;
  Resize(New);
}

/*----------------------------------------------------------------------*/
/* Resize method*/
/*----------------------------------------------------------------------*/

static void Resize(Widget W)
{
  PaletteWidget w = (PaletteWidget) W;

  if(w->palette.orientation==XkwVertical) {
    w->palette.scale=
      (w->palette.maximum-w->palette.minimum)/w->core.height;
    w->palette.division=2*w->core.width/3;
  } else {
    w->palette.scale=
      (w->palette.maximum-w->palette.minimum)/w->core.width;
    w->palette.division=2*w->core.height/3;
  }
}

/*----------------------------------------------------------------------*/
/* Destroy method*/
/*----------------------------------------------------------------------*/

static void Destroy(Widget W)
{
  PaletteWidget w = (PaletteWidget) W;
  XtReleaseGC(W, w->palette.gc);
  XtReleaseGC(W, w->palette.eraseGC);
  XtRemoveAllCallbacks(W, XkwNvalueChangeCallback);
}

/*----------------------------------------------------------------------*/
/* Draw ColourBar function*/
/*----------------------------------------------------------------------*/

static void DrawColourBar(Widget W)
{
  PaletteWidget w = (PaletteWidget) W;
  int i,clr;
  float scale;
  GC gc;
  int numpix;
  unsigned long *pixvals;

  if (!XtIsRealized(W))
    return;
  numpix=kcmap_get_pixels(w->palette.dcm,&pixvals);

  if (w->core.visible) {
    if(w->palette.orientation==XkwVertical) {
      for(i=0;i<w->core.height;i++) {
	clr=(int)(i*((float)numpix/(float)w->core.height));
	XSetForeground(XtDisplay(w),w->palette.cbarGC,pixvals[clr]);
	XDrawLine(XtDisplay(w),XtWindow(w),w->palette.cbarGC,
		  0,i,w->palette.division,i);
      }
    } else {
      for(i=0;i<w->core.width;i++) {
	clr=(int)(i*((float)numpix/(float)w->core.width));
	XSetForeground(XtDisplay(w),w->palette.cbarGC,pixvals[clr]);
	XDrawLine(XtDisplay(w),XtWindow(w),w->palette.cbarGC,
		  i,0,i,w->palette.division);
      }
    }
  }
}

/*----------------------------------------------------------------------*/
/* DrawSpot function*/
/*----------------------------------------------------------------------*/

static void DrawSpot(Widget W,GC gc)
{
  PaletteWidget w = (PaletteWidget) W;
  float y=(w->palette.value+w->palette.minimum)/w->palette.scale;
  
  if (w->core.visible) {
    if(w->palette.orientation==XkwVertical) {
      XDrawLine(XtDisplay(w),XtWindow(w),gc,
		w->palette.division+1,y,w->core.width-1,y);
    } else {
      XDrawLine(XtDisplay(w),XtWindow(w),gc,
		y,w->palette.division+1,y,w->core.height-1);
    }
  }
}

/*----------------------------------------------------------------------*/
/* Redisplay method*/
/*----------------------------------------------------------------------*/

static void Redisplay(Widget W,XEvent *event,Region region)
{
  PaletteWidget w = (PaletteWidget) W;
  if (w->core.visible)
    {
      XClearWindow(XtDisplay(w), XtWindow(w));
      DrawSpot(W, w->palette.gc);
      DrawColourBar(W);
    }
  XFlush(XtDisplay(w));
}

/*----------------------------------------------------------------------*/
/* SetVaues method*/
/*----------------------------------------------------------------------*/

static Boolean SetValues(Widget Current,Widget Request,Widget New)
{
  PaletteWidget current = (PaletteWidget) Current;
  PaletteWidget request = (PaletteWidget) Request;
  PaletteWidget new = (PaletteWidget) New;
  Boolean redisplay = FALSE;
  Display *display = XtDisplay(new);
  Boolean newErase = FALSE;
  XGCValues eraseGCValues, gcVal;
  int eraseMask = 0;

  check_values(&new->palette.minimum,&new->palette.maximum,
	       &new->palette.value);

  if (new->palette.minimum != current->palette.minimum ||
      new->palette.maximum != current->palette.maximum ||
      new->core.height != current->core.height ||
      new->core.width != current->core.width ||
      new->palette.orientation != current->palette.orientation) {
    Resize(New);
  }
  
  if (new->palette.foreground != current->palette.foreground)  {
    XtReleaseGC(New, new->palette.gc);
    gcVal.foreground = new->palette.foreground;
    new->palette.gc = XtGetGC(New, GCForeground, &gcVal);
    redisplay = TRUE;
  }

  if (new->core.background_pixel != current->core.background_pixel)  {
    eraseGCValues.foreground = new->core.background_pixel;
    newErase = TRUE;
    eraseMask |= GCForeground;
    redisplay = TRUE;
  }

  if (newErase)  {
    XtReleaseGC(New, new->palette.eraseGC);
    new->palette.eraseGC = XtGetGC(New, eraseMask, &eraseGCValues);
  }

  if (new->palette.value != current->palette.value) {
    if (!redisplay)  {
      DrawSpot(Current, new->palette.eraseGC);
      DrawSpot(New, new->palette.gc);
      return FALSE;
    }
  }

  if(new->palette.dcm != current->palette.dcm) {
    kcmap_register_resize_func(new->palette.dcm,resize_kcmap,new);
  }
  
  return redisplay;
}

/*----------------------------------------------------------------------*/
/* Set action*/
/*----------------------------------------------------------------------*/

static void Set(Widget W,XEvent *event,String *argv,int *argc)
{
  PaletteWidget w = (PaletteWidget) W;
  float x, y;
  PaletteCallbackStruct hdcs;

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
  DrawSpot(W,w->palette.eraseGC);

  if(w->palette.orientation==XkwVertical) {
    w->palette.value=(float)y*w->palette.scale+w->palette.minimum;
  } else {
    w->palette.value=(float)x*w->palette.scale+w->palette.minimum;
  }

  check_values(&w->palette.minimum,&w->palette.maximum,&w->palette.value,"");
  
  DrawSpot(W,w->palette.gc);

  hdcs.value = w->palette.value;

  XtCallCallbacks(W, XkwNvalueChangeCallback, &hdcs);
}

/*----------------------------------------------------------------------*/
/* Drag action*/
/*----------------------------------------------------------------------*/

static void Drag(Widget W,XMotionEvent *xme)
{
  PaletteWidget w = (PaletteWidget) W;
  XButtonEvent xe;

  xe.type = ButtonPress;
  xe.x = xme->x;
  xe.y = xme->y;

  Set(W, (XEvent *)&xe, NULL, NULL);
}

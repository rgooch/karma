/*LINTLIBRARY*/
/*  Canvas.c

    This code provides a canvas widget for Xt.

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

/*

    This file contains all routines needed for a canvas widget for Xt.


    Written by      Richard Gooch   17-JUL-1994

    Updated by      Richard Gooch   19-JUL-1994

    Updated by      Richard Gooch   30-JUL-1994: Create graphics context on
  realisation.

    Updated by      Richard Gooch   9-AUG-1994: Added MapNotify and UnmapNotify
  event handlers.

    Updated by      Richard Gooch   12-OCT1994: Added ButtonRelease translation
  and used button release event codes.

    Updated by      Richard Gooch   25-OCT-1994: Moved some #include's around.

    Updated by      Richard Gooch   5-NOV-1994: Added VisibilityNotify event
  handler.

    Updated by      Richard Gooch   23-DEC-1994: Added realiseCallback resource

    Last updated by Richard Gooch   4-JAN-1995: Cosmetic changes.


*/

#include <stdio.h>
#define X11
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>
#include <karma.h>
#include <k_event_codes.h>
#include <Xkw/CanvasP.h>
#include <X11/Xmu/Drawing.h>

#define offset(field) XtOffsetOf(CanvasRec, canvas.field)

static XtResource resources[] =
{
    {XtNinsensitiveBorder, XtCInsensitive, XtRPixmap, sizeof (Pixmap),
     offset (insensitive_border), XtRImmediate, (XtPointer) NULL},
    {XtNvisual, XtCVisual, XtRVisual, sizeof (Visual *),
     offset (visual), XtRImmediate, CopyFromParent},
    {XkwNpixCanvas, XkwCPixCanvas, XtRPointer, sizeof (XtPointer *),
     offset (pixcanvas), XtRImmediate, NULL},
    {XkwNclipEvents, XkwCClipEvents, XtRBool, sizeof (Boolean),
     offset (clip), XtRImmediate, False},
    {XkwNsilenceUnconsumed, XkwCSilenceUnconsumed, XtRBool, sizeof (Boolean),
     offset (silence_unconsumed_events), XtRImmediate, False},
    {XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
     offset (foreground_pixel), XtRString, XtDefaultForeground},
    {XkwNforceNewCmap, XkwCForceNewCmap, XtRBool, sizeof (Boolean),
     offset (force_new_cmap), XtRImmediate, False},
    {XkwNretainFgBg, XkwCRetainFgBg, XtRBool, sizeof (Boolean),
     offset (retain_fgbg), XtRImmediate, False},
    {XkwNrealiseCallback, XtCCallback, XtRCallback, sizeof (caddr_t),
     offset (realiseCallback), XtRCallback, (caddr_t) NULL},
#undef offset
};

static void ClassPartInitialize ();
static void ClassInitialize ();
static void Realise ();
static Boolean SetValues ();
static Boolean ChangeSensitive ();
static void Redisplay ();
static void CanvasColourmapChange ();
static void CanvasEnterNotify ();
static void CanvasLeaveNotify ();
static void CanvasMotionNotify ();
static void CanvasButtonNotify ();
static void CanvasMapNotify ();
static void CanvasUnmapNotify ();
static void CanvasVisibilityNotify ();

static char defaultTranslations[] =
  "#override \n\
   <ColormapNotify>:    CanvasColourmapChange()  \n\
   <EnterNotify>:       CanvasEnterNotify() \n\
   <LeaveNotify>:       CanvasLeaveNotify() \n\
   <MotionNotify>:      CanvasMotionNotify() \n\
   <ButtonPress>:       CanvasButtonNotify() \n\
   <ButtonRelease>:     CanvasButtonNotify() \n\
   <MapNotify>:         CanvasMapNotify() \n\
   <UnmapNotify>:       CanvasUnmapNotify() \n\
   <VisibilityNotify>:  CanvasVisibilityNotify()";

static XtActionsRec actions[] = {
  {"CanvasColourmapChange",      CanvasColourmapChange},
  {"CanvasEnterNotify",          CanvasEnterNotify},
  {"CanvasLeaveNotify",          CanvasLeaveNotify},
  {"CanvasMotionNotify",         CanvasMotionNotify},
  {"CanvasButtonNotify",         CanvasButtonNotify},
  {"CanvasMapNotify",            CanvasMapNotify},
  {"CanvasUnmapNotify",          CanvasUnmapNotify},
  {"CanvasVisibilityNotify",     CanvasVisibilityNotify},
};

CanvasClassRec canvasClassRec =
{
    {
	/* core fields  */
	(WidgetClass) &widgetClassRec,  /*  superclass             */
	"Canvas",                       /*  class_name             */
	sizeof(CanvasRec),              /*  widget_size            */
	ClassInitialize,                /*  class_initialize       */
	ClassPartInitialize,            /*  class_part_initialize  */
	FALSE,                          /*  class_inited           */
	NULL,                           /*  initialize             */
	NULL,                           /*  initialize_hook        */
	Realise,                        /*  realise                */
	actions,                        /*  actions                */
	XtNumber(actions),              /*  num_actions            */
	resources,                      /*  resources              */
	XtNumber(resources),            /*  num_resources          */
	NULLQUARK,                      /*  xrm_class              */
	TRUE,                           /*  compress_motion        */
	TRUE,                           /*  compress_exposure      */
	TRUE,                           /*  compress_enterleave    */
	TRUE,                           /*  visible_interest       */
	NULL,                           /*  destroy                */
	NULL,                           /*  resize                 */
	Redisplay,                      /*  expose                 */
	SetValues,                      /*  set_values             */
	NULL,                           /*  set_values_hook        */
	XtInheritSetValuesAlmost,       /*  set_values_almost      */
	NULL,                           /*  get_values_hook        */
	NULL,                           /*  accept_focus           */
	XtVersion,                      /*  version                */
	NULL,                           /*  callback_private       */
	defaultTranslations,            /*  tm_table               */
	XtInheritQueryGeometry,         /*  query_geometry         */
	XtInheritDisplayAccelerator,    /*  display_accelerator    */
	NULL                            /*  extension              */
    },
    {
	/*  canvas fields */
	ChangeSensitive                 /*  change_sensitive       */
    }
};

WidgetClass canvasWidgetClass = (WidgetClass) &canvasClassRec;

static void ClassInitialize ()
{
    static XtConvertArgRec convertArg[] = {
        {XtWidgetBaseOffset, (XtPointer) XtOffsetOf(WidgetRec, core.screen),
	 sizeof(Screen *)},
        {XtWidgetBaseOffset, (XtPointer) XtOffsetOf(WidgetRec, core.colormap),
	 sizeof(Colormap)}
    };

    XawInitializeWidgetSet();
}

static void ClassPartInitialize (class)
WidgetClass class;
{
    register CanvasWidgetClass c = (CanvasWidgetClass) class;
    register CanvasWidgetClass super;

    super = (CanvasWidgetClass) c->core_class.superclass;

    if (c->canvas_class.change_sensitive == NULL)
    {
	char buf[BUFSIZ];

	(void) sprintf (buf,
			"%s Widget: The Canvas Widget class method %s%s",
			c->core_class.class_name,
			"'change_sensitive' is undefined.\n",
			"A function must be defined or inherited.");
	XtWarning(buf);
	c->canvas_class.change_sensitive = ChangeSensitive;
    }
    if (c->canvas_class.change_sensitive == XtInheritChangeSensitive)
    c->canvas_class.change_sensitive = super->canvas_class.change_sensitive;
}

static void Realise (w, valueMask, attributes)
register Widget w;
Mask *valueMask;
XSetWindowAttributes *attributes;
{
    KPixCanvas pixcanvas;
    int count;
    GC gc;
    int num_vinfos, num_cmaps;
    XWindowAttributes parent_att;
    Widget parent;
    Colormap cmap, *cmap_list;
    Display *dpy;
    Visual *visual;
    Screen *screen;
    CanvasWidget cnv = (CanvasWidget) w;
    Pixmap border_pixmap;
    XVisualInfo vinfo_template;
    XGCValues gcvalues;
    XVisualInfo *vinfos;
    unsigned long pixels[2];
    XColor colour_defs[2];
    static char function_name[] = "Canvas::Realise";

    if (!XtIsSensitive(w))
    {
	/* change border to gray; have to remember the old one,
	 * so XtDestroyWidget deletes the proper one */
	if (((CanvasWidget)w)->canvas.insensitive_border == None)
	((CanvasWidget)w)->canvas.insensitive_border =
	XmuCreateStippledPixmap(XtScreen(w),
				w->core.border_pixel, 
				w->core.background_pixel,
				w->core.depth);
        border_pixmap = w->core.border_pixmap;
	attributes->border_pixmap =
	w->core.border_pixmap = ((CanvasWidget)w)->canvas.insensitive_border;

	*valueMask |= CWBorderPixmap;
	*valueMask &= ~CWBorderPixel;
    }

    /*  Create the X window  */
    visual = cnv->canvas.visual;
    dpy = XtDisplay (cnv);
    parent = XtParent (w);
    screen = XtScreen (parent);
    /*  Determine parent information  */
    if (XGetWindowAttributes (dpy, XtWindow (parent), &parent_att) == 0)
    {
	(void) fprintf (stderr, "Error getting parent window attributes\n");
	a_prog_bug (function_name);
    }
    if (visual == CopyFromParent) visual = parent_att.visual;
    if (visual != parent_att.visual)
    {
	/*  Canvas visual different from parent  */
	vinfo_template.visualid = XVisualIDFromVisual (visual);
	vinfos = XGetVisualInfo (dpy, VisualIDMask, &vinfo_template,
				 &num_vinfos);
	if (num_vinfos < 1)
	{
	    (void) fprintf (stderr,
			    "Error getting visual info for visual: %p\n",
			    visual);
	    a_prog_bug (function_name);
	}
	if (num_vinfos > 1)
	{
	    (void) fprintf (stderr,
			    "%s: WARNING: number of visuals for visual: %p is: %d\n",
			    function_name, visual, num_vinfos);
	}
	(void) fprintf (stderr,
			"creating window with visualID: 0x%x\tdepth: %d\n",
			vinfos[0].visualid, vinfos[0].depth);
	cnv->core.depth = vinfos[0].depth;
	XFree (vinfos);
	if (parent_att.colormap == cnv->core.colormap)
	{
	    /*  Invalidate original colourmap  */
	    cnv->canvas.force_new_cmap = True;
	}
    }
    if (cnv->canvas.force_new_cmap)
    {
	/*  Need to create a colourmap  */
	if ( ( cnv->core.colormap =
	      XCreateColormap (dpy, XRootWindowOfScreen (screen),
			       visual, AllocNone) ) == (Colormap) None )
	{
	    (void) fprintf (stderr, "Could not create colourmap\n");
	    exit (1);
	}
	(void) fprintf (stderr, "new cmap: %x\n", cnv->core.colormap);
	/* Copy over foreground and background colours from parent colourmap */
	colour_defs[0].pixel = cnv->canvas.foreground_pixel;
	colour_defs[1].pixel = cnv->core.background_pixel;
	XQueryColors (dpy, parent_att.colormap, colour_defs, 2);
	if (XAllocColor (dpy, cnv->core.colormap, colour_defs) == 0)
	{
	    (void) fprintf (stderr, "Could not allocate colour\n");
	    exit (1);
	}
	if (XAllocColor (dpy, cnv->core.colormap, colour_defs + 1) == 0)
	{
	    (void) fprintf (stderr, "Could not allocate colour\n");
	    exit (1);
	}
	pixels[0] = colour_defs[0].pixel;
	pixels[1] = colour_defs[1].pixel;
	if (!cnv->canvas.retain_fgbg) 
	{
	    XFreeColors (dpy, cnv->core.colormap, pixels, 2, 0);
	}
	cnv->canvas.foreground_pixel = pixels[0];
	cnv->core.background_pixel = pixels[1];
    }
    attributes->colormap = cnv->core.colormap;
    attributes->background_pixel = cnv->core.background_pixel;
    *valueMask |= CWColormap | CWBackPixel;
    XtCreateWindow (w, (unsigned int) InputOutput, visual,
		    *valueMask, attributes);
    /*  Create graphics context  */
    gcvalues.function = GXcopy;
    gcvalues.foreground = cnv->canvas.foreground_pixel;
    gcvalues.background = cnv->core.background_pixel;
    gc = XCreateGC (dpy, cnv->core.window,
		    GCForeground | GCBackground | GCFunction,
		    &gcvalues);
    /*  Create the KPixCanvas  */
    if ( ( pixcanvas =
	  kwin_create_x (dpy, cnv->core.window, gc,
			 0, 0, cnv->core.width, cnv->core.height) )
	== NULL )
    {
	m_abort (function_name, "pixel canvas");
    }
    kwin_set_attributes (pixcanvas,
			 KWIN_ATT_VISIBLE, FALSE,
			 KWIN_ATT_END);
    cnv->canvas.cmap_installed = FALSE;
    cnv->canvas.mapped = FALSE;
    cnv->canvas.partly_unobscured = FALSE;
    cmap_list = XListInstalledColormaps (dpy, cnv->core.window, &num_cmaps);
    cmap = cnv->core.colormap;
    for (count = 0; count < num_cmaps; ++count)
    {
	if (cmap == cmap_list[count]) cnv->canvas.cmap_installed = TRUE;
    }
    XFree (cmap_list);
    cnv->canvas.cmap_installed_internally = FALSE;
    cnv->canvas.pixcanvas = pixcanvas;
    if ( !XtIsSensitive(w) )
    w->core.border_pixmap = border_pixmap;
    XtCallCallbacks (w, XkwNrealiseCallback,
		     (XtPointer) cnv->canvas.pixcanvas);
}   /*  End Function Realise  */


/* ARGSUSED */
static Boolean SetValues (Current, Request, New)
Widget Current, Request, New;
{
    CanvasWidget old = (CanvasWidget) Current;
    CanvasWidget new = (CanvasWidget) New;

    new->canvas.pixcanvas = old->canvas.pixcanvas;
    new->canvas.visual = old->canvas.visual;

    if ( XtIsSensitive(Current) != XtIsSensitive(New) )
    (*((CanvasWidgetClass)XtClass(New))->
     canvas_class.change_sensitive) (New);

    return False;   
}


static Boolean ChangeSensitive(w)
    register Widget w;
{
    if (XtIsRealized(w)) {
if (XtIsSensitive(w))
    if (w->core.border_pixmap != XtUnspecifiedPixmap)
XSetWindowBorderPixmap( XtDisplay(w), XtWindow(w),
        w->core.border_pixmap );
    else
XSetWindowBorder( XtDisplay(w), XtWindow(w), 
  w->core.border_pixel );
else {
    if (((CanvasWidget)w)->canvas.insensitive_border == None)
((CanvasWidget)w)->canvas.insensitive_border =
    XmuCreateStippledPixmap(XtScreen(w),
    w->core.border_pixel, 
    w->core.background_pixel,
    w->core.depth);
    XSetWindowBorderPixmap( XtDisplay(w), XtWindow(w),
    ((CanvasWidget)w)->
        canvas.insensitive_border );
}
    }
    return False;
}

static void Redisplay (cnv, event, region)
CanvasWidget cnv;
XExposeEvent *event;
Region region;
{
    Widget w = (Widget) cnv;

    if ( !XtIsRealized (w) )
    {
	(void) fprintf (stderr, "Expose on non-realised canvas widget!\n");
	return;
    }
    if (!cnv->canvas.cmap_installed)
    {
	(void) fprintf (stderr, "installing cmap: 0x%x\n",
			cnv->core.colormap);
	XInstallColormap (XtDisplay (cnv), cnv->core.colormap);
	cnv->canvas.cmap_installed_internally = TRUE;
    }
    if (!cnv->core.visible) return;
    if ( !kwin_resize (cnv->canvas.pixcanvas, FALSE,
		       0, 0, cnv->core.width, cnv->core.height) )
    {
	(void) fprintf (stderr,
			"Error redisplaying KPixCanvas of canvas widget\n");
	return;
    }
}   /*  End Function Redisplay  */

static void CanvasColourmapChange (cnv, report)
CanvasWidget cnv;
XEvent *report;
{
    static char function_name[] = "Canvas::CanvasColourmapChange";

#ifdef dummy
    switch (report->xcolormap.state)
    {
      case ColormapInstalled:
	(void) fprintf (stderr, "cmap: 0x%x installed\n",
			report->xcolormap.colormap);
	break;
      case ColormapUninstalled:
	(void) fprintf (stderr, "cmap: 0x%x uninstalled\n",
			report->xcolormap.colormap);
	break;
      default:
	(void) fprintf (stderr, "Illegal state: %d\n",
			report->xcolormap.state);
	a_prog_bug (function_name);
	break;
    }
#endif
    if (cnv->core.window != report->xcolormap.window) return;
    switch (report->xcolormap.state)
    {
      case ColormapInstalled:
	cnv->canvas.cmap_installed = TRUE;
	break;
      case ColormapUninstalled:
	cnv->canvas.cmap_installed = FALSE;
	break;
      default:
	(void) fprintf (stderr, "Illegal state: %d\n",
			report->xcolormap.state);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function CanvasColourmapChange   */

static void CanvasEnterNotify (cnv, report)
CanvasWidget cnv;
XEvent *report;
{
    if (!cnv->canvas.cmap_installed)
    {
#ifdef dummy
	(void) fprintf (stderr, "installing cmap: 0x%x\n",
			cnv->core.colormap);
#endif
	XInstallColormap (XtDisplay (cnv), cnv->core.colormap);
	cnv->canvas.cmap_installed_internally = TRUE;
    }
}   /*  End Function CanvasEnterNotify   */

static void CanvasLeaveNotify (cnv, report)
CanvasWidget cnv;
XEvent *report;
{
#ifdef dummy
    if (cnv->canvas.cmap_installed_internally)
    {
	(void) fprintf (stderr, "uninstalling my cmap\n");
	XUninstallColormap (XtDisplay (cnv), cnv->core.colormap);
	cnv->canvas.cmap_installed = FALSE;
	cnv->canvas.cmap_installed_internally = FALSE;
    }
#endif
}   /*  End Function CanvasLeaveNotify   */

static void CanvasMotionNotify (cnv, report)
CanvasWidget cnv;
XEvent *report;
{
    flag consumed;
    unsigned int k_event;

    if (!cnv->canvas.cmap_installed)
    {
#ifdef dummy
	(void) fprintf (stderr, "installing cmap: 0x%x\n",
			cnv->core.colormap);
#endif
	XInstallColormap (XtDisplay (cnv), cnv->core.colormap);
	cnv->canvas.cmap_installed_internally = TRUE;
    }
    if (report->xmotion.state & Button1Mask)
    {
	k_event = K_CANVAS_EVENT_LEFT_MOUSE_DRAG;
    }
    else if (report->xmotion.state & Button2Mask)
    {
	k_event = K_CANVAS_EVENT_MIDDLE_MOUSE_DRAG;
    }
    else if (report->xmotion.state & Button3Mask)
    {
	k_event = K_CANVAS_EVENT_RIGHT_MOUSE_DRAG;
    }
    else
    {
	k_event = K_CANVAS_EVENT_POINTER_MOVE;
    }
    consumed = kwin_process_position_event (cnv->canvas.pixcanvas,
					    report->xmotion.x,
					    report->xmotion.y,
					    cnv->canvas.clip, k_event,
					    NULL);
    if (consumed || cnv->canvas.silence_unconsumed_events) return;
    (void) fprintf (stderr, "Karma event: %u not consumed\n", k_event);
}   /*  End Function CanvasMotionNotify   */

static void CanvasButtonNotify (cnv, report)
CanvasWidget cnv;
XEvent *report;
{
    flag consumed;
    unsigned int k_event;

    if (!cnv->canvas.cmap_installed)
    {
#ifdef dummy
	(void) fprintf (stderr, "installing cmap: 0x%x\n",
			cnv->core.colormap);
#endif
	XInstallColormap (XtDisplay (cnv), cnv->core.colormap);
	cnv->canvas.cmap_installed_internally = TRUE;
    }
    switch (report->xbutton.button)
    {
      case Button1:
	if (report->xbutton.state & Button1Mask)
	{
	    k_event = K_CANVAS_EVENT_LEFT_MOUSE_RELEASE;
	}
	else
	{
	    k_event = K_CANVAS_EVENT_LEFT_MOUSE_CLICK;
	}
	break;
      case Button2:
	if (report->xbutton.state & Button2Mask)
	{
	    k_event = K_CANVAS_EVENT_MIDDLE_MOUSE_RELEASE;
	}
	else
	{
	    k_event = K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK;
	}
	break;
      case Button3:
	if (report->xbutton.state & Button3Mask)
	{
	    k_event = K_CANVAS_EVENT_RIGHT_MOUSE_RELEASE;
	}
	else
	{
	    k_event = K_CANVAS_EVENT_RIGHT_MOUSE_CLICK;
	}
	break;
      default:
	k_event = K_CANVAS_EVENT_UNDEFINED;
	break;
    }
    consumed = kwin_process_position_event (cnv->canvas.pixcanvas,
					    report->xbutton.x,
					    report->xbutton.y,
					    cnv->canvas.clip, k_event,
					    NULL);
    if (consumed || cnv->canvas.silence_unconsumed_events) return;
    (void) fprintf (stderr, "Karma event: %u not consumed\n", k_event);
}   /*  End Function CanvasButtonNotify   */

static void CanvasMapNotify (cnv, report)
CanvasWidget cnv;
XEvent *report;
{
    cnv->canvas.mapped = TRUE;
    kwin_set_attributes (cnv->canvas.pixcanvas,
			 KWIN_ATT_VISIBLE, cnv->canvas.partly_unobscured,
			 KWIN_ATT_END);
}   /*  End Function CanvasMapNotify   */

static void CanvasUnmapNotify (cnv, report)
CanvasWidget cnv;
XEvent *report;
{
    cnv->canvas.mapped = TRUE;
    kwin_set_attributes (cnv->canvas.pixcanvas,
			 KWIN_ATT_VISIBLE, FALSE,
			 KWIN_ATT_END);
}   /*  End Function CanvasUnmapNotify   */

static void CanvasVisibilityNotify (cnv, report)
CanvasWidget cnv;
XEvent *report;
{
    switch (report->xvisibility.state)
    {
      case VisibilityFullyObscured:
	/*  This is only received when the window is obscured behind other
	    windows, not when it is iconified. There seem to be no events
	    dispatched when the window is iconified. A pity.  */
	cnv->canvas.partly_unobscured = FALSE;
	kwin_set_attributes (cnv->canvas.pixcanvas,
			     KWIN_ATT_VISIBLE, FALSE,
			     KWIN_ATT_END);
	break;
      case VisibilityPartiallyObscured:
      case VisibilityUnobscured:
	cnv->canvas.partly_unobscured = TRUE;
	kwin_set_attributes (cnv->canvas.pixcanvas,
			     KWIN_ATT_VISIBLE, cnv->canvas.mapped,
			     KWIN_ATT_END);
	break;
      default:
	(void) fprintf (stderr, "Unknown visibility event state: %d\n",
			report->xvisibility.state);
	break;
    }
}   /*  End Function CanvasVisibilityNotify   */

/*LINTLIBRARY*/
/*  Canvas.c

    This code provides a canvas widget for Xt.

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

    Updated by      Richard Gooch   4-JAN-1995: Cosmetic changes.

    Updated by      Richard Gooch   28-JAN-1995: Added stereo support.

    Updated by      Richard Gooch   17-APR-1995: Fixed mix of XtRBool and
  sizeof Boolean in resource fields. Use Bool because it is of int size.

    Updated by      Richard Gooch   26-JUL-1995: Removed buggy setting of
  border pixmap in <Realise>.

    Updated by      Richard Gooch   23-SEP-1995: Made use of XGL.

    Updated by      Richard Gooch   13-OCT-1995: Ensure XGL stereo canvases are
  not visible upon creation.

    Updated by      Richard Gooch   20-OCT-1995: Removed call to special XGL
  resize function.

    Updated by      Richard Gooch   22-OCT-1995: Force redisplay upon Expose
  even if canvas state not visible: print a warning message.

    Updated by      Richard Gooch   29-OCT-1995: Made use of OpenGL.

    Updated by      Richard Gooch   23-DEC-1995: Supported split (fake) stereo
  mode.

    Updated by      Richard Gooch   28-DEC-1995: Added verbose resource.

    Updated by      Richard Gooch   30-APR-1996: Added Initialise method.

    Updated by      Richard Gooch   4-MAY-1996: Subclassed from SimpleWidget.

    Updated by      Richard Gooch   9-MAY-1996: Took account of changes to
  OpenGL interface.

    Updated by      Richard Gooch   11-MAY-1996: Added verticalSplit and
  splitSeparation resources.

    Last updated by Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/

#include <stdio.h>
#define X11
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>
#include <X11/extensions/multibuf.h>
#include <karma.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>
#include <k_event_codes.h>
#include <Xkw/CanvasP.h>
#include <X11/Xmu/Drawing.h>

#define offset(field) XtOffsetOf(CanvasRec, canvas.field)

static XtResource resources[] =
{
    {XtNvisual, XtCVisual, XtRVisual, sizeof (Visual *),
     offset (visual), XtRImmediate, CopyFromParent},
    {XkwNstereoMode, XkwCStereoMode, XtRInt, sizeof (int),
     offset (stereoMode), XtRImmediate, XkwSTEREO_MODE_MONO},
    {XkwNverticalSplit, XkwCVerbose, XtRBool, sizeof (Bool),
     offset (verticalSplit), XtRImmediate, False},
    {XkwNsplitSeparation, XkwCSplitSeparation, XtRInt, sizeof (int),
     offset (splitSeparation), XtRImmediate, 0},
    {XkwNmonoPixCanvas, XkwCPixCanvas, XtRPointer, sizeof (XtPointer),
     offset (monoPixCanvas), XtRImmediate, NULL},
    {XkwNleftPixCanvas, XkwCPixCanvas, XtRPointer, sizeof (XtPointer),
     offset (leftPixCanvas), XtRImmediate, NULL},
    {XkwNrightPixCanvas, XkwCPixCanvas, XtRPointer, sizeof (XtPointer),
     offset (rightPixCanvas), XtRImmediate, NULL},
    {XkwNclipEvents, XkwCClipEvents, XtRBool, sizeof (Bool),
     offset (clip), XtRImmediate, False},
    {XkwNsilenceUnconsumed, XkwCSilenceUnconsumed, XtRBool, sizeof (Bool),
     offset (silence_unconsumed_events), XtRImmediate, False},
    {XtNforeground, XtCForeground, XtRPixel, sizeof (Pixel),
     offset (foreground_pixel), XtRString, XtDefaultForeground},
    {XkwNforceNewCmap, XkwCForceNewCmap, XtRBool, sizeof (Bool),
     offset (force_new_cmap), XtRImmediate, False},
    {XkwNretainFgBg, XkwCRetainFgBg, XtRBool, sizeof (Bool),
     offset (retain_fgbg), XtRImmediate, False},
    {XkwNrealiseCallback, XtCCallback, XtRCallback, sizeof (caddr_t),
     offset (realiseCallback), XtRCallback, (caddr_t) NULL},
    {XtNfont, XtCFont, XtRString, sizeof (String),
     offset (fontName), XtRImmediate, NULL},
    {XkwNverbose, XkwCVerbose, XtRBool, sizeof (Bool),
     offset (verbose), XtRImmediate, False},
    
#undef offset
};

STATIC_FUNCTION (void Canvas__Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (void Canvas__Realise, () );
STATIC_FUNCTION (Boolean Canvas__SetValues, () );
STATIC_FUNCTION (void Canvas__Redisplay, () );
STATIC_FUNCTION (void CanvasColourmapChange,
		 (CanvasWidget cnv, XEvent *event) );
STATIC_FUNCTION (void CanvasEnterNotify, (CanvasWidget cnv, XEvent *event) );
STATIC_FUNCTION (void CanvasMotionNotify, (CanvasWidget cnv, XEvent *event) );
STATIC_FUNCTION (void CanvasButtonNotify, (CanvasWidget cnv, XEvent *event) );
STATIC_FUNCTION (void CanvasMapNotify, (CanvasWidget cnv, XEvent *event) );
STATIC_FUNCTION (void CanvasUnmapNotify, (CanvasWidget cnv, XEvent *event) );
STATIC_FUNCTION (void CanvasVisibilityNotify,
		 (CanvasWidget cnv, XEvent *event) );
STATIC_FUNCTION (void CanvasKeyPress, (CanvasWidget cnv, XEvent *event) );
STATIC_FUNCTION (void create_stereo_window,
		 (Widget widget, unsigned int window_class, Visual *visual,
		  XtValueMask value_mask, XSetWindowAttributes *attributes,
		  Multibuffer *left, Multibuffer *right) );
STATIC_FUNCTION (void set_visibility, (CanvasWidget cnv, flag visible) );
STATIC_FUNCTION (void inject_event,
		 (CanvasWidget cnv, int x, int y, unsigned int k_event,
		  CONST char *string) );
STATIC_FUNCTION (XVisualInfo *get_visinfo_for_visual,
		 (Display *dpy, Visual *visual) );


static char defaultTranslations[] =
  "#override \n\
   <ColormapNotify>:    CanvasColourmapChange()  \n\
   <EnterNotify>:       CanvasEnterNotify() \n\
   <MotionNotify>:      CanvasMotionNotify() \n\
   <ButtonPress>:       CanvasButtonNotify() \n\
   <ButtonRelease>:     CanvasButtonNotify() \n\
   <MapNotify>:         CanvasMapNotify() \n\
   <UnmapNotify>:       CanvasUnmapNotify() \n\
   <VisibilityNotify>:  CanvasVisibilityNotify() \n\
   <KeyPress>:          CanvasKeyPress()";

static XtActionsRec actions[] = {
  {"CanvasColourmapChange",      (XtActionProc) CanvasColourmapChange},
  {"CanvasEnterNotify",          (XtActionProc) CanvasEnterNotify},
  {"CanvasMotionNotify",         (XtActionProc) CanvasMotionNotify},
  {"CanvasButtonNotify",         (XtActionProc) CanvasButtonNotify},
  {"CanvasMapNotify",            (XtActionProc) CanvasMapNotify},
  {"CanvasUnmapNotify",          (XtActionProc) CanvasUnmapNotify},
  {"CanvasVisibilityNotify",     (XtActionProc) CanvasVisibilityNotify},
  {"CanvasKeyPress",             (XtActionProc) CanvasKeyPress},
};

CanvasClassRec canvasClassRec =
{
    {
	/* core fields  */
	(WidgetClass) &simpleClassRec,  /*  superclass             */
	"Canvas",                       /*  class_name             */
	sizeof (CanvasRec),             /*  widget_size            */
	NULL,                           /*  class_initialise       */
	NULL,                           /*  class_part_initialise  */
	FALSE,                          /*  class_inited           */
	(XtInitProc) Canvas__Initialise,/*  initialise             */
	NULL,                           /*  initialise_hook        */
	Canvas__Realise,                /*  realise                */
	actions,                        /*  actions                */
	XtNumber (actions),             /*  num_actions            */
	resources,                      /*  resources              */
	XtNumber (resources),           /*  num_resources          */
	NULLQUARK,                      /*  xrm_class              */
	TRUE,                           /*  compress_motion        */
	TRUE,                           /*  compress_exposure      */
	TRUE,                           /*  compress_enterleave    */
	TRUE,                           /*  visible_interest       */
	NULL,                           /*  destroy                */
	NULL,                           /*  resize                 */
	Canvas__Redisplay,              /*  expose                 */
	Canvas__SetValues,              /*  set_values             */
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
    {   /* simple fields */
	XtInheritChangeSensitive        /* change_sensitive        */
    },
    {
	/*  canvas fields */
	0                               /*  ignore                 */
    }
};

WidgetClass canvasWidgetClass = (WidgetClass) &canvasClassRec;

static void Canvas__Initialise (Widget Request, Widget New)
{
    int nmono, nstereo, dummy;
    Window root_window;
    /*CanvasWidget request = (CanvasWidget) Request;*/
    CanvasWidget new = (CanvasWidget) New;
    Display *dpy;
    Screen *screen;
    Visual *visual;
    XVisualInfo *vinfo;
    XmbufBufferInfo *dummy_info1, *dummy_info2;
    static char function_name[] = "Canvas::Initialise";

    dpy = XtDisplay (New);
    screen = XtScreen (New);
    root_window = RootWindowOfScreen (screen);
    /*  Get visual information  */
    if ( (visual = new->canvas.visual) == CopyFromParent )
    {
	visual = XDefaultVisualOfScreen (screen);
    }
    vinfo = get_visinfo_for_visual (dpy, visual);
    /*  If stereo mode requested, check if supported  */
    switch (new->canvas.stereoMode)
    {
      case XkwSTEREO_MODE_XMBUF:
	if ( !XmbufQueryExtension (dpy, &dummy, &dummy) )
	{
	    (void) fprintf (stderr,
			    "Multi-Buffering extension not available\n");
	    exit (RV_UNDEF_ERROR);
	}
	if (XmbufGetScreenInfo (dpy, root_window, &nmono, &dummy_info1,
				&nstereo, &dummy_info2) == 0)
	{
	    (void) fprintf (stderr,
			    "Error getting Multi-Buffering screen info\n");
	    exit (RV_UNDEF_ERROR);
	}
	if (nmono > 0) XFree ( (char *) dummy_info1 );
	if (nstereo > 0) XFree ( (char *) dummy_info2 );
	if (nstereo < 1)
	{
	    (void) fprintf (stderr, "No stereo multibuffers available\n");
	    exit (RV_UNDEF_ERROR);
	}
	break;
      case XkwSTEREO_MODE_XGL:
	if (kwin_xgl_test_stereo (dpy, root_window)
	    != KWIN_XGL_STEREO_AVAILABLE)
	{
	    (void) fprintf (stderr, "XGL stereo not available\n");
	    exit (RV_UNDEF_ERROR);
	}
	break;
      case XkwSTEREO_MODE_OpenGL:
	if ( !kwin_open_gl_test_stereo (dpy, vinfo) )
	{
	    (void) fprintf (stderr, "OpenGL stereo not available\n");
	    exit (RV_UNDEF_ERROR);
	}
	break;
      case XkwSTEREO_MODE_SPLIT:
      case XkwSTEREO_MODE_MONO:
	break;
      default:
	(void) fprintf (stderr, "Illegal stereo mode: %d\n",
			new->canvas.stereoMode);
	a_prog_bug (function_name);
	return;
    }
    XFree ( (char *) vinfo );
}   /*  End Function Initialise  */

static void Canvas__Realise (register Widget w, Mask *valueMask,
			     XSetWindowAttributes *attributes)
{
    KPixCanvasFont font = NULL;  /*  Initialised to keep compiler happy  */
    KPixCanvas pixcanvas, stereopixcanvases[2];
    flag verbose;
    int count;
    int split_width, split_height, split_xoff, split_yoff;
    GC gc;
    int num_vinfos, num_cmaps;
    XWindowAttributes parent_att;
    Window parentwin;
    Widget parent;
    Colormap cmap, *cmap_list;
    Display *dpy;
    Visual *visual;
    Screen *screen;
    CanvasWidget cnv = (CanvasWidget) w;
    XVisualInfo vinfo_template;
    XGCValues gcvalues;
    XVisualInfo *vinfos;
    unsigned long pixels[2];
    XColor colour_defs[2];
    static char function_name[] = "Canvas::Realise";

    verbose = cnv->canvas.verbose;
    parentwin = (w->core.parent ? w->core.parent->core.window :
		 w->core.screen->root);
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
	if (verbose)
	{
	    (void) fprintf (stderr,
			    "%s: creating window with visualID: 0x%lx\tdepth: %d\n",
			    function_name,
			    vinfos[0].visualid, vinfos[0].depth);
	}
	cnv->core.depth = vinfos[0].depth;
	XFree ( (void *) vinfos );
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
	if (verbose) (void) fprintf (stderr, "%s: new cmap: 0x%lx\n",
				     function_name, cnv->core.colormap);
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
    switch (cnv->canvas.stereoMode)
    {
      case XkwSTEREO_MODE_XMBUF:
	create_stereo_window (w, (unsigned int) InputOutput, visual,
			      *valueMask, attributes,
			      &cnv->canvas.left_window,
			      &cnv->canvas.right_window);
	break;
      case XkwSTEREO_MODE_XGL:
      case XkwSTEREO_MODE_OpenGL:
	if (cnv->core.depth == 8) (void) fprintf (stderr, "bg: %lu\n", cnv->core.background_pixel);
	/*  Fall through  */
      case XkwSTEREO_MODE_SPLIT:
      case XkwSTEREO_MODE_MONO:
	XtCreateWindow (w, (unsigned int) InputOutput, visual,
			*valueMask, attributes);
	break;
      default:(void) fprintf (stderr, "Illegal stereo mode: %d\n",
			cnv->canvas.stereoMode);
	a_prog_bug (function_name);
	return;
    }
    cnv->canvas.cmap_installed = FALSE;
    cnv->canvas.mapped = FALSE;
    cnv->canvas.partly_unobscured = FALSE;
    cmap_list = XListInstalledColormaps (dpy, cnv->core.window, &num_cmaps);
    cmap = cnv->core.colormap;
    for (count = 0; count < num_cmaps; ++count)
    {
	if (cmap == cmap_list[count]) cnv->canvas.cmap_installed = TRUE;
    }
    XFree ( (void *) cmap_list );
    cnv->canvas.cmap_installed_internally = FALSE;
    /*  Create graphics context  */
    gcvalues.function = GXcopy;
    gcvalues.foreground = cnv->canvas.foreground_pixel;
    gcvalues.background = cnv->core.background_pixel;
    gc = XCreateGC (dpy, cnv->core.window,
		    GCForeground | GCBackground | GCFunction,
		    &gcvalues);
    /*  Create the KPixCanvas  */
    switch (cnv->canvas.stereoMode)
    {
      case XkwSTEREO_MODE_XMBUF:
	if ( ( stereopixcanvases[0] =
	      kwin_create_x (dpy, cnv->canvas.left_window, gc,
			     0, 0, cnv->core.width, cnv->core.height) )
	    == NULL )
	{
	    m_abort (function_name, "left pixel canvas");
	}
	if (cnv->canvas.fontName != NULL)
	{
	    if ( ( font = kwin_load_font (stereopixcanvases[0],
					  cnv->canvas.fontName) ) == NULL )
	    {
		m_abort (function_name, "font");
	    }
	    kwin_set_attributes (stereopixcanvases[0],
				 KWIN_ATT_FONT, font,
				 KWIN_ATT_END);
	}
	kwin_set_attributes (stereopixcanvases[0],
			     KWIN_ATT_VISIBLE, FALSE,
			     KWIN_ATT_END);
	cnv->canvas.leftPixCanvas = stereopixcanvases[0];
	if ( ( stereopixcanvases[1] =
	      kwin_create_x (dpy, cnv->canvas.right_window, gc,
			     0, 0, cnv->core.width, cnv->core.height) )
	    == NULL )
	{
	    m_abort (function_name, "right pixel canvas");
	}
	if (cnv->canvas.fontName != NULL)
	{
	    kwin_set_attributes (stereopixcanvases[1],
				 KWIN_ATT_FONT, font,
				 KWIN_ATT_END);
	}
	kwin_set_attributes (stereopixcanvases[1],
			     KWIN_ATT_VISIBLE, FALSE,
			     KWIN_ATT_END);
	cnv->canvas.rightPixCanvas = stereopixcanvases[1];
	XtCallCallbacks (w, XkwNrealiseCallback,(XtPointer) stereopixcanvases);
	break;
      case XkwSTEREO_MODE_XGL:
	if ( !kwin_xgl_create_stereo (dpy, cnv->core.window,
				      0, 0, cnv->core.width, cnv->core.height,
				      &pixcanvas, stereopixcanvases,
				      stereopixcanvases + 1) )
	{
	    (void) fprintf (stderr, "%s: error creating stereo canvas\n",
			    function_name);
	    exit (RV_UNDEF_ERROR);
	}
#ifdef DISABLED
	if (cnv->canvas.fontName != NULL)
	{
	    if ( ( font = kwin_load_font (stereopixcanvases[0],
					  cnv->canvas.fontName) ) == NULL )
	    {
		m_abort (function_name, "font");
	    }
	    kwin_set_attributes (stereopixcanvases[0],
				 KWIN_ATT_FONT, font,
				 KWIN_ATT_END);
	    kwin_set_attributes (stereopixcanvases[1],
				 KWIN_ATT_FONT, font,
				 KWIN_ATT_END);
	}
#endif
	kwin_set_attributes (stereopixcanvases[0],
			     KWIN_ATT_VISIBLE, FALSE,
			     KWIN_ATT_END);
	kwin_set_attributes (stereopixcanvases[1],
			     KWIN_ATT_VISIBLE, FALSE,
			     KWIN_ATT_END);
	cnv->canvas.leftPixCanvas = stereopixcanvases[0];
	cnv->canvas.rightPixCanvas = stereopixcanvases[1];
	XtCallCallbacks (w, XkwNrealiseCallback,(XtPointer) stereopixcanvases);
	break;
      case XkwSTEREO_MODE_OpenGL:
	if ( !kwin_open_gl_create_stereo (dpy, cnv->core.window,
					  0, 0,
					  cnv->core.width, cnv->core.height,
					  stereopixcanvases,
					  stereopixcanvases + 1) )
	{
	    (void) fprintf (stderr, "%s: error creating stereo canvas\n",
			    function_name);
	    exit (RV_UNDEF_ERROR);
	}
	if (cnv->canvas.fontName != NULL)
	{
	    if ( ( font = kwin_load_font (stereopixcanvases[0],
					  cnv->canvas.fontName) ) == NULL )
	    {
		m_abort (function_name, "font");
	    }
	    kwin_set_attributes (stereopixcanvases[0],
				 KWIN_ATT_FONT, font,
				 KWIN_ATT_END);
	    kwin_set_attributes (stereopixcanvases[1],
				 KWIN_ATT_FONT, font,
				 KWIN_ATT_END);
	}
	kwin_set_attributes (stereopixcanvases[0],
			     KWIN_ATT_VISIBLE, FALSE,
			     KWIN_ATT_END);
	kwin_set_attributes (stereopixcanvases[1],
			     KWIN_ATT_VISIBLE, FALSE,
			     KWIN_ATT_END);
	cnv->canvas.leftPixCanvas = stereopixcanvases[0];
	cnv->canvas.rightPixCanvas = stereopixcanvases[1];
	XtCallCallbacks (w, XkwNrealiseCallback,(XtPointer) stereopixcanvases);
	break;
      case XkwSTEREO_MODE_SPLIT:
	if (cnv->canvas.verticalSplit)
	{
	    split_width = cnv->core.width;
	    split_height = (cnv->core.height -cnv->canvas.splitSeparation) / 2;
	    split_xoff = 0;
	    split_yoff = split_height + cnv->canvas.splitSeparation;
	}
	else
	{
	    split_width = (cnv->core.width - cnv->canvas.splitSeparation) / 2;
	    split_height = cnv->core.height;
	    split_xoff = split_width + cnv->canvas.splitSeparation;
	    split_yoff = 0;
	}
	if ( ( stereopixcanvases[0] =
	       kwin_create_x (dpy, cnv->core.window, gc,
			      0, 0,
			      split_width, split_height) )
	    == NULL )
	{
	    m_abort (function_name, "pixel canvas");
	}
	kwin_set_attributes (stereopixcanvases[0],
			     KWIN_ATT_VISIBLE, FALSE,
			     KWIN_ATT_END);
	if ( ( stereopixcanvases[1] =
	       kwin_create_x (dpy, cnv->core.window, gc,
			      split_xoff, split_yoff,
			      split_width, split_height) )
	    == NULL )
	{
	    m_abort (function_name, "pixel canvas");
	}
	kwin_set_attributes (stereopixcanvases[1],
			     KWIN_ATT_VISIBLE, FALSE,
			     KWIN_ATT_END);
	if (cnv->canvas.fontName != NULL)
	{
	    if ( ( font = kwin_load_font (stereopixcanvases[0],
					  cnv->canvas.fontName) ) == NULL )
	    {
		m_abort (function_name, "font");
	    }
	    kwin_set_attributes (stereopixcanvases[0],
				 KWIN_ATT_FONT, font,
				 KWIN_ATT_END);
	    kwin_set_attributes (stereopixcanvases[1],
				 KWIN_ATT_FONT, font,
				 KWIN_ATT_END);
	}
	cnv->canvas.leftPixCanvas = stereopixcanvases[0];
	cnv->canvas.rightPixCanvas = stereopixcanvases[1];
	XtCallCallbacks (w, XkwNrealiseCallback,(XtPointer) stereopixcanvases);
	break;
      case XkwSTEREO_MODE_MONO:
	if ( ( pixcanvas =
	      kwin_create_x (dpy, cnv->core.window, gc,
			     0, 0, cnv->core.width, cnv->core.height) )
	    == NULL )
	{
	    m_abort (function_name, "pixel canvas");
	}
	if (cnv->canvas.fontName != NULL)
	{
	    if ( ( font = kwin_load_font (pixcanvas,
					  cnv->canvas.fontName) ) == NULL )
	    {
		m_abort (function_name, "font");
	    }
	    kwin_set_attributes (pixcanvas,
				 KWIN_ATT_FONT, font,
				 KWIN_ATT_END);
	}
	kwin_set_attributes (pixcanvas,
			     KWIN_ATT_VISIBLE, FALSE,
			     KWIN_ATT_END);
	cnv->canvas.monoPixCanvas = pixcanvas;
	XtCallCallbacks (w, XkwNrealiseCallback, (XtPointer) pixcanvas);
	break;
    }
}   /*  End Function Realise  */


/* ARGSUSED */
static Boolean Canvas__SetValues (Widget Current, Widget Request, Widget New)
{
    CanvasWidget old = (CanvasWidget) Current;
    CanvasWidget new = (CanvasWidget) New;

    new->canvas.monoPixCanvas = old->canvas.monoPixCanvas;
    new->canvas.leftPixCanvas = old->canvas.leftPixCanvas;
    new->canvas.rightPixCanvas = old->canvas.rightPixCanvas;
    new->canvas.visual = old->canvas.visual;
    new->canvas.stereoMode = old->canvas.stereoMode;

    return False;   
}   /*  End Function SetValues  */

static void Canvas__Redisplay (CanvasWidget cnv, XEvent *event, Region region)
{
    int split_width, split_height, split_xoff, split_yoff;
    Widget w = (Widget) cnv;

    if ( !XtIsRealized (w) )
    {
	(void) fprintf (stderr, "Expose on non-realised canvas widget!\n");
	return;
    }
    if (event->xexpose.window != cnv->core.window)
    {
	(void) fprintf (stderr, "Expose event not on canvas widget window!\n");
	return;
    }
    if (!cnv->canvas.cmap_installed)
    {
	(void) fprintf (stderr, "installing cmap: 0x%lx\n",
			cnv->core.colormap);
	XInstallColormap (XtDisplay (cnv), cnv->core.colormap);
	cnv->canvas.cmap_installed_internally = TRUE;
    }
    if (!cnv->core.visible)
    {
	(void) fprintf (stderr, "Expose on: %p which is not visible\n", cnv);
    }
    if (!cnv->canvas.partly_unobscured)
    {
	(void) fprintf (stderr,
			"Expose on: %p which is fully obscured: unobscuring\n",
			cnv);
	cnv->canvas.partly_unobscured = TRUE;
	set_visibility (cnv, cnv->canvas.mapped);
    }
    if (cnv->canvas.monoPixCanvas != NULL)
    {
	if ( !kwin_resize (cnv->canvas.monoPixCanvas, FALSE,
			   0, 0, cnv->core.width, cnv->core.height) )
	{
	    (void) fprintf(stderr,
			   "Error redisplaying mono KPixCanvas of canvas widget\n");
	    return;
	}
    }
    if (cnv->canvas.stereoMode == XkwSTEREO_MODE_SPLIT)
    {
	if (cnv->canvas.verticalSplit)
	{
	    split_width = cnv->core.width;
	    split_height = (cnv->core.height -cnv->canvas.splitSeparation) / 2;
	    split_xoff = 0;
	    split_yoff = split_height + cnv->canvas.splitSeparation;
	}
	else
	{
	    split_width = (cnv->core.width - cnv->canvas.splitSeparation) / 2;
	    split_height = cnv->core.height;
	    split_xoff = split_width + cnv->canvas.splitSeparation;
	    split_yoff = 0;
	}
	if ( !kwin_resize (cnv->canvas.leftPixCanvas, FALSE,
			   0, 0, split_width, split_height) )
	{
	    (void) fprintf(stderr,
			   "Error redisplaying left KPixCanvas of canvas widget\n");
	    return;
	}
	if ( !kwin_resize (cnv->canvas.rightPixCanvas, FALSE,
			   split_xoff, split_yoff, split_width, split_height) )
	{
	    (void) fprintf(stderr,
			   "Error redisplaying right KPixCanvas of canvas widget\n");
	    return;
	}
	return;
    }
    if (cnv->canvas.leftPixCanvas != NULL)
    {
	if ( !kwin_resize (cnv->canvas.leftPixCanvas, FALSE,
			   0, 0, cnv->core.width, cnv->core.height) )
	{
	    (void) fprintf(stderr,
			   "Error redisplaying left KPixCanvas of canvas widget\n");
	    return;
	}
    }
    if (cnv->canvas.rightPixCanvas != NULL)
    {
	if ( !kwin_resize (cnv->canvas.rightPixCanvas, FALSE,
			   0, 0, cnv->core.width, cnv->core.height) )
	{
	    (void) fprintf(stderr,
			   "Error redisplaying right KPixCanvas of canvas widget\n");
	    return;
	}
    }
}   /*  End Function Redisplay  */

static void CanvasColourmapChange (CanvasWidget cnv, XEvent *event)
{
    static char function_name[] = "Canvas::CanvasColourmapChange";

    if (event->xcolormap.window != cnv->core.window)
    {
	(void) fprintf (stderr,
			"Colourmap event not on canvas widget window!\n");
	return;
    }
    switch (event->xcolormap.state)
    {
      case ColormapInstalled:
	cnv->canvas.cmap_installed = TRUE;
	break;
      case ColormapUninstalled:
	cnv->canvas.cmap_installed = FALSE;
	break;
      default:
	(void) fprintf (stderr, "Illegal state: %d\n", event->xcolormap.state);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function CanvasColourmapChange   */

static void CanvasEnterNotify (CanvasWidget cnv, XEvent *event)
{
    if (event->xcrossing.window != cnv->core.window)
    {
	(void) fprintf (stderr,
			"EnterNotify event not on canvas widget window!\n");
	return;
    }
    if (!cnv->canvas.cmap_installed)
    {
	XInstallColormap (XtDisplay (cnv), cnv->core.colormap);
	cnv->canvas.cmap_installed_internally = TRUE;
    }
}   /*  End Function CanvasEnterNotify   */

static void CanvasMotionNotify (CanvasWidget cnv, XEvent *event)
{
    unsigned int k_event;

    if (event->xmotion.window != cnv->core.window)
    {
	(void) fprintf (stderr,
			"MotionNotify event not on canvas widget window!\n");
	return;
    }
    if (!cnv->canvas.cmap_installed)
    {
	XInstallColormap (XtDisplay (cnv), cnv->core.colormap);
	cnv->canvas.cmap_installed_internally = TRUE;
    }
    if (event->xmotion.state & Button1Mask)
    {
	k_event = K_CANVAS_EVENT_LEFT_MOUSE_DRAG;
    }
    else if (event->xmotion.state & Button2Mask)
    {
	k_event = K_CANVAS_EVENT_MIDDLE_MOUSE_DRAG;
    }
    else if (event->xmotion.state & Button3Mask)
    {
	k_event = K_CANVAS_EVENT_RIGHT_MOUSE_DRAG;
    }
    else
    {
	k_event = K_CANVAS_EVENT_POINTER_MOVE;
    }
    inject_event (cnv, event->xmotion.x, event->xmotion.y, k_event, NULL);
}   /*  End Function CanvasMotionNotify   */

static void CanvasButtonNotify (CanvasWidget cnv, XEvent *event)
{
    unsigned int k_event;

    if (event->xbutton.window != cnv->core.window)
    {
	(void) fprintf (stderr,
			"ButtonNotify event not on canvas widget window!\n");
	return;
    }
    if (!cnv->canvas.cmap_installed)
    {
	XInstallColormap (XtDisplay (cnv), cnv->core.colormap);
	cnv->canvas.cmap_installed_internally = TRUE;
    }
    switch (event->xbutton.button)
    {
      case Button1:
	if (event->xbutton.state & Button1Mask)
	{
	    k_event = K_CANVAS_EVENT_LEFT_MOUSE_RELEASE;
	}
	else
	{
	    k_event = K_CANVAS_EVENT_LEFT_MOUSE_CLICK;
	}
	break;
      case Button2:
	if (event->xbutton.state & Button2Mask)
	{
	    k_event = K_CANVAS_EVENT_MIDDLE_MOUSE_RELEASE;
	}
	else
	{
	    k_event = K_CANVAS_EVENT_MIDDLE_MOUSE_CLICK;
	}
	break;
      case Button3:
	if (event->xbutton.state & Button3Mask)
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
    inject_event (cnv, event->xbutton.x, event->xbutton.y, k_event, NULL);
}   /*  End Function CanvasButtonNotify   */

static void CanvasMapNotify (CanvasWidget cnv, XEvent *event)
{
    if (event->xmap.window != cnv->core.window)
    {
	(void) fprintf (stderr,
			"MapNotify event not on canvas widget window!\n");
	return;
    }
    cnv->canvas.mapped = TRUE;
    set_visibility (cnv, cnv->canvas.partly_unobscured);
}   /*  End Function CanvasMapNotify   */

static void CanvasUnmapNotify (CanvasWidget cnv, XEvent *event)
{
    if (event->xunmap.window != cnv->core.window)
    {
	(void) fprintf (stderr,
			"UnmapNotify event not on canvas widget window!\n");
	return;
    }
    cnv->canvas.mapped = FALSE;
    set_visibility (cnv, FALSE);
}   /*  End Function CanvasUnmapNotify   */

static void CanvasVisibilityNotify (CanvasWidget cnv, XEvent *event)
{
    if (event->xvisibility.window != cnv->core.window)
    {
	(void) fprintf (stderr,
			"VisibilityNotify event not on canvas widget window!\n");
	return;
    }
    switch (event->xvisibility.state)
    {
      case VisibilityFullyObscured:
	/*  This is only received when the window is obscured behind other
	    windows, not when it is iconified. There seem to be no events
	    dispatched when the window is iconified. A pity.  */
	cnv->canvas.partly_unobscured = FALSE;
	set_visibility (cnv, FALSE);
	break;
      case VisibilityPartiallyObscured:
      case VisibilityUnobscured:
	cnv->canvas.partly_unobscured = TRUE;
	set_visibility (cnv, cnv->canvas.mapped);
	break;
      default:
	(void) fprintf (stderr, "Unknown visibility event state: %d\n",
			event->xvisibility.state);
	break;
    }
}   /*  End Function CanvasVisibilityNotify   */

static void CanvasKeyPress (CanvasWidget cnv, XEvent *event)
{
    int len;    
    char buffer[STRING_LENGTH + 1];

    if (event->xkey.window != cnv->core.window)
    {
	(void) fprintf (stderr,
			"KeyPress event not on canvas widget window!\n");
	return;
    }
    if (event->xkey.state & Button1Mask) return;
    if (event->xkey.state & Button2Mask) return;
    if (event->xkey.state & Button3Mask) return;
    if (event->xkey.state & Button4Mask) return;
    if (event->xkey.state & Button5Mask) return;
    if (event->xkey.state & ControlMask) return;
    if (event->xkey.state & Mod1Mask) return;
    if (event->xkey.state & Mod2Mask) return;
    if (event->xkey.state & Mod3Mask) return;
    if (event->xkey.state & Mod4Mask) return;
    if (event->xkey.state & Mod5Mask) return;
    len = XLookupString (&event->xkey, buffer, STRING_LENGTH, NULL, NULL);
    if (len < 1) return;
    buffer[len] = '\0';
    if ( (event->xkey.state & ShiftMask) || (event->xkey.state & LockMask) )
    {
	st_upr (buffer);
    }
    inject_event (cnv, event->xkey.x, event->xkey.y,
		  K_CANVAS_EVENT_PLAIN_KEY_PRESS, buffer);
}   /*  End Function CanvasKeyPress  */

static void create_stereo_window (Widget widget, unsigned int window_class,
				  Visual *visual, XtValueMask value_mask,
				  XSetWindowAttributes *attributes,
				  Multibuffer *left, Multibuffer *right)
/*  [PURPOSE] This routine will create a stereo window.
*/
{
    Window win, parentwin;

    parentwin = (widget->core.parent ? widget->core.parent->core.window :
		 widget->core.screen->root);
    if (widget->core.window == None)
    {
	if (widget->core.width == 0 || widget->core.height == 0)
	{
	    (void) fprintf (stderr, "Widget %s has zero width and/or height\n",
			    (char *) &widget->core.name);
	}
	win = XmbufCreateStereoWindow (XtDisplay (widget), parentwin,
				       (int) widget->core.x,
				       (int) widget->core.y,
				       (unsigned int) widget->core.width,
				       (unsigned int) widget->core.height,
				       (unsigned) widget->core.border_width,
				       (int) widget->core.depth,
				       window_class, visual, value_mask,
				       attributes, left, right);
	if (win == (Window) None)
	{
	    (void) fprintf (stderr, "Error creating stereo window\n");
	    exit (RV_UNDEF_ERROR);
	}
	widget->core.window = win;
    }
}   /*  End Function create_stereo_window  */

static void set_visibility (CanvasWidget cnv, flag visible)
/*  [PURPOSE] This routine will set the visibility state of any pixel canvases.
    <cnv> The Canvas widget.
    <visible> If TRUE, the pixel canvases are set to be visible, else they are
    invisible.
    [RETURNS] Nothing.
*/
{
    if (cnv->canvas.monoPixCanvas != NULL)
    {
	kwin_set_attributes (cnv->canvas.monoPixCanvas,
			     KWIN_ATT_VISIBLE, visible,
			     KWIN_ATT_END);
    }
    if (cnv->canvas.leftPixCanvas != NULL)
    {
	kwin_set_attributes (cnv->canvas.leftPixCanvas,
			     KWIN_ATT_VISIBLE, visible,
			     KWIN_ATT_END);
    }
    if (cnv->canvas.rightPixCanvas != NULL)
    {
	kwin_set_attributes (cnv->canvas.rightPixCanvas,
			     KWIN_ATT_VISIBLE, visible,
			     KWIN_ATT_END);
    }
}   /*  End Function set_visibility  */

static void inject_event (CanvasWidget cnv, int x, int y, unsigned int k_event,
			  CONST char *string)
/*  [PURPOSE] This routine will inject an event into an appropriate pixel
    canvas.
    <cnv> The Canvas widget.
    <k_event> The Karma event code.
    <string> The keystring. This is NULL if a key was not pressed.
    [RETURNS] Nothing.
*/
{
    flag consumed = FALSE;

    if (cnv->canvas.monoPixCanvas != NULL)
    {
	consumed = kwin_process_position_event (cnv->canvas.monoPixCanvas,
						x, y,
						cnv->canvas.clip, k_event,
						(void *) string);
    }
    else if (cnv->canvas.leftPixCanvas != NULL)
    {
	consumed = kwin_process_position_event (cnv->canvas.leftPixCanvas,
						x, y,
						cnv->canvas.clip, k_event,
						(void *) string);
    }
    if (consumed || cnv->canvas.silence_unconsumed_events) return;
    (void) fprintf (stderr, "Karma event: %u not consumed\n", k_event);
}   /*  End Function inject_event  */

static XVisualInfo *get_visinfo_for_visual (Display *dpy, Visual *visual)
/*  This routine will get the visual information structure for a visual.
    The X display must be given by  dpy  .
    The visual must be given by  visual  .
    The routine returns a pointer to an XVisualInfo structure on succes, else
    it returns NULL. The XVisualInfo structure must be freed by XFree()
*/
{
    int num_vinfos;
    XVisualInfo vinfo_template;
    XVisualInfo *vinfos;
    static char function_name[] = "Canvas::get_visinfo_for_visual";

    vinfo_template.visualid = XVisualIDFromVisual (visual);
    vinfos = XGetVisualInfo (dpy, VisualIDMask, &vinfo_template, &num_vinfos);
    if (num_vinfos < 1)
    {
	(void) fprintf (stderr, "Error getting visual info for visual: %p\n",
			visual);
	a_prog_bug (function_name);
    }
    if (num_vinfos > 1)
    {
	(void) fprintf (stderr,
			"WARNING: number of visuals for visual: %p is: %d\n",
			visual, num_vinfos);
    }
    return (vinfos);
}   /*  End Function get_visinfo_for_visual  */

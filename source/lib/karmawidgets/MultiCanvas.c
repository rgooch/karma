/*LINTLIBRARY*/
/*  MultiCanvas.c

    This code provides a multi canvas widget for Xt.

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


    Written by      Richard Gooch   17-DEC-1994

    Updated by      Richard Gooch   17-DEC-1994

    Updated by      Richard Gooch   4-JAN-1995: Cosmetic changes.

    Updated by      Richard Gooch   30-JAN-1995: Added stereo support.

    Updated by      Richard Gooch   16-APR-1995: Added function name to
  colourmap creation message.

    Updated by      Richard Gooch   26-JUL-1995: Fixed '=' bugs in if() tests.

    Updated by      Richard Gooch   23-SEP-1995: Made use of XGL.

    Updated by      Richard Gooch   29-OCT-1995: Made use of OpenGL.

    Updated by      Richard Gooch   23-DEC-1995: Supported split (fake) stereo
  mode through splitStereo resource.

    Last updated by Richard Gooch   28-DEC-1995: Added verbose resource.


*/

#include <stdio.h>
#define X11
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/Form.h>
#include <X11/extensions/multibuf.h>
#include <karma.h>
#include <Xkw/MultiCanvasP.h>

#define offset(field) XtOffsetOf(MultiCanvasRec, multiCanvas.field)

static XtResource resources[] =
{
    {XkwNcanvasTypes, XkwCCanvasTypes, XtRInt, sizeof (int),
     offset (requestList), XtRImmediate, 0},
    {XkwNsplitStereo, XkwCSplitStereo, XtRBool, sizeof (Bool),
     offset (splitStereo), XtRImmediate, False},
    {XkwNverbose, XkwCVerbose, XtRBool, sizeof (Bool),
     offset (verbose), XtRImmediate, False},
#undef offset
};

STATIC_FUNCTION (void Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (Boolean SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void get_visuals,
		 (Screen *screen, Visual **pseudocolour, Visual **truecolour,
		  Visual **directcolour) );
STATIC_FUNCTION (XVisualInfo *get_visinfo_for_visual,
		 (Display *dpy, Visual *visual) );
STATIC_FUNCTION (flag stereo_supported_for_visual,
		 (Visual *visual, int num_info, XmbufBufferInfo *info) );

MultiCanvasClassRec multiCanvasClassRec =
{
    {
	/* core fields  */
	(WidgetClass) &formClassRec,    /*  superclass             */
	"MultiCanvas",                  /*  class_name             */
	sizeof (MultiCanvasRec),        /*  widget_size            */
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
	sizeof (MultiCanvasConstraintsRec) /* constraint_size    */,
	NULL                            /* initialise         */,
	NULL                            /* destroy            */,
	NULL                            /* set_values         */,
	NULL                            /* extension          */ 
    },
    { /* Form */
	XtInheritLayout                 /* layout             */
    },
    {
	/*  multiCanvas fields */
	0                               /*  ignore                 */
    }
};

WidgetClass multiCanvasWidgetClass = (WidgetClass) &multiCanvasClassRec;

static void Initialise (Widget Request, Widget New)
{
    flag xmbuf_stereo = FALSE;
    flag xgl_stereo = FALSE;
    flag open_gl_stereo = FALSE;
    flag split_stereo = FALSE;
    flag verbose;
    int dummy, num_mono_mbuf, num_stereo_mbuf, stereo_mode;
    MultiCanvasWidget request = (MultiCanvasWidget) Request;
    MultiCanvasWidget new = (MultiCanvasWidget) New;
    Widget w;
    Colormap pseudocolour_cmap, directcolour_cmap, truecolour_cmap;
    Display *dpy;
    XVisualInfo *vinfo;
    Visual *pseudocolour_visual, *directcolour_visual, *truecolour_visual;
    Visual *root_visual;
    Screen *screen;
    XmbufBufferInfo *mono_mbuf_info, *stereo_mbuf_info;
    static char function_name[] = "MultiCanvas::Initialise";

    verbose = new->multiCanvas.verbose;
    new->form.default_spacing = 0;
    dpy = XtDisplay (New);
    screen = XtScreen (New);
    w = XtParent (New);
    /*  Get visual information  */
    root_visual = XDefaultVisualOfScreen (screen);
    vinfo = get_visinfo_for_visual (dpy, root_visual);
    get_visuals (screen, &pseudocolour_visual,
		 &truecolour_visual, &directcolour_visual);
    if ( (vinfo->depth == 8) && (vinfo->class == PseudoColor) &&
	(vinfo->colormap_size == 256) )
    {
	/*  Use the default visual  */
	pseudocolour_visual = root_visual;
    }
    if ( (vinfo->depth == 24) && (vinfo->class == DirectColor) &&
	(vinfo->colormap_size == 256) )
    {
	/*  Use the default visual  */
	directcolour_visual = root_visual;
    }
    if ( (vinfo->depth == 24) && (vinfo->class == TrueColor) &&
	(vinfo->colormap_size == 256) )
    {
	/*  Use the default visual  */
	truecolour_visual = root_visual;
    }
    XFree ( (char *) vinfo );
    vinfo = NULL;
    /*  Determine if stereo was requested  */
    if (XkwCanvasTypeStereo & new->multiCanvas.requestList)
    {
	if ( XmbufQueryExtension (dpy, &dummy, &dummy) )
	{
	    xmbuf_stereo = TRUE;
	}
    }
    if (xmbuf_stereo)
    {
	if (XmbufGetScreenInfo (dpy, New->core.screen->root,
				&num_mono_mbuf, &mono_mbuf_info,
				&num_stereo_mbuf, &stereo_mbuf_info) == 0)
	{
	    xmbuf_stereo = FALSE;
	    num_stereo_mbuf = 0;
	}
	else
	{
	    if (num_mono_mbuf > 0) XFree ( (char *) mono_mbuf_info );
	    if (num_stereo_mbuf < 1)
	    {
		xmbuf_stereo = FALSE;
	    }
	}
    }
    else num_stereo_mbuf = 0;
    if ( (XkwCanvasTypeStereo & new->multiCanvas.requestList) &&
	!xmbuf_stereo )
    {
	/*  Want stereo but MultiBuffering no good  */
	if (kwin_xgl_test_stereo (dpy, New->core.screen->root) ==
	    KWIN_XGL_STEREO_AVAILABLE)
	{
	    xgl_stereo = TRUE;
	}
	else if (kwin_open_gl_test_stereo (dpy,
					   XScreenNumberOfScreen
					   (New->core.screen),
					   0, 0)
		 == KWIN_OpenGL_STEREO_AVAILABLE)
	{
	    open_gl_stereo = TRUE;
	}
	else if (new->multiCanvas.splitStereo) split_stereo = TRUE;
    }
    if ( (XkwCanvasTypePseudoColour & new->multiCanvas.requestList) &&
	(pseudocolour_visual != NULL) )
    {
	if (pseudocolour_visual == root_visual)
	{
	    /*  Get root window colourmap  */
	    pseudocolour_cmap = XDefaultColormapOfScreen (screen);
	}
	else
	{
	    /*  Inheriting the colourmap from the root won't work: make one  */
	    if ( ( pseudocolour_cmap =
		  XCreateColormap (dpy, XRootWindowOfScreen (screen),
				   pseudocolour_visual, AllocNone) ) ==
		(Colormap) NULL )
	    {
		(void) fprintf (stderr, "Could not create colourmap\n");
		exit (1);
	    }
	    XSync (dpy, False);
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: created colourmap: 0x%x for PseudoColour visual\n",
				function_name, pseudocolour_cmap);
	    }
	}
	if (verbose)
	{
	    (void) fprintf (stderr,
			    "%s: creating mono PseudoColour canvas...\n",
			    function_name);
	}
	w = XtVaCreateManagedWidget ("pseudoColourCanvas", canvasWidgetClass,
				     New,
				     XtNmappedWhenManaged, False,
				     XtNwidth, new->core.width,
				     XtNheight, new->core.height,
				     XtNvisual, pseudocolour_visual,
				     XtNcolormap, pseudocolour_cmap,
				     XtNborderWidth, 0,
				     NULL);
	if ( stereo_supported_for_visual (pseudocolour_visual, num_stereo_mbuf,
					  stereo_mbuf_info) )
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating Xmbuf stereo PseudoColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_XMBUF;
	}
	else if (xgl_stereo)
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating XGL stereo PseudoColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_XGL;
	}
	else if ( open_gl_stereo &&
		 (kwin_open_gl_test_stereo (dpy, XScreenNumberOfScreen
					    (New->core.screen),
					    KWIN_VISUAL_PSEUDOCOLOUR, 8)
		  == KWIN_OpenGL_STEREO_AVAILABLE) )
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating OpenGL stereo PseudoColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_OpenGL;
	}
	else if (split_stereo)
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating split stereo PseudoColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_SPLIT;
	}
	else stereo_mode = XkwSTEREO_MODE_MONO;
	if (stereo_mode != XkwSTEREO_MODE_MONO)
	{
	    w = XtVaCreateManagedWidget ("pseudoColourStereoCanvas",
					 canvasWidgetClass, New,
					 XkwNstereoMode, stereo_mode,
					 XtNmappedWhenManaged, False,
					 XtNwidth, new->core.width,
					 XtNheight, new->core.height,
					 XtNvisual, pseudocolour_visual,
					 XtNcolormap, pseudocolour_cmap,
					 XtNborderWidth, 0,
					 NULL);
	}
    }
    if ( (XkwCanvasTypeDirectColour & new->multiCanvas.requestList) &&
	(directcolour_visual != NULL) )
    {
	if (directcolour_visual == root_visual)
	{
	    /*  Get root window colourmap  */
	    directcolour_cmap = XDefaultColormapOfScreen (screen);
	}
	else
	{
	    /*  Inheriting the colourmap from the root won't work: make one  */
	    if ( ( directcolour_cmap =
		  XCreateColormap (dpy, XRootWindowOfScreen (screen),
				   directcolour_visual, AllocNone) ) ==
		(Colormap) NULL )
	    {
		(void) fprintf (stderr, "Could not create colourmap\n");
		exit (1);
	    }
	    XSync (dpy, False);
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: created colourmap: 0x%x for DirectColour visual\n",
				function_name, directcolour_cmap);
	    }
	}
	if (verbose)
	{
	    (void) fprintf (stderr,
			    "%s: creating mono DirectColour canvas...\n",
			    function_name);
	}
	w = XtVaCreateManagedWidget ("directColourCanvas", canvasWidgetClass,
				     New,
				     XtNmappedWhenManaged, False,
				     XtNwidth, new->core.width,
				     XtNheight, new->core.height,
				     XtNvisual, directcolour_visual,
				     XtNcolormap, directcolour_cmap,
				     XtNborderWidth, 0,
				     NULL);
	if ( stereo_supported_for_visual (directcolour_visual, num_stereo_mbuf,
					  stereo_mbuf_info) )
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating Xmbuf stereo DirectColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_XMBUF;
	}
	else if (xgl_stereo)
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating XGL stereo DirectColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_XGL;
	}
	else if ( open_gl_stereo &&
		 (kwin_open_gl_test_stereo (dpy, XScreenNumberOfScreen
					    (New->core.screen),
					    KWIN_VISUAL_DIRECTCOLOUR, 24)
		  == KWIN_OpenGL_STEREO_AVAILABLE) )
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating OpenGL stereo DirectColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_OpenGL;
	}
	else if (split_stereo)
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating split stereo DirectColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_SPLIT;
	}
	else stereo_mode = XkwSTEREO_MODE_MONO;
	if (stereo_mode != XkwSTEREO_MODE_MONO)
	{
	    w = XtVaCreateManagedWidget ("directColourStereoCanvas",
					 canvasWidgetClass, New,
					 XkwNstereoMode, stereo_mode,
					 XtNmappedWhenManaged, False,
					 XtNwidth, new->core.width,
					 XtNheight, new->core.height,
					 XtNvisual, directcolour_visual,
					 XtNcolormap, directcolour_cmap,
					 XtNborderWidth, 0,
					 NULL);
	}
    }
    if ( (XkwCanvasTypeTrueColour & new->multiCanvas.requestList) &&
	(truecolour_visual != NULL) )
    {
	if (truecolour_visual == root_visual)
	{
	    /*  Get root window colourmap  */
	    truecolour_cmap = XDefaultColormapOfScreen (screen);
	}
	else
	{
	    /*  Inheriting the colourmap from the root won't work: make one  */
	    if ( ( truecolour_cmap =
		  XCreateColormap (dpy, XRootWindowOfScreen (screen),
				   truecolour_visual, AllocNone) ) ==
		(Colormap) NULL )
	    {
		(void) fprintf (stderr, "Could not create colourmap\n");
		exit (1);
	    }
	    XSync (dpy, False);
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: created colourmap: 0x%x for TrueColour visual\n",
				function_name, truecolour_cmap);
	    }
	}
	if (verbose)
	{
	    (void) fprintf (stderr, "%s: creating mono TrueColour canvas...\n",
			    function_name);
	}
	w = XtVaCreateManagedWidget ("trueColourCanvas", canvasWidgetClass,New,
				     XtNmappedWhenManaged, False,
				     XtNwidth, new->core.width,
				     XtNheight, new->core.height,
				     XtNvisual, truecolour_visual,
				     XtNcolormap, truecolour_cmap,
				     XtNborderWidth, 0,
				     NULL);
	if ( stereo_supported_for_visual (truecolour_visual, num_stereo_mbuf,
					  stereo_mbuf_info) )
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating Xmbuf stereo TrueColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_XMBUF;
	}
	else if (xgl_stereo)
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating XGL stereo TrueColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_XGL;
	}
	else if ( open_gl_stereo &&
		 (kwin_open_gl_test_stereo (dpy, XScreenNumberOfScreen
					    (New->core.screen),
					    KWIN_VISUAL_TRUECOLOUR, 24)
		  == KWIN_OpenGL_STEREO_AVAILABLE) )
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating OpenGL stereo TrueColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_OpenGL;
	}
	else if (split_stereo)
	{
	    if (verbose)
	    {
		(void) fprintf (stderr,
				"%s: creating split stereo TrueColour canvas...\n",
				function_name);
	    }
	    stereo_mode = XkwSTEREO_MODE_SPLIT;
	}
	else stereo_mode = XkwSTEREO_MODE_MONO;
	if (stereo_mode != XkwSTEREO_MODE_MONO)
	{
	    w = XtVaCreateManagedWidget ("trueColourStereoCanvas",
					 canvasWidgetClass, New,
					 XkwNstereoMode, stereo_mode,
					 XtNmappedWhenManaged, False,
					 XtNwidth, new->core.width,
					 XtNheight, new->core.height,
					 XtNvisual, truecolour_visual,
					 XtNcolormap, truecolour_cmap,
					 XtNborderWidth, 0,
					 NULL);
	}
    }
    if (num_stereo_mbuf > 0) XFree ( (char *) stereo_mbuf_info );
}   /*  End Function Initialise  */

static Boolean SetValues (Widget Current, Widget Request, Widget New)
{
    MultiCanvasWidget current = (MultiCanvasWidget) Current;
    MultiCanvasWidget request = (MultiCanvasWidget) Request;
    MultiCanvasWidget new = (MultiCanvasWidget) New;

    return True;
}   /*  End Function SetValues  */

static void get_visuals (screen, pseudocolour, truecolour, directcolour)
/*  This routine will attempt to get supported visuals available on a screen.
    The X Window screen must be given by  screen  .
    A PseudoColour visual will be written to the storage pointed to by
    pseudocolour  .If this is NULL, nothing is written here. If no
    PseudoColour visual is supported, NULL is written here
    A TrueColour visual will be written to the storage pointed to by
    truecolour  .If this is NULL, nothing is written here. If no TrueColour
    visual is supported, NULL is written here.
    A DirectColour visual will be written to the storage pointed to by
    directcolour  .If this is NULL, nothing is written here. If no DirectColour
    visual is supported, NULL is written here
    The routine returns nothing.
*/
Screen *screen;
Visual **pseudocolour;
Visual **truecolour;
Visual **directcolour;
{
    int num_vinfos;
    XVisualInfo vinfo_template, *vinfos;

    vinfo_template.screen = XScreenNumberOfScreen (screen);
    vinfo_template.colormap_size = 256;
    if (pseudocolour != NULL)
    {
	/*  Get PseudoColour visual  */
	vinfo_template.depth = 8;
	vinfo_template.class = PseudoColor;
	if ( ( vinfos =
	      XGetVisualInfo (XDisplayOfScreen (screen),
			      VisualScreenMask | VisualDepthMask |
			      VisualClassMask | VisualColormapSizeMask,
			      &vinfo_template, &num_vinfos) ) == NULL )
	{
	    *pseudocolour = NULL;
	}
	else
	{
	    *pseudocolour = vinfos[0].visual;
	}
	XFree ( (char *) vinfos );
    }
    if (truecolour != NULL)
    {
	/*  Get TrueColour visual  */
	vinfo_template.depth = 24;
	vinfo_template.class = TrueColor;
	if ( ( vinfos =
	      XGetVisualInfo (XDisplayOfScreen (screen),
			      VisualScreenMask | VisualDepthMask |
			      VisualClassMask | VisualColormapSizeMask,
			      &vinfo_template, &num_vinfos) ) == NULL )
	{
	    *truecolour = NULL;
	}
	else
	{
	    *truecolour = vinfos[0].visual;
	}
	XFree (vinfos);
    }
    if (directcolour != NULL)
    {
	/*  Get DirectColour visual  */
	vinfo_template.depth = 24;
	vinfo_template.class = DirectColor;
	if ( ( vinfos =
	      XGetVisualInfo (XDisplayOfScreen (screen),
			      VisualScreenMask | VisualDepthMask |
			      VisualClassMask | VisualColormapSizeMask,
			      &vinfo_template, &num_vinfos) ) == NULL )
	{
	    *directcolour = NULL;
	}
	else
	{
	    *directcolour = vinfos[0].visual;
	}
	XFree ( (char *) vinfos );
    }
}   /*  End Function get_visuals  */

static XVisualInfo *get_visinfo_for_visual (dpy, visual)
/*  This routine will get the visual information structure for a visual.
    The X display must be given by  dpy  .
    The visual must be given by  visual  .
    The routine returns a pointer to an XVisualInfo structure on succes, else
    it returns NULL. The XVisualInfo structure must be freed by XFree()
*/
Display *dpy;
Visual *visual;
{
    int num_vinfos;
    XVisualInfo vinfo_template;
    XVisualInfo *vinfos;
    static char function_name[] = "get_visinfo_for_visual";

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

static flag stereo_supported_for_visual (Visual *visual, int num_info,
					 XmbufBufferInfo *info)
/*  [PURPOSE] This function will determine if stereo is supported for a
    particular VisualID.
    <visual> The VisualID.
    <num_info> The number of stereo multibuffer info structures.
    <info> The stereo multibuffer info structures.
    [RETURNS] TRUE if stereo is supported, else FALSE.
*/
{
    int count;
    VisualID vid;

    vid = XVisualIDFromVisual (visual);
    for (count = 0; count < num_info; ++count)
    {
	if (info[count].visualid == vid) return (TRUE);
    }
    return (FALSE);
}   /*  End Function stereo_supported_for_visual  */

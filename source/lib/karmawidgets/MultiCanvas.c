/*LINTLIBRARY*/
/*  MultiCanvas.c

    This code provides a multi canvas widget for Xt.

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

    Updated by      Richard Gooch   28-DEC-1995: Added verbose resource.

    Updated by      Richard Gooch   9-MAY-1996: Took account of changes to
  OpenGL interface and revamped canvas creation.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   15-SEP-1996: Made use of new <xv_*>
  package.

    Last updated by Richard Gooch   1-DEC-1996: Inherit colourmap from parent
  rather than root window in <create_canvases_for_visual>.


*/

#include <stdio.h>
#include <ctype.h>
#define X11
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/Form.h>
#include <X11/extensions/multibuf.h>
#include <karma.h>
#include <karma_kwin.h>
#include <karma_xv.h>
#include <karma_a.h>
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

STATIC_FUNCTION (void MultiCanvas__Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (Boolean MultiCanvas__SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (flag stereo_supported_for_vinfo,
		 (XVisualInfo *vinfo, int num_info, XmbufBufferInfo *info) );
STATIC_FUNCTION (void create_canvases_for_visual,
		 (MultiCanvasWidget parent, XVisualInfo *vinfo,
		  XVisualInfo *root_vinfo,
		  int num_stereo_mbuf, XmbufBufferInfo *stereo_mbuf_info,
		  flag have_open_gl, flag xgl_stereo, flag split_stereo) );

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
	(XtInitProc) MultiCanvas__Initialise,  /*  initialise             */
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
	(XtSetValuesFunc) MultiCanvas__SetValues,/*  set_values    */
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

static void MultiCanvas__Initialise (Widget Request, Widget New)
{
    flag xmbuf_stereo = FALSE;
    flag xgl_stereo = FALSE;
    flag have_open_gl = FALSE;
    flag split_stereo = FALSE;
    flag verbose;
    int dummy, num_mono_mbuf, num_stereo_mbuf;
    /*MultiCanvasWidget request = (MultiCanvasWidget) Request;*/
    MultiCanvasWidget new = (MultiCanvasWidget) New;
    Widget w;
    Window root_window;
    Display *dpy;
    XVisualInfo *root_vinfo, *pc_vinfo, *dc_vinfo, *tc_vinfo; /*  Must free  */
    XVisualInfo *pseudocolour_vinfo, *directcolour_vinfo, *truecolour_vinfo;
    Visual *root_visual;
    Screen *screen;
    XmbufBufferInfo *mono_mbuf_info, *stereo_mbuf_info;
    /*static char function_name[] = "MultiCanvas::Initialise";*/

    verbose = new->multiCanvas.verbose;
    new->form.default_spacing = 0;
    dpy = XtDisplay (New);
    screen = XtScreen (New);
    root_window = RootWindowOfScreen (screen);
    w = XtParent (New);
    /*  Get visual information  */
    root_visual = XDefaultVisualOfScreen (screen);
    root_vinfo = xv_get_visinfo_for_visual (dpy, root_visual);
    xv_get_vinfos (screen, &pc_vinfo, &tc_vinfo, &dc_vinfo);
    if ( (root_vinfo->depth == 8) && (root_vinfo->class == PseudoColor) &&
	 (root_vinfo->colormap_size == 256) )
    {
	/*  Use the default visual  */
	pseudocolour_vinfo = root_vinfo;
    }
    else pseudocolour_vinfo = pc_vinfo;
    if ( (root_vinfo->depth == 24) && (root_vinfo->class == DirectColor) &&
	 (root_vinfo->colormap_size == 256) )
    {
	/*  Use the default visual  */
	directcolour_vinfo = root_vinfo;
    }
    else directcolour_vinfo = dc_vinfo;
    if ( (root_vinfo->depth == 24) && (root_vinfo->class == TrueColor) &&
	 (root_vinfo->colormap_size == 256) )
    {
	/*  Use the default visual  */
	truecolour_vinfo = root_vinfo;
    }
    else truecolour_vinfo = tc_vinfo;
    /*  If stereo is requested, search for best support. Prefer MultiBuffering,
	and then OpenGL and then XGL  */
    if (XkwCanvasTypeStereo & new->multiCanvas.requestList)
    {
	/*  Stereo was requested: always test for MultiBuffering and OpenGL  */
	if ( XmbufQueryExtension (dpy, &dummy, &dummy) )
	{
	    xmbuf_stereo = TRUE;
	}
	have_open_gl = kwin_open_gl_test_available (dpy);
	if (new->multiCanvas.splitStereo) split_stereo = TRUE;
    }
    if (xmbuf_stereo)
    {
	if (XmbufGetScreenInfo (dpy, root_window,
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
	 !xmbuf_stereo && !have_open_gl )
    {
	/*  Want stereo but MultiBuffering and OpenGL no good  */
	if (kwin_xgl_test_stereo (dpy, root_window) ==
	    KWIN_XGL_STEREO_AVAILABLE)
	{
	    xgl_stereo = TRUE;
	}
    }
    if ( (XkwCanvasTypePseudoColour & new->multiCanvas.requestList) &&
	(pseudocolour_vinfo != NULL) )
    {
	create_canvases_for_visual (new, pseudocolour_vinfo, root_vinfo,
				    num_stereo_mbuf, stereo_mbuf_info,
				    have_open_gl, xgl_stereo, split_stereo);
    }
    if ( (XkwCanvasTypeDirectColour & new->multiCanvas.requestList) &&
	(directcolour_vinfo != NULL) )
    {
	create_canvases_for_visual (new, directcolour_vinfo, root_vinfo,
				    num_stereo_mbuf, stereo_mbuf_info,
				    have_open_gl, xgl_stereo, split_stereo);
    }
    if ( (XkwCanvasTypeTrueColour & new->multiCanvas.requestList) &&
	(truecolour_vinfo != NULL) )
    {
	create_canvases_for_visual (new, truecolour_vinfo, root_vinfo,
				    num_stereo_mbuf, stereo_mbuf_info,
				    have_open_gl, xgl_stereo, split_stereo);
    }
    /*  Clean up  */
    XFree ( (char *) root_vinfo );
    if (pc_vinfo != NULL) XFree ( (char *) pc_vinfo );
    if (dc_vinfo != NULL) XFree ( (char *) dc_vinfo );
    if (tc_vinfo != NULL) XFree ( (char *) tc_vinfo );
    if (num_stereo_mbuf > 0) XFree ( (char *) stereo_mbuf_info );
}   /*  End Function Initialise  */

static Boolean MultiCanvas__SetValues (Widget Current, Widget Request,
				       Widget New)
{
    /*MultiCanvasWidget current = (MultiCanvasWidget) Current;
      MultiCanvasWidget request = (MultiCanvasWidget) Request;
      MultiCanvasWidget new = (MultiCanvasWidget) New;*/

    return True;
}   /*  End Function SetValues  */

static flag stereo_supported_for_vinfo (XVisualInfo *vinfo, int num_info,
					XmbufBufferInfo *info)
/*  [PURPOSE] This function will determine if stereo is supported for a
    particular visual using the MultiBuffering extension.
    <visual> The VisualID.
    <num_info> The number of stereo multibuffer info structures.
    <info> The stereo multibuffer info structures.
    [RETURNS] TRUE if stereo is supported, else FALSE.
*/
{
    int count;

    for (count = 0; count < num_info; ++count)
    {
	if (info[count].visualid == vinfo->visualid) return (TRUE);
    }
    return (FALSE);
}   /*  End Function stereo_supported_for_vinfo  */

static void create_canvases_for_visual (MultiCanvasWidget parent,
					XVisualInfo *vinfo,
					XVisualInfo *root_vinfo,
					int num_stereo_mbuf,
					XmbufBufferInfo *stereo_mbuf_info,
					flag have_open_gl,
					flag xgl_stereo, flag split_stereo)
/*  [SUMMARY] Create canvas widgets.
    <parent> The parent widget.
    <vinfo> The visual information.
    <root_vinfo> The visual information for the root window.
    <num_stereo_mbuf> The number of stereo multibuffers available.
    <stereo_mbuf_info> The stereo MultiBuffering info.
    <have_open_gl> If TRUE OpenGL is available.
    <xgl_stereo> If TRUE, XGL is available and stereo is supported.
    <split_stereo> If TRUE, split stereo is requested.
    [RETURNS] Nothing.
*/
{
    flag verbose;
    int stereo_mode;
    Colormap cmap;
    Window root_window;
    Widget w, Parent;
    Display *dpy;
    Screen *screen;
    char visual_name[STRING_LENGTH];
    char mono_canvas_name[STRING_LENGTH];
    char stereo_canvas_name[STRING_LENGTH];
    static char function_name[] = "MultiCanvas::create_canvases_for_visual";

    verbose = parent->multiCanvas.verbose;
    parent->form.default_spacing = 0;
    dpy = XtDisplay ( (Widget) parent );
    screen = XtScreen ( (Widget) parent );
    root_window = RootWindowOfScreen (screen);
    Parent = (Widget) parent;
    if (vinfo->class == PseudoColor) strcpy (visual_name, "PseudoColour");
    else if (vinfo->class == DirectColor) strcpy (visual_name, "DirectColour");
    else if (vinfo->class == TrueColor) strcpy (visual_name, "TrueColour");
    else
    {
	fprintf (stderr, "Illegal visual\n");
	exit (1);
    }
    strcpy (mono_canvas_name, visual_name);
    mono_canvas_name[0] = tolower (mono_canvas_name[0]);
    strcat (mono_canvas_name, "Canvas");
    strcpy (stereo_canvas_name, visual_name);
    stereo_canvas_name[0] = tolower (stereo_canvas_name[0]);
    strcat (stereo_canvas_name, "StereoCanvas");
    if (vinfo == root_vinfo)
    {
	/*  Get root window colourmap  */
	cmap = parent->core.colormap;
    }
    else
    {
	/*  Inheriting the colourmap from the parent won't work: make one  */
	if ( ( cmap = XCreateColormap (dpy, root_window,
				       vinfo->visual, AllocNone) ) ==
	     (Colormap) NULL )
	{
	    fprintf (stderr, "Could not create colourmap\n");
	    exit (1);
	}
	XSync (dpy, False);
	if (verbose)
	{
	    fprintf (stderr, "%s: created colourmap: 0x%lx for %s visual\n",
		     function_name, cmap, visual_name);
	}
    }
    if (verbose)
    {
	fprintf (stderr, "%s: creating mono %s canvas...\n",
		 function_name, visual_name);
    }
    w = XtVaCreateManagedWidget (mono_canvas_name, canvasWidgetClass, Parent,
				 XtNforeground, 0,
				 XtNmappedWhenManaged, False,
				 XtNwidth, parent->core.width,
				 XtNheight, parent->core.height,
				 XtNvisual, vinfo->visual,
				 XtNcolormap, cmap,
				 XtNborderWidth, 0,
				 NULL);
    if ( stereo_supported_for_vinfo (vinfo, num_stereo_mbuf,
				     stereo_mbuf_info) )
    {
	if (verbose)
	{
	    fprintf (stderr, "%s: creating Xmbuf stereo %s canvas...\n",
		     function_name, visual_name);
	}
	stereo_mode = XkwSTEREO_MODE_XMBUF;
    }
    else if ( have_open_gl && kwin_open_gl_test_stereo (dpy, vinfo) )
    {
	if (verbose)
	{
	    fprintf (stderr, "%s: creating OpenGL stereo %s canvas...\n",
		     function_name, visual_name);
	}
	stereo_mode = XkwSTEREO_MODE_OpenGL;
    }
    else if (xgl_stereo)
    {
	if (verbose)
	{
	    fprintf (stderr, "%s: creating XGL stereo %s canvas...\n",
		     function_name, visual_name);
	}
	stereo_mode = XkwSTEREO_MODE_XGL;
    }
    else if (split_stereo)
    {
	if (verbose)
	{
	    fprintf (stderr,
		     "%s: creating split stereo PseudoColour canvas...\n",
		     function_name);
	}
	stereo_mode = XkwSTEREO_MODE_SPLIT;
    }
    else stereo_mode = XkwSTEREO_MODE_MONO;
    if (stereo_mode != XkwSTEREO_MODE_MONO)
    {
	w = XtVaCreateManagedWidget
	    (stereo_canvas_name, canvasWidgetClass, Parent,
	     XtNforeground, 0,
	     XkwNstereoMode, stereo_mode,
	     XtNmappedWhenManaged, False,
	     XtNwidth, parent->core.width,
	     XtNheight, parent->core.height,
	     XtNvisual, vinfo->visual,
	     XtNcolormap, cmap,
	     XtNborderWidth, 0,
	     NULL);
    }
}   /*  End Function create_canvases_for_visual  */

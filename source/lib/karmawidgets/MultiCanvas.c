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

    Last updated by Richard Gooch   4-JAN-1995: Cosmetic changes.


*/

#include <stdio.h>
#define X11
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/Form.h>
#include <karma.h>
#include <Xkw/MultiCanvasP.h>

#define offset(field) XtOffsetOf(MultiCanvasRec, multiCanvas.field)

static XtResource resources[] =
{
    {XkwNcanvasTypes, XkwCCanvasTypes, XtRInt, sizeof (int),
     offset (requestList), XtRImmediate, 0},
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
    MultiCanvasWidget request = (MultiCanvasWidget) Request;
    MultiCanvasWidget new = (MultiCanvasWidget) New;
    Widget w;
    Colormap pseudocolour_cmap, directcolour_cmap, truecolour_cmap;
    Display *dpy;
    XVisualInfo *vinfo;
    Visual *pseudocolour_visual, *directcolour_visual, *truecolour_visual;
    Visual *root_visual;
    Screen *screen;

    new->form.default_spacing = 0;
    dpy = XtDisplay (New);
    screen = XtScreen (New);
    /*  Get visual information  */
    root_visual = XDefaultVisualOfScreen (screen);
    vinfo = get_visinfo_for_visual (dpy, root_visual);
    get_visuals (screen, &pseudocolour_visual,
		 &truecolour_visual, &directcolour_visual);
    if ( (vinfo->depth == 8) && (vinfo->class = PseudoColor) &&
	(vinfo->colormap_size == 256) )
    {
	/*  Use the default visual  */
	pseudocolour_visual = root_visual;
    }
    if ( (vinfo->depth == 24) && (vinfo->class = DirectColor) &&
	(vinfo->colormap_size == 256) )
    {
	/*  Use the default visual  */
	directcolour_visual = root_visual;
    }
    if ( (vinfo->depth == 24) && (vinfo->class = TrueColor) &&
	(vinfo->colormap_size == 256) )
    {
	/*  Use the default visual  */
	truecolour_visual = root_visual;
    }
    XFree (vinfo);
    vinfo = NULL;
    if (XkwCanvasTypeStereo & new->multiCanvas.requestList)
    {
	(void) fprintf (stderr, "Stereo canvases not supported yet\n");
	exit (RV_UNDEF_ERROR);
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
	    (void) fprintf (stderr,
			    "Creating colourmap for PseudoColour visual...\n");
	    if ( ( pseudocolour_cmap =
		  XCreateColormap (dpy, XRootWindowOfScreen (screen),
				   pseudocolour_visual, AllocNone) ) ==
		(Colormap) NULL )
	    {
		(void) fprintf (stderr, "Could not create colourmap\n");
		exit (1);
	    }
	    XSync (dpy, False);
	    (void) fprintf (stderr, "Created colourmap: 0x%x\n",
			    pseudocolour_cmap);
	}
	(void) fprintf (stderr, "Creating pseudoColour canvas...\n");
	w = XtVaCreateManagedWidget ("pseudoColourCanvas", canvasWidgetClass,
				     New,
				     XtNmappedWhenManaged, False,
				     XtNwidth, new->core.width,
				     XtNheight, new->core.height,
				     XtNvisual, pseudocolour_visual,
				     XtNcolormap, pseudocolour_cmap,
				     XtNborderWidth, 0,
				     XtNbottom, XtChainBottom,
				     XtNtop, XtChainTop,
				     XtNleft, XtChainLeft,
				     XtNright, XtChainRight,
				     XtNborderWidth, 0,
				     NULL);
	new->multiCanvas.pseudoCanvas = w;
    }
    else
    {
	new->multiCanvas.pseudoCanvas = NULL;
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
	    (void) fprintf (stderr,
			    "Creating colourmap for directcolour visual...\n");
	    if ( ( directcolour_cmap =
		  XCreateColormap (dpy, XRootWindowOfScreen (screen),
				   directcolour_visual, AllocNone) ) ==
		(Colormap) NULL )
	    {
		(void) fprintf (stderr, "Could not create colourmap\n");
		exit (1);
	    }
	    XSync (dpy, False);
	    (void) fprintf (stderr, "Created colourmap: 0x%x\n",
			    directcolour_cmap);
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
	new->multiCanvas.directCanvas = w;
    }
    else
    {
	new->multiCanvas.directCanvas = NULL;
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
	    (void) fprintf (stderr,
			    "Creating colourmap for truecolour visual...\n");
	    if ( ( truecolour_cmap =
		  XCreateColormap (dpy, XRootWindowOfScreen (screen),
				   truecolour_visual, AllocNone) ) ==
		(Colormap) NULL )
	    {
		(void) fprintf (stderr, "Could not create colourmap\n");
		exit (1);
	    }
	    XSync (dpy, False);
	    (void) fprintf (stderr, "Created colourmap: 0x%x\n",
			    truecolour_cmap);
	}
	w = XtVaCreateManagedWidget ("trueColourCanvas", canvasWidgetClass,New,
				     XtNmappedWhenManaged, False,
				     XtNwidth, new->core.width,
				     XtNheight, new->core.height,
				     XtNvisual, truecolour_visual,
				     XtNcolormap, truecolour_cmap,
				     XtNborderWidth, 0,
				     NULL);
	new->multiCanvas.trueCanvas = w;
    }
    else
    {
	new->multiCanvas.trueCanvas = NULL;
    }
}   /*  End Function Initialise  */

static Boolean SetValues (Widget Current, Widget Request, Widget New)
{
    MultiCanvasWidget current = (MultiCanvasWidget) Current;
    MultiCanvasWidget request = (MultiCanvasWidget) Request;
    MultiCanvasWidget new = (MultiCanvasWidget) New;

    new->multiCanvas.pseudoCanvas = current->multiCanvas.pseudoCanvas;
    new->multiCanvas.directCanvas = current->multiCanvas.directCanvas;
    new->multiCanvas.trueCanvas = current->multiCanvas.trueCanvas;
    return True;
}   /*  End Function SetValues  */

void get_visuals (screen, pseudocolour, truecolour, directcolour)
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
	XFree (vinfos);
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
	XFree (vinfos);
    }
}   /*  End Function get_visuals  */

XVisualInfo *get_visinfo_for_visual (dpy, visual)
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

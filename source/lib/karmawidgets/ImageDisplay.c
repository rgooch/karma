/*LINTLIBRARY*/
/*  ImageDisplay.c

    This code provides an image display widget for Xt.

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

    This file contains all routines needed for an image display widget for Xt.


    Written by      Richard Gooch   18-DEC-1994

    Updated by      Richard Gooch   25-DEC-1994

    Updated by      Richard Gooch   27-DEC-1994: Trapped NULL visibleCanvas
  resource in SetValues.

    Updated by      Richard Gooch   3-JAN-1995: Removed *ColourCanvas resources
  Use XtNameToWidget instead.

    Updated by      Richard Gooch   6-JAN-1995: Took account of change in
  DataclipWidget from passing flag by value to by reference for
  intensityScaleCallback.

    Updated by      Richard Gooch   12-JAN-1995: Created cmapSize resource.

    Updated by      Richard Gooch   20-JAN-1995: No longer set visible canvas
  upon canvas refresh since not all refreshes are due to CanvasWidget mapping.

    Updated by      Richard Gooch   30-JAN-1995: Added stereo support.

    Updated by      Richard Gooch   17-APR-1995: Fixed mix of XtRBool and
  sizeof Boolean in resource fields. Use Bool because it is of int size.

    Updated by      Richard Gooch   10-JUL-1995: Added showAnimateButton
  resource.

    Updated by      Richard Gooch   13-JUL-1995: Made use of
  VIEWIMG_ATT_ALLOW_TRUNCATION attribute.

    Updated by      Richard Gooch   6-AUG-1995: Don't assume PseudoColour
  visual is always available.

    Updated by      Richard Gooch   29-AUG-1995: Set saturation pixels to auto.

    Updated by      Richard Gooch   1-SEP-1995: Moved to <kcmap_va_create>.

    Updated by      Richard Gooch   4-SEP-1995: Ensure one of the mono canvases
  is mapped when managed.

    Updated by      Richard Gooch   6-SEP-1995: Added "Export" menu. Supported
  Sun Rasterfile format.

    Updated by      Richard Gooch   7-SEP-1995: Update imageName member with
  dynamically allocated private copy of image name.

    Updated by      Richard Gooch   8-SEP-1995: Added experimental fullscreen
  resource.

    Updated by      Richard Gooch   23-SEP-1995: Took account of stereo modes.

    Uupdated by     Richard Gooch   11-OCT-1995: Forced use of same Kcolourmap
  for mono and stereo pseudocolour world canvases.

    Updated by      Richard Gooch   13-OCT-1995: Prevented unmap/map when
  switching active canvas from left<->right within same visual type.

    Last updated by Richard Gooch   28-DEC-1995: Added verbose resource.


*/

#include <stdio.h>
#include <math.h>
#define X11
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Toggle.h>
#include <karma.h>
#include <k_event_codes.h>
#include <karma_foreign.h>
#include <karma_viewimg.h>
#include <karma_xc.h>
#include <karma_ch.h>
#include <karma_st.h>
#include <karma_m.h>
#include <Xkw/ImageDisplayP.h>
#include <Xkw/MultiCanvas.h>
#include <Xkw/Filepopup.h>
#include <Xkw/Cmapwinpopup.h>
#include <Xkw/Postscript.h>
#include <Xkw/Dataclip.h>
#include <Xkw/ChoiceMenu.h>

#define DEFAULT_COLOURMAP_NAME "Greyscale1"

STATIC_FUNCTION (void Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (void Realise, (Widget w, Mask *valueMask,
				XSetWindowAttributes *attributes) );
STATIC_FUNCTION (Boolean SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void quit_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );
STATIC_FUNCTION (void popup_cbk, (Widget w, XtPointer client_data,
				  XtPointer call_data) );
STATIC_FUNCTION (void canvas_realise_cbk, (Widget w, XtPointer client_data,
					   XtPointer call_data) );
STATIC_FUNCTION (void zoom_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );
STATIC_FUNCTION (void postscript_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (void aspect_cbk, (Widget w, XtPointer client_data,
				   XtPointer call_data) );
STATIC_FUNCTION (void crosshair_cbk, (Widget w, XtPointer client_data,
				      XtPointer call_data) );
STATIC_FUNCTION (void iscale_cbk, (Widget w, XtPointer client_data,
				   XtPointer call_data) );
STATIC_FUNCTION (void region_cbk, (Widget w, XtPointer client_data,
				   XtPointer call_data) );
STATIC_FUNCTION (void worldcanvas_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale, Kcolourmap cmap,
		  flag cmap_resize, void **info) );
STATIC_FUNCTION (void load_cmap_for_frame, (multi_array *multi_desc) );
STATIC_FUNCTION (flag crosshair_click_consumer,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info, void **f_info) );
STATIC_FUNCTION (void draw_crosshair, (KWorldCanvas canvas, double x, double y,
				       ImageDisplayWidget w) );
STATIC_FUNCTION (void zoom,
		 (ImageDisplayWidget w, flag horizontal, flag vertical) );
STATIC_FUNCTION (void clear_crosshairs, (ImageDisplayWidget w) );
STATIC_FUNCTION (void show_crosshair_positions, (ImageDisplayWidget w) );
STATIC_FUNCTION (void irange_control, (double min, double max) );
STATIC_FUNCTION (flag log_iscale_func,
		 (double *out, unsigned int out_stride,
		  double *inp, unsigned int inp_stride,
		  unsigned int num_values, double i_min, double i_max,
		  void *info) );
STATIC_FUNCTION (void export_cbk, (Widget w, XtPointer client_data,
				   XtPointer call_data) );
STATIC_FUNCTION (void show_fullscreen, (ImageDisplayWidget w) );
STATIC_FUNCTION (void GlobalKeyPress, () );


static char *def_image_name = "fred";


#define offset(field) XtOffsetOf(ImageDisplayRec, imageDisplay.field)

static XtResource resources[] =
{
    {XkwNpseudoColourCanvas, XkwCWorldCanvas, XtRPointer, sizeof (XtPointer),
     offset (pseudoCanvas), XtRImmediate, NULL},
    {XkwNdirectColourCanvas, XkwCWorldCanvas, XtRPointer, sizeof (XtPointer),
     offset (directCanvas), XtRImmediate, NULL},
    {XkwNtrueColourCanvas, XkwCWorldCanvas, XtRPointer, sizeof (XtPointer),
     offset (trueCanvas), XtRImmediate, NULL},
    {XkwNpseudoColourLeftCanvas, XkwCWorldCanvas, XtRPointer,
     sizeof (XtPointer), offset (pseudoCanvasLeft), XtRImmediate, NULL},
    {XkwNpseudoColourRightCanvas, XkwCWorldCanvas, XtRPointer,
     sizeof (XtPointer), offset (pseudoCanvasRight), XtRImmediate, NULL},
    {XkwNdirectColourLeftCanvas, XkwCWorldCanvas, XtRPointer,
     sizeof (XtPointer), offset (directCanvasLeft), XtRImmediate, NULL},
    {XkwNdirectColourRightCanvas, XkwCWorldCanvas, XtRPointer,
     sizeof (XtPointer), offset (directCanvasRight), XtRImmediate, NULL},
    {XkwNtrueColourLeftCanvas, XkwCWorldCanvas, XtRPointer,
     sizeof (XtPointer), offset (trueCanvasLeft), XtRImmediate, NULL},
    {XkwNtrueColourRightCanvas, XkwCWorldCanvas, XtRPointer,
     sizeof (XtPointer), offset (trueCanvasRight), XtRImmediate, NULL},
    {XkwNvisibleCanvas, XkwCWorldCanvas, XtRPointer, sizeof (XtPointer),
     offset (visibleCanvas), XtRImmediate, NULL},
    {XkwNimageName, XkwCImageName, XtRString, sizeof (String),
     offset (imageName), XtRImmediate, NULL},
    {XkwNenableAnimation, XkwCEnableAnimation, XtRBool, sizeof (Bool),
     offset (enableAnimation), XtRImmediate, (XtPointer) False},
    {XkwNshowAnimateButton, XkwCShowAnimateButton, XtRBool, sizeof (Bool),
     offset (showAnimateButton), XtRImmediate, (XtPointer) True},
    {XkwNshowQuitButton, XkwCShowQuitButton, XtRBool, sizeof (Bool),
     offset (showQuitButton), XtRImmediate, (XtPointer) True},
    {XkwNcmapSize, XkwCCmapSize, XtRInt, sizeof (int),
     offset (cmapSize), XtRImmediate, (XtPointer) 200},
    {XkwNfullscreen, XkwCFullscreen, XtRBool, sizeof (Bool),
     offset (fullscreen), XtRImmediate, (XtPointer) False},
    {XkwNverbose, XkwCVerbose, XtRBool, sizeof (Bool),
     offset (verbose), XtRImmediate, False},
#undef offset
};

#define ZOOMCODE_HORIZONTAL (unsigned int) 0
#define ZOOMCODE_VERTICAL   (unsigned int) 1
#define ZOOMCODE_AREA       (unsigned int) 2
#define ZOOMCODE_UNZOOM     (unsigned int) 3
#define ZOOMCODE_INTENSITY  (unsigned int) 4
/*#define ZOOMCODE_FULLSCREEN  (unsigned int) 5*/
#define NUM_ZOOM_CHOICES 5

static char *zoom_choices[NUM_ZOOM_CHOICES] =
{
    "Horizontal", "Vertical", "2-dimensional", "Unzoom", "Intensity",
/*    "Fullscreen"*/
};

#define CROSSHAIRCODE_CLEAR (unsigned int) 0
#define CROSSHAIRCODE_SHOW  (unsigned int) 1
#define NUM_CROSSHAIR_CHOICES 2

static char *crosshair_choices[NUM_CROSSHAIR_CHOICES] =
{
    "Clear", "Show"
};

#define EXPORT_POSTSCRIPT (unsigned int) 0
#define EXPORT_SUNRAS     (unsigned int) 1
#define EXPORT_PPM        (unsigned int) 2
#define NUM_EXPORT_CHOICES 2

static char *export_choices[NUM_EXPORT_CHOICES] =
{
    "PostScript", "SunRasterfile" /*, "PortablePixelMap" */
};

ImageDisplayClassRec imageDisplayClassRec =
{
    {
	/* core fields  */
	(WidgetClass) &formClassRec,    /*  superclass             */
	"ImageDisplay",                 /*  class_name             */
	sizeof (ImageDisplayRec),       /*  widget_size            */
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
	sizeof (ImageDisplayConstraintsRec) /* constraint_size    */,
	NULL                            /* initialise         */,
	NULL                            /* destroy            */,
	NULL                            /* set_values         */,
	NULL                            /* extension          */ 
    },
    { /* Form */
	XtInheritLayout                 /* layout             */
    },
    {
	/*  imageDisplay fields */
	0                               /*  ignore                 */
    }
};

WidgetClass imageDisplayWidgetClass = (WidgetClass) &imageDisplayClassRec;

static void Initialise (Widget Request, Widget New)
{
    Boolean one_to_map = True;
    ImageDisplayWidget request = (ImageDisplayWidget) Request;
    ImageDisplayWidget new = (ImageDisplayWidget) New;
    int canvas_types;
    Widget filewin, files_btn, track_label, canvas;
    Widget colourmap_btn, aspect_tgl, quit_btn, menu_btn;
    Widget quit_neighbour, animate_btn;
    Widget pswinpopup, izoomwinpopup, animatepopup, shell;
    Widget pseudo_cnvs, direct_cnvs, true_cnvs;
    Display *dpy;
    Screen *screen;
    struct dual_crosshair_type *crosshairs;
    /*static char function_name[] = "ImageDisplayWidget::Initialise";*/

    if (new->imageDisplay.imageName == NULL)
    {
	new->imageDisplay.imageName = def_image_name;
    }
    new->imageDisplay.override_shell = NULL;
    crosshairs = &new->imageDisplay.crosshairs;
    m_clear ( (char *) crosshairs, sizeof *crosshairs );
    new->imageDisplay.visibleCanvas = NULL;
    new->imageDisplay.pseudo_cmap = NULL;
    dpy = XtDisplay (New);
    /*  Create as many popups as possible first  */
    filewin = XtVaCreatePopupShell ("filewinPopup", filepopupWidgetClass, New,
				    XtNtitle, "kview_2d File Selector",
				    XtNx, 0,
				    XtNy, 0,
				    XkwNautoPopdown, True,
				    NULL);
    new->imageDisplay.filepopup = filewin;
    pswinpopup = XtVaCreatePopupShell ("postscriptwinpopup",
				       postscriptWidgetClass, New,
				       XtNtitle, "Postscript Window",
				       NULL);
    new->imageDisplay.pswinpopup = pswinpopup;
    XtAddCallback (pswinpopup, XtNcallback, postscript_cbk, New);
    izoomwinpopup = XtVaCreatePopupShell ("izoomwinpopup",
					  dataclipWidgetClass, New,
					  XtNtitle, "Intensity Zoom",
					  XkwNshowIscaleButton, True,
					  NULL);
    new->imageDisplay.izoomwinpopup = izoomwinpopup;
    XtAddCallback (izoomwinpopup, XkwNintensityScaleCallback, iscale_cbk, New);
    XtAddCallback (izoomwinpopup, XkwNregionCallback, region_cbk, New);
    XtRealizeWidget (izoomwinpopup);
    if (new->imageDisplay.enableAnimation)
    {
	animatepopup = XtVaCreatePopupShell ("animatepopup",
					     animateControlWidgetClass, New,
					     XtNtitle, "Animation Control",
					     NULL);
	new->imageDisplay.animatepopup = animatepopup;
	XtRealizeWidget (animatepopup);
    }
    /*  Now create the widgets for the main window  */
    files_btn = XtVaCreateManagedWidget ("button", commandWidgetClass, New,
					 XtNlabel, "Files",
					 NULL);
    XtAddCallback (files_btn, XtNcallback, popup_cbk, filewin);
    colourmap_btn = XtVaCreateManagedWidget ("button", commandWidgetClass, New,
					     XtNlabel, "Colourmap",
					     XtNfromHoriz, files_btn,
					     NULL);
    new->imageDisplay.cmap_btn = colourmap_btn;
    menu_btn = XtVaCreateManagedWidget ("menuButton", choiceMenuWidgetClass,
					New,
					XtNlabel, "Zoom",
					XkwNmenuTitle, "Zoom Menu",
					XtNfromHoriz, colourmap_btn,
					XtNmenuName, "zoomMenu",
					XkwNnumItems, NUM_ZOOM_CHOICES,
					XkwNitemStrings, zoom_choices,
					NULL);
    XtAddCallback (menu_btn, XkwNselectCallback, zoom_cbk, New);
    aspect_tgl = XtVaCreateManagedWidget ("toggle", toggleWidgetClass, New,
					  XtNlabel, "Fix Aspect",
					  XtNstate, True,
					  XtNfromHoriz, menu_btn,
					  NULL);
    XtAddCallback (aspect_tgl, XtNcallback, aspect_cbk, New);
    menu_btn = XtVaCreateManagedWidget ("menuButton", choiceMenuWidgetClass,
					New,
					XtNlabel, "Crosshair",
					XkwNmenuTitle, "Crosshair Menu",
					XtNfromHoriz, aspect_tgl,
					XtNmenuName, "crosshairMenu",
					XkwNnumItems, NUM_CROSSHAIR_CHOICES,
					XkwNitemStrings, crosshair_choices,
					NULL);
    XtAddCallback (menu_btn, XkwNselectCallback, crosshair_cbk, New);
    menu_btn = XtVaCreateManagedWidget ("menuButton", choiceMenuWidgetClass,
					New,
					XtNlabel, "Export",
					XkwNmenuTitle, "Export Menu",
					XtNfromHoriz, menu_btn,
					XtNmenuName, "exportMenu",
					XkwNnumItems, NUM_EXPORT_CHOICES,
					XkwNitemStrings, export_choices,
					NULL);
    XtAddCallback (menu_btn, XkwNselectCallback, export_cbk, New);
    if (new->imageDisplay.enableAnimation &&
	new->imageDisplay.showAnimateButton)
    {
	animate_btn = XtVaCreateManagedWidget ("button", commandWidgetClass,
					       New,
					       XtNlabel, "Animate",
					       XtNfromHoriz, menu_btn,
					       NULL);
	quit_neighbour = animate_btn;
	XtAddCallback (animate_btn, XtNcallback, popup_cbk, animatepopup);
    }
    else
    {
	quit_neighbour = menu_btn;
    }
    if (new->imageDisplay.showQuitButton)
    {
	quit_btn = XtVaCreateManagedWidget ("quit", commandWidgetClass, New,
					    XtNlabel, "Quit",
					    XtNfromHoriz, quit_neighbour,
					    NULL);
	XtAddCallback (quit_btn, XtNcallback, quit_cbk, NULL);
    }
    track_label = XtVaCreateManagedWidget ("trackLabel", labelWidgetClass,New,
					   XtNlabel, "Track Output",
					   XtNwidth, 512,
					   XtNfromVert, files_btn,
					   NULL);
    new->imageDisplay.trackLabel = track_label;
    /*  Create the canvases  */
    canvas_types = XkwCanvasTypePseudoColour | XkwCanvasTypeDirectColour;
    canvas_types |= XkwCanvasTypeTrueColour | XkwCanvasTypeStereo;
    if (new->imageDisplay.fullscreen)
    {
	screen = new->core.screen;
	/*  Create the override shell  */
	shell = XtVaCreatePopupShell ("fullscreenPopup",
				      overrideShellWidgetClass, New,
				      XtNx, 0,
				      XtNy, 0,
				      XtNwidth, WidthOfScreen (screen),
				      XtNheight, HeightOfScreen (screen),
				      XtNborderWidth, 0,
				      NULL);
	new->imageDisplay.override_shell = shell;
	canvas = XtVaCreateManagedWidget ("multiCanvas",
					  multiCanvasWidgetClass, shell,
					  XkwNcanvasTypes, canvas_types,
					  XtNwidth, WidthOfScreen (screen),
					  XtNheight, HeightOfScreen (screen),
					  NULL);
    }
    else
    {
	canvas = XtVaCreateManagedWidget ("multiCanvas",multiCanvasWidgetClass,
					  New,
					  XtNfromVert, track_label,
					  XtNwidth, 512,
					  XtNheight, 512,
					  XkwNcanvasTypes, canvas_types,
					  NULL);
    }
    new->imageDisplay.multi_canvas = canvas;
    /*  Setup for mono canvases  */
    pseudo_cnvs = XtNameToWidget (canvas, "pseudoColourCanvas");
    direct_cnvs = XtNameToWidget (canvas, "directColourCanvas");
    true_cnvs = XtNameToWidget (canvas, "trueColourCanvas");
    if (pseudo_cnvs != NULL)
    {
	XtVaSetValues (pseudo_cnvs,
		       XtNmappedWhenManaged, one_to_map,
		       XkwNsilenceUnconsumed, True,
		       NULL);
	XtAddCallback (pseudo_cnvs, XkwNrealiseCallback, canvas_realise_cbk,
		       (XtPointer) New);
	one_to_map = False;
    }
    if (direct_cnvs != NULL)
    {
	XtVaSetValues (direct_cnvs,
		       XtNmappedWhenManaged, one_to_map,
		       XkwNsilenceUnconsumed, True,
		       NULL);
	XtAddCallback (direct_cnvs, XkwNrealiseCallback, canvas_realise_cbk,
		       (XtPointer) New);
	one_to_map = False;
    }
    if (true_cnvs != NULL)
    {
	XtVaSetValues (true_cnvs,
		       XtNmappedWhenManaged, one_to_map,
		       XkwNsilenceUnconsumed, True,
		       NULL);
	XtAddCallback (true_cnvs, XkwNrealiseCallback, canvas_realise_cbk,
		       (XtPointer) New);
	one_to_map = False;
    }
    /*  Setup for stereo canvases  */
    pseudo_cnvs = XtNameToWidget (canvas, "pseudoColourStereoCanvas");
    direct_cnvs = XtNameToWidget (canvas, "directColourStereoCanvas");
    true_cnvs = XtNameToWidget (canvas, "trueColourStereoCanvas");
    if (pseudo_cnvs != NULL)
    {
	XtVaSetValues (pseudo_cnvs,
		       XkwNsilenceUnconsumed, True,
		       NULL);
	XtAddCallback (pseudo_cnvs, XkwNrealiseCallback, canvas_realise_cbk,
		       (XtPointer) New);
    }
    if (direct_cnvs != NULL)
    {
	XtVaSetValues (direct_cnvs,
		       XkwNsilenceUnconsumed, True,
		       NULL);
	XtAddCallback (direct_cnvs, XkwNrealiseCallback, canvas_realise_cbk,
		       (XtPointer) New);
    }
    if (true_cnvs != NULL)
    {
	XtVaSetValues (true_cnvs,
		       XkwNsilenceUnconsumed, True,
		       NULL);
	XtAddCallback (true_cnvs, XkwNrealiseCallback, canvas_realise_cbk,
		       (XtPointer) New);
    }
    if (new->imageDisplay.fullscreen)
    {
	XtRealizeWidget (shell);
	XtPopup (shell, XtGrabNone);
    }
}   /*  End Function Initialise  */

static Boolean SetValues (Widget Current, Widget Request, Widget New)
{
    KWorldCanvas old_wc, new_wc;
    ImageDisplayWidget current = (ImageDisplayWidget) Current;
    ImageDisplayWidget request = (ImageDisplayWidget) Request;
    ImageDisplayWidget new = (ImageDisplayWidget) New;
    Widget multi_canvas, old_cnv, new_cnv;
    static char function_name[] = "ImageDisplayWidget::SetValues";

    if (new->imageDisplay.cmapSize != current->imageDisplay.cmapSize)
    {
	if ( !kcmap_change (new->imageDisplay.pseudo_cmap, NULL,
			    (unsigned int) new->imageDisplay.cmapSize, TRUE) )
	{
	    (void) fprintf (stderr, "Error resizing colourmap to: %d cells\n",
			    new->imageDisplay.cmapSize);
	    new->imageDisplay.cmapSize = current->imageDisplay.cmapSize;
	}
    }
    if (new->imageDisplay.imageName != current->imageDisplay.imageName)
    {
	if ( (current->imageDisplay.imageName != NULL) &&
	    (current->imageDisplay.imageName != def_image_name) )
	{
	    /*  The was an old name and it's not the default: free it  */
	    m_free ( (char *) current->imageDisplay.imageName );
	}
	if (new->imageDisplay.imageName == NULL)
	{
	    new->imageDisplay.imageName = def_image_name;
	}
	else
	{
	    if ( ( new->imageDisplay.imageName =
		  st_dup (new->imageDisplay.imageName) ) == NULL )
	    {
		m_abort (function_name, "image name");
	    }
	}
    }
    old_wc = current->imageDisplay.visibleCanvas;
    new_wc = new->imageDisplay.visibleCanvas;
    multi_canvas = new->imageDisplay.multi_canvas;
    if (new_wc != old_wc)
    {
	if (new_wc == NULL)
	{
	    (void) fprintf (stderr, "NULL visibleCanvas resource!\n");
	    a_prog_bug (function_name);
	}
	if (old_wc == NULL) old_cnv = NULL;
	else if (old_wc == new->imageDisplay.pseudoCanvas)
	{
	    old_cnv = XtNameToWidget (multi_canvas, "pseudoColourCanvas");
	}
	else if (old_wc == new->imageDisplay.directCanvas)
	{
	    old_cnv = XtNameToWidget (multi_canvas, "directColourCanvas");
	}
	else if (old_wc == new->imageDisplay.trueCanvas)
	{
	    old_cnv = XtNameToWidget (multi_canvas, "trueColourCanvas");
	}
	else if (old_wc == new->imageDisplay.pseudoCanvasLeft)
	{
	    old_cnv = XtNameToWidget (multi_canvas,"pseudoColourStereoCanvas");
	}
	else if (old_wc == new->imageDisplay.pseudoCanvasRight)
	{
	    old_cnv = XtNameToWidget (multi_canvas,"pseudoColourStereoCanvas");
	}
	else if (old_wc == new->imageDisplay.directCanvasLeft)
	{
	    old_cnv = XtNameToWidget (multi_canvas,"directColourStereoCanvas");
	}
	else if (old_wc == new->imageDisplay.directCanvasRight)
	{
	    old_cnv = XtNameToWidget (multi_canvas,"directColourStereoCanvas");
	}
	else if (old_wc == new->imageDisplay.trueCanvasLeft)
	{
	    old_cnv = XtNameToWidget (multi_canvas, "trueColourStereoCanvas");
	}
	else if (old_wc == new->imageDisplay.trueCanvasRight)
	{
	    old_cnv = XtNameToWidget (multi_canvas, "trueColourStereoCanvas");
	}
	else
	{
	    (void) fprintf (stderr, "Visible canvas: %p unknown!\n", old_wc);
	    a_prog_bug (function_name);
	}
	if (new_wc == new->imageDisplay.pseudoCanvas)
	{
	    new_cnv = XtNameToWidget (multi_canvas, "pseudoColourCanvas");
	}
	else if (new_wc == new->imageDisplay.directCanvas)
	{
	    new_cnv = XtNameToWidget (multi_canvas, "directColourCanvas");
	}
	else if (new_wc == new->imageDisplay.trueCanvas)
	{
	    new_cnv = XtNameToWidget (multi_canvas, "trueColourCanvas");
	}
	else if (new_wc == new->imageDisplay.pseudoCanvasLeft)
	{
	    new_cnv = XtNameToWidget (multi_canvas,"pseudoColourStereoCanvas");
	}
	else if (new_wc == new->imageDisplay.pseudoCanvasRight)
	{
	    new_cnv = XtNameToWidget (multi_canvas,"pseudoColourStereoCanvas");
	}
	else if (new_wc == new->imageDisplay.directCanvasLeft)
	{
	    new_cnv = XtNameToWidget (multi_canvas,"directColourStereoCanvas");
	}
	else if (new_wc == new->imageDisplay.directCanvasRight)
	{
	    new_cnv = XtNameToWidget (multi_canvas,"directColourStereoCanvas");
	}
	else if (new_wc == new->imageDisplay.trueCanvasLeft)
	{
	    new_cnv = XtNameToWidget (multi_canvas, "trueColourStereoCanvas");
	}
	else if (new_wc == new->imageDisplay.trueCanvasRight)
	{
	    new_cnv = XtNameToWidget (multi_canvas, "trueColourStereoCanvas");
	}
	else
	{
	    (void) fprintf (stderr, "Visible canvas: %p unknown!\n", new_wc);
	    a_prog_bug (function_name);
	}
	if (old_cnv == new_cnv) return False;
	if (old_cnv != NULL) XtUnmapWidget (old_cnv);
	XtMapWidget (new_cnv);
	return False;
    }
    return True;
}   /*  End Function SetValues  */

static void quit_cbk (w, client_data, call_data)
/*  This is the quit button callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    exit (RV_OK);
}   /*  End Function quit_cbk   */

static void popup_cbk (w, client_data, call_data)
/*  This is the generic popup button callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    Widget popup = (Widget) client_data;

    XtPopup (popup, XtGrabNone);
}   /*  End Function popup_cbk   */

static void canvas_realise_cbk (w, client_data, call_data)
/*  This is the canvas realise callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    Kcolourmap kcmap;
    Kdisplay dpy_handle;
    flag verbose;
    Boolean mappable;
    KPixCanvas pixcanvas, *stereopixcanvases;
    int stereo_mode;
    unsigned int num_ccels;
    unsigned int visual_type;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;
    Colormap xcmap;
    GC gc_image, gc_crosshair;
    XGCValues gcvalues;
    Widget colourmapwinpopup, multicanvas;
    Widget pseudo_cnvs, direct_cnvs, true_cnvs;
    KWorldCanvas *worldcanvas, *leftworldcanvas, *rightworldcanvas;
    unsigned long *pixel_values;
    Display *dpy;
    Visual *visual;
    struct win_scale_type win_scale;
    static char function_name[] = "ImageDisplayWidget::canvas_realise_cbk";

    verbose = top->imageDisplay.verbose;
    dpy = XtDisplay (w);
    multicanvas = top->imageDisplay.multi_canvas;
    XtVaGetValues (w,
		   XkwNstereoMode, &stereo_mode,
		   NULL);
    if (stereo_mode == XkwSTEREO_MODE_MONO)
    {
	pseudo_cnvs = XtNameToWidget (multicanvas, "pseudoColourCanvas");
	direct_cnvs = XtNameToWidget (multicanvas, "directColourCanvas");
	true_cnvs = XtNameToWidget (multicanvas, "trueColourCanvas");
	pixcanvas = (KPixCanvas) call_data;
    }
    else
    {
	pseudo_cnvs = XtNameToWidget (multicanvas, "pseudoColourStereoCanvas");
	direct_cnvs = XtNameToWidget (multicanvas, "directColourStereoCanvas");
	true_cnvs = XtNameToWidget (multicanvas, "trueColourStereoCanvas");
	stereopixcanvases = (KPixCanvas *) call_data;
	pixcanvas = stereopixcanvases[0];
    }
    kwin_get_attributes (pixcanvas,
			 KWIN_ATT_VISUAL, &visual_type,
			 KWIN_ATT_END);
    XtVaGetValues (w,
		   XtNcolormap, &xcmap,
		   XtNbackground, &gcvalues.background,
		   XtNmappedWhenManaged, &mappable,
		   XtNvisual, &visual,
		   NULL);
    if (verbose)
    {
	(void) fprintf ( stderr, "%s: visual: %p visualID: %p\n",
			function_name, visual, XVisualIDFromVisual (visual) );
    }
    if (stereo_mode != XkwSTEREO_MODE_XGL)
    {
	/*  Create graphics context for crosshairs  */
	gcvalues.function = GXinvert;
	gc_image = kwin_get_gc_x (pixcanvas);
	gc_crosshair = XCreateGC (dpy, XtWindow (w), GCBackground | GCFunction,
				  &gcvalues);
    }
    else
    {
	gc_image = NULL;
	gc_crosshair = NULL;
    }
    if (w == pseudo_cnvs)
    {
	if (visual_type != KWIN_VISUAL_PSEUDOCOLOUR)
	{
	    (void) fprintf (stderr,"pseudo_canvas not PseudoColour visual!\n");
	    a_prog_bug (function_name);
	}
	if (top->imageDisplay.pseudo_cmap == NULL)
	{
	    if ( ( dpy_handle = xc_get_dpy_handle (dpy, xcmap) )
		== NULL )
	    {
		(void) fprintf (stderr, "Error getting display handle\n");
		a_prog_bug (function_name);
	    }
	    if ( ( kcmap = kcmap_va_create (DEFAULT_COLOURMAP_NAME,
					    top->imageDisplay.cmapSize, TRUE,
					    dpy_handle,
					    xc_alloc_colours, xc_free_colours,
					    xc_store_colours, xc_get_location,
					    KCMAP_ATT_END) )
		== NULL )
	    {
		(void) fprintf (stderr, "Error creating main colourmap\n");
		a_prog_bug (function_name);
	    }
	    top->imageDisplay.pseudo_cmap = kcmap;
	    num_ccels = kcmap_get_pixels (kcmap, &pixel_values);
	    if (verbose)
	    {
		(void) fprintf (stderr, "%s: num colours: %u\n",
				function_name, num_ccels);
	    }
	    colourmapwinpopup = XtVaCreatePopupShell ("cmapwinpopup",
						      cmapwinpopupWidgetClass,
						      (Widget) top,
						      XtNvisual, visual,
						      XtNdepth, 8,
						      XtNcolormap, xcmap,
						      XkwNkarmaColourmap,kcmap,
						      NULL);
	    XtAddCallback (top->imageDisplay.cmap_btn, XtNcallback, popup_cbk,
			   colourmapwinpopup);
	}
	else kcmap = top->imageDisplay.pseudo_cmap;
	top->imageDisplay.pseudo_main_gc = gc_image;
	top->imageDisplay.pseudo_crosshair_gc = gc_crosshair;
	worldcanvas = &top->imageDisplay.pseudoCanvas;
	leftworldcanvas = &top->imageDisplay.pseudoCanvasLeft;
	rightworldcanvas = &top->imageDisplay.pseudoCanvasRight;
    }
    else if (w == direct_cnvs)
    {
	if (visual_type != KWIN_VISUAL_DIRECTCOLOUR)
	{
	    (void) fprintf (stderr,"direct_canvas not DirectColour visual!\n");
	    a_prog_bug (function_name);
	}
	kcmap = NULL;
	top->imageDisplay.direct_main_gc = gc_image;
	top->imageDisplay.direct_crosshair_gc = gc_crosshair;
	worldcanvas = &top->imageDisplay.directCanvas;
	leftworldcanvas = &top->imageDisplay.directCanvasLeft;
	rightworldcanvas = &top->imageDisplay.directCanvasRight;
    }
    else if (w == true_cnvs)
    {
	if (visual_type != KWIN_VISUAL_TRUECOLOUR)
	{
	    (void) fprintf (stderr, "huge_canvas not TrueColour visual!\n");
	    a_prog_bug (function_name);
	}
	kcmap = NULL;
	top->imageDisplay.true_main_gc = gc_image;
	top->imageDisplay.true_crosshair_gc = gc_crosshair;
	worldcanvas = &top->imageDisplay.trueCanvas;
	leftworldcanvas = &top->imageDisplay.trueCanvasLeft;
	rightworldcanvas = &top->imageDisplay.trueCanvasRight;
    }
    else
    {
	(void) fprintf (stderr, "Bad canvas passed: %p\n", w);
	a_prog_bug (function_name);
    }
    /*  First initialise win_scale since all data is wiped here  */
    canvas_init_win_scale (&win_scale, K_WIN_SCALE_MAGIC_NUMBER);
    win_scale.blank_pixel = gcvalues.background;
    /*  Create the main world canvas (with dummy world min and max)  */
    win_scale.conv_type = CONV1_REAL;
/*
    win_scale.iscale_func = log_iscale_func;
*/
    if (stereo_mode == XkwSTEREO_MODE_MONO)
    {
	if ( ( *worldcanvas = canvas_create (pixcanvas, kcmap, &win_scale) )
	    == NULL )
	{
	    exit (RV_UNDEF_ERROR);
	}
	canvas_set_attributes (*worldcanvas,
			       CANVAS_ATT_AUTO_MIN_SAT, TRUE,
			       CANVAS_ATT_AUTO_MAX_SAT, TRUE,
			       CANVAS_ATT_END);
	if (mappable) top->imageDisplay.visibleCanvas = *worldcanvas;
	canvas_register_position_event_func (*worldcanvas,
					     crosshair_click_consumer,
					     (void *) top);
	viewimg_init (*worldcanvas);
	viewimg_set_canvas_attributes (*worldcanvas,
				       VIEWIMG_ATT_MAINTAIN_ASPECT, TRUE,
				       VIEWIMG_ATT_ALLOW_TRUNCATION, TRUE,
				       VIEWIMG_ATT_END);
	canvas_register_refresh_func (*worldcanvas, worldcanvas_refresh_func,
				      (void *) top);
    }
    else
    {
	if ( ( *leftworldcanvas = canvas_create (stereopixcanvases[0], kcmap,
						 &win_scale) ) == NULL )
	{
	    exit (RV_UNDEF_ERROR);
	}
	canvas_set_attributes (*leftworldcanvas,
			       CANVAS_ATT_AUTO_MIN_SAT, TRUE,
			       CANVAS_ATT_AUTO_MAX_SAT, TRUE,
			       CANVAS_ATT_END);
	if ( ( *rightworldcanvas = canvas_create (stereopixcanvases[1], kcmap,
						  &win_scale) ) == NULL )
	{
	    exit (RV_UNDEF_ERROR);
	}
	canvas_set_attributes (*rightworldcanvas,
			       CANVAS_ATT_AUTO_MIN_SAT, TRUE,
			       CANVAS_ATT_AUTO_MAX_SAT, TRUE,
			       CANVAS_ATT_END);
	if (mappable) top->imageDisplay.visibleCanvas = *leftworldcanvas;
	canvas_register_position_event_func (*leftworldcanvas,
					     crosshair_click_consumer,
					     (void *) top);
	viewimg_init (*leftworldcanvas);
	viewimg_set_canvas_attributes (*leftworldcanvas,
				       VIEWIMG_ATT_MAINTAIN_ASPECT, TRUE,
				       VIEWIMG_ATT_ALLOW_TRUNCATION, TRUE,
				       VIEWIMG_ATT_END);
	canvas_register_refresh_func (*leftworldcanvas,
				      worldcanvas_refresh_func, (void *) top);
	canvas_register_position_event_func (*rightworldcanvas,
					     crosshair_click_consumer,
					     (void *) top);
	viewimg_init (*rightworldcanvas);
	viewimg_set_canvas_attributes (*rightworldcanvas,
				       VIEWIMG_ATT_MAINTAIN_ASPECT, TRUE,
				       VIEWIMG_ATT_ALLOW_TRUNCATION, TRUE,
				       VIEWIMG_ATT_END);
	canvas_register_refresh_func (*rightworldcanvas,
				      worldcanvas_refresh_func, (void *) top);
    }
}   /*  End Function canvas_realise_cbk   */

static void zoom_cbk (w, client_data, call_data)
/*  This is the zoom menu callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    unsigned int zoomcode = *(int *) call_data;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;
    static char function_name[] = "ImageDisplayWidget::zoom_cbk";

    switch (zoomcode)
    {
      case ZOOMCODE_HORIZONTAL:
	zoom (top, TRUE, FALSE);
	break;
      case ZOOMCODE_VERTICAL:
	zoom (top, FALSE, TRUE);
	break;
      case ZOOMCODE_AREA:
	zoom (top, TRUE, TRUE);
	break;
      case ZOOMCODE_UNZOOM:
	zoom (top, FALSE, FALSE);
	break;
      case ZOOMCODE_INTENSITY:
	XtPopup (top->imageDisplay.izoomwinpopup, XtGrabNone);
	break;
/*
      case ZOOMCODE_FULLSCREEN:
	show_fullscreen (top);
	break;
*/
      default:
	(void) fprintf (stderr, "Illegal zoom code: %u\n", zoomcode);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function zoom_cbk   */

static void postscript_cbk (w, client_data, call_data)
/*  This is the PostScript callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    KPixCanvas pixcanvas;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;

    pixcanvas = canvas_get_pixcanvas (top->imageDisplay.visibleCanvas);
    XkwPostscriptRegisterImageAndName (w, pixcanvas,
				       top->imageDisplay.imageName);
}   /*  End Function postscript_cbk   */

static void aspect_cbk (w, client_data, call_data)
/*  This is the aspect callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    flag keep_aspect_ratio = (flag) call_data;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;

    viewimg_set_canvas_attributes (top->imageDisplay.pseudoCanvas,
				   VIEWIMG_ATT_MAINTAIN_ASPECT,
				   keep_aspect_ratio,
				   VIEWIMG_ATT_END);
    if (top->imageDisplay.directCanvas != NULL)
    {
	viewimg_set_canvas_attributes (top->imageDisplay.directCanvas,
				       VIEWIMG_ATT_MAINTAIN_ASPECT,
				       keep_aspect_ratio,
				       VIEWIMG_ATT_END);
    }
    if (top->imageDisplay.trueCanvas != NULL)
    {
	viewimg_set_canvas_attributes (top->imageDisplay.trueCanvas,
				       VIEWIMG_ATT_MAINTAIN_ASPECT,
				       keep_aspect_ratio,
				       VIEWIMG_ATT_END);
    }
    /*  Resize the canvas  */
    if ( !canvas_resize (top->imageDisplay.visibleCanvas, NULL, FALSE) )
    {
	(void) fprintf (stderr, "Error resizing window\n");
    }
}   /*  End Function aspect_cbk   */

static void crosshair_cbk (w, client_data, call_data)
/*  This is the crosshair menu callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    unsigned int crosshaircode = *(int *) call_data;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;
    static char function_name[] = "ImageDisplayWidget::crosshair_cbk";

    switch (crosshaircode)
    {
      case CROSSHAIRCODE_CLEAR:
	clear_crosshairs (top);
	break;
      case CROSSHAIRCODE_SHOW:
	show_crosshair_positions (top);
	break;
      default:
	(void) fprintf (stderr, "Illegal crosshair code: %u\n", crosshaircode);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function crosshair_cbk   */

static void iscale_cbk (w, client_data, call_data)
/*  This is the intensity scale callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    flag auto_v = *(flag *) call_data;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;
    static char function_name[] = "ImageDisplayWidget::iscale_cbk";

    FLAG_VERIFY (auto_v);
    viewimg_set_canvas_attributes (top->imageDisplay.pseudoCanvas,
				   VIEWIMG_ATT_AUTO_V, auto_v,
				   VIEWIMG_ATT_END);
    if (top->imageDisplay.directCanvas != NULL)
    {
	viewimg_set_canvas_attributes (top->imageDisplay.directCanvas,
				       VIEWIMG_ATT_AUTO_V, auto_v,
				       VIEWIMG_ATT_END);
    }
    if (top->imageDisplay.trueCanvas != NULL)
    {
	viewimg_set_canvas_attributes (top->imageDisplay.trueCanvas,
				       VIEWIMG_ATT_AUTO_V, auto_v,
				       VIEWIMG_ATT_END);
    }
    /*  Resize the canvas  */
    if ( !canvas_resize (top->imageDisplay.visibleCanvas, NULL, FALSE) )
    {
	(void) fprintf (stderr, "Error resizing window\n");
    }
}   /*  End Function iscale_cbk   */

static void region_cbk (w, client_data, call_data)
/*  This is the region callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    DataclipRegions *regions = (DataclipRegions *) call_data;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;
    int dummy;
    struct win_scale_type win_scale;
    static char function_name[] = "ImageDisplayWidget::region_cbk";

    if (regions->num_regions != 1)
    {
	(void) fprintf (stderr, "num_regions: %u is not 1\n",
			regions->num_regions);
	a_prog_bug (function_name);
    }
    canvas_get_size (top->imageDisplay.pseudoCanvas, &dummy, &dummy,
		     &win_scale);
    win_scale.z_min = regions->minima[0];
    win_scale.z_max = regions->maxima[0];
    if ( !canvas_resize (top->imageDisplay.visibleCanvas, &win_scale,
			 FALSE) )
    {
	(void) fprintf (stderr, "Error resizing window\n");
    }
}   /*  End Function region_cbk   */

static void worldcanvas_refresh_func (canvas, width, height, win_scale,
				      cmap, cmap_resize, info)
/*  This routine is a refresh event consumer for a world canvas.
    The canvas is given by  canvas  .
    The width of the canvas in pixels is given by  width  .
    The height of the canvas in pixels is given by  height  .
    The window scaling information is pointed to by  win_scale  .
    The colourmap associated with the canvas is given by  cmap  .
    If the refresh function was called as a result of a colourmap resize
    the value of  cmap_resize  will be TRUE.
    The arbitrary canvas information pointer is pointed to by  info  .
    The routine returns nothing.
*/
KWorldCanvas canvas;
int width;
int height;
struct win_scale_type *win_scale;
Kcolourmap cmap;
flag cmap_resize;
void **info;
{
    ImageDisplayWidget w = (ImageDisplayWidget) *info;
    struct dual_crosshair_type crosshairs;

    crosshairs = w->imageDisplay.crosshairs;
    /*  Draw crosshairs (if neccessary)  */
    if (crosshairs.num_crosshairs > 0)
    {
	draw_crosshair (canvas, crosshairs.first.x, crosshairs.first.y, w);
	if (crosshairs.num_crosshairs > 1)
	{
	    draw_crosshair (canvas, crosshairs.second.x, crosshairs.second.y,
			    w);
	}
    }
}   /*  End Function worldcanvas_refresh_func  */

static flag crosshair_click_consumer (canvas, x, y, event_code, e_info, f_info)
/*  This routine is a position event consumer for a world canvas.
    The canvas is given by  canvas  .
    The horizontal world co-ordinate of the event will be given by  x  .
    The vertical world co-ordinate of the event will be given by  y  .
    The arbitrary event code is given by  event_code  .
    The arbitrary event information is pointed to by  e_info  .
    The arbitrary function information pointer is pointed to by  f_info  .
    The routine returns TRUE if the event was consumed, else it return
    FALSE indicating that the event is still to be processed.
*/
KWorldCanvas canvas;
double x;
double y;
unsigned int event_code;
void *e_info;
void **f_info;
{
    ImageDisplayWidget w = (ImageDisplayWidget) *f_info;
    struct dual_crosshair_type *crosshairs;
    static char function_name[]="ImageDisplayWidget::crosshair_click_consumer";

    if (event_code != K_CANVAS_EVENT_LEFT_MOUSE_CLICK) return (FALSE);
    crosshairs = &w->imageDisplay.crosshairs;
    switch (crosshairs->num_crosshairs)
    {
      case 0:
	/*  Set the first crosshairs  */
	crosshairs->first.x = x;
	crosshairs->first.y = y;
	++crosshairs->num_crosshairs;
	/*  Draw the first crosshairs  */
	draw_crosshair (canvas, crosshairs->first.x, crosshairs->first.y,
			w);
	break;
      case 1:
	/*  Set the second crosshairs  */
	crosshairs->second.x = x;
	crosshairs->second.y = y;
	++crosshairs->num_crosshairs;
	/*  Draw the second crosshairs  */
	draw_crosshair (canvas, crosshairs->second.x, crosshairs->second.y,
			w);
	break;
      case 2:
	/*  Remove the old first crosshairs on the canvas  */
	draw_crosshair (canvas, crosshairs->first.x, crosshairs->first.y,
			w);
	/*  Set the first crosshairs  */
	crosshairs->first.x = x;
	crosshairs->first.y = y;
	++crosshairs->num_crosshairs;
	/*  Draw the new first crosshairs  */
	draw_crosshair (canvas, crosshairs->first.x, crosshairs->first.y,
			w);
	break;
      case 3:
	/*  Remove the old second crosshairs on the canvas  */
	draw_crosshair (canvas, crosshairs->second.x, crosshairs->second.y,
			w);
	/*  Set the second crosshairs  */
	crosshairs->second.x = x;
	crosshairs->second.y = y;
	crosshairs->num_crosshairs = 2;
	/*  Draw the new second crosshairs  */
	draw_crosshair (canvas, crosshairs->second.x, crosshairs->second.y,
			w);
	break;
      default:
	(void) fprintf (stderr, "Number of crosshairs: %u is too large\n",
			crosshairs->num_crosshairs);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function crosshair_click_consumer  */

static void draw_crosshair (KWorldCanvas canvas, double x, double y,
			    ImageDisplayWidget w)
/*  This routine will draw (using an XOR graphics context) a crosshair
    The canvas must be given by  canvas  .
    The co-ordinates must be specified by  x  and  y  .
    The routine returns nothing.
*/
{
    KPixCanvas pixcanvas;
    int width, height;
    GC oldGC, newGC;
    double range;
    double value[2];
    struct win_scale_type win_scale;

    value[0] = 0.0;
    value[1] = 0.0;
    pixcanvas = canvas_get_pixcanvas (canvas);
    if (canvas == w->imageDisplay.pseudoCanvas)
    {
	oldGC = w->imageDisplay.pseudo_main_gc;
	newGC = w->imageDisplay.pseudo_crosshair_gc;
    }
    else
    {
	oldGC = w->imageDisplay.true_main_gc;
	newGC = w->imageDisplay.true_crosshair_gc;
    }
    kwin_set_gc_x (pixcanvas, newGC);
    canvas_get_size (canvas, &width, &height, &win_scale);
    /*  Draw vertical hair  */
    range = win_scale.y_max - win_scale.y_min;
    canvas_draw_line_p (canvas,
			x, win_scale.y_min - range,
			x, win_scale.y_max + range, 0);
    /*  Draw horizontal hair  */
    range = win_scale.x_max - win_scale.x_min;
    canvas_draw_line_p (canvas,
			win_scale.x_min - range, y,
			win_scale.x_max + range, y, 0);
    kwin_set_gc_x (pixcanvas, oldGC);
}   /*  End Function draw_crosshair  */

static void zoom (ImageDisplayWidget w, flag horizontal, flag vertical)
/*  This routine will zoom the canvas.
    If the value of the  horizontal  flag is TRUE, the canvas will be zoomed
    horizontally.
    If the value of the  vertical  flag is TRUE, the canvas will be zoomed
    vertically.
    If both  horizontal  and  vertical  are not TRUE, the canvas will be
    unzoomed.
    The routine returns nothing.
*/
{
    KWorldCanvas visible_worldcanvas = w->imageDisplay.visibleCanvas;
    int width, height;
    struct win_scale_type win_scale;
    struct dual_crosshair_type crosshairs;
    static char function_name[] = "zoom";

    FLAG_VERIFY (horizontal);
    FLAG_VERIFY (vertical);
    crosshairs = w->imageDisplay.crosshairs;
    if (horizontal || vertical)
    {
	if (crosshairs.num_crosshairs < 2)
	{
	    /*  Clear the crosshairs (not enough to zoom)  */
	    clear_crosshairs (w);
	    return;
	}
    }
    else
    {
	/*  Unzoom  */
	clear_crosshairs (w);
	viewimg_set_canvas_attributes (visible_worldcanvas,
				       VIEWIMG_ATT_AUTO_X, TRUE,
				       VIEWIMG_ATT_AUTO_Y, TRUE,
				       VIEWIMG_ATT_END);
	/*  Redraw the canvas  */
	if ( !canvas_resize (visible_worldcanvas, NULL, FALSE) )
	{
	    (void) fprintf (stderr, "Error refreshing window\n");
	}
	return;
    }
    /*  Not an unzoom  */
    canvas_get_size (visible_worldcanvas, &width, &height, &win_scale);
    if (horizontal)
    {
	if (crosshairs.first.x == crosshairs.second.x)
	{
	    /*  Can't do an infinite zoom  */
	    clear_crosshairs (w);
	    return;
	}
	viewimg_set_canvas_attributes (visible_worldcanvas,
				       VIEWIMG_ATT_AUTO_X, FALSE,
				       VIEWIMG_ATT_END);
	if (crosshairs.first.x < crosshairs.second.x)
	{
	    win_scale.x_min = crosshairs.first.x;
	    win_scale.x_max = crosshairs.second.x;
	}
	else
	{
	    win_scale.x_min = crosshairs.second.x;
	    win_scale.x_max = crosshairs.first.x;
	}
    }
    if (vertical)
    {
	if (crosshairs.first.y == crosshairs.second.y)
	{
	    /*  Can't do an infinite zoom  */
	    clear_crosshairs (w);
	    return;
	}
	viewimg_set_canvas_attributes (visible_worldcanvas,
				       VIEWIMG_ATT_AUTO_Y, FALSE,
				       VIEWIMG_ATT_END);
	if (crosshairs.first.y < crosshairs.second.y)
	{
	    win_scale.y_min = crosshairs.first.y;
	    win_scale.y_max = crosshairs.second.y;
	}
	else
	{
	    win_scale.y_min = crosshairs.second.y;
	    win_scale.y_max = crosshairs.first.y;
	}
    }
    clear_crosshairs (w);
    /*  Resize the canvas  */
    if ( !canvas_resize (visible_worldcanvas, &win_scale, FALSE) )
    {
	(void) fprintf (stderr, "Error resizing window\n");
    }
}   /*  End Function zoom  */

static void clear_crosshairs (ImageDisplayWidget w)
/*  This routine will clear the crosshairs.
    The routine returns nothing.
*/
{
    KWorldCanvas canvas;
    struct dual_crosshair_type *crosshairs;

    canvas = w->imageDisplay.visibleCanvas;
    crosshairs = &w->imageDisplay.crosshairs;
    if (crosshairs->num_crosshairs == 0)
    {
	return;
    }
    /*  Remove the first crosshair  */
    draw_crosshair (canvas, crosshairs->first.x, crosshairs->first.y, w);
    if (crosshairs->num_crosshairs > 1)
    {
	/*  Remove second crosshair  */
	draw_crosshair (canvas,
			crosshairs->second.x, crosshairs->second.y, w);
    }
    crosshairs->num_crosshairs = 0;
}   /*  End Function clear_crosshairs  */

static void show_crosshair_positions (ImageDisplayWidget w)
/*  This routine will display the crosshair position(s).
*/
{
    struct dual_crosshair_type crosshairs;

    crosshairs = w->imageDisplay.crosshairs;

    if (crosshairs.num_crosshairs < 1)
    {
	return;
    }
    if (crosshairs.num_crosshairs < 2)
    {
	/*  Only one crosshair to show position of  */
	(void) fprintf (stderr, "\nCrosshair: abscissa: %g  ordinate: %g\n",
			crosshairs.first.x, crosshairs.first.y);
	return;
    }
    /*  Two crosshairs to show position of  */
    (void) fprintf (stderr, "\nFirst crosshair: abscissa: %g  ordinate: %g\n",
		    crosshairs.first.x, crosshairs.first.y);
    (void) fprintf (stderr, "Second crosshair: abscissa: %g  ordinate: %g\n",
		    crosshairs.second.x, crosshairs.second.y);
    (void) fprintf (stderr, "Abscissa difference: %f\n",
		    fabs (crosshairs.second.x - crosshairs.first.x) );
    (void) fprintf (stderr, "Ordinate difference: %f\n",
		    fabs (crosshairs.second.y - crosshairs.first.y) );
}   /*  End Function show_crosshair_positions  */

static flag log_iscale_func (double *out, unsigned int out_stride,
			     double *inp, unsigned int inp_stride,
			     unsigned int num_values,
			     double i_min, double i_max,
			     void *info)
{
    unsigned int count;
    double tmp, factor;
    double toobig = TOOBIG;

    if (i_max <= 0.0)
    {
	(void) fprintf (stderr, "i_max: %e must be greater than 0\n", i_max);
	return (FALSE);
    }
    if (i_min <= 0.0) i_min = 1.0;
    if (i_min <= 0.0)
    {
	(void) fprintf (stderr, "i_min: %e must be greater than 0\n", i_min);
	return (FALSE);
    }
    factor = (i_max - i_min) / log (i_max / i_min);
    for (count = 0; count < num_values;
	 ++count, out += out_stride, inp += inp_stride)
    {
	if ( (tmp = *inp) >= toobig) continue;
	if (tmp <= i_min) continue;
	if (tmp >= i_max) continue;
	tmp = log (tmp / i_min) * factor + i_min;
	*out = tmp;
    }
    return (TRUE);
}   /*  End Function log_iscale_func  */

static void export_cbk (w, client_data, call_data)
/*  This is the export menu callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    ViewableImage vimage;
    Kcolourmap cmap;
    Channel channel;
    flag truecolour;
    unsigned int exportcode = *(int *) call_data;
    unsigned int hdim, vdim;
    unsigned int pseudo_index, red_index, green_index, blue_index;
    unsigned int cmap_size;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;
    double i_min, i_max;
    array_desc *arr_desc;
    CONST unsigned char *image;
    unsigned short *intensities;
    char fname[STRING_LENGTH];
    static char function_name[] = "ImageDisplayWidget::export_cbk";

    if (exportcode == EXPORT_POSTSCRIPT)
    {
	XtPopup (top->imageDisplay.pswinpopup, XtGrabNone);
	return;
    }
    if (top->imageDisplay.visibleCanvas == NULL)
    {
	(void) fprintf (stderr, "No visible canvas!\n");
	return;
    }
    if ( ( vimage = viewimg_get_active (top->imageDisplay.visibleCanvas) )
	== NULL )
    {
	(void) fprintf (stderr, "No visible image!\n");
	return;
    }
    viewimg_get_attributes (vimage,
			    VIEWIMG_VATT_TRUECOLOUR, &truecolour,
			    VIEWIMG_VATT_ARRAY_DESC, &arr_desc,
			    VIEWIMG_VATT_SLICE, &image,
			    VIEWIMG_VATT_HDIM, &hdim,
			    VIEWIMG_VATT_VDIM, &vdim,
			    VIEWIMG_VATT_END);
    if (truecolour)
    {
	viewimg_get_attributes (vimage,
				VIEWIMG_VATT_RED_INDEX, &red_index,
				VIEWIMG_VATT_GREEN_INDEX, &green_index,
				VIEWIMG_VATT_BLUE_INDEX, &blue_index,
				VIEWIMG_VATT_END);
    }
    else
    {
	canvas_get_attributes (top->imageDisplay.visibleCanvas,
			       CANVAS_ATT_VALUE_MIN, &i_min,
			       CANVAS_ATT_VALUE_MAX, &i_max,
			       CANVAS_ATT_END);
	viewimg_get_attributes (vimage,
				VIEWIMG_VATT_PSEUDO_INDEX, &pseudo_index,
				VIEWIMG_VATT_END);
	cmap = canvas_get_cmap (top->imageDisplay.visibleCanvas);
	if ( ( intensities = kcmap_get_rgb_values (cmap, &cmap_size) )
	    == NULL )
	{
	    return;
	}
    }
    switch (exportcode)
    {
      case EXPORT_PPM:
	(void) fprintf (stderr, "PPM not supported yet\n");
	break;
      case EXPORT_SUNRAS:
	(void) sprintf (fname, "%s.ras", top->imageDisplay.imageName);
	if ( ( channel = ch_open_file (fname, "w") ) == NULL )
	{
	    if (!truecolour) m_free ( (char *) intensities );
	    return;
	}
	if (truecolour)
	{
	    if ( !foreign_sunras_write_rgb
		(channel,
		 image + ds_get_element_offset (arr_desc->packet, red_index),
		 image + ds_get_element_offset (arr_desc->packet, green_index),
		 image + ds_get_element_offset (arr_desc->packet, blue_index),
		 arr_desc->offsets[hdim], arr_desc->offsets[vdim],
		 arr_desc->dimensions[hdim]->length,
		 arr_desc->dimensions[vdim]->length) )
	    {
		(void) ch_close (channel);
		(void) unlink (fname);
		return;
	    }
	}
	else
	{
	    if ( !foreign_sunras_write_pseudo
		(channel,
		 (CONST char *) image +
		 ds_get_element_offset (arr_desc->packet,pseudo_index),
		 arr_desc->packet->element_types[pseudo_index],
		 arr_desc->offsets[hdim], arr_desc->offsets[vdim],
		 arr_desc->dimensions[hdim]->length,
		 arr_desc->dimensions[vdim]->length,
		 intensities, intensities + 1, intensities + 2, cmap_size, 3,
		 i_min, i_max) )
	    {
		m_free ( (char *) intensities );
		(void) ch_close (channel);
		(void) unlink (fname);
		return;
	    }
	    m_free ( (char *) intensities );
	}
	(void) ch_close (channel);
	(void) fprintf (stderr, "Wrote file: \"%s\"\n", fname);
	break;
      default:
	(void) fprintf (stderr, "Illegal export code: %u\n", exportcode);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function export_cbk   */
#ifdef dummy
static char shellTranslations[] =
  "#override \n\
   <KeyPress>:          GlobalKeyPress()";

static XtActionsRec shell_actions[] = {
  {"GlobalKeyPress",                   GlobalKeyPress},
};

static void show_fullscreen (ImageDisplayWidget w)
/*  [PURPOSE] This routine will show the fullscreen canvas.
    <w> The ImageDisplay widget.
    [RETURNS] Nothing.
*/
{
    Widget shell;
    XtTranslations translations;
    Screen *screen;

    if (w->imageDisplay.override_shell == NULL)
    {
	XtAddActions ( shell_actions, XtNumber (shell_actions) );
	translations = XtParseTranslationTable (shellTranslations);
	screen = w->core.screen;
	/*  Create the override shell  */
	shell = XtVaCreatePopupShell ("fullscreenPopup",
				      overrideShellWidgetClass, (Widget ) w,
				      XtNx, 0,
				      XtNy, 0,
				      XtNwidth, WidthOfScreen (screen),
				      XtNheight, HeightOfScreen (screen),
				      XtNborderWidth, 0,
				      NULL);
	w->imageDisplay.override_shell = shell;
	XtOverrideTranslations (shell, translations);
	XtRealizeWidget (shell);
	XtPopup (shell, XtGrabNone);
    }
}   /*  End Function show_fullscreen  */

static void GlobalKeyPress (Widget w, XEvent *event)
{
    exit (1);
}
#endif

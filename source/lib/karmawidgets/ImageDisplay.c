/*LINTLIBRARY*/
/*  ImageDisplay.c

    This code provides an image display widget for Xt.

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

    Updated by      Richard Gooch   28-DEC-1995: Added verbose resource.

    Updated by      Richard Gooch   21-FEB-1996: Created
  <XkwImageDisplayRefresh> routine.

    Updated by      Richard Gooch   26-FEB-1996: Commented out registering of
  crosshair event consumers for stereo canvases, since the events are not
  properly dealt with yet.

    Updated by      Richard Gooch   17-MAR-1996: Made use of <xtmisc_popup_cbk>

    Updated by      Richard Gooch   21-APR-1996: Made use of
  XkwNsimpleColourbar resource for Cmapwinpopup widget.

    Updated by      Richard Gooch   29-APR-1996: Added control for DirectColour
  colourmaps.

    Updated by      Richard Gooch   3-MAY-1996: Switched to KtoggleWidget.

    Updated by      Richard Gooch   11-MAY-1996: Added raise button and keyed
  right mouse button press to lower fullscreen canvas.

    Updated by      Richard Gooch   12-MAY-1996: Made use of ZoomPolicy widget.

    Updated by      Richard Gooch   16-MAY-1996: Removed <iscale_cbk>.

    Updated by      Richard Gooch   19-MAY-1996: Changed from using window
  scale structure to using <canvas_set_attributes>.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   4-JUN-1996: Switched to left-right
  bottom-top co-ordinate specification instead of min-max x and y. Made use of
  colourmap for DirectColour visual (if needed) when exporting Sun Rasterfiles.
  Added export of PPM files.

    Updated by      Richard Gooch   7-JUN-1996: Register image name with
  PostScript widget when new name is given, no longer register name inside
  <postscript_cbk>.

    Updated by      Richard Gooch   9-JUN-1996: Added magnifier canvases.

    Updated by      Richard Gooch   14-JUN-1996: Added XkwNnumTrackLabels
  resource.

    Updated by      Richard Gooch   28-JUN-1996: Made use of XkwNautoIncrement
  resource for PostScript widget.

    Updated by      Richard Gooch   21-JUL-1996: Removed unnecessary world
  canvas refresh function.

    Updated by      Richard Gooch   6-AUG-1996: Added export of entire dataset
  in FITS format.

    Updated by      Richard Gooch   7-AUG-1996: Added export of entire dataset
  in Miriad Image format.

    Updated by      Richard Gooch   8-AUG-1996: Added export of entire dataset
  in GIPSY format.

    Updated by      Richard Gooch   15-AUG-1996: Made use of improved
  dialogpopup widget when saving foreign data.

    Updated by      Richard Gooch   15-SEP-1996: Made use of new <kwin_xutil_*>
  package.

    Updated by      Richard Gooch   28-SEP-1996: Added export of entire dataset
  in Karma format.

    Updated by      Richard Gooch   1-OCT-1996: Move export functionality into
  ExportMenu widget.

    Updated by      Richard Gooch   21-OCT-1996: Update magnifierPseudoCanvas
  in <region_cbk>.

    Last updated by Richard Gooch   22-OCT-1996: Added statistics reporting.


*/

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#define X11
#include <X11/IntrinsicP.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/XawInit.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Box.h>
#include <karma.h>
#define NEW_WIN_SCALE
#include <k_event_codes.h>
#include <karma_viewimg.h>
#include <karma_xtmisc.h>
#include <karma_dsxfr.h>
#include <karma_kwin.h>
#include <karma_xc.h>
#include <karma_ch.h>
#include <karma_st.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>
#include <Xkw/ImageDisplayP.h>
#include <Xkw/MultiCanvas.h>
#include <Xkw/Filepopup.h>
#include <Xkw/Cmapwinpopup.h>
#include <Xkw/Dialogpopup.h>
#include <Xkw/Postscript.h>
#include <Xkw/Dataclip.h>
#include <Xkw/ChoiceMenu.h>
#include <Xkw/ZoomPolicy.h>
#include <Xkw/ExportMenu.h>


#define DEFAULT_COLOURMAP_NAME "Greyscale1"

STATIC_FUNCTION (void ImageDisplay__Initialise, (Widget request, Widget new) );
STATIC_FUNCTION (Boolean ImageDisplay__SetValues,
		 (Widget current, Widget request, Widget new) );
STATIC_FUNCTION (void quit_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );
STATIC_FUNCTION (void ImageDisplay__canvas_realise_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void zoom_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );
STATIC_FUNCTION (void zoom_policy_cbk, (Widget w, XtPointer client_data,
					XtPointer call_data) );
STATIC_FUNCTION (void region_cbk, (Widget w, XtPointer client_data,
				   XtPointer call_data) );
STATIC_FUNCTION (flag canvas_event_handler,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info,
		  void **f_info, double x_lin, double y_lin) );
STATIC_FUNCTION (void unzoom, (ImageDisplayWidget w) );
STATIC_FUNCTION (void colourmap_cbk, (Widget w, XtPointer client_data,
				      XtPointer call_data) );
STATIC_FUNCTION (void raise_cbk, (Widget w, XtPointer client_data,
				  XtPointer call_data) );
STATIC_FUNCTION (Kcolourmap get_colourmap,
		 (ImageDisplayWidget w, Visual *visual,
		  unsigned int visual_type, Colormap xcmap) );
STATIC_FUNCTION (Widget handle_canvas,
		 (ImageDisplayWidget w, Widget canvas,
		  flag *one_to_map, Widget form, Widget reference) );


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
    {XkwNmagnifierPseudoColourCanvas, XkwCWorldCanvas, XtRPointer,
     sizeof (XtPointer), offset (magnifierPseudoCanvas), XtRImmediate, NULL},
    {XkwNmagnifierDirectColourCanvas, XkwCWorldCanvas, XtRPointer,
     sizeof (XtPointer), offset (magnifierDirectCanvas), XtRImmediate, NULL},
    {XkwNmagnifierTrueColourCanvas, XkwCWorldCanvas, XtRPointer,
     sizeof (XtPointer), offset (magnifierTrueCanvas), XtRImmediate, NULL},
    {XkwNmagnifierVisibleCanvas, XkwCWorldCanvas, XtRPointer,
     sizeof (XtPointer), offset (magnifierVisibleCanvas), XtRImmediate, NULL},
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
    {XkwNautoIntensityScale, XkwCAutoIntensityScale, XtRBool, sizeof (Bool),
     offset (autoIntensityScale), XtRImmediate, (XtPointer) True},
    {XkwNnumTrackLabels, XkwCNumTrackLabels, XtRCardinal, sizeof (Cardinal),
     offset (numTrackLabels), XtRImmediate, (XtPointer) 0},
    {XkwNverbose, XkwCVerbose, XtRBool, sizeof (Bool),
     offset (verbose), XtRImmediate, False},
#undef offset
};

#define ZOOMCODE_UNZOOM     (unsigned int) 0
#define ZOOMCODE_INTENSITY  (unsigned int) 1
#define NUM_ZOOM_CHOICES 2

static char *zoom_choices[NUM_ZOOM_CHOICES] =
{
    "Unzoom", "Intensity",
};

#define COLOURMAP_PSEUDOCOLOUR (unsigned int) 0
#define COLOURMAP_DIRECTCOLOUR (unsigned int) 1
#define NUM_COLOURMAP_CHOICES 2

static char *colourmap_choices[NUM_COLOURMAP_CHOICES] =
{
    "PseudoColour (8 bit)", "DirectColour (24 bit)"
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
	(XtInitProc) ImageDisplay__Initialise, /*  initialise             */
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
	(XtSetValuesFunc) ImageDisplay__SetValues, /*  set_values  */
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

static void ImageDisplay__Initialise (Widget Request, Widget New)
{
    flag one_to_map = TRUE;
    /*ImageDisplayWidget request = (ImageDisplayWidget) Request;*/
    ImageDisplayWidget new = (ImageDisplayWidget) New;
    int canvas_types, count, vertical_distance;
    Widget filewin, files_btn, app_box, m_cnv, w;
    Widget colourmap_btn = NULL;
    Widget quit_btn, menu_btn, animate_btn, raise_btn, mag_open_btn;
    Widget izoomwinpopup;
    Widget animatepopup = NULL;  /*  Initialised to keep compiler happy  */
    Widget shell = NULL;         /*  Initialised to keep compiler happy  */
    Widget zoom_policy_popup;
    Widget pseudo_cnvs, direct_cnvs, true_cnvs;
    Widget left_widget, top_widget;
    Widget mag_popup, mag_form, mag_close_btn;
    Display *dpy;
    Screen *screen;
    Visual *pseudocolour_visual, *directcolour_visual;
    char txt[STRING_LENGTH];
    /*static char function_name[] = "ImageDisplayWidget::Initialise";*/

    if (new->imageDisplay.imageName == NULL)
    {
	new->imageDisplay.imageName = def_image_name;
    }
    new->imageDisplay.override_shell = NULL;
    new->imageDisplay.visibleCanvas = NULL;
    new->imageDisplay.pseudo_cmap = NULL;
    new->imageDisplay.direct_cmap = NULL;
    new->imageDisplay.cmapwinpopup_psuedo = NULL;
    new->imageDisplay.cmapwinpopup_direct = NULL;
    new->imageDisplay.cmap_btn = NULL;
    new->imageDisplay.set_canvases = FALSE;
    new->imageDisplay.magnifier_pseudo_canvas = NULL;
    new->imageDisplay.magnifier_direct_canvas = NULL;
    new->imageDisplay.magnifier_true_canvas = NULL;
    dpy = XtDisplay (New);
    screen = XtScreen (New);
    /*  Create as many popups as possible first. Note that the colourmap
	control popup(s) cannot be created yet since they require a visual
	structure which cannot be predicted until their respective canvas
	widgets are realised. It is possible to check which visuals are
	available for each visual type and then setup the colourmap popup
	button/menu.
	*/
    filewin = XtVaCreatePopupShell ("filewinPopup", filepopupWidgetClass, New,
				    XtNtitle, "kview_2d File Selector",
				    XtNx, 0,
				    XtNy, 0,
				    XkwNautoPopdown, True,
				    NULL);
    new->imageDisplay.filepopup = filewin;
    izoomwinpopup = XtVaCreatePopupShell ("izoomwinpopup",
					  dataclipWidgetClass, New,
					  XtNtitle, "Intensity Zoom",
					  XkwNshowBlankControl, TRUE,
					  NULL);
    new->imageDisplay.izoomwinpopup = izoomwinpopup;
    XtAddCallback (izoomwinpopup, XkwNregionCallback, region_cbk, New);
    if (new->imageDisplay.enableAnimation)
    {
	animatepopup = XtVaCreatePopupShell ("animatepopup",
					     animateControlWidgetClass, New,
					     XtNtitle, "Animation Control",
					     NULL);
	new->imageDisplay.animatepopup = animatepopup;
    }
    zoom_policy_popup = XtVaCreatePopupShell ("zoomPolicyPopup",
					      zoomPolicyWidgetClass, New,
					      XkwNautoIntensityScale,
					      new->imageDisplay.autoIntensityScale,
					      NULL);
    new->imageDisplay.zoom_policy_popup = zoom_policy_popup;
    /*  Now create the widgets for the main window  */
    files_btn = XtVaCreateManagedWidget ("filesButton", commandWidgetClass,New,
					 XtNlabel, "Files",
					 NULL);
    XtAddCallback (files_btn, XtNcallback, xtmisc_popup_cbk, filewin);
    /*  A bit of buggerising is required to setup the colourmap popup button or
	menu  */
    kwin_xutil_get_visuals (screen, &pseudocolour_visual, NULL,
			    &directcolour_visual);
    if ( (pseudocolour_visual == NULL) && (directcolour_visual == NULL) )
    {
	left_widget = files_btn;
    }
    else if ( (pseudocolour_visual != NULL) && (directcolour_visual != NULL) )
    {
	colourmap_btn = XtVaCreateManagedWidget
	    ("menuButton", choiceMenuWidgetClass, New,
	     XtNlabel, "Colourmap",
	     XkwNmenuTitle, "Colourmap Menu",
	     XtNfromHoriz, files_btn,
	     XtNmenuName, "colourmapMenu",
	     XkwNnumItems, NUM_COLOURMAP_CHOICES,
	     XkwNitemStrings, colourmap_choices,
	     NULL);
	XtAddCallback (colourmap_btn, XkwNselectCallback, colourmap_cbk, New);
	left_widget = colourmap_btn;
    }
    else
    {
	colourmap_btn = XtVaCreateManagedWidget
	    ("button", commandWidgetClass, New,
	     XtNlabel, "Colourmap",
	     XtNfromHoriz, files_btn,
	     NULL);
	new->imageDisplay.cmap_btn = colourmap_btn;
	left_widget = colourmap_btn;
    }
    menu_btn = XtVaCreateManagedWidget ("menuButton", choiceMenuWidgetClass,
					New,
					XtNlabel, "Zoom",
					XkwNmenuTitle, "Zoom Menu",
					XtNfromHoriz, left_widget,
					XtNmenuName, "zoomMenu",
					XkwNnumItems, NUM_ZOOM_CHOICES,
					XkwNitemStrings, zoom_choices,
					NULL);
    XtAddCallback (menu_btn, XkwNselectCallback, zoom_cbk, New);
    left_widget = menu_btn;
    left_widget = XtVaCreateManagedWidget ("zoomPolicy", commandWidgetClass,
					   New,
					   XtNlabel, "Zoom Policy",
					   XtNfromHoriz, left_widget,
					   NULL);
    XtAddCallback (left_widget, XtNcallback, xtmisc_popup_cbk,
		   zoom_policy_popup);
    XtAddCallback (left_widget, XtNcallback, zoom_policy_cbk, New);
    menu_btn = XtVaCreateManagedWidget ("exportMenu", exportMenuWidgetClass,
					New,
					XtNfromHoriz, left_widget,
					NULL);
    new->imageDisplay.export_menu = menu_btn;
    left_widget = menu_btn;
    if (new->imageDisplay.enableAnimation &&
	new->imageDisplay.showAnimateButton)
    {
	animate_btn = XtVaCreateManagedWidget ("button", commandWidgetClass,
					       New,
					       XtNlabel, "Movie",
					       XtNfromHoriz, left_widget,
					       NULL);
	left_widget = animate_btn;
	XtAddCallback (animate_btn, XtNcallback, xtmisc_popup_cbk,
		       animatepopup);
    }
    if (new->imageDisplay.fullscreen)
    {
	/*  Need fullscreen canvas: create the raise button  */
	raise_btn = XtVaCreateManagedWidget ("button", commandWidgetClass, New,
					     XtNlabel, "Raise",
					     XtNfromHoriz, left_widget,
					     NULL);
	left_widget = raise_btn;
	XtAddCallback (raise_btn, XtNcallback, raise_cbk, new);
    }
    left_widget = XtVaCreateManagedWidget ("button", commandWidgetClass, New,
					   XtNlabel, "Magnifier",
					   XtNfromHoriz, left_widget,
					   NULL);
    mag_open_btn = left_widget;
    if (new->imageDisplay.showQuitButton)
    {
	quit_btn = XtVaCreateManagedWidget ("quit", commandWidgetClass, New,
					    XtNlabel, "Quit",
					    XtNfromHoriz, left_widget,
					    NULL);
	XtAddCallback (quit_btn, XtNcallback, quit_cbk, NULL);
	left_widget = quit_btn;
    }
    app_box = XtVaCreateManagedWidget ("applicationBox", boxWidgetClass, New,
				       XtNfromVert, files_btn,
				       XtNwidth, 516,
				       XtNheight, 0,
				       XtNborderWidth, 0,
				       XtNleft, XtChainLeft,
				       XtNright, XtChainRight,
				       XtNorientation, XtorientVertical,
				       XtNhorizDistance, 0,
				       XtNvertDistance, 0,
				       NULL);
    top_widget = app_box;
    vertical_distance = 0;
    for (count = 0; count < new->imageDisplay.numTrackLabels; ++count)
    {
	sprintf (txt, "trackLabel%d", count);
	w = XtVaCreateManagedWidget (txt, labelWidgetClass, New,
				     XtNlabel, "Track Output",
				     XtNwidth, 512,
				     XtNfromVert, top_widget,
				     XtNvertDistance, vertical_distance,
				     NULL);
	top_widget = w;
	vertical_distance = new->form.default_spacing;
    }
    /*  Create the canvases  */
    canvas_types = XkwCanvasTypePseudoColour | XkwCanvasTypeDirectColour;
    canvas_types |= XkwCanvasTypeTrueColour | XkwCanvasTypeStereo;
    if (new->imageDisplay.fullscreen)
    {
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
	m_cnv = XtVaCreateManagedWidget ("multiCanvas",
					 multiCanvasWidgetClass, shell,
					 XkwNcanvasTypes, canvas_types,
					 XtNwidth, WidthOfScreen (screen),
					 XtNheight, HeightOfScreen (screen),
					 NULL);
    }
    else
    {
	m_cnv = XtVaCreateManagedWidget ("multiCanvas", multiCanvasWidgetClass,
					 New,
					 XtNfromVert, top_widget,
					 XtNwidth, 512,
					 XtNheight, 512,
					 XkwNcanvasTypes, canvas_types,
					 NULL);
    }
    new->imageDisplay.multi_canvas = m_cnv;
    /*  Create magnifier window  */
    mag_popup = XtVaCreatePopupShell ("magnifierShell",
				      topLevelShellWidgetClass, New,
				      NULL);
    XtAddCallback (mag_open_btn, XtNcallback, xtmisc_popup_cbk, mag_popup);
    mag_form = XtVaCreateManagedWidget ("topform", formWidgetClass, mag_popup,
					NULL);
    mag_close_btn = XtVaCreateManagedWidget ("closeButton", commandWidgetClass,
					     mag_form,
					     XtNlabel, "Close",
					     XtNwidth, 126,
					     XtNtop, XtChainTop,
					     XtNbottom, XtChainTop,
					     XtNleft, XtChainLeft,
					     XtNright, XtChainRight,
					     NULL);
    XtAddCallback (mag_close_btn, XtNcallback, xtmisc_popdown_cbk, mag_popup);
    /*  Setup for mono canvases  */
    pseudo_cnvs = XtNameToWidget (m_cnv, "pseudoColourCanvas");
    direct_cnvs = XtNameToWidget (m_cnv, "directColourCanvas");
    true_cnvs = XtNameToWidget (m_cnv, "trueColourCanvas");
    new->imageDisplay.magnifier_pseudo_canvas =
	handle_canvas (new, pseudo_cnvs, &one_to_map, mag_form, mag_close_btn);
    new->imageDisplay.magnifier_direct_canvas =
	handle_canvas (new, direct_cnvs, &one_to_map, mag_form, mag_close_btn);
    new->imageDisplay.magnifier_true_canvas =
	handle_canvas (new, true_cnvs, &one_to_map, mag_form, mag_close_btn);
    /*  Setup for stereo canvases  */
    pseudo_cnvs = XtNameToWidget (m_cnv, "pseudoColourStereoCanvas");
    direct_cnvs = XtNameToWidget (m_cnv, "directColourStereoCanvas");
    true_cnvs = XtNameToWidget (m_cnv, "trueColourStereoCanvas");
    one_to_map = FALSE;
    handle_canvas (new, pseudo_cnvs, &one_to_map, NULL, NULL);
    handle_canvas (new, direct_cnvs, &one_to_map, NULL, NULL);
    handle_canvas (new, true_cnvs, &one_to_map, NULL, NULL);
    if (new->imageDisplay.fullscreen)
    {
	XtRealizeWidget (shell);
	XtPopup (shell, XtGrabNone);
    }
    XtRealizeWidget (mag_popup);
}   /*  End Function Initialise  */

static Boolean ImageDisplay__SetValues (Widget Current, Widget Request,
					Widget New)
{
    KWorldCanvas old_wc, new_wc;
    ImageDisplayWidget current = (ImageDisplayWidget) Current;
    /*ImageDisplayWidget request = (ImageDisplayWidget) Request;*/
    ImageDisplayWidget new = (ImageDisplayWidget) New;
    Widget multi_canvas;
    Widget old_cnv = NULL;      /*  Initialised to keep compiler happy  */
    Widget new_cnv = NULL;      /*  Initialised to keep compiler happy  */
    Widget mag_old_cnv = NULL;  /*  Initialised to keep compiler happy  */
    Widget mag_new_cnv = NULL;  /*  Initialised to keep compiler happy  */
    Widget w;
    static char function_name[] = "ImageDisplayWidget::SetValues";

    if (new->imageDisplay.cmapSize != current->imageDisplay.cmapSize)
    {
	if ( !kcmap_change (new->imageDisplay.pseudo_cmap, NULL,
			    (unsigned int) new->imageDisplay.cmapSize, TRUE) )
	{
	    fprintf (stderr, "Error resizing colourmap to: %d cells\n",
			    new->imageDisplay.cmapSize);
	    new->imageDisplay.cmapSize = current->imageDisplay.cmapSize;
	}
    }
    if (new->imageDisplay.imageName != current->imageDisplay.imageName)
    {
	if ( (current->imageDisplay.imageName != NULL) &&
	    (current->imageDisplay.imageName != def_image_name) )
	{
	    /*  There was an old name and it's not the default: free it  */
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
	w = XtNameToWidget (new->imageDisplay.export_menu, "savePopup");
	XtVaSetValues (w,
		       XkwNdefaultName, new->imageDisplay.imageName,
		       NULL);
	w = XtNameToWidget (new->imageDisplay.export_menu,
			    "postscriptwinpopup");
	XkwPostscriptRegisterImageAndName (w, NULL,
					   new->imageDisplay.imageName);
	
    }
    /*  Deal with requests to change active canvas  */
    old_wc = current->imageDisplay.visibleCanvas;
    new_wc = new->imageDisplay.visibleCanvas;
    /*  Prevent direct changing of magnifier canvas  */
    new->imageDisplay.magnifierVisibleCanvas =
	current->imageDisplay.magnifierVisibleCanvas;
    multi_canvas = new->imageDisplay.multi_canvas;
    if (new_wc != old_wc)
    {
	if (new_wc == NULL)
	{
	    fprintf (stderr, "NULL visibleCanvas resource!\n");
	    a_prog_bug (function_name);
	}
	if (old_wc == NULL)
	{
	    old_cnv = NULL;
	    mag_old_cnv = NULL;
	}
	else if (old_wc == new->imageDisplay.pseudoCanvas)
	{
	    old_cnv = XtNameToWidget (multi_canvas, "pseudoColourCanvas");
	    mag_old_cnv = new->imageDisplay.magnifier_pseudo_canvas;
	}
	else if (old_wc == new->imageDisplay.directCanvas)
	{
	    old_cnv = XtNameToWidget (multi_canvas, "directColourCanvas");
	    mag_old_cnv = new->imageDisplay.magnifier_direct_canvas;
	}
	else if (old_wc == new->imageDisplay.trueCanvas)
	{
	    old_cnv = XtNameToWidget (multi_canvas, "trueColourCanvas");
	    mag_old_cnv = new->imageDisplay.magnifier_true_canvas;
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
	    fprintf (stderr, "Visible canvas: %p unknown!\n", old_wc);
	    a_prog_bug (function_name);
	}
	if (new_wc == new->imageDisplay.pseudoCanvas)
	{
	    new_cnv = XtNameToWidget (multi_canvas, "pseudoColourCanvas");
	    mag_new_cnv = new->imageDisplay.magnifier_pseudo_canvas;
	    new->imageDisplay.magnifierVisibleCanvas =
		new->imageDisplay.magnifierPseudoCanvas;
	}
	else if (new_wc == new->imageDisplay.directCanvas)
	{
	    new_cnv = XtNameToWidget (multi_canvas, "directColourCanvas");
	    mag_new_cnv = new->imageDisplay.magnifier_direct_canvas;
	    new->imageDisplay.magnifierVisibleCanvas =
		new->imageDisplay.magnifierDirectCanvas;
	}
	else if (new_wc == new->imageDisplay.trueCanvas)
	{
	    new_cnv = XtNameToWidget (multi_canvas, "trueColourCanvas");
	    mag_new_cnv = new->imageDisplay.magnifier_true_canvas;
	    new->imageDisplay.magnifierVisibleCanvas =
		new->imageDisplay.magnifierTrueCanvas;
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
	    fprintf (stderr, "Visible canvas: %p unknown!\n", new_wc);
	    a_prog_bug (function_name);
	}
	if (old_cnv == new_cnv) return False;
	if (old_cnv != NULL) XtUnmapWidget (old_cnv);
	XtMapWidget (new_cnv);
	if (mag_old_cnv != NULL) XtUnmapWidget (mag_old_cnv);
	if (mag_new_cnv == NULL)
	    new->imageDisplay.magnifierVisibleCanvas = NULL;
	else XtMapWidget (mag_new_cnv);
	XtVaSetValues (new->imageDisplay.export_menu,
		       XkwNworldCanvas, new_wc,
		       NULL);
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

static void ImageDisplay__canvas_realise_cbk (Widget w, XtPointer client_data,
					      XtPointer call_data)
/*  [SUMMARY] Canvas realise callback.
    [PURPOSE] This routine is called when a Canvas widget is realised. A
    Kcolourmap object is created if required, then the KWorldCanvas is created
    and initialised for the [<viewimg>] package.
    <w> The Canvas widget that has just been realised.
    <client_data> The ImageDisplay widget.
    <call_data> The pixel canvas(es).
    [RETURNS] Nothing.
*/
{
    Kcolourmap kcmap = NULL;        /*  Initialised to keep compiler happy  */
    flag verbose;
    Boolean mappable;
    KPixCanvas pixcanvas;
    KPixCanvas *stereopixcanvases = NULL;/*Initialised to keep compiler happy*/
    int stereo_mode;
    unsigned int visual_type;
    unsigned long background_pixel;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;
    Colormap xcmap;
    Widget multicanvas;
    Widget pseudo_cnvs, direct_cnvs, true_cnvs;
    KWorldCanvas *worldcanvas = NULL;
    KWorldCanvas *leftworldcanvas = NULL;
    KWorldCanvas *rightworldcanvas = NULL;
    Visual *visual;
    struct win_scale_type win_scale;
    static char function_name[] = "ImageDisplayWidget::canvas_realise_cbk";

    verbose = top->imageDisplay.verbose;
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
		   XtNbackground, &background_pixel,
		   XtNmappedWhenManaged, &mappable,
		   XtNvisual, &visual,
		   NULL);
    if (verbose)
    {
	fprintf ( stderr, "%s: visual: %p visualID: %#lx\n",
			function_name, visual, XVisualIDFromVisual (visual) );
    }
    /*  Visual-specific work  */
    if (w == pseudo_cnvs)
    {
	if (visual_type != KWIN_VISUAL_PSEUDOCOLOUR)
	{
	    fprintf (stderr,"pseudo_canvas not PseudoColour visual!\n");
	    a_prog_bug (function_name);
	}
	kcmap = get_colourmap (top, visual, visual_type, xcmap);
	worldcanvas = &top->imageDisplay.pseudoCanvas;
	leftworldcanvas = &top->imageDisplay.pseudoCanvasLeft;
	rightworldcanvas = &top->imageDisplay.pseudoCanvasRight;
    }
    else if (w == direct_cnvs)
    {
	if (visual_type != KWIN_VISUAL_DIRECTCOLOUR)
	{
	    fprintf (stderr,"direct_canvas not DirectColour visual!\n");
	    a_prog_bug (function_name);
	}
	get_colourmap (top, visual, visual_type, xcmap);
	kcmap = NULL;
	worldcanvas = &top->imageDisplay.directCanvas;
	leftworldcanvas = &top->imageDisplay.directCanvasLeft;
	rightworldcanvas = &top->imageDisplay.directCanvasRight;
    }
    else if (w == true_cnvs)
    {
	if (visual_type != KWIN_VISUAL_TRUECOLOUR)
	{
	    fprintf (stderr, "true_canvas not TrueColour visual!\n");
	    a_prog_bug (function_name);
	}
	kcmap = NULL;
	worldcanvas = &top->imageDisplay.trueCanvas;
	leftworldcanvas = &top->imageDisplay.trueCanvasLeft;
	rightworldcanvas = &top->imageDisplay.trueCanvasRight;
    }
    else
    {
	fprintf (stderr, "Bad canvas passed: %p\n", w);
	a_prog_bug (function_name);
    }
    /*  Create world canvas(es) and initialise for [<viewimg>] package  */
    /*  First initialise win_scale since all data is wiped here  */
    canvas_init_win_scale (&win_scale, K_WIN_SCALE_MAGIC_NUMBER);
    win_scale.blank_pixel = background_pixel;
    /*  Create the main world canvas (with dummy world min and max)  */
    win_scale.conv_type = CONV1_REAL;
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
	if (mappable)
	{
	    top->imageDisplay.visibleCanvas = *worldcanvas;
	    XtVaSetValues (top->imageDisplay.export_menu,
			   XkwNworldCanvas, top->imageDisplay.visibleCanvas,
			   NULL);
	}
	viewimg_create_drag_and_zoom_interface (*worldcanvas);
	canvas_register_position_event_func (*worldcanvas,
					     canvas_event_handler,
					     (void *) top);
	viewimg_init (*worldcanvas);
	if (visual_type == KWIN_VISUAL_PSEUDOCOLOUR)
	{
	    viewimg_register_position_event_func (*worldcanvas,
						  viewimg_statistics_position_func,
						  NULL);
	}
	viewimg_set_canvas_attributes (*worldcanvas,
				       VIEWIMG_ATT_MAINTAIN_ASPECT, TRUE,
				       VIEWIMG_ATT_ALLOW_TRUNCATION, TRUE,
				       VIEWIMG_ATT_INT_X, TRUE,
				       VIEWIMG_ATT_INT_Y, TRUE,
				       VIEWIMG_ATT_AUTO_V,
				       top->imageDisplay.autoIntensityScale,
				       VIEWIMG_ATT_END);
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
	if (mappable)
	{
	    top->imageDisplay.visibleCanvas = *leftworldcanvas;
	    XtVaSetValues (top->imageDisplay.export_menu,
			   XkwNworldCanvas, top->imageDisplay.visibleCanvas,
			   NULL);
	}
	canvas_register_position_event_func (*leftworldcanvas,
					     canvas_event_handler,
					     (void *) top);
	viewimg_init (*leftworldcanvas);
	viewimg_set_canvas_attributes (*leftworldcanvas,
				       VIEWIMG_ATT_MAINTAIN_ASPECT, TRUE,
				       VIEWIMG_ATT_ALLOW_TRUNCATION, TRUE,
				       VIEWIMG_ATT_AUTO_V,
				       top->imageDisplay.autoIntensityScale,
				       VIEWIMG_ATT_END);
	canvas_register_position_event_func (*rightworldcanvas,
					     canvas_event_handler,
					     (void *) top);
	viewimg_init (*rightworldcanvas);
	viewimg_set_canvas_attributes (*rightworldcanvas,
				       VIEWIMG_ATT_MAINTAIN_ASPECT, TRUE,
				       VIEWIMG_ATT_ALLOW_TRUNCATION, TRUE,
				       VIEWIMG_ATT_AUTO_V,
				       top->imageDisplay.autoIntensityScale,
				       VIEWIMG_ATT_END);
    }
}   /*  End Function canvas_realise_cbk   */

static void ImageDisplay__magnifier_canvas_realise_cbk (Widget w,
							XtPointer client_data,
							XtPointer call_data)
/*  [SUMMARY] Canvas realise callback.
    [PURPOSE] This routine is called when a Canvas widget is realised. A
    Kcolourmap object is created if required, then the KWorldCanvas is created
    and initialised for the [<viewimg>] package.
    <w> The Canvas widget that has just been realised.
    <client_data> The ImageDisplay widget.
    <call_data> The pixel canvas(es).
    [RETURNS] Nothing.
*/
{
    Kcolourmap kcmap = NULL;        /*  Initialised to keep compiler happy  */
    flag verbose;
    Boolean mappable;
    KPixCanvas pixcanvas;
    unsigned int visual_type;
    unsigned long background_pixel;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;
    Colormap xcmap;
    KWorldCanvas *worldcanvas = NULL;
    Visual *visual;
    struct win_scale_type win_scale;
    static char function_name[] = "ImageDisplayWidget::magnifier_canvas_realise_cbk";

    verbose = top->imageDisplay.verbose;
    pixcanvas = (KPixCanvas) call_data;
    kwin_get_attributes (pixcanvas,
			 KWIN_ATT_VISUAL, &visual_type,
			 KWIN_ATT_END);
    XtVaGetValues (w,
		   XtNcolormap, &xcmap,
		   XtNbackground, &background_pixel,
		   XtNmappedWhenManaged, &mappable,
		   XtNvisual, &visual,
		   NULL);
    if (verbose)
    {
	fprintf ( stderr, "%s: visual: %p visualID: %#lx\n",
			function_name, visual, XVisualIDFromVisual (visual) );
    }
    /*  Visual-specific work  */
    if (w == top->imageDisplay.magnifier_pseudo_canvas)
    {
	if (visual_type != KWIN_VISUAL_PSEUDOCOLOUR)
	{
	    fprintf (stderr,"pseudo_canvas not PseudoColour visual!\n");
	    a_prog_bug (function_name);
	}
	kcmap = get_colourmap (top, visual, visual_type, xcmap);
	worldcanvas = &top->imageDisplay.magnifierPseudoCanvas;
    }
    else if (w == top->imageDisplay.magnifier_direct_canvas)
    {
	if (visual_type != KWIN_VISUAL_DIRECTCOLOUR)
	{
	    fprintf (stderr,"direct_canvas not DirectColour visual!\n");
	    a_prog_bug (function_name);
	}
	get_colourmap (top, visual, visual_type, xcmap);
	kcmap = NULL;
	worldcanvas = &top->imageDisplay.magnifierDirectCanvas;
    }
    else if (w == top->imageDisplay.magnifier_true_canvas)
    {
	if (visual_type != KWIN_VISUAL_TRUECOLOUR)
	{
	    fprintf (stderr, "true_canvas not TrueColour visual!\n");
	    a_prog_bug (function_name);
	}
	kcmap = NULL;
	worldcanvas = &top->imageDisplay.magnifierTrueCanvas;
    }
    else
    {
	fprintf (stderr, "Bad canvas passed: %p\n", w);
	a_prog_bug (function_name);
    }
    /*  Create world canvas(es) and initialise for [<viewimg>] package  */
    /*  First initialise win_scale since all data is wiped here  */
    canvas_init_win_scale (&win_scale, K_WIN_SCALE_MAGIC_NUMBER);
    win_scale.blank_pixel = background_pixel;
    /*  Create the main world canvas (with dummy world min and max)  */
    win_scale.conv_type = CONV1_REAL;
    if ( ( *worldcanvas = canvas_create (pixcanvas, kcmap, &win_scale) )
	 == NULL )
    {
	exit (RV_UNDEF_ERROR);
    }
    canvas_set_attributes (*worldcanvas,
			   CANVAS_ATT_AUTO_MIN_SAT, TRUE,
			   CANVAS_ATT_AUTO_MAX_SAT, TRUE,
			   CANVAS_ATT_END);
    if (mappable) top->imageDisplay.magnifierVisibleCanvas = *worldcanvas;
    viewimg_init (*worldcanvas);
    viewimg_set_canvas_attributes (*worldcanvas,
				   VIEWIMG_ATT_AUTO_V, FALSE,
				   VIEWIMG_ATT_ENABLE_PANNING, TRUE,
				   VIEWIMG_ATT_END);
}   /*  End Function magnifier_canvas_realise_cbk   */

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
      case ZOOMCODE_UNZOOM:
	unzoom (top);
	break;
      case ZOOMCODE_INTENSITY:
	XtPopup (top->imageDisplay.izoomwinpopup, XtGrabNone);
	break;
      default:
	fprintf (stderr, "Illegal zoom code: %u\n", zoomcode);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function zoom_cbk   */

static void zoom_policy_cbk (w, client_data, call_data)
/*  This is the zoom policy popup callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;
    KWorldCanvas *canvases, *ptr;
    static char function_name[] = "ImageDisplayWidget::zoom_policy_cbk";

    if (top->imageDisplay.set_canvases) return;
    if ( ( canvases = (KWorldCanvas *) m_alloc (sizeof *canvases * 10) )
	 == NULL )
    {
	m_abort (function_name, "canvas array");
    }
    ptr = canvases;
    /*  PseudoColour canvases  */
    if (top->imageDisplay.pseudoCanvas != NULL)
    {
	*ptr++ = top->imageDisplay.pseudoCanvas;
    }
    if (top->imageDisplay.pseudoCanvasLeft != NULL)
    {
	*ptr++ = top->imageDisplay.pseudoCanvasLeft;
    }
    if (top->imageDisplay.pseudoCanvasRight != NULL)
    {
	*ptr++ = top->imageDisplay.pseudoCanvasRight;
    }
    /*  DirectColour canveses  */
    if (top->imageDisplay.directCanvas != NULL)
    {
	*ptr++ = top->imageDisplay.directCanvas;
    }
    if (top->imageDisplay.directCanvasLeft != NULL)
    {
	*ptr++ = top->imageDisplay.directCanvasLeft;
    }
    if (top->imageDisplay.directCanvasRight != NULL)
    {
	*ptr++ = top->imageDisplay.directCanvasRight;
    }
    /*  TrueColour canvases  */
    if (top->imageDisplay.trueCanvas != NULL)
    {
	*ptr++ = top->imageDisplay.trueCanvas;
    }
    if (top->imageDisplay.trueCanvasLeft != NULL)
    {
	*ptr++ = top->imageDisplay.trueCanvasLeft;
    }
    if (top->imageDisplay.trueCanvasRight != NULL)
    {
	*ptr++ = top->imageDisplay.trueCanvasRight;
    }
    *ptr = NULL;
    XtVaSetValues (top->imageDisplay.zoom_policy_popup,
		   XkwNcanvases, canvases,
		   NULL);
    top->imageDisplay.set_canvases = TRUE;
}   /*  End Function zoom_policy_cbk   */

static void region_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the region callback.
*/
{
    KWorldCanvas vc;
    KPixCanvas pc;
    flag visible;
    DataclipRegions *regions = (DataclipRegions *) call_data;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;
    static char function_name[] = "ImageDisplayWidget::region_cbk";

    if (regions->num_regions != 1)
    {
	fprintf (stderr, "num_regions: %u is not 1\n", regions->num_regions);
	a_prog_bug (function_name);
    }
    if ( (vc = top->imageDisplay.visibleCanvas) == NULL ) return;
    if (top->imageDisplay.pseudoCanvas != NULL)
    {
	canvas_set_attributes (top->imageDisplay.pseudoCanvas,
			       CANVAS_ATT_VALUE_MIN, regions->minima[0],
			       CANVAS_ATT_VALUE_MAX, regions->maxima[0],
			       CANVAS_ATT_END);
	canvas_set_attributes (top->imageDisplay.magnifierPseudoCanvas,
			       CANVAS_ATT_VALUE_MIN, regions->minima[0],
			       CANVAS_ATT_VALUE_MAX, regions->maxima[0],
			       CANVAS_ATT_END);
	if (top->imageDisplay.pseudoCanvas == vc)
	{
	    if ( !kwin_resize (canvas_get_pixcanvas (vc), TRUE, 0, 0, 0, 0) )
	    {
		fprintf (stderr, "Error resizing window\n");
	    }
	    pc = canvas_get_pixcanvas(top->imageDisplay.magnifierPseudoCanvas);
	    kwin_get_attributes (pc,
				 KWIN_ATT_VISIBLE, &visible,
				 KWIN_ATT_END);
	    if (visible) kwin_resize (pc, TRUE, 0, 0, 0, 0);
	}
    }
    if (top->imageDisplay.pseudoCanvasLeft != NULL)
    {
	canvas_set_attributes (top->imageDisplay.pseudoCanvasLeft,
			       CANVAS_ATT_VALUE_MIN, regions->minima[0],
			       CANVAS_ATT_VALUE_MAX, regions->maxima[0],
			       CANVAS_ATT_END);
	canvas_set_attributes (top->imageDisplay.pseudoCanvasRight,
			       CANVAS_ATT_VALUE_MIN, regions->minima[0],
			       CANVAS_ATT_VALUE_MAX, regions->maxima[0],
			       CANVAS_ATT_END);
    }
}   /*  End Function region_cbk   */

static flag canvas_event_handler (KWorldCanvas canvas, double x, double y,
				  unsigned int event_code, void *e_info,
				  void **f_info, double x_lin, double y_lin)
/*  [SUMMARY] Position event callback.
    [PURPOSE] This routine is a position event consumer for a world canvas.
    <canvas> The canvas on which the event occurred.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    [NOTE] These values will have been transformed by the registered
    transform function (see [<canvas_register_transform_func>]).
    <event_code> The arbitrary event code.
    <e_info> A pointer to arbitrary event information.
    <f_info> A pointer to an arbitrary function information pointer.
    <x_lin> The horizontal linear world co-ordinate prior to the transform
    function being called.
    <y_lin> The vertical linear world co-ordinate prior to the transform
    function being called.
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    ImageDisplayWidget w = (ImageDisplayWidget) *f_info;
    /*static char function_name[]="ImageDisplayWidget::canvas_event_handler";*/

    if ( (event_code == K_CANVAS_EVENT_RIGHT_MOUSE_CLICK) &&
	 (w->imageDisplay.override_shell != NULL) )
    {
	/*  Lower fullscreen canvas  */
	XLowerWindow ( XtDisplay (w),
		       XtWindow (w->imageDisplay.override_shell) );
	return (TRUE);
    }
    return (FALSE);
}   /*  End Function canvas_event_handler  */

static void unzoom (ImageDisplayWidget w)
/*  This routine will unzoom the canvas.
    The routine returns nothing.
*/
{
    KWorldCanvas visible_worldcanvas = w->imageDisplay.visibleCanvas;
    /*static char function_name[] = "unzoom";*/

    /*  Unzoom  */
    viewimg_set_canvas_attributes (visible_worldcanvas,
				   VIEWIMG_ATT_AUTO_X, TRUE,
				   VIEWIMG_ATT_AUTO_Y, TRUE,
				   VIEWIMG_ATT_END);
    /*  Redraw the canvas  */
    if ( !kwin_resize (canvas_get_pixcanvas (visible_worldcanvas),
		       TRUE, 0, 0, 0, 0) )
    {
	fprintf (stderr, "Error refreshing window\n");
    }
}   /*  End Function unzoom  */

static void colourmap_cbk (Widget w, XtPointer client_data,XtPointer call_data)
/*  This is the colourmap menu callback.
*/
{
    unsigned int colourmapcode = *(int *) call_data;
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;
    static char function_name[] = "ImageDisplayWidget::colourmap_cbk";

    switch (colourmapcode)
    {
      case COLOURMAP_PSEUDOCOLOUR:
	if (top->imageDisplay.cmapwinpopup_psuedo == NULL)
	{
	    XBell (XtDisplay (w), 100);
	    return;
	}
	XtPopup (top->imageDisplay.cmapwinpopup_psuedo, XtGrabNone);
	break;
      case COLOURMAP_DIRECTCOLOUR:
	if (top->imageDisplay.cmapwinpopup_direct == NULL)
	{
	    XBell (XtDisplay (w), 100);
	    return;
	}
	XtPopup (top->imageDisplay.cmapwinpopup_direct, XtGrabNone);
	break;
      default:
	fprintf (stderr, "Illegal colourmap code: %u\n", colourmapcode);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function colourmap_cbk   */

static void raise_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the raise button callback.
*/
{
    ImageDisplayWidget top = (ImageDisplayWidget) client_data;

    XRaiseWindow ( XtDisplay (w),XtWindow (top->imageDisplay.override_shell) );
}   /*  End Function raise_cbk   */

static Kcolourmap get_colourmap (ImageDisplayWidget w, Visual *visual,
				 unsigned int visual_type,
				 Colormap xcmap)
/*  [SUMMARY] Get or create colourmap.
    <w> The ImageDisplay widget.
    <visual> The visual type.
    <xcmap> The XColormap ID.
    [RETURN] The Kcolourmap object.
*/
{
    Kcolourmap kcmap;
    Kdisplay dpy_handle;
    flag verbose;
    unsigned int num_ccels;
    Widget colourmapwinpopup;
    unsigned long *pixel_values;
    Display *dpy;
    static char function_name[] = "ImageDisplayWidget::get_colourmap";

    dpy = XtDisplay (w);
    verbose = w->imageDisplay.verbose;
    if (visual_type == KWIN_VISUAL_PSEUDOCOLOUR)
    {
	if (w->imageDisplay.pseudo_cmap != NULL)
	{
	    return (w->imageDisplay.pseudo_cmap);
	}
	if ( ( dpy_handle = xc_get_dpy_handle (dpy, xcmap) ) == NULL )
	{
	    fprintf (stderr, "Error getting display handle\n");
	    a_prog_bug (function_name);
	}
	if ( ( kcmap = kcmap_va_create (DEFAULT_COLOURMAP_NAME,
					w->imageDisplay.cmapSize, TRUE,
					dpy_handle,
					xc_alloc_colours, xc_free_colours,
					xc_store_colours, xc_get_location,
					KCMAP_ATT_END) ) == NULL )
	{
	    fprintf (stderr, "Error creating main colourmap\n");
	    a_prog_bug (function_name);
	}
	w->imageDisplay.pseudo_cmap = kcmap;
	num_ccels = kcmap_get_pixels (kcmap, &pixel_values);
	if (verbose)
	{
	    fprintf (stderr, "%s: num colours for PseudoColour: %u\n",
			    function_name, num_ccels);
	}
	colourmapwinpopup = XtVaCreatePopupShell
	    ("pseudoCmapwinpopup", cmapwinpopupWidgetClass, (Widget) w,
	     XkwNcolourbarVisual, visual,
	     XtNdepth, 8,
	     XtNcolormap, xcmap,
	     XkwNkarmaColourmap, kcmap,
	     XkwNsimpleColourbar, True,
	     NULL);
	if (w->imageDisplay.cmap_btn != NULL)
	{
	    XtAddCallback (w->imageDisplay.cmap_btn, XtNcallback,
			   xtmisc_popup_cbk, colourmapwinpopup);
	}
	w->imageDisplay.cmapwinpopup_psuedo = colourmapwinpopup;
	return (kcmap);
    }
    /*  DirectColour visual  */
    if (w->imageDisplay.direct_cmap != NULL)
	return (w->imageDisplay.direct_cmap);
    if ( ( dpy_handle = xc_get_dpy_handle (dpy, xcmap) ) == NULL )
    {
	fprintf (stderr, "Error getting display handle\n");
	a_prog_bug (function_name);
    }
    if ( ( kcmap = kcmap_va_create (DEFAULT_COLOURMAP_NAME, 255, FALSE,
				    dpy_handle,
				    xc_alloc_colours, xc_free_colours,
				    xc_store_colours, xc_get_location,
				    KCMAP_ATT_DIRECT_VISUAL, TRUE,
				    KCMAP_ATT_END) ) == NULL )
    {
	fprintf (stderr, "Error creating main colourmap\n");
	a_prog_bug (function_name);
    }
    w->imageDisplay.direct_cmap = kcmap;
    num_ccels = kcmap_get_pixels (kcmap, &pixel_values);
    if (verbose)
    {
	fprintf (stderr, "%s: num colours for DirectColour: %u\n",
			function_name, num_ccels);
    }
    colourmapwinpopup = XtVaCreatePopupShell
	("directCmapwinpopup", cmapwinpopupWidgetClass, (Widget) w,
	 XkwNcolourbarVisual, visual,
	 XkwNkarmaColourmap, kcmap,
	 XkwNsimpleColourbar, True,
	 NULL);
    if (w->imageDisplay.cmap_btn != NULL)
    {
	XtAddCallback (w->imageDisplay.cmap_btn, XtNcallback,
		       xtmisc_popup_cbk, colourmapwinpopup);
    }
    w->imageDisplay.cmapwinpopup_direct = colourmapwinpopup;
    return (kcmap);
}   /*  End Function get_colourmap  */

static Widget handle_canvas (ImageDisplayWidget w, Widget canvas,
			     flag *one_to_map, Widget form, Widget reference)
/*  [SUMMARY] Handle a canvas widget.
    <w> The ImageDisplay widget.
    <canvas> The Canvas widget.
    <one_to_map> If the value pointed to is TRUE the canvas should be mapped,
    else it is not mapped. This is set to FALSE.
    <form> The form widget for the magnifier.
    <reference> The reference widget for the magnifier.
    [RETURNS] The magnifier canvas widget.
*/
{
    flag map;
    unsigned long foreground_pixel;
    Widget mag_canvas;
    Colormap xcmap;
    Visual *visual;

    if (canvas == NULL) return (NULL);
    map = *one_to_map;
    *one_to_map = FALSE;
    XtVaSetValues (canvas,
		   XtNmappedWhenManaged, map,
		   XkwNsilenceUnconsumed, True,
		   NULL);
    XtAddCallback (canvas, XkwNrealiseCallback,
		   ImageDisplay__canvas_realise_cbk, (XtPointer) w);
    XtVaGetValues (canvas,
		   XtNforeground, &foreground_pixel,
		   XtNvisual, &visual,
		   XtNcolormap, &xcmap,
		   NULL);
    if (form == NULL) return (NULL);
    mag_canvas = XtVaCreateManagedWidget (XtName (canvas), canvasWidgetClass,
					  form,
					  XtNfromVert, reference,
					  XtNwidth, 128,
					  XtNheight, 128,
					  XtNtop, XtChainTop,
					  XtNbottom, XtChainBottom,
					  XtNleft, XtChainLeft,
					  XtNright, XtChainRight,
					  XtNforeground, foreground_pixel,
					  XtNvisual, visual,
					  XtNcolormap, xcmap,
					  XtNmappedWhenManaged, map,
					  XtNborderWidth, 0,
					  XkwNsilenceUnconsumed, True,
					  NULL);
    XtAddCallback (mag_canvas, XkwNrealiseCallback,
		   ImageDisplay__magnifier_canvas_realise_cbk, (XtPointer) w);
    return (mag_canvas);
}   /*  End Function handle_canvas  */


/*  Public functions follow  */

void XkwImageDisplayRefresh (Widget W, flag clear)
/*  [PURPOSE] This routine will refresh the active display canvas for the
    ImageDisplay widget. If the active canvas is a stereo canvas both the left
    and right canvases are refreshed.
    <W> The widget.
    <clear> If TRUE, the canvas is clear prior to refresh.
    [RETURNS] Nothing.
*/
{
    KWorldCanvas vc;
    ImageDisplayWidget w;
    static char function_name[] = "XkwImageDisplayRefresh";

    if ( !XtIsImageDisplay (W) )
    {
	fprintf (stderr, "Invalid widget passed\n");
	a_prog_bug (function_name);
    }
    w = (ImageDisplayWidget) W;
    if ( (vc = w->imageDisplay.visibleCanvas) == NULL ) return;
    if ( (vc == w->imageDisplay.pseudoCanvasLeft) ||
	 (vc == w->imageDisplay.pseudoCanvasRight) )
    {
	canvas_resize (w->imageDisplay.pseudoCanvasLeft, NULL, clear);
	canvas_resize (w->imageDisplay.pseudoCanvasRight, NULL, clear);
    }
    else if ( (vc == w->imageDisplay.directCanvasLeft) ||
	 (vc == w->imageDisplay.directCanvasRight) )
    {
	canvas_resize (w->imageDisplay.directCanvasLeft, NULL, clear);
	canvas_resize (w->imageDisplay.directCanvasRight, NULL, clear);
    }
    else if ( (vc == w->imageDisplay.trueCanvasLeft) ||
	 (vc == w->imageDisplay.trueCanvasRight) )
    {
	canvas_resize (w->imageDisplay.trueCanvasLeft, NULL, clear);
	canvas_resize (w->imageDisplay.trueCanvasRight, NULL, clear);
    }
    else canvas_resize (vc, NULL, clear);
}   /*  End Function XkwImageDisplayRefresh  */

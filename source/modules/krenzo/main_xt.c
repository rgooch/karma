/*  main_xt.c

    Main file for  krenzo  (X11 contour field tool for Karma).

    Copyright (C) 1996  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*
    This Karma module will enable interactive viewing of a contour field
    (otherwise known as a "Renzogram", after Renzo Sanchissi).
    This module runs on an X11 server.


    Written by      Richard Gooch   4-SEP-1996: Copied from kpvslice module.

    Updated by      Richard Gooch   6-SEP-1996: Added start, end and skip
  channel sliders. Do not set levels if level value has not changed.

    Updated by      Richard Gooch   11-SEP-1996: Invalidate old level when new
  cube is loaded.

    Updated by      Richard Gooch   12-SEP-1996: Make sure after loading cube
  <change_active_states> is called after globals are updated.

    Updated by      Richard Gooch   14-SEP-1996: Made use of MomentGenerator
  widget.

    Updated by      Richard Gooch   16-SEP-1996: Made use of TracePopup
  widget.

    Updated by      Richard Gooch   19-SEP-1996: Changed to special function
  call interface for setting new array for MomentGenerator and TracePopup.

    Updated by      Richard Gooch   26-SEP-1996: Fixed display of cube maximum.

    Updated by      Richard Gooch   1-OCT-1996: Fixed resource names for new
  ExportMenu widget. Added "0th channel" image mode.

    Updated by      Richard Gooch   28-OCT-1996: Added hostname and port number
  to title.

    Updated by      Richard Gooch   3-NOV-1996: Made use of new
  <viewimg_track_compute> routine.

    Last updated by Richard Gooch   1-DEC-1996: Made use of
  <xtmisc_init_app_initialise>.


*/
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Dialog.h>
#include <karma.h>
#include <k_event_codes.h>
#include <karma_foreign.h>
#include <karma_overlay.h>
#include <karma_viewimg.h>
#include <karma_xtmisc.h>
#include <karma_conn.h>
#include <karma_wcs.h>
#include <karma_dir.h>
#include <karma_chx.h>
#include <karma_ds.h>
#include <karma_im.h>
#include <karma_hi.h>
#include <karma_xc.h>
#include <karma_ic.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_m.h>
#include <Xkw/ImageDisplay.h>
#include <Xkw/Dialogpopup.h>
#include <Xkw/Filewin.h>
#include <Xkw/ExclusiveMenu.h>
#include <Xkw/Cmapwinpopup.h>
#include <Xkw/SimpleSlider.h>
#include <Xkw/MomentGenerator.h>
#include <Xkw/TracePopup.h>
#ifdef OS_SunOS
#  include <floatingpoint.h>
#endif
#define NEED_IMAGE_MODE_ALTERNATIVES
#include "krenzo.h"


#define VERSION "1.4.3"

#define DEFAULT_COLOURMAP_NAME "Greyscale1"
#define NUM_CONTOUR_COLOURS (unsigned int) 64


/*  Private functions  */
STATIC_FUNCTION (void cubeselect_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (flag cube_dirselect_cbk,
                 (Widget w, void *info, CONST char *dirname) );
STATIC_FUNCTION (void imageselect_cbk, (Widget w, XtPointer client_data,
					XtPointer call_data) );
STATIC_FUNCTION (flag image_dirselect_cbk,
                 (Widget w, void *info, CONST char *dirname) );
STATIC_FUNCTION (track_canvas_event,
		 (ViewableImage vimage, double x, double y,
		  void *value, unsigned int event_code,
		  void *e_info, void **f_info,
		  double x_lin, double y_lin, unsigned int value_type) );
STATIC_FUNCTION (void image_mode_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void show_image,
		 (KWorldCanvas canvas, iarray image,ViewableImage vimage) );
STATIC_FUNCTION (void show_mom0, (KWorldCanvas canvas) );
STATIC_FUNCTION (void show_mom1, (KWorldCanvas canvas) );
STATIC_FUNCTION (Widget create_levels_popup, (Widget main_shell) );
STATIC_FUNCTION (void moment_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void apply_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void channels_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void destroy_func, (iarray array, void *info) );
STATIC_FUNCTION (void trace_worldcanvas_realise_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void trace_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize, void **info,
		  PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );
STATIC_FUNCTION (void show_channel0, (KWorldCanvas canvas) );


/*  Private data  */
String fallback_resources[] =
{
    "Krenzo.geometry:                                 +0+0",
    "Krenzo*pseudoColourCanvas*background:            black",
    "Krenzo*directColourCanvas*background:            black",
    "Krenzo*trueColourCanvas*background:              black",
    "Krenzo*pseudoColourStereoCanvas*background:      black",
    "Krenzo*directColourStereoCanvas*background:      black",
    "Krenzo*trueColourStereoCanvas*background:        black",
    "Krenzo*AnimateControl*positionCanvas.background: black",
    "Krenzo*pseudoColourCanvas*foreground:            white",
    "Krenzo*Command*background:                       grey70",
    "Krenzo*Repeater*background:                      grey70",
    "Krenzo*Ktoggle*background:                       grey80",
    "Krenzo*closeButton*background:                   grey90",
    "Krenzo*ChoiceMenu.background:                    turquoise",
    "Krenzo*ExclusiveMenu.background:                 turquoise",
    "Krenzo*Value*background:                         #d0a0a0",
    "Krenzo*ImageDisplay*quit*background:             orange",
    "Krenzo*background:                               aquamarine",
    "Krenzo*SimpleSlider.foreground:                  Sea Green",
    "Krenzo*ImageDisplay*trackLabel0*font:            8x13bold",
    "Krenzo*ImageDisplay*trackLabel1*font:            8x13bold",
    "Krenzo*ImageDisplay*trackLabel2*font:            8x13bold",
    "Krenzo*ImageDisplay*trackLabel3*font:            8x13bold",
    "Krenzo*ExclusiveMenu.font:                       8x13bold",
    "Krenzo*ImageDisplay*zoomMenu*font:               10x20",
    "Krenzo*ImageDisplay*crosshairMenu*font:          10x20",
    "Krenzo*ImageDisplay*exportMenu*theMenu*font:     10x20",
    "Krenzo*AnimateControl*modeMenu*font:             10x20",
    "Krenzo*imageModeMenu*font:                       10x20",
    "Krenzo*ImageDisplay.cmapSize:                    128",
    "Krenzo*ImageDisplay.Command.font:                7x13bold",
    "Krenzo*ImageDisplay.applicationBox.Command.font: 7x13bold",
    "Krenzo*ImageDisplay.applicationBox.Ktoggle.font: 7x13bold",
    "Krenzo*ImageDisplay.ChoiceMenu.font:             7x13bold",
    "Krenzo*ImageDisplay.exportMenu.menuButton.font:  7x13bold",
    "Krenzo*ImageDisplay*zoomMenu*Unzoom*foreground:  red",
    "Krenzo*Dataclip.AutoValueScale:                  False",
    "Krenzo*SimpleSlider.borderWidth:                 0",
    "Krenzo*font:                                     9x15bold",
    "Krenzo*borderColor:                              black",
    NULL
};
static XrmOptionDescRec Options[] =
{
    {"-private_cmap", ".topForm.multiCanvas.pseudoColourCanvas.forceNewCmap",
     XrmoptionNoArg, (XPointer) "True"},
    {"-num_colours", ".topForm.cmapSize", XrmoptionSepArg, (XPointer) NULL},
    {"-fullscreen", ".topForm.fullscreen", XrmoptionNoArg, (XPointer) "True"},
    {"-verbose", "*verbose", XrmoptionNoArg, (XPointer) "True"},
};
static Widget main_shell = NULL;
static Widget image_display = NULL;
static ViewableImage magnified_image = NULL;
static Widget moment_winpopup = NULL;
static Widget contour_level_dlg = NULL;
static Widget start_sld = NULL;
static Widget end_sld = NULL;
static float old_level = TOOBIG;
static Widget cube_min_label = NULL;
static Widget cube_max_label = NULL;
static Widget trace_winpopup = NULL;
static ViewableImage channel0_vimage = NULL;
static double cube_min = TOOBIG;
static double cube_max = -TOOBIG;


/*  Public functions follow  */

int main (int argc, char **argv)
{
    KOverlayList olist;
    Kdisplay dpy_handle;
    Kcolourmap main_cmap;
    XtAppContext app_context;
    Widget filewin, filepopup, w, box, files_btn, menu_btn, cmap_btn;
    Widget trace_btn;
    Widget cmapwinpopup, levels_winpopup;
    Display *dpy;
    extern KWorldCanvas main_canvas;
    extern KwcsAstro main_ap;
    extern Kcolourmap contour_cmap;
    extern Widget main_shell, image_display, moment_winpopup;
    extern Widget cube_min_label, cube_max_label;
    extern Widget trace_winpopup;
    extern char title_name[STRING_LENGTH];
    static char function_name[] = "main";

#ifdef SIGFPE_ABORT
	ieee_handler ("set", "division", SIGFPE_ABORT);
#endif
    /*  Initialise module  */
    im_register_module_name ("krenzo");
    im_register_module_version_date (VERSION);
    im_register_lib_version (KARMA_VERSION);
    /*  Start up Xt  */
    main_shell = xtmisc_init_app_initialise (&app_context, "Krenzo",
					     Options, XtNumber (Options),
					     &argc, argv, fallback_resources,
					     200,
					     NULL);
    xtmisc_set_icon (main_shell, ic_write_kimage_icon);
    /*  Initialise communications  */
    chx_register_app_context (app_context);
    conn_register_managers ( ( flag (*) () ) chx_manage,
			     ( void (*) () ) chx_unmanage,
			     ( void (*) () ) NULL );
    dpy = XtDisplay (main_shell);
    setup_comms (dpy);
    XtVaSetValues (main_shell,
		   XtNtitle, title_name,
		   NULL);
    image_display = XtVaCreateManagedWidget ("topForm",
					     imageDisplayWidgetClass,
					     main_shell,
					     XtNborderWidth, 0,
					     XkwNenableAnimation, FALSE,
					     XkwNautoIntensityScale, False,
					     XkwNnumTrackLabels, 3,
					     NULL);
    if ( ( w = XtNameToWidget (image_display, "filesButton") ) != NULL )
    {
	/*  Force main "Files" button to be re-labelled as "Cubes"  */
	XtVaSetValues (w,
		       XtNlabel, "Cubes",
		       NULL);
    }
    filepopup = XtNameToWidget (image_display, "filewinPopup");
    filewin = XtNameToWidget (filepopup, "form.selector");
    XtVaSetValues (filepopup,
		   XkwNfilenameTester, XkwFilewinStandardFileTester_3D,
		   NULL);
    XtAddCallback (filepopup, XkwNfileSelectCallback, cubeselect_cbk, NULL);
    XkwFilewinRegisterDirCbk (filewin, cube_dirselect_cbk, (Widget) filepopup);
    /*  Get the application box  */
    box = XtNameToWidget (image_display, "applicationBox");
    /*  Create the Image file selector and popup button  */
    filepopup = XtVaCreatePopupShell ("filewinPopup", filepopupWidgetClass,
				      main_shell,
				      XtNtitle, "Krenzo Image Selector",
				      XtNx, 0,
				      XtNy, 0,
				      XkwNautoPopdown, True,
				      NULL);
    files_btn = XtVaCreateManagedWidget ("filesButton", commandWidgetClass,box,
					 XtNlabel, "Images",
					 NULL);
    XtAddCallback (files_btn, XtNcallback, xtmisc_popup_cbk, filepopup);
    filewin = XtNameToWidget (filepopup, "form.selector");
    XtVaSetValues (filepopup,
		   XkwNfilenameTester, XkwFilewinStandardFileTester_nD,
		   NULL);
    XtAddCallback (filepopup, XkwNfileSelectCallback, imageselect_cbk, NULL);
    XkwFilewinRegisterDirCbk (filewin, image_dirselect_cbk,(Widget) filepopup);
    /*  Create the image mode menu  */
    w = XtVaCreateManagedWidget ("menuButton", exclusiveMenuWidgetClass, box,
				 XtNmenuName, "imageModeMenu",
				 XkwNchoiceName, "Image Mode",
				 XkwNnumItems, NUM_IMAGE_MODE_ALTERNATIVES,
				 XkwNitemStrings, image_mode_alternatives,
				 NULL);
    menu_btn = w;
    cmap_btn = XtVaCreateManagedWidget ("button", commandWidgetClass, box,
					XtNlabel, "Contour Colourmap",
					NULL);
    /*  Create the trace window popup button  */
    w = XtVaCreateManagedWidget ("traceButton", commandWidgetClass, box,
				 XtNlabel, "Profile",
				 NULL);
    trace_btn = w;
    cube_min_label = XtVaCreateManagedWidget ("minLabel",labelWidgetClass, box,
					      XtNborderWidth, 0,
					      XtNlabel,
					      "Cube min:                  ",
					      NULL);
    cube_max_label = XtVaCreateManagedWidget ("maxLabel",labelWidgetClass, box,
					      XtNborderWidth, 0,
					      XtNlabel,
					      "Cube max:                  ",
					      NULL);
    XtRealizeWidget (main_shell);
    XtVaGetValues (image_display,
		   XkwNpseudoColourCanvas, &main_canvas,
		   NULL);
    if (main_canvas == NULL)
    {
	fprintf (stderr, "PseudoColour canvas not found!\n");
	exit (RV_UNDEF_ERROR);
    }
    contour_init (main_canvas,
		  CONTOUR_CANVAS_ATT_COLOURNAME, "green",
		  CONTOUR_CANVAS_ATT_END);
    canvas_use_astro_transform (main_canvas, &main_ap);
    XtAddCallback (menu_btn, XkwNselectCallback, image_mode_cbk, main_canvas);
    viewimg_register_position_event_func (main_canvas,
					  ( flag (*) () ) track_canvas_event,
					  (void *) image_display);
    if ( (argc == 2) && (strcmp (argv[1], "-overlay") == 0) )
    {
	fprintf (stderr, "Creating overlay list...\n");
	olist = overlay_create_list ( (void *) NULL );
	overlay_specify_canvas (olist, main_canvas);
	overlay_associate_display_canvas (olist, main_canvas);
    }
    /*  Create colourmap and control widget for contours  */
    main_cmap = canvas_get_cmap (main_canvas);
    kcmap_get_attributes (main_cmap,
			  KCMAP_ATT_DPY_HANDLE, &dpy_handle,
			  KCMAP_ATT_END);
    /*  Initialise colourmap  */
    if ( ( contour_cmap = kcmap_va_create (DEFAULT_COLOURMAP_NAME,
					   NUM_CONTOUR_COLOURS, TRUE,
					   dpy_handle,
					   xc_alloc_colours, xc_free_colours,
					   xc_store_colours, xc_get_location,
					   KCMAP_ATT_END) )
	== NULL )
    {
	(void) fprintf (stderr, "Error creating main colourmap\n");
	a_prog_bug (function_name);
    }
    cmapwinpopup = XtVaCreatePopupShell ("cmapwinpopup",
					 cmapwinpopupWidgetClass, main_shell,
					 XtNtitle, "Contour Colour Control",
					 XkwNkarmaColourmap, contour_cmap,
					 XkwNsimpleColourbar, True,
					 NULL);
    XtAddCallback (cmap_btn, XtNcallback, xtmisc_popup_cbk, cmapwinpopup);
    moment_winpopup = XtVaCreatePopupShell ("orientwinpopup",
					    momentGeneratorWidgetClass,
					    main_shell,
					    XtNtitle,
					    "Moment Generator Control Window",
					    NULL);
    XtAddCallback (moment_winpopup, XkwNmomentCallback, moment_cbk, NULL);
    XtRealizeWidget (moment_winpopup);
    levels_winpopup = create_levels_popup (main_shell);
    XtPopup (levels_winpopup, XtGrabNone);
    trace_winpopup = XtVaCreatePopupShell ("tracewinpopup",
					   tracePopupWidgetClass, main_shell,
					   XtNtitle, "Profile Window",
					   XkwNkarmaColourmap, contour_cmap,
					   NULL);
    XtAddCallback (trace_btn, XtNcallback, xtmisc_popup_cbk, trace_winpopup);
    XtAddCallback (trace_winpopup, XkwNrealiseCallback,
		   trace_worldcanvas_realise_cbk, NULL);    
    XtAppMainLoop (app_context);
    return (RV_OK);
}   /*  End Function main   */

void load_and_setup (CONST char *filename)
{
    KWorldCanvas pseudo_canvas;
    KWorldCanvas mag_pseudo_canvas;
    iarray arr;
    Widget izoomwinpopup, animate_winpopup;
    char *ptr;
    char stripped_filename[STRING_LENGTH];
    char title[STRING_LENGTH];
    extern ViewableImage image_vimage, magnified_image;
    extern iarray image_arr;
    extern Widget main_shell, image_display;
    extern char title_name[STRING_LENGTH];

    strcpy (stripped_filename, filename);
    if ( ( ptr = strrchr (stripped_filename, '.') ) != NULL )
    {
	if (strcmp (ptr, ".kf") == 0) *ptr = '\0';
    }
    XtVaGetValues (image_display,
		   XkwNpseudoColourCanvas, &pseudo_canvas,
		   XkwNmagnifierPseudoColourCanvas, &mag_pseudo_canvas,
		   NULL);
    izoomwinpopup = XtNameToWidget (image_display, "izoomwinpopup");
    animate_winpopup = XtNameToWidget (image_display, "animatepopup");
    if ( ( arr = load_image (filename, pseudo_canvas, mag_pseudo_canvas,
			     &image_vimage, &magnified_image) )
	 == NULL ) return;
    if (image_arr != NULL) iarray_dealloc (image_arr);
    image_arr = arr;
    sprintf (title, "%s  file: %s\n", title_name, filename);
    XtVaSetValues (main_shell,
		   XtNtitle, title,
		   NULL);
    XtVaSetValues (image_display,
		   XkwNimageName, stripped_filename,
		   NULL);
    XtVaSetValues (izoomwinpopup,
		   XkwNiarray, image_arr,
		   NULL);
    XtVaSetValues (image_display,
		   XkwNvisibleCanvas, pseudo_canvas,
		   NULL);
}   /*  End Function load_and_setup  */


/*  Private routines follow  */

static void cubeselect_cbk (Widget w, XtPointer client_data,
			    XtPointer call_data)
/*  [SUMMARY] Cube file selector callback.
    [PURPOSE] This routine is called when a file is selected from the "Cubes"
    file selector.
    <w> The file selector.
    <client_data> Client data.
    <call_data> A pointer to the selected file.
    [RETURNS] Nothing.
*/
{
    iarray arr;
    char *filename;
    char txt[STRING_LENGTH];
    extern KWorldCanvas main_canvas;
    extern iarray cube_arr, mom0_arr, mom1_arr;
    extern ViewableImage channel0_vimage, mom0_vimage, mom1_vimage;
    extern unsigned int num_cimages;
    extern float old_level;
    extern double cube_min, cube_max;
    extern Widget end_sld;
    extern Widget cube_min_label, cube_max_label;
    extern Widget moment_winpopup, trace_winpopup;
    static char function_name[] = "cubeselect_cbk";

    filename = (char *) call_data;
    if ( ( arr = load_cube (filename, &cube_min, &cube_max) ) == NULL ) return;
    if (cube_arr != NULL) iarray_dealloc (cube_arr);
    cube_arr = arr;
    old_level = TOOBIG;
    XkwMomentGeneratorNewArray (moment_winpopup, cube_arr, cube_min, cube_max);
    XkwTracePopupNewArray (trace_winpopup, cube_arr, cube_min, cube_max);
    if (channel0_vimage != NULL) viewimg_destroy (channel0_vimage);
    if ( ( channel0_vimage = viewimg_create_restr (main_canvas,
						   cube_arr->multi_desc,
						   cube_arr->arr_desc,
						   cube_arr->data,
						   2, 1, 0,
						   0, NULL, NULL) ) == NULL )
    {
	m_abort (function_name, "0th channel ViewableImage");
    }
    if (mom0_vimage != NULL)
    {
	fprintf (stderr,"WARNING: 0th moment ViewableImage not deallocated\n");
    }
    if (mom1_vimage != NULL)
    {
	fprintf (stderr,"WARNING: 1st moment ViewableImage not deallocated\n");
    }
    XtVaGetValues (moment_winpopup,
		   XkwNmom0Array, &mom0_arr,
		   XkwNmom1Array, &mom1_arr,
		   NULL);
    if ( ( mom0_vimage = viewimg_create_from_iarray (main_canvas, mom0_arr,
						     FALSE) ) == NULL )
    {
	m_abort (function_name, "0th moment ViewableImage");
    }
    if ( ( mom1_vimage = viewimg_create_from_iarray (main_canvas, mom1_arr,
						     FALSE) ) == NULL )
    {
	m_abort (function_name, "1st moment ViewableImage");
    }
    iarray_register_destroy_func (mom0_arr, ( flag (*) () ) destroy_func,
				  &mom0_vimage);
    iarray_register_destroy_func (mom1_arr, ( flag (*) () ) destroy_func,
				  &mom1_vimage);
    sprintf (txt, "Cube min: %e", cube_min);
    XtVaSetValues (cube_min_label,
		   XtNlabel, txt,
		   NULL);
    sprintf (txt, "Cube max: %e", cube_max);
    XtVaSetValues (cube_max_label,
		   XtNlabel, txt,
		   NULL);
    XtVaSetValues (start_sld,
		   XtNvalue, 0,
		   XkwNmaximum, num_cimages - 1,
		   NULL);
    XtVaSetValues (end_sld,
		   XtNvalue, num_cimages,
		   XkwNmaximum, num_cimages - 1,
		   NULL);
    /*  Must wait until sliders are updated before changing the active states,
	since the sliders update some global data  */
    change_active_states ();
}   /*  End Function cubeselect_cbk  */

static flag cube_dirselect_cbk (Widget w, void *info, CONST char *dirname)
{
    if ( !foreign_miriad_test (dirname) ) return (FALSE);
    XtPopdown ( (Widget) info );
    XSync (XtDisplay (w), False);
    cubeselect_cbk (NULL, NULL, (XtPointer) dirname);
    return (TRUE);
}   /*  End Function cube_dirselect_cbk  */

static void imageselect_cbk (Widget w, XtPointer client_data,
			     XtPointer call_data)
/*  [SUMMARY] Image file selector callback.
    [PURPOSE] This routine is called when a file is selected from the "Images"
    file selector.
    <w> The file selector.
    <client_data> Client data.
    <call_data> A pointer to the selected file.
    [RETURNS] Nothing.
*/
{
    char *filename;

    filename = (char *) call_data;
    load_and_setup (filename);
}   /*  End Function imageselect_cbk  */

static flag image_dirselect_cbk (Widget w, void *info, CONST char *dirname)
{
    if ( !foreign_miriad_test (dirname) ) return (FALSE);
    XtPopdown ( (Widget) info );
    XSync (XtDisplay (w), False);
    imageselect_cbk (NULL, NULL, (XtPointer) dirname);
    return (TRUE);
}   /*  End Function image_dirselect_cbk  */

static flag track_canvas_event (ViewableImage vimage, double x, double y,
				void *value, unsigned int event_code,
				void *e_info, void **f_info,
				double x_lin, double y_lin,
				unsigned int value_type)
/*  [PURPOSE] This routine is a position event consumer for a world canvas
    which has a number of ViewableImage objects associated with it.
    <viewimg> The active viewable image.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    These values will have been transformed by the registered transform
    function (see  canvas_register_transform_func  ) for the associated
    world canvas.
    <value> A pointer to the data value in the viewable image corresponding
    to the event co-ordinates.
    <event_code> The arbitrary event code.
    <e_info> The arbitrary event information.
    <f_info> The arbitrary function information pointer.
    <x_lin> The linear horizontal world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <y_lin> The linear vertical world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <value_type> The type of the data value. This may be K_DCOMPLEX or
    K_UB_RGB.
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    KWorldCanvas magnifier_canvas;
    unsigned long pointer_x_index, pointer_y_index;
    double cube_x, cube_y;
    Widget image_display = (Widget) *f_info;
    Widget first_track_label, second_track_label, third_track_label;
    dim_desc *dim;
    uaddr coords[2];
    char pix_string[STRING_LENGTH];
    char world_string[STRING_LENGTH], extra_string[STRING_LENGTH];
    unsigned int dim_indices[2];
    extern iarray cube_arr;
    extern KwcsAstro main_ap, cube_ap;
    extern Widget trace_winpopup;
    /*static char function_name[] = "track_canvas_event";*/

    if (event_code != K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    viewimg_track_compute (vimage, value, value_type, x, y, x_lin, y_lin,
			   main_ap, pix_string, world_string,
			   extra_string, &pointer_x_index, &pointer_y_index);
    if (cube_arr != NULL)
    {
	convert_lin_world_coords (&cube_x, &cube_y, cube_ap,
				  &x_lin, &y_lin, main_ap, 1);
	dim_indices[0] = 2;
	dim_indices[1] = 1;
	dim = iarray_get_dim_desc (cube_arr, 2);
	coords[0] = ds_get_coord_num (dim, cube_x, SEARCH_BIAS_CLOSEST);
	dim = iarray_get_dim_desc (cube_arr, 1);
	coords[1] = ds_get_coord_num (dim, cube_y, SEARCH_BIAS_CLOSEST);
	XkwTracePopupShowTrace (trace_winpopup, dim_indices, coords);
    }
    first_track_label = XtNameToWidget (image_display, "trackLabel0");
    second_track_label = XtNameToWidget (image_display, "trackLabel1");
    third_track_label = XtNameToWidget (image_display, "trackLabel2");
    XtVaSetValues (first_track_label, XtNlabel, pix_string, NULL);
    XtVaSetValues (second_track_label, XtNlabel, world_string, NULL);
    XtVaSetValues (third_track_label, XtNlabel, extra_string, NULL);
    XtVaGetValues (image_display,
		   XkwNmagnifierVisibleCanvas, &magnifier_canvas,
		   NULL);
    viewimg_set_canvas_attributes (magnifier_canvas,
				   VIEWIMG_ATT_PAN_CENTRE_X, pointer_x_index,
				   VIEWIMG_ATT_PAN_CENTRE_Y, pointer_y_index,
				   VIEWIMG_ATT_END);
    kwin_resize (canvas_get_pixcanvas (magnifier_canvas), FALSE, 0, 0, 0, 0);
    return (TRUE);
}   /*  End Function track_canvas_event  */

static void image_mode_cbk (Widget w, XtPointer client_data,
			    XtPointer call_data)
/*  [SUMMARY] Image mode change callback.
    <w> The widget the event occurred on.
    <client_data> Client data.
    <call_data> Call data.
    [RETURNS] Nothing.
*/
{
    KWorldCanvas canvas = (KWorldCanvas) client_data;
    unsigned int code = *(int *) call_data;
    extern ViewableImage image_vimage;
    extern iarray image_arr;
    extern KwcsAstro main_ap, loaded_image_ap, cube_ap;
    extern unsigned int image_mode;
    extern Widget moment_winpopup;
    /*static char function_name[] = "image_mode_cbk";*/

    image_mode = code;
    switch (code)
    {
      case IMAGE_MODE_LOADED:
	show_image (canvas, image_arr, image_vimage);
	main_ap = loaded_image_ap;
	/*XtPopdown (moment_winpopup);*/
	break;
      case IMAGE_MODE_MOM0:
	XtPopup (moment_winpopup, XtGrabNone);
	show_mom0 (canvas);
	main_ap = cube_ap;
	break;
      case IMAGE_MODE_MOM1:
	XtPopup (moment_winpopup, XtGrabNone);
	show_mom1 (canvas);
	main_ap = cube_ap;
	break;
      case IMAGE_MODE_CHANNEL0:
	show_channel0 (canvas);
	main_ap = cube_ap;
	break;
    }
}   /*  End Function image_mode_cbk  */

static void show_image (KWorldCanvas canvas, iarray image,ViewableImage vimage)
/*  [SUMMARY] Show the loaded image.
    <canvas> The world canvas.
    <image> The image array.
    <vimage> The image.
    [RETURNS] Nothing.
*/
{
    double i_min, i_max;
    /*static char function_name[] = "show_image";*/

    if (vimage == NULL) return;
    if (viewimg_get_active (canvas) == vimage) return;
    /*  Must change the canvas intensity scale  */
    if ( !iarray_min_max (image, CONV1_REAL, &i_min, &i_max) )
    {
	fprintf (stderr, "Error computing min-max\n");
    }
    canvas_set_attributes (canvas,
			   CANVAS_ATT_VALUE_MIN, i_min,
			   CANVAS_ATT_VALUE_MAX, i_max,
			   CANVAS_ATT_END);
    viewimg_make_active (vimage);
}   /*  End Function show_image  */

static void show_mom0 (KWorldCanvas canvas)
/*  [SUMMARY] Show the 0th moment map.
    <canvas> The world canvas.
    [RETURNS] Nothing.
*/
{
    double i_min, i_max;
    extern iarray mom0_arr;
    extern ViewableImage mom0_vimage;
    /*static char function_name[] = "show_mom0";*/

    if (mom0_vimage == NULL) return;
    if (viewimg_get_active (canvas) == mom0_vimage) return;
    /*  Must change the canvas intensity scale  */
    if ( !iarray_min_max (mom0_arr, CONV1_REAL, &i_min, &i_max) )
    {
	fprintf (stderr, "Error computing min-max\n");
    }
    if (i_min >= i_max)
    {
	fprintf (stderr, "Computed 0th moment map is blank: ignoring\n");
	return;
    }
    canvas_set_attributes (canvas,
			   CANVAS_ATT_VALUE_MIN, i_min,
			   CANVAS_ATT_VALUE_MAX, i_max,
			   CANVAS_ATT_END);
    viewimg_make_active (mom0_vimage);
}   /*  End Function show_mom0  */

static void show_mom1 (KWorldCanvas canvas)
/*  [SUMMARY] Show the 1st moment map.
    <canvas> The world canvas.
    [RETURNS] Nothing.
*/
{
    double i_min, i_max;
    extern iarray mom1_arr;
    extern ViewableImage mom0_vimage, mom1_vimage;
    /*static char function_name[] = "show_mom1";*/

    if (mom1_vimage == NULL) return;
    if ( (viewimg_get_active (canvas) == NULL) ||
	 (viewimg_get_active (canvas) == mom0_vimage) )
    {
	/*  Must change the canvas intensity scale  */
	if ( !iarray_min_max (mom1_arr, CONV1_REAL, &i_min, &i_max) )
	{
	    fprintf (stderr, "Error computing min-max\n");
	}
	if (i_min >= i_max)
	{
	    fprintf (stderr, "Computed 1st moment map is blank: ignoring\n");
	    return;
	}
	canvas_set_attributes (canvas,
			       CANVAS_ATT_VALUE_MIN, i_min,
			       CANVAS_ATT_VALUE_MAX, i_max,
			       CANVAS_ATT_END);
    }
    viewimg_make_active (mom1_vimage);
}   /*  End Function show_mom1  */

static Widget create_levels_popup (Widget main_shell)
/*  [SUMMARY] Create the level control popup.
    <main_shell> The main shell widget.
    [RETURNS] The popup.
*/
{
    Widget form, btn, popup, skip_sld;
    extern int start_channel, end_channel, channel_skip;
    extern Widget contour_level_dlg, start_sld, end_sld;

    popup = XtVaCreatePopupShell ("orientwinpopup",
				  topLevelShellWidgetClass, main_shell,
				  XtNtitle, "Contour Level Control Window",
				  NULL);
    form = XtVaCreateManagedWidget ("form", formWidgetClass, popup,
				    XtNborderWidth,0,
				    NULL);
#ifdef DISABLED
    btn = XtVaCreateManagedWidget ("closeButton", commandWidgetClass, form,
				   XtNlabel, "Close",
				   XtNtop, XtChainTop,
				   XtNbottom, XtChainTop,
				   XtNleft, XtChainLeft,
				   XtNright, XtChainRight,
				   XtNwidth, 100,
				   XtNheight, 20,
				   NULL);
    XtAddCallback (btn, XtNcallback, xtmisc_popdown_cbk, popup);
#endif
    btn = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
				   XtNlabel, "Apply Parameters",
				   /*XtNfromHoriz, btn,*/
				   NULL);
    XtAddCallback (btn, XtNcallback, apply_cbk, NULL);
    contour_level_dlg = XtVaCreateManagedWidget ("contourLevel",
						 dialogWidgetClass, form,
						 XtNlabel, "Contour Level",
						 XtNfromVert, btn,
						 XtNvalue, "5e-2",
						 NULL);
    start_sld = XtVaCreateManagedWidget ("startSlider",
					 simpleSliderWidgetClass, form,
					 XtNlabel, "Start Channel",
					 XtNfromVert, contour_level_dlg,
					 XkwNmodifier, 1,
					 XkwNshowRange, TRUE,
					 XkwNvaluePtr, &start_channel,
					 NULL);
    XtAddCallback (start_sld, XkwNvalueChangeCallback, channels_cbk, NULL);
    end_sld = XtVaCreateManagedWidget ("endSlider",
				       simpleSliderWidgetClass, form,
				       XtNlabel, "End Channel",
				       XtNfromVert, start_sld,
				       XkwNmodifier, 1,
				       XkwNshowRange, TRUE,
				       XkwNvaluePtr, &end_channel,
				       NULL);
    XtAddCallback (end_sld, XkwNvalueChangeCallback, channels_cbk, NULL);
    skip_sld = XtVaCreateManagedWidget ("skipSlider",
					simpleSliderWidgetClass, form,
					XtNlabel, "Channel Skip",
					XtNfromVert, end_sld,
					XkwNmodifier, 1,
					XkwNminimum, 1,
					XkwNmaximum, 10,
					XtNvalue, channel_skip,
					XkwNvaluePtr, &channel_skip,
					NULL);
    XtAddCallback (skip_sld, XkwNvalueChangeCallback, channels_cbk, NULL);
    XtRealizeWidget (popup);
    return (popup);
}   /*  End Function create_levels_popup  */

static void moment_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [SUMMARY] Moment computation callback.
    <w> The widget the event occurred on.
    <client_data> Client data.
    <call_data> Call data.
    [RETURNS] Nothing.
*/
{
    extern iarray cube_arr;
    extern KWorldCanvas main_canvas;
    extern ViewableImage mom0_vimage, mom1_vimage;
    extern unsigned int image_mode;
    /*static char function_name[] = "moment_cbk";*/

    if (cube_arr == NULL) return;
    viewimg_register_data_change (mom0_vimage);
    viewimg_register_data_change (mom1_vimage);
    if (image_mode == IMAGE_MODE_MOM0) show_mom0 (main_canvas);
    if (image_mode == IMAGE_MODE_MOM1) show_mom1 (main_canvas);
}   /*  End Function moment_cbk  */

static void apply_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [SUMMARY] Contour levels change callback.
    <w> The widget the event occurred on.
    <client_data> Client data.
    <call_data> Call data.
    [RETURNS] Nothing.
*/
{
    unsigned int count, num_pixels;
    unsigned long pixel_value;
    float tmp, level;
    double d_level;
    char *clip_str, *p;
    unsigned long *pixel_values;
    extern KWorldCanvas main_canvas;
    extern Kcolourmap contour_cmap;
    extern iarray cube_arr;
    extern flag change_active_cimages;
    extern unsigned int num_cimages;
    extern float old_level;
    extern Widget contour_level_dlg;
    extern KContourImage *cimages;
    /*static char function_name[] = "apply_cbk";*/

    if (cube_arr == NULL) return;
    XtVaGetValues (contour_level_dlg,
		   XtNvalue, &clip_str,
		   NULL);
    level = ex_float (clip_str, &p);
    if (level != old_level)
    {
	num_pixels = kcmap_get_pixels (contour_cmap, &pixel_values);
	d_level = level;
	for (count = 0; count < num_cimages; ++count)
	{
	    tmp = (float) count / (float) (num_cimages - 1) *
		(float) (num_pixels - 1);
	    pixel_value = pixel_values[(int) tmp];
	    contour_set_levels (cimages[count], 1, &d_level, &pixel_value);
	}
	old_level = level;
    }
    if (change_active_cimages) change_active_states ();
    kwin_resize (canvas_get_pixcanvas (main_canvas), FALSE, 0, 0, 0, 0);
}   /*  End Function apply_cbk  */

static void channels_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [SUMMARY] Channels change callback.
    <w> The widget the event occurred on.
    <client_data> Client data.
    <call_data> Call data.
    [RETURNS] Nothing.
*/
{
    extern flag change_active_cimages;

    change_active_cimages = TRUE;
}   /*  End Function channels_cbk  */

static void destroy_func (iarray array, void *info)
/*  [SUMMARY] Register destruction of an Intelligent Array.
    <array> The Intelligent Array.
    <info> A pointer to the arbitrary information.
    [RETURNS] Nothing.
*/
{
    ViewableImage *vimage = info;

    viewimg_destroy (*vimage);
    *vimage = NULL;
}   /*  End Function destroy_func  */

static void trace_worldcanvas_realise_cbk (Widget w, XtPointer client_data,
					   XtPointer call_data)
/*  [SUMMARY] Canvas realise callback.
    [PURPOSE] This routine is called when a Canvas widget is realised and the
    KWorldCanvas object is created.
    <w> The Canvas widget that has just been realised.
    <client_data> The client data.
    <call_data> The world canvas.
    [RETURNS] Nothing.
*/
{
    KWorldCanvas worldcanvas = (KWorldCanvas) call_data;

    canvas_register_refresh_func (worldcanvas, trace_refresh_func, NULL);
}   /*  End Function trace_worldcanvas_realise_cbk   */

static void trace_refresh_func (KWorldCanvas canvas, int width, int height,
				struct win_scale_type *win_scale,
				Kcolourmap cmap, flag cmap_resize, void **info,
				PostScriptPage pspage,
				unsigned int num_areas,
				KPixCanvasRefreshArea *areas,
				flag *honoured_areas)
/*  [SUMMARY] Refresh event callback.
    [PURPOSE] This routine is a refresh event consumer for a world canvas.
    <canvas> The world canvas being refreshed.
    <width> The width of the canvas in pixels.
    <height> The height of the canvas in pixels.
    <win_scale> A pointer to the window scaling information.
    <cmap> The colourmap associated with the canvas.
    <cmap_resize> TRUE if the refresh function was called as a result of a
    colourmap resize, else FALSE.
    <info> A pointer to the arbitrary canvas information pointer.
    <pspage> If not NULL, the PostScriptPage object the refresh is
    redirected to.
    <num_areas> The number of areas that need to be refreshed. If this is
    0 then the entire pixel canvas needs to be refreshed.
    <areas> The list of areas that need to be refreshed. Note that these
    areas are given in pixel co-ordinates.
    <honoured_areas> If the value TRUE is written here it is assumed the
    routine honoured the list of refresh areas and did not write outside
    these areas and hence the list of areas will be passed to subsequent
    registered refresh routines. If FALSE is written here (or nothing is
    written here), implying the routine refreshed the entire canvas,
    subsequent refresh routines will be told to refresh the entire canvas.
    [RETURNS] Nothing.
*/
{
    unsigned long pixel_value;
    double left_x, right_x, dummy;
    extern float old_level;

    if ( !canvas_get_colour (canvas, "white", &pixel_value, NULL, NULL, NULL) )
    {
	pixel_value = 0;
    }
    canvas_get_attributes (canvas,
			   CANVAS_ATT_LEFT_X, &left_x,
			   CANVAS_ATT_RIGHT_X, &right_x,
			   CANVAS_ATT_END);
    /*  Convert left&right x values to non-linear co-ordinates  */
    dummy = 0.0;
    canvas_coords_transform (canvas, 1, &left_x, FALSE, &dummy, FALSE);
    dummy = 0.0;
    canvas_coords_transform (canvas, 1, &right_x, FALSE, &dummy, FALSE);
    canvas_draw_line_p (canvas, left_x, old_level, right_x, old_level,
			pixel_value);
}   /*  End Function trace_refresh_func  */

static void show_channel0 (KWorldCanvas canvas)
/*  [SUMMARY] Show the 0th channel map.
    <canvas> The world canvas.
    [RETURNS] Nothing.
*/
{
    extern ViewableImage channel0_vimage;
    extern double cube_min, cube_max;
    /*static char function_name[] = "show_channel0";*/

    if (channel0_vimage == NULL) return;
    if (viewimg_get_active (canvas) == channel0_vimage) return;
    /*  Must change the canvas intensity scale  */
    canvas_set_attributes (canvas,
			   CANVAS_ATT_VALUE_MIN, cube_min,
			   CANVAS_ATT_VALUE_MAX, cube_max,
			   CANVAS_ATT_END);
    viewimg_make_active (channel0_vimage);
}   /*  End Function show_channel0  */

/*  main_xt.c

    Main file for  kpvslice  (X11 Position-Velocity slice tool for Karma).

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
    This Karma module will enable interactive selection of a Position-Velocity
    slice through a data cube.
    This module runs on an X11 server.


    Written by      Richard Gooch   12-JUN-1996: Copied from kview module.

    Updated by      Richard Gooch   16-JUN-1996: Added check for *__SCALE and
  *__OFFSET data.

    Updated by      Richard Gooch   19-JUN-1996: Made use of
  <iarray_format_value> routine.

    Updated by      Richard Gooch   22-JUN-1996: Made use of
  <XkwFilewinStandardFileTester_nD> and <XkwFilewinStandardFileTester_3D>
  routines.

    Updated by Richard Gooch   27-JUN-1996: Switched to
  <canvas_use_astro_transform>

    Updated by      Richard Gooch   1-AUG-1996: Created separate directory
  select callbacks for cubes and images, otherwise Miriad cubes would be sent
  to the image loader.

    Updated by      Richard Gooch   10-AUG-1996: Added trace and main refresh
  functions.

    Updated by      Richard Gooch   11-AUG-1996: Computed 1st moment map.

    Updated by      Richard Gooch   13-AUG-1996: Added image mode menu.

    Updated by      Richard Gooch   14-AUG-1996: Added moment control popup.

    Updated by      Richard Gooch   15-AUG-1996: Added version number to title.

    Updated by      Richard Gooch   22-AUG-1996: Made code more robust to
  images/cubes/moment maps having 0 data range.

    Updated by      Richard Gooch   31-AUG-1996: Only compute moments in
  response to "Apply Parameters" button. Show cube min and max in moment
  control window.

    Updated by      Richard Gooch   3-SEP-1996: Restored main canvas magnifier
  which did not display under some conditions.

    Updated by      Richard Gooch   6-SEP-1996: Added median algorithm.

    Updated by      Richard Gooch   11-SEP-1996: Added "Show Trace" toggle.

    Updated by      Richard Gooch   14-SEP-1996: Made use of MomentGenerator
  widget.

    Updated by      Richard Gooch   16-SEP-1996: Made use of TracePopup
  widget.

    Updated by      Richard Gooch   19-SEP-1996: Changed to special function
  call interface for setting new array for MomentGenerator and TracePopup.

    Updated by      Richard Gooch   1-OCT-1996: Fixed resource names for new
  ExportMenu widget. Added "0th channel" image mode.

    Updated by      Richard Gooch   28-OCT-1996: Added hostname and port number
  to title.

    Updated by      Richard Gooch   3-NOV-1996: Made use of new
  <viewimg_track_compute> routine.

    Updated by      Richard Gooch   10-NOV-1996: Made use of new attributes for
  ViewableImage objects to override the canvas intensity range.

    Updated by      Richard Gooch   11-NOV-1996: Only refresh magnifier canvas
  if visible.

    Updated by      Richard Gooch   14-NOV-1996: Trap when displaying
  ViewableImage for moment maps not yet computed.

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
#include <Xkw/Ktoggle.h>
#include <Xkw/ExclusiveMenu.h>
#include <Xkw/MomentGenerator.h>
#include <Xkw/TracePopup.h>
#ifdef OS_SunOS
#  include <floatingpoint.h>
#endif
#define NEED_IMAGE_MODE_ALTERNATIVES
#include "kpvslice.h"


#define VERSION "1.7.9"


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
STATIC_FUNCTION (void toggle_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void image_mode_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void show_image,
		 (KWorldCanvas canvas, iarray image,ViewableImage vimage) );
STATIC_FUNCTION (void show_mom0, (KWorldCanvas canvas) );
STATIC_FUNCTION (void show_mom1, (KWorldCanvas canvas) );
STATIC_FUNCTION (void moment_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void show_trace_toggle_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void destroy_func, (iarray array, void *info) );
STATIC_FUNCTION (void show_channel0, (KWorldCanvas canvas) );


/*  Private data  */
String fallback_resources[] =
{
    "Kpvslice.geometry:                                 +0+0",
    "Kpvslice*pseudoColourCanvas*background:            black",
    "Kpvslice*directColourCanvas*background:            black",
    "Kpvslice*trueColourCanvas*background:              black",
    "Kpvslice*pseudoColourStereoCanvas*background:      black",
    "Kpvslice*directColourStereoCanvas*background:      black",
    "Kpvslice*trueColourStereoCanvas*background:        black",
    "Kpvslice*AnimateControl*positionCanvas.background: black",
    "Kpvslice*pseudoColourCanvas*foreground:            white",
    "Kpvslice*Command*background:                       grey70",
    "Kpvslice*Repeater*background:                      grey70",
    "Kpvslice*Ktoggle*background:                       grey80",
    "Kpvslice*closeButton*background:                   grey90",
    "Kpvslice*ChoiceMenu.background:                    turquoise",
    "Kpvslice*ExclusiveMenu.background:                 turquoise",
    "Kpvslice*Value*background:                         #d0a0a0",
    "Kpvslice*ImageDisplay*quit*background:             orange",
    "Kpvslice*background:                               aquamarine",
    "Kpvslice*SimpleSlider.foreground:                  Sea Green",
    "Kpvslice*auxShell.geometry:                        -0+0",
    "Kpvslice*ImageDisplay*trackLabel0*font:            8x13bold",
    "Kpvslice*ImageDisplay*trackLabel1*font:            8x13bold",
    "Kpvslice*ImageDisplay*trackLabel2*font:            8x13bold",
    "Kpvslice*ImageDisplay*trackLabel3*font:            8x13bold",
    "Kpvslice*ExclusiveMenu.font:                       8x13bold",
    "Kpvslice*ImageDisplay*zoomMenu*font:               10x20",
    "Kpvslice*ImageDisplay*crosshairMenu*font:          10x20",
    "Kpvslice*ImageDisplay*exportMenu*theMenu*font:     10x20",
    "Kpvslice*AnimateControl*modeMenu*font:             10x20",
    "Kpvslice*imageModeMenu*font:                       10x20",
    "Kpvslice*ImageDisplay.cmapSize:                    100",
    "Kpvslice*ImageDisplay.Command.font:                7x13bold",
    "Kpvslice*ImageDisplay.applicationBox.Command.font: 7x13bold",
    "Kpvslice*ImageDisplay.applicationBox.Ktoggle.font: 7x13bold",
    "Kpvslice*ImageDisplay.ChoiceMenu.font:             7x13bold",
    "Kpvslice*ImageDisplay.exportMenu.menuButton.font:  7x13bold",
    "Kpvslice*ImageDisplay*zoomMenu*Unzoom*foreground:  red",
    "Kpvslice*Dataclip.AutoValueScale:                  False",
    "Kpvslice*SimpleSlider.borderWidth:                 0",
    "Kpvslice*font:                                     9x15bold",
    "Kpvslice*borderColor:                              black",
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
static Widget main_image_display = NULL;
static Widget aux_image_display = NULL;
static ViewableImage magnified_image = NULL;
static KWorldCanvas main_canvas = NULL;
static KWorldCanvas aux_canvas = NULL;
static Widget moment_winpopup = NULL;
static Widget trace_winpopup = NULL;
static ViewableImage channel0_vimage = NULL;
static double cube_min = TOOBIG;
static double cube_max = -TOOBIG;


/*  Public functions follow  */

int main (int argc, char **argv)
{
    /*KWorldCanvas aux_canvas;*/
    KOverlayList olist;
    XtAppContext app_context;
    Widget filewin, filepopup, w, box, files_btn;
    Widget aux_shell;
    Widget auxbox, menu_btn, trace_btn;
    Display *dpy;
    extern flag auto_update, show_trace;
    extern KwcsAstro main_ap, aux_ap;
    extern Widget main_shell, main_image_display, moment_winpopup;
    extern Widget trace_winpopup;
    extern char title_name[STRING_LENGTH];

#ifdef SIGFPE_ABORT
	ieee_handler ("set", "division", SIGFPE_ABORT);
#endif
    /*  Initialise module  */
    im_register_module_name ("kpvslice");
    im_register_module_version_date (VERSION);
    im_register_lib_version (KARMA_VERSION);
    /*  Start up Xt  */
    main_shell = xtmisc_init_app_initialise (&app_context, "Kpvslice",
					     Options, XtNumber (Options),
					     &argc, argv, fallback_resources,
					     210,
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
    main_image_display = XtVaCreateManagedWidget ("topForm",
						  imageDisplayWidgetClass,
						  main_shell,
						  XtNborderWidth, 0,
						  XkwNenableAnimation, FALSE,
						  XkwNautoIntensityScale,False,
						  XkwNnumTrackLabels, 4,
						  NULL);
    if ( ( w = XtNameToWidget (main_image_display, "filesButton") ) != NULL )
    {
	/*  Force main "Files" button to be re-labelled as "Cubes"  */
	XtVaSetValues (w,
		       XtNlabel, "Cubes",
		       NULL);
    }
    filepopup = XtNameToWidget (main_image_display, "filewinPopup");
    filewin = XtNameToWidget (filepopup, "form.selector");
    XtVaSetValues (filepopup,
		   XkwNfilenameTester, XkwFilewinStandardFileTester_3D,
		   NULL);
    XtAddCallback (filepopup, XkwNfileSelectCallback, cubeselect_cbk, NULL);
    XkwFilewinRegisterDirCbk (filewin, cube_dirselect_cbk, (Widget) filepopup);
    /*  Get the application box  */
    box = XtNameToWidget (main_image_display, "applicationBox");
    /*  Create the Image file selector and popup button  */
    filepopup = XtVaCreatePopupShell ("filewinPopup", filepopupWidgetClass,
				      main_shell,
				      XtNtitle, "Kpvslice Image Selector",
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
    /*  Create the auto update toggle  */
    w = XtVaCreateManagedWidget ("autoUpdateToggle", ktoggleWidgetClass, box,
				 XtNlabel, "Auto Update",
				 XtNstate, auto_update,
				 XkwNcrosses, FALSE,
				 NULL);
    XtAddCallback (w, XtNcallback, toggle_cbk, (XtPointer) &auto_update);
    /*  Create the trace window popup button  */
    w = XtVaCreateManagedWidget ("traceButton", commandWidgetClass, box,
				 XtNlabel, "Profile",
				 NULL);
    trace_btn = w;
    XtRealizeWidget (main_shell);
    XtVaGetValues (main_image_display,
		   XkwNpseudoColourCanvas, &main_canvas,
		   NULL);
    if (main_canvas == NULL)
    {
	fprintf (stderr, "PseudoColour canvas not found!\n");
	exit (RV_UNDEF_ERROR);
    }
    canvas_use_astro_transform (main_canvas, &main_ap);
    XtAddCallback (menu_btn, XkwNselectCallback, image_mode_cbk, main_canvas);
    /* this reads back the coords of cursor in main window */
    viewimg_register_position_event_func (main_canvas,
					  ( flag (*) () ) track_canvas_event,
					  (void *) main_image_display);
    if ( (argc == 2) && (strcmp (argv[1], "-overlay") == 0) )
    {
	fprintf (stderr, "Creating overlay list...\n");
	olist = overlay_create_list ( (void *) NULL );
	overlay_specify_canvas (olist, main_canvas);
	overlay_associate_display_canvas (olist, main_canvas);
    }
    aux_shell = XtVaCreatePopupShell ("auxShell", topLevelShellWidgetClass,
				      main_shell,
				      XtNtitle, "Slice Window",
				      NULL);
    aux_image_display = XtVaCreateManagedWidget ("topForm",
						 imageDisplayWidgetClass,
						 aux_shell,
						 XtNborderWidth, 0,
						 XkwNenableAnimation, FALSE,
						 XkwNautoIntensityScale, FALSE,
                                                 XkwNnumTrackLabels, 3,
						 XkwNshowQuitButton, FALSE,
						 NULL);
    /* get application box of auxilary window */
    auxbox = XtNameToWidget (aux_image_display, "applicationBox");
    /*  Create the show trace toggle  */
    w = XtVaCreateManagedWidget ("showTraceToggle", ktoggleWidgetClass, auxbox,
				 XtNlabel, "Show Trace",
				 XtNstate, show_trace,
				 XkwNcrosses, FALSE,
				 NULL);
    XtAddCallback (w, XtNcallback, show_trace_toggle_cbk,
		   (XtPointer) &show_trace);
    XtRealizeWidget (aux_shell);
    XtVaGetValues (aux_image_display,
		   XkwNpseudoColourCanvas, &aux_canvas,
		   NULL);
    if (aux_canvas == NULL)
    {
	fprintf (stderr, "PseudoColour canvas not found!\n");
	exit (RV_UNDEF_ERROR);
    }
    canvas_use_astro_transform (aux_canvas, &aux_ap);
    /* this reads back cursor coords in 'slice' window. */

    /* how to do it otherwise?
       I want to  have the extra header info transferred only once.
       I also want to be prompted for a fits file name.

       I suppose fix_slice_headers could be called when cursor enters the
       aux window, or the export menu is selected, so the file is ready to go
       as soon as the file name has been entered.
       Put in a request for a fits_file_name from the aux_display widget, and
       when that request is fulfilled, run the fix_slice_headers function?
    */

    viewimg_register_position_event_func (aux_canvas,
					  ( flag (*) () ) track_aux_canvas_event,
					  (void *) main_canvas);
					  
    /* this draws the slice & calls extraction */
    canvas_register_position_event_func (main_canvas,
					 ( flag (*) () )endpoint_position_func,
					 aux_canvas);
    canvas_register_refresh_func (main_canvas, main_refresh_func, NULL);
    canvas_register_refresh_func (aux_canvas, trace_refresh_func, NULL);
    XtPopup (aux_shell, XtGrabNone);
    moment_winpopup = XtVaCreatePopupShell ("momentwinpopup",
					    momentGeneratorWidgetClass,
					    main_shell,
					    XtNtitle,
					    "Moment Generator Control Window",
					    NULL);
    XtAddCallback (moment_winpopup, XkwNmomentCallback, moment_cbk, NULL);
    XtRealizeWidget (moment_winpopup);
    trace_winpopup = XtVaCreatePopupShell ("tracewinpopup",
					   tracePopupWidgetClass, main_shell,
					   XtNtitle, "Profile Window",
					   NULL);
    XtAddCallback (trace_btn, XtNcallback, xtmisc_popup_cbk, trace_winpopup);
    XtAppMainLoop (app_context);
    return (RV_OK);
}   /*  End Function main   */

void load_and_setup (CONST char *filename)
{
    KWorldCanvas pseudo_canvas;
    KWorldCanvas mag_pseudo_canvas;
    Widget izoomwinpopup, animate_winpopup;
    char *ptr;
    char stripped_filename[STRING_LENGTH];
    char title[STRING_LENGTH];
    extern ViewableImage image_vimage, magnified_image;
    extern iarray image_arr;
    extern Widget main_shell, main_image_display;
    extern char title_name[STRING_LENGTH];

    strcpy (stripped_filename, filename);
    if ( ( ptr = strrchr (stripped_filename, '.') ) != NULL )
    {
	if (strcmp (ptr, ".kf") == 0) *ptr = '\0';
    }
    XtVaGetValues (main_image_display,
		   XkwNpseudoColourCanvas, &pseudo_canvas,
		   XkwNmagnifierPseudoColourCanvas, &mag_pseudo_canvas,
		   NULL);
    izoomwinpopup = XtNameToWidget (main_image_display, "izoomwinpopup");
    animate_winpopup = XtNameToWidget (main_image_display, "animatepopup");
    if ( !load_image (filename, &image_arr, pseudo_canvas, mag_pseudo_canvas,
		      &image_vimage, &magnified_image) ) return;
    sprintf (title, "%s  file: %s\n", title_name, filename);
    XtVaSetValues (main_shell,
		   XtNtitle, title,
		   NULL);
    XtVaSetValues (main_image_display,
		   XkwNimageName, stripped_filename,
		   NULL);
    XtVaSetValues (izoomwinpopup,
		   XkwNiarray, image_arr,
		   NULL);
    XtVaSetValues (main_image_display,
		   XkwNvisibleCanvas, pseudo_canvas,
		   NULL);
}   /*  End Function load_and_setup  */

void print_slice_info (CONST char *window_string)
/*  [SUMMARY] Print information about the slice.
    <window_string> The string to display.
    96.08.01: Now goes to slice window as well as main window
    [RETURNS] Nothing.
*/
{
    Widget label;
    extern Widget main_image_display;

    label = XtNameToWidget (main_image_display, "trackLabel3");
    XtVaSetValues (label,
		   XtNlabel, window_string,
		   NULL);
}   /*  End Function print_slice_info  */

flag track_aux_canvas_xtcoord (CONST char *track_txt, int track_num)
/*  [PURPOSE] Handles the Xt part of outputting coords while tracking cursor
    in auxilary window.
    NB: The first track coord is slice centre & PA, which is sent from
    track_canvas_event, as the slice is moved around in the main window.
    This routine updates the second or third track coord lines, which have
    the offset arcsec & velocity coords, and
    the RA & DEC of that offset, respectively.
    This routine should only be called by track_aux_canvas_event.
    [RETURNS] FALSE if an error, TRUE otherwise.
*/
{
    Widget track_label;
    char trackLabeln[STRING_LENGTH];
    extern Widget aux_image_display;
    /*static char function_name[] = "track_aux_canvas_xtcoord";*/

    if ( track_num < 0 || track_num > 2 ) {
      fprintf (stderr, "TRACK_AUX_CANVAS_XTSTUFF: Bad track label number\n");
      return(FALSE);
    }
    sprintf (trackLabeln, "%s%d", "trackLabel", track_num);

    track_label = XtNameToWidget (aux_image_display, trackLabeln);
    XtVaSetValues (track_label, XtNlabel, track_txt, NULL);

    return(TRUE);
}

flag track_aux_canvas_xtmagupd (ViewableImage vimage,
				unsigned long xi, unsigned long yi)
/*  [PURPOSE] Handles the Xt part of updating magnifier while tracking cursor
    in auxilary window.
    There are two tasks; displaying coordinates in the track windows, and
    updating the magnifier window.
    NB: The first track coord is slice centre & PA, which is sent from
    track_canvas_event, as the slice is moved around in the main window.
    This routine updates the second or third track coord lines, which have
    the offset arcsec & velocity coords, and
    the RA & DEC of that offset, respectively.
    This routine should only be called by track_aux_canvas_event.
    [NOTE] The slice magnifier is not implemented yet.
    [RETURNS] FALSE if an error, TRUE otherwise.
*/
{
    KWorldCanvas  magnifier_canvas;
    extern Widget aux_image_display;
    /*static char function_name[] = "track_aux_canvas_xtmagupd";*/

    XtVaGetValues (aux_image_display,
		   XkwNmagnifierVisibleCanvas, &magnifier_canvas,
		   NULL);
    viewimg_set_canvas_attributes (magnifier_canvas,
				   VIEWIMG_ATT_PAN_CENTRE_X, xi,
				   VIEWIMG_ATT_PAN_CENTRE_Y, yi,
				   VIEWIMG_ATT_END);
    kwin_resize (canvas_get_pixcanvas (magnifier_canvas), FALSE, 0, 0, 0, 0);
    return (TRUE);
}   /*  End Function track_aux_canvas_xtmagupd  */


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
    KWorldCanvas aux_canvas;
    Widget izoomwinpopup;
    char *filename;
    extern KWorldCanvas main_canvas;
    extern iarray cube_arr, mom0_arr, mom1_arr;
    extern ViewableImage channel0_vimage, mom0_vimage, mom1_vimage;
    extern KwcsAstro main_ap, cube_ap;
    extern unsigned int image_mode;
    extern double cube_min, cube_max;
    extern Widget aux_image_display;
    extern Widget moment_winpopup, trace_winpopup;
    static char function_name[] = "cubeselect_cbk";

    filename = (char *) call_data;
    if ( !load_cube (filename, &cube_arr, &cube_min, &cube_max) ) return;
    XtVaGetValues (aux_image_display,
		   XkwNpseudoColourCanvas, &aux_canvas,
		   NULL);
    canvas_set_attributes (aux_canvas,
			   CANVAS_ATT_VALUE_MIN, cube_min,
			   CANVAS_ATT_VALUE_MAX, cube_max,
			   CANVAS_ATT_END);
    kwin_resize (canvas_get_pixcanvas (aux_canvas), TRUE, 0, 0, 0, 0);
    izoomwinpopup = XtNameToWidget (aux_image_display, "izoomwinpopup");
    XtVaSetValues (izoomwinpopup,
		   XkwNiarray, cube_arr,
		   NULL);
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
    viewimg_set_attributes (channel0_vimage,
			    VIEWIMG_VATT_VALUE_MIN, cube_min,
			    VIEWIMG_VATT_VALUE_MAX, cube_max,
			    VIEWIMG_VATT_END);
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
    if ( (image_mode == IMAGE_MODE_MOM0) ||
	 (image_mode == IMAGE_MODE_MOM1) )
    {
	main_ap = cube_ap;
    }
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
    KPixCanvas pixcanvas;
    flag visible;
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
    viewimg_track_compute (vimage, value, value_type, x, y, x_lin, y_lin,
			   main_ap, pix_string, world_string,
			   extra_string, &pointer_x_index, &pointer_y_index);
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
    pixcanvas = canvas_get_pixcanvas (magnifier_canvas);
    kwin_get_attributes (pixcanvas,
			 KWIN_ATT_VISIBLE, &visible,
			 KWIN_ATT_END);
    if (visible) kwin_resize (pixcanvas, FALSE, 0, 0, 0, 0);
    return (TRUE);
}   /*  End Function track_canvas_event  */

static void toggle_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [PURPOSE] This routine is called when the user clicks a toggle.
*/
{
    *(flag *) client_data = (iaddr) call_data ? TRUE : FALSE;
}   /*  End Function toggle_cbk   */

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
    extern KWorldCanvas aux_canvas;
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
    /*static char function_name[] = "show_image";*/

    if (vimage == NULL) return;
    viewimg_make_active (vimage);
}   /*  End Function show_image  */

static void show_mom0 (KWorldCanvas canvas)
/*  [SUMMARY] Show the 0th moment map.
    <canvas> The world canvas.
    [RETURNS] Nothing.
*/
{
    double min, max;
    extern ViewableImage mom0_vimage;
    /*static char function_name[] = "show_mom0";*/

    if (mom0_vimage == NULL) return;
    viewimg_get_attributes (mom0_vimage,
			    VIEWIMG_VATT_VALUE_MIN, &min,
			    VIEWIMG_VATT_VALUE_MAX, &max,
			    VIEWIMG_VATT_END);
    if (min >= max)
    {
	fprintf (stderr, "0th moment image contains only: %e: ignoring\n",
		 min);
	return;
    }
    viewimg_make_active (mom0_vimage);
}   /*  End Function show_mom0  */

static void show_mom1 (KWorldCanvas canvas)
/*  [SUMMARY] Show the 1st moment map.
    <canvas> The world canvas.
    [RETURNS] Nothing.
*/
{
    double min, max;
    extern ViewableImage mom1_vimage;
    /*static char function_name[] = "show_mom1";*/

    if (mom1_vimage == NULL) return;
    viewimg_get_attributes (mom1_vimage,
			    VIEWIMG_VATT_VALUE_MIN, &min,
			    VIEWIMG_VATT_VALUE_MAX, &max,
			    VIEWIMG_VATT_END);
    if (min >= max)
    {
	fprintf (stderr, "1st moment image contains only: %e: ignoring\n",
		 min);
	return;
    }
    viewimg_make_active (mom1_vimage);
}   /*  End Function show_mom1  */

static void moment_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [SUMMARY] Moment computation callback.
    <w> The widget the event occurred on.
    <client_data> Client data.
    <call_data> Call data.
    [RETURNS] Nothing.
*/
{
    double min, max;
    extern iarray cube_arr, mom0_arr, mom1_arr;
    extern KWorldCanvas main_canvas, aux_canvas;
    extern ViewableImage mom0_vimage, mom1_vimage;
    extern unsigned int image_mode;
    /*static char function_name[] = "moment_cbk";*/

    if (cube_arr == NULL) return;
    iarray_min_max (mom0_arr, CONV_CtoR_REAL, &min, &max);
    viewimg_set_attributes (mom0_vimage,
			    VIEWIMG_VATT_VALUE_MIN, min,
			    VIEWIMG_VATT_VALUE_MAX, max,
			    VIEWIMG_VATT_END);
    viewimg_register_data_change (mom0_vimage);
    iarray_min_max (mom1_arr, CONV_CtoR_REAL, &min, &max);
    viewimg_set_attributes (mom1_vimage,
			    VIEWIMG_VATT_VALUE_MIN, min,
			    VIEWIMG_VATT_VALUE_MAX, max,
			    VIEWIMG_VATT_END);
    viewimg_register_data_change (mom1_vimage);
    if (image_mode == IMAGE_MODE_MOM0) show_mom0 (main_canvas);
    if (image_mode == IMAGE_MODE_MOM1) show_mom1 (main_canvas);
    kwin_resize (canvas_get_pixcanvas (aux_canvas), TRUE, 0, 0, 0, 0);
}   /*  End Function moment_cbk  */

static void show_trace_toggle_cbk (Widget w, XtPointer client_data,
				   XtPointer call_data)
/*  [PURPOSE] This routine is called when the user clicks the show trace toggle
*/
{
    extern KWorldCanvas aux_canvas;

    *(flag *) client_data = (iaddr) call_data ? TRUE : FALSE;
    kwin_resize (canvas_get_pixcanvas (aux_canvas), TRUE, 0, 0, 0, 0);
}   /*  End Function show_trace_toggle_cbk   */

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

static void show_channel0 (KWorldCanvas canvas)
/*  [SUMMARY] Show the 0th channel map.
    <canvas> The world canvas.
    [RETURNS] Nothing.
*/
{
    extern ViewableImage channel0_vimage;
    /*static char function_name[] = "show_channel0";*/

    if (channel0_vimage == NULL) return;
    viewimg_make_active (channel0_vimage);
}   /*  End Function show_channel0  */

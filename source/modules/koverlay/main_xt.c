/*  main_xt.c

    Main file for  koverlay  (X11 image+contour display tool for Karma).

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
    This Karma module will enable on-screen display of an image overlayed with
    contours from another image/cube.
    This module runs on an X11 server.


    Written by      Richard Gooch   13-OCT-1996: Copied from  kview/main_xt.c

    Last updated by Richard Gooch   16-OCT-1996: Renamed to <koverlay>.


*/
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Label.h>
#include <karma.h>
#define NEW_WIN_SCALE
#include <k_event_codes.h>
#include <karma_foreign.h>
#include <karma_overlay.h>
#include <karma_viewimg.h>
#include <karma_contour.h>
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
#include <karma_a.h>
#include <Xkw/ImageDisplay.h>
#include <Xkw/Filewin.h>
#include <Xkw/TracePopup.h>
#ifdef OS_SunOS
#  include <floatingpoint.h>
#endif
#include "koverlay.h"


#define VERSION "1.0.0"


/*  File: contour_xt.c  */
EXTERN_FUNCTION (Widget create_contour_popup, (Widget main_shell) );

/*  Private functions  */
STATIC_FUNCTION (void image_fileselect_cbk, (Widget w, XtPointer client_data,
					     XtPointer call_data) );
STATIC_FUNCTION (flag image_dirselect_cbk,
		 (Widget w, void *info, CONST char *dirname) );
STATIC_FUNCTION (void contour_fileselect_cbk, (Widget w, XtPointer client_data,
					       XtPointer call_data) );
STATIC_FUNCTION (flag contour_dirselect_cbk,
		 (Widget w, void *info, CONST char *dirname) );
STATIC_FUNCTION (void load_contour_and_setup, (CONST char *filename) );
STATIC_FUNCTION (track_canvas_event,
		 (ViewableImage vimage, double x, double y,
		  void *value, unsigned int event_code,
		  void *e_info, void **f_info,
		  double x_lin, double y_lin, unsigned int value_type) );
STATIC_FUNCTION (void new_frame_cbk, (Widget w, XtPointer client_data,
				      XtPointer call_data) );


/*  Private data  */
String fallback_resources[] =
{
    "Koverlay*pseudoColourCanvas*background:            black",
    "Koverlay*directColourCanvas*background:            black",
    "Koverlay*trueColourCanvas*background:              black",
    "Koverlay*pseudoColourStereoCanvas*background:      black",
    "Koverlay*directColourStereoCanvas*background:      black",
    "Koverlay*trueColourStereoCanvas*background:        black",
    "Koverlay*AnimateControl*positionCanvas.background: black",
    "Koverlay*pseudoColourCanvas*foreground:            white",
    "Koverlay*Command*background:                       grey70",
    "Koverlay*Repeater*background:                      grey70",
    "Koverlay*Ktoggle*background:                       grey80",
    "Koverlay*closeButton*background:                   grey90",
    "Koverlay*ChoiceMenu.background:                    turquoise",
    "Koverlay*ExclusiveMenu.background:                 turquoise",
    "Koverlay*Value*background:                         #d0a0a0",
    "Koverlay*ImageDisplay*quit*background:             orange",
    "Koverlay*background:                               aquamarine",
    "Koverlay*SimpleSlider.foreground:                  Sea Green",
    "Koverlay*ImageDisplay*trackLabel0*font:            8x13bold",
    "Koverlay*ImageDisplay*trackLabel1*font:            8x13bold",
    "Koverlay*ImageDisplay*trackLabel2*font:            8x13bold",
    "Koverlay*ImageDisplay*zoomMenu*font:               10x20",
    "Koverlay*ImageDisplay*crosshairMenu*font:          10x20",
    "Koverlay*ImageDisplay*exportMenu*theMenu*font:     10x20",
    "Koverlay*AnimateControl*modeMenu*font:             10x20",
    "Koverlay*ImageDisplay.Command.font:                7x13bold",
    "Koverlay*ImageDisplay.applicationBox.Command.font: 7x13bold",
    "Koverlay*ImageDisplay.Ktoggle.font:                7x13bold",
    "Koverlay*ImageDisplay.ChoiceMenu.font:             7x13bold",
    "Koverlay*ImageDisplay.exportMenu.menuButton.font:  7x13bold",
    "Koverlay*ImageDisplay*zoomMenu*Unzoom*foreground:  red",
    "Koverlay*Dataclip.AutoValueScale:                  False",
    "Koverlay*SimpleSlider.borderWidth:                 0",
    "Koverlay*font:                                     9x15bold",
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
static ViewableImage image = NULL;
static ViewableImage magnified_image = NULL;
static iarray pseudo_arr = NULL;
static ViewableImage *movie = NULL;
static ViewableImage *magnified_movie = NULL;
static unsigned int num_vimages = 0;
static double pseudo_scale = 1.0;
static double pseudo_offset = 0.0;
static Widget trace_winpopup = NULL;


int main (int argc, char **argv)
{
    KWorldCanvas wc_pseudo, wc_direct, wc_true;
    KOverlayList olist;
    XtAppContext app_context;
    Widget filewin, filepopup, animate_control, w, box, contour_btn;
    Display *dpy;
    char txt[STRING_LENGTH];
    extern KwcsAstro image_ap;
    extern Widget main_shell, image_display, trace_winpopup;
    extern char module_name[STRING_LENGTH + 1];

#ifdef SIGFPE_ABORT
	ieee_handler ("set", "division", SIGFPE_ABORT);
#endif
    /*  Initialise module  */
    im_register_module_name ("koverlay");
    im_register_module_version_date (VERSION);
    im_register_lib_version (KARMA_VERSION);
    /*  Start up Xt  */
    sprintf (txt, "%s v%s", module_name, VERSION);
    main_shell = XtVaAppInitialize (&app_context, "Koverlay",
				    Options, XtNumber (Options),
				    &argc, argv, fallback_resources,
				    XtNtitle, txt,
				    NULL, 0);
    xtmisc_set_icon (main_shell, ic_write_kimage_icon);
    /*  Initialise communications  */
    chx_register_app_context (app_context);
    conn_register_managers ( ( flag (*) () ) chx_manage,
			     ( void (*) () ) chx_unmanage,
			     ( void (*) () ) NULL );
    dpy = XtDisplay (main_shell);
    setup_comms (dpy);
    image_display = XtVaCreateManagedWidget ("topForm",
					     imageDisplayWidgetClass,
					     main_shell,
					     XtNborderWidth, 0,
					     XkwNenableAnimation, True,
					     XkwNautoIntensityScale, False,
					     XkwNnumTrackLabels, 3,
					     NULL);
    if ( ( w = XtNameToWidget (image_display, "filesButton") ) != NULL )
    {
	/*  Force main "Files" button to be re-labelled as "Cubes"  */
	XtVaSetValues (w,
		       XtNlabel, "Images",
		       NULL);
    }
    filepopup = XtNameToWidget (image_display, "filewinPopup");
    filewin = XtNameToWidget (filepopup, "form.selector");
    animate_control = XtNameToWidget (image_display, "animatepopup");
    XtVaSetValues (filepopup,
		   XkwNfilenameTester, XkwFilewinStandardFileTester_nD,
		   NULL);
    XtAddCallback (filepopup, XkwNfileSelectCallback, image_fileselect_cbk,
		   NULL);
    XkwFilewinRegisterDirCbk (filewin, image_dirselect_cbk,
			      (Widget) filepopup);
    /*  Get the application box  */
    box = XtNameToWidget (image_display, "applicationBox");
    /*  Create the Image file selector and popup button  */
    filepopup = XtVaCreatePopupShell ("filewinPopup", filepopupWidgetClass,
				      main_shell,
				      XtNtitle,
				      "Koverlay Contour Files Selector",
				      XtNx, 0,
				      XtNy, 0,
				      XkwNautoPopdown, True,
				      NULL);
    w = XtVaCreateManagedWidget ("filesButton", commandWidgetClass, box,
				 XtNlabel, "Contour Files",
				 NULL);
    XtAddCallback (w, XtNcallback, xtmisc_popup_cbk, filepopup);
    filewin = XtNameToWidget (filepopup, "form.selector");
    XtVaSetValues (filepopup,
		   XkwNfilenameTester, XkwFilewinStandardFileTester_nD,
		   NULL);
    XtAddCallback (filepopup, XkwNfileSelectCallback, contour_fileselect_cbk,
		   NULL);
    XkwFilewinRegisterDirCbk (filewin, contour_dirselect_cbk,
			      (Widget) filepopup);
    contour_btn = XtVaCreateManagedWidget ("button", commandWidgetClass, box,
					   XtNlabel, "Contour Controls",
					   NULL);
    XtRealizeWidget (main_shell);
    XtAddCallback (animate_control, XkwNnewFrameCallback, new_frame_cbk, NULL);
    XtVaGetValues (image_display,
		   XkwNpseudoColourCanvas, &wc_pseudo,
		   XkwNdirectColourCanvas, &wc_direct,
		   XkwNtrueColourCanvas, &wc_true,
		   NULL);
    if (wc_pseudo != NULL)
    {
	canvas_use_astro_transform (wc_pseudo, &image_ap);
	viewimg_register_position_event_func (wc_pseudo,
					      ( flag (*)() )track_canvas_event,
					      (void *) image_display);
	contour_init (wc_pseudo,
		      CONTOUR_CANVAS_ATT_COLOURNAME, "green",
		      CONTOUR_CANVAS_ATT_END);
    }
    if (wc_direct != NULL)
    {
	canvas_use_astro_transform (wc_direct, &image_ap);
	viewimg_register_position_event_func (wc_direct,
					      ( flag (*)() )track_canvas_event,
					      (void *) image_display);
	contour_init (wc_direct,
		      CONTOUR_CANVAS_ATT_COLOURNAME, "green",
		      CONTOUR_CANVAS_ATT_END);
    }
    if (wc_true != NULL)
    {
	canvas_use_astro_transform (wc_true, &image_ap);
	viewimg_register_position_event_func (wc_true,
					      ( flag (*)() )track_canvas_event,
					      (void *) image_display);
	contour_init (wc_true,
		      CONTOUR_CANVAS_ATT_COLOURNAME, "green",
		      CONTOUR_CANVAS_ATT_END);
    }
    if ( (argc == 2) && (strcmp (argv[1], "-overlay") == 0) )
    {
	fprintf (stderr, "Creating overlay list...\n");
	olist = overlay_create_list ( (void *) NULL );
	overlay_specify_canvas (olist, wc_pseudo);
	overlay_associate_display_canvas (olist, wc_pseudo);
    }
    trace_winpopup = XtVaCreatePopupShell ("tracewinpopup",
					   tracePopupWidgetClass, main_shell,
					   XtNtitle, "Profile Window",
					   NULL);
    w = create_contour_popup (main_shell);
    XtAddCallback (contour_btn, XtNcallback, xtmisc_popup_cbk, w);
    XtAppMainLoop (app_context);
    return (RV_OK);
}   /*  End Function main   */

void load_image_and_setup (CONST char *filename)
/*  [SUMMARY] Load and display an image/movie file and setup widget resources.
    <filename> The file to load.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    KWorldCanvas pseudo_canvas, direct_canvas, true_canvas, rgb_canvas;
    KWorldCanvas mag_pseudo_canvas, mag_direct_canvas, mag_true_canvas;
    KWorldCanvas mag_rgb_canvas;
    unsigned int num_frames;
    double min, max;
    Widget izoomwinpopup, animate_winpopup;
    char *ptr;
    char stripped_filename[STRING_LENGTH];
    extern ViewableImage image, magnified_image;
    extern iarray pseudo_arr;
    extern double pseudo_scale, pseudo_offset;
    extern ViewableImage *movie, *magnified_movie;
    extern unsigned int num_vimages, num_cimages;
    extern Widget main_shell, image_display;
    extern Widget trace_winpopup;
    static char title_name[STRING_LENGTH];

    strcpy (stripped_filename, filename);
    if ( ( ptr = strrchr (stripped_filename, '.') ) != NULL )
    {
	if (strcmp (ptr, ".kf") == 0) *ptr = '\0';
    }
    XtVaGetValues (image_display,
		   XkwNpseudoColourCanvas, &pseudo_canvas,
		   XkwNdirectColourCanvas, &direct_canvas,
		   XkwNtrueColourCanvas, &true_canvas,
		   XkwNmagnifierPseudoColourCanvas, &mag_pseudo_canvas,
		   XkwNmagnifierDirectColourCanvas, &mag_direct_canvas,
		   XkwNmagnifierTrueColourCanvas, &mag_true_canvas,
		   NULL);
    izoomwinpopup = XtNameToWidget (image_display, "izoomwinpopup");
    animate_winpopup = XtNameToWidget (image_display, "animatepopup");
    rgb_canvas = (direct_canvas == NULL) ? true_canvas : direct_canvas;
    mag_rgb_canvas = (mag_direct_canvas == NULL) ?
	mag_true_canvas : mag_direct_canvas;
    if ( !load_image (filename, pseudo_canvas, rgb_canvas,
		      mag_pseudo_canvas, mag_rgb_canvas,
		      &pseudo_arr, &image, &movie,
		      &magnified_image, &magnified_movie,
		      &num_vimages, &min, &max) ) return;
    sprintf (title_name, "koverlay v%s  file: %s\n", VERSION, filename);
    XtVaSetValues (main_shell,
		   XtNtitle, title_name,
		   NULL);
    /*  Deal with the animate widget  */
    num_frames = (num_vimages > num_cimages) ? num_vimages : num_cimages;
    XtVaSetValues (animate_winpopup,
		   XkwNnumFrames, num_frames,
		   NULL);
    if (num_frames > 0) XtPopup (animate_winpopup, XtGrabNone);
    else XtPopdown (animate_winpopup);
    XtVaSetValues (image_display,
		   XkwNimageName, stripped_filename,
		   NULL);
    XtVaSetValues (izoomwinpopup,
		   XkwNiarray, pseudo_arr,
		   NULL);
    if (pseudo_arr == NULL)
    {
	/*  RGB movie  */
	if (rgb_canvas != NULL)
	{
	    XtVaSetValues (image_display,
			   XkwNvisibleCanvas, rgb_canvas,
			   NULL);
	    return;
	}
    }
    else
    {
	/*  PseudoColour movie  */
	XtVaSetValues (image_display,
		       XkwNvisibleCanvas, pseudo_canvas,
		       NULL);
	iarray_get_data_scaling (pseudo_arr, &pseudo_scale, &pseudo_offset);
	if ( (iarray_num_dim (pseudo_arr) == 3) &&
	     (iarray_type (pseudo_arr) == K_FLOAT) )
	{
	    /*  Is a float cube: popup and setup the TracePopup widget  */
	    XkwTracePopupNewArray (trace_winpopup, pseudo_arr, min, max);
	    XtPopup (trace_winpopup, XtGrabNone);
	}
    }
}   /*  End Function load_image_and_setup  */

void refresh_canvas ()
/*  [SUMMARY] Refresh the active canvas.
    [RETURNS] Nothing.
*/
{
    extern Widget image_display;

    XkwImageDisplayRefresh (image_display, TRUE);
}   /*  End Function refresh_canvas  */


/*  Private routines follow  */

static void image_fileselect_cbk (Widget w, XtPointer client_data,
			      XtPointer call_data)
{
    char *filename;

    filename = (char *) call_data;
    load_image_and_setup (filename);
}   /*  End Function image_fileselect_cbk  */

static flag image_dirselect_cbk (Widget w, void *info, CONST char *dirname)
{
    if ( !foreign_miriad_test (dirname) ) return (FALSE);
    XtPopdown ( (Widget) info );
    XSync (XtDisplay (w), False);
    load_image_and_setup (dirname);
    return (TRUE);
}   /*  End Function image_dirselect_cbk  */

static void contour_fileselect_cbk (Widget w, XtPointer client_data,
			      XtPointer call_data)
{
    char *filename;

    filename = (char *) call_data;
    load_contour_and_setup (filename);
}   /*  End Function contour_fileselect_cbk  */

static flag contour_dirselect_cbk (Widget w, void *info, CONST char *dirname)
{
    if ( !foreign_miriad_test (dirname) ) return (FALSE);
    XtPopdown ( (Widget) info );
    XSync (XtDisplay (w), False);
    load_contour_and_setup (dirname);
    return (TRUE);
}   /*  End Function contour_dirselect_cbk  */

static void load_contour_and_setup (CONST char *filename)
/*  [SUMMARY] Load and display a contour file and setup widget resources.
    <filename> The file to load.
    [RETURNS] Nothing.
*/
{
    KWorldCanvas pseudo_canvas, direct_canvas, true_canvas, rgb_canvas;
    unsigned int num_frames;
    Widget animate_winpopup;
    extern unsigned int num_vimages, num_cimages;
    extern double contour_arr_min, contour_arr_max;
    extern Widget image_display;
    static iarray contour_arr = NULL;

    XtVaGetValues (image_display,
		   XkwNpseudoColourCanvas, &pseudo_canvas,
		   XkwNdirectColourCanvas, &direct_canvas,
		   XkwNtrueColourCanvas, &true_canvas,
		   NULL);
    rgb_canvas = (direct_canvas == NULL) ? true_canvas : direct_canvas;
    if ( !load_contour (filename, &contour_arr, pseudo_canvas, rgb_canvas,
			&contour_arr_min, &contour_arr_max) ) return;
    XkwImageDisplayRefresh (image_display, TRUE);
    /*  Deal with the animate widget  */
    animate_winpopup = XtNameToWidget (image_display, "animatepopup");
    num_frames = (num_vimages > num_cimages) ? num_vimages : num_cimages;
    XtVaSetValues (animate_winpopup,
		   XkwNnumFrames, num_frames,
		   NULL);
    if (num_frames > 0) XtPopup (animate_winpopup, XtGrabNone);
    else XtPopdown (animate_winpopup);
}   /*  End Function load_contour_and_setup  */

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
    unsigned int hdim, vdim, rdim;
    unsigned int num_restr, count;
    unsigned long pointer_x_index, pointer_y_index, r_index;
    Widget image_display = (Widget) *f_info;
    Widget first_track_label, second_track_label, third_track_label;
    unsigned char *rgb_ptr = (unsigned char *) value;
    char *xlabel, *ylabel;
    char **restr_names;
    double *restr_values;
    array_desc *arr_desc;
    uaddr coords[2];
    char txt[STRING_LENGTH];
    char value_string[STRING_LENGTH], index_string[STRING_LENGTH];
    char world_string[STRING_LENGTH], extra_string[STRING_LENGTH];
    unsigned int dim_indices[2];
    extern iarray pseudo_arr;
    extern KwcsAstro image_ap;
    extern double pseudo_scale, pseudo_offset;
    extern Widget trace_winpopup;
    static char function_name[] = "track_canvas_event";

    if (event_code != K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    /*  Create value string  */
    switch (value_type)
    {
      case K_DCOMPLEX:
	/*  Compute value  */
	iarray_format_value (pseudo_arr, value_string,
			     *(double *) value, pseudo_scale, pseudo_offset);
	break;
      case K_UB_RGB:
	sprintf (value_string, "RGB: %u %u %u",
			rgb_ptr[0], rgb_ptr[1], rgb_ptr[2]);
	break;
      default:
	fprintf (stderr, "Illegal type: %u\n", value_type);
	a_prog_bug (function_name);
	break;
    }
    canvas_get_specification (viewimg_get_worldcanvas (vimage),
			      &xlabel, &ylabel, &num_restr,
			      &restr_names, &restr_values);
    viewimg_get_attributes (vimage,
			    VIEWIMG_VATT_ARRAY_DESC, &arr_desc,
			    VIEWIMG_VATT_HDIM, &hdim,
			    VIEWIMG_VATT_VDIM, &vdim,
			    VIEWIMG_VATT_END);
    /*  Convert linear world co-ordinates to array indices and display  */
    pointer_x_index = ds_get_coord_num (arr_desc->dimensions[hdim], x_lin,
					SEARCH_BIAS_CLOSEST);
    pointer_y_index = ds_get_coord_num (arr_desc->dimensions[vdim], y_lin,
					SEARCH_BIAS_CLOSEST);
    if ( (pseudo_arr != NULL) && (iarray_num_dim (pseudo_arr) == 3) &&
	 (iarray_type (pseudo_arr) == K_FLOAT) )
    {
	dim_indices[0] = 2;
	dim_indices[1] = 1;
	coords[0] = pointer_x_index;
	coords[1] = pointer_y_index;
	XkwTracePopupShowTrace (trace_winpopup, dim_indices, coords);
    }
    sprintf (index_string, "x: %lu  y: %lu  ",
	     pointer_x_index, pointer_y_index);
    /*  Add any restriction information  */
    for (count = 0; count < num_restr; ++count)
    {
	if ( ( rdim = ds_f_dim_in_array (arr_desc, restr_names[count]) )
	     >= arr_desc->num_dimensions ) continue;
	r_index = ds_get_coord_num (arr_desc->dimensions[rdim],
				    restr_values[count], SEARCH_BIAS_CLOSEST);
	sprintf (txt, "z%u: %lu  ", rdim, r_index);
	strcat (index_string, txt);
    }
    strcat (index_string, value_string);
    first_track_label = XtNameToWidget (image_display, "trackLabel0");
    second_track_label = XtNameToWidget (image_display, "trackLabel1");
    third_track_label = XtNameToWidget (image_display, "trackLabel2");
    XtVaSetValues (first_track_label, XtNlabel, index_string, NULL);
    /*  Now display the world co-ordinate information  */
    if (image_ap == NULL)
    {
	sprintf (world_string, "%5e %s  %5e %s  ", x, xlabel, y, ylabel);
	/*  Add any restriction information  */
	for (count = 0; count < num_restr; ++count)
	{
	    sprintf (txt, "%5e %s  ", restr_values[count], restr_names[count]);
	    strcat (world_string, txt);
	}
    }
    else
    {
	wcs_astro_format_all (image_ap, world_string,
			      xlabel, x_lin, ylabel, y_lin, NULL, 0.0,
			      num_restr, (CONST char **) restr_names,
			      restr_values, extra_string);
	XtVaSetValues (third_track_label, XtNlabel, extra_string, NULL);
    }
    XtVaSetValues (second_track_label, XtNlabel, world_string, NULL);
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

static void new_frame_cbk (Widget w, XtPointer client_data,
			   XtPointer call_data)
/*  This is the new frame callback.
*/
{
    int frame_number = *(int *) call_data;
    extern unsigned int num_vimages, num_cimages;
    extern Widget image_display;
    extern ViewableImage *movie, *magnified_movie;
    extern KContourImage *pc_cimages, *rgb_cimages;

    if (frame_number < num_cimages)
    {
	if ( (pc_cimages != NULL) && (pc_cimages[frame_number] != NULL) )
	{
	    contour_set_active (pc_cimages[frame_number], TRUE, FALSE, FALSE,
				TRUE);
	}
	if ( (rgb_cimages != NULL) && (rgb_cimages[frame_number] != NULL) )
	{
	    contour_set_active (rgb_cimages[frame_number], TRUE, FALSE, FALSE,
				TRUE);
	}
    }
    if (frame_number < num_vimages)
    {
	if ( (movie != NULL) && (movie[frame_number] != NULL) )
	{
	    viewimg_set_active (movie[frame_number], FALSE);
	}
	if ( (magnified_movie != NULL) && (magnified_movie[frame_number] !=
					   NULL) )
	{
	    viewimg_set_active (magnified_movie[frame_number], FALSE);
	}
    }
    XkwImageDisplayRefresh (image_display, FALSE);
}   /*  End Function new_frame_cbk   */

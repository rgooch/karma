/*  main_xt.c

    Main file for  kview  (X11 image/movie display tool for Karma).

    Copyright (C) 1995-1996  Richard Gooch

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
    This Karma module will enable on-screen imaging of a single arrayfile of
    the general data structure format. The arrayfile must contain a 2 or 3
    dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   30-AUG-1995: Copied from  kmovie_main.c

    Updated by      Richard Gooch   1-SEP-1995: Stripped <kcmap_>
  initialisation.

    Updated by      Richard Gooch   7-SEP-1995: Made image name automatically
  allocated rather than statically allocated because ImageDisplay widget now
  makes a private copy.

    Updated by      Richard Gooch   8-SEP-1995: Added experimental fullscreen
  option.

    Updated by      Richard Gooch   28-SEP-1995: Added Miriad Image support.

    Updated by      Richard Gooch   29-FEB-1996: Made background colour for
  both mono and stereo canvases black.

    Updated by      Richard Gooch   17-MAR-1996: Added icon.

    Updated by      Richard Gooch   30-APR-1996: Added -verbose option.

    Updated by      Richard Gooch   4-MAY-1996: Switched to KtoggleWidget
  resources.

    Updated by      Richard Gooch   16-MAY-1996: Made use of
  XkwNautoIntensityScale resource for ImageDisplay widget.

    Updated by      Richard Gooch   5-JUN-1996: Added resources for sliders.

    Updated by      Richard Gooch   7-JUN-1996: Added filename to title.

    Updated by      Richard Gooch   11-JUN-1996: Added magnifier support.

    Updated by      Richard Gooch   15-JUN-1996: Tidied up astronomical
  projection support.

    Updated by      Richard Gooch   16-JUN-1996: Added check for *__SCALE and
  *__OFFSET data.

    Updated by      Richard Gooch   19-JUN-1996: Made use of
  <iarray_format_value> routine.

    Updated by      Richard Gooch   22-JUN-1996: Made use of
  <XkwFilewinStandardFileTester_nD> routine.

    Updated by      Richard Gooch   27-JUN-1996: Switched to
  <canvas_use_astro_transform>

    Updated by      Richard Gooch   18-JUL-1996: Fixed fatal bug with
  registering astro transform on DirectColour and TrueColour canvases.

    Updated by      Richard Gooch   3-SEP-1996: Restored magnifier which did
  not display under some conditions.

    Updated by      Richard Gooch   17-SEP-1996: Made use of TracePopup
  widget.

    Updated by      Richard Gooch   19-SEP-1996: Changed to special function
  call interface for setting new array for TracePopup widget.

    Updated by      Richard Gooch   1-OCT-1996: Fixed resource names for new
  ExportMenu widget.

    Last updated by Richard Gooch   27-OCT-1996: Added hostname and port number
  to title.


*/
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Label.h>
#include <karma.h>
#define NEW_WIN_SCALE
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
#include <karma_a.h>
#include <Xkw/ImageDisplay.h>
#include <Xkw/Filewin.h>
#include <Xkw/TracePopup.h>
#ifdef OS_SunOS
#  include <floatingpoint.h>
#endif

#define VERSION "1.5.2"


/*  External functions  */
/*  File: generic.c  */
EXTERN_FUNCTION (void setup_comms, (Display *display) );
EXTERN_FUNCTION (flag display_file,
		 (CONST char *inp_filename,
		  KWorldCanvas pseudo_canvas, KWorldCanvas rgb_canvas,
		  KWorldCanvas mag_pseudo_canvas, KWorldCanvas mag_rgb_canvas,
		  iarray *pseudo_arr,
		  ViewableImage *image, ViewableImage **movie,
		  ViewableImage *magnified_image,
		  ViewableImage **magnified_movie,
		  unsigned int *num_frames, double *min, double *max) );


/*  Local functions  */
EXTERN_FUNCTION (void load_and_setup, (CONST char *filename) );


/*  Private functions  */
STATIC_FUNCTION (void fileselect_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (flag dirselect_cbk,
		 (Widget w, void *info, CONST char *dirname) );
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
    "Kview*pseudoColourCanvas*background:            black",
    "Kview*directColourCanvas*background:            black",
    "Kview*trueColourCanvas*background:              black",
    "Kview*pseudoColourStereoCanvas*background:      black",
    "Kview*directColourStereoCanvas*background:      black",
    "Kview*trueColourStereoCanvas*background:        black",
    "Kview*AnimateControl*positionCanvas.background: black",
    "Kview*pseudoColourCanvas*foreground:            white",
    "Kview*Command*background:                       grey70",
    "Kview*Repeater*background:                      grey70",
    "Kview*Ktoggle*background:                       grey80",
    "Kview*closeButton*background:                   grey90",
    "Kview*ChoiceMenu.background:                    turquoise",
    "Kview*ExclusiveMenu.background:                 turquoise",
    "Kview*Value*background:                         #d0a0a0",
    "Kview*ImageDisplay*quit*background:             orange",
    "Kview*background:                               aquamarine",
    "Kview*SimpleSlider.foreground:                  Sea Green",
    "Kview*ImageDisplay*trackLabel0*font:            8x13bold",
    "Kview*ImageDisplay*trackLabel1*font:            8x13bold",
    "Kview*ImageDisplay*trackLabel2*font:            8x13bold",
    "Kview*ImageDisplay*zoomMenu*font:               10x20",
    "Kview*ImageDisplay*crosshairMenu*font:          10x20",
    "Kview*ImageDisplay*exportMenu*theMenu*font:     10x20",
    "Kview*AnimateControl*modeMenu*font:             10x20",
    "Kview*ImageDisplay.Command.font:                7x13bold",
    "Kview*ImageDisplay.Ktoggle.font:                7x13bold",
    "Kview*ImageDisplay.ChoiceMenu.font:             7x13bold",
    "Kview*ImageDisplay.exportMenu.menuButton.font:  7x13bold",
    "Kview*ImageDisplay*zoomMenu*Unzoom*foreground:  red",
    "Kview*SimpleSlider.borderWidth:                 0",
    "Kview*font:                                     9x15bold",
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
static unsigned int num_frames = 0;
static double pseudo_scale = 1.0;
static double pseudo_offset = 0.0;
static Widget trace_winpopup = NULL;


int main (int argc, char **argv)
{
    KWorldCanvas wc_pseudo, wc_direct, wc_true;
    KOverlayList olist;
    XtAppContext app_context;
    Widget filewin, filepopup, animate_control;
    Display *dpy;
    extern KwcsAstro astro_projection;
    extern Widget main_shell, image_display, trace_winpopup;
    extern char title_name[STRING_LENGTH];

#ifdef SIGFPE_ABORT
	ieee_handler ("set", "division", SIGFPE_ABORT);
#endif
    /*  Initialise module  */
    im_register_module_name ("kview");
    im_register_module_version_date (VERSION);
    im_register_lib_version (KARMA_VERSION);
    /*  Start up Xt  */
    main_shell = XtVaAppInitialize (&app_context, "Kview",
				    Options, XtNumber (Options),
				    &argc, argv, fallback_resources,
				    NULL, 0);
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
					     XkwNenableAnimation, True,
					     XkwNautoIntensityScale, False,
					     XkwNnumTrackLabels, 3,
					     NULL);
    filepopup = XtNameToWidget (image_display, "filewinPopup");
    filewin = XtNameToWidget (filepopup, "form.selector");
    animate_control = XtNameToWidget (image_display, "animatepopup");
    XtVaSetValues (filepopup,
		   XkwNfilenameTester, XkwFilewinStandardFileTester_nD,
		   NULL);
    XtAddCallback (filepopup, XkwNfileSelectCallback, fileselect_cbk, NULL);
    XkwFilewinRegisterDirCbk (filewin, dirselect_cbk,
				     (Widget) filepopup);
    XtRealizeWidget (main_shell);
    XtAddCallback (animate_control, XkwNnewFrameCallback, new_frame_cbk, NULL);
    XtVaGetValues (image_display,
		   XkwNpseudoColourCanvas, &wc_pseudo,
		   XkwNdirectColourCanvas, &wc_direct,
		   XkwNtrueColourCanvas, &wc_true,
		   NULL);
    if (wc_pseudo != NULL)
    {
	canvas_use_astro_transform (wc_pseudo, &astro_projection);
	viewimg_register_position_event_func (wc_pseudo,
					      ( flag (*)() )track_canvas_event,
					      (void *) image_display);
    }
    if (wc_direct != NULL)
    {
	canvas_use_astro_transform (wc_direct, &astro_projection);
	viewimg_register_position_event_func (wc_direct,
					      ( flag (*)() )track_canvas_event,
					      (void *) image_display);
    }
    if (wc_true != NULL)
    {
	canvas_use_astro_transform (wc_true, &astro_projection);
	viewimg_register_position_event_func (wc_true,
					      ( flag (*)() )track_canvas_event,
					      (void *) image_display);
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
    if ( !conn_controlled_by_cm_tool () ) XtPopup (filepopup, XtGrabNone);
    XtAppMainLoop (app_context);
    return (RV_OK);
}   /*  End Function main   */

void load_and_setup (CONST char *filename)
{
    KWorldCanvas pseudo_canvas, direct_canvas, true_canvas, rgb_canvas;
    KWorldCanvas mag_pseudo_canvas, mag_direct_canvas, mag_true_canvas;
    KWorldCanvas mag_rgb_canvas;
    double min, max;
    Widget izoomwinpopup, animate_winpopup;
    char *ptr;
    char stripped_filename[STRING_LENGTH];
    char title[STRING_LENGTH];
    extern ViewableImage image, magnified_image;
    extern iarray pseudo_arr;
    extern double pseudo_scale, pseudo_offset;
    extern ViewableImage *movie, *magnified_movie;
    extern unsigned int num_frames;
    extern Widget main_shell, image_display;
    extern Widget trace_winpopup;
    extern char title_name[STRING_LENGTH];

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
    if ( !display_file (filename, pseudo_canvas, rgb_canvas,
			mag_pseudo_canvas, mag_rgb_canvas,
			&pseudo_arr, &image, &movie,
			&magnified_image, &magnified_movie,
			&num_frames, &min, &max) ) return;
    sprintf (title, "%s  file: %s\n", title_name, filename);
    XtVaSetValues (main_shell,
		   XtNtitle, title,
		   NULL);
    /*  Deal with the animate widget  */
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
}   /*  End Function load_and_setup  */


/*  Private routines follow  */

static void fileselect_cbk (Widget w, XtPointer client_data,
			    XtPointer call_data)
{
    char *filename;

    filename = (char *) call_data;
    load_and_setup (filename);
}   /*  End Function fileselect_cbk  */

static flag dirselect_cbk (Widget w, void *info, CONST char *dirname)
{
    if ( !foreign_miriad_test (dirname) ) return (FALSE);
    XtPopdown ( (Widget) info );
    XSync (XtDisplay (w), False);
    load_and_setup (dirname);
    return (TRUE);
}   /*  End Function dirselect_cbk  */

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
    extern KwcsAstro astro_projection;
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
    if (astro_projection == NULL)
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
	wcs_astro_format_all (astro_projection, world_string,
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
    extern ViewableImage *movie, *magnified_movie;

    if ( (movie != NULL) && (movie[frame_number] != NULL) )
    {
	viewimg_make_active (movie[frame_number]);
    }
    if ( (magnified_movie != NULL) && (magnified_movie[frame_number] != NULL) )
    {
	viewimg_make_active (magnified_movie[frame_number]);
    }
}   /*  End Function new_frame_cbk   */

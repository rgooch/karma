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

    Last updated by Richard Gooch   27-JUN-1996: Switched to
  <canvas_use_astro_transform>


*/
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Command.h>
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
#include <karma_a.h>
#include <Xkw/ImageDisplay.h>
#include <Xkw/Filewin.h>
#include <Xkw/Ktoggle.h>
#ifdef OS_SunOS
#  include <floatingpoint.h>
#endif

#define VERSION "1.3"


/*  External functions  */
/*  File: generic.c  */
EXTERN_FUNCTION (void setup_comms, (Display *display) );
EXTERN_FUNCTION (iarray load_image,
		 (CONST char *inp_filename,
		  KWorldCanvas pseudo_canvas, KWorldCanvas mag_pseudo_canvas,
		  ViewableImage *image, ViewableImage *magnified_image) );
EXTERN_FUNCTION (iarray load_cube, (CONST char *inp_filename) );

/*  File: event.c  */
EXTERN_FUNCTION (flag endpoint_position_func,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info,
		  void **f_info, double x_lin, double y_lin) );


/*  Local functions  */
EXTERN_FUNCTION (void load_and_setup, (CONST char *filename) );
EXTERN_FUNCTION (void print_slice_info, (CONST char *window_string) );


/*  Private functions  */
STATIC_FUNCTION (void cubeselect_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (void imageselect_cbk, (Widget w, XtPointer client_data,
					XtPointer call_data) );
STATIC_FUNCTION (flag dirselect_cbk,
		 (Widget w, void *info, CONST char *dirname) );
STATIC_FUNCTION (track_canvas_event,
		 (ViewableImage vimage, double x, double y,
		  void *value, unsigned int event_code,
		  void *e_info, void **f_info,
		  double x_lin, double y_lin, unsigned int value_type) );
STATIC_FUNCTION (void toggle_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );


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
    "Kpvslice*ImageDisplay*zoomMenu*font:               10x20",
    "Kpvslice*ImageDisplay*crosshairMenu*font:          10x20",
    "Kpvslice*ImageDisplay*exportMenu*font:             10x20",
    "Kpvslice*AnimateControl*modeMenu*font:             10x20",
    "Kpvslice*ImageDisplay.cmapSize:                    100",
    "Kpvslice*ImageDisplay.Command.font:                7x13bold",
    "Kpvslice*ImageDisplay.applicationBox.Command.font: 7x13bold",
    "Kpvslice*ImageDisplay.applicationBox.Ktoggle.font: 7x13bold",
    "Kpvslice*ImageDisplay.ChoiceMenu.font:             7x13bold",
    "Kpvslice*ImageDisplay*zoomMenu*Unzoom*foreground:  red",
    "Kpvslice*Dataclip.AutoValueScale:                  False",
    "Kpvslice*SimpleSlider.borderWidth:                 0",
    "Kpvslice*font:                                     9x15bold",
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
static ViewableImage image = NULL;
static ViewableImage magnified_image = NULL;
static iarray image_arr = NULL;
static double image_scale = 1.0;
static double image_offset = 0.0;

/*  Global data  */
iarray cube_arr = NULL;
KwcsAstro image_ap = NULL;
KwcsAstro cube_ap = NULL;
flag auto_update = TRUE;


int main (int argc, char **argv)
{
    KWorldCanvas main_canvas, aux_canvas;
    KOverlayList olist;
    XtAppContext app_context;
    Widget filewin, filepopup, w, box, files_btn;
    Widget aux_shell;
    Display *dpy;
    extern flag auto_update;
    extern KwcsAstro image_ap;
    extern Widget main_shell, main_image_display;
    extern char *sys_errlist[];

#ifdef SIGFPE_ABORT
	ieee_handler ("set", "division", SIGFPE_ABORT);
#endif
    /*  Initialise module  */
    im_register_module_name ("Kpvslice");
    im_register_module_version_date (VERSION);
    im_register_lib_version (KARMA_VERSION);
    /*  Start up Xt  */
    main_shell = XtVaAppInitialize (&app_context, "Kpvslice",
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
    (void) XkwFilewinRegisterDirCbk (filewin, dirselect_cbk,
				     (Widget) filepopup);
    /*  Get the applicationg box  */
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
    (void) XkwFilewinRegisterDirCbk (filewin, dirselect_cbk,
				     (Widget) filepopup);
    /*  Create the auto update toggle  */
    w = XtVaCreateManagedWidget ("autoUpdateToggle", ktoggleWidgetClass, box,
				 XtNlabel, "Auto Update",
				 XtNstate, auto_update,
				 XkwNcrosses, FALSE,
				 NULL);
    XtAddCallback (w, XtNcallback, toggle_cbk, (XtPointer) &auto_update);
    XtRealizeWidget (main_shell);
    XtVaGetValues (main_image_display,
		   XkwNpseudoColourCanvas, &main_canvas,
		   NULL);
    if (main_canvas == NULL)
    {
	(void) fprintf (stderr, "PseudoColour canvas not found!\n");
	exit (RV_UNDEF_ERROR);
    }
    canvas_use_astro_transform (main_canvas, &image_ap);
    viewimg_register_position_event_func (main_canvas,
					  ( flag (*) () ) track_canvas_event,
					  (void *) main_image_display);
    if ( (argc == 2) && (strcmp (argv[1], "-overlay") == 0) )
    {
	(void) fprintf (stderr, "Creating overlay list...\n");
	olist = overlay_create_list ( (void *) NULL );
	overlay_specify_canvas (olist, main_canvas);
	(void) overlay_associate_display_canvas (olist, main_canvas);
    }
    aux_shell = XtVaCreatePopupShell ("auxShell", topLevelShellWidgetClass,
				      main_shell,
				      XtNtitle, "Image Window",
				      NULL);
    aux_image_display = XtVaCreateManagedWidget ("topForm",
						 imageDisplayWidgetClass,
						 aux_shell,
						 XtNborderWidth, 0,
						 XkwNenableAnimation, FALSE,
						 XkwNautoIntensityScale, FALSE,
						 XkwNshowQuitButton, FALSE,
						 NULL);
    XtRealizeWidget (aux_shell);
    XtVaGetValues (aux_image_display,
		   XkwNpseudoColourCanvas, &aux_canvas,
		   NULL);
    if (aux_canvas == NULL)
    {
	(void) fprintf (stderr, "PseudoColour canvas not found!\n");
	exit (RV_UNDEF_ERROR);
    }
    /*viewimg_register_position_event_func (aux_canvas,
					  ( flag (*) () ) track_canvas_event,
					  (void *) main_image_display);*/
    canvas_register_position_event_func (main_canvas,
					 ( flag (*) () )endpoint_position_func,
					 aux_canvas);
    XtPopup (aux_shell, XtGrabNone);
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
    extern ViewableImage image, magnified_image;
    extern iarray image_arr;
    extern double image_scale, image_offset;
    extern Widget main_shell, main_image_display;
    static char title_name[STRING_LENGTH];

    (void) strcpy (stripped_filename, filename);
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
    if ( ( arr = load_image (filename, pseudo_canvas, mag_pseudo_canvas,
			     &image, &magnified_image) ) == NULL ) return;
    if (image_arr != NULL) iarray_dealloc (image_arr);
    image_arr = arr;
    (void) sprintf (title_name, "kpvslice  file: %s\n", filename);
    XtVaSetValues (main_shell,
		   XtNtitle, title_name,
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
    iarray_get_data_scaling (image_arr, &image_scale, &image_offset);
}   /*  End Function load_and_setup  */

void print_slice_info (CONST char *window_string)
/*  [SUMMARY] Print information about the slice.
    <window_string> The string to display.
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
    iarray arr;
    double i_min, i_max;
    Widget izoomwinpopup;
    char *filename;
    extern iarray cube_arr;
    extern Widget aux_image_display;

    filename = (char *) call_data;
    if ( ( arr = load_cube (filename) ) == NULL ) return;
    if (cube_arr != NULL) iarray_dealloc (cube_arr);
    cube_arr = arr;
    if ( !iarray_min_max (cube_arr, CONV1_REAL, &i_min, &i_max) )
    {
	(void) fprintf (stderr, "Error computing min-max\n");
    }
    (void) fprintf (stderr, "Cube minimum: %e  maximum: %e\n", i_min, i_max);
    XtVaGetValues (aux_image_display,
		   XkwNpseudoColourCanvas, &aux_canvas,
		   NULL);
    canvas_set_attributes (aux_canvas,
			   CANVAS_ATT_VALUE_MIN, i_min,
			   CANVAS_ATT_VALUE_MAX, i_max,
			   CANVAS_ATT_END);
    kwin_resize (canvas_get_pixcanvas (aux_canvas), TRUE, 0, 0, 0, 0);
    izoomwinpopup = XtNameToWidget (aux_image_display, "izoomwinpopup");
    XtVaSetValues (izoomwinpopup,
		   XkwNiarray, cube_arr,
		   NULL);
}   /*  End Function cubeselect_cbk  */

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
    char txt[STRING_LENGTH];
    char value_string[STRING_LENGTH], index_string[STRING_LENGTH];
    char world_string[STRING_LENGTH], extra_string[STRING_LENGTH];
    extern iarray image_arr;
    extern KwcsAstro image_ap;
    extern double image_scale, image_offset;
    static char function_name[] = "track_canvas_event";

    if (event_code != K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    /*  Create value string  */
    switch (value_type)
    {
      case K_DCOMPLEX:
	/*  Compute value  */
	iarray_format_value (image_arr, value_string,
			     *(double *) value, image_scale, image_offset);
	break;
      case K_UB_RGB:
	(void) sprintf (value_string, "RGB: %u %u %u",
			rgb_ptr[0], rgb_ptr[1], rgb_ptr[2]);
	break;
      default:
	(void) fprintf (stderr, "Illegal type: %u\n", value_type);
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
    (void) sprintf (index_string, "x: %lu  y: %lu  ",
		    pointer_x_index, pointer_y_index);
    /*  Add any restriction information  */
    for (count = 0; count < num_restr; ++count)
    {
	if ( ( rdim = ds_f_dim_in_array (arr_desc, restr_names[count]) )
	     >= arr_desc->num_dimensions ) continue;
	r_index = ds_get_coord_num (arr_desc->dimensions[rdim],
				    restr_values[count], SEARCH_BIAS_CLOSEST);
	(void) sprintf (txt, "z%u: %lu  ", rdim, r_index);
	(void) strcat (index_string, txt);
    }
    (void) strcat (index_string, value_string);
    first_track_label = XtNameToWidget (image_display, "trackLabel0");
    second_track_label = XtNameToWidget (image_display, "trackLabel1");
    third_track_label = XtNameToWidget (image_display, "trackLabel2");
    XtVaSetValues (first_track_label, XtNlabel, index_string, NULL);
    /*  Now display the world co-ordinate information  */
    if (image_ap == NULL)
    {
	(void) sprintf (world_string, "%5e %s  %5e %s  ",
			x, xlabel, y, ylabel);
    }
    else
    {
	wcs_format_all (image_ap, world_string,
			xlabel, x_lin, ylabel, y_lin, NULL, 0.0,
			num_restr, (CONST char **) restr_names, restr_values,
			extra_string);
	XtVaSetValues (second_track_label, XtNlabel, world_string, NULL);
	XtVaSetValues (third_track_label, XtNlabel, extra_string, NULL);
	return (TRUE);
    }
    /*  Add any restriction information  */
    for (count = 0; count < num_restr; ++count)
    {
	(void) sprintf (txt, "%5e %s  ",
			restr_values[count], restr_names[count]);
	(void) strcat (world_string, txt);
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

static void toggle_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [PURPOSE] This routine is called when the user clicks a toggle.
*/
{
    *(flag *) client_data = (iaddr) call_data ? TRUE : FALSE;
}   /*  End Function toggle_cbk   */

/*  main_xt.c

    Main file for  koords  (X11 co-ordinate generator tool for Karma).

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
    This Karma module will enable interactive generation of an astronomical
    co-ordinate system on a target image, using a reference image.
    This module runs on an X11 server.


    Written by      Richard Gooch   14-OCT-1996: Copied from kshell/main_xt.c

    Updated by      Richard Gooch   17-OCT-1996: Updated pick sequencing.

    Last updated by Richard Gooch   28-OCT-1996: Added hostname and port number
  to title.


*/
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/cursorfont.h>
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
#include <Xkw/TracePopup.h>
#ifdef OS_SunOS
#  include <floatingpoint.h>
#endif
#include "koords.h"


#define VERSION "1.0.2"


/*  Private functions  */
STATIC_FUNCTION (void load_and_setup,
		 (CONST char *filename, iarray *array, KwcsAstro *ap,
		  ViewableImage *image, ViewableImage *magnified_image,
		  Widget shell, Widget image_display) );
STATIC_FUNCTION (void reference_select_cbk, (Widget w, XtPointer client_data,
					     XtPointer call_data) );
STATIC_FUNCTION (flag reference_dirselect_cbk,
                 (Widget w, void *info, CONST char *dirname) );
STATIC_FUNCTION (void target_select_cbk, (Widget w, XtPointer client_data,
					  XtPointer call_data) );
STATIC_FUNCTION (flag target_dirselect_cbk,
                 (Widget w, void *info, CONST char *dirname) );
STATIC_FUNCTION (track_ref_event,
		 (ViewableImage vimage, double x, double y,
		  void *value, unsigned int event_code,
		  void *e_info, void **f_info,
		  double x_lin, double y_lin, unsigned int value_type) );
STATIC_FUNCTION (track_tar_event,
		 (ViewableImage vimage, double x, double y,
		  void *value, unsigned int event_code,
		  void *e_info, void **f_info,
		  double x_lin, double y_lin, unsigned int value_type) );
STATIC_FUNCTION (void undo_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );


/*  Private data  */
String fallback_resources[] =
{
    "Koords.geometry:                                 +0+0",
    "Koords*pseudoColourCanvas*background:            black",
    "Koords*directColourCanvas*background:            black",
    "Koords*trueColourCanvas*background:              black",
    "Koords*pseudoColourStereoCanvas*background:      black",
    "Koords*directColourStereoCanvas*background:      black",
    "Koords*trueColourStereoCanvas*background:        black",
    "Koords*AnimateControl*positionCanvas.background: black",
    "Koords*pseudoColourCanvas*foreground:            white",
    "Koords*Command*background:                       grey70",
    "Koords*Repeater*background:                      grey70",
    "Koords*Ktoggle*background:                       grey80",
    "Koords*closeButton*background:                   grey90",
    "Koords*ChoiceMenu.background:                    turquoise",
    "Koords*ExclusiveMenu.background:                 turquoise",
    "Koords*Value*background:                         #d0a0a0",
    "Koords*ImageDisplay*quit*background:             orange",
    "Koords*background:                               aquamarine",
    "Koords*SimpleSlider.foreground:                  Sea Green",
    "Koords*tarShell.geometry:                        -0+0",
    "Koords*ImageDisplay*trackLabel0*font:            8x13bold",
    "Koords*ImageDisplay*trackLabel1*font:            8x13bold",
    "Koords*ImageDisplay*trackLabel2*font:            8x13bold",
    "Koords*ImageDisplay*trackLabel3*font:            8x13bold",
    "Koords*ExclusiveMenu.font:                       8x13bold",
    "Koords*ImageDisplay*zoomMenu*font:               10x20",
    "Koords*ImageDisplay*crosshairMenu*font:          10x20",
    "Koords*ImageDisplay*exportMenu*theMenu*font:     10x20",
    "Koords*AnimateControl*modeMenu*font:             10x20",
    "Koords*imageModeMenu*font:                       10x20",
    "Koords*ImageDisplay.cmapSize:                    100",
    "Koords*ImageDisplay.Command.font:                7x13bold",
    "Koords*ImageDisplay.applicationBox.Command.font: 7x13bold",
    "Koords*ImageDisplay.applicationBox.Ktoggle.font: 7x13bold",
    "Koords*ImageDisplay.ChoiceMenu.font:             7x13bold",
    "Koords*ImageDisplay.exportMenu.menuButton.font:  7x13bold",
    "Koords*ImageDisplay*zoomMenu*Unzoom*foreground:  red",
    "Koords*Dataclip.AutoValueScale:                  False",
    "Koords*SimpleSlider.borderWidth:                 0",
    "Koords*font:                                     9x15bold",
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
static Widget ref_image_display = NULL;
static KWorldCanvas ref_canvas = NULL;
static Widget tar_shell = NULL;
static Widget tar_image_display = NULL;
static KWorldCanvas tar_canvas = NULL;


/*  Public functions follow  */

int main (int argc, char **argv)
{
    KOverlayList olist;
    Cursor cursor;
    XtAppContext app_context;
    Widget filewin, filepopup, w, box;
    Display *dpy;
    extern KwcsAstro reference_ap, target_ap;
    extern Widget main_shell, ref_image_display;
    extern char title_name[STRING_LENGTH];

#ifdef SIGFPE_ABORT
	ieee_handler ("set", "division", SIGFPE_ABORT);
#endif
    /*  Initialise module  */
    im_register_module_name ("koords");
    im_register_module_version_date (VERSION);
    im_register_lib_version (KARMA_VERSION);
    /*  Start up Xt  */
    main_shell = XtVaAppInitialize (&app_context, "Koords",
				    Options, XtNumber (Options),
				    &argc, argv, fallback_resources,
				    NULL);
    xtmisc_set_icon (main_shell, ic_write_kimage_icon);
    cursor = XCreateFontCursor (XtDisplay (main_shell), XC_target);
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
    /*  Setup the reference window  */
    ref_image_display = XtVaCreateManagedWidget ("topForm",
						 imageDisplayWidgetClass,
						 main_shell,
						 XtNborderWidth, 0,
						 XkwNenableAnimation, FALSE,
						 XkwNautoIntensityScale, FALSE,
						 XkwNnumTrackLabels, 3,
						 NULL);
    if ( ( w = XtNameToWidget (ref_image_display, "filesButton") ) != NULL )
    {
	/*  Force main "Files" button to be re-labelled as "Reference Images"
	 */
	XtVaSetValues (w,
		       XtNlabel, "Ref. Image",
		       NULL);
    }
    filepopup = XtNameToWidget (ref_image_display, "filewinPopup");
    filewin = XtNameToWidget (filepopup, "form.selector");
    XtVaSetValues (filepopup,
		   XkwNfilenameTester, XkwFilewinStandardFileTester_nD,
		   NULL);
    XtAddCallback (filepopup, XkwNfileSelectCallback, reference_select_cbk,
		   NULL);
    XkwFilewinRegisterDirCbk (filewin, reference_dirselect_cbk,
			      (Widget) filepopup);
    /*  Get the application box  */
    box = XtNameToWidget (ref_image_display, "applicationBox");
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, box,
				 XtNlabel, "Undo Pair",
				 NULL);
    XtAddCallback (w, XtNcallback, undo_cbk, NULL);
    XtRealizeWidget (main_shell);
    XtVaGetValues (ref_image_display,
		   XkwNpseudoColourCanvas, &ref_canvas,
		   NULL);
    if (ref_canvas == NULL)
    {
	fprintf (stderr, "PseudoColour canvas not found!\n");
	exit (RV_UNDEF_ERROR);
    }
    w = XtNameToWidget (ref_image_display, "multiCanvas.pseudoColourCanvas");
    XDefineCursor (XtDisplay (w), XtWindow (w), cursor);
    canvas_use_astro_transform (ref_canvas, &reference_ap);
    viewimg_register_position_event_func (ref_canvas,
					  ( flag (*) () ) track_ref_event,
					  (void *) ref_image_display);
    viewimg_register_position_event_func (ref_canvas,
					  ( flag (*) () ) reference_event_func,
					  (void *) tar_canvas);
    canvas_register_refresh_func (ref_canvas, ref_refresh_func, NULL);
    /*  Setup the target window  */
    tar_shell = XtVaCreatePopupShell ("tarShell", topLevelShellWidgetClass,
				      main_shell,
				      XtNtitle, "Slice Window",
				      NULL);
    tar_image_display = XtVaCreateManagedWidget ("topForm",
						 imageDisplayWidgetClass,
						 tar_shell,
						 XtNborderWidth, 0,
						 XkwNenableAnimation, FALSE,
						 XkwNautoIntensityScale, FALSE,
                                                 XkwNnumTrackLabels, 2,
						 XkwNshowQuitButton, FALSE,
						 NULL);
    if ( ( w = XtNameToWidget (tar_image_display, "filesButton") ) != NULL )
    {
	/*  Force target "Files" button to be re-labelled as "Target Images" */
	XtVaSetValues (w,
		       XtNlabel, "Target Image",
		       NULL);
    }
    filepopup = XtNameToWidget (tar_image_display, "filewinPopup");
    filewin = XtNameToWidget (filepopup, "form.selector");
    XtVaSetValues (filepopup,
		   XkwNfilenameTester, XkwFilewinStandardFileTester_nD,
		   NULL);
    XtAddCallback (filepopup, XkwNfileSelectCallback, target_select_cbk,
		   NULL);
    XkwFilewinRegisterDirCbk (filewin, target_dirselect_cbk,
			      (Widget) filepopup);
    /*  Get application box of target window */
    box = XtNameToWidget (tar_image_display, "applicationBox");
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, box,
				 XtNlabel, "Undo Pair",
				 NULL);
    XtAddCallback (w, XtNcallback, undo_cbk, NULL);
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, box,
				 XtNlabel, "Compute Co-ordinate System",
				 NULL);
    XtAddCallback (w, XtNcallback, compute_and_store, NULL);
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, box,
				 XtNlabel, "Apply Co-ordinate System",
				 NULL);
    XtAddCallback (w, XtNcallback, apply_coordinates, NULL);
    XtRealizeWidget (tar_shell);
    XtVaGetValues (tar_image_display,
		   XkwNpseudoColourCanvas, &tar_canvas,
		   NULL);
    if (tar_canvas == NULL)
    {
	fprintf (stderr, "PseudoColour canvas not found!\n");
	exit (RV_UNDEF_ERROR);
    }
    w = XtNameToWidget (tar_image_display, "multiCanvas.pseudoColourCanvas");
    XDefineCursor (XtDisplay (w), XtWindow (w), cursor);
    canvas_use_astro_transform (tar_canvas, &target_ap);
    viewimg_register_position_event_func (tar_canvas,
					  ( flag (*) () ) track_tar_event,
					  (void *) tar_image_display);
    viewimg_register_position_event_func (tar_canvas,
					  ( flag (*) () ) target_event_func,
					  (void *) ref_canvas);
    canvas_register_refresh_func (tar_canvas, tar_refresh_func, NULL);
    XtPopup (tar_shell, XtGrabNone);
    if ( (argc == 2) && (strcmp (argv[1], "-overlay") == 0) )
    {
	fprintf (stderr, "Creating overlay list...\n");
	olist = overlay_create_list ( (void *) NULL );
	overlay_specify_canvas (olist, ref_canvas);
	overlay_associate_display_canvas (olist, ref_canvas);
    }
    XtAppMainLoop (app_context);
    return (RV_OK);
}   /*  End Function main   */

void ring_bell ()
/*  [SUMMARY] Ring the bell.
    [RETURNS] Nothing.
*/
{
    extern Widget ref_image_display;

    XBell (XtDisplay (ref_image_display), 100);
}   /*  End Function ring_bell  */


/*  Private routines follow  */

static void load_and_setup (CONST char *filename, iarray *array, KwcsAstro *ap,
			    ViewableImage *image,
			    ViewableImage *magnified_image,
			    Widget shell, Widget image_display)
{
    KWorldCanvas pseudo_canvas, mag_pseudo_canvas;
    double min, max;
    Widget izoomwinpopup;
    char *ptr;
    char stripped_filename[STRING_LENGTH];
    char title[STRING_LENGTH];
    extern char title_name[STRING_LENGTH];

    strcpy (stripped_filename, filename);
    if ( ( ptr = strrchr (stripped_filename, '.') ) != NULL )
    {
	if (strcmp (ptr, ".kf") == 0) *ptr = '\0';
    }
    izoomwinpopup = XtNameToWidget (image_display, "izoomwinpopup");
    XtVaGetValues (image_display,
		   XkwNpseudoColourCanvas, &pseudo_canvas,
		   XkwNmagnifierPseudoColourCanvas, &mag_pseudo_canvas,
		   NULL);
    if ( !load_image (filename, array, ap, pseudo_canvas, mag_pseudo_canvas,
		      image, magnified_image, &min, &max) ) return;
    sprintf (title, "%s  file: %s\n", title_name, filename);
    XtVaSetValues (shell,
		   XtNtitle, title,
		   NULL);
    XtVaSetValues (image_display,
		   XkwNimageName, stripped_filename,
		   NULL);
    XtVaSetValues (izoomwinpopup,
		   XkwNiarray, *array,
		   NULL);
    XtVaSetValues (image_display,
		   XkwNvisibleCanvas, pseudo_canvas,
		   NULL);
}   /*  End Function load_and_setup  */

static void reference_select_cbk (Widget w, XtPointer client_data,
				  XtPointer call_data)
/*  [SUMMARY] Image file selector callback.
    [PURPOSE] This routine is called when a file is selected from the
    "Reference Images" file selector.
    <w> The file selector.
    <client_data> Client data.
    <call_data> A pointer to the selected file.
    [RETURNS] Nothing.
*/
{
    char *filename;
    extern iarray ref_array;
    extern Widget main_shell, ref_image_display;
    extern KwcsAstro reference_ap;
    static ViewableImage ref_vimage = NULL;
    static ViewableImage ref_magnified_vimage = NULL;

    filename = (char *) call_data;
    load_and_setup (filename, &ref_array, &reference_ap, &ref_vimage,
		    &ref_magnified_vimage, main_shell, ref_image_display);
}   /*  End Function reference_select_cbk  */

static flag reference_dirselect_cbk (Widget w, void *info, CONST char *dirname)
{
    if ( !foreign_miriad_test (dirname) ) return (FALSE);
    XtPopdown ( (Widget) info );
    XSync (XtDisplay (w), False);
    reference_select_cbk (NULL, NULL, (XtPointer) dirname);
    return (TRUE);
}   /*  End Function reference_dirselect_cbk  */

static void target_select_cbk (Widget w, XtPointer client_data,
			       XtPointer call_data)
/*  [SUMMARY] Cube file selector callback.
    [PURPOSE] This routine is called when a file is selected from the
    "Target Images" file selector.
    <w> The file selector.
    <client_data> Client data.
    <call_data> A pointer to the selected file.
    [RETURNS] Nothing.
*/
{
    char *filename;
    extern iarray tar_array;
    extern Widget tar_shell, tar_image_display;
    static ViewableImage tar_vimage = NULL;
    static ViewableImage tar_magnified_vimage = NULL;

    filename = (char *) call_data;
    load_and_setup (filename, &tar_array, NULL, &tar_vimage,
		    &tar_magnified_vimage, tar_shell, tar_image_display);
}   /*  End Function target_select_cbk  */

static flag target_dirselect_cbk (Widget w, void *info, CONST char *dirname)
{
    if ( !foreign_miriad_test (dirname) ) return (FALSE);
    XtPopdown ( (Widget) info );
    XSync (XtDisplay (w), False);
    target_select_cbk (NULL, NULL, (XtPointer) dirname);
    return (TRUE);
}   /*  End Function target_dirselect_cbk  */

static flag track_ref_event (ViewableImage vimage, double x, double y,
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
    KWorldCanvas canvas, magnifier_canvas;
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
    extern iarray ref_array;
    extern KwcsAstro reference_ap;
    static char function_name[] = "track_ref_event";

    if (event_code != K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    /*  Create value string  */
    if (ref_array == NULL) return (TRUE);
    switch (value_type)
    {
      case K_DCOMPLEX:
	/*  Compute value  */
	iarray_format_value (ref_array, value_string,
			     *(double *) value, TOOBIG, TOOBIG);
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
    canvas = viewimg_get_worldcanvas (vimage);
    canvas_get_specification (canvas, &xlabel, &ylabel, &num_restr,
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
    if (reference_ap == NULL)
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
	wcs_astro_format_all (reference_ap, world_string,
			      xlabel, x_lin, ylabel, y_lin, NULL, 0.0,
			      num_restr, (CONST char **) restr_names,
			      restr_values, extra_string);
	XtVaSetValues (third_track_label, XtNlabel, extra_string, NULL);
    }
    XtVaSetValues (second_track_label, XtNlabel, world_string, NULL);
    XtVaGetValues (image_display,
		   XkwNmagnifierVisibleCanvas, &magnifier_canvas,
		   NULL);
    update_magnifier (magnifier_canvas, x_lin, y_lin, canvas);
    return (TRUE);
}   /*  End Function track_ref_event  */

static flag track_tar_event (ViewableImage vimage, double x, double y,
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
    KWorldCanvas canvas, magnifier_canvas;
    unsigned int hdim, vdim, rdim;
    unsigned int num_restr, count;
    unsigned long pointer_x_index, pointer_y_index, r_index;
    Widget image_display = (Widget) *f_info;
    Widget first_track_label, second_track_label;
    unsigned char *rgb_ptr = (unsigned char *) value;
    char *xlabel, *ylabel;
    char **restr_names;
    double *restr_values;
    array_desc *arr_desc;
    char txt[STRING_LENGTH];
    char value_string[STRING_LENGTH], index_string[STRING_LENGTH];
    char world_string[STRING_LENGTH], extra_string[STRING_LENGTH];
    extern iarray tar_array;
    extern KwcsAstro target_ap;
    static char function_name[] = "track_tar_event";

    if (event_code != K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    /*  Create value string  */
    if (tar_array == NULL) return (TRUE);
    switch (value_type)
    {
      case K_DCOMPLEX:
	/*  Compute value  */
	iarray_format_value (tar_array, value_string,
			     *(double *) value, TOOBIG, TOOBIG);
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
    canvas = viewimg_get_worldcanvas (vimage);
    canvas_get_specification (canvas, &xlabel, &ylabel, &num_restr,
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
    XtVaSetValues (first_track_label, XtNlabel, index_string, NULL);
    /*  Now display the world co-ordinate information  */
    if (target_ap == NULL)
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
	wcs_astro_format_all (target_ap, world_string,
			      "RA---ARC", x_lin, "DEC--ARC", y_lin, NULL, 0.0,
			      num_restr, (CONST char **) restr_names,
			      restr_values, extra_string);
    }
    XtVaSetValues (second_track_label, XtNlabel, world_string, NULL);
    XtVaGetValues (image_display,
		   XkwNmagnifierVisibleCanvas, &magnifier_canvas,
		   NULL);
    update_magnifier (magnifier_canvas, x_lin, y_lin, canvas);
    return (TRUE);
}   /*  End Function track_tar_event  */

static void undo_cbk (Widget w, XtPointer client_data, XtPointer call_data)
{
    extern KWorldCanvas ref_canvas, tar_canvas;
    extern KwcsAstro target_ap;
    extern flag last_click_was_reference;
    extern unsigned int num_reference_points;
    extern unsigned int num_target_points;

    last_click_was_reference = FALSE;
    if (num_reference_points < 1)
    {
	XBell (XtDisplay (w), 100);
	return;
    }
    --num_reference_points;
    num_target_points = num_reference_points;
    kwin_resize (canvas_get_pixcanvas (ref_canvas), FALSE, 0, 0, 0, 0);
    kwin_resize (canvas_get_pixcanvas (tar_canvas), FALSE, 0, 0, 0, 0);
    if (target_ap != NULL) wcs_astro_destroy (target_ap);
    target_ap = NULL;
}   /*  End Function undo_cbk  */

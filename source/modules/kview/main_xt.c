/*  main_xt.c

    Main file for  kview  (X11 image/movie display tool for Karma).

    Copyright (C) 1995  Richard Gooch

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

    Last updated by Richard Gooch   28-SEP-1995: Added Miriad Image support.


*/
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <karma.h>
#include <k_event_codes.h>
#include <karma_foreign.h>
#include <karma_overlay.h>
#include <karma_viewimg.h>
#include <karma_conn.h>
#include <karma_dir.h>
#include <karma_chx.h>
#include <karma_im.h>
#include <karma_hi.h>
#include <karma_xc.h>
#include <karma_a.h>
#include <Xkw/ImageDisplay.h>
#include <Xkw/Filewin.h>
#ifdef OS_SunOS
#  include <floatingpoint.h>
#endif

#define VERSION "1.3"


/*  External functions  */
/*  File: generic.c  */
EXTERN_FUNCTION (void setup_comms, (Display *display) );
EXTERN_FUNCTION (flag display_file,
		 (CONST char *inp_filename, KWorldCanvas pseudo_canvas,
		  KWorldCanvas rgb_canvas, iarray *pseudo_arr,
		  ViewableImage *image,
		  ViewableImage **movie, unsigned int *num_frames) );


/*  Local functions  */
EXTERN_FUNCTION (void load_and_setup, (CONST char *filename) );


/*  Private functions  */
STATIC_FUNCTION (flag accept_file, (KFileInfo finfo) );
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
    "Kview*hugeColourCanvas*background:              black",
    "Kview*AnimateControl*positionCanvas.background: black",
    "Kview*pseudoColourCanvas*foreground:            white",
    "Kview*Command*background:                       grey70",
    "Kview*Repeater*background:                      grey70",
    "Kview*Toggle*background:                        grey80",
    "Kview*closeButton*background:                   grey90",
    "Kview*ChoiceMenu.background:                    turquoise",
    "Kview*ExclusiveMenu.background:                 turquoise",
    "Kview*Value*background:                         #d0a0a0",
    "Kview*ImageDisplay*quit*background:             orange",
    "Kview*background:                               aquamarine",
    "Kview*ImageDisplay*trackLabel*font :            8x13bold",
    "Kview*ImageDisplay*zoomMenu*font:               10x20",
    "Kview*ImageDisplay*crosshairMenu*font:          10x20",
    "Kview*ImageDisplay*exportMenu*font:             10x20",
    "Kview*AnimateControl*modeMenu*font:             10x20",
    "Kview*ImageDisplay.Command.font:                7x13bold",
    "Kview*ImageDisplay.Toggle.font:                 7x13bold",
    "Kview*ImageDisplay.ChoiceMenu.font:             7x13bold",
    "Kview*ImageDisplay*zoomMenu*Unzoom*foreground:  red",
    "Kview*Dataclip.AutoValueScale:                  False",
    "Kview*font:                                     9x15bold",
    NULL
};
static XrmOptionDescRec Options[] =
{
    {"-private_cmap", ".topForm.multiCanvas.pseudoColourCanvas.forceNewCmap",
     XrmoptionNoArg, (XtPointer) "True"},
    {"-num_colours", ".topForm.cmapSize", XrmoptionSepArg, (XtPointer) NULL},
    {"-fullscreen", ".topForm.fullscreen", XrmoptionNoArg, (XtPointer) "True"}
};
static Widget image_display = NULL;
static ViewableImage image = NULL;
static iarray pseudo_arr = NULL;
static ViewableImage *movie = NULL;
static unsigned int num_frames = 0;


main (argc, argv)
int argc;
char **argv;
{
    KWorldCanvas wc_pseudo, wc_direct, wc_true;
    KOverlayList olist;
    XtAppContext app_context;
    Widget main_shell;
    Widget filewin, filepopup, track_label, animate_control;
    Display *dpy;
    extern Widget image_display;
    extern char module_name[STRING_LENGTH + 1];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

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
    /*  Initialise communications  */
    chx_register_app_context (app_context);
    conn_register_managers (chx_manage, chx_unmanage, ( void (*) () ) NULL);
    dpy = XtDisplay (main_shell);
    setup_comms (dpy);
    image_display = XtVaCreateManagedWidget ("topForm",
					     imageDisplayWidgetClass,
					     main_shell,
					     XtNborderWidth, 0,
					     XkwNenableAnimation, True,
					     NULL);
    filepopup = XtNameToWidget (image_display, "filewinPopup");
    filewin = XtNameToWidget (filepopup, "form.selector");
    animate_control = XtNameToWidget (image_display, "animatepopup");
    track_label = XtNameToWidget (image_display, "trackLabel");
    XtVaSetValues (filepopup,
		   XkwNfilenameTester, accept_file,
		   NULL);
    XtAddCallback (filepopup, XkwNfileSelectCallback, fileselect_cbk, NULL);
    (void) XkwFilewinRegisterDirCbk (filewin, dirselect_cbk,
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
	viewimg_register_position_event_func (wc_pseudo, track_canvas_event,
					      (void *) track_label);
	viewimg_set_canvas_attributes (wc_pseudo,
				       VIEWIMG_ATT_AUTO_V, FALSE,
				       VIEWIMG_ATT_END);
    }
    if (wc_direct != NULL)
    {
	viewimg_register_position_event_func (wc_direct, track_canvas_event,
					      (void *) track_label);
    }
    if (wc_true != NULL)
    {
	viewimg_register_position_event_func (wc_true, track_canvas_event,
					      (void *) track_label);
    }
    if ( (argc == 2) && (strcmp (argv[1], "-overlay") == 0) )
    {
	(void) fprintf (stderr, "Creating overlay list...\n");
	olist = overlay_create_list ( (void *) NULL );
	overlay_specify_canvas (olist, wc_pseudo);
	(void) overlay_associate_display_canvas (olist, wc_pseudo);
    }
    if ( !conn_controlled_by_cm_tool () ) XtPopup (filepopup, XtGrabNone);
    XtAppMainLoop (app_context);
    return (RV_OK);
}   /*  End Function main   */

void load_and_setup (CONST char *filename)
{
    KWorldCanvas pseudo_canvas, direct_canvas, true_canvas, rgb_canvas;
    Widget izoomwinpopup, animate_winpopup;
    char *ptr;
    char stripped_filename[STRING_LENGTH];
    extern ViewableImage image;
    extern iarray pseudo_arr;
    extern ViewableImage *movie;
    extern unsigned int num_frames;
    extern Widget image_display;

    (void) strcpy (stripped_filename, filename);
    if ( ( ptr = strrchr (stripped_filename, '.') ) != NULL )
    {
	if (strcmp (ptr, ".kf") == 0) *ptr = '\0';
    }
    XtVaGetValues (image_display,
		   XkwNpseudoColourCanvas, &pseudo_canvas,
		   XkwNdirectColourCanvas, &direct_canvas,
		   XkwNtrueColourCanvas, &true_canvas,
		   NULL);
    izoomwinpopup = XtNameToWidget (image_display, "izoomwinpopup");
    animate_winpopup = XtNameToWidget (image_display, "animatepopup");
/*
    rgb_canvas = (direct_canvas == NULL) ? true_canvas : direct_canvas;
*/
    rgb_canvas = (true_canvas == NULL) ? direct_canvas : true_canvas;
    if ( !display_file (filename, pseudo_canvas, rgb_canvas, &pseudo_arr,
			&image, &movie, &num_frames) ) return;
    /*  Deal with the animate widget  */
    XtVaSetValues (animate_winpopup, XkwNnumFrames, num_frames, NULL);
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
    }
}   /*  End Function load_and_setup  */


/*  Private routines follow  */

static flag accept_file (finfo)
KFileInfo finfo;
{
    /*  Accept all directories  */
    if (finfo.type == KFILETYPE_DIRECTORY) return (TRUE);
    if (foreign_guess_format_from_filename (finfo.filename)
	== FOREIGN_FILE_FORMAT_UNKNOWN) return (FALSE);
    /*  Filetype understood: accept it  */
    return (TRUE);
}   /*  End Function accept_file  */

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
    Widget track_label = (Widget) *f_info;
    unsigned char *rgb_ptr = (unsigned char *) value;
    char window_string[STRING_LENGTH];
    static char function_name[] = "track_canvas_event";

    if (event_code != K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    switch (value_type)
    {
      case K_DCOMPLEX:
	(void) sprintf (window_string, "abs: %5e  ord: %5e      value: %e",
			x, y, *(double *) value);
	break;
      case K_UB_RGB:
	(void) sprintf (window_string, "abs: %5e  ord: %5e      RGB: %u %u %u",
			x, y, rgb_ptr[0], rgb_ptr[1], rgb_ptr[2]);
	break;
      default:
	(void) fprintf (stderr, "Illegal type: %u\n", value_type);
	a_prog_bug (function_name);
	break;
    }
    XtVaSetValues (track_label, XtNlabel, window_string, NULL);
    return (TRUE);
}   /*  End Function track_canvas_event  */

static void new_frame_cbk (w, client_data, call_data)
/*  This is the new frame callback.
*/
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    int frame_number = *(int *) call_data;
    extern ViewableImage *movie;

    if (movie == NULL) return;
    if (movie[frame_number] == NULL) return;
    (void) viewimg_make_active (movie[frame_number]);
}   /*  End Function new_frame_cbk   */

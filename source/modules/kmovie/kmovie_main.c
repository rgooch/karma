/*  kmovie_main.c

    Main file for  kmovie  (X11 movie display tool for Karma).

    Copyright (C) 1994,1995  Richard Gooch

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
    the general data structure format. The arrayfile must contain a 3
    dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   25-DEC-1994: Copied from  kview_2d_main.c

    Updated by      Richard Gooch   25-DEC-1994

    Updated by      Richard Gooch   4-JAN-1995: Use  XtNameToWidget  to get
  child widgets.

    Last updated by Richard Gooch   20-JAN-1995: Use TrueColour visual rather
  than DirectColour (no need to worry about colourmap contents).


*/
#include <stdio.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <karma.h>
#include <k_event_codes.h>
#include <karma_overlay.h>
#include <karma_viewimg.h>
#include <karma_kcmap.h>
#include <karma_conn.h>
#include <karma_dir.h>
#include <karma_chx.h>
#include <karma_im.h>
#include <karma_hi.h>
#include <karma_xc.h>
#include <karma_a.h>
#include <Xkw/ImageDisplay.h>
#ifdef OS_SunOS
#  include <floatingpoint.h>
#endif

#define VERSION "1.3"


/*  External functions  */
/*  File: kmovie_generic.c  */
EXTERN_FUNCTION (void setup_comms, (Display *display) );
EXTERN_FUNCTION (flag load,
		 (char *arrayfile,
		  KWorldCanvas pseudo_canvas, KWorldCanvas rgb_canvas) );


/*  Local functions  */
EXTERN_FUNCTION (void load_and_setup, (char *arrayfile) );


/*  Private functions  */
STATIC_FUNCTION (flag accept_file, (KFileInfo finfo) );
STATIC_FUNCTION (void fileselect_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (track_canvas_event,
		 (ViewableImage vimage, double x, double y, double value[2],
		  unsigned int event_code, void *e_info, void **f_info) );
STATIC_FUNCTION (void new_frame_cbk, (Widget w, XtPointer client_data,
				      XtPointer call_data) );


/*  Private data  */
String fallback_resources[] =
{
    "Kmovie*pseudoColourCanvas*background:            gray20",
    "Kmovie*hugeColourCanvas*background:              black",
    "Kmovie*pseudoColourCanvas*foreground:            white",
    "Kmovie*Command*background:                       grey70",
    "Kmovie*Repeater*background:                      grey70",
    "Kmovie*Toggle*background:                        grey80",
    "Kmovie*closeButton*background:                   grey90",
    "Kmovie*ChoiceMenu.background:                    turquoise",
    "Kmovie*ExclusiveMenu.background:                 turquoise",
    "Kmovie*Value*background:                         #d0a0a0",
    "Kmovie*ImageDisplay*quit*background:             orange",
    "Kmovie*background:                               aquamarine",
    "Kmovie*ImageDisplay*trackLabel*font :            8x13bold",
    "Kmovie*ImageDisplay*zoomMenu*font:               10x20",
    "Kmovie*ImageDisplay*crosshairMenu*font:          10x20",
    "Kmovie*AnimateControl*modeMenu*font:             10x20",
    "Kmovie*ImageDisplay.Command.font:                7x13bold",
    "Kmovie*ImageDisplay.Toggle.font:                 7x13bold",
    "Kmovie*ImageDisplay.ChoiceMenu.font:             7x13bold",
    "Kmovie*ImageDisplay*zoomMenu*Unzoom*foreground:  red",
    "Kmovie*Dataclip.AutoValueScale:                  False",
    "Kmovie*font:                                     9x15bold",
    NULL
};
static Widget image_display = NULL;


main (argc, argv)
int argc;
char **argv;
{
    KWorldCanvas wc;
    KOverlayList olist;
    XtAppContext app_context;
    Widget main_shell;
    Widget filewin, track_label, animate_control;
    Display *dpy;
    extern Widget image_display;
    extern char module_name[STRING_LENGTH + 1];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

#ifdef SIGFPE_ABORT
	ieee_handler ("set", "division", SIGFPE_ABORT);
#endif
    /*  Initialise module  */
    im_register_module_name ("kmovie");
    im_register_module_version_date (VERSION);
    /*  Start up Xt  */
    main_shell = XtVaAppInitialize (&app_context, "Kmovie", NULL, 0,
				    &argc, argv, fallback_resources,
				    NULL, 0);
    kcmap_init (xc_alloc_colours, xc_free_colours, xc_store_colours,
		xc_get_location);
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
    filewin = XtNameToWidget (image_display, "filewinPopup");
    animate_control = XtNameToWidget (image_display, "animatepopup");
    track_label = XtNameToWidget (image_display, "trackLabel");
    XtVaSetValues (filewin,
		   XkwNfilenameTester, accept_file,
		   NULL);
    XtAddCallback (filewin, XkwNfileSelectCallback, fileselect_cbk, NULL);
    XtRealizeWidget (main_shell);
    XtAddCallback (animate_control, XkwNnewFrameCallback, new_frame_cbk, NULL);
    XtVaGetValues (image_display,
		   XkwNpseudoColourCanvas, &wc,
		   NULL);
    viewimg_register_position_event_func (wc, track_canvas_event,
					  (void *) track_label);
    viewimg_set_canvas_attributes (wc,
				   VIEWIMG_ATT_AUTO_V, FALSE,
				   VIEWIMG_ATT_END);
    olist = overlay_create_list ( (void *) NULL );
    overlay_specify_canvas (olist, wc);
    (void) overlay_associate_display_canvas (olist, wc);
    if ( !conn_controlled_by_cm_tool () ) XtPopup (filewin, XtGrabNone);
    XtAppMainLoop (app_context);
}   /*  End Function main   */

void load_and_setup (char *arrayfile)
{
    KWorldCanvas pseudo_canvas, direct_canvas, true_canvas, rgb_canvas;
    Widget izoomwinpopup, animate_winpopup;
    char *ptr;
    extern iarray movie_pseudo;
    extern unsigned int num_frames;
    extern Widget image_display;
    static char arrayfile_name[STRING_LENGTH];  /*  Must be static!  */

    (void) strcpy (arrayfile_name, arrayfile);
    if ( ( ptr = strrchr (arrayfile_name, '.') ) != NULL )
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
    if ( !load (arrayfile_name, pseudo_canvas, rgb_canvas) ) return;
    XtVaSetValues (animate_winpopup, XkwNnumFrames, num_frames, NULL);
    if (movie_pseudo == NULL)
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
		       XkwNimageName, arrayfile_name,
		       NULL);
	XtVaSetValues (izoomwinpopup,
		       XkwNiarray, movie_pseudo,
		       NULL);
    }
}   /*  End Function load_and_setup  */


/*  Private routines follow  */

static flag accept_file (finfo)
KFileInfo finfo;
{
    /*  Accept all directories  */
    if (finfo.type == KFILETYPE_DIRECTORY) return (TRUE);
    /*  Accept all .kf files  */
    if (strcmp (finfo.filename + strlen (finfo.filename) - 3, ".kf")
	== 0) return (TRUE);
    /*  Reject everything else  */
    return (FALSE);
}   /*  End Function accept_file  */

static void fileselect_cbk (w, client_data, call_data)
Widget w;
XtPointer client_data;
XtPointer call_data;
{
    char *arrayfile_name;

    arrayfile_name = (char *) call_data;
    load_and_setup (arrayfile_name);
}   /*  End Function fileselect_cbk  */

static flag track_canvas_event (vimage, x, y, value, event_code, e_info,f_info)
/*  This routine is a position event consumer for a viewable image canvas.
    The viewable image is given by  vimage  .
    The horizontal position of the event, relative to the canvas origin,
    will be given by  x  .
    The vertical position of the event, relative to the canvas origin,
    will be given by  y  .
    The value in the viewable image at the co-ordinates is given by  value.
    This is of type K_DOUBLE.
    The arbitrary event code is given by  event_code  .
    The arbitrary event information is pointed to by  e_info  .
    The arbitrary function information pointer is pointed to by  f_info  .
    The routine returns TRUE if the event was consumed, else it return
    FALSE indicating that the event is still to be processed.
*/
ViewableImage vimage;
double x;
double y;
double value[2];
unsigned int event_code;
void *e_info;
void **f_info;
{
    Widget track_label = (Widget) *f_info;
    char window_string[STRING_LENGTH];

    if (event_code != K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    (void) sprintf (window_string, "abs: %5e  ord: %5e      value: %e",
		    x, y, value[0]);
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
    extern ViewableImage *frames;

    (void) viewimg_make_active (frames[frame_number]);
}   /*  End Function new_frame_cbk   */

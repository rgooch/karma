/*  main_xt.c

    Main file for  kslice_3d  (X11 3D slicer display tool for Karma).

    Copyright (C) 1994-1996  Richard Gooch

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
    This Karma module will enable on-screen viewing of orthogonal slices in a
    3 dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   28-JAN-1995: Copied from  kview_2d_main.c

    Updated by      Richard Gooch   28-JAN-1995

    Updated by      Richard Gooch   4-FEB-1995: Tidying up.

    Updated by      Richard Gooch   15-APR-1995: Added call to
  <im_register_lib_version>.

    Updated by      Richard Gooch   17-APR-1995: Added "-private_cmap" option.

    Updated by      Richard Gooch   23-APR-1995: Added FITS file support.

    Updated by      Richard Gooch   31-JUL-1995: Made use of ThreeDeeSlice
  widget.

    Updated by      Richard Gooch   1-SEP-1995: Changed to <kcmap_va_create>.

    Updated by      Richard Gooch   28-SEP-1995: Added Miriad Image support.

    Updated by      Richard Gooch   22-OCT-1995: Print number of colours.

    Updated by      Richard Gooch   28-OCT-1995: Search for and require 8 bit
  PseudoColour visual.

    Updated by      Richard Gooch   28-DEC-1995: Set canvas background to black

    Updated by      Richard Gooch   10-JAN-1996: Added #include for iarray.

    Updated by      Richard Gooch   21-APR-1996: Made use of
  XkwNsimpleColourbar resource for Cmapwinpopup widget.

    Updated by      Richard Gooch   4-MAY-1996: Switched to KtoggleWidget
  resources.

    Updated by      Richard Gooch   5-MAY-1996: Switched to <xtmisc_popup_cbk>
  routine and improved documentation.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   22-JUN-1996: Made use of
  <XkwFilewinStandardFileTester_nD> and <XkwFilewinStandardFileTester_3D>
  routines.

    Updated by      Richard Gooch   15-SEP-1996: Made use of new <kwin_xutil_*>
  routines.

    Last updated by Richard Gooch   29-OCT-1996: Added hostname and port number
  to title.


*/
#include <stdio.h>
#include <errno.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <karma.h>
#include <k_event_codes.h>
#ifndef X11
#  define X11
#endif
#include <karma_foreign.h>
#include <karma_iarray.h>
#include <karma_xtmisc.h>
#include <karma_dsxfr.h>
#include <karma_kcmap.h>
#include <karma_kwin.h>
#include <karma_conn.h>
#include <karma_chx.h>
#include <karma_ds.h>
#include <karma_im.h>
#include <karma_hi.h>
#include <karma_xc.h>
#include <karma_a.h>
#include <karma_r.h>
#include <Xkw/ThreeDeeSlice.h>
#include <Xkw/Filepopup.h>
#include <Xkw/Cmapwinpopup.h>
#include <Xkw/Filewin.h>
#ifdef OS_SunOS
#  include <floatingpoint.h>
#endif

#define DEFAULT_COLOURMAP_NAME "Greyscale1"
#define NUM_COLOURS (unsigned int) 200

#define VERSION "1.4.1"


/*  Private functions  */
STATIC_FUNCTION (void load_and_setup, (CONST char *filename) );
STATIC_FUNCTION (void quit_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );
STATIC_FUNCTION (void fileselect_cbk, (Widget w, XtPointer client_data,
				       XtPointer call_data) );
STATIC_FUNCTION (flag dirselect_cbk,
		 (Widget w, void *info, CONST char *dirname) );
STATIC_FUNCTION (void setup_comms, (Display *display) );
STATIC_FUNCTION (void new_data_on_connection, (flag first_time_data) );
STATIC_FUNCTION (void connection_closed, (flag data_deallocated ));


/*  Private data  */
String fallback_resources[] =
{
    "Kslice_3d*Canvas*background:                        black",
    "Kslice_3d*Canvas*foreground:                        white",
    "Kslice_3d*Command*background:                       grey70",
    "Kslice_3d*Ktoggle*background:                       grey80",
    "Kslice_3d*Value*background:                         #d0a0a0",
    "Kslice_3d*closeButton*background:                   grey90",
    "Kslice_3d*ChoiceMenu.background:                    turquoise",
    "Kslice_3d*topForm*quit*background:                  orange",
    "Kslice_3d*background:                               aquamarine",
    "Kslice_3d*ImageDisplay*trackLabel*font:             8x13bold",
    "Kslice_3d*ImageDisplay*zoomMenu*font:               10x20",
    "Kslice_3d*ImageDisplay*crosshairMenu*font:          10x20",
    "Kslice_3d*ImageDisplay*font:                        9x15bold",
    "Kslice_3d*ImageDisplay*zoomMenu*Unzoom*foreground:  red",
    "Kslice_3d*font:                                     9x15bold",
    NULL
};
static XrmOptionDescRec Options[] =
{
    {"-private_cmap", ".topForm.canvas.forceNewCmap", XrmoptionNoArg,
     (XtPointer) "True"},
};
static Widget slicer = NULL;
static iarray cube = NULL;
static char title_name[STRING_LENGTH];


int main (int argc, char **argv)
{
    Kdisplay dpy_handle;
    Kcolourmap kcmap;
    unsigned int num_ccels;
    XtAppContext app_context;
    Widget main_shell, form, filewin, filepopup, colourmapwinpopup;
    Widget files_btn, cmap_btn, precomp_btn, quit_btn;
    Colormap xcmap;
    unsigned long *pixel_values;
    Display *dpy;
    XVisualInfo *vinfo;
    Visual *pseudocolour_visual;
    Visual *root_visual;
    Screen *screen;
    extern Widget slicer;
    extern char title_name[STRING_LENGTH];
    static char function_name[] = "main";

#ifdef SIGFPE_ABORT
	ieee_handler ("set", "division", SIGFPE_ABORT);
#endif
    /*  Initialise module  */
    im_register_module_name ("kslice_3d");
    im_register_module_version_date (VERSION);
    im_register_lib_version (KARMA_VERSION);
    /*  Start up Xt  */
    main_shell = XtVaAppInitialize (&app_context, "Kslice_3d",
				    Options, XtNumber (Options),
				    &argc, argv, fallback_resources,
				    NULL, 0);
    /*  Initialise communications  */
    chx_register_app_context (app_context);
    conn_register_managers (chx_manage, chx_unmanage, ( void (*) () ) NULL);
    dpy = XtDisplay (main_shell);
    screen = XtScreen (main_shell);
    /*  Get visual information  */
    root_visual = XDefaultVisualOfScreen (screen);
    vinfo = kwin_xutil_get_visinfo_for_visual (dpy, root_visual);
    kwin_xutil_get_visuals (screen, &pseudocolour_visual, NULL, NULL);
    if (pseudocolour_visual == NULL)
    {
	fprintf (stderr, "No 8 bit PseudoColour visual available\n");
	exit (RV_UNDEF_ERROR);
    }
    if ( (vinfo->depth == 8) && (vinfo->class == PseudoColor) &&
	(vinfo->colormap_size == 256) )
    {
	/*  Use the default visual  */
	pseudocolour_visual = root_visual;
	XtVaGetValues (main_shell,
		       XtNcolormap, &xcmap,
		       NULL);
    }
    else
    {
	/*  Different visual: create colourmap  */
	if ( ( xcmap = XCreateColormap (dpy, XRootWindowOfScreen (screen),
					pseudocolour_visual, AllocNone) ) ==
	    (Colormap) NULL )
	{
	    fprintf (stderr, "Could not create colourmap\n");
	    exit (1);
	}
	XSync (dpy, False);
	fprintf (stderr,
			"Created colourmap: 0x%lx for PseudoColour visual\n",
			xcmap);
    }
    setup_comms (dpy);
    XtVaSetValues (main_shell,
		   XtNtitle, title_name,
		   NULL);
    if ( ( dpy_handle = xc_get_dpy_handle (dpy, xcmap) ) == NULL )
    {
	fprintf (stderr, "Error getting display handle\n");
	a_prog_bug (function_name);
    }
    /*  Initialise colourmap  */
    if ( ( kcmap = kcmap_va_create (DEFAULT_COLOURMAP_NAME,
				    NUM_COLOURS, TRUE,
				    dpy_handle,
				    xc_alloc_colours, xc_free_colours,
				    xc_store_colours, xc_get_location,
				    KCMAP_ATT_END) )
	== NULL )
    {
	fprintf (stderr, "Error creating main colourmap\n");
	a_prog_bug (function_name);
    }
    num_ccels = kcmap_get_pixels (kcmap, &pixel_values);
    fprintf (stderr, "num colours: %u\n", num_ccels);
    form = XtVaCreateManagedWidget ("topForm", formWidgetClass, main_shell,
				    XtNborderWidth, 0,
				    NULL);
    filepopup = XtVaCreatePopupShell ("filewinPopup", filepopupWidgetClass,
				      form,
				      XtNtitle, "kslice_3d File Selector",
				      XtNx, 0,
				      XtNy, 0,
				      XkwNautoPopdown, True,
				      NULL);
    /*  Register callback which will determine which files are visible  */
    XtVaSetValues (filepopup,
		   XkwNfilenameTester, XkwFilewinStandardFileTester_3D,
		   NULL);
    /*  Register callback which will process file selection events  */
    XtAddCallback (filepopup, XkwNfileSelectCallback, fileselect_cbk, NULL);
    /*  Register callback which will process directory selection events  */
    filewin = XtNameToWidget (filepopup, "form.selector");
    XkwFilewinRegisterDirCbk (filewin, dirselect_cbk,
				     (Widget) filepopup);
    /*  Now start creating sundry buttons  */
    files_btn = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
					 XtNlabel, "Files",
					 NULL);
    XtAddCallback (files_btn, XtNcallback, xtmisc_popup_cbk, filepopup);
    cmap_btn = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
					XtNlabel, "Colourmap",
					XtNfromHoriz, files_btn,
					NULL);
    precomp_btn = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
					   XtNlabel, "Precompute",
					   XtNfromHoriz, cmap_btn,
					   NULL);
    quit_btn = XtVaCreateManagedWidget ("quit", commandWidgetClass, form,
					XtNlabel, "Quit",
					XtNfromHoriz, precomp_btn,
					NULL);
    XtAddCallback (quit_btn, XtNcallback, quit_cbk, NULL);
    slicer = XtVaCreateManagedWidget ("slicer", threeDeeSliceWidgetClass, form,
				      XtNborderWidth, 0,
				      XtNhorizDistance, 0,
				      XtNvertDistance, 0,
				      XtNfromVert, quit_btn,
				      XkwNkarmaColourmap, kcmap,
				      XkwNcanvasVisual, pseudocolour_visual,
				      NULL);
    XtAddCallback (precomp_btn, XtNcallback, XkwThreeDeeSlicePrecompute,
		   (XtPointer) slicer);
    XtRealizeWidget (main_shell);
    colourmapwinpopup = XtVaCreatePopupShell ("cmapwinpopup",
					      cmapwinpopupWidgetClass, form,
					      XkwNkarmaColourmap, kcmap,
					      XkwNsimpleColourbar, True,
					      NULL);
    XtAddCallback (cmap_btn, XtNcallback, xtmisc_popup_cbk, colourmapwinpopup);
    if ( !conn_controlled_by_cm_tool () ) XtPopup (filepopup, XtGrabNone);
    XtAppMainLoop (app_context);
    return (RV_OK);
}   /*  End Function main   */


/*  Private routines follow  */

static void load_and_setup (CONST char *filename)
{
    unsigned int ftype;
    multi_array *multi_desc;
    extern iarray cube;
    extern Widget slicer;

    /*  Read data  */
    if ( ( multi_desc = foreign_guess_and_read (filename, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	fprintf (stderr, "Error reading cube: \"%s\"\n", filename);
	return;
    }
    if (cube != NULL) iarray_dealloc (cube);
    cube = iarray_get_from_multi_array (multi_desc, NULL, 3, NULL, NULL);
    ds_dealloc_multi (multi_desc);
    if (cube == NULL ) return;
    XtVaSetValues (slicer,
		   XkwNiarray, cube,
		   NULL);
}   /*  End Function load_and_setup  */

static void quit_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the quit button callback.
*/
{
    exit (RV_OK);
}   /*  End Function quit_cbk   */

static void fileselect_cbk (Widget w, XtPointer client_data,
			    XtPointer call_data)
{
    char *filename;

    filename = (char *) call_data;
    load_and_setup (filename);
}   /*  End Function fileselect_cbk  */

static flag dirselect_cbk (Widget w, void *info, CONST char *dirname)
/*  [SUMMARY] Callback for directory selection.
    [PURPOSE] This routine is called when the user selects a directory in the
    file selector. Normally the directory is descended and another group of
    files is visible. This callback makes it to possible to "capture" a
    directory if required. This can be useful when a conceptual "data file" is
    in fact implemented as a directory or files, such as with Miriad.
    <w> The Filewin widget on which the selection occurred.
    <info> A pointer to arbitrary information.
    <dirname> The name of the directory.
    [RETURNS] TRUE if the selection event is to be consumed, else FALSE
    (meaning the event should be passed on to other callbacks, or, if none
    consume the event, the widget will change it's active directory).
*/
{
    /*  If not a Miriad file do nothing with the directory  */
    if ( !foreign_miriad_test (dirname) ) return (FALSE);
    /*  It is a Miriad file. First pop-down the fileselector  */
    XtPopdown ( (Widget) info );
    XSync (XtDisplay (w), False);
    /*  Process the directory as a normal file would be  */
    load_and_setup (dirname);
    /*  Consume selection event: i.e. don't allow directory to become the
	active directory for the fileselector.  */
    return (TRUE);
}   /*  End Function dirselect_cbk  */

static void setup_comms (Display *display)
/*  This routine will initialise the kslice_3d module communications.
    The display the module is connected to must be pointed to by  display  .
    The routine returns nothing.
*/
{
    int def_port_number;
    unsigned int server_port_number;
    char hostname[STRING_LENGTH];
    extern char module_name[STRING_LENGTH + 1];
    extern char module_version_date[STRING_LENGTH + 1];
    extern char title_name[STRING_LENGTH];

    /*  Get default port number  */
    if ( ( def_port_number = r_get_def_port ( module_name,
					     DisplayString (display) ) ) < 0 )
    {
	fprintf (stderr, "Could not get default port number\n");
	return;
    }
    r_gethostname (hostname, STRING_LENGTH);
    server_port_number = def_port_number;
    if (conn_become_server (&server_port_number, CONN_MAX_INSTANCES) != TRUE)
    {
	fprintf (stderr, "Module not operating as Karma server\n");
	sprintf (title_name, "%s v%s @%s", module_name, module_version_date,
		 hostname);
    }
    else
    {
	fprintf (stderr, "Port allocated: %d\n", server_port_number);
	/*  Register the protocols  */
/*
	conn_register_client_protocol ("3D_cursor_position", 0, 1,
				       ( flag (*) () ) NULL,
				       ( flag (*) () ) NULL,
				       read_3d_cursor,
				       ( void (*) () ) NULL);
*/
	dsxfr_register_connection_limits (1, -1);
	dsxfr_register_read_func (new_data_on_connection);
	dsxfr_register_close_func (connection_closed);
	sprintf (title_name, "%s v%s @%s:%u",
		 module_name, module_version_date, hostname,
		 server_port_number);
    }
}   /*  End Function setup_comms  */

static void new_data_on_connection (flag first_time_data)
/*  This routine is called when new data arrives.
    If data is arriving the first time, then  first_time_data  will be TRUE.
    The routine returns nothing.
*/
{
    fprintf (stderr, "new data...\n");
    load_and_setup ("connection");
}   /*  End Function new_data_on_connection  */

static void connection_closed (flag data_deallocated)
/*  This routine is called when the "multi_array" connection closes.
    If there was data on the connection, the value of  data_deallocated  will
    be TRUE.
    The routine returns nothing.
*/
{
    fprintf (stderr, "conn close...\n");
    /*  Deallocate old ViewableImage objects  */
    /*free_data ();*/
    fprintf (stderr, "Destroyed viewable images...\n");
}   /*  End Function connection_closed  */

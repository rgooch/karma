/*  main_xt.c

    Main file for  kcolourmap  (X11 colourmap control tool for Karma).

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
    This Karma module will enable on-screen imaging of a single arrayfile of
    the general data structure format. The arrayfile must contain a 2 or 3
    dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   17-MAR-1996

    Updated by      Richard Gooch   21-APR-1996: Made use of
  XkwNsimpleColourbar resource for Cmapwin widget.

    Updated by      Richard Gooch   4-MAY-1996: Switched to KtoggleWidget.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   15-SEP-1996: Made use of new <kwin_xutil_*>
  routines.


*/
#include <stdio.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <karma.h>
#include <k_event_codes.h>
#include <karma_xtmisc.h>
#include <karma_dsxfr.h>
#include <karma_kwin.h>
#include <karma_conn.h>
#include <karma_dir.h>
#include <karma_chx.h>
#include <karma_im.h>
#include <karma_hi.h>
#include <karma_xc.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_r.h>
#include <karma_m.h>
#include <Xkw/Cmapwin.h>
#include <Xkw/Dialogpopup.h>
#include <Xkw/Filepopup.h>
#include <Xkw/Filewin.h>
#include <Xkw/Ktoggle.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Label.h>
#include <X11/Xaw/Box.h>
#ifdef OS_SunOS
#  include <floatingpoint.h>
#endif

#define DEFAULT_COLOURMAP_NAME "Greyscale1"
#define NUM_COLOURS (unsigned int) 200

#define VERSION "2.0"


/*  Private functions  */
STATIC_FUNCTION (void quit_cbk, (Widget w, XtPointer client_data,
				 XtPointer call_data) );
STATIC_FUNCTION (void reverse_cbk, (Widget w, XtPointer client_data,
				    XtPointer call_data) );
STATIC_FUNCTION (void invert_cbk, (Widget w, XtPointer client_data,
				   XtPointer call_data) );
STATIC_FUNCTION (flag accept_file, (KFileInfo finfo) );
STATIC_FUNCTION (void cmap_got_one,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void save_dialog_cbk,
		 (Widget w,XtPointer client_data,XtPointer call_data) );
STATIC_FUNCTION (Widget create_button,
		 (char *name, WidgetClass type, char *label, Widget parent,
		  Widget left, flag small) );
STATIC_FUNCTION (void setup_comms, (Display *display) );


/*  Private data  */
String fallback_resources[] =
{
    "Kcolourmap*Command*background:                       grey70",
    "Kcolourmap*Repeater*background:                      grey70",
    "Kcolourmap*Ktoggle*background:                       grey80",
    "Kcolourmap*closeButton*background:                   grey90",
    "Kcolourmap*ChoiceMenu.background:                    turquoise",
    "Kcolourmap*ExclusiveMenu.background:                 turquoise",
    "Kcolourmap*Value*background:                         #d0a0a0",
    "Kcolourmap*quitButton*background:                    orange",
    "Kcolourmap*SimpleSlider.foreground:                  Sea Green",
    "Kcolourmap*background:                               aquamarine",
    "Kcolourmap*font:                                     9x15bold",
    NULL
};
static Kcolourmap kcmap = NULL;
static Widget colourmapwin = NULL;
static Widget filepopup = NULL;


int main (int argc, char **argv)
{
    Kdisplay dpy_handle;
    unsigned int num_ccels = NUM_COLOURS;
    XtAppContext app_context;
    Widget main_shell;
    Widget form, reverse_tgl, invert_tgl, quit_btn;
    Widget save_dialog, save_btn, load_btn;
    Colormap xcmap;
    unsigned long *pixel_values;
    Display *dpy;
    XVisualInfo *vinfo;
    Visual *pseudocolour_visual;
    Visual *root_visual;
    Screen *screen;
    static char function_name[] = "main";

#ifdef SIGFPE_ABORT
	ieee_handler ("set", "division", SIGFPE_ABORT);
#endif
    /*  Initialise module  */
    im_register_module_name ("kcolourmap");
    im_register_module_version_date (VERSION);
    im_register_lib_version (KARMA_VERSION);
    /*  Start up Xt  */
    main_shell = XtVaAppInitialize (&app_context, "Kcolourmap",
				    NULL, 0,
				    &argc, argv, fallback_resources,
				    NULL, 0);
    if (argc == 3)
    {
	if (strcmp (argv[1], "-num_colours") == 0)
	{
	    num_ccels = atoi (argv[2]);
	}
    }
    /*  Initialise communications  */
    chx_register_app_context (app_context);
    conn_register_managers (chx_manage, chx_unmanage, ( void (*) () ) NULL);
    dpy = XtDisplay (main_shell);
    screen = XtScreen (main_shell);
    setup_comms (dpy);
    /*  Get visual information  */
    root_visual = XDefaultVisualOfScreen (screen);
    vinfo = kwin_xutil_get_visinfo_for_visual (dpy, root_visual);
    kwin_xutil_get_visuals (screen, &pseudocolour_visual, NULL, NULL);
    if (pseudocolour_visual == NULL)
    {
	(void) fprintf (stderr, "No 8 bit PseudoColour visual available\n");
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
	    (void) fprintf (stderr, "Could not create colourmap\n");
	    exit (1);
	}
	XSync (dpy, False);
	(void) fprintf (stderr,
			"Created colourmap: 0x%lx for PseudoColour visual\n",
			xcmap);
    }
    if ( ( dpy_handle = xc_get_dpy_handle (dpy, xcmap) ) == NULL )
    {
	(void) fprintf (stderr, "Error getting display handle\n");
	a_prog_bug (function_name);
    }
    /*  Initialise colourmap  */
    if ( ( kcmap = kcmap_va_create (DEFAULT_COLOURMAP_NAME,
				    num_ccels, TRUE,
				    dpy_handle,
				    xc_alloc_colours, xc_free_colours,
				    xc_store_colours, xc_get_location,
				    KCMAP_ATT_END) )
	== NULL )
    {
	(void) fprintf (stderr, "Error creating main colourmap\n");
	a_prog_bug (function_name);
    }
    num_ccels = kcmap_get_pixels (kcmap, &pixel_values);
    (void) fprintf (stderr, "num colours: %u\n", num_ccels);
    form = XtVaCreateManagedWidget ("topForm", formWidgetClass, main_shell,
				    XtNborderWidth, 0,
				    NULL);
    reverse_tgl = create_button ("reverseToggle", ktoggleWidgetClass,"Reverse",
				 form, NULL, False);
    invert_tgl = create_button ("invertToggle", ktoggleWidgetClass, "Invert",
				form, reverse_tgl, True);
    save_btn = create_button ("saveButton", commandWidgetClass, "Save",
			      form, invert_tgl, False);
    load_btn = create_button ("loadButton", commandWidgetClass, "Load",
			      form, save_btn, False);
    quit_btn = create_button ("quitButton", commandWidgetClass, "Quit",
			      form, load_btn, False);
    XtAddCallback (quit_btn, XtNcallback, quit_cbk, NULL);
    XtAddCallback (reverse_tgl, XtNcallback, reverse_cbk, NULL);
    XtAddCallback (invert_tgl, XtNcallback, invert_cbk, NULL);
    colourmapwin = XtVaCreateManagedWidget ("cmapwin", cmapwinWidgetClass,form,
					    XkwNkarmaColourmap, kcmap,
					    XtNvisual, pseudocolour_visual,
					    XkwNsimpleColourbar, True,
					    XtNborderWidth, 0,
					    XtNfromVert, quit_btn,
					    NULL);
    save_dialog = XtVaCreatePopupShell ("save_dialog", dialogpopupWidgetClass,
					main_shell,
					XtNlabel, "Save Filename:",
					NULL);
    XtAddCallback (save_dialog, XtNcallback, save_dialog_cbk, NULL);
    XtAddCallback (save_btn, XtNcallback, xtmisc_popup_cbk, save_dialog);
    filepopup = XtVaCreatePopupShell ("filepopup", filepopupWidgetClass,
				      main_shell,
				      XkwNfilenameTester, accept_file,
				      XtNtitle, "Colourmap File Selector",
				      XkwNautoPopdown, True,
				      NULL);
    XtAddCallback (filepopup, XkwNfileSelectCallback, cmap_got_one, NULL);
    XtAddCallback (load_btn, XtNcallback, xtmisc_popup_cbk, filepopup);
    XtRealizeWidget (main_shell);
    XtAppMainLoop (app_context);
    return (RV_OK);
}   /*  End Function main   */

static void quit_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the quit button callback.
*/
{
    exit (RV_OK);
}   /*  End Function quit_cbk   */

static void reverse_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the reverse toggle callback.
*/
{
    flag bool = (flag) call_data;

    kcmap_set_attributes (kcmap,
			  KCMAP_ATT_REVERSE, bool,
			  KCMAP_ATT_END);
    XtVaSetValues (colourmapwin,
		   XkwNregenerateColourmap, True,
		   NULL);
}   /*  End Function reverse_cbk  */

static void invert_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  This is the invert toggle callback.
*/
{
    flag bool = (flag) call_data;

    kcmap_set_attributes (kcmap,
			  KCMAP_ATT_INVERT, bool,
			  KCMAP_ATT_END);
    XtVaSetValues (colourmapwin,
		   XkwNregenerateColourmap, True,
		   NULL);
}   /*  End Function invert_cbk  */

static flag accept_file (KFileInfo finfo)
{
    if (finfo.type == KFILETYPE_DIRECTORY) return TRUE;
    if (strcmp (finfo.filename + strlen (finfo.filename) - 3, ".kf") == 0)
    {
	return TRUE;
    }
    /*  Reject everything else  */
    return FALSE;
}   /*  End Function accept_file  */

static void cmap_got_one (Widget w, XtPointer client_data, XtPointer call_data)
{
    char *fname = (char *) call_data;
    multi_array *multi_desc;

    if ( ( multi_desc = dsxfr_get_multi (fname, FALSE, K_CH_MAP_NEVER, FALSE) )
	== NULL )
    {
	XBell (XtDisplay (w), 100);
	return;
    }
    kcmap_copy_from_struct (kcmap, multi_desc->headers[0],
			    multi_desc->data[0]);
    ds_dealloc_multi (multi_desc);
}   /*  End Function cmap_got_one  */

static void save_dialog_cbk(Widget w,XtPointer client_data,XtPointer call_data)
{
    char fname[500];
    char *cwd;
    char *filename = (char *) call_data;
    multi_array *multi_desc;
    Widget filewin;
    static char function_name[] = "CmapwinpopupWidget::save_dialog_cbk";

    filewin = XtNameToWidget (filepopup, "*selector");
    if ( !XtIsFilewin (filewin) )
    {
	(void) fprintf (stderr, "Could not find filewin widget\n");
	a_prog_bug (function_name);
    }
    if ( ( multi_desc = ds_alloc_multi (1) ) == NULL )
    {
	m_abort (function_name, "multi_array descriptor");
    }
    kcmap_copy_to_struct (kcmap, multi_desc->headers, multi_desc->data);
    cwd = XkwFilewinCurrentDirectory (filewin);
    (void) sprintf (fname, "%s/%s", cwd, filename);
    dsxfr_put_multi (fname, multi_desc);
    ds_dealloc_multi (multi_desc);
}   /*  End Function save_dialog_cbk  */

static Widget create_button (char *name, WidgetClass type, char *label,
			     Widget parent, Widget left, flag small)
{
    int width = 73;

    if (small) width -= 2;

    return XtVaCreateManagedWidget (name, type, parent,
				    XtNlabel, label,
				    XkwNcrosses, False,
				    XtNheight, 20,
				    XtNwidth, width,
				    XtNfromHoriz, left,
				    XtNtop, XtChainTop,
				    XtNbottom, XtChainTop,
				    NULL);
}   /*  End Function create_button  */

static void setup_comms (Display *display)
/*  This routine will initialise the kslice_3d module communications.
    The display the module is connected to must be pointed to by  display  .
    The routine returns nothing.
*/
{
    int def_port_number;
    unsigned int server_port_number;
    extern char module_name[STRING_LENGTH + 1];

    /*  Get default port number  */
    if ( ( def_port_number = r_get_def_port ( module_name,
					     DisplayString (display) ) ) < 0 )
    {
	(void) fprintf (stderr, "Could not get default port number\n");
	return;
    }
    server_port_number = def_port_number;
    if (conn_become_server (&server_port_number, CONN_MAX_INSTANCES) != TRUE)
    {
	(void) fprintf (stderr, "Module not operating as Karma server\n");
    }
    else
    {
	(void) fprintf (stderr, "Port allocated: %d\n", server_port_number);
    }
}   /*  End Function setup_comms  */

/*  contour_xt.c

    Contour file for  koverlay  (X11 image+contour display tool for Karma).

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


    Written by      Richard Gooch   13-OCT-1996

    Last updated by Richard Gooch   16-OCT-1996: Renamed to <koverlay>.


*/
#include <stdio.h>
#include <ctype.h>
#include <math.h>
#include <X11/Xatom.h>
#include <X11/Intrinsic.h>
#include <X11/StringDefs.h>
#include <X11/Xaw/Form.h>
#include <X11/Xaw/Command.h>
#include <X11/Xaw/Dialog.h>
#include <karma.h>
#define NEW_WIN_SCALE
#include <k_event_codes.h>
#include <karma_contour.h>
#include <karma_xtmisc.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_im.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <Xkw/ImageDisplay.h>
#include <Xkw/Filewin.h>
#include <Xkw/TracePopup.h>
#ifdef OS_SunOS
#  include <floatingpoint.h>
#endif
#include "koverlay.h"


/*  Private functions  */
STATIC_FUNCTION (void apply_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );
STATIC_FUNCTION (void reset_cbk,
		 (Widget w, XtPointer client_data, XtPointer call_data) );


/*  Public functions follow  */

Widget create_contour_popup (Widget main_shell)
/*  [SUMMARY] Create contour control popup window.
    <main_shell> The main shell for the application.
    [RETURNS] The popup widget
*/
{
    Widget popup, form, close_btn, apply_btn, reset_btn, w;

    popup = XtVaCreatePopupShell ("contourwinpopup",
				  topLevelShellWidgetClass, main_shell,
				  XtNtitle, "Contour Control Window",
				  NULL);
    form = XtVaCreateManagedWidget ("form", formWidgetClass, popup,
				    XtNborderWidth, 0,
				    NULL);
    w = XtVaCreateManagedWidget ("closeButton", commandWidgetClass, form,
				 XtNlabel, "Close",
				 NULL);
    close_btn = w;
    XtAddCallback (w, XtNcallback, xtmisc_popdown_cbk, popup);
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
				 XtNlabel, "Apply",
				 XtNfromHoriz, close_btn,
				 NULL);
    apply_btn = w;
    w = XtVaCreateManagedWidget ("button", commandWidgetClass, form,
				 XtNlabel, "Reset",
				 XtNfromHoriz, apply_btn,
				 NULL);
    reset_btn = w;
    w = XtVaCreateManagedWidget ("contourLevels", dialogWidgetClass, form,
				 XtNlabel, "Contour Levels                   ",
				 XtNborderWidth, 0,
				 XtNfromVert, w,
				 XtNvalue, "%10:90+10",
				 NULL);
    XtAddCallback (apply_btn, XtNcallback, apply_cbk, w);
    XtAddCallback (reset_btn, XtNcallback, reset_cbk, w);
    return (popup);
}   /*  End Function create_contour_popup  */

static void apply_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [SUMMARY] Contour levels change callback.
    <w> The widget the event occurred on.
    <client_data> Client data.
    <call_data> Call data.
    [RETURNS] Nothing.
*/
{
    flag relative;
    flag geometric;
    unsigned int count;
    double value, start, stop, increment, tmp, first_inc;
    Widget dlg = (Widget) client_data;
    char *level_str, *p;
    extern KContourImage pc_cimage, rgb_cimage;
    extern unsigned int num_cimages, num_contour_levels;
    extern double contour_arr_min, contour_arr_max;
    extern KContourImage *pc_cimages, *rgb_cimages;
    extern double contour_levels[MAX_CONTOUR_LEVELS];
    /*static char function_name[] = "apply_cbk";*/

    XtVaGetValues (dlg,
		   XtNvalue, &level_str,
		   NULL);
    p = level_str;
    for (num_contour_levels = 0; p != NULL; )
    {
	if (*p == '%')
	{
	    relative = TRUE;
	    ++p;
	}
	else relative = FALSE;
	/*  Get a value  */
	value = ex_float (p, &p);
	if ( (p == NULL) || isspace (*p) )
	{
	    /*  End of the line or separate value follows  */
	    if (num_contour_levels >= MAX_CONTOUR_LEVELS)
	    {
		fprintf (stderr, "Too many contour levels: maximum is %u\n",
			 MAX_CONTOUR_LEVELS);
		return;
	    }
	    if (relative) value = contour_arr_min +
			      value * 1e-2 * (contour_arr_max -
					      contour_arr_min);
	    contour_levels[num_contour_levels++] = value;
	    continue;
	}
	/*  Special character  */
	if (*p != ':')
	{
	    fprintf (stderr, "Illegal control character: '%c'\n", *p);
	    return;
	}
	start = value;
	/*  Get stop and increment values  */
	stop = ex_float (p, &p);
	if (p == NULL)
	{
	    fprintf (stderr, "increment character, found end of line\n");
	    return;
	}
	switch (*p)
	{
	  case '+':
	    geometric = FALSE;
	    break;
	  case '*':
	    geometric = TRUE;
	    break;
	  default:
	    fprintf (stderr,
		     "Missing increment character, found: '%c' instead\n", *p);
	    return;
	    /*break;*/
	}
	++p;
	increment = ex_float (p, &p);
	first_inc = TOOBIG;
	for (value = start; value <= stop; )
	{
	    if (num_contour_levels >= MAX_CONTOUR_LEVELS)
	    {
		fprintf (stderr, "Too many contour levels: maximum is %u\n",
			 MAX_CONTOUR_LEVELS);
		return;
	    }
	    if (relative)
	    {
		tmp = contour_arr_min + value * 1e-2 * (contour_arr_max -
							contour_arr_min);
	    }
	    else tmp = value;
	    contour_levels[num_contour_levels++] = tmp;
	    if (geometric) value *= increment;
	    else value += increment;
	    if (first_inc >= TOOBIG) first_inc = value - start;
	    if (fabs (stop - value) < first_inc * 1e-5) value = stop;
	}
    }
    if (pc_cimage != NULL) contour_set_levels (pc_cimage, num_contour_levels,
					       contour_levels, NULL);
    if (rgb_cimage != NULL) contour_set_levels (rgb_cimage, num_contour_levels,
						contour_levels, NULL);
    for (count = 0; count < num_cimages; ++count)
    {
	if ( (pc_cimages != NULL) && (pc_cimages[count] != NULL) )
	    contour_set_levels (pc_cimages[count], num_contour_levels,
				contour_levels, NULL);
	if ( (rgb_cimages != NULL) && (rgb_cimages[count] != NULL) )
	    contour_set_levels (rgb_cimages[count], num_contour_levels,
				contour_levels, NULL);
    }
    refresh_canvas ();
}   /*  End Function apply_cbk  */

static void reset_cbk (Widget w, XtPointer client_data, XtPointer call_data)
/*  [SUMMARY] Contour levels reset callback.
    <w> The widget the event occurred on.
    <client_data> Client data.
    <call_data> Call data.
    [RETURNS] Nothing.
*/
{
    Widget dlg = (Widget) client_data;
    /*static char function_name[] = "reset_cbk";*/

    XtVaSetValues (dlg,
		   XtNvalue, "%10:90+10",
		   NULL);
    apply_cbk (NULL, (XtPointer) dlg, NULL);
}   /*  End Function reset_cbk  */

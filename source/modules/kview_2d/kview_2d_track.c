/*  kview_2d_track.c

    Track event consumer file for  kview_2d  (X11 image display tool for Karma)

    Copyright (C) 1993  Richard Gooch

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
    the general data structure format. The arrayfile must contain a 2
    dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   12-JUL-1993

    Last updated by Richard Gooch   28-SEP-1993


*/
#include <stdio.h>
#include <string.h>
#include <X11/Xlib.h>
#include <k_event_codes.h>
#include <karma_viewimg.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>
#include "kview_2d.h"


/*  External functions  */


/*  Local functions  */
void draw_track_window ();
flag compute_track_string ();


/*  Private data  */
static char window_string[STRING_LENGTH] = "";


void draw_track_window (display, window, win_width, win_height)
Display *display;
Window window;
int win_width;
int win_height;
{
    int screen;
    int string_length;
    int string_pixels;
    int x_offset;
    int y_offset;
    XGCValues gcvalues;
    XFontStruct *new_font;
    extern char *track_font_name;
    extern char window_string[STRING_LENGTH];
    static int old_width = -1;
    static int old_height = -1;
    static GC gc;
    static flag gc_set = FALSE;
    static Display *dpy = NULL;
    static Window win = (Window) NULL;
    static char *current_font_name;
    static XFontStruct *font = NULL;

    if (dpy == NULL)
    {
	if (display == NULL)
	{
	    (void) fprintf (stderr, "No display for track window repaint\n");
	    return;
	}
	dpy = display;
	win = window;
    }
    if (display == NULL)
    {
	display = dpy;
	window = win;
	win_width = old_width;
	win_height = old_height;
    }
    else
    {
	old_width = win_width;
	old_height = win_height;
    }
    screen = DefaultScreen (display);
    if (gc_set != TRUE)
    {
	/*  Need to create Graphics Context  */
	gcvalues.background = WhitePixel (display, screen);
	gcvalues.foreground = BlackPixel (display, screen);
	/*  Try to load named font  */
	current_font_name = track_font_name;
	if ( ( font = XLoadQueryFont (display, track_font_name) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "Error opening font: \"%s\"  using default\n",
			    track_font_name);
	    gc = XCreateGC (display, window, GCForeground | GCBackground,
			    &gcvalues);
	}
	else
	{
	    gcvalues.font = (*font).fid;
	    gc = XCreateGC (display, window,
			    GCForeground | GCBackground | GCFont,
			    &gcvalues);
	}
	gc_set = TRUE;
    }
    else
    {
	/*  Graphics context has already been created: check for font name  */
	if (strcmp (current_font_name, track_font_name) != 0)
	{
	    /*  Font name has changed  */
	    if ( ( new_font = XLoadQueryFont (display, track_font_name) )
		== NULL )
	    {
		(void) fprintf (stderr,
				"Error opening font: \"%s\"  using old font\n",
				track_font_name);
	    }
	    else
	    {
		/*  Loaded font: change GC  */
		XSetFont (display, gc, (*new_font).fid);
		/*  Free old font  */
		if (font != NULL)
		{
		    XFreeFont (display, font);
		}
		font = new_font;
	    }
	}
    }
    /*  Determine height  */
    if ( (*font).ascent + (*font).descent > win_height )
    {
	(void) fprintf (stderr, "Track font too large for track window\n");
	return;
    }
    y_offset = win_height / 2;
    if (y_offset + 1 - (*font).ascent < 0)
    {
	/*  Top would be chopped  */
	y_offset = (*font).ascent - 1;
    }
    else if (y_offset + (*font).descent > win_height)
    {
	/*  Bottom would be chopped  */
	y_offset = win_height - (*font).descent;
    }
    /*  Determine width  */
    string_length = strlen (window_string);
    string_pixels = XTextWidth (font, window_string, string_length);
    if (string_pixels > win_width)
    {
	(void) fprintf (stderr,
			"Track string: \"%s\" too large for track window\n",
			window_string);
	return;
    }
    x_offset = win_width / 2 - string_pixels / 2;
    if (x_offset < 0)
    {
	/*  Left would be chopped  */
	x_offset = 0;
    }
    else if (x_offset + string_pixels > win_width)
    {
	/*  Right would be chopped  */
	x_offset = 0;
    }
    /*  Clear the window first (it is not auto-cleared)  */
    XClearWindow (display, window);
    XDrawString (display, window, gc, x_offset, y_offset, window_string,
		 string_length);
}   /*  End Function draw_track_window  */

flag track_canvas_event (vimage, x, y, value, event_code, e_info,f_info)
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
    extern char window_string[STRING_LENGTH];
    static char function_name[] = "track_canvas_event";

    if (event_code != K_CANVAS_EVENT_POINTER_MOVE) return (FALSE);
    (void) sprintf (window_string, "abs: %e    ord: %e        value: %e",
		    x, y, value[0]);
    draw_track_window ( (Display *) NULL, NULL, -1, -1);
    return (TRUE);
}   /*  End Function track_canvas_event  */

/*  kmovie_colour.c

    Colour control file for  kmovie  (X11 movie display tool for Karma).

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
    This Karma module will enable on-screen image animation of a single
    arrayfile of the general data structure format. The arrayfile must contain
    a 3 dimensional "Intelligent Array".
    This module runs on an X11 server.
    This file contains the various functions to manipulate colourmaps.


    Written by      Richard Gooch   8-SEP-1993: Copied from  kview_2d_colour.c

    Last updated by Richard Gooch   20-SEP-1993


*/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <ctype.h>
#include <karma.h>
#include <karma_kcmap.h>
#include <karma_kwin.h>
#include <karma_conn.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>
#include <karma_r.h>
#include "kmovie.h"


/*  Local functions  */
void register_shimmer_toggle ();
flag update_shimmer_colourmap ();
flag attempt_slave_cmap_connection (/* full */);


/*  External functions  */


void refresh_colourbar (canvas, width, height, info)
/*  This routine registers a refresh event consumer for a pixel canvas.
    The canvas is given by  canvas  .
    The width of the canvas is given by  width  .
    The height of the canvas is given by  height  .
    The arbitrary canvas information pointer is pointed to by  info  .
    The routine returns nothing.
*/
KPixCanvas canvas;
int width;
int height;
void **info;
{
    Kcolourmap cmap;
    int x;
    int x_offset;
    int pixel_repeat;
    int colour_count;
    int pixel_count;
    int line_length;
    unsigned int num_allocated_pixels;
    unsigned long *pixel_values;
    static char function_name[] = "refresh_colourbar";

    if ( (cmap = (Kcolourmap) *info) == NULL )
    {
	(void) fprintf (stderr, "NULL colourmap\n");
	a_prog_bug (function_name);
    }
    num_allocated_pixels = kcmap_get_pixels (cmap, &pixel_values);
    if (num_allocated_pixels > width)
    {
	(void) fprintf (stderr, "Window too narrow to fit colourbar%c\n", BEL);
	return;
    }
    pixel_repeat = width / num_allocated_pixels;
    line_length = pixel_repeat * num_allocated_pixels;
    x_offset = (width - line_length) / 2;
    /*  Draw all lines of the colourbar  */
    for (colour_count = 0, x = x_offset;
	 colour_count < num_allocated_pixels;
	 ++colour_count)
    {
	/*  Draw a colour  */
	for (pixel_count = 0; pixel_count < pixel_repeat;
	     ++pixel_count, ++x)
	{
	    kwin_draw_line (canvas, x, 0, x, height - 1,
			    pixel_values[colour_count]);
	}
    }
#ifdef dummy
    if (x_offset > 0)
    {
	kwin_draw_line (canvas, x_offset, 0, x_offset, height - 1, BLACK);
    }
    if (x_offset + (int) line_length < width - 1)
    {
	kwin_draw_line (canvas, x_offset + line_length, 0,
			x_offset + line_length, height - 1, BLACK);
    }
#endif
}   /*  End Function refresh_colourbar  */

void colourbar_cmap_resize_func (cmap, info)
/*  This routine registers a change in the size of a colourmap.
    The colourmap must be given by  cmap  .
    The arbitrary colourmap information pointer is pointed to by  info  .
    The routine returns nothing.
*/
Kcolourmap cmap;
void **info;
{
    KPixCanvas canvas;
    int width;
    int height;
    static char function_name[] = "colourbar_cmap_resize_func";

    if ( (canvas = (KPixCanvas *) *info) == NULL )
    {
	(void) fprintf (stderr, "NULL pixel canvas\n");
	a_prog_bug (function_name);
    }
    kwin_resize (canvas, TRUE, 0, 0, 0, 0);
}   /*  End Function colourbar_cmap_resize_func  */

flag attempt_slave_cmap_connection (full)
/*  This routine will attempt to initiate a slave colourmap connection to a
    colourmap server specified by the user.
    If the value of  full  is TRUE, the routine will attempt a "full_colourmap"
    connection, else it will try a "colourmap_indices" connection.
    The routine returns TRUE on success, else it returns FALSE.
*/
flag full;
{
    flag return_value;
    int serv_port;
    char *serv_host;
    char *serv_hostname;
    char *serv_name;
    extern int cmap_port;
    extern char *cmap_display;
    extern Display *my_display;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    extern char module_name[];
    static char function_name[] = "attempt_slave_cmap_connection";

    if (cmap_display != NULL)
    {
	/*  User specified a display to get colourmap from  */
	serv_name = cmap_display;
    }
    else
    {
	/*  Must look at display that was connected to  */
	serv_name = DisplayString (my_display);
    }
    if (cmap_port < 0)
    {
	if ( ( serv_port = r_get_def_port (module_name, serv_name) ) < 0 )
	{
	    (void) fprintf (stderr,
			    "Error getting colourmap server port number\n");
	    return (FALSE);
	}
    }
    else
    {
	serv_port = cmap_port;
    }
    /*  Get host to connect to  */
    if ( ( serv_host = r_get_host_from_display (serv_name) ) == NULL )
    {
	(void) fprintf (stderr, "Error connecting to service\n");
	return (FALSE);
    }
    if ( ( serv_hostname = st_dup (serv_host) ) == NULL )
    {
	m_error_notify (function_name, "server hostname");
	return (FALSE);
    }
    if (full == TRUE)
    {
	return_value = conn_attempt_connection (serv_hostname,
						(unsigned int) serv_port,
						"full_colourmap");
    }
    else
    {
	return_value = conn_attempt_connection (serv_hostname,
						(unsigned int) serv_port,
						"colourmap_indices");
    }
    m_free (serv_hostname);
    return (return_value);
}   /*  End Function attempt_slave_cmap_connection  */

/*LINTLIBRARY*/
/*  main.c

    This code provides X colourmap manipulation routines.

    Copyright (C) 1992,1993,1994  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*

    This file contains the various utility routines for manipulating X
  colourmaps.


    Written by      Richard Gooch   25-FEB-1993

    Updated by      Richard Gooch   4-MAR-1993

    Updated by      Richard Gooch   22-NOV-1994: Moved typedef of  Kdisplay  to
  header file.

    Last updated by Richard Gooch   26-NOV-1994: Moved to  packages/xc/main.c


*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/types.h>
#include <X11/Xlib.h>
#include <karma.h>
#include <karma_xc.h>
#include <karma_a.h>
#include <karma_r.h>
#include <karma_m.h>


#define MAGIC_NUMBER (unsigned int) 587924389

struct kdisplay_handle_type
{
    unsigned int magic_number;
    Display *display;
    Colormap cmap;
};

/*PUBLIC_FUNCTION*/
Kdisplay xc_get_dpy_handle (display, cmap)
/*  This routine will generate a display handle for later use.
    The X display must be pointed to by  display  .
    The X colourmap ID must be given by  cmap  .
    The routine returns a pointer to the display handle on success,
    else it returns NULL.
*/
Display *display;
Colormap cmap;
{
    Kdisplay dpy_handle;
    static char function_name[] = "xc_get_dpy_handle";

    if ( ( dpy_handle = (Kdisplay) m_alloc (sizeof *dpy_handle) ) == NULL )
    {
	m_error_notify (function_name, "display handle");
	return (NULL);
    }
    (*dpy_handle).magic_number = MAGIC_NUMBER;
    (*dpy_handle).display = display;
    (*dpy_handle).cmap = cmap;
    return (dpy_handle);
}   /*  End Function xc_get_dpy_handle  */

/*PUBLIC_FUNCTION*/
unsigned int xc_alloc_colours (num_cells, pixel_values, min_cells, dpy_handle)
/*  This routine will allocate a number of colourcells in a low level
    colourmap (eg. using the Xlib routine XAllocColorCells).
    The number of colourcells to allocate must be given by  num_cells  .
    The pixel values allocated will be written to the array pointed to by
    pixel_values  .
    The minimum number of colourcells to allocate must be given by  min_cells
    The routine will try to allocate at least this number of colourcells.
    The low level display handle must be pointed to by  dpy_handle  .The
    meaning of this value depends on the lower level graphics library used.
    The routine returns the number of colourcells allocated.
*/
unsigned int num_cells;
unsigned long *pixel_values;
unsigned int min_cells;
Kdisplay dpy_handle;
{
    unsigned long dummy;
    static char function_name[] = "xc_alloc_colours";

    if ( (*dpy_handle).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    if (num_cells < 1)
    {
	(void) fprintf (stderr, "num_cells must not be zero\n");
	a_prog_bug (function_name);
    }
    if (min_cells < 1)
    {
	(void) fprintf (stderr, "min_cells must not be zero\n");
	a_prog_bug (function_name);
    }
    /*  Try to get as many colourcells as possible  */
    for (; num_cells >= min_cells; --num_cells)
    {
	if (XAllocColorCells ( (*dpy_handle).display, (*dpy_handle).cmap,
			      False, &dummy, 0, pixel_values, num_cells )
	    != 0)
	{
	    /*  Success  */
	    return (num_cells);
	}
	/*  Try again with less cells  */
    }
    /*  Could not allocate enough colourcells  */
    return (0);
}   /*  End Function xc_alloc_colours  */

/*PUBLIC_FUNCTION*/
void xc_free_colours (num_cells, pixel_values, dpy_handle)
/*  This routine will free a number of colourcells in a low level colourmap
    The number of colourcells to free must be given by  num_cells  .
    The pixel values (colourcells) to free must be pointed to by
    pixel_values  .
    The low level display handle must be pointed to by  dpy_handle  .The
    meaning of this value depends on the lower level graphics library used.
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned long *pixel_values;
Kdisplay dpy_handle;
{
    static char function_name[] = "xc_free_colours";

    if ( (*dpy_handle).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    XFreeColors ( (*dpy_handle).display, (*dpy_handle).cmap, pixel_values,
		 (int) num_cells, 0 );
}   /*  End Function xc_free_colours  */

/*PUBLIC_FUNCTION*/
void xc_store_colours (num_cells, pixel_values, reds, greens, blues, stride,
		       dpy_handle)
/*  This routine will store colours into a low level colourmap.
    The number of colourcells to store must be given by  num_cells  .
    The pixel values must be pointed to by  pixel_values  .
    The red intensity values must be pointed to by  reds  .
    The green intensity values must be pointed to by  greens  .
    The blue intensity values must be pointed to by  blues  .
    The stride (in unsigned shorts) between intensity values in each array
    must be given by  stride  .
    The low level display handle must be pointed to by  dpy_handle  .The
    meaning of this value depends on the lower level graphics library used.
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned long *pixel_values;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
Kdisplay dpy_handle;
{
    unsigned int count;
    static unsigned int old_length = 0;
    static XColor *xcolours = NULL;
    static char function_name[] = "xc_store_colours";

    if ( (*dpy_handle).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    if (num_cells > old_length)
    {
	if (xcolours != NULL)
	{
	    m_free ( (char *) xcolours );
	    xcolours = NULL;
	    old_length = 0;
	}
	if ( ( xcolours = (XColor *) m_alloc (sizeof *xcolours * num_cells) )
	    == NULL )
	{
	    m_abort (function_name, "array of XColor structures");
	}
	old_length = num_cells;
    }
    for (count = 0; count < num_cells; ++count)
    {
	xcolours[count].pixel = pixel_values[count];
	xcolours[count].red = reds[count * stride];
	xcolours[count].green = greens[count * stride];
	xcolours[count].blue = blues[count * stride];
	xcolours[count].flags = DoRed | DoGreen | DoBlue;
    }
    XStoreColors ( (*dpy_handle).display, (*dpy_handle).cmap, xcolours,
		  (int) num_cells );
}   /*  End Function xc_store_colours  */

/*PUBLIC_FUNCTION*/
void xc_get_location (dpy_handle, serv_hostaddr, serv_display_num)
/*  This routine will determine the location of the graphics display being
    used.
    The low level display handle must be given by  dpy_handle  .The meaning
    of this value depends on the lower level graphics library used.
    The Internet address of the host on which the display is running will
    be written to the storage pointed to by  serv_hostaddr  .
    The number of the display will be written to the storage pointed to by
    serv_display_num  .
    The routine returns nothing.
*/
Kdisplay dpy_handle;
unsigned long *serv_hostaddr;
unsigned long *serv_display_num;
{
    int display_num;
    char *display_string;
    char *serv_hostname;
    static char function_name[] = "xc_get_location";

    if ( (*dpy_handle).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    display_string = DisplayString ( (*dpy_handle).display );
    if ( ( serv_hostname = r_get_host_from_display (display_string) )
	== NULL )
    {
	(void) fprintf (stderr, "Error getting X display hostname\n");
	exit (RV_UNDEF_ERROR);
    }
    if ( ( *serv_hostaddr = r_get_inet_addr_from_host (serv_hostname,
						       (flag *) NULL) ) == 0 )
    {
	(void) fprintf (stderr, "Error getting Internet address of: \"%s\"\n",
			serv_hostname);
	exit (RV_SYS_ERROR);
    }
    if ( ( display_num =
	  r_get_display_num_from_display (display_string) )
	< 0 )
    {
	(void) fprintf (stderr, "Error getting X display number\n");
	exit (RV_UNDEF_ERROR);
    }
    *serv_display_num = display_num;
}   /*  End Function xc_get_location  */

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
Kdisplay xc_get_dpy_handle (Display *display, Colormap cmap)
/*  [PURPOSE] This routine will generate a display handle for later use.
    <display> The X display.
    <cmap> The X colourmap ID.
    [RETURNS] A pointer to the display handle on success, else NULL.
*/
{
    Kdisplay dpy_handle;
    static char function_name[] = "xc_get_dpy_handle";

    if ( ( dpy_handle = (Kdisplay) m_alloc (sizeof *dpy_handle) ) == NULL )
    {
	m_error_notify (function_name, "display handle");
	return (NULL);
    }
    dpy_handle->magic_number = MAGIC_NUMBER;
    dpy_handle->display = display;
    dpy_handle->cmap = cmap;
    return (dpy_handle);
}   /*  End Function xc_get_dpy_handle  */

/*PUBLIC_FUNCTION*/
unsigned int xc_alloc_colours (unsigned int num_cells,
			       unsigned long *pixel_values,
			       unsigned int min_cells, Kdisplay dpy_handle)
/*  [PURPOSE] This routine will allocate a number of colourcells in a low level
    colourmap (eg. using the Xlib routine XAllocColorCells).
    <num_cells> The number of colourcells to allocate.
    <pixel_values> A pointer to the array where the pixel values allocated will
    be written.
    <min_cells> The minimum number of colourcells to allocate. The routine will
    try to allocate at least this number of colourcells.
    <dpy_handle> The low level display handle. The meaning of this value
    depends on the lower level graphics library used.
    [RETURNS] The number of colourcells allocated.
*/
{
    unsigned long dummy;
    static char function_name[] = "xc_alloc_colours";

    if (dpy_handle->magic_number != MAGIC_NUMBER)
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
	if (XAllocColorCells (dpy_handle->display, dpy_handle->cmap,
			      False, &dummy, 0, pixel_values, num_cells) != 0)
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
void xc_free_colours (unsigned int num_cells, unsigned long *pixel_values,
		      Kdisplay dpy_handle)
/*  [PURPOSE] This routine will free a number of colourcells in a low level
    colourmap.
    <num_cells> The number of colourcells to free.
    <pixel_values> The array of pixel values (colourcells) to free.
    <dpy_handle> The low level display handle. The meaning of this value
    depends on the lower level graphics library used.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "xc_free_colours";

    if (dpy_handle->magic_number != MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    XFreeColors (dpy_handle->display, dpy_handle->cmap, pixel_values,
		 (int) num_cells, 0);
}   /*  End Function xc_free_colours  */

/*PUBLIC_FUNCTION*/
void xc_store_colours (unsigned int num_cells, unsigned long *pixel_values,
		       unsigned short *reds, unsigned short *greens,
		       unsigned short *blues, unsigned int stride,
		       Kdisplay dpy_handle)
/*  [PURPOSE] This routine will store colours into a low level colourmap.
    <num_cells> The number of colourcells to store.
    <pixel_values> The array of pixel values.
    <reds> The array of red intensity values.
    <greens> The array of green intensity values.
    <blues> The array of blue intensity values.
    <stride> The stride (in unsigned shorts) between intensity values in each
    array.
    <dpy_handle> The low level display handle. The meaning of this value
    depends on the lower level graphics library used.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    static unsigned int old_length = 0;
    static XColor *xcolours = NULL;
    static char function_name[] = "xc_store_colours";

    if (dpy_handle->magic_number != MAGIC_NUMBER)
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
    XStoreColors (dpy_handle->display, dpy_handle->cmap, xcolours,
		  (int) num_cells);
}   /*  End Function xc_store_colours  */

/*PUBLIC_FUNCTION*/
void xc_get_location (Kdisplay dpy_handle, unsigned long *serv_hostaddr,
		      unsigned long *serv_display_num)
/*  [PURPOSE] This routine will determine the location of the graphics display
    being used.
    <dpy_handle> The low level display handle. The meaning of this value
    depends on the lower level graphics library used.
    <serv_hostaddr> The Internet address of the host on which the display is
    running will be written here.
    <serv_display_num> The number of the display will be written here.
    [RETURNS] Nothing.
*/
{
    int display_num;
    char *display_string;
    char *serv_hostname;
    static char function_name[] = "xc_get_location";

    if (dpy_handle->magic_number != MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    display_string = DisplayString (dpy_handle->display);
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

/*PUBLIC_FUNCTION*/
Colormap xc_get_cmap (Kdisplay dpy_handle)
/*  [PURPOSE] This routine will get the X11 colourmap for a display handle.
    <dpy_handle> The display handle.
    [RETURNS] The X11 colourmap.
*/
{
    static char function_name[] = "xc_get_cmap";

    if (dpy_handle->magic_number != MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    return (dpy_handle->cmap);
}   /*  End Function xc_get_cmap  */

/*LINTLIBRARY*/
/*PREFIX:"vc_"*/
/*  vc.c

    This code provides VX colourmap manipulation routines.

    Copyright (C) 1992,1993  Richard Gooch

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

    This file contains the various utility routines for manipulating VX
  colourmaps.


    Written by      Richard Gooch   5-MAR-1993

    Last updated by Richard Gooch   8-MAR-1993: Added 24 bit (DirectColour)
  support.


*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <karma_a.h>
#include <karma_r.h>
#include <karma_m.h>
#include <vx/vx.h>


typedef struct display_handle_type * Kdisplay;

#define KDISPLAY_DEFINED
#include <karma_vc.h>

#define MAGIC_NUMBER (unsigned int) 1598459834
#define COLOURMAP_SIZE 256

struct display_handle_type
{
    unsigned int magic_number;
    unsigned int cmap_index;
    flag pseudocolour;
    unsigned int *frame_buffer;
    flag allocated[COLOURMAP_SIZE];
    int colours_8bit[COLOURMAP_SIZE];
    int colours_24bit[COLOURMAP_SIZE];
};

/*PUBLIC_FUNCTION*/
Kdisplay vc_get_dpy_handle ()
/*  This routine will generate a display handle for later use.
    The routine returns a pointer to the display handle on success,
    else it returns NULL.
*/
{
    unsigned int count;
    Kdisplay dpy_handle;
    static flag already_called = FALSE;
    static char function_name[] = "vc_get_dpy_handle";

    if (already_called)
    {
	(void) fprintf (stderr, "Only one display handle available\n");
	a_prog_bug (function_name);
    }
    if ( ( dpy_handle = (Kdisplay) m_alloc (sizeof *dpy_handle) ) == NULL )
    {
	m_error_notify (function_name, "display handle");
	return (NULL);
    }
    (*dpy_handle).magic_number = MAGIC_NUMBER;
    (*dpy_handle).cmap_index = 0;
    (*dpy_handle).frame_buffer = (unsigned int *) 0x80000000;
    for (count = 0; count < COLOURMAP_SIZE; ++count)
    {
	(*dpy_handle).allocated[count] = FALSE;
	(*dpy_handle).colours_8bit[count] = 0;
	(*dpy_handle).colours_24bit[count] =count |(count << 8) |(count << 16);
    }
    vc_set_visual (dpy_handle, TRUE);
    return (dpy_handle);
}   /*  End Function vc_get_dpy_handle  */

/*PUBLIC_FUNCTION*/
unsigned int vc_alloc_colours (num_cells, pixel_values, min_cells, dpy_handle)
/*  This routine will allocate a number of colourcells in a low level
    colourmap.
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
    unsigned int count;
    unsigned int num_free_cells;
    unsigned int num_allocated;
    static char function_name[] = "vc_alloc_colours";

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
    /*  Determine number of free colourcells  */
    for (count = 0, num_free_cells = 0; count < COLOURMAP_SIZE; ++count)
    {
	if (!(*dpy_handle).allocated[count]) ++num_free_cells;
    }
    if (min_cells > num_free_cells)
    {
	/*  Not enough cells available  */
	return (0);
    }
    /*  Try to get as many colourcells as possible  */
    for (count = 0, num_allocated = 0;
	 (count < COLOURMAP_SIZE) && (num_allocated < num_cells);
	 ++count)
    {
	if (!(*dpy_handle).allocated[count])
	{
	    /*  Have another free cell  */
	    pixel_values[num_allocated++] = count;
	    (*dpy_handle).allocated[count] = TRUE;
	}
    }
    return (num_allocated);
}   /*  End Function vc_alloc_colours  */

/*PUBLIC_FUNCTION*/
void vc_free_colours (num_cells, pixel_values, dpy_handle)
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
    unsigned int count;
    static char function_name[] = "vc_free_colours";

    if ( (*dpy_handle).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    for (count = 0; count < num_cells; ++count)
    {
	if (!(*dpy_handle).allocated[ pixel_values[count] ])
	{
	    (void) fprintf (stderr, "Attempt to free colourcell: %d\n",
			    (int) pixel_values[count]);
	    a_prog_bug (function_name);
	}
	(*dpy_handle).allocated[ pixel_values[count] ] = FALSE;
    }
}   /*  End Function vc_free_colours  */

/*PUBLIC_FUNCTION*/
void vc_store_colours (num_cells, pixel_values, reds, greens, blues, stride,
		       dpy_handle)
/*  This routine will store colours into a low level colourmap. The colours are
    stored for the PseudoColour (8 bit) visual type.
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
    unsigned int colour;
    unsigned int count;
    Vx_color_map cmap;
    static char function_name[] = "vc_store_colours";

    if ( (*dpy_handle).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    for (count = 0; count < num_cells; ++count)
    {
	colour = reds[count * stride] >> 8;
	colour |= greens[count * stride] & 0xff00;
	colour |= ( (blues[count * stride] & 0xff00) << 8 );
	(*dpy_handle).colours_8bit[ pixel_values[count] ] = colour;
    }
    if ( (*dpy_handle).pseudocolour )
    {
	cmap.start = 0;
	cmap.len = COLOURMAP_SIZE;
	cmap.colors = (*dpy_handle).colours_8bit;
	vx_direct_video_color_map_set (NULL, (*dpy_handle).cmap_index, &cmap);
    }
}   /*  End Function vc_store_colours  */

/*PUBLIC_FUNCTION*/
void vc_store_colours_24bit (num_cells, pixel_values, reds, greens, blues,
			     stride, dpy_handle)
/*  This routine will store colours into a low level colourmap. The colours are
    stored for the DirectColour (24 bit) visual type.
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
    unsigned int colour;
    unsigned int count;
    Vx_color_map cmap;
    static char function_name[] = "vc_store_colours_24bit";

    if ( (*dpy_handle).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    for (count = 0; count < num_cells; ++count)
    {
	colour = reds[count * stride] >> 8;
	colour |= greens[count * stride] & 0xff00;
	colour |= ( (blues[count * stride] & 0xff00) << 8 );
	(*dpy_handle).colours_24bit[ pixel_values[count] ] = colour;
    }
    if (!(*dpy_handle).pseudocolour)
    {
	cmap.start = 0;
	cmap.len = COLOURMAP_SIZE;
	cmap.colors = (*dpy_handle).colours_24bit;
	vx_direct_video_color_map_set (NULL, (*dpy_handle).cmap_index, &cmap);
    }
}   /*  End Function vc_store_colours_24bit  */

/*PUBLIC_FUNCTION*/
void vc_get_location (dpy_handle, serv_hostaddr, serv_display_num)
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
    static char function_name[] = "vc_get_location";

    if ( (*dpy_handle).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    *serv_hostaddr = 0;
    *serv_display_num = 0;
}   /*  End Function vc_get_location  */

/*PUBLIC_FUNCTION*/
void vc_set_visual (dpy_handle, pseudo_colour)
/*  This routine will set the current visual of the viewable colourmap.
    The low level display handle must be pointed to by  dpy_handle  .The
    meaning of this value depends on the lower level graphics library used.
    If the value of  pseudo_colour  is TRUE, then the display is set to 8 bit
    PseudoColour, else it is set to 24 bit DirectColour.
    NOTE: the frame buffer contains 32 bits per pixel. When viewing
    PseudoColour, bits 24-31 (big endian) drive the colourmap.
    When viewing DirectColour, bits 0-7, 8-15 and 16-23 drive the red, green
    and blue colourmaps, respectively.
    The routine returns nothing.
*/
Kdisplay dpy_handle;
flag pseudo_colour;
{
    Vx_color_map cmap;
    static char function_name[] = "vc_set_visual";

    if ( (*dpy_handle).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    (*dpy_handle).pseudocolour = pseudo_colour;
    vx_win_mgr_init (NULL, VX_VIDEO_DIRECT);
    vx_direct_video_mode_set (NULL, VX_DISPLAY_VX_ONLY);
    if (pseudo_colour)
    {
	vx_direct_video_key_set (NULL, 0, VX_DISPLAY_CHANNEL, VX_DISPLAY_ALPHA,
				 VX_COLOR_MAP, (*dpy_handle).cmap_index,
				 VX_SHOW_OVERLAY, FALSE,
				 NULL);
	cmap.colors = (*dpy_handle).colours_8bit;
    }
    else
    {
	vx_direct_video_key_set (NULL, 0,
				 VX_DISPLAY_CHANNEL, VX_DISPLAY_TRUE_COLOR,
				 VX_COLOR_MAP, (*dpy_handle).cmap_index,
				 VX_SHOW_OVERLAY, FALSE,
				 NULL);
	cmap.colors = (*dpy_handle).colours_24bit;
    }
    cmap.start = 0;
    cmap.len = COLOURMAP_SIZE;
    vx_direct_video_color_map_set (NULL, (*dpy_handle).cmap_index, &cmap);
}   /*  End Function vc_set_visual  */
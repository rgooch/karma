/*LINTLIBRARY*/
/*  main.c

    This code provides VX colourmap manipulation routines.

    Copyright (C) 1993-1996  Richard Gooch

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

    Updated by      Richard Gooch   8-MAR-1993: Added 24 bit (DirectColour)
  support.

    Updated by      Richard Gooch   22-FEB-1994: Added support for multiple
  visual types.

    Updated by      Richard Gooch   26-FEB-1994: Added  vc_get_visualtype  .

    Updated by      Richard Gooch   22-NOV-1994: Moved typedef of  Kdisplay  to
  header file.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/vc/main.c

    Last updated by Richard Gooch   27-APR-1996: Changed to new documentation
  format.


*/

#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <karma_vc.h>
#include <karma_a.h>
#include <karma_r.h>
#include <karma_m.h>
#include <vx/vx.h>


#define MAGIC_NUMBER (unsigned int) 1598459834
#define COLOURMAP_SIZE 256

struct kdisplay_handle_type
{
    unsigned int magic_number;
    unsigned int cmap_index;
    unsigned int visual;
    flag overlay;
    flag allocated[COLOURMAP_SIZE];
    int colours_8bit[COLOURMAP_SIZE];
    int colours_24bit[COLOURMAP_SIZE];
};

/*PUBLIC_FUNCTION*/
Kdisplay vc_get_dpy_handle ()
/*  [SUMMARY] Generate a display handle for later use.
    [RETURNS] A pointer to the display handle on success, else NULL.
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
unsigned int vc_alloc_colours (unsigned int num_cells,
			       unsigned long *pixel_values,
			       unsigned int min_cells, Kdisplay dpy_handle)
/*  [SUMMARY] Allocate colourcells.
    [PURPOSE] This routine will allocate a number of colourcells in a low level
    colourmap (e.g. using the Xlib routine XAllocColorCells).
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
void vc_free_colours (unsigned int num_cells, unsigned long *pixel_values,
		      Kdisplay dpy_handle)
/*  [SUMMARY] Free a number of colourcells in a low level colourmap.
    <num_cells> The number of colourcells to free.
    <pixel_values> The array of pixel values (colourcells) to free.
    <dpy_handle> The low level display handle. The meaning of this value
    depends on the lower level graphics library used.
    [RETURNS] Nothing.
*/
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
void vc_store_colours (unsigned int num_cells, unsigned long *pixel_values,
		       unsigned short *reds, unsigned short *greens,
		       unsigned short *blues, unsigned int stride,
		       Kdisplay dpy_handle)
/*  [SUMMARY] Store colours into a low level colourmap.
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
    if ( (*dpy_handle).visual != VC_VX_VISUAL_DIRECTCOLOUR )
    {
	cmap.start = 0;
	cmap.len = COLOURMAP_SIZE;
	cmap.colors = (*dpy_handle).colours_8bit;
	vx_direct_video_color_map_set (NULL, (*dpy_handle).cmap_index, &cmap);
    }
}   /*  End Function vc_store_colours  */

/*PUBLIC_FUNCTION*/
void vc_store_colours_24bit (unsigned int num_cells,
			     unsigned long *pixel_values, unsigned short *reds,
			     unsigned short *greens, unsigned short *blues,
			     unsigned int stride, Kdisplay dpy_handle)
/*  [SUMMARY] Store colours into a low level colourmap.
    [PURPOSE] This routine will store colours into a low level colourmap. The
    colours are stored for the DirectColour (24 bit) visual type.
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
    if ( (*dpy_handle).visual == VC_VX_VISUAL_DIRECTCOLOUR )
    {
	cmap.start = 0;
	cmap.len = COLOURMAP_SIZE;
	cmap.colors = (*dpy_handle).colours_24bit;
	vx_direct_video_color_map_set (NULL, (*dpy_handle).cmap_index, &cmap);
    }
}   /*  End Function vc_store_colours_24bit  */

/*PUBLIC_FUNCTION*/
void vc_get_location (Kdisplay dpy_handle, unsigned long *serv_hostaddr,
		      unsigned long *serv_display_num)
/*  [SUMMARY] Determine the location of the graphics display being used.
    <dpy_handle> The low level display handle. The meaning of this value
    depends on the lower level graphics library used.
    <serv_hostaddr> The Internet address of the host on which the display is
    running will be written here.
    <serv_display_num> The number of the display will be written here.
    [RETURNS] Nothing.
*/
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

/*OBSOLETE_FUNCTION*/
void vc_set_visual (Kdisplay dpy_handle, flag pseudo_colour)
/*  This routine will set the current visual of the viewable colourmap. This
    routine is obsoleted by  vc_set_visualtype  .
    If  pseudo_colour  is TRUE, this routine translates to:
        vc_set_visualtype (dpy_handle, VC_VX_VISUAL_PSEUDOCOLOUR3, FALSE);
    else it translates to:
        vc_set_visualtype (dpy_handle, VC_VX_VISUAL_DIRECTCOLOUR, FALSE);
    The routine returns nothing.
*/
{
    unsigned int visual;

    if (pseudo_colour)
    {
	visual = VC_VX_VISUAL_PSEUDOCOLOUR3;
    }
    else
    {
	visual = VC_VX_VISUAL_DIRECTCOLOUR;
    }
    vc_set_visualtype (dpy_handle, visual, FALSE);
}   /*  End Function vc_set_visual  */

/*PUBLIC_FUNCTION*/
void vc_set_visualtype (Kdisplay dpy_handle, unsigned int visual, flag overlay)
/*  [SUMMARY] Set the current visual of the viewable colourmap.
    <dpy_handle> The low level display handle.
    <visual> The visual type for the canvas. Legal values are:
        VC_VX_VISUAL_PSEUDOCOLOUR0    8  bits deep (alpha/overlay channel)
        VC_VX_VISUAL_PSEUDOCOLOUR1    8  bits deep (blue channel)
        VC_VX_VISUAL_PSEUDOCOLOUR2    8  bits deep (green channel)
        VC_VX_VISUAL_PSEUDOCOLOUR3    8  bits deep (red channel)
        VC_VX_VISUAL_DIRECTCOLOUR     24 bits deep
    [NOTE] PseudoColour channels 1, 2 and 3 occupy the same area of screen
    memory as the DirectColour channel. When viewing a PseudoColour canvas,
    the lower appropriate 8 bits are used. When viewing a DirectColour
    canvas, the lower 24 bits (big endian) are used. Bits 0-7, 8-15
    and 16-23 contain the red, green and blue components, respectively.
    <overlay> If TRUE and PseudoColour channel 0 is NOT selected, the overlay
    channel is displayed. A non-zero pixel in the overlay channel will override
    the PseudoColour/DirectColour pixel.
    [RETURNS] Nothing.
*/
{
    int channel;
    Vx_color_map cmap;
    static char function_name[] = "vc_set_visualtype";

    if ( (*dpy_handle).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    switch (visual)
    {
	case VC_VX_VISUAL_PSEUDOCOLOUR0:
	channel = VX_DISPLAY_ALPHA;
	cmap.colors = (*dpy_handle).colours_8bit;
	overlay = FALSE;
	break;
      case VC_VX_VISUAL_PSEUDOCOLOUR1:
	channel = VX_DISPLAY_BLUE;
	cmap.colors = (*dpy_handle).colours_8bit;
	break;
      case VC_VX_VISUAL_PSEUDOCOLOUR2:
	channel = VX_DISPLAY_GREEN;
	cmap.colors = (*dpy_handle).colours_8bit;
	break;
      case VC_VX_VISUAL_PSEUDOCOLOUR3:
	channel = VX_DISPLAY_RED;
	cmap.colors = (*dpy_handle).colours_8bit;
	break;
      case VC_VX_VISUAL_DIRECTCOLOUR:
	channel = VX_DISPLAY_TRUE_COLOR;
	cmap.colors = (*dpy_handle).colours_24bit;
	break;
      default:
	(void) fprintf (stderr, "Illegal visual type: %u\n", visual);
	a_prog_bug (function_name);
	break;
    }
    (*dpy_handle).visual = visual;
    vx_win_mgr_init (NULL, VX_VIDEO_DIRECT);
    vx_direct_video_mode_set (NULL, VX_DISPLAY_VX_ONLY);
    /*  Set the visual  */
    vx_direct_video_key_set (NULL, 0, VX_DISPLAY_CHANNEL, channel,
			     VX_COLOR_MAP, (*dpy_handle).cmap_index,
			     VX_SHOW_OVERLAY, overlay,
			     NULL);
    /*  Set the colourmap  */
    cmap.start = 0;
    cmap.len = COLOURMAP_SIZE;
    vx_direct_video_color_map_set (NULL, (*dpy_handle).cmap_index, &cmap);
    if (overlay)
    {
	/*  Set the overlay colourmap  */
	cmap.colors = (*dpy_handle).colours_8bit;
	vx_direct_video_color_map_set (NULL, 3, &cmap);
	vx_direct_video_overlay_mask_set (NULL, 0xff);
    }
    else
    {
	vx_direct_video_overlay_mask_set (NULL, 0x00);
    }
    (*dpy_handle).overlay = overlay;
}   /*  End Function vc_set_visualtype  */

/*PUBLIC_FUNCTION*/
void vc_get_visualtype (Kdisplay dpy_handle, unsigned int *visual,
			flag *overlay)
/*  [SUMMARY] Get the current visual of the viewable colourmap.
    <dpy_handle> The low level display handle.
    <visual> The visual type for the canvas will be written here.
    <overlay> If the overlay channel is visible, the value TRUE will be written
    here, else the value FALSE will be written here.
    [RETURNS] Nothing.
*/
{
    int channel;
    static char function_name[] = "vc_get_visualtype";

    if ( (*dpy_handle).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid display object\n");
	a_prog_bug (function_name);
    }
    *visual = (*dpy_handle).visual;
    *overlay = (*dpy_handle).overlay;
}   /*  End Function vc_get_visualtype  */

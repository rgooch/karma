/*LINTLIBRARY*/
/*  main.c

    This code provides PostScript output routines.

    Copyright (C) 1994-1996  Richard Gooch

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

/*  This file contains all routines needed for the generation of PostScript.
    This code is based on a few simple PostScript routines obtained from
    Jeanne Joung (jyoung@rp.csiro.au).


    Written by      Richard Gooch   10-MAY-1994

    Updated by      Richard Gooch   20-MAY-1994: Defined  PostScriptPage  and
  the various support routines.

    Updated by      Richard Gooch   22-MAY-1994: Created
  psw_pseudocolour_image  .

    Updated by      Richard Gooch   22-JUL-1994: Added  gsave  command in
  psw_create   to match  grestore  command in  psw_finish  .

    Updated by      Richard Gooch   8-AUG-1994: Changed to  uaddr  type.

    Updated by      Richard Gooch   26-AUG-1994: Moved  typedef  of
  PostScriptPage  class to header.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/psw/main.c

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>

    Updated by      Richard Gooch   15-APR-1995: Added library version number
  printing.

    Updated by      Richard Gooch   7-MAY-1995: Added  #include <karma_ch.h>
  and  #include <karma_a.h>

    Updated by      Richard Gooch   15-JUL-1995: Made linewidth setting code
  more understandable, set linewidth to 0.1 mm and increased floats from
  %6.3f to %7.4f

    Updated by      Richard Gooch   23-SEP-1995: Remember colour between draw
  operations: saves specifying colours every time.

    Updated by      Richard Gooch   26-SEP-1995: Fixed bug in <write_header>
  which printf'ed "0.0 0.0 0.0 setrgbcolor\t%Default Colour": one '%' missing

    Updated by      Richard Gooch   4-JAN-1996: Added comments when writing
  images and partially moved to <ch_printf> routine.

    Updated by      Richard Gooch   13-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   18-APR-1996: Fixed return value bug in
  <psw_rgb_line> and completed move to <ch_printf> routine.

    Updated by      Richard Gooch   19-APR-1996: Created <psw_set_attributes>.

    Updated by      Richard Gooch   1-JUN-1996: Created
  <psw_directcolour_image>.

    Updated by      Richard Gooch   1-JUL-1996: Created <psw_va_create> and
  <psw_close> routines.

    Last updated by Richard Gooch   14-SEP-1996: Fixed offset problem when
  drawing images in landscape mode and image is offset.


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <stdarg.h>
#include <errno.h>
#include <karma.h>
#include <karma_psw.h>
#include <karma_ch.h>
#include <karma_r.h>
#include <karma_m.h>
#include <karma_a.h>


#define LINEWIDTH 0.1  /*  Measured in mm  */
#define COLOUR_QUANTISATION 0.001  /*  Fraction of full-scale colour  */

#if __STDC__ == 1
#  define MAGIC_NUMBER 578942390U
#else
#  define MAGIC_NUMBER (unsigned int) 578942390
#endif

#define VERIFY_PSPAGE(pspage) if (pspage == NULL) \
{(void) fprintf (stderr, "NULL PostScript page passed\n"); \
 a_prog_bug (function_name); } \
if (pspage->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid PostScript page object\n"); \
 a_prog_bug (function_name); }

typedef struct
{
    double red;
    double green;
    double blue;
} PSColour;

/*  Internal definition of PostScriptPage object structure type  */
struct pspage_type
{
    double fsize;
    unsigned int magic_number;
    Channel channel;
    flag portrait;
    flag eps;
    PSColour colour;
};


/*  Private functions  */
STATIC_FUNCTION (flag write_header,
		 (PostScriptPage pspage,
		  double hoffset, double voffset, double hsize,double vsize) );
STATIC_FUNCTION (flag write_mono_line,
		 (Channel channel, CONST unsigned char *line,
		  unsigned int length, CONST uaddr *offsets, uaddr stride,
		  CONST unsigned char imap[256], flag reverse) );
STATIC_FUNCTION (flag set_colour, (PostScriptPage pspage,
				   double red, double green, double blue) );
STATIC_FUNCTION (flag set_linewidth,
		 (PostScriptPage pspage, double linewidth, flag mm) );
STATIC_FUNCTION (flag process_attributes,
		 (PostScriptPage pspage, va_list argp) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
PostScriptPage psw_va_create (Channel channel, double hoffset, double voffset,
			      double hsize, double vsize, flag portrait,
			      flag eps, ...)
/*  [SUMMARY] Create a PostScriptPage object.
    [PURPOSE] This routine will create a PostScriptPage object ready for
    writing PostScript commands to. The routine automatically writes the
    PostScript header.
    <channel> The channel to write to.
    <hoffset> The horizontal offset (in centimeters) of the co-ordinate origin.
    <voffset> The vertical offset (in centimeters) of the co-ordinate origin.
    <hsize> The desired horizontal size of the co-ordinate system (in
    centimeters).
    <vsize> The desired vertical size of the co-ordinate system (in
    centimeters).
    <portrait> If TRUE, objects will be drawn in portrait mode (i.e. the x-axis
    will be horizontal), else they will be drawn in landscape mode (i.e. the
    x-axis will be vertical).
    <eps>  If TRUE the file format will be Encapsulated PostScript, else
    regular PostScript is written.
    [NOTE] All subsequent objects drawn must lie in the range (0.0, 0.0) to
    (1.0, 1.0)
    [VARARGS] The optional list of parameter attribute-key attribute-value
    pairs must follow. This list must be terminated with the value
    PSW_ATT_END. See [<PSW_ATTRIBUTES>] for a list of defined attributes.
    [RETURNS] A PostScriptPage object on success, else NULL
*/
{
    va_list argp;
    PostScriptPage pspage;
    static char function_name[] = "psw_va_create";

    va_start (argp, eps);
    FLAG_VERIFY (portrait);
    if ( ( pspage = (PostScriptPage) m_alloc (sizeof *pspage) ) == NULL )
    {
	m_error_notify (function_name, "PostScriptPage object");
    }
    pspage->channel = channel;
    pspage->fsize = sqrt (hsize * hsize + vsize * vsize) / 1.414213562;
    pspage->portrait = portrait;
    pspage->eps = eps;
    pspage->colour.red = 0.0;
    pspage->colour.green = 0.0;
    pspage->colour.blue = 0.0;
    if ( !write_header (pspage, hoffset, voffset, hsize, vsize) )
    {
	(void) fprintf (stderr, "Error writing PostScript header\n");
	return (NULL);
    }
    pspage->magic_number = MAGIC_NUMBER;
    if ( !ch_puts (channel, "gsave", TRUE) ) return (FALSE);
    if ( !process_attributes (pspage, argp) )
    {
	pspage->magic_number = 0;
	m_free ( (char *) pspage );
	return (NULL);
    }
    va_end (argp);
    return (pspage);
}   /*  End Function psw_va_create  */

/*OBSOLETE_FUNCTION*/
PostScriptPage psw_create (Channel channel, double hoffset, double voffset,
			   double hsize, double vsize, flag portrait)
/*  [SUMMARY] Create a PostScriptPage object.
    [PURPOSE] This routine will create a PostScriptPage object ready for
    writing PostScript commands to. The routine automatically writes the
    PostScript header.
    <channel> The channel to write to.
    <hoffset> The horizontal offset (in centimeters) of the co-ordinate origin.
    <voffset> The vertical offset (in centimeters) of the co-ordinate origin.
    <hsize> The desired horizontal size of the co-ordinate system (in
    centimeters).
    <vsize> The desired vertical size of the co-ordinate system (in
    centimeters).
    <portrait> If TRUE, objects will be drawn in portrait mode (i.e. the x-axis
    will be horizontal), else they will be drawn in landscape mode (i.e. the
    x-axis will be vertical).
    [NOTE] All subsequent objects drawn must lie in the range (0.0, 0.0) to
    (1.0, 1.0)
    [RETURNS] A PostScriptPage object on success, else NULL
*/
{
    static char function_name[] = "psw_create";

    (void) fprintf (stderr,
		    "WARNING: the <%s> routine will be removed in Karma ",
		    function_name);
    (void)fprintf (stderr,
		   "version 2.0\nUse the <psw_va_create> routine instead.\n");
    return ( psw_va_create (channel, hoffset, voffset, hsize, vsize, portrait,
			    FALSE, PSW_ATT_END) );
}   /*  End Function psw_create  */

/*PUBLIC_FUNCTION*/
flag psw_mono_image (PostScriptPage pspage, CONST unsigned char *image,
		     unsigned int xlen, unsigned int ylen,
		     CONST uaddr *xoffsets, CONST uaddr *yoffsets,
		     CONST unsigned char imap[256],
		     double xstart, double ystart, double xend, double yend)
/*  [SUMMARY] Write a monochrome image to a PostScriptPage object.
    <pspage> The PostScriptPage object.
    <image> The image.
    <xlen> The horizontal size of the image (in pixels).
    <ylen> The vertical size of the image (in pixels).
    <xoffsets> The horizontal offset array. If this is NULL, the image data is
    assumed to be contiguous in memory.
    <yoffsets> The vertical offset array. If this is NULL, the image data is
    assumed to be contiguous in memory.
    <imap> The intensity mapping to use. If this is NULL, a linear mapping is
    used (0 = black, 255 = white).
    <xstart> The x starting point (scaled from 0.0 to 1.0).
    <ystart> The y starting point (scaled from 0.0 to 1.0).
    <xend> The x ending point (scaled from 0.0 to 1.0).
    <yend> The y ending point (scaled from 0.0 to 1.0).
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    uaddr voff;
    unsigned int vcount;
    unsigned int hlen, vlen;
    double hos, vos, hss, vss;
    static char function_name[] = "psw_mono_image";

    VERIFY_PSPAGE (pspage);
    channel = pspage->channel;
    if ( !ch_puts (channel, "gsave", TRUE) ) return (FALSE);
    if (pspage->portrait)
    {
	hos = xstart;
	vos = ystart;
	hss = xend - xstart;
	vss = yend - ystart;
	hlen = xlen;
	vlen = ylen;
    }
    else
    {
	hos = ystart;
	vos = 1.0 - xend;
	hss = yend - ystart;
	vss = xend - xstart;
	hlen = ylen;
	vlen = xlen;
    }
    if ( !ch_printf (channel,
		     "%% Greyscale image follows at: %e %e to %e %e\n",
		     xstart, ystart, xend, yend) ) return (FALSE);
    if ( !ch_printf (channel, "%7.4f  %7.4f translate %7.4f  %7.4f scale\n",
		     hos, vos, hss, vss) ) return (FALSE);
    if ( !ch_printf (channel,
		     "/nx %5d def /ny %5d def /nbits %3d def /line %5d string def incimage\n",
		     hlen, vlen, 8, hlen) ) return (FALSE);	
    /*  Write the image  */
    if (pspage->portrait)
    {
	for (vcount = 0; vcount < ylen; ++vcount)
	{
	    voff = (yoffsets == NULL) ? xlen * vcount : yoffsets[vcount];
	    if ( !write_mono_line (channel, image + voff, xlen, xoffsets, 1,
				   imap, FALSE) ) return (FALSE);
	}
    }
    else
    {
	for (vcount = 0; vcount < xlen; ++vcount)
	{
	    voff = (xoffsets ==
		    NULL) ? xlen - vcount - 1 : xoffsets[xlen - vcount - 1];
	    if ( !write_mono_line (channel, image + voff, ylen, yoffsets, xlen,
				   imap, FALSE) ) return (FALSE);
	}
    }
    if ( !ch_puts (channel, "grestore", TRUE) ) return (FALSE);
    return (TRUE);
}   /*  End Function psw_mono_image  */

/*PUBLIC_FUNCTION*/
flag psw_pseudocolour_image (PostScriptPage pspage, CONST unsigned char *image,
			     unsigned int xlen, unsigned int ylen,
			     CONST uaddr *xoffsets, CONST uaddr *yoffsets,
			     CONST unsigned char imap_red[256],
			     CONST unsigned char imap_green[256],
			     CONST unsigned char imap_blue[256],
			     double xstart, double ystart,
			     double xend, double yend)
/*  [SUMMARY] Write a pseudocolour image to a PostScriptPage object.
    <pspage> The PostScriptPage object.
    <image> The image.
    <xlen> The horizontal size of the image (in pixels).
    <ylen> The vertical size of the image (in pixels).
    <xoffsets> The horizontal offset array. If this is NULL, the image data is
    assumed to be contiguous in memory.
    <yoffsets> The vertical offset array. If this is NULL, the image data is
    assumed to be contiguous in memory.
    <imap_red> The red component colourmap entries.
    <imap_green> The green component colourmap entries.
    <imap_blue> The blue component colourmap entries.
    <xstart> The x starting point (scaled from 0.0 to 1.0).
    <ystart> The y starting point (scaled from 0.0 to 1.0).
    <xend> The x ending point (scaled from 0.0 to 1.0).
    <yend> The y ending point (scaled from 0.0 to 1.0).
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    uaddr voff;
    unsigned int vcount;
    unsigned int hlen, vlen;
    double hos, vos, hss, vss;
    static char function_name[] = "psw_pseudocolour_image";

    VERIFY_PSPAGE (pspage);
    if ( (imap_red == NULL) || (imap_green == NULL) || (imap_blue == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    channel = pspage->channel;
    if ( !ch_puts (channel, "gsave", TRUE) ) return (FALSE);
    if (pspage->portrait)
    {
	hos = xstart;
	vos = ystart;
	hss = xend - xstart;
	vss = yend - ystart;
	hlen = xlen;
	vlen = ylen;
    }
    else
    {
	hos = ystart;
	vos = 1.0 - xend;
	hss = yend - ystart;
	vss = xend - xstart;
	hlen = ylen;
	vlen = xlen;
    }
    if ( !ch_printf (channel,
		     "%% PseudoColour image follows at: %e %e to %e %e\n",
		     xstart, ystart, xend, yend) ) return (FALSE);
    if ( !ch_printf (channel, "%7.4f  %7.4f translate %7.4f  %7.4f scale\n",
		     hos, vos, hss, vss) ) return (FALSE);
    if ( !ch_printf (channel,
		     "/nx %5d def /ny %5d def /nbits %3d def /rline %5d string def\n",
		     hlen, vlen, 8, hlen) ) return (FALSE);
    if ( !ch_printf (channel,
		     "/gline %5d string def /bline %5d string def incclrimage\n",
		     hlen, hlen) ) return (FALSE);	
    /*  Write the image  */
    if (pspage->portrait)
    {
	for (vcount = 0; vcount < ylen; ++vcount)
	{
	    voff = (yoffsets == NULL) ? xlen * vcount : yoffsets[vcount];
	    if ( !write_mono_line (channel, image + voff, xlen, xoffsets, 1,
				   imap_red, FALSE) ) return (FALSE);
	    if ( !write_mono_line (channel, image + voff, xlen, xoffsets, 1,
				   imap_green, FALSE) ) return (FALSE);
	    if ( !write_mono_line (channel, image + voff, xlen, xoffsets, 1,
				   imap_blue, FALSE) ) return (FALSE);
	}
    }
    else
    {
	for (vcount = 0; vcount < xlen; ++vcount)
	{
	    voff = (xoffsets ==
		    NULL) ? xlen - vcount - 1 : xoffsets[xlen - vcount - 1];
	    if ( !write_mono_line (channel, image + voff, ylen, yoffsets, xlen,
				   imap_red, FALSE) ) return (FALSE);
	    if ( !write_mono_line (channel, image + voff, ylen, yoffsets, xlen,
				   imap_green, FALSE) ) return (FALSE);
	    if ( !write_mono_line (channel, image + voff, ylen, yoffsets, xlen,
				   imap_blue, FALSE) ) return (FALSE);
	}
    }
    if ( !ch_puts (channel, "grestore", TRUE) ) return (FALSE);
    return (TRUE);
}   /*  End Function psw_pseudocolour_image  */

/*PUBLIC_FUNCTION*/
flag psw_rgb_image (PostScriptPage pspage, CONST unsigned char *image_reds,
		    CONST unsigned char *image_greens,
		    CONST unsigned char *image_blues,
		    unsigned int xlen, unsigned int ylen,
		    CONST uaddr *xoffsets_red, CONST uaddr *yoffsets_red,
		    CONST uaddr *xoffsets_green, CONST uaddr *yoffsets_green,
		    CONST uaddr *xoffsets_blue, CONST uaddr *yoffsets_blue,
		    uaddr stride,
		    double xstart, double ystart, double xend, double yend)
/*  [SUMMARY] Write a truecolour image to a PostScriptPage object.
    <pspage> The PostScriptPage object.
    <image_red> The red component image.
    <image_green> The green component image.
    <image_blue> The blue component image.
    <xlen> The horizontal size of the image (in pixels).
    <ylen> The vertical size of the image (in pixels).
    <xoffsets_red> The horizontal offset array for the red component image. If
    this is NULL, the image data is assumed to be contiguous in memory.
    <yoffsets_red> The vertical offset array for the red component image. If
    this is NULL, the image data is assumed to be contiguous in memory.
    <xoffsets_green> The horizontal offset array for the green component image.
    If this is NULL, the image data is assumed to be contiguous in memory.
    <yoffsets_green> The vertical offset array for the green component image.
    If this is NULL, the image data is assumed to be contiguous in memory.
    <xoffsets_blue> The horizontal offset array for the blue component image.
    If this is NULL, the image data is assumed to be contiguous in memory.
    <yoffsets_blue> The vertical offset array for the blue component image. If
    this is NULL, the image data is assumed to be contiguous in memory.
    <stride> The stride of successive component values. This is ignored if
    both offset arrays are supplied for a component image.
    <xstart> The x starting point (scaled from 0.0 to 1.0).
    <ystart> The y starting point (scaled from 0.0 to 1.0).
    <xend> The x ending point (scaled from 0.0 to 1.0).
    <yend> The y ending point (scaled from 0.0 to 1.0).
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    uaddr voff_red, voff_green, voff_blue;
    unsigned int vcount, tmp;
    unsigned int hlen, vlen;
    double hos, vos, hss, vss;
    static char function_name[] = "psw_rgb_image";

    VERIFY_PSPAGE (pspage);
    channel = pspage->channel;
    if ( !ch_puts (channel, "gsave", TRUE) ) return (FALSE);
    if (pspage->portrait)
    {
	hos = xstart;
	vos = ystart;
	hss = xend - xstart;
	vss = yend - ystart;
	hlen = xlen;
	vlen = ylen;
    }
    else
    {
	hos = ystart;
	vos = 1.0 - xend;
	hss = yend - ystart;
	vss = xend - xstart;
	hlen = ylen;
	vlen = xlen;
    }
    if ( !ch_printf (channel,
		     "%% TrueColour image follows at: %e %e to %e %e\n",
		     xstart, ystart, xend, yend) ) return (FALSE);
    if ( !ch_printf (channel, "%7.4f  %7.4f translate %7.4f  %7.4f scale\n",
		     hos, vos, hss, vss) ) return (FALSE);
    if ( !ch_printf (channel,
		     "/nx %5d def /ny %5d def /nbits %3d def /rline %5d string def\n",
		     hlen, vlen, 8, hlen) ) return (FALSE);
    if ( !ch_printf (channel,
		     "/gline %5d string def /bline %5d string def incclrimage\n",
		     hlen, hlen) ) return (FALSE);
    /*  Write the image  */
    if (pspage->portrait)
    {
	for (vcount = 0; vcount < ylen; ++vcount)
	{
	    tmp = xlen * vcount * stride;
	    voff_red = (yoffsets_red == NULL) ? tmp : yoffsets_red[vcount];
	    voff_green = (yoffsets_green==NULL) ? tmp : yoffsets_green[vcount];
	    voff_blue = (yoffsets_blue == NULL) ? tmp : yoffsets_blue[vcount];
	    if ( !write_mono_line (channel, image_reds + voff_red, xlen,
				   xoffsets_red,
				   stride, NULL, FALSE) ) return (FALSE);
	    if ( !write_mono_line (channel, image_greens + voff_green, xlen,
				   xoffsets_green,
				   stride, NULL, FALSE) ) return (FALSE);
	    if ( !write_mono_line (channel, image_blues + voff_blue, xlen,
				   xoffsets_blue,
				   stride, NULL, FALSE) ) return (FALSE);
	}
    }
    else
    {
	for (vcount = 0; vcount < xlen; ++vcount)
	{
	    tmp = stride * (xlen - vcount - 1);
	    voff_red = (xoffsets_red ==
			NULL) ? tmp : xoffsets_red[xlen - vcount - 1];
	    voff_green = (xoffsets_green ==
			  NULL) ? tmp : xoffsets_green[xlen - vcount - 1];
	    voff_blue = (xoffsets_blue ==
			 NULL) ? tmp : xoffsets_blue[xlen - vcount - 1];
	    if ( !write_mono_line (channel, image_reds + voff_red, ylen,
				   yoffsets_red,
				   xlen * stride, NULL, FALSE) ) return(FALSE);
	    if ( !write_mono_line (channel, image_greens + voff_green, ylen,
				   yoffsets_green,
				   xlen * stride, NULL, FALSE) ) return(FALSE);
	    if ( !write_mono_line (channel, image_blues + voff_blue, ylen,
				   yoffsets_blue,
				   xlen * stride, NULL, FALSE) ) return(FALSE);
	}
    }
    if ( !ch_puts (channel, "grestore", TRUE) ) return (FALSE);
    return (TRUE);
}   /*  End Function psw_rgb_image  */

/*PUBLIC_FUNCTION*/
flag psw_directcolour_image (PostScriptPage pspage,
			     CONST unsigned char *image_reds,
			     CONST unsigned char *image_greens,
			     CONST unsigned char *image_blues,
			     unsigned int xlen, unsigned int ylen,
			     CONST uaddr *xoffsets_red,
			     CONST uaddr *yoffsets_red,
			     CONST uaddr *xoffsets_green,
			     CONST uaddr *yoffsets_green,
			     CONST uaddr *xoffsets_blue,
			     CONST uaddr *yoffsets_blue,
			     uaddr stride,
			     CONST unsigned char imap_red[256],
			     CONST unsigned char imap_green[256],
			     CONST unsigned char imap_blue[256],
			     double xstart, double ystart,
			     double xend, double yend)
/*  [SUMMARY] Write a directcolour image to a PostScriptPage object.
    <pspage> The PostScriptPage object.
    <image_red> The red component image.
    <image_green> The green component image.
    <image_blue> The blue component image.
    <xlen> The horizontal size of the image (in pixels).
    <ylen> The vertical size of the image (in pixels).
    <xoffsets_red> The horizontal offset array for the red component image. If
    this is NULL, the image data is assumed to be contiguous in memory.
    <yoffsets_red> The vertical offset array for the red component image. If
    this is NULL, the image data is assumed to be contiguous in memory.
    <xoffsets_green> The horizontal offset array for the green component image.
    If this is NULL, the image data is assumed to be contiguous in memory.
    <yoffsets_green> The vertical offset array for the green component image.
    If this is NULL, the image data is assumed to be contiguous in memory.
    <xoffsets_blue> The horizontal offset array for the blue component image.
    If this is NULL, the image data is assumed to be contiguous in memory.
    <yoffsets_blue> The vertical offset array for the blue component image. If
    this is NULL, the image data is assumed to be contiguous in memory.
    <stride> The stride of successive component values. This is ignored if
    both offset arrays are supplied for a component image.
    <imap_red> The red component colourmap entries.
    <imap_green> The green component colourmap entries.
    <imap_blue> The blue component colourmap entries.
    <xstart> The x starting point (scaled from 0.0 to 1.0).
    <ystart> The y starting point (scaled from 0.0 to 1.0).
    <xend> The x ending point (scaled from 0.0 to 1.0).
    <yend> The y ending point (scaled from 0.0 to 1.0).
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    uaddr voff_red, voff_green, voff_blue;
    unsigned int vcount, tmp;
    unsigned int hlen, vlen;
    double hos, vos, hss, vss;
    static char function_name[] = "psw_directcolour_image";

    VERIFY_PSPAGE (pspage);
    channel = pspage->channel;
    if ( !ch_puts (channel, "gsave", TRUE) ) return (FALSE);
    if (pspage->portrait)
    {
	hos = xstart;
	vos = ystart;
	hss = xend - xstart;
	vss = yend - ystart;
	hlen = xlen;
	vlen = ylen;
    }
    else
    {
	hos = ystart;
	vos = 1.0 - xend;
	hss = yend - ystart;
	vss = xend - xstart;
	hlen = ylen;
	vlen = xlen;
    }
    if ( !ch_printf (channel,
		     "%% DirectColour image follows at: %e %e to %e %e\n",
		     xstart, ystart, xend, yend) ) return (FALSE);
    if ( !ch_printf (channel, "%7.4f  %7.4f translate %7.4f  %7.4f scale\n",
		     hos, vos, hss, vss) ) return (FALSE);
    if ( !ch_printf (channel,
		     "/nx %5d def /ny %5d def /nbits %3d def /rline %5d string def\n",
		     hlen, vlen, 8, hlen) ) return (FALSE);
    if ( !ch_printf (channel,
		     "/gline %5d string def /bline %5d string def incclrimage\n",
		     hlen, hlen) ) return (FALSE);
    /*  Write the image  */
    if (pspage->portrait)
    {
	for (vcount = 0; vcount < ylen; ++vcount)
	{
	    tmp = xlen * vcount * stride;
	    voff_red = (yoffsets_red == NULL) ? tmp : yoffsets_red[vcount];
	    voff_green = (yoffsets_green==NULL) ? tmp : yoffsets_green[vcount];
	    voff_blue = (yoffsets_blue == NULL) ? tmp : yoffsets_blue[vcount];
	    if ( !write_mono_line (channel, image_reds + voff_red, xlen,
				   xoffsets_red,
				   stride, imap_red, FALSE) ) return (FALSE);
	    if ( !write_mono_line (channel, image_greens + voff_green, xlen,
				   xoffsets_green,
				   stride, imap_green, FALSE) ) return (FALSE);
	    if ( !write_mono_line (channel, image_blues + voff_blue, xlen,
				   xoffsets_blue,
				   stride, imap_blue, FALSE) ) return (FALSE);
	}
    }
    else
    {
	for (vcount = 0; vcount < xlen; ++vcount)
	{
	    tmp = stride * (xlen - vcount - 1);
	    voff_red = (xoffsets_red ==
			NULL) ? tmp : xoffsets_red[xlen - vcount - 1];
	    voff_green = (xoffsets_green ==
			  NULL) ? tmp : xoffsets_green[xlen - vcount - 1];
	    voff_blue = (xoffsets_blue ==
			 NULL) ? tmp : xoffsets_blue[xlen - vcount - 1];
	    if ( !write_mono_line (channel, image_reds + voff_red, ylen,
				   yoffsets_red,
				   xlen * stride, imap_red,
				   FALSE) ) return(FALSE);
	    if ( !write_mono_line (channel, image_greens + voff_green, ylen,
				   yoffsets_green,
				   xlen * stride, imap_green,
				   FALSE) ) return(FALSE);
	    if ( !write_mono_line (channel, image_blues + voff_blue, ylen,
				   yoffsets_blue,
				   xlen * stride, imap_blue,
				   FALSE) ) return(FALSE);
	}
    }
    if ( !ch_puts (channel, "grestore", TRUE) ) return (FALSE);
    return (TRUE);
}   /*  End Function psw_directcolour_image  */

/*PUBLIC_FUNCTION*/
flag psw_close (PostScriptPage pspage, flag flush, flag close)
/*  [SUMMARY] Close a PostScriptPage object.
    [PURPOSE] This routine will write a tail to a PostScriptPage object and
    will deallocate the object.
    <pspage> The PostScriptPage object.
    <flush> If TRUE, the underlying channel obect is flushed.
    <close> If TRUE, the underlying channel obect is closed.
    [RETURNS] TRUE on succes, else FALSE. In either case, the PostScriptPage
    object is deallocated (and closed if <<close>> is TRUE).
*/
{
    Channel channel;
    static char function_name[] = "psw_close";

    VERIFY_PSPAGE (pspage);
    channel = pspage->channel;
    pspage->magic_number = 0;
    m_free ( (char *) pspage );
    if ( !ch_puts (channel, "grestore", TRUE) )
    {
	if (close) (void) ch_close (channel);
	return (FALSE);
    }
    if (!pspage->eps)
    {
	if ( !ch_puts (channel, "showpage", TRUE) )
	{
	    if (close) (void) ch_close (channel);
	    return (FALSE);
	}
    }
    if (close) return ( ch_close (channel) );
    if (flush) return ( ch_flush (channel) );
    return (TRUE);
}   /*  End Function psw_close  */

/*PUBLIC_FUNCTION*/
flag psw_finish (PostScriptPage pspage, flag eps, flag flush, flag close)
/*  [SUMMARY] Close a PostScriptPage object.
    [PURPOSE] This routine will write a tail to a PostScriptPage object and
    will deallocate the object.
    <pspage> The PostScriptPage object.
    <eps> If TRUE the file format should be Encapsulated PostScript, else
    regular PostScript is written.
    <flush> If TRUE, the underlying channel obect is flushed.
    <close> If TRUE, the underlying channel obect is closed.
    [RETURNS] TRUE on succes, else FALSE. In either case, the PostScriptPage
    object is deallocated (and closed if <<close>> is TRUE).
*/
{
    Channel channel;
    static char function_name[] = "psw_finish";

    VERIFY_PSPAGE (pspage);
    FLAG_VERIFY (eps);
    (void) fprintf (stderr,
		    "WARNING: the <%s> routine will be removed in Karma ",
		    function_name);
    (void)fprintf (stderr,
		   "version 2.0\nUse the <psw_close> routine instead.\n");
    channel = pspage->channel;
    pspage->magic_number = 0;
    m_free ( (char *) pspage );
    if ( !ch_puts (channel, "grestore", TRUE) )
    {
	if (close) (void) ch_close (channel);
	return (FALSE);
    }
    if (!eps)
    {
	if ( !ch_puts (channel, "showpage", TRUE) )
	{
	    if (close) (void) ch_close (channel);
	    return (FALSE);
	}
    }
    if (close) return ( ch_close (channel) );
    if (flush) return ( ch_flush (channel) );
    return (TRUE);
}   /*  End Function psw_finish  */

/*PUBLIC_FUNCTION*/
flag psw_rgb_line (PostScriptPage pspage, double red, double green,double blue,
		   double xstart, double ystart, double xend, double yend)
/*  [SUMMARY] Write a colour line to a PostScriptPage object.
    <pspage> The PostScriptPage object.
    <red> The red component value. The range is: 0.0 (dark) to 1.0 (light)
    <green> The green component value. The range is: 0.0 (dark) to 1.0 (light)
    <blue> The blue component value. The range is: 0.0 (dark) to 1.0 (light)
    <xstart> The x starting point (scaled from 0.0 to 1.0).
    <ystart> The y starting point (scaled from 0.0 to 1.0).
    <xend> The x ending point (scaled from 0.0 to 1.0).
    <yend> The y ending point (scaled from 0.0 to 1.0).
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    static char function_name[] = "psw_rgb_line";

    VERIFY_PSPAGE (pspage);
    channel = pspage->channel;
    if ( !set_colour (pspage, red, green, blue) ) return (FALSE);
    if (pspage->portrait)
    {
	return ( ch_printf (channel, "%7.4f  %7.4f M %7.4f  %7.4f D str\n",
			    xstart, ystart, xend, yend) );
    }
    return ( ch_printf (channel, "%7.4f  %7.4f M %7.4f  %7.4f D str\n",
			ystart, 1.0 - xstart, yend, 1.0 - xend) );
}   /*  End Function psw_rgb_line  */

/*PUBLIC_FUNCTION*/
flag psw_rgb_polygon (PostScriptPage pspage,
		      double red, double green, double blue,
		      CONST double *x_arr, CONST double *y_arr,
		      unsigned int num_points, flag fill)
/*  [SUMMARY] Write a colour polygon to a PostScriptPage object.
    <pspage> The PostScriptPage object.
    <red> The red component value. The range is: 0.0 (dark) to 1.0 (light)
    <green> The green component value. The range is: 0.0 (dark) to 1.0 (light)
    <blue> The blue component value. The range is: 0.0 (dark) to 1.0 (light)
    <x_arr> The array of x vertices. This is scaled from 0.0 to 1.0
    <y_arr> The array of y vertices. This is scaled from 0.0 to 1.0
    <num_points> The number of vertices.
    <fill> If TRUE, the polygon is filled.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    unsigned int count;
    static char function_name[] = "psw_rgb_polygon";

    VERIFY_PSPAGE (pspage);
    if ( (x_arr == NULL) || (y_arr == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    FLAG_VERIFY (fill);
    if (num_points < 2) return (TRUE);
    channel = pspage->channel;
    if ( !set_colour (pspage, red, green, blue) ) return (FALSE);
    if (pspage->portrait)
    {
	if ( !ch_printf (channel, "%7.4f  %7.4f M\n",
			 x_arr[0], y_arr[0]) ) return (FALSE);
    }
    else
    {
	if ( !ch_printf (channel, "%7.4f  %7.4f M\n",
			 y_arr[0], 1.0 -x_arr[0]) ) return (FALSE);
    }
    for (count = 1; count < num_points; ++count)
    {
	if (pspage->portrait)
	{
	    if ( !ch_printf (channel, "%7.4f  %7.4f D\n",
			     x_arr[count], y_arr[count]) ) return (FALSE);
	}
	else
	{
	    if ( !ch_printf (channel, "%7.4f  %7.4f D\n",
			     y_arr[count], 1.0 -x_arr[count]) ) return (FALSE);
	    
	}
    }
    if (fill) return ( ch_puts (channel, "  closepath  fill", TRUE) );
    return ( ch_puts (channel, "  closepath  stroke", TRUE) );
}   /*  End Function psw_rgb_polygon  */

/*PUBLIC_FUNCTION*/
flag psw_rgb_ellipse (PostScriptPage pspage,
		      double red, double green, double blue,
		      double cx, double cy, double rx, double ry, flag fill)
/*  [SUMMARY] Write a colour ellipse to a PostScriptPage object.
    <pspage> The PostScriptPage object.
    <red> The red component value. The range is: 0.0 (dark) to 1.0 (light)
    <green> The green component value. The range is: 0.0 (dark) to 1.0 (light)
    <blue> The blue component value. The range is: 0.0 (dark) to 1.0 (light)
    <cx> The x centre. This is scaled from 0.0 to 1.0
    <cy> The y centre. This is scaled from 0.0 to 1.0
    <rx> The x radius. This is scaled from 0.0 to 1.0
    <ry> The y radius. This is scaled from 0.0 to 1.0
    <fill> If TRUE, the ellipse is filled.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    double ch, cv, rh, rv, vscale;
    static char function_name[] = "psw_rgb_ellipse";

    VERIFY_PSPAGE (pspage);
    FLAG_VERIFY (fill);
    channel = pspage->channel;
    if ( !set_colour (pspage, red, green, blue) ) return (FALSE);
    if ( !ch_puts (channel, "gsave", TRUE) ) return (FALSE);
    if (pspage->portrait)
    {
	ch = cx;
	cv = cy;
	rh = rx;
	rv = ry;
    }
    else
    {
	ch = cy;
	cv = 1.0 - cx;
	rh = ry;
	rv = rx;
    }
    /*  Fiddle scale to make ellipse  */
    vscale = rv / rh;
    if ( !ch_printf (channel,
		     "newpath  1.0 %7.4f scale  %7.4f %7.4f %7.4f 0 360 arc closepath %s\n",
		     vscale, ch, (cv) / vscale, rh,
		     fill ? "fill" : "stroke") ) return (FALSE);
    return ( ch_puts (channel, "grestore", TRUE) );
}   /*  End Function psw_rgb_ellipse  */

/*PUBLIC_FUNCTION*/
flag psw_rgb_text (PostScriptPage pspage, double red, double green,double blue,
		   CONST char *string, CONST char *fontname,
		   unsigned int fontsize,
		   double xstart, double ystart, double angle)
/*  [SUMMARY] Write a colour string to a PostScriptPage object.
    <pspage> The PostScriptPage object.
    <red> The red component value. The range is: 0.0 (dark) to 1.0 (light)
    <green> The green component value. The range is: 0.0 (dark) to 1.0 (light)
    <blue> The blue component value. The range is: 0.0 (dark) to 1.0 (light)
    <string> The string.
    <fontname> The fontname to use. If this is NULL, the default font will be
    used.
    <fontsize> The size of the font (in millimeters).
    <xstart> The x starting point (scaled from 0.0 to 1.0).
    <ystart> The y starting point (scaled from 0.0 to 1.0).
    <angle> The angle to rotate the text through (in degrees). Positive angles
    rotate counter-clockwise from horizontal.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    static char function_name[] = "psw_rgb_text";

    VERIFY_PSPAGE (pspage);
    if ( (string == NULL) || (fontname == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    channel = pspage->channel;
    if ( !set_colour (pspage, red, green, blue) ) return (FALSE);
    if ( !ch_puts (channel, "gsave", TRUE) ) return (FALSE);
    if ( !ch_printf (channel, "/%s findfont\n", fontname) ) return (FALSE);
    if ( !ch_printf (channel, "%7.4f scalefont  setfont\n",
		     (double) fontsize / 10.0 /pspage->fsize) ) return (FALSE);
    if (pspage->portrait)
    {
	if ( !ch_printf (channel, "%7.4f  %7.4f  moveto\n",
			 xstart, ystart) ) return (FALSE);
    }
    else
    {
	if ( !ch_printf (channel, "%7.4f  %7.4f  moveto\n",
			 ystart, 1.0 - xstart) ) return (FALSE);
    }
    if (pspage->portrait)
    {
	if ( !ch_printf (channel, "%7.4f rotate\n", angle) ) return (FALSE);
    }
    else
    {
	if ( !ch_printf (channel, "%7.4f rotate\n",
			 angle + 90.0) ) return (FALSE);
    }
    if ( !ch_printf (channel, "(%s)  show\n", string) ) return (FALSE);
    return ( ch_puts (channel, "grestore", TRUE) );
}   /*  End Function psw_rgb_text  */

/*PUBLIC_FUNCTION*/
flag psw_set_attributes (PostScriptPage pspage, ...)
/*  [SUMMARY] Set the attributes for a PostScriptPage object.
    <pspage> The PostScriptPage object.
    [VARARGS] The optional list of parameter attribute-key attribute-value
    pairs must follow. This list must be terminated with the value
    PSW_ATT_END. See [<PSW_ATTRIBUTES>] for the list of attributes.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    static char function_name[] = "psw_set_attributes";

    va_start (argp, pspage);
    VERIFY_PSPAGE (pspage);
    if ( !process_attributes (pspage, argp) ) return (FALSE);
    va_end (argp);
    return (TRUE);
}   /*  End Function psw_set_attributes  */


/*  Private functions follow  */

static flag write_header (PostScriptPage pspage,
			  double hoffset, double voffset,
			  double hsize, double vsize)
/*  This routine will write a PostScript header required prior to writing an
    image.
    <pspage> The PostScriptPage object.
    The horizontal offset of the origin (in centimeters) must be given by
    hoffset  .
    The vertical offset of the origin (in centimeters) must be given by
    voffset  .
    The horizontal size of the bounding box (in centimeters) must be given by
    hsize  .
    The vertical size of the bounding box (in centimeters) must be given by
    vsize  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    Channel channel = pspage->channel;
    int lx0, lx1, ly0, ly1;
    time_t clock;
    struct timeval date;
    double cm_to_points;
    struct tm *ltime;
    char txt[STRING_LENGTH];
    char hostname[STRING_LENGTH];
    extern char module_name[STRING_LENGTH + 1];
    extern char module_version_date[STRING_LENGTH + 1];
    extern char module_lib_version[STRING_LENGTH + 1];
    extern char karma_library_version[STRING_LENGTH + 1];
    extern char *sys_errlist[];
    static char function_name[] = "write_header";

    /*  Put the bounding box in pts.  */
    cm_to_points = 72.0 / 2.54;
    lx0 = (int) (cm_to_points * hoffset + 0.5);
    ly0 = (int) (cm_to_points * voffset + 0.5);
    lx1 = (int) (cm_to_points * (hoffset + hsize) + 0.5);
    ly1 = (int) (cm_to_points * (voffset + vsize) + 0.5);

    if (pspage->eps)
    {
	if ( !ch_puts (channel, "%!PS-Adobe-2.0 EPSF-2.0", TRUE) )
	    return (FALSE);
    }
    else
    {
	if ( !ch_puts (channel, "%!PS", TRUE) ) return (FALSE);
    }
    if ( !ch_puts (channel, "%%Title: ", TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "%%Creator: ", FALSE) ) return (FALSE);
    if (strcmp (module_name, "<<Unknown>>") == 0)
    {
	if ( !ch_puts (channel, "Karma  psw_  package", TRUE) ) return (FALSE);
    }
    else
    {
	if (strcmp (module_version_date, "Unknown") == 0)
	{
	    if ( !ch_printf (channel,
			     "module: \"%s\" using Karma <psw_> package\n",
			     module_name) ) return (FALSE);
	}
	else
	{
	    if ( !ch_printf (channel,
			     "module: \"%s\" version: \"%s\" using Karma  psw_  package\n",
			     module_name,module_version_date) ) return (FALSE);
	}
    }
    if ( !ch_printf (channel,
		     "%%%%Karma library version: %s\n%%%%Module compiled with library version: %s\n",
		     karma_library_version, module_lib_version) ) return FALSE;
    if ( !ch_puts (channel, "%%CreationDate: ", FALSE) ) return (FALSE);
    if (gettimeofday (&date, (struct timezone *) NULL) != 0)
    {
	(void) fprintf (stderr, "Error getting system time\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    clock = date.tv_sec;
    ltime = localtime (&clock);
    if (strftime (txt, STRING_LENGTH, "%r  %a %d %h %Y  %Z", ltime) == 0)
    {
	(void) fprintf (stderr, "Buffer too small for time string\n");
	a_prog_bug (function_name);
    }
    if ( !ch_puts (channel, txt, TRUE) ) return (FALSE);
    r_gethostname (hostname, STRING_LENGTH);
    if ( !ch_printf (channel, "%%%%For: %s@%s\n", r_getenv ("USER"),
		     hostname) ) return (FALSE);
    if ( !ch_puts (channel, "%%Pages: 0", TRUE) ) return (FALSE);
    if ( !ch_printf (channel, "%%%%BoundingBox: %d %d %d %d\n",
		     lx0, ly0, lx1, ly1) ) return (FALSE);
    if ( !ch_puts (channel, "%%EndComments", TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "%", TRUE) ) return (FALSE);
    if ( !set_linewidth (pspage, LINEWIDTH, TRUE) ) return (FALSE);
    if ( !ch_puts (channel,"1 setlinejoin 1 setlinecap",TRUE) ) return (FALSE);
    /*  Compute translation and scale such that (0.0, 0.0) is the origin and
	(1.0, 1.0) is the far end of the region  */
    if ( !ch_printf (channel, "%7.4f %7.4f translate  %7.4f %7.4f scale\n",
		     hoffset * cm_to_points, voffset * cm_to_points,
		     hsize * cm_to_points,
		     vsize * cm_to_points) ) return (FALSE);
    /*  Set up definitions.  */
    if ( !ch_puts (channel, "/M {moveto} def /D {lineto} def ",
		   TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/m {rmoveto} def /d {rlineto} def",
		   TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/r {rotate} def /solid {[]0 setdash} def",
		   TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/sp {currentpoint /y exch def /x exch def} def",
		   TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/rp {x y M} def", TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/str {sp stroke rp} def  /dot { 0 0 d} def",
		   TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/cfont /Courier def ",TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/sfont /Symbol def", TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/CF {cfont findfont} def ",TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/SF {sfont findfont} def ",TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/HF {/Helvetica findfont} def ",
		   TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/HBF {/Helvetica-bold findfont} def ",
		   TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/TF {/Times-Roman findfont} def ",
		   TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/TBF {/Times-Bold findfont} def ",
		   TRUE) ) return (FALSE);
    if ( !ch_puts (channel, "/SS {scalefont setfont } def ",
		   TRUE) ) return (FALSE);

    if ( !ch_puts (channel,
		   "/incimage {nx ny nbits [nx 0 0 ny 0 0] {currentfile line readhexstring pop} image} def ",
		   TRUE) ) return (FALSE);
    if ( !ch_puts (channel,
		   "/incclrimage {nx ny nbits [nx 0 0 ny 0 0] {currentfile rline readhexstring pop}  {currentfile gline readhexstring pop}  {currentfile bline readhexstring pop}  true 3 colorimage} def ",
		   TRUE) ) return (FALSE);
    return ( ch_puts (channel, "0.0 0.0 0.0  setrgbcolor\t%Default Colour",
		      TRUE) );
}   /*  End Function write_header  */

static flag write_mono_line (Channel channel, CONST unsigned char *line,
			     unsigned int length, CONST uaddr *offsets,
			     uaddr stride, CONST unsigned char imap[256],
			     flag reverse)
/*  This routine will write a single line.
*/
{
    uaddr hoffset;
    unsigned int hcount;
    int byte, nibble;
    char buf[2];
    static char function_name[] = "write_mono_line";

    FLAG_VERIFY (reverse);
    for (hcount = 0; hcount < length; ++hcount)
    {
	hoffset = reverse ? (length - hcount - 1) : hcount;
	if (offsets == NULL)
	{
	    hoffset *= stride;
	}
	else
	{
	    hoffset = offsets[hoffset];
	}
	byte = (imap == NULL) ? line[hoffset] : imap[ line[hoffset] ];
	nibble = byte >> 4;
	buf[0] = (nibble < 0x0a) ? nibble + '0' : nibble + 'a' - 0x0a;
	nibble = byte & 0x0f;
	buf[1] = (nibble < 0x0a) ? nibble + '0' : nibble + 'a' - 0x0a;
	if (ch_write (channel, buf, 2) < 2) return (FALSE);
    }
    return ( ch_puts (channel, "", TRUE) );
}   /*  End Function write_mono_line  */

static flag set_colour (PostScriptPage pspage,
			double red, double green, double blue)
/*  [SUMMARY] Set the drawing colour for a PostScript page.
    <pspage> The PostScript page.
    <red> The red RGB component.
    <green> The green RGB component.
    <blue> The blue RGB component.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    if ( (fabs (red - pspage->colour.red) < COLOUR_QUANTISATION ) &&
	(fabs (green - pspage->colour.green) < COLOUR_QUANTISATION ) &&
	(fabs (blue - pspage->colour.blue) < COLOUR_QUANTISATION ) )
    {
	return (TRUE);
    }
    if ( !ch_printf (pspage->channel, "%7.4f  %7.4f  %7.4f  setrgbcolor\n",
		     red, green, blue) ) return (FALSE);
    pspage->colour.red = red;
    pspage->colour.green = green;
    pspage->colour.blue = blue;
    return (TRUE);
}   /*  End Function set_colour  */

static flag set_linewidth (PostScriptPage pspage, double linewidth, flag mm)
/*  [SUMMARY] Set the linewidth for a PostScriptPage object.
    <pspage> The PostScriptPage object.
    <linewidth> The linewidth.
    <mm> If TRUE, the linewidth is in units of millimeters, else the units are
    fractions of the page size.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    double linewidth_scale, linewidth_mm;

    if (linewidth == 0.0)
    {
	linewidth = LINEWIDTH;
	mm = TRUE;
    }
    if (mm)
    {
	linewidth_mm = linewidth;
	linewidth_scale = linewidth / pspage->fsize / 10.0;
    }
    else
    {
	linewidth_mm = linewidth * pspage->fsize * 10.0;
	linewidth_scale = linewidth;
    }
    return ( ch_printf (pspage->channel, "%e setlinewidth %% %7.4f mm\n",
			linewidth_scale, linewidth_mm) );
}   /*  End Function set_linewidth  */

static flag process_attributes (PostScriptPage pspage, va_list argp)
/*  [SUMMARY] Set the attributes for a PostScriptPage object.
    <pspage> The PostScriptPage object.
    <argp> The optional list of parameter attribute-key attribute-value
    pairs.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    double d_val;
    unsigned int att_key;
    static char function_name[] = "__psw_process_attributes";

    while ( ( att_key = va_arg (argp, unsigned int) ) != PSW_ATT_END )
    {
	switch (att_key)
	{
	  case PSW_ATT_LINEWIDTH_MM:
	  case PSW_ATT_LINEWIDTH_RELATIVE:
	    d_val = va_arg (argp, double);
	    if (d_val < 0.0)
	    {
		(void) fprintf (stderr, "linewidth value: %e less than zero\n",
				d_val);
		a_prog_bug (function_name);
	    }
	    if ( !set_linewidth (pspage, d_val,
				 (att_key ==
				  PSW_ATT_LINEWIDTH_MM) ? TRUE : FALSE) )
	    {
		return (FALSE);
	    }
	    break;
	  default:
	    (void) fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    return (TRUE);
}   /*  End Function process_attributes  */

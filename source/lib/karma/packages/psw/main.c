/*LINTLIBRARY*/
/*  main.c

    This code provides PostScript output routines.

    Copyright (C) 1994  Richard Gooch

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

    Last updated by Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <sys/time.h>
#include <time.h>
#include <errno.h>
#include <karma.h>
#include <karma_psw.h>
#include <karma_r.h>
#include <karma_m.h>


#if __STDC__ == 1
#  define MAGIC_NUMBER 578942390U
#else
#  define MAGIC_NUMBER (unsigned int) 578942390
#endif

#define VERIFY_PSPAGE(pspage) if (pspage == NULL) \
{(void) fprintf (stderr, "NULL PostScript page passed\n"); \
 a_prog_bug (function_name); } \
if ( (*pspage).magic_number != MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid PostScript page object\n"); \
 a_prog_bug (function_name); }

/*  Internal definition of PostScriptPage object structure type  */
struct pspage_type
{
    double fsize;
    unsigned int magic_number;
    Channel channel;
    flag portrait;
};


/*  Private functions  */
STATIC_FUNCTION (flag write_header,
		 (Channel channel,
		  double hoffset, double voffset, double hsize,double vsize) );
STATIC_FUNCTION (flag write_mono_line,
		 (Channel channel, CONST unsigned char *line,
		  unsigned int length, CONST uaddr *offsets, uaddr stride,
		  CONST unsigned char imap[256], flag reverse) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
PostScriptPage psw_create (Channel channel, double hoffset, double voffset,
			   double hsize, double vsize, flag portrait)
/*  This routine will create a PostScriptPage object ready for writing
    PostScript commands to. The routine automatically writes the PostScript
    header.
    The channel to write to must be given by  channel  .
    The horizontal offset (in centimeters) of the co-ordinate origin must be
    given by  hoffset  .
    The vertical offset (in centimeters) of the co-ordinate origin must be
    given by  voffset  .
    The desired horizontal size of the co-ordinate system (in centimeters) must
    be given by  hsize  .
    The desired vertical size of the co-ordinate system (in centimeters) must
    be given by  vsize  .
    If the value of  portrait  is TRUE, objects will be drawn in portrait
    mode (ie. the x-axis will be horizontal), else they will be drawn in
    landscape mode (ie. the x-axis will be vertical).
    NOTE: all subsequent objects drawn must lie in the range (0.0, 0.0) to
    (1.0, 1.0)
    The routine returns a PostScriptPage object on success,
    else it returns NULL
*/
{
    PostScriptPage pspage;
    static char function_name[] = "psw_create";

    FLAG_VERIFY (portrait);
    if (write_header (channel, hoffset, voffset, hsize, vsize) != TRUE)
    {
	(void) fprintf (stderr, "Error writing PostScript header\n");
	return (NULL);
    }
    if ( ( pspage = (PostScriptPage) m_alloc (sizeof *pspage) ) == NULL )
    {
	m_error_notify (function_name, "PostScriptPage object");
    }
    (*pspage).magic_number = MAGIC_NUMBER;
    (*pspage).fsize = sqrt (hsize * hsize + vsize * vsize) / 1.41;
    (*pspage).channel = channel;
    (*pspage).portrait = portrait;
    if (ch_puts (channel, "gsave", TRUE) != TRUE) return (FALSE);
    return (pspage);
}   /*  End Function psw_create  */

/*PUBLIC_FUNCTION*/
flag psw_mono_image (PostScriptPage pspage, CONST unsigned char *image,
		     unsigned int xlen, unsigned int ylen,
		     CONST uaddr *xoffsets, CONST uaddr *yoffsets,
		     CONST unsigned char imap[256],
		     double xstart, double ystart, double xend, double yend)
/*  This routine will write a monochrome image to a PostScriptPage object.
    The PostScriptPage object must be given by  pspage  .
    The image must be pointed to by  image  .
    The size of the image (in pixels) must be given by  xlen  and  ylen  .
    The offsets for each axis (in bytes) must pointed by  xoffsets  and
    yoffsets  .If these are NULL, the image data is assumed to be contiguous in
    memory.
    The intensity mapping to use must be pointed to by  imap  .If this is NULL,
    a linear mapping is used (0 = black, 255 = white).
    The x starting point (scaled from 0.0 to 1.0) must be given by  xstart  .
    The y starting point (scaled from 0.0 to 1.0) must be given by  ystart  .
    The x ending point (scaled from 0.0 to 1.0) must be given by  xend  .
    The y ending point (scaled from 0.0 to 1.0) must be given by  yend  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    Channel channel;
    uaddr voff;
    unsigned int vcount;
    unsigned int hlen, vlen;
    double hos, vos, hss, vss;
    char txt[STRING_LENGTH];
    static char function_name[] = "psw_mono_image";

    VERIFY_PSPAGE (pspage);
    channel = (*pspage).channel;
    if (ch_puts (channel, "gsave", TRUE) != TRUE) return (FALSE);
    if ( (*pspage).portrait )
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
	vos = xstart;
	hss = yend - ystart;
	vss = xend - xstart;
	hlen = ylen;
	vlen = xlen;
    }
    (void) sprintf (txt, "%6.3f  %6.3f translate %6.3f  %6.3f scale",
		    hos, vos, hss, vss);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    (void) sprintf (txt,
		    "/nx %5d def /ny %5d def /nbits %3d def /line %5d string def incimage",
		    hlen, vlen, 8, hlen);	
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);	
    /*  Write the image  */
    if ( (*pspage).portrait )
    {
	for (vcount = 0; vcount < ylen; ++vcount)
	{
	    voff = (yoffsets == NULL) ? xlen * vcount : yoffsets[vcount];
	    if (write_mono_line (channel, image + voff, xlen, xoffsets, 1,
				 imap, FALSE) != TRUE) return (FALSE);
	}
    }
    else
    {
	for (vcount = 0; vcount < xlen; ++vcount)
	{
	    voff = (xoffsets ==
		    NULL) ? xlen - vcount - 1 : xoffsets[xlen - vcount - 1];
	    if (write_mono_line (channel, image + voff, ylen, yoffsets, xlen,
				 imap, FALSE) != TRUE) return (FALSE);
	}
    }
    if (ch_puts (channel, "grestore", TRUE) != TRUE) return (FALSE);
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
/*  This routine will write a pseudocolour image to a PostScriptPage object.
    The PostScriptPage object must be given by  pspage  .
    The image must be pointed to by  image  .
    The size of the image (in pixels) must be given by  xlen  and  ylen  .
    The offsets for each axis (in bytes) must pointed by  xoffsets  and
    yoffsets  .If these are NULL, the image data is assumed to be contiguous in
    memory.
    The red component colourmap entries must be pointed to by  imap_red  .
    The green component colourmap entries must be pointed to by  imap_green  .
    The blue component colourmap entries must be pointed to by  imap_blue  .
    The x starting point (scaled from 0.0 to 1.0) must be given by  xstart  .
    The y starting point (scaled from 0.0 to 1.0) must be given by  ystart  .
    The x ending point (scaled from 0.0 to 1.0) must be given by  xend  .
    The y ending point (scaled from 0.0 to 1.0) must be given by  yend  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    Channel channel;
    uaddr voff;
    unsigned int vcount;
    unsigned int hlen, vlen;
    double hos, vos, hss, vss;
    char txt[STRING_LENGTH];
    static char function_name[] = "psw_pseudocolour_image";

    VERIFY_PSPAGE (pspage);
    if ( (imap_red == NULL) || (imap_green == NULL) || (imap_blue == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    channel = (*pspage).channel;
    if (ch_puts (channel, "gsave", TRUE) != TRUE) return (FALSE);
    if ( (*pspage).portrait )
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
	vos = xstart;
	hss = yend - ystart;
	vss = xend - xstart;
	hlen = ylen;
	vlen = xlen;
    }
    (void) sprintf (txt, "%6.3f  %6.3f translate %6.3f  %6.3f scale",
		    hos, vos, hss, vss);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    (void) sprintf (txt,
		    "/nx %5d def /ny %5d def /nbits %3d def /rline %5d string def",
		    hlen, vlen, 8, hlen);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    (void) sprintf (txt,
		    "/gline %5d string def /bline %5d string def incclrimage",
		    hlen, hlen);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);	
    /*  Write the image  */
    if ( (*pspage).portrait )
    {
	for (vcount = 0; vcount < ylen; ++vcount)
	{
	    voff = (yoffsets == NULL) ? xlen * vcount : yoffsets[vcount];
	    if (write_mono_line (channel, image + voff, xlen, xoffsets, 1,
				 imap_red, FALSE) != TRUE) return (FALSE);
	    if (write_mono_line (channel, image + voff, xlen, xoffsets, 1,
				 imap_green, FALSE) != TRUE) return (FALSE);
	    if (write_mono_line (channel, image + voff, xlen, xoffsets, 1,
				 imap_blue, FALSE) != TRUE) return (FALSE);
	}
    }
    else
    {
	for (vcount = 0; vcount < xlen; ++vcount)
	{
	    voff = (xoffsets ==
		    NULL) ? xlen - vcount - 1 : xoffsets[xlen - vcount - 1];
	    if (write_mono_line (channel, image + voff, ylen, yoffsets, xlen,
				 imap_red, FALSE) != TRUE) return (FALSE);
	    if (write_mono_line (channel, image + voff, ylen, yoffsets, xlen,
				 imap_green, FALSE) != TRUE) return (FALSE);
	    if (write_mono_line (channel, image + voff, ylen, yoffsets, xlen,
				 imap_blue, FALSE) != TRUE) return (FALSE);
	}
    }
    if (ch_puts (channel, "grestore", TRUE) != TRUE) return (FALSE);
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
/*  This routine will write a colour image to a PostScriptPage object.
    The PostScriptPage object must be given by  pspage  .
    The red component image values must be pointed to by  image_reds  .
    The green component image values must be pointed to by  image_greens  .
    The blue component image values must be pointed to by  image_blues  .
    The size of the image (in pixels) must be given by  xlen  and  ylen  .
    The red component offsets for each axis (in bytes) must pointed by
    xoffsets_red  and  yoffsets_red  .If these are NULL, the image stride must
    be given.
    The green component offsets for each axis (in bytes) must pointed by
    xoffsets_green  and  yoffsets_green  .If these are NULL, the image stride
    must be given.
    The blue component offsets for each axis (in bytes) must pointed by
    xoffsets_blue  and  yoffsets_blue  .If these are NULL, the image stride
    must be given.
    The stride of successive component values must be given by  stride  .This
    is ignored if  xoffsets  and  yoffsets  are both supplied.
    The x starting point (scaled from 0.0 to 1.0) must be given by  xstart  .
    The y starting point (scaled from 0.0 to 1.0) must be given by  ystart  .
    The x ending point (scaled from 0.0 to 1.0) must be given by  xend  .
    The y ending point (scaled from 0.0 to 1.0) must be given by  yend  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    Channel channel;
    uaddr voff_red, voff_green, voff_blue;
    unsigned int vcount, tmp;
    unsigned int hlen, vlen;
    double hos, vos, hss, vss;
    char txt[STRING_LENGTH];
    static char function_name[] = "psw_rgb_image";

    VERIFY_PSPAGE (pspage);
    channel = (*pspage).channel;
    if (ch_puts (channel, "gsave", TRUE) != TRUE) return (FALSE);
    if ( (*pspage).portrait )
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
	vos = xstart;
	hss = yend - ystart;
	vss = xend - xstart;
	hlen = ylen;
	vlen = xlen;
    }
    (void) sprintf (txt, "%6.3f  %6.3f translate %6.3f  %6.3f scale",
		    hos, vos, hss, vss);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    (void) sprintf (txt,
		    "/nx %5d def /ny %5d def /nbits %3d def /rline %5d string def",
		    hlen, vlen, 8, hlen);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    (void) sprintf (txt,
		    "/gline %5d string def /bline %5d string def incclrimage",
		    hlen, hlen);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    /*  Write the image  */
    if ( (*pspage).portrait )
    {
	for (vcount = 0; vcount < ylen; ++vcount)
	{
	    tmp = xlen * vcount * stride;
	    voff_red = (yoffsets_red == NULL) ? tmp : yoffsets_red[vcount];
	    voff_green = (yoffsets_green==NULL) ? tmp : yoffsets_green[vcount];
	    voff_blue = (yoffsets_blue == NULL) ? tmp : yoffsets_blue[vcount];
	    if (write_mono_line (channel, image_reds + voff_red, xlen,
				 xoffsets_red,
				 stride, NULL, FALSE) != TRUE) return (FALSE);
	    if (write_mono_line (channel, image_greens + voff_green, xlen,
				 xoffsets_green,
				 stride, NULL, FALSE) != TRUE) return (FALSE);
	    if (write_mono_line (channel, image_blues + voff_blue, xlen,
				 xoffsets_blue,
				 stride, NULL, FALSE) != TRUE) return (FALSE);
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
	    if (write_mono_line (channel, image_reds + voff_red, ylen,
				 yoffsets_red,
				 xlen * stride, NULL, FALSE)
		!= TRUE) return (FALSE);
	    if (write_mono_line (channel, image_greens + voff_green, ylen,
				 yoffsets_green,
				 xlen * stride, NULL, FALSE)
		!= TRUE) return (FALSE);
	    if (write_mono_line (channel, image_blues + voff_blue, ylen,
				 yoffsets_blue,
				 xlen * stride, NULL, FALSE)
		!= TRUE) return (FALSE);
	}
    }
    if (ch_puts (channel, "grestore", TRUE) != TRUE) return (FALSE);
    return (TRUE);
}   /*  End Function psw_rgb_image  */

/*PUBLIC_FUNCTION*/
flag psw_finish (PostScriptPage pspage, flag eps, flag flush, flag close)
/*  This routine will write a tail to a PostScriptPage object and will
    deallocate the object.
    The PostScriptPage object must be given by  pspage  .
    If the file format should be Encapsulated PostScript, the value of  eps
    should be TRUE, else regular PostScript is written.
    If the value of  flush  is TRUE, the underlying channel obect is flushed.
    If the value of  close  is TRUE, the underlying channel obect is closed  .
    The routine returns TRUE on succes, else it returns FALSE. In either case,
    the PostScriptPage object is deallocated (and closed if  close  is TRUE).
*/
{
    Channel channel;
    static char function_name[] = "psw_finish";

    VERIFY_PSPAGE (pspage);
    FLAG_VERIFY (eps);
    channel = (*pspage).channel;
    (*pspage).magic_number = 0;
    m_free ( (char *) pspage );
    if (ch_puts (channel, "grestore", TRUE) != TRUE)
    {
	if (close) (void) ch_close (channel);
	return (FALSE);
    }
    if (!eps)
    {
	if (ch_puts (channel, "showpage", TRUE) != TRUE)
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
/*  This routine will write a colour line to a PostScriptPage object.
    The PostScriptPage object must be given by  pspage  .
    The red component value must be given by  image_red  .
    The green component value must be given by  image_green  .
    The blue component value must be given by  image_blue  .
    NOTE: the colour component values are scaled from 0.0 (dark) to 1.0 (light)
    The x starting point (scaled from 0.0 to 1.0) must be given by  xstart  .
    The y starting point (scaled from 0.0 to 1.0) must be given by  ystart  .
    The x ending point (scaled from 0.0 to 1.0) must be given by  xend  .
    The y ending point (scaled from 0.0 to 1.0) must be given by  yend  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    Channel channel;
    char txt[STRING_LENGTH];
    static char function_name[] = "psw_rgb_line";

    VERIFY_PSPAGE (pspage);
    channel = (*pspage).channel;
    (void) sprintf (txt, "%6.3f  %6.3f  %6.3f  setrgbcolor", red, green, blue);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    if ( (*pspage).portrait )
    {
	(void) sprintf (txt, "%6.3f  %6.3f M %6.3f  %6.3f D str",
			xstart, ystart, xend, yend);
    }
    else
    {
	(void) sprintf (txt, "%6.3f  %6.3f M %6.3f  %6.3f D str",
			ystart, 1.0 - xstart, yend, 1.0 - xend);
    }
    return ( ch_puts (channel, txt, TRUE) );
}   /*  End Function psw_rgb_line  */

/*PUBLIC_FUNCTION*/
flag psw_rgb_polygon (PostScriptPage pspage,
		      double red, double green, double blue,
		      CONST double *x_arr, CONST double *y_arr,
		      unsigned int num_points, flag fill)
/*  This routine will write a colour polygon to a PostScriptPage object.
    The PostScriptPage object must be given by  pspage  .
    The red component value must be given by  image_red  .
    The green component value must be given by  image_green  .
    The blue component value must be given by  image_blue  .
    NOTE: the colour component values are scaled from 0.0 (dark) to 1.0 (light)
    The array of x vertices must be pointed to by  x_arr  .
    The array of y vertices must be pointed to by  y_arr  .
    NOTE: the vertices are scaled from 0.0 to 1.0
    The number of vertices must be given by  num_points  .
    If the polygon is to be filled, the value of  fill  must be TRUE.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    Channel channel;
    unsigned int count;
    char txt[STRING_LENGTH];
    static char function_name[] = "psw_rgb_polygon";

    VERIFY_PSPAGE (pspage);
    if ( (x_arr == NULL) || (y_arr == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    FLAG_VERIFY (fill);
    if (num_points < 2) return (TRUE);
    channel = (*pspage).channel;
    (void) sprintf (txt, "%6.3f  %6.3f  %6.3f  setrgbcolor", red, green, blue);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    if ( (*pspage).portrait )
    {
	(void) sprintf (txt, "%6.3f  %6.3f M", x_arr[0], y_arr[0]);
    }
    else
    {
	(void) sprintf (txt, "%6.3f  %6.3f M", y_arr[0], 1.0 -x_arr[0]);
    }
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    for (count = 1; count < num_points; ++count)
    {
	if ( (*pspage).portrait )
	{
	    (void) sprintf (txt, "%6.3f  %6.3f D", x_arr[count], y_arr[count]);
	}
	else
	{
	    (void) sprintf (txt, "%6.3f  %6.3f D",
			    y_arr[count], 1.0 - x_arr[count]);
	}
	if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    }
    if (fill) return ( ch_puts (channel, "  closepath  fill", TRUE) );
    return ( ch_puts (channel, "  closepath  stroke", TRUE) );
}   /*  End Function psw_rgb_polygon  */

/*PUBLIC_FUNCTION*/
flag psw_rgb_ellipse (PostScriptPage pspage,
		      double red, double green, double blue,
		      double cx, double cy, double rx, double ry, flag fill)
/*  This routine will write a colour ellipse to a PostScriptPage object.
    The PostScriptPage object must be given by  pspage  .
    The red component value must be given by  image_red  .
    The green component value must be given by  image_green  .
    The blue component value must be given by  image_blue  .
    NOTE: the colour component values are scaled from 0.0 (dark) to 1.0 (light)
    The x centre must be given by  cx  .
    The y centre must be given by  cy  .
    The x radius must be given by  rx  .
    The y radius must be given by  ry  .
    NOTE: the centre and radius values are scaled from 0.0 to 1.0
    If the ellipse is to be filled, the value of  fill  must be TRUE.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    Channel channel;
    double ch, cv, rh, rv, vscale;
    char txt[STRING_LENGTH];
    static char function_name[] = "psw_rgb_ellipse";

    VERIFY_PSPAGE (pspage);
    FLAG_VERIFY (fill);
    channel = (*pspage).channel;
    (void) sprintf (txt, "%6.3f  %6.3f  %6.3f  setrgbcolor", red, green, blue);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "gsave", TRUE) != TRUE) return (FALSE);
    if ( (*pspage).portrait )
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
    (void) sprintf (txt,
		    "newpath  1.0 %6.3f scale  %6.3f %6.3f %6.3f 0 360 arc closepath %s",
		    vscale, ch, (cv) / vscale, rh, fill ? "fill" : "stroke");
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    return ( ch_puts (channel, "grestore", TRUE) );
}   /*  End Function psw_rgb_ellipse  */

/*PUBLIC_FUNCTION*/
flag psw_rgb_text (PostScriptPage pspage, double red, double green,double blue,
		   CONST char *string, CONST char *fontname,
		   unsigned int fontsize,
		   double xstart, double ystart, double angle)
/*  This routine will write a colour string to a PostScriptPage object.
    The PostScriptPage object must be given by  pspage  .
    The red component value must be given by  image_red  .
    The green component value must be given by  image_green  .
    The blue component value must be given by  image_blue  .
    NOTE: the colour component values are scaled from 0.0 (dark) to 1.0 (light)
    The string must be pointed to by  string  .
    The fontname to use must be given by  fontname  .If this is NULL, the
    default font will be used.
    The size of the font (in millimeters) must be given by  fontsize  .
    The x starting point (scaled from 0.0 to 1.0) must be given by  xstart  .
    The y starting point (scaled from 0.0 to 1.0) must be given by  ystart  .
    The angle to rotate the text through (in degrees) must be given by  angle
    Positive angle rotate counter-clockwise from horizontal.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    Channel channel;
    char txt[STRING_LENGTH];
    static char function_name[] = "psw_rgb_text";

    VERIFY_PSPAGE (pspage);
    if ( (string == NULL) || (fontname == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    channel = (*pspage).channel;
    if (ch_puts (channel, "gsave", TRUE) != TRUE) return (FALSE);
    (void) sprintf (txt, "%6.3f  %6.3f  %6.3f  setrgbcolor", red, green, blue);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    (void) sprintf (txt, "/%s findfont", fontname);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    (void) sprintf (txt, "%6.3f scalefont  setfont",
		    (double) fontsize / 10.0 / (*pspage).fsize);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    if ( (*pspage).portrait )
    {
	(void) sprintf (txt, "%6.3f  %6.3f  moveto", xstart, ystart);
    }
    else
    {
	(void) sprintf (txt, "%6.3f  %6.3f  moveto", ystart, 1.0 - xstart);
    }
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    if ( (*pspage).portrait )
    {
	(void) sprintf (txt, "%6.3f rotate", angle);
    }
    else
    {
	(void) sprintf (txt, "%6.3f rotate", angle + 90.0);
    }
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    (void) sprintf (txt, "(%s)  show", string);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    return ( ch_puts (channel, "grestore", TRUE) );
}   /*  End Function psw_rgb_text  */


/*  Private functions follow  */

static flag write_header (Channel channel, double hoffset, double voffset,
			  double hsize, double vsize)
/*  This routine will write a PostScript header required prior to writing an
    image.
    The channel to write to must be given by  channel  .
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
    int lxi, lyi;
    time_t clock;
    struct timeval date;
    double cm_to_points;
    struct tm *ltime;
    char txt[STRING_LENGTH];
    char hostname[STRING_LENGTH];
    extern char module_name[STRING_LENGTH + 1];
    extern char module_version_date[STRING_LENGTH + 1];
    extern char *sys_errlist[];
    static char function_name[] = "write_header";

    /*  Put the bounding box in pts.  */
    cm_to_points = 72.0 / 2.54;
    lxi = (int) (cm_to_points * hsize + 0.5);
    lyi = (int) (cm_to_points * vsize + 0.5);

    /*  Put header for encapsulated postscript */
    if (ch_puts (channel, "%!PS", TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "%%Title: ", TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "%%Creator: ", FALSE) != TRUE) return (FALSE);
    if (strcmp (module_name, "<<Unknown>>") == 0)
    {
	(void) sprintf (txt, "Karma  psw_  package");
    }
    else
    {
	if (strcmp (module_version_date, "Unknown") == 0)
	{
	    (void) sprintf (txt,
			    "module: \"%s\" using Karma  psw_  package",
			    module_name);
	}
	else
	{
	    (void) sprintf (txt,
			    "module: \"%s\" version: \"%s\" using Karma  psw_  package",
			    module_name, module_version_date);
	}
    }
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "%%CreationDate: ", FALSE) != TRUE) return (FALSE);
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
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    r_gethostname (hostname, STRING_LENGTH);
    (void) sprintf (txt, "%%%%For: %s@%s", r_getenv ("USER"), hostname);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "%%Pages: 0", TRUE) != TRUE) return (FALSE);
    (void) sprintf (txt, "%%%%BoundingBox: 0 0 %d %d", lxi, lyi);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "%%EndComments", TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "%", TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel,
		 "/lwid   0.0001 def  /slw {lwid mul setlinewidth} def",
		 TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "1   slw 1 setlinejoin 1 setlinecap",
		 TRUE) != TRUE) return (FALSE);
    /*  Compute translation and scale such that (0.0, 0.0) is the origin and
	(1.0, 1.0) is the far end of the region  */
    (void) sprintf (txt, "%6.3f %6.3f translate  %6.3f %6.3f scale",
		    hoffset * cm_to_points, voffset * cm_to_points,
		    hsize * cm_to_points, vsize * cm_to_points);
    if (ch_puts (channel, txt, TRUE) != TRUE) return (FALSE);
    /*  Set up definitions.  */
    if (ch_puts (channel, "/M {moveto} def /D {lineto} def ",
		 TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/m {rmoveto} def /d {rlineto} def",
		 TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/r {rotate} def /solid {[]0 setdash} def",
		 TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/sp {currentpoint /y exch def /x exch def} def",
		 TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/rp {x y M} def", TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/str {sp stroke rp} def  /dot { 0 0 d} def",
		 TRUE) != TRUE) return (FALSE);
  
    if (ch_puts (channel, "/cfont /Courier def ",TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/sfont /Symbol def", TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/CF {cfont findfont} def ",
		 TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/SF {sfont findfont} def ",
		 TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/HF {/Helvetica findfont} def ",
		 TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/HBF {/Helvetica-bold findfont} def ", TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/TF {/Times-Roman findfont} def ",
		 TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/TBF {/Times-Bold findfont} def ",
		 TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel, "/SS {scalefont setfont } def ",
		 TRUE) != TRUE) return (FALSE);

    if (ch_puts (channel,
		 "/incimage {nx ny nbits [nx 0 0 ny 0 0] {currentfile line readhexstring pop} image} def ",
		 TRUE) != TRUE) return (FALSE);
    if (ch_puts (channel,
		 "/incclrimage {nx ny nbits [nx 0 0 ny 0 0] {currentfile rline readhexstring pop}  {currentfile gline readhexstring pop}  {currentfile bline readhexstring pop}  true 3 colorimage} def ",
		 TRUE) != TRUE) return (FALSE);
    return (TRUE);
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

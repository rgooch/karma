/*LINTLIBRARY*/
/*  misc.c

    This code provides image conversion routines.

    Copyright (C) 1993,1994  Richard Gooch

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

/*  This file contains all routines needed for the conversion of images.


    Written by      Richard Gooch   18-SEP-1993

    Updated by      Richard Gooch   19-SEP-1993

    Updated by      Richard Gooch   9-AUG-1994: Converted to full ANSI C
  function declarations.

    Last updated by Richard Gooch   26-NOV-1994: Moved to  packages/imc/main.c


*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <karma.h>
#include <karma_imc.h>
#include <karma_m.h>
#include <karma_a.h>

#define MIN_COLOUR_DIFFERENCE 64
#define MAX_INTENSITY 255
#define MAX_NORM 196608 /* (256*256)*3 */
#define MAX_COLOURS 256


/*  Private functions  */
STATIC_FUNCTION (unsigned int c_24_to_8, (int image_size,
					  unsigned char *image_reds,
					  unsigned char *image_greens,
					  unsigned char *image_blues,
					  int stride24,
					  unsigned char *out_image,
					  int stride8,
					  unsigned int max_colours,
					  unsigned char *palette_reds,
					  unsigned char *palette_greens,
					  unsigned char *palette_blues,
					  unsigned int speed) );
STATIC_FUNCTION (unsigned int c_24_to_8_slow, (int image_size,
					       unsigned char *image_reds,
					       unsigned char *image_greens,
					       unsigned char *image_blues,
					       int stride24,
					       unsigned char *out_image,
					       int stride8,
					       unsigned int max_colours,
					       unsigned char *palette_reds,
					       unsigned char *palette_greens,
					       unsigned char *palette_blues,
					       unsigned int speed) );

/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag imc_24to8 (unsigned int image_size, unsigned char *image_reds,
		unsigned char *image_greens, unsigned char *image_blues,
		int stride24, unsigned char *out_image, int stride8,
		unsigned int max_colours, unsigned int speed,
		packet_desc **pack_desc, char **packet)
/*  This routine will convert a 24 bit truecolour image to an 8 bit
    pseudocolour image.
    The size of the image (in pixels) must be given by  image_size  .
    The red component data of the truecolour image must be pointed to by
    image_reds  .
    The green component data of the truecolour image must be pointed to by
    image_greens  .
    The blue component data of the truecolour image must be pointed to by
    image_blues  .
    The stride (in bytes) between adjacent pixels in the truecolour image must
    be given by  stride24  .
    The output (8 bit pseudocolour) image date must be pointed to by  out_image
    The stride (in bytes) between adjacent pixels in the pseudocolour image
    must be given by  stride8  .
    The maximum number of unique colours permitted (ie. the maximum colour
    palette size that can be supported) must be given by  max_colours  .
    The desired speed of the routine must be given by  speed  .This value may
    range from 0 to 9. A value of 0 will result in the slowest but highest
    quality conversion (ie. the routine tries very hard to choose the best
    colour palette). A value of 9 will result in the fastest conversion, at the
    expense of image quality.
    The pointer to the top level packet descriptor of the general data
    structure which contains the colourmap will be written to the storage
    pointed to by  pack_desc  .
    The pointer to the top level packet of the general data structure which
    contains the colourmap will be written to the storage pointed to by  
    packet  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int count;
    unsigned int cmap_size;
    unsigned short *cmap;
    unsigned char palette_reds[MAX_COLOURS];
    unsigned char palette_greens[MAX_COLOURS];
    unsigned char palette_blues[MAX_COLOURS];
    static char function_name[] = "imc_24to8";

    if ( (image_reds == NULL) || (image_greens == NULL) ||
	(image_blues == NULL) || (out_image == NULL) ||
	(pack_desc == NULL) || (packet == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if (max_colours > MAX_COLOURS)
    {
	(void) fprintf (stderr,
			"max_colours: %u must not be greater than: %u\n",
			max_colours, MAX_COLOURS);
	a_prog_bug (function_name);
    }
    if ( ( cmap_size = c_24_to_8 ( (int) image_size, image_reds, image_greens,
				  image_blues, stride24, out_image, stride8,
				  max_colours, palette_reds, palette_greens,
				  palette_blues, speed ) ) < 2 )
    {
	(void) fprintf (stderr, "Error compressing 24bit TrueColour image\n");
	return (FALSE);
    }
    if ( ( cmap = ds_cmap_alloc_colourmap (cmap_size, (multi_array **) NULL,
					   pack_desc, packet) ) == NULL )
    {
	m_error_notify (function_name, "RGBcolourmap");
	return (FALSE);
    }
    for (count = 0; count < cmap_size; ++count)
    {
	*cmap++ = palette_reds[count] << 8;
	*cmap++ = palette_greens[count] << 8;
	*cmap++ = palette_blues[count] << 8;
    }
    return (TRUE);
}   /*  End Function imc_24to8  */


/*  Private functions follow  */

static unsigned int c_24_to_8 (image_size, image_reds, image_greens,
			       image_blues, stride24, out_image, stride8,
			       max_colours, palette_reds, palette_greens,
			       palette_blues, speed)
/*  This routine will convert a 24 bit truecolour image to an 8 bit
    pseudocolour image.
    The size image (in pixels) must be given by  image_size  .
    The red component data of the truecolour image must be pointed to by
    image_reds  .
    The green component data of the truecolour image must be pointed to by
    image_greens  .
    The blue component data of the truecolour image must be pointed to by
    image_blues  .
    The stride (in bytes) between adjacent pixels in the truecolour image must
    be given by  stride24  .
    The output (8 bit pseudocolour) image date must be pointed to by  out_image
    The stride (in bytes) between adjacent pixels in the pseudocolour image
    must be given by  stride8  .
    The maximum number of unique colours (ie. the maximum colour palette size)
    must be given by  max_colours  .
    The palette red components must be written to the array pointed to by
    palette_reds  .
    The palette green components must be written to the array pointed to by
    palette_greens  .
    The palette blue components must be written to the array pointed to by
    palette_blues  .
    The desired speed of the routine must be given by  speed  .This value may
    range from 0 to 9. A value of 0 will result in the slowest but highest
    quality conversion (ie. the routine tries very hard to choose the best
    colour palette). A value of 9 will result in the fastest conversion, at the
    expense of image quality.
    The routine returns the actual number of unique colours in the palette.
*/
int image_size;
unsigned char *image_reds;
unsigned char *image_greens;
unsigned char *image_blues;
int stride24;
unsigned char *out_image;
int stride8;
unsigned int max_colours;
unsigned char *palette_reds;
unsigned char *palette_greens;
unsigned char *palette_blues;
unsigned int speed;
{
    int red, green, blue;
    int count;
    unsigned char pixel;
    static char function_name[] = "c_24_to_8";

    if (speed > 5)
    {
	if (max_colours < 128)
	{
	    (void) fprintf (stderr, "Not enough colours: must have 128\n");
	    return (0);
	}
	for (count = 0; count < image_size; ++count, image_reds += stride24,
	     image_greens += stride24, image_blues += stride24,
	     out_image += stride8)
	{
	    pixel = (unsigned char) (*image_reds & 0xe0) >> 5;
	    pixel |= (unsigned char) (*image_greens & 0xc0) >> 3;
	    pixel |= (unsigned char) (*image_blues & 0xc0) >> 1;
	    *out_image = pixel;
	}
	/*  Setup palette  */
	for (count = 0; count < 128; ++count)
	{
	    red = count & 0x07;
	    green = (count & 0x18) >> 3;
	    blue = (count & 0x60) >> 5;
	    palette_reds[count] = ( (red * MAX_INTENSITY) / 7 );
	    palette_greens[count] = ( (green * MAX_INTENSITY) / 3 );
	    palette_blues[count] = ( (blue * MAX_INTENSITY) / 3 );
	}
	return (128);
    }
    return ( c_24_to_8_slow (image_size, image_reds, image_greens,
			     image_blues, stride24, out_image, stride8,
			     max_colours, palette_reds, palette_greens,
			     palette_blues, speed) );
}   /*  End Function c_24_to_8  */

static unsigned int c_24_to_8_slow (image_size, image_reds, image_greens,
				    image_blues, stride24, out_image, stride8,
				    max_colours, palette_reds, palette_greens,
				    palette_blues, speed)
int image_size;
unsigned char *image_reds;
unsigned char *image_greens;
unsigned char *image_blues;
int stride24;
unsigned char *out_image;
int stride8;
unsigned int max_colours;
unsigned char *palette_reds;
unsigned char *palette_greens;
unsigned char *palette_blues;
unsigned int speed;
{
    int norm;
    int norm_red;
    int norm_green;
    int norm_blue;
    int min_norm;
    int cnt_pixel;
    int cnt1;
    flag not_matched;
    unsigned int red, green, blue;
    unsigned int cnt_mutual;
    unsigned int cnt_colour;
    unsigned int cnt_palette;
    unsigned int palette_num;
    unsigned int num_colours;
    unsigned int cnt_freq;
    unsigned char *reds;
    unsigned char *greens;
    unsigned char *blues;
    unsigned int *frequency;
    unsigned int max_freq;
    unsigned int colour_num;
    static char function_name[] = "c_24_to_8_slow";

    /* Program consists of   stages */
    (void) fprintf (stderr, "Stage 1: size: %d...\n", (int) image_size);
    /* Stage 1. Count input palette colour number and calculate frequencies */
    /* 	of every colour.						*/
    if ( ( reds = (unsigned char *) m_alloc (sizeof *reds * image_size) )
	== NULL )
    {
	m_abort (function_name, "red array");
    }
    if ( ( greens = (unsigned char *) m_alloc (sizeof *greens * image_size) )
	== NULL )
    {
	m_abort (function_name, "green array");
    }
    if ( ( blues = (unsigned char *) m_alloc (sizeof *blues * image_size) )
	== NULL )
    {
	m_abort (function_name, "blue array");
    }
    if ( ( frequency = (unsigned int *)
	  m_alloc (sizeof *frequency * image_size) )
	== NULL )
    {
	m_abort (function_name, "frequency array");
    }
    *reds = *image_reds;
    *greens = *image_greens;
    *blues = *image_blues;
    *frequency = 1;
    num_colours = 1;
    for (cnt_pixel = 1; cnt_pixel < image_size; cnt_pixel++)
    {
	cnt1 = cnt_pixel * stride24;
	red = image_reds[cnt1];
	green = image_greens[cnt1];
	blue = image_blues[cnt1];
	for (cnt_mutual = 0, not_matched = TRUE;
	     (cnt_mutual < num_colours) && not_matched;
	     cnt_mutual++)
	{
#ifndef dummy
	    norm_red = (int) red - (int) reds[cnt_mutual];
	    norm_green = (int) green - (int) greens[cnt_mutual];
	    norm_blue = (int) blue - (int) blues[cnt_mutual];
	    norm = norm_red*norm_red+norm_green*norm_green+norm_blue*norm_blue;
	    if (norm < MIN_COLOUR_DIFFERENCE)
#else
	    if ( (red == reds[cnt_mutual]) &&
		(green == greens[cnt_mutual]) &&
		(blue == blues[cnt_mutual]) )
#endif
	    {
		++frequency[cnt_mutual];
		/*  Exit this loop  */
		not_matched = FALSE;
	    }
	}
	if (not_matched)
	{
	    reds[num_colours] = red;
	    greens[num_colours] = green;
	    blues[num_colours] = blue;
	    frequency[num_colours] = 1;
	    ++num_colours;
	}
	if (cnt_pixel % 100000 == 0)
	{
	    (void) fprintf (stderr, "Unique colours: %u\tpixel number: %u\n",
			    num_colours, cnt_pixel);
	}
    }

    (void) fprintf (stderr, "1: %d unique colours...\n", (int) num_colours);
    /* If original palette containes less then maximum allowed colours */
    /* convert it to output image */
    if (num_colours <= max_colours)
    {
	for (cnt_colour = 0; cnt_colour < num_colours; cnt_colour++)
	{
	    palette_reds[cnt_colour] = reds[cnt_colour];
	    palette_greens[cnt_colour] = greens[cnt_colour];
	    palette_blues[cnt_colour] = blues[cnt_colour];
	}
	for (cnt_pixel = 0; cnt_pixel < image_size; cnt_pixel++)
	{
	    min_norm = MAX_NORM;
	    colour_num = 0;
	    cnt1 = cnt_pixel * stride24;
	    red = image_reds[cnt1];
	    green = image_greens[cnt1];
	    blue = image_blues[cnt1];
	    for (cnt_colour = 0; cnt_colour < num_colours; cnt_colour++)
	    {
		norm_red = (int) red - (int) palette_reds[cnt_colour];
		norm_green = (int) green - (int) palette_greens[cnt_colour];
		norm_blue = (int) blue - (int) palette_blues[cnt_colour];
		norm =
		norm_red*norm_red+norm_green*norm_green+norm_blue*norm_blue;
		if (norm < MIN_COLOUR_DIFFERENCE)
		{
		    colour_num = cnt_colour;
		    cnt_colour = num_colours;
		    continue;
		}
		if (norm < min_norm)
		{
		    min_norm = norm;
		    colour_num = cnt_colour;
		}
	    }
	    out_image[cnt_pixel * stride8] = colour_num;
	    if (cnt_pixel % 100000 == 0) (void) fprintf (stderr, ".");
	}
	m_free ( (char *) reds );
	m_free ( (char *) greens );
	m_free ( (char *) blues );
	m_free ( (char *) frequency );
	return (num_colours);
    }
    (void) fprintf (stderr, "Stage 2...\n");
    /*				                	*/
    /* Stage 2. Find  max_colours  colours of highest frequency  */
    /*							*/
    for (cnt_palette = 0; cnt_palette < max_colours; cnt_palette++)
    {
	max_freq = *frequency;
	palette_num = 0;
	for (cnt_freq = 0; cnt_freq < num_colours; cnt_freq++)
	{
	    if (frequency[cnt_freq] > max_freq)
	    {
		max_freq = frequency[cnt_freq];
		palette_num = cnt_freq;
	    }
	}
	palette_reds[cnt_palette] = reds[palette_num];
	palette_greens[cnt_palette] = greens[palette_num];
	palette_blues[cnt_palette] = blues[palette_num];
	frequency[palette_num] = 0;	/* spoil it */
    }  /* End 8 bit loop */
    (void) fprintf (stderr, "Stage 3...\n");
    /*							*/
    /* Stage 3. Approximate real pixel colour with colour	*/
    /*	from 8 bit palette.				*/
    /*    That's where various approximation techniques	*/
    /* 	can lead to different calculation speeds.	*/
    for (cnt_pixel = 0; cnt_pixel < image_size; cnt_pixel++)
    {
	min_norm = MAX_NORM;
	colour_num = 0;
	cnt1 = cnt_pixel * stride24;
	red = image_reds[cnt1];
	green = image_greens[cnt1];
	blue = image_blues[cnt1];
	for (cnt_colour = 0; cnt_colour < max_colours; cnt_colour++)
	{
	    norm_red = (int) red - (int) palette_reds[cnt_colour];
	    norm_green = (int) green - (int) palette_greens[cnt_colour];
	    norm_blue = (int) blue - (int) palette_blues[cnt_colour];
	    norm = norm_red*norm_red+norm_green*norm_green+norm_blue*norm_blue;
	    if (norm < MIN_COLOUR_DIFFERENCE)
	    {
		colour_num = cnt_colour;
		cnt_colour = max_colours;
		continue;
	    }
	    if (norm < min_norm)
	    {
		min_norm = norm;
		colour_num = cnt_colour;
	    }
	}
	out_image[cnt_pixel * stride8] = colour_num;
	if (cnt_pixel % 100000 == 0) (void) fprintf (stderr, ".");
    }
    (void) fprintf (stderr, "\n");
    m_free( (char *) reds);
    m_free( (char *) greens);
    m_free( (char *) blues);
    m_free( (char *) frequency);
    return (max_colours);
}   /*  End Function c_24_to_8_slow  */

/*LINTLIBRARY*/
/*  misc1.c

    This code provides simple colourmap generation routines.

    Copyright (C) 1992-1996  Richard Gooch

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

    This file contains the various utility routines for writing simple colour
    values.


    Written by      Richard Gooch   22-FEB-1993: Copied from
  kimage_cmap_funcs.c  in source/modules/kimage

    Updated by      Richard Gooch   23-FEB-1993: Modified from original code
  and added stripchart functionality.

    Updated by      Richard Gooch   2-MAR-1993: Changed some parameters and
  moved from level1 (c_) to level2.

    Updated by      Richard Gooch   6-AUG-1993: Fixed bug in  cf_greyscale1
  which could cause a divide by zero when x == 0.0

    Updated by      Richard Gooch   12-NOV-1993: Added colourmap functions
  submitted by Tom Oosterlo.

    Updated by      Richard Gooch   22-NOV-1993: Added colourmap function
  submitted by Jeanne Young.

    Updated by      Richard Gooch   5-MAY-1994: Changed variable  random  to
  random_colour_table  to allow compilation on SGI. Also split up into many
  files because of a fucking stupid limit with the SGI compiler.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/cf/misc1.c

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Updated by      Richard Gooch   31-MAR-1996: Changed documentation style.

    Updated by      Richard Gooch   28-APR-1996: Made greyscale functions cope
  with NULL colour arrays.

    Last updated by Richard Gooch   23-JUN-1996: Created new <cf_greyscale1>
  routine and renamed others.


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_cf.h>
#include <karma_a.h>
#include <karma_n.h>

#define MAX_INTENSITY 65535


/*  Private functions  */
STATIC_FUNCTION (double ef, (double xx, double c, double x0) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void cf_greyscale1 (unsigned int num_cells, unsigned short *reds,
		    unsigned short *greens, unsigned short *blues,
		    unsigned int stride, double x, double y, void *var_param)
/*  [SUMMARY] Compute a greyscale colourmap.
    <num_cells> The number of colour cells to modify.
    <reds> The array of red intensity values. This may be NULL.
    <greens> The array of green intensity values. This may be NULL.
    <blues> The array of blue intensity values. This may be NULL.
    <stride> The stride (in unsigned shorts) between intensity values in the
    array.
    <x> A parameter used to compute the colour values.
    <y> A parameter used to compute the colour values.
    <max_cells> A parameter used to compute the colour values.
    [RETURNS] Nothing.
*/
{
    unsigned int pixel_count;
    double slope, offset, factor, intensity;
    static char function_name[] = "cf_greyscale1";

    if ( (x < 0.0) || (x > 1.0) || (y < 0.0) || (y > 1.0) )
    {
	(void) fprintf (stderr, "x or y out of range\n");
	a_prog_bug (function_name);
    }
    /*  Invert y  */
    y = 1.0 - y;
    if (y < 1e-3) slope = 1e3;
    else slope = (1.0 - y) / y;
    offset = 0.5 - slope * x;
    factor = 1.0 / (double) (num_cells - 1);
    /*  Now compute the colours  */
    for (pixel_count = 0; pixel_count < num_cells; ++pixel_count)
    {
	intensity = slope * (double) pixel_count * factor + offset;
	if (intensity < 0.0) intensity = 0.0;
	else if (intensity > 1.0) intensity = 1.0;
	intensity *= MAX_INTENSITY;
	if (reds != NULL) reds[pixel_count * stride] = intensity;
	if (greens != NULL) greens[pixel_count * stride] = intensity;
	if (blues != NULL) blues[pixel_count * stride] = intensity;
    }
}   /*  End Function cf_greyscale1  */

/*PUBLIC_FUNCTION*/
void cf_greyscale2 (unsigned int num_cells, unsigned short *reds,
		    unsigned short *greens, unsigned short *blues,
		    unsigned int stride, double x, double y, void *var_param)
/*  [SUMMARY] Compute a greyscale colourmap.
    <num_cells> The number of colour cells to modify.
    <reds> The array of red intensity values. This may be NULL.
    <greens> The array of green intensity values. This may be NULL.
    <blues> The array of blue intensity values. This may be NULL.
    <stride> The stride (in unsigned shorts) between intensity values in the
    array.
    <x> A parameter used to compute the colour values.
    <y> A parameter used to compute the colour values.
    <max_cells> A parameter used to compute the colour values.
    [RETURNS] Nothing.
*/
{
    unsigned int pixel_count;
    float intensity;
    static char function_name[] = "cf_greyscale2";

    if ( (x < 0.0) || (x > 1.0) || (y < 0.0) || (y > 1.0) )
    {
	(void) fprintf (stderr, "x or y out of range\n");
	a_prog_bug (function_name);
    }
    /*  Now compute the colours  */
    for (pixel_count = 0;
	 (float) pixel_count / (float) (num_cells - 1) <= x;
	 ++pixel_count)
    {
	if (x == 0.0)
	{
	    intensity = y;
	}
	else
	{
	    intensity = (float) pixel_count / (float) (num_cells - 1) / x * y;
	}
	intensity *= MAX_INTENSITY;
	if (reds != NULL) reds[pixel_count * stride] = intensity;
	if (greens != NULL) greens[pixel_count * stride] = intensity;
	if (blues != NULL) blues[pixel_count * stride] = intensity;
    }
    for (; pixel_count < num_cells; ++pixel_count)
    {
	intensity = (float) pixel_count / (float) (num_cells - 1) - x;
	intensity = y + (1.0 - y) * intensity / (1.0 - x);
	intensity *= MAX_INTENSITY;
	if (reds != NULL) reds[pixel_count * stride] = intensity;
	if (greens != NULL) greens[pixel_count * stride] = intensity;
	if (blues != NULL) blues[pixel_count * stride] = intensity;
    }
}   /*  End Function cf_greyscale2  */

/*PUBLIC_FUNCTION*/
void cf_greyscale3 (unsigned int num_cells, unsigned short *reds,
		    unsigned short *greens, unsigned short *blues,
		    unsigned int stride, double x, double y, void *var_param)
/*  [SUMMARY] Compute a greyscale colourmap.
    [PURPOSE] This routine will compute a greyscale colourmap using the Glynn
    Rogers curvature function.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values. This may be NULL.
    <greens> The green intensity values. This may be NULL.
    <blues> The blue intensity values. This may be NULL.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <y> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <var_param> A parameter used to compute the colour values. Ignored.
    [RETURNS] Nothing.
*/
{
    stripchart s;

    s.patternlength = 1;

    s.pattern[0][0] = 1;
    s.pattern[0][1] = 1;
    s.pattern[0][2] = 1;

    cf_stripchart (num_cells, reds, greens, blues, stride, x, y, &s);
}   /*  End Function cf_greyscale3  */

/*PUBLIC_FUNCTION*/
void cf_rainbow1 (unsigned int num_cells, unsigned short *reds,
		  unsigned short *greens, unsigned short *blues,
		  unsigned int stride, double x, double y, void *var_param)
/*  [SUMMARY] Compute a rainbow colourmap.
    [PURPOSE] This routine will compute a colourmap using the Glynn Rogers
    curvature function. This map goes: blue-cyan-green-yellow-red-magenta.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values.
    <greens> The green intensity values.
    <blues> The blue intensity values.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <y> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <var_param> A parameter used to compute the colour values. Ignored.
    [RETURNS] Nothing.
*/
{
    stripchart s;

    s.patternlength=5;

    s.pattern[0][0] = 0;       s.pattern[0][1] = 1;       s.pattern[0][2] = 99;
    s.pattern[1][0] = 0;       s.pattern[1][1] = 99;      s.pattern[1][2] = -1;
    s.pattern[2][0] = 1;       s.pattern[2][1] = 99;      s.pattern[2][2] = 0;
    s.pattern[3][0] = 99;      s.pattern[3][1] = -1;      s.pattern[3][2] = 0;
    s.pattern[4][0] = 99;      s.pattern[4][1] = 0;       s.pattern[4][2] = 1;

    cf_stripchart(num_cells,reds,greens,blues, stride, x, y, &s);
}   /*  End Function cf_rainbow1  */

/*PUBLIC_FUNCTION*/
void cf_rainbow2 (unsigned int num_cells, unsigned short *reds,
		  unsigned short *greens, unsigned short *blues,
		  unsigned int stride, double x, double y, void *var_param)
/*  [SUMMARY] Compute a rainbow colourmap.
    [PURPOSE] This routine will compute a colourmap using the Glynn Rogers
    curvature function. This map goes: black-blue-cyan-green-yellow-red.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values.
    <greens> The green intensity values.
    <blues> The blue intensity values.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <y> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <var_param> A parameter used to compute the colour values. Ignored.
    [RETURNS] Nothing.
*/
{
    stripchart s;

    s.patternlength=5;

    s.pattern[0][0] = 0;       s.pattern[0][1] = 0;       s.pattern[0][2] = 1;
    s.pattern[1][0] = 0;       s.pattern[1][1] = 1;       s.pattern[1][2] = 99;
    s.pattern[2][0] = 0;       s.pattern[2][1] = 99;      s.pattern[2][2] = -1;
    s.pattern[3][0] = 1;       s.pattern[3][1] = 99;      s.pattern[3][2] = 0;
    s.pattern[4][0] = 99;      s.pattern[4][1] = -1;      s.pattern[4][2] = 0;

    cf_stripchart(num_cells,reds,greens,blues, stride, x, y, &s);
}   /*  End Function cf_rainbow2  */

/*PUBLIC_FUNCTION*/
void cf_rainbow3 (unsigned int num_cells, unsigned short *reds,
		  unsigned short *greens, unsigned short *blues,
		  unsigned int stride, double x, double y, void *var_param)
/*  [SUMMARY] Compute a rainbow colourmap.
    [PURPOSE] This routine will compute a colourmap using the Glynn Rogers
    curvature function. This map goes: black-blue-cyan-green-yellow-white.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values.
    <greens> The green intensity values.
    <blues> The blue intensity values.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <y> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <var_param> A parameter used to compute the colour values. Ignored.
    [RETURNS] Nothing.
*/
{
    stripchart s;

    s.patternlength=5;

    s.pattern[0][0] = 0;       s.pattern[0][1] = 0;       s.pattern[0][2] = 1;
    s.pattern[1][0] = 0;       s.pattern[1][1] = 1;       s.pattern[1][2] = 99;
    s.pattern[2][0] = 0;       s.pattern[2][1] = 99;      s.pattern[2][2] = -1;
    s.pattern[3][0] = 1;       s.pattern[3][1] = 99;      s.pattern[3][2] = 0;
    s.pattern[4][0] = 99;      s.pattern[4][1] = 99;      s.pattern[4][2] = 1;

    cf_stripchart(num_cells,reds,greens,blues, stride, x, y, &s);
}   /*  End Function cf_rainbow3  */

/*PUBLIC_FUNCTION*/
void cf_cyclic1 (unsigned int num_cells, unsigned short *reds,
		 unsigned short *greens, unsigned short *blues,
		 unsigned int stride, double x, double y, void *var_param)
/*  [SUMMARY] Compute a rainbow colourmap.
    [PURPOSE] This routine will compute a colourmap using the Glynn Rogers
    curvature function. This map goes: blue-cyan-green-yellow-white.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values.
    <greens> The green intensity values.
    <blues> The blue intensity values.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <y> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <var_param> A parameter used to compute the colour values. Ignored.
    [RETURNS] Nothing.
*/
{
    stripchart s;

    s.patternlength=6;

    s.pattern[0][0] = 0;       s.pattern[0][1] = 2;       s.pattern[0][2] = 99;
    s.pattern[1][0] = 0;       s.pattern[1][1] = 99;      s.pattern[1][2] = -2;
    s.pattern[2][0] = 2;       s.pattern[2][1] = 99;      s.pattern[2][2] = 0;
    s.pattern[3][0] = 99;      s.pattern[3][1] = -2;      s.pattern[3][2] = 0;
    s.pattern[4][0] = 99;      s.pattern[4][1] = 0;       s.pattern[4][2] = 2;
    s.pattern[5][0] = -2;      s.pattern[5][1] = 0;       s.pattern[5][2] = 99;

    cf_stripchart (num_cells,reds,greens,blues, stride, x, y, &s);
}   /*  End Function cf_cyclic1  */

/*PUBLIC_FUNCTION*/
void cf_stripchart (unsigned int num_cells, unsigned short *reds,
		    unsigned short *greens, unsigned short *blues,
		    unsigned int stride, double x, double y, void *chart)
/*  [SUMMARY] Compute a rainbow colourmap.
    [PURPOSE] This routine will compute a colourmap using the Glynn Rogers
    curvature function and a strip chart.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values. This may be NULL.
    <greens> The green intensity values. This may be NULL.
    <blues> The blue intensity values. This may be NULL.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    This changes the curvature of the ramping function.
    <y> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    This changes the curvature of the ramping function.
    <chart> A pointer to a stripchart of type <<stripchart>>.
    The following values are used to determine the shape of the graph:
    0 : graph is at zero for this interval
    99 : graph is at maximum
    1 : graph ramps up from zero to max using Glynn Rogers function
    -1 : graph ramps down from max to zero using Glynn Rogers function
    2 (-2) : graph ramps up (down) using the sine of Glynn Rogers function
    [RETURNS] Nothing.
*/
{
    unsigned int pixel_count;
    double pixel,table,denominator,x0,c,offset;
    double patnumf,divn,up_intensity,down_intensity;
    double temp = 0.0;  /*  Initialised to keep compiler happy  */
    int patnum,i,val;
    stripchart *schart=(stripchart *)chart;
    static char function_name[] = "cf_stripchart";

    x0 = x;
    c = y * 12.;
    if (c <= 0.0) c = 0.001;
    offset = ef (0.0, c, x0);
    denominator = ef (1.0, c, x0) - offset;

    for (pixel_count = 0; pixel_count < num_cells; ++pixel_count)    {
	pixel = (double) pixel_count / ( (double) num_cells - 1.0 );
	table = ef (pixel, c, x0) - offset;
	if (denominator <= 0.0) denominator = 0.1;
	table /= denominator;
	if(table<0.0) table=0.0; /* Ensure table is >=0.0 */
	if(table>=1.0) table=0.999; /* Ensure table is <1.0 */

	patnumf=table*(double)schart->patternlength;
	patnum=(int)patnumf;
	divn=(double)patnum/(double)schart->patternlength;
	up_intensity = (table - divn) * schart->patternlength;
	down_intensity = 1.0 - up_intensity;

	for(i=0;i<3;i++) {
	    val=(schart->pattern)[patnum][i];
	    if (val ==  99) temp = MAX_INTENSITY;
	    else if (val ==  0) temp = 0;
	    else if (val ==  1) temp = up_intensity*MAX_INTENSITY;
	    else if (val == -1) temp = down_intensity*MAX_INTENSITY;
	    else if (val ==  2) temp = sin(up_intensity*PI_ON_2)*MAX_INTENSITY;
	    else if (val == -2) temp = sin(down_intensity*PI_ON_2)*MAX_INTENSITY;
	    else
	    {
		(void) fprintf (stderr,
				"Invalid value in pattern[%d][%d] = %d\n",
				patnum, i, val);
		a_prog_bug (function_name);
	    }
	    if ( (i == 0) && (reds != NULL) )
		reds[pixel_count * stride] = temp;
	    if ( (i == 1) && (greens != NULL) )
		greens[pixel_count * stride] = temp;
	    if ( (i == 2) && (blues != NULL) )
		blues[pixel_count * stride]  = temp;
	}
    }
}   /*  End Function cf_stripchart  */

/*PUBLIC_FUNCTION*/
void cf_random_grey (unsigned int num_cells, unsigned short *reds,
		     unsigned short *greens, unsigned short *blues,
		     unsigned int stride, double x, double y, void *var_param)
/*  [SUMMARY] Compute a random grey colourmap.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values. This may be NULL.
    <greens> The green intensity values. This may be NULL.
    <blues> The blue intensity values. This may be NULL.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <y> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <var_param> A parameter used to compute the colour values. Ignored.
    [RETURNS] Nothing.
*/
{
    unsigned short intensity;
    unsigned int pixel_count;
/*
    static char function_name[] = "cf_random_grey";
*/

    for (pixel_count = 0; pixel_count < num_cells; ++pixel_count)
    {
	intensity = (n_uniform () * MAX_INTENSITY);
	if (reds != NULL) reds[pixel_count * stride] = intensity;
	if (greens != NULL) greens[pixel_count * stride] = intensity;
	if (blues != NULL) blues[pixel_count * stride] = intensity;
    }
}   /*  End Function cf_random_grey  */

/*PUBLIC_FUNCTION*/
void cf_random_pseudocolour (unsigned int num_cells, unsigned short *reds,
			     unsigned short *greens, unsigned short *blues,
			     unsigned int stride, double x, double y,
			     void *var_param)
/*  [SUMMARY] Compute a random pseudocolour colourmap.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values.
    <greens> The green intensity values.
    <blues> The blue intensity values.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <y> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <var_param> A parameter used to compute the colour values. Ignored.
    [RETURNS] Nothing.
*/
{
    unsigned int pixel_count;
/*
    static char function_name[] = "cf_random_pseudocolour";
*/

    /*  Now compute the colours  */
    for (pixel_count = 0; pixel_count < num_cells; ++pixel_count)
    {
	reds[pixel_count * stride] = (n_uniform () * MAX_INTENSITY);
	greens[pixel_count * stride] = (n_uniform () * MAX_INTENSITY);
	blues[pixel_count * stride] = (n_uniform () * MAX_INTENSITY);
    }
}   /*  End Function cf_random_pseudocolour  */

/*PUBLIC_FUNCTION*/
void cf_velocity_compensating_tones (unsigned int num_cells,
				     unsigned short *reds,
				     unsigned short *greens,
				     unsigned short *blues,
				     unsigned int stride, double x, double y,
				     void *var_param)
/*  [SUMMARY] Compute a Velocity (compensating tones) colourmap.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values.
    <greens> The green intensity values.
    <blues> The blue intensity values.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <y> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <var_param> A parameter used to compute the colour values. Ignored.
    [RETURNS] Nothing.
*/
{
    unsigned int pixel_count;
    float intensity;
    static char function_name[] = "cf_velocity_compensating_tones";

    if ( (x < 0.0) || (x > 1.0) || (y < 0.0) || (y > 1.0) )
    {
	(void) fprintf (stderr, "x or y out of range\n");
	a_prog_bug (function_name);
    }
    /*  Now compute the colours  */
    for (pixel_count = 0; pixel_count < num_cells; ++pixel_count)
    {
	intensity = (float) pixel_count / (float) (num_cells - 1);
	reds[pixel_count * stride] = intensity  * MAX_INTENSITY * x;
	greens[pixel_count * stride] = ( (y * MAX_INTENSITY) +
					(1.0 - intensity) *
					(1 - y) * MAX_INTENSITY );
	blues[pixel_count * stride] = ( (y * MAX_INTENSITY) +
				       (1.0 - intensity) *
				       (1 - y) * MAX_INTENSITY );
    }
}   /*  End Function cf_velocity_compensating_tones  */

/*PUBLIC_FUNCTION*/
void cf_compressed_colourmap_3r2g2b (unsigned int num_cells,
				     unsigned short *reds,
				     unsigned short *greens,
				     unsigned short *blues,
				     unsigned int stride, double x, double y,
				     void *var_param)
/*  [SUMMARY] Compute a compressed colourmap.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values.
    <greens> The green intensity values.
    <blues> The blue intensity values.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <y> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <var_param> A parameter used to compute the colour values. Ignored.
    [RETURNS] Nothing.
*/
{
    int red, green, blue;
    unsigned int pixel_count;
    static char function_name[] = "cf_compressed_colourmap_3r2g2b";

    if (num_cells != 128)
    {
	a_func_abort (function_name, "Must have EXACTLY 128 colourcells");
	return;
    }
    /*  Compute the colours  */
    for (pixel_count = 0; pixel_count < num_cells; ++pixel_count)
    {
	red = pixel_count & 0x07;
	green = (pixel_count & 0x18) >> 3;
	blue = (pixel_count & 0x60) >> 5;
	reds[pixel_count * stride] = ( (red * MAX_INTENSITY) / 7 );
	greens[pixel_count * stride] = ( (green * MAX_INTENSITY) / 3 );
	blues[pixel_count * stride] = ( (blue * MAX_INTENSITY) / 3 );
    }
}   /*  End Function cf_compressed_colourmap_3r2g2b  */


/*PUBLIC_FUNCTION*/
void cf_ronekers (unsigned int num_cells, unsigned short *reds,
		  unsigned short *greens, unsigned short *blues,
		  unsigned int stride, double x, double y, void *var_param)
/*  [SUMMARY] Compute a Ron Ekers colourmap.
    <num_cells> The number of colour cells to modify.
    <reds> The red intensity values.
    <greens> The green intensity values.
    <blues> The blue intensity values.
    <stride> The stride (in unsigned shorts) between intensity values.
    <x> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <y> A parameter used to compute the colour values, ranging from 0.0 to 1.0.
    <var_param> A parameter used to compute the colour values. Ignored.
    [RETURNS] Nothing.
*/
{
    int               n, k;
    double            shift, slope, xx, yy;

    if ((x < 0.0) || (x > 1.0) || (y < 0.0) || (y > 1.0))
    {
	shift = 0.5;
	slope = 1.0;
    }
    else
    {
	shift = x;
	slope = tan(1.57*y);
    }
    for (n = 1; n <= num_cells; n++)
    {
	xx = 1.0 / (float) (num_cells+1) * (n);
	yy = slope * (xx-shift) + 0.5;
	if (yy < 0.0)
	{
	    yy = 0.0;
	}
	else
	{
	    if (yy > 1.0)
	    {
		yy = 1.0;
	    }
	}

	k = (yy * 9.0);
	switch( k )
	{			/* which colour */
	  case 0:
	    /* black */
	    reds[(n-1)*stride]   = 0.19608 * MAX_INTENSITY;
	    greens[(n-1)*stride] = 0.19608 * MAX_INTENSITY;
	    blues[(n-1)*stride]  = 0.19608 * MAX_INTENSITY;
	    break;
	  case 1:
	    /* purple */
	    reds[(n-1)*stride]   = 0.47451 * MAX_INTENSITY;
	    greens[(n-1)*stride] = 0;
	    blues[(n-1)*stride]  = 0.60784 * MAX_INTENSITY;
	    break;
	  case 2:
	    /* dark blue */
	    reds[(n-1)*stride]   = 0;
	    greens[(n-1)*stride] = 0;
	    blues[(n-1)*stride]  = 0.78431 * MAX_INTENSITY;
	    break;
	  case 3:
	    /* light blue */
	    reds[(n-1)*stride]   = 0.37255 * MAX_INTENSITY;
	    greens[(n-1)*stride] = 0.65490 * MAX_INTENSITY;
	    blues[(n-1)*stride]  = 0.92549 * MAX_INTENSITY;
	    break;
	  case 4:
	    /* dark green */
	    reds[(n-1)*stride]   = 0;
	    greens[(n-1)*stride] = 0.56863 * MAX_INTENSITY;
	    blues[(n-1)*stride]  = 0;
	    break;
	  case 5:
	    /* light green */
	    reds[(n-1)*stride]   = 0;
	    greens[(n-1)*stride] = 0.96471 * MAX_INTENSITY;
	    blues[(n-1)*stride]  = 0;
	    break;
	  case 6:
	    /* yellow */
	    reds[(n-1)*stride]   = 1.00000 * MAX_INTENSITY;
	    greens[(n-1)*stride] = 1.00000 * MAX_INTENSITY;
	    blues[(n-1)*stride]  = 0;
	    break;
	  case 7:
	    /* orange */
	    reds[(n-1)*stride]   = 1.00000 * MAX_INTENSITY;
	    greens[(n-1)*stride] = 0.69412 * MAX_INTENSITY;
	    blues[(n-1)*stride]  = 0;
	    break;
	  default:
	    /* red */
	    reds[(n-1)*stride]   = 1.00000 * MAX_INTENSITY;
	    greens[(n-1)*stride] = 0;
	    blues[(n-1)*stride]  = 0;
	    break;
	}
    }
}   /*  End Function cf_ronekers  */


/*  Private functions follow  */

static double ef (double xx, double c, double x0)
/*  This routine will calculate the Glynn Rogers function for the 
    stripchart colourmap algorithm.
*/
{
    double tmp;

    tmp = exp ( (double) c * (xx - x0) );
    tmp = tmp / (tmp + 1.0);
    return ( (double) tmp );
}   /*  End Function ef  */

/*LINTLIBRARY*/
/*PREFIX:"cf_"*/
/*  cmap.c

    This code provides simple colourmap generation routines.

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

    This file contains the various utility routines for writing simple colour
    values.


    Written by      Richard Gooch   22-FEB-1993: Copied from
  kimage_cmap_funcs.c  in source/modules/kimage

    Updated by      Richard Gooch   23-FEB-1993: Modified from original code
  and added stripchart functionality.

    Updated by      Richard Gooch   2-MAR-1993: Changed some parameters and
  moved from level1 (c_) to level2.

    Last updated by Richard Gooch   6-AUG-1993: Fixed bug in  cf_greyscale1
  which could cause a divide by zero when x == 0.0


*/

#include <stdio.h>
#include <math.h>
#include <karma_cf.h>
#include <karma_a.h>
#include <karma_n.h>
#include <karma.h>

#define MAX_INTENSITY 65535


/*  Private functions follow  */

static double ef (xx, c, x0)
/*  This routine will calculate the Glynn Rogers function for the 
    stripchart colourmap algorithm.
*/
double xx;
double c;
double x0;
{
    double tmp;

    tmp = exp ( (double) c * (xx - x0) );
    tmp = tmp / (tmp + 1.0);
    return ( (double) tmp );
}   /*  End Function ef  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void cf_greyscale1 (num_cells, reds, greens, blues, stride, x, y, var_param)
/*  This routine will compute a grey scale colourmap and write out the pixel
    colours.
    Parameters are as for cf_stripchart, except var_param, which is not used.
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
double x;
double y;
void *var_param;
{
    unsigned int pixel_count;
    float intensity;
    static char function_name[] = "cf_greyscale1";

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
	reds[pixel_count * stride] = intensity;
	blues[pixel_count * stride] = intensity;
	greens[pixel_count * stride] = intensity;
    }
    for (; pixel_count < num_cells; ++pixel_count)
    {
	intensity = (float) pixel_count / (float) (num_cells - 1) - x;
	intensity = y + (1.0 - y) * intensity / (1.0 - x);
	intensity *= MAX_INTENSITY;
	reds[pixel_count * stride] = intensity;
	blues[pixel_count * stride] = intensity;
	greens[pixel_count * stride] = intensity;
    }
}   /*  End Function cf_greyscale1  */

/*PUBLIC_FUNCTION*/
void cf_greyscale2 (num_cells, reds, greens, blues, stride, x, y, var_param)
/*  This routine will compute a greyscale colourmap using cf_stripchart.
    This map goes: black-grey-white.
    This map uses the Glynn Rogers curvature function.
    Parameters are as for cf_stripchart, except var_param, which is not used.
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
double x;
double y;
void *var_param;
{
    stripchart s;

    s.patternlength=1;

    s.pattern[0][0] = 1;        s.pattern[0][1] = 1;        s.pattern[0][2] = 1;

    cf_stripchart(num_cells,reds,greens,blues, stride,x, y, &s);
}   /*  End Function cf_greyscale2  */

/*PUBLIC_FUNCTION*/
void cf_rainbow1 (num_cells, reds, greens, blues, stride, x, y, var_param)
/*  This routine will compute a colourmap using cf_stripchart.
    This map goes: blue-cyan-green-yellow-red-magenta.
    This map uses the Glynn Rogers curvature function.
    Parameters are as for cf_stripchart, except var_param, which is not used.
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
double x;
double y;
void *var_param;
{
    stripchart s;

    s.patternlength=5;

    s.pattern[0][0] = 0;        s.pattern[0][1] = 1;        s.pattern[0][2] = 99;
    s.pattern[1][0] = 0;        s.pattern[1][1] = 99;       s.pattern[1][2] = -1;
    s.pattern[2][0] = 1;        s.pattern[2][1] = 99;       s.pattern[2][2] = 0;
    s.pattern[3][0] = 99;       s.pattern[3][1] = -1;       s.pattern[3][2] = 0;
    s.pattern[4][0] = 99;       s.pattern[4][1] = 0;        s.pattern[4][2] = 1;

    cf_stripchart(num_cells,reds,greens,blues, stride, x, y, &s);
}   /*  End Function cf_rainbow1  */

/*PUBLIC_FUNCTION*/
void cf_rainbow2 (num_cells, reds, greens, blues, stride, x, y, var_param)
/*  This routine will compute a colourmap using cf_stripchart.
    This map goes: black-blue-cyan-green-yellow-red.
    This map uses the Glynn Rogers curvature function.
    Parameters are as for cf_stripchart, except var_param, which is not used.
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
double x;
double y;
void *var_param;
{
    stripchart s;

    s.patternlength=5;

    s.pattern[0][0] = 0;        s.pattern[0][1] = 0;        s.pattern[0][2] = 1;
    s.pattern[1][0] = 0;        s.pattern[1][1] = 1;        s.pattern[1][2] = 99;
    s.pattern[2][0] = 0;        s.pattern[2][1] = 99;       s.pattern[2][2] = -1;
    s.pattern[3][0] = 1;        s.pattern[3][1] = 99;       s.pattern[3][2] = 0;
    s.pattern[4][0] = 99;       s.pattern[4][1] = -1;       s.pattern[4][2] = 0;

    cf_stripchart(num_cells,reds,greens,blues, stride, x, y, &s);
}   /*  End Function cf_rainbow2  */

/*PUBLIC_FUNCTION*/
void cf_rainbow3 (num_cells, reds, greens, blues, stride, x, y, var_param)
/*  This routine will compute a colourmap using cf_stripchart.
    This map goes: black-blue-cyan-green-yellow-white.
    This map uses the Glynn Rogers curvature function.
    Parameters are as for cf_stripchart, except var_param, which is not used.
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
double x;
double y;
void *var_param;
{
    stripchart s;

    s.patternlength=5;

    s.pattern[0][0] = 0;        s.pattern[0][1] = 0;        s.pattern[0][2] = 1;
    s.pattern[1][0] = 0;        s.pattern[1][1] = 1;        s.pattern[1][2] = 99;
    s.pattern[2][0] = 0;        s.pattern[2][1] = 99;       s.pattern[2][2] = -1;
    s.pattern[3][0] = 1;        s.pattern[3][1] = 99;       s.pattern[3][2] = 0;
    s.pattern[4][0] = 99;       s.pattern[4][1] = 99;       s.pattern[4][2] = 1;

    cf_stripchart(num_cells,reds,greens,blues, stride, x, y, &s);
}   /*  End Function cf_rainbow3  */

/*PUBLIC_FUNCTION*/
void cf_cyclic1 (num_cells, reds, greens, blues, stride, x, y, var_param)
/*  This routine will compute a colourmap using cf_stripchart.
    This map goes: blue-cyan-green-yellow-white.
    This map uses the sine of the Glynn Rogers curvature function.
    Parameters are as for cf_stripchart, except var_param, which is not used.
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
double x;
double y;
void *var_param;
{
    stripchart s;

    s.patternlength=6;

    s.pattern[0][0] = 0;        s.pattern[0][1] = 2;        s.pattern[0][2] = 99;
    s.pattern[1][0] = 0;        s.pattern[1][1] = 99;       s.pattern[1][2] = -2;
    s.pattern[2][0] = 2;        s.pattern[2][1] = 99;       s.pattern[2][2] = 0;
    s.pattern[3][0] = 99;       s.pattern[3][1] = -2;       s.pattern[3][2] = 0;
    s.pattern[4][0] = 99;       s.pattern[4][1] = 0;        s.pattern[4][2] = 2;
    s.pattern[5][0] = -2;       s.pattern[5][1] = 0;        s.pattern[5][2] = 99;

    cf_stripchart (num_cells,reds,greens,blues, stride, x, y, &s);
}   /*  End Function cf_cyclic1  */

/*PUBLIC_FUNCTION*/
void cf_stripchart (num_cells, reds, greens, blues, stride, x, y, chart)
/*  This routine will compute a colourmap using a strip chart, and
    a number of methods for modifying the curvature of the chart.
    The stride (in unsigned shorts) between intensity values in each array
    must be given by  stride  .
    The graph is stored in chart, which should be a stripchart struct * cast
    to a void *.
    The following values are used to determine the shape of the graph:
    0 : graph is at zero for this interval
    99 : graph is at maximum
    1 : graph ramps up from zero to max using Glynn Rogers function
    -1 : graph ramps down from max to zero using Glynn Rogers function
    2 (-2) : graph ramps up (down) using the sine of Glynn Rogers function
    The two parameters used to calculate the shape of the ramping function
    must be given by  x  and  y  .The range for these values is 0.0 to 1.0
    The number of pixel colours to compute must be given by  num_cells  .
    The values are stored in the reds,greens,blues arrays. Values are between
    zero and 65535.
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
double x;
double y;
void *chart;
{
    unsigned int pixel_count;
    double pixel,table,denominator,x0,c,intensity,offset;
    double patnumf,divn,up_intensity,down_intensity;
    double temp;
    int patnum,i,val;
    static char function_name[] = "cf_stripchart";
    stripchart *schart=(stripchart *)chart;

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
	    else printf ("Invalid value in pattern[%d][%d] = %d\n",patnum,i,val);

	    if(i==0) reds[pixel_count * stride]   = temp;
	    if(i==1) greens[pixel_count * stride] = temp;
	    if(i==2) blues[pixel_count * stride]  = temp;
	}
    }
}   /*  End Function cf_stripchart  */

/*PUBLIC_FUNCTION*/
void cf_random_grey (num_cells, reds, greens, blues, stride, x, y, var_param)
/*  This routine will compute a random grey colourmap and will write out the
    pixel colours.
    The number of colour cells to modify must be given by  num_cells  .
    The red intensity values must be pointed to by  reds  .
    The green intensity values must be pointed to by  greens  .
    The blue intensity values must be pointed to by  blues  .
    The stride (in unsigned shorts) between intensity values in each array
    must be given by  stride  .
    The parameters used to compute the colour values must be given by
    x  ,  y  and  var_param  .
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
double x;
double y;
void *var_param;
{
    unsigned short intensity;
    unsigned int pixel_count;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "cf_random_grey";

    for (pixel_count = 0; pixel_count < num_cells; ++pixel_count)
    {
	intensity = (n_uniform () * MAX_INTENSITY);
	reds[pixel_count * stride] = intensity;
	greens[pixel_count * stride] = intensity;
	blues[pixel_count * stride] = intensity;
    }
}   /*  End Function cf_random_grey  */

/*PUBLIC_FUNCTION*/
void cf_random_pseudocolour (num_cells, reds, greens, blues, stride,
			     x, y, var_param)
/*  This routine will compute a random grey pseudocolour and will write out the
    pixel colours.
    The number of colour cells to modify must be given by  num_cells  .
    The red intensity values must be pointed to by  reds  .
    The green intensity values must be pointed to by  greens  .
    The blue intensity values must be pointed to by  blues  .
    The stride (in unsigned shorts) between intensity values in each array
    must be given by  stride  .
    The parameters used to compute the colour values must be given by
    x  ,  y  and  var_param  .
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
double x;
double y;
void *var_param;
{
    unsigned int pixel_count;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "cf_random_pseudocolour";

    /*  Now compute the colours  */
    for (pixel_count = 0; pixel_count < num_cells; ++pixel_count)
    {
	reds[pixel_count * stride] = (n_uniform () * MAX_INTENSITY);
	greens[pixel_count * stride] = (n_uniform () * MAX_INTENSITY);
	blues[pixel_count * stride] = (n_uniform () * MAX_INTENSITY);
    }
}   /*  End Function cf_random_pseudocolour  */

/*PUBLIC_FUNCTION*/
void cf_velocity_compensating_tones (num_cells, reds, greens, blues, stride,
				     x, y, var_param)
/*  This routine will compute a Velocity (compensating tones) colourmap and
    will write out the pixel colours.
    The number of colour cells to modify must be given by  num_cells  .
    The red intensity values must be pointed to by  reds  .
    The green intensity values must be pointed to by  greens  .
    The blue intensity values must be pointed to by  blues  .
    The stride (in unsigned shorts) between intensity values in each array
    must be given by  stride  .
    The parameters used to compute the colour values must be given by
    x  ,  y  and  var_param  .
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
double x;
double y;
void *var_param;
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
void cf_compressed_colourmap_3r2g2b (num_cells, reds, greens, blues, stride,
				     x, y, var_param)
/*  This function will create a compressed colourmap.
    The number of colour cells to modify must be given by  num_cells  .
    The red intensity values must be pointed to by  reds  .
    The green intensity values must be pointed to by  greens  .
    The blue intensity values must be pointed to by  blues  .
    The stride (in unsigned shorts) between intensity values in each array
    must be given by  stride  .
    The parameters used to compute the colour values must be given by
    x  ,  y  and  var_param  .
    The routine returns nothing.
*/
unsigned int num_cells;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
double x;
double y;
void *var_param;
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

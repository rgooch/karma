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

    Updated by      Richard Gooch   6-AUG-1993: Fixed bug in  cf_greyscale1
  which could cause a divide by zero when x == 0.0

    Updated by      Richard Gooch   12-NOV-1993: Added colourmap functions
  submitted by Tom Oosterlo.

    Last updated by Richard Gooch   22-NOV-1993: Added colourmap function
  submitted by Jeanne Young.


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

/*  Colourmaps submitted by Tom  */
#define LUT_SIZE      256

typedef struct colour_struct
{
    float   red;
    float   green;
    float   blue;
} Colour_struct;


static Colour_struct background[LUT_SIZE] =
{
    0.00000, 0.00000, 0.00000 ,
    0.01587, 0.01587, 0.01587 ,
    0.03174, 0.03174, 0.03174 ,
    0.04761, 0.04761, 0.04761 ,
    0.06348, 0.06348, 0.06348 ,
    0.07935, 0.07935, 0.07935 ,
    0.09522, 0.09522, 0.09522 ,
    0.11109, 0.11109, 0.11109 ,
    0.12696, 0.12696, 0.12696 ,
    0.14283, 0.14283, 0.14283 ,
    0.15870, 0.15870, 0.15870 ,
    0.17457, 0.17457, 0.17457 ,
    0.19044, 0.19044, 0.19044 ,
    0.20631, 0.20631, 0.20631 ,
    0.22218, 0.22218, 0.22218 ,
    0.23805, 0.23805, 0.23805 ,
    0.25392, 0.25392, 0.25392 ,
    0.26979, 0.26979, 0.26979 ,
    0.28566, 0.28566, 0.28566 ,
    0.30153, 0.30153, 0.30153 ,
    0.31740, 0.31740, 0.31740 ,
    0.33327, 0.33327, 0.33327 ,
    0.34914, 0.34914, 0.34914 ,
    0.36501, 0.36501, 0.36501 ,
    0.38088, 0.38088, 0.38088 ,
    0.39675, 0.39675, 0.39675 ,
    0.41262, 0.41262, 0.41262 ,
    0.42849, 0.42849, 0.42849 ,
    0.44436, 0.44436, 0.44436 ,
    0.46023, 0.46023, 0.46023 ,
    0.47610, 0.47610, 0.47610 ,
    0.49197, 0.49197, 0.49197 ,
    0.50784, 0.50784, 0.50784 ,
    0.52371, 0.52371, 0.52371 ,
    0.53958, 0.53958, 0.53958 ,
    0.55545, 0.55545, 0.55545 ,
    0.57132, 0.57132, 0.57132 ,
    0.58719, 0.58719, 0.58719 ,
    0.60306, 0.60306, 0.60306 ,
    0.61893, 0.61893, 0.61893 ,
    0.63480, 0.63480, 0.63480 ,
    0.65067, 0.65067, 0.65067 ,
    0.66654, 0.66654, 0.66654 ,
    0.68241, 0.68241, 0.68241 ,
    0.69828, 0.69828, 0.69828 ,
    0.71415, 0.71415, 0.71415 ,
    0.73002, 0.73002, 0.73002 ,
    0.74589, 0.74589, 0.74589 ,
    0.76176, 0.76176, 0.76176 ,
    0.77763, 0.77763, 0.77763 ,
    0.79350, 0.79350, 0.79350 ,
    0.80937, 0.80937, 0.80937 ,
    0.82524, 0.82524, 0.82524 ,
    0.84111, 0.84111, 0.84111 ,
    0.85698, 0.85698, 0.85698 ,
    0.87285, 0.87285, 0.87285 ,
    0.88872, 0.88872, 0.88872 ,
    0.90459, 0.90459, 0.90459 ,
    0.92046, 0.92046, 0.92046 ,
    0.93633, 0.93633, 0.93633 ,
    0.95220, 0.95220, 0.95220 ,
    0.96807, 0.96807, 0.96807 ,
    0.98394, 0.98394, 0.98394 ,
    0.99981, 0.99981, 0.99981 ,
    0.00000, 0.00000, 0.99981 ,
    0.00000, 0.01587, 0.98394 ,
    0.00000, 0.03174, 0.96807 ,
    0.00000, 0.04761, 0.95220 ,
    0.00000, 0.06348, 0.93633 ,
    0.00000, 0.07935, 0.92046 ,
    0.00000, 0.09522, 0.90459 ,
    0.00000, 0.11109, 0.88872 ,
    0.00000, 0.12696, 0.87285 ,
    0.00000, 0.14283, 0.85698 ,
    0.00000, 0.15870, 0.84111 ,
    0.00000, 0.17457, 0.82524 ,
    0.00000, 0.19044, 0.80937 ,
    0.00000, 0.20631, 0.79350 ,
    0.00000, 0.22218, 0.77763 ,
    0.00000, 0.23805, 0.76176 ,
    0.00000, 0.25392, 0.74589 ,
    0.00000, 0.26979, 0.73002 ,
    0.00000, 0.28566, 0.71415 ,
    0.00000, 0.30153, 0.69828 ,
    0.00000, 0.31740, 0.68241 ,
    0.00000, 0.33327, 0.66654 ,
    0.00000, 0.34914, 0.65067 ,
    0.00000, 0.36501, 0.63480 ,
    0.00000, 0.38088, 0.61893 ,
    0.00000, 0.39675, 0.60306 ,
    0.00000, 0.41262, 0.58719 ,
    0.00000, 0.42849, 0.57132 ,
    0.00000, 0.44436, 0.55545 ,
    0.00000, 0.46023, 0.53958 ,
    0.00000, 0.47610, 0.52371 ,
    0.00000, 0.49197, 0.50784 ,
    0.00000, 0.50784, 0.49197 ,
    0.00000, 0.52371, 0.47610 ,
    0.00000, 0.53958, 0.46023 ,
    0.00000, 0.55545, 0.44436 ,
    0.00000, 0.57132, 0.42849 ,
    0.00000, 0.58719, 0.41262 ,
    0.00000, 0.60306, 0.39675 ,
    0.00000, 0.61893, 0.38088 ,
    0.00000, 0.63480, 0.36501 ,
    0.00000, 0.65067, 0.34914 ,
    0.00000, 0.66654, 0.33327 ,
    0.00000, 0.68241, 0.31740 ,
    0.00000, 0.69828, 0.30153 ,
    0.00000, 0.71415, 0.28566 ,
    0.00000, 0.73002, 0.26979 ,
    0.00000, 0.74589, 0.25392 ,
    0.00000, 0.76176, 0.23805 ,
    0.00000, 0.77763, 0.22218 ,
    0.00000, 0.79350, 0.20631 ,
    0.00000, 0.80937, 0.19044 ,
    0.00000, 0.82524, 0.17457 ,
    0.00000, 0.84111, 0.15870 ,
    0.00000, 0.85698, 0.14283 ,
    0.00000, 0.87285, 0.12696 ,
    0.00000, 0.88872, 0.11109 ,
    0.00000, 0.90459, 0.09522 ,
    0.00000, 0.92046, 0.07935 ,
    0.00000, 0.93633, 0.06348 ,
    0.00000, 0.95220, 0.04761 ,
    0.00000, 0.96807, 0.03174 ,
    0.00000, 0.98394, 0.01587 ,
    0.00000, 0.99981, 0.00000 ,
    0.00000, 1.00000, 0.00000 ,
    0.01587, 1.00000, 0.00000 ,
    0.03174, 1.00000, 0.00000 ,
    0.04761, 1.00000, 0.00000 ,
    0.06348, 1.00000, 0.00000 ,
    0.07935, 1.00000, 0.00000 ,
    0.09522, 1.00000, 0.00000 ,
    0.11109, 1.00000, 0.00000 ,
    0.12696, 1.00000, 0.00000 ,
    0.14283, 1.00000, 0.00000 ,
    0.15870, 1.00000, 0.00000 ,
    0.17457, 1.00000, 0.00000 ,
    0.19044, 1.00000, 0.00000 ,
    0.20631, 1.00000, 0.00000 ,
    0.22218, 1.00000, 0.00000 ,
    0.23805, 1.00000, 0.00000 ,
    0.25392, 1.00000, 0.00000 ,
    0.26979, 1.00000, 0.00000 ,
    0.28566, 1.00000, 0.00000 ,
    0.30153, 1.00000, 0.00000 ,
    0.31740, 1.00000, 0.00000 ,
    0.33327, 1.00000, 0.00000 ,
    0.34914, 1.00000, 0.00000 ,
    0.36501, 1.00000, 0.00000 ,
    0.38088, 1.00000, 0.00000 ,
    0.39675, 1.00000, 0.00000 ,
    0.41262, 1.00000, 0.00000 ,
    0.42849, 1.00000, 0.00000 ,
    0.44436, 1.00000, 0.00000 ,
    0.46023, 1.00000, 0.00000 ,
    0.47610, 1.00000, 0.00000 ,
    0.49197, 1.00000, 0.00000 ,
    0.50784, 1.00000, 0.00000 ,
    0.52371, 1.00000, 0.00000 ,
    0.53958, 1.00000, 0.00000 ,
    0.55545, 1.00000, 0.00000 ,
    0.57132, 1.00000, 0.00000 ,
    0.58719, 1.00000, 0.00000 ,
    0.60306, 1.00000, 0.00000 ,
    0.61893, 1.00000, 0.00000 ,
    0.63480, 1.00000, 0.00000 ,
    0.65067, 1.00000, 0.00000 ,
    0.66654, 1.00000, 0.00000 ,
    0.68241, 1.00000, 0.00000 ,
    0.69828, 1.00000, 0.00000 ,
    0.71415, 1.00000, 0.00000 ,
    0.73002, 1.00000, 0.00000 ,
    0.74589, 1.00000, 0.00000 ,
    0.76176, 1.00000, 0.00000 ,
    0.77763, 1.00000, 0.00000 ,
    0.79350, 1.00000, 0.00000 ,
    0.80937, 1.00000, 0.00000 ,
    0.82524, 1.00000, 0.00000 ,
    0.84111, 1.00000, 0.00000 ,
    0.85698, 1.00000, 0.00000 ,
    0.87285, 1.00000, 0.00000 ,
    0.88872, 1.00000, 0.00000 ,
    0.90459, 1.00000, 0.00000 ,
    0.92046, 1.00000, 0.00000 ,
    0.93633, 1.00000, 0.00000 ,
    0.95220, 1.00000, 0.00000 ,
    0.96807, 1.00000, 0.00000 ,
    0.98394, 1.00000, 0.00000 ,
    0.99981, 1.00000, 0.00000 ,
    1.00000, 0.99981, 0.00000 ,
    1.00000, 0.98394, 0.00000 ,
    1.00000, 0.96807, 0.00000 ,
    1.00000, 0.95220, 0.00000 ,
    1.00000, 0.93633, 0.00000 ,
    1.00000, 0.92046, 0.00000 ,
    1.00000, 0.90459, 0.00000 ,
    1.00000, 0.88872, 0.00000 ,
    1.00000, 0.87285, 0.00000 ,
    1.00000, 0.85698, 0.00000 ,
    1.00000, 0.84111, 0.00000 ,
    1.00000, 0.82524, 0.00000 ,
    1.00000, 0.80937, 0.00000 ,
    1.00000, 0.79350, 0.00000 ,
    1.00000, 0.77763, 0.00000 ,
    1.00000, 0.76176, 0.00000 ,
    1.00000, 0.74589, 0.00000 ,
    1.00000, 0.73002, 0.00000 ,
    1.00000, 0.71415, 0.00000 ,
    1.00000, 0.69828, 0.00000 ,
    1.00000, 0.68241, 0.00000 ,
    1.00000, 0.66654, 0.00000 ,
    1.00000, 0.65067, 0.00000 ,
    1.00000, 0.63480, 0.00000 ,
    1.00000, 0.61893, 0.00000 ,
    1.00000, 0.60306, 0.00000 ,
    1.00000, 0.58719, 0.00000 ,
    1.00000, 0.57132, 0.00000 ,
    1.00000, 0.55545, 0.00000 ,
    1.00000, 0.53958, 0.00000 ,
    1.00000, 0.52371, 0.00000 ,
    1.00000, 0.50784, 0.00000 ,
    1.00000, 0.49197, 0.00000 ,
    1.00000, 0.47610, 0.00000 ,
    1.00000, 0.46023, 0.00000 ,
    1.00000, 0.44436, 0.00000 ,
    1.00000, 0.42849, 0.00000 ,
    1.00000, 0.41262, 0.00000 ,
    1.00000, 0.39675, 0.00000 ,
    1.00000, 0.38088, 0.00000 ,
    1.00000, 0.36501, 0.00000 ,
    1.00000, 0.34914, 0.00000 ,
    1.00000, 0.33327, 0.00000 ,
    1.00000, 0.31740, 0.00000 ,
    1.00000, 0.30153, 0.00000 ,
    1.00000, 0.28566, 0.00000 ,
    1.00000, 0.26979, 0.00000 ,
    1.00000, 0.25392, 0.00000 ,
    1.00000, 0.23805, 0.00000 ,
    1.00000, 0.22218, 0.00000 ,
    1.00000, 0.20631, 0.00000 ,
    1.00000, 0.19044, 0.00000 ,
    1.00000, 0.17457, 0.00000 ,
    1.00000, 0.15870, 0.00000 ,
    1.00000, 0.14283, 0.00000 ,
    1.00000, 0.12696, 0.00000 ,
    1.00000, 0.11109, 0.00000 ,
    1.00000, 0.09522, 0.00000 ,
    1.00000, 0.07935, 0.00000 ,
    1.00000, 0.06348, 0.00000 ,
    1.00000, 0.04761, 0.00000 ,
    1.00000, 0.03174, 0.00000 ,
    1.00000, 0.01587, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
};

static Colour_struct heat[LUT_SIZE] =
{
    0.00000, 0.00000, 0.00000 ,
    0.01176, 0.00392, 0.00000 ,
    0.02353, 0.00784, 0.00000 ,
    0.03529, 0.01176, 0.00000 ,
    0.04706, 0.01569, 0.00000 ,
    0.05882, 0.01961, 0.00000 ,
    0.07059, 0.02353, 0.00000 ,
    0.08235, 0.02745, 0.00000 ,
    0.09412, 0.03137, 0.00000 ,
    0.10588, 0.03529, 0.00000 ,
    0.11765, 0.03922, 0.00000 ,
    0.12941, 0.04314, 0.00000 ,
    0.14118, 0.04706, 0.00000 ,
    0.15294, 0.05098, 0.00000 ,
    0.16471, 0.05490, 0.00000 ,
    0.17647, 0.05882, 0.00000 ,
    0.18824, 0.06275, 0.00000 ,
    0.20000, 0.06667, 0.00000 ,
    0.21176, 0.07059, 0.00000 ,
    0.22353, 0.07451, 0.00000 ,
    0.23529, 0.07843, 0.00000 ,
    0.24706, 0.08235, 0.00000 ,
    0.25882, 0.08627, 0.00000 ,
    0.27059, 0.09020, 0.00000 ,
    0.28235, 0.09412, 0.00000 ,
    0.29412, 0.09804, 0.00000 ,
    0.30588, 0.10196, 0.00000 ,
    0.31765, 0.10588, 0.00000 ,
    0.32941, 0.10980, 0.00000 ,
    0.34118, 0.11373, 0.00000 ,
    0.35294, 0.11765, 0.00000 ,
    0.36471, 0.12157, 0.00000 ,
    0.37647, 0.12549, 0.00000 ,
    0.38824, 0.12941, 0.00000 ,
    0.40000, 0.13333, 0.00000 ,
    0.41176, 0.13725, 0.00000 ,
    0.42353, 0.14118, 0.00000 ,
    0.43529, 0.14510, 0.00000 ,
    0.44706, 0.14902, 0.00000 ,
    0.45882, 0.15294, 0.00000 ,
    0.47059, 0.15686, 0.00000 ,
    0.48235, 0.16078, 0.00000 ,
    0.49412, 0.16471, 0.00000 ,
    0.50588, 0.16863, 0.00000 ,
    0.51765, 0.17255, 0.00000 ,
    0.52941, 0.17647, 0.00000 ,
    0.54118, 0.18039, 0.00000 ,
    0.55294, 0.18431, 0.00000 ,
    0.56471, 0.18824, 0.00000 ,
    0.57647, 0.19216, 0.00000 ,
    0.58824, 0.19608, 0.00000 ,
    0.60000, 0.20000, 0.00000 ,
    0.61176, 0.20392, 0.00000 ,
    0.62353, 0.20784, 0.00000 ,
    0.63529, 0.21176, 0.00000 ,
    0.64706, 0.21569, 0.00000 ,
    0.65882, 0.21961, 0.00000 ,
    0.67059, 0.22353, 0.00000 ,
    0.68235, 0.22745, 0.00000 ,
    0.69412, 0.23137, 0.00000 ,
    0.70588, 0.23529, 0.00000 ,
    0.71765, 0.23922, 0.00000 ,
    0.72941, 0.24314, 0.00000 ,
    0.74118, 0.24706, 0.00000 ,
    0.75294, 0.25098, 0.00000 ,
    0.76471, 0.25490, 0.00000 ,
    0.77647, 0.25882, 0.00000 ,
    0.78824, 0.26275, 0.00000 ,
    0.80000, 0.26667, 0.00000 ,
    0.81176, 0.27059, 0.00000 ,
    0.82353, 0.27451, 0.00000 ,
    0.83529, 0.27843, 0.00000 ,
    0.84706, 0.28235, 0.00000 ,
    0.85882, 0.28627, 0.00000 ,
    0.87059, 0.29020, 0.00000 ,
    0.88235, 0.29412, 0.00000 ,
    0.89412, 0.29804, 0.00000 ,
    0.90588, 0.30196, 0.00000 ,
    0.91765, 0.30588, 0.00000 ,
    0.92941, 0.30980, 0.00000 ,
    0.94118, 0.31373, 0.00000 ,
    0.95294, 0.31765, 0.00000 ,
    0.96471, 0.32157, 0.00000 ,
    0.97647, 0.32549, 0.00000 ,
    0.98824, 0.32941, 0.00000 ,
    1.00000, 0.33333, 0.00000 ,
    1.00000, 0.33725, 0.00000 ,
    1.00000, 0.34118, 0.00000 ,
    1.00000, 0.34510, 0.00000 ,
    1.00000, 0.34902, 0.00000 ,
    1.00000, 0.35294, 0.00000 ,
    1.00000, 0.35686, 0.00000 ,
    1.00000, 0.36078, 0.00000 ,
    1.00000, 0.36471, 0.00000 ,
    1.00000, 0.36863, 0.00000 ,
    1.00000, 0.37255, 0.00000 ,
    1.00000, 0.37647, 0.00000 ,
    1.00000, 0.38039, 0.00000 ,
    1.00000, 0.38431, 0.00000 ,
    1.00000, 0.38824, 0.00000 ,
    1.00000, 0.39216, 0.00000 ,
    1.00000, 0.39608, 0.00000 ,
    1.00000, 0.40000, 0.00000 ,
    1.00000, 0.40392, 0.00000 ,
    1.00000, 0.40784, 0.00000 ,
    1.00000, 0.41176, 0.00000 ,
    1.00000, 0.41569, 0.00000 ,
    1.00000, 0.41961, 0.00000 ,
    1.00000, 0.42353, 0.00000 ,
    1.00000, 0.42745, 0.00000 ,
    1.00000, 0.43137, 0.00000 ,
    1.00000, 0.43529, 0.00000 ,
    1.00000, 0.43922, 0.00000 ,
    1.00000, 0.44314, 0.00000 ,
    1.00000, 0.44706, 0.00000 ,
    1.00000, 0.45098, 0.00000 ,
    1.00000, 0.45490, 0.00000 ,
    1.00000, 0.45882, 0.00000 ,
    1.00000, 0.46275, 0.00000 ,
    1.00000, 0.46667, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.47451, 0.00000 ,
    1.00000, 0.47843, 0.00000 ,
    1.00000, 0.48235, 0.00000 ,
    1.00000, 0.48627, 0.00000 ,
    1.00000, 0.49020, 0.00000 ,
    1.00000, 0.49412, 0.00000 ,
    1.00000, 0.49804, 0.00000 ,
    1.00000, 0.50196, 0.00000 ,
    1.00000, 0.50588, 0.00000 ,
    1.00000, 0.50980, 0.00000 ,
    1.00000, 0.51373, 0.00000 ,
    1.00000, 0.51765, 0.00000 ,
    1.00000, 0.52157, 0.00000 ,
    1.00000, 0.52549, 0.00000 ,
    1.00000, 0.52941, 0.00000 ,
    1.00000, 0.53333, 0.00000 ,
    1.00000, 0.53725, 0.00000 ,
    1.00000, 0.54118, 0.00000 ,
    1.00000, 0.54510, 0.00000 ,
    1.00000, 0.54902, 0.00000 ,
    1.00000, 0.55294, 0.00000 ,
    1.00000, 0.55686, 0.00000 ,
    1.00000, 0.56078, 0.00000 ,
    1.00000, 0.56471, 0.00000 ,
    1.00000, 0.56863, 0.00000 ,
    1.00000, 0.57255, 0.00000 ,
    1.00000, 0.57647, 0.00000 ,
    1.00000, 0.58039, 0.00000 ,
    1.00000, 0.58431, 0.00000 ,
    1.00000, 0.58824, 0.00000 ,
    1.00000, 0.59216, 0.00000 ,
    1.00000, 0.59608, 0.00000 ,
    1.00000, 0.60000, 0.00000 ,
    1.00000, 0.60392, 0.00000 ,
    1.00000, 0.60784, 0.00000 ,
    1.00000, 0.61176, 0.00000 ,
    1.00000, 0.61569, 0.00000 ,
    1.00000, 0.61961, 0.00000 ,
    1.00000, 0.62353, 0.00000 ,
    1.00000, 0.62745, 0.00000 ,
    1.00000, 0.63137, 0.00000 ,
    1.00000, 0.63529, 0.00000 ,
    1.00000, 0.63922, 0.00000 ,
    1.00000, 0.64314, 0.00000 ,
    1.00000, 0.64706, 0.00000 ,
    1.00000, 0.65098, 0.01176 ,
    1.00000, 0.65490, 0.02353 ,
    1.00000, 0.65882, 0.03529 ,
    1.00000, 0.66275, 0.04706 ,
    1.00000, 0.66667, 0.05882 ,
    1.00000, 0.67059, 0.07059 ,
    1.00000, 0.67451, 0.08235 ,
    1.00000, 0.67843, 0.09412 ,
    1.00000, 0.68235, 0.10588 ,
    1.00000, 0.68627, 0.11765 ,
    1.00000, 0.69020, 0.12941 ,
    1.00000, 0.69412, 0.14118 ,
    1.00000, 0.69804, 0.15294 ,
    1.00000, 0.70196, 0.16471 ,
    1.00000, 0.70588, 0.17647 ,
    1.00000, 0.70980, 0.18824 ,
    1.00000, 0.71373, 0.20000 ,
    1.00000, 0.71765, 0.21176 ,
    1.00000, 0.72157, 0.22353 ,
    1.00000, 0.72549, 0.23529 ,
    1.00000, 0.72941, 0.24706 ,
    1.00000, 0.73333, 0.25882 ,
    1.00000, 0.73725, 0.27059 ,
    1.00000, 0.74118, 0.28235 ,
    1.00000, 0.74510, 0.29412 ,
    1.00000, 0.74902, 0.30588 ,
    1.00000, 0.75294, 0.31765 ,
    1.00000, 0.75686, 0.32941 ,
    1.00000, 0.76078, 0.34118 ,
    1.00000, 0.76471, 0.35294 ,
    1.00000, 0.76863, 0.36471 ,
    1.00000, 0.77255, 0.37647 ,
    1.00000, 0.77647, 0.38824 ,
    1.00000, 0.78039, 0.40000 ,
    1.00000, 0.78431, 0.41176 ,
    1.00000, 0.78824, 0.42353 ,
    1.00000, 0.79216, 0.43529 ,
    1.00000, 0.79608, 0.44706 ,
    1.00000, 0.80000, 0.45882 ,
    1.00000, 0.80392, 0.47059 ,
    1.00000, 0.80784, 0.48235 ,
    1.00000, 0.81176, 0.49412 ,
    1.00000, 0.81569, 0.50588 ,
    1.00000, 0.81961, 0.51765 ,
    1.00000, 0.82353, 0.52941 ,
    1.00000, 0.82745, 0.54118 ,
    1.00000, 0.83137, 0.55294 ,
    1.00000, 0.83529, 0.56471 ,
    1.00000, 0.83922, 0.57647 ,
    1.00000, 0.84314, 0.58824 ,
    1.00000, 0.84706, 0.60000 ,
    1.00000, 0.85098, 0.61176 ,
    1.00000, 0.85490, 0.62353 ,
    1.00000, 0.85882, 0.63529 ,
    1.00000, 0.86275, 0.64706 ,
    1.00000, 0.86667, 0.65882 ,
    1.00000, 0.87059, 0.67059 ,
    1.00000, 0.87451, 0.68235 ,
    1.00000, 0.87843, 0.69412 ,
    1.00000, 0.88235, 0.70588 ,
    1.00000, 0.88627, 0.71765 ,
    1.00000, 0.89020, 0.72941 ,
    1.00000, 0.89412, 0.74118 ,
    1.00000, 0.89804, 0.75294 ,
    1.00000, 0.90196, 0.76471 ,
    1.00000, 0.90588, 0.77647 ,
    1.00000, 0.90980, 0.78824 ,
    1.00000, 0.91373, 0.80000 ,
    1.00000, 0.91765, 0.81176 ,
    1.00000, 0.92157, 0.82353 ,
    1.00000, 0.92549, 0.83529 ,
    1.00000, 0.92941, 0.84706 ,
    1.00000, 0.93333, 0.85882 ,
    1.00000, 0.93725, 0.87059 ,
    1.00000, 0.94118, 0.88235 ,
    1.00000, 0.94510, 0.89412 ,
    1.00000, 0.94902, 0.90588 ,
    1.00000, 0.95294, 0.91765 ,
    1.00000, 0.95686, 0.92941 ,
    1.00000, 0.96078, 0.94118 ,
    1.00000, 0.96471, 0.95294 ,
    1.00000, 0.96863, 0.96471 ,
    1.00000, 0.97255, 0.97647 ,
    1.00000, 0.97647, 0.98824 ,
    1.00000, 0.98039, 1.00000 ,
    1.00000, 0.98431, 1.00000 ,
    1.00000, 0.98824, 1.00000 ,
    1.00000, 0.99216, 1.00000 ,
    1.00000, 0.99608, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
};

static Colour_struct isophot[LUT_SIZE] =
{
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.03922 ,
    0.00000, 0.00000, 0.07843 ,
    0.00000, 0.00000, 0.11765 ,
    0.00000, 0.00000, 0.15686 ,
    0.00000, 0.00000, 0.19608 ,
    0.00000, 0.00000, 0.23529 ,
    0.00000, 0.00000, 0.27843 ,
    0.00000, 0.00000, 0.31765 ,
    0.00000, 0.00000, 0.35686 ,
    0.00000, 0.00000, 0.39608 ,
    0.00000, 0.00000, 0.43529 ,
    0.00000, 0.00000, 0.47451 ,
    0.00000, 0.00000, 0.51765 ,
    0.00000, 0.00000, 0.55686 ,
    0.00000, 0.00000, 0.59608 ,
    0.00000, 0.00000, 0.63529 ,
    0.00000, 0.00000, 0.67451 ,
    0.00000, 0.00000, 0.71765 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    0.00000, 0.00000, 0.87843 ,
    0.00000, 0.00000, 0.91765 ,
    0.00000, 0.00000, 0.95686 ,
    0.00000, 0.00000, 1.00000 ,
    0.00000, 0.03137, 1.00000 ,
    0.00000, 0.06275, 1.00000 ,
    0.00000, 0.09412, 1.00000 ,
    0.00000, 0.12549, 1.00000 ,
    0.00000, 0.15686, 1.00000 ,
    0.00000, 0.18824, 1.00000 ,
    0.00000, 0.21961, 1.00000 ,
    0.00000, 0.25490, 1.00000 ,
    0.00000, 0.28627, 1.00000 ,
    0.00000, 0.31765, 1.00000 ,
    0.00000, 0.34902, 1.00000 ,
    0.00000, 0.38039, 1.00000 ,
    0.00000, 0.41176, 1.00000 ,
    0.00000, 0.44314, 1.00000 ,
    0.00000, 0.47843, 1.00000 ,
    0.00000, 0.49804, 1.00000 ,
    0.00000, 0.51765, 1.00000 ,
    0.00000, 0.53725, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    0.00000, 0.61961, 1.00000 ,
    0.00000, 0.63922, 1.00000 ,
    0.00000, 0.65882, 1.00000 ,
    0.00000, 0.67843, 1.00000 ,
    0.00000, 0.70196, 1.00000 ,
    0.00000, 0.72157, 1.00000 ,
    0.00000, 0.74118, 1.00000 ,
    0.00000, 0.76078, 1.00000 ,
    0.00000, 0.78431, 1.00000 ,
    0.00000, 0.79608, 1.00000 ,
    0.00000, 0.81176, 1.00000 ,
    0.00000, 0.82353, 1.00000 ,
    0.00000, 0.83922, 1.00000 ,
    0.00000, 0.85490, 1.00000 ,
    0.00000, 0.86667, 1.00000 ,
    0.00000, 0.88235, 1.00000 ,
    0.00000, 0.89412, 1.00000 ,
    0.00000, 0.90980, 1.00000 ,
    0.00000, 0.92549, 1.00000 ,
    0.00000, 0.93725, 1.00000 ,
    0.00000, 0.95294, 1.00000 ,
    0.00000, 0.96863, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    0.00000, 1.00000, 0.96078 ,
    0.00000, 1.00000, 0.94118 ,
    0.00000, 1.00000, 0.92157 ,
    0.00000, 1.00000, 0.90196 ,
    0.00000, 1.00000, 0.88235 ,
    0.00000, 1.00000, 0.86275 ,
    0.00000, 1.00000, 0.84314 ,
    0.00000, 1.00000, 0.82353 ,
    0.00000, 1.00000, 0.80392 ,
    0.00000, 1.00000, 0.78431 ,
    0.00000, 1.00000, 0.76471 ,
    0.00000, 1.00000, 0.74510 ,
    0.00000, 1.00000, 0.72549 ,
    0.00000, 1.00000, 0.70588 ,
    0.00000, 1.00000, 0.65490 ,
    0.00000, 1.00000, 0.60784 ,
    0.00000, 1.00000, 0.56078 ,
    0.00000, 1.00000, 0.51373 ,
    0.00000, 1.00000, 0.46667 ,
    0.00000, 1.00000, 0.41961 ,
    0.00000, 1.00000, 0.37255 ,
    0.00000, 1.00000, 0.32549 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    0.00000, 1.00000, 0.13725 ,
    0.00000, 1.00000, 0.09020 ,
    0.00000, 1.00000, 0.04314 ,
    0.00000, 1.00000, 0.00000 ,
    0.04706, 1.00000, 0.00000 ,
    0.09412, 1.00000, 0.00000 ,
    0.14118, 1.00000, 0.00000 ,
    0.18824, 1.00000, 0.00000 ,
    0.23529, 1.00000, 0.00000 ,
    0.28235, 1.00000, 0.00000 ,
    0.32941, 1.00000, 0.00000 ,
    0.37647, 1.00000, 0.00000 ,
    0.42353, 1.00000, 0.00000 ,
    0.47059, 1.00000, 0.00000 ,
    0.51765, 1.00000, 0.00000 ,
    0.56471, 1.00000, 0.00000 ,
    0.61176, 1.00000, 0.00000 ,
    0.65882, 1.00000, 0.00000 ,
    0.70588, 1.00000, 0.00000 ,
    0.72549, 1.00000, 0.00000 ,
    0.74510, 1.00000, 0.00000 ,
    0.76471, 1.00000, 0.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    0.84314, 1.00000, 0.00000 ,
    0.86275, 1.00000, 0.00000 ,
    0.88235, 1.00000, 0.00000 ,
    0.90196, 1.00000, 0.00000 ,
    0.92157, 1.00000, 0.00000 ,
    0.94118, 1.00000, 0.00000 ,
    0.96078, 1.00000, 0.00000 ,
    0.98039, 1.00000, 0.00000 ,
    1.00000, 1.00000, 0.00000 ,
    0.99608, 0.98039, 0.00000 ,
    0.99608, 0.96078, 0.00000 ,
    0.99608, 0.94118, 0.00000 ,
    0.99608, 0.92549, 0.00000 ,
    0.99216, 0.90588, 0.00000 ,
    0.99216, 0.88627, 0.00000 ,
    0.99216, 0.87059, 0.00000 ,
    0.99216, 0.85098, 0.00000 ,
    0.98824, 0.83137, 0.00000 ,
    0.98824, 0.81569, 0.00000 ,
    0.98824, 0.79608, 0.00000 ,
    0.98824, 0.77647, 0.00000 ,
    0.98824, 0.76078, 0.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    0.98824, 0.69020, 0.00000 ,
    0.98824, 0.67059, 0.00000 ,
    0.98824, 0.65490, 0.00000 ,
    0.98824, 0.63922, 0.00000 ,
    0.98824, 0.61961, 0.00000 ,
    0.99216, 0.60392, 0.00000 ,
    0.99216, 0.58824, 0.00000 ,
    0.99216, 0.56863, 0.00000 ,
    0.99216, 0.55294, 0.00000 ,
    0.99608, 0.53725, 0.00000 ,
    0.99608, 0.51765, 0.00000 ,
    0.99608, 0.50196, 0.00000 ,
    0.99608, 0.48627, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.43529, 0.00000 ,
    1.00000, 0.40392, 0.00000 ,
    1.00000, 0.37255, 0.00000 ,
    1.00000, 0.34118, 0.00000 ,
    1.00000, 0.30980, 0.00000 ,
    1.00000, 0.27843, 0.00000 ,
    1.00000, 0.24706, 0.00000 ,
    1.00000, 0.21569, 0.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 0.09020, 0.00000 ,
    1.00000, 0.05882, 0.00000 ,
    1.00000, 0.02745, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.04706 ,
    1.00000, 0.00000, 0.09412 ,
    1.00000, 0.00000, 0.14118 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 0.00000, 0.32941 ,
    1.00000, 0.00000, 0.37647 ,
    1.00000, 0.00000, 0.42353 ,
    1.00000, 0.00000, 0.47059 ,
    1.00000, 0.00000, 0.51765 ,
    1.00000, 0.00000, 0.56471 ,
    1.00000, 0.00000, 0.61176 ,
    1.00000, 0.00000, 0.65882 ,
    1.00000, 0.00000, 0.70588 ,
    1.00000, 0.00000, 0.72549 ,
    1.00000, 0.00000, 0.74902 ,
    1.00000, 0.00000, 0.77255 ,
    1.00000, 0.00000, 0.79608 ,
    1.00000, 0.00000, 0.81569 ,
    1.00000, 0.00000, 0.83922 ,
    1.00000, 0.00000, 0.86275 ,
    1.00000, 0.00000, 0.88627 ,
    1.00000, 0.00000, 0.90588 ,
    1.00000, 0.00000, 0.92941 ,
    1.00000, 0.00000, 0.95294 ,
    1.00000, 0.00000, 0.97647 ,
    1.00000, 0.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 0.14118, 1.00000 ,
    1.00000, 0.17647, 1.00000 ,
    1.00000, 0.21176, 1.00000 ,
    1.00000, 0.25098, 1.00000 ,
    1.00000, 0.28627, 1.00000 ,
    1.00000, 0.32157, 1.00000 ,
    1.00000, 0.36078, 1.00000 ,
    1.00000, 0.39608, 1.00000 ,
    1.00000, 0.43137, 1.00000 ,
    1.00000, 0.47059, 1.00000 ,
    1.00000, 0.48627, 1.00000 ,
    1.00000, 0.50588, 1.00000 ,
    1.00000, 0.52157, 1.00000 ,
    1.00000, 0.54118, 1.00000 ,
    1.00000, 0.56078, 1.00000 ,
    1.00000, 0.57647, 1.00000 ,
    1.00000, 0.59608, 1.00000 ,
    1.00000, 0.61176, 1.00000 ,
    1.00000, 0.63137, 1.00000 ,
    1.00000, 0.65098, 1.00000 ,
    1.00000, 0.66667, 1.00000 ,
    1.00000, 0.68627, 1.00000 ,
    1.00000, 0.70588, 1.00000 ,
    1.00000, 0.74510, 1.00000 ,
    1.00000, 0.78824, 1.00000 ,
    1.00000, 0.83137, 1.00000 ,
    1.00000, 0.87059, 1.00000 ,
    1.00000, 0.91373, 1.00000 ,
    1.00000, 0.95686, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
};

static Colour_struct mono[LUT_SIZE] =
{
    0.00000 ,    0.00000 ,    0.00000 ,
    0.00392 ,    0.00392 ,    0.00392 ,
    0.00784 ,    0.00784 ,    0.00784 ,
    0.01176 ,    0.01176 ,    0.01176 ,
    0.01569 ,    0.01569 ,    0.01569 ,
    0.01961 ,    0.01961 ,    0.01961 ,
    0.02353 ,    0.02353 ,    0.02353 ,
    0.02745 ,    0.02745 ,    0.02745 ,
    0.03137 ,    0.03137 ,    0.03137 ,
    0.03529 ,    0.03529 ,    0.03529 ,
    0.03922 ,    0.03922 ,    0.03922 ,
    0.04314 ,    0.04314 ,    0.04314 ,
    0.04706 ,    0.04706 ,    0.04706 ,
    0.05098 ,    0.05098 ,    0.05098 ,
    0.05490 ,    0.05490 ,    0.05490 ,
    0.05882 ,    0.05882 ,    0.05882 ,
    0.06275 ,    0.06275 ,    0.06275 ,
    0.06667 ,    0.06667 ,    0.06667 ,
    0.07059 ,    0.07059 ,    0.07059 ,
    0.07451 ,    0.07451 ,    0.07451 ,
    0.07843 ,    0.07843 ,    0.07843 ,
    0.08235 ,    0.08235 ,    0.08235 ,
    0.08627 ,    0.08627 ,    0.08627 ,
    0.09020 ,    0.09020 ,    0.09020 ,
    0.09412 ,    0.09412 ,    0.09412 ,
    0.09804 ,    0.09804 ,    0.09804 ,
    0.10196 ,    0.10196 ,    0.10196 ,
    0.10588 ,    0.10588 ,    0.10588 ,
    0.10980 ,    0.10980 ,    0.10980 ,
    0.11373 ,    0.11373 ,    0.11373 ,
    0.11765 ,    0.11765 ,    0.11765 ,
    0.12157 ,    0.12157 ,    0.12157 ,
    0.12549 ,    0.12549 ,    0.12549 ,
    0.12941 ,    0.12941 ,    0.12941 ,
    0.13333 ,    0.13333 ,    0.13333 ,
    0.13725 ,    0.13725 ,    0.13725 ,
    0.14118 ,    0.14118 ,    0.14118 ,
    0.14510 ,    0.14510 ,    0.14510 ,
    0.14902 ,    0.14902 ,    0.14902 ,
    0.15294 ,    0.15294 ,    0.15294 ,
    0.15686 ,    0.15686 ,    0.15686 ,
    0.16078 ,    0.16078 ,    0.16078 ,
    0.16471 ,    0.16471 ,    0.16471 ,
    0.16863 ,    0.16863 ,    0.16863 ,
    0.17255 ,    0.17255 ,    0.17255 ,
    0.17647 ,    0.17647 ,    0.17647 ,
    0.18039 ,    0.18039 ,    0.18039 ,
    0.18431 ,    0.18431 ,    0.18431 ,
    0.18824 ,    0.18824 ,    0.18824 ,
    0.19216 ,    0.19216 ,    0.19216 ,
    0.19608 ,    0.19608 ,    0.19608 ,
    0.20000 ,    0.20000 ,    0.20000 ,
    0.20392 ,    0.20392 ,    0.20392 ,
    0.20784 ,    0.20784 ,    0.20784 ,
    0.21176 ,    0.21176 ,    0.21176 ,
    0.21569 ,    0.21569 ,    0.21569 ,
    0.21961 ,    0.21961 ,    0.21961 ,
    0.22353 ,    0.22353 ,    0.22353 ,
    0.22745 ,    0.22745 ,    0.22745 ,
    0.23137 ,    0.23137 ,    0.23137 ,
    0.23529 ,    0.23529 ,    0.23529 ,
    0.23922 ,    0.23922 ,    0.23922 ,
    0.24314 ,    0.24314 ,    0.24314 ,
    0.24706 ,    0.24706 ,    0.24706 ,
    0.25098 ,    0.25098 ,    0.25098 ,
    0.25490 ,    0.25490 ,    0.25490 ,
    0.25882 ,    0.25882 ,    0.25882 ,
    0.26275 ,    0.26275 ,    0.26275 ,
    0.26667 ,    0.26667 ,    0.26667 ,
    0.27059 ,    0.27059 ,    0.27059 ,
    0.27451 ,    0.27451 ,    0.27451 ,
    0.27843 ,    0.27843 ,    0.27843 ,
    0.28235 ,    0.28235 ,    0.28235 ,
    0.28627 ,    0.28627 ,    0.28627 ,
    0.29020 ,    0.29020 ,    0.29020 ,
    0.29412 ,    0.29412 ,    0.29412 ,
    0.29804 ,    0.29804 ,    0.29804 ,
    0.30196 ,    0.30196 ,    0.30196 ,
    0.30588 ,    0.30588 ,    0.30588 ,
    0.30980 ,    0.30980 ,    0.30980 ,
    0.31373 ,    0.31373 ,    0.31373 ,
    0.31765 ,    0.31765 ,    0.31765 ,
    0.32157 ,    0.32157 ,    0.32157 ,
    0.32549 ,    0.32549 ,    0.32549 ,
    0.32941 ,    0.32941 ,    0.32941 ,
    0.33333 ,    0.33333 ,    0.33333 ,
    0.33725 ,    0.33725 ,    0.33725 ,
    0.34118 ,    0.34118 ,    0.34118 ,
    0.34510 ,    0.34510 ,    0.34510 ,
    0.34902 ,    0.34902 ,    0.34902 ,
    0.35294 ,    0.35294 ,    0.35294 ,
    0.35686 ,    0.35686 ,    0.35686 ,
    0.36078 ,    0.36078 ,    0.36078 ,
    0.36471 ,    0.36471 ,    0.36471 ,
    0.36863 ,    0.36863 ,    0.36863 ,
    0.37255 ,    0.37255 ,    0.37255 ,
    0.37647 ,    0.37647 ,    0.37647 ,
    0.38039 ,    0.38039 ,    0.38039 ,
    0.38431 ,    0.38431 ,    0.38431 ,
    0.38824 ,    0.38824 ,    0.38824 ,
    0.39216 ,    0.39216 ,    0.39216 ,
    0.39608 ,    0.39608 ,    0.39608 ,
    0.40000 ,    0.40000 ,    0.40000 ,
    0.40392 ,    0.40392 ,    0.40392 ,
    0.40784 ,    0.40784 ,    0.40784 ,
    0.41176 ,    0.41176 ,    0.41176 ,
    0.41569 ,    0.41569 ,    0.41569 ,
    0.41961 ,    0.41961 ,    0.41961 ,
    0.42353 ,    0.42353 ,    0.42353 ,
    0.42745 ,    0.42745 ,    0.42745 ,
    0.43137 ,    0.43137 ,    0.43137 ,
    0.43529 ,    0.43529 ,    0.43529 ,
    0.43922 ,    0.43922 ,    0.43922 ,
    0.44314 ,    0.44314 ,    0.44314 ,
    0.44706 ,    0.44706 ,    0.44706 ,
    0.45098 ,    0.45098 ,    0.45098 ,
    0.45490 ,    0.45490 ,    0.45490 ,
    0.45882 ,    0.45882 ,    0.45882 ,
    0.46275 ,    0.46275 ,    0.46275 ,
    0.46667 ,    0.46667 ,    0.46667 ,
    0.47059 ,    0.47059 ,    0.47059 ,
    0.47451 ,    0.47451 ,    0.47451 ,
    0.47843 ,    0.47843 ,    0.47843 ,
    0.48235 ,    0.48235 ,    0.48235 ,
    0.48627 ,    0.48627 ,    0.48627 ,
    0.49020 ,    0.49020 ,    0.49020 ,
    0.49412 ,    0.49412 ,    0.49412 ,
    0.49804 ,    0.49804 ,    0.49804 ,
    0.50196 ,    0.50196 ,    0.50196 ,
    0.50588 ,    0.50588 ,    0.50588 ,
    0.50980 ,    0.50980 ,    0.50980 ,
    0.51373 ,    0.51373 ,    0.51373 ,
    0.51765 ,    0.51765 ,    0.51765 ,
    0.52157 ,    0.52157 ,    0.52157 ,
    0.52549 ,    0.52549 ,    0.52549 ,
    0.52941 ,    0.52941 ,    0.52941 ,
    0.53333 ,    0.53333 ,    0.53333 ,
    0.53725 ,    0.53725 ,    0.53725 ,
    0.54118 ,    0.54118 ,    0.54118 ,
    0.54510 ,    0.54510 ,    0.54510 ,
    0.54902 ,    0.54902 ,    0.54902 ,
    0.55294 ,    0.55294 ,    0.55294 ,
    0.55686 ,    0.55686 ,    0.55686 ,
    0.56078 ,    0.56078 ,    0.56078 ,
    0.56471 ,    0.56471 ,    0.56471 ,
    0.56863 ,    0.56863 ,    0.56863 ,
    0.57255 ,    0.57255 ,    0.57255 ,
    0.57647 ,    0.57647 ,    0.57647 ,
    0.58039 ,    0.58039 ,    0.58039 ,
    0.58431 ,    0.58431 ,    0.58431 ,
    0.58824 ,    0.58824 ,    0.58824 ,
    0.59216 ,    0.59216 ,    0.59216 ,
    0.59608 ,    0.59608 ,    0.59608 ,
    0.60000 ,    0.60000 ,    0.60000 ,
    0.60392 ,    0.60392 ,    0.60392 ,
    0.60784 ,    0.60784 ,    0.60784 ,
    0.61176 ,    0.61176 ,    0.61176 ,
    0.61569 ,    0.61569 ,    0.61569 ,
    0.61961 ,    0.61961 ,    0.61961 ,
    0.62353 ,    0.62353 ,    0.62353 ,
    0.62745 ,    0.62745 ,    0.62745 ,
    0.63137 ,    0.63137 ,    0.63137 ,
    0.63529 ,    0.63529 ,    0.63529 ,
    0.63922 ,    0.63922 ,    0.63922 ,
    0.64314 ,    0.64314 ,    0.64314 ,
    0.64706 ,    0.64706 ,    0.64706 ,
    0.65098 ,    0.65098 ,    0.65098 ,
    0.65490 ,    0.65490 ,    0.65490 ,
    0.65882 ,    0.65882 ,    0.65882 ,
    0.66275 ,    0.66275 ,    0.66275 ,
    0.66667 ,    0.66667 ,    0.66667 ,
    0.67059 ,    0.67059 ,    0.67059 ,
    0.67451 ,    0.67451 ,    0.67451 ,
    0.67843 ,    0.67843 ,    0.67843 ,
    0.68235 ,    0.68235 ,    0.68235 ,
    0.68627 ,    0.68627 ,    0.68627 ,
    0.69020 ,    0.69020 ,    0.69020 ,
    0.69412 ,    0.69412 ,    0.69412 ,
    0.69804 ,    0.69804 ,    0.69804 ,
    0.70196 ,    0.70196 ,    0.70196 ,
    0.70588 ,    0.70588 ,    0.70588 ,
    0.70980 ,    0.70980 ,    0.70980 ,
    0.71373 ,    0.71373 ,    0.71373 ,
    0.71765 ,    0.71765 ,    0.71765 ,
    0.72157 ,    0.72157 ,    0.72157 ,
    0.72549 ,    0.72549 ,    0.72549 ,
    0.72941 ,    0.72941 ,    0.72941 ,
    0.73333 ,    0.73333 ,    0.73333 ,
    0.73725 ,    0.73725 ,    0.73725 ,
    0.74118 ,    0.74118 ,    0.74118 ,
    0.74510 ,    0.74510 ,    0.74510 ,
    0.74902 ,    0.74902 ,    0.74902 ,
    0.75294 ,    0.75294 ,    0.75294 ,
    0.75686 ,    0.75686 ,    0.75686 ,
    0.76078 ,    0.76078 ,    0.76078 ,
    0.76471 ,    0.76471 ,    0.76471 ,
    0.76863 ,    0.76863 ,    0.76863 ,
    0.77255 ,    0.77255 ,    0.77255 ,
    0.77647 ,    0.77647 ,    0.77647 ,
    0.78039 ,    0.78039 ,    0.78039 ,
    0.78431 ,    0.78431 ,    0.78431 ,
    0.78824 ,    0.78824 ,    0.78824 ,
    0.79216 ,    0.79216 ,    0.79216 ,
    0.79608 ,    0.79608 ,    0.79608 ,
    0.80000 ,    0.80000 ,    0.80000 ,
    0.80392 ,    0.80392 ,    0.80392 ,
    0.80784 ,    0.80784 ,    0.80784 ,
    0.81176 ,    0.81176 ,    0.81176 ,
    0.81569 ,    0.81569 ,    0.81569 ,
    0.81961 ,    0.81961 ,    0.81961 ,
    0.82353 ,    0.82353 ,    0.82353 ,
    0.82745 ,    0.82745 ,    0.82745 ,
    0.83137 ,    0.83137 ,    0.83137 ,
    0.83529 ,    0.83529 ,    0.83529 ,
    0.83922 ,    0.83922 ,    0.83922 ,
    0.84314 ,    0.84314 ,    0.84314 ,
    0.84706 ,    0.84706 ,    0.84706 ,
    0.85098 ,    0.85098 ,    0.85098 ,
    0.85490 ,    0.85490 ,    0.85490 ,
    0.85882 ,    0.85882 ,    0.85882 ,
    0.86275 ,    0.86275 ,    0.86275 ,
    0.86667 ,    0.86667 ,    0.86667 ,
    0.87059 ,    0.87059 ,    0.87059 ,
    0.87451 ,    0.87451 ,    0.87451 ,
    0.87843 ,    0.87843 ,    0.87843 ,
    0.88235 ,    0.88235 ,    0.88235 ,
    0.88627 ,    0.88627 ,    0.88627 ,
    0.89020 ,    0.89020 ,    0.89020 ,
    0.89412 ,    0.89412 ,    0.89412 ,
    0.89804 ,    0.89804 ,    0.89804 ,
    0.90196 ,    0.90196 ,    0.90196 ,
    0.90588 ,    0.90588 ,    0.90588 ,
    0.90980 ,    0.90980 ,    0.90980 ,
    0.91373 ,    0.91373 ,    0.91373 ,
    0.91765 ,    0.91765 ,    0.91765 ,
    0.92157 ,    0.92157 ,    0.92157 ,
    0.92549 ,    0.92549 ,    0.92549 ,
    0.92941 ,    0.92941 ,    0.92941 ,
    0.93333 ,    0.93333 ,    0.93333 ,
    0.93725 ,    0.93725 ,    0.93725 ,
    0.94118 ,    0.94118 ,    0.94118 ,
    0.94510 ,    0.94510 ,    0.94510 ,
    0.94902 ,    0.94902 ,    0.94902 ,
    0.95294 ,    0.95294 ,    0.95294 ,
    0.95686 ,    0.95686 ,    0.95686 ,
    0.96078 ,    0.96078 ,    0.96078 ,
    0.96471 ,    0.96471 ,    0.96471 ,
    0.96863 ,    0.96863 ,    0.96863 ,
    0.97255 ,    0.97255 ,    0.97255 ,
    0.97647 ,    0.97647 ,    0.97647 ,
    0.98039 ,    0.98039 ,    0.98039 ,
    0.98431 ,    0.98431 ,    0.98431 ,
    0.98824 ,    0.98824 ,    0.98824 ,
    0.99216 ,    0.99216 ,    0.99216 ,
    0.99608 ,    0.99608 ,    0.99608 ,
    1.00000 ,    1.00000 ,    1.00000 
};

static Colour_struct mousse[LUT_SIZE] =
{
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.06667 ,
    0.00000, 0.00000, 0.06667 ,
    0.00000, 0.00000, 0.06667 ,
    0.00000, 0.00000, 0.06667 ,
    0.00000, 0.00000, 0.06667 ,
    0.00000, 0.00000, 0.06667 ,
    0.00000, 0.00000, 0.06667 ,
    0.00000, 0.00000, 0.06667 ,
    0.00000, 0.00000, 0.13333 ,
    0.00000, 0.00000, 0.13333 ,
    0.00000, 0.00000, 0.13333 ,
    0.00000, 0.00000, 0.13333 ,
    0.00000, 0.00000, 0.13333 ,
    0.00000, 0.00000, 0.13333 ,
    0.00000, 0.00000, 0.13333 ,
    0.00000, 0.00000, 0.13333 ,
    0.00000, 0.00000, 0.20000 ,
    0.00000, 0.00000, 0.20000 ,
    0.00000, 0.00000, 0.20000 ,
    0.00000, 0.00000, 0.20000 ,
    0.00000, 0.00000, 0.20000 ,
    0.00000, 0.00000, 0.20000 ,
    0.00000, 0.00000, 0.20000 ,
    0.00000, 0.00000, 0.20000 ,
    0.00000, 0.00000, 0.26667 ,
    0.00000, 0.00000, 0.26667 ,
    0.00000, 0.00000, 0.26667 ,
    0.00000, 0.00000, 0.26667 ,
    0.00000, 0.00000, 0.26667 ,
    0.00000, 0.00000, 0.26667 ,
    0.00000, 0.00000, 0.26667 ,
    0.00000, 0.00000, 0.26667 ,
    0.00000, 0.00000, 0.33333 ,
    0.00000, 0.00000, 0.33333 ,
    0.00000, 0.00000, 0.33333 ,
    0.00000, 0.00000, 0.33333 ,
    0.00000, 0.00000, 0.33333 ,
    0.00000, 0.00000, 0.33333 ,
    0.00000, 0.00000, 0.33333 ,
    0.00000, 0.00000, 0.33333 ,
    0.00000, 0.00000, 0.40000 ,
    0.00000, 0.00000, 0.40000 ,
    0.00000, 0.00000, 0.40000 ,
    0.00000, 0.00000, 0.40000 ,
    0.00000, 0.00000, 0.40000 ,
    0.00000, 0.00000, 0.40000 ,
    0.00000, 0.00000, 0.40000 ,
    0.00000, 0.00000, 0.40000 ,
    0.00000, 0.00000, 0.46667 ,
    0.00000, 0.00000, 0.46667 ,
    0.00000, 0.00000, 0.46667 ,
    0.00000, 0.00000, 0.46667 ,
    0.00000, 0.00000, 0.46667 ,
    0.00000, 0.00000, 0.46667 ,
    0.00000, 0.00000, 0.46667 ,
    0.00000, 0.00000, 0.46667 ,
    0.00000, 0.00000, 0.53333 ,
    0.00000, 0.00000, 0.53333 ,
    0.00000, 0.00000, 0.53333 ,
    0.00000, 0.00000, 0.53333 ,
    0.00000, 0.00000, 0.53333 ,
    0.00000, 0.00000, 0.53333 ,
    0.00000, 0.00000, 0.53333 ,
    0.00000, 0.00000, 0.53333 ,
    0.06667, 0.00000, 0.53333 ,
    0.06667, 0.00000, 0.53333 ,
    0.06667, 0.00000, 0.53333 ,
    0.06667, 0.00000, 0.53333 ,
    0.06667, 0.00000, 0.53333 ,
    0.06667, 0.00000, 0.53333 ,
    0.06667, 0.00000, 0.53333 ,
    0.06667, 0.00000, 0.53333 ,
    0.13333, 0.00000, 0.53333 ,
    0.13333, 0.00000, 0.53333 ,
    0.13333, 0.00000, 0.53333 ,
    0.13333, 0.00000, 0.53333 ,
    0.13333, 0.00000, 0.53333 ,
    0.13333, 0.00000, 0.53333 ,
    0.13333, 0.00000, 0.53333 ,
    0.13333, 0.00000, 0.53333 ,
    0.20000, 0.00000, 0.53333 ,
    0.20000, 0.00000, 0.53333 ,
    0.20000, 0.00000, 0.53333 ,
    0.20000, 0.00000, 0.53333 ,
    0.20000, 0.00000, 0.53333 ,
    0.20000, 0.00000, 0.53333 ,
    0.20000, 0.00000, 0.53333 ,
    0.20000, 0.00000, 0.53333 ,
    0.26667, 0.00000, 0.53333 ,
    0.26667, 0.00000, 0.53333 ,
    0.26667, 0.00000, 0.53333 ,
    0.26667, 0.00000, 0.53333 ,
    0.26667, 0.00000, 0.53333 ,
    0.26667, 0.00000, 0.53333 ,
    0.26667, 0.00000, 0.53333 ,
    0.26667, 0.00000, 0.53333 ,
    0.33333, 0.00000, 0.53333 ,
    0.33333, 0.00000, 0.53333 ,
    0.33333, 0.00000, 0.53333 ,
    0.33333, 0.00000, 0.53333 ,
    0.33333, 0.00000, 0.53333 ,
    0.33333, 0.00000, 0.53333 ,
    0.33333, 0.00000, 0.53333 ,
    0.33333, 0.00000, 0.53333 ,
    0.40000, 0.00000, 0.53333 ,
    0.40000, 0.00000, 0.53333 ,
    0.40000, 0.00000, 0.53333 ,
    0.40000, 0.00000, 0.53333 ,
    0.40000, 0.00000, 0.53333 ,
    0.40000, 0.00000, 0.53333 ,
    0.40000, 0.00000, 0.53333 ,
    0.40000, 0.00000, 0.53333 ,
    0.46667, 0.00000, 0.53333 ,
    0.46667, 0.00000, 0.53333 ,
    0.46667, 0.00000, 0.53333 ,
    0.46667, 0.00000, 0.53333 ,
    0.46667, 0.00000, 0.53333 ,
    0.46667, 0.00000, 0.53333 ,
    0.46667, 0.00000, 0.53333 ,
    0.46667, 0.00000, 0.53333 ,
    0.53333, 0.00000, 0.53333 ,
    0.53333, 0.00000, 0.53333 ,
    0.53333, 0.00000, 0.53333 ,
    0.53333, 0.00000, 0.53333 ,
    0.53333, 0.00000, 0.46667 ,
    0.53333, 0.00000, 0.46667 ,
    0.53333, 0.00000, 0.46667 ,
    0.53333, 0.00000, 0.46667 ,
    0.60000, 0.00000, 0.40000 ,
    0.60000, 0.00000, 0.40000 ,
    0.60000, 0.00000, 0.40000 ,
    0.60000, 0.00000, 0.40000 ,
    0.60000, 0.00000, 0.33333 ,
    0.60000, 0.00000, 0.33333 ,
    0.60000, 0.00000, 0.33333 ,
    0.60000, 0.00000, 0.33333 ,
    0.66667, 0.00000, 0.26667 ,
    0.66667, 0.00000, 0.26667 ,
    0.66667, 0.00000, 0.26667 ,
    0.66667, 0.00000, 0.26667 ,
    0.66667, 0.00000, 0.20000 ,
    0.66667, 0.00000, 0.20000 ,
    0.66667, 0.00000, 0.20000 ,
    0.66667, 0.00000, 0.20000 ,
    0.73333, 0.00000, 0.13333 ,
    0.73333, 0.00000, 0.13333 ,
    0.73333, 0.00000, 0.13333 ,
    0.73333, 0.00000, 0.13333 ,
    0.73333, 0.00000, 0.06667 ,
    0.73333, 0.00000, 0.06667 ,
    0.73333, 0.00000, 0.06667 ,
    0.73333, 0.00000, 0.06667 ,
    0.80000, 0.00000, 0.00000 ,
    0.80000, 0.00000, 0.00000 ,
    0.80000, 0.00000, 0.00000 ,
    0.80000, 0.00000, 0.00000 ,
    0.80000, 0.00000, 0.00000 ,
    0.80000, 0.00000, 0.00000 ,
    0.80000, 0.00000, 0.00000 ,
    0.80000, 0.00000, 0.00000 ,
    0.86667, 0.00000, 0.00000 ,
    0.86667, 0.00000, 0.00000 ,
    0.86667, 0.00000, 0.00000 ,
    0.86667, 0.00000, 0.00000 ,
    0.86667, 0.00000, 0.00000 ,
    0.86667, 0.00000, 0.00000 ,
    0.86667, 0.00000, 0.00000 ,
    0.86667, 0.00000, 0.00000 ,
    0.93333, 0.00000, 0.00000 ,
    0.93333, 0.00000, 0.00000 ,
    0.93333, 0.00000, 0.00000 ,
    0.93333, 0.00000, 0.00000 ,
    0.93333, 0.00000, 0.00000 ,
    0.93333, 0.00000, 0.00000 ,
    0.93333, 0.00000, 0.00000 ,
    0.93333, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.06667, 0.00000 ,
    1.00000, 0.06667, 0.00000 ,
    1.00000, 0.13333, 0.00000 ,
    1.00000, 0.13333, 0.00000 ,
    1.00000, 0.20000, 0.00000 ,
    1.00000, 0.20000, 0.00000 ,
    1.00000, 0.26667, 0.00000 ,
    1.00000, 0.26667, 0.00000 ,
    1.00000, 0.33333, 0.00000 ,
    1.00000, 0.33333, 0.00000 ,
    1.00000, 0.40000, 0.00000 ,
    1.00000, 0.40000, 0.00000 ,
    1.00000, 0.46667, 0.00000 ,
    1.00000, 0.46667, 0.00000 ,
    1.00000, 0.53333, 0.00000 ,
    1.00000, 0.53333, 0.00000 ,
    1.00000, 0.60000, 0.00000 ,
    1.00000, 0.60000, 0.00000 ,
    1.00000, 0.66667, 0.00000 ,
    1.00000, 0.66667, 0.00000 ,
    1.00000, 0.73333, 0.00000 ,
    1.00000, 0.73333, 0.00000 ,
    1.00000, 0.80000, 0.00000 ,
    1.00000, 0.80000, 0.00000 ,
    1.00000, 0.86667, 0.00000 ,
    1.00000, 0.86667, 0.00000 ,
    1.00000, 0.93333, 0.00000 ,
    1.00000, 0.93333, 0.00000 ,
    1.00000, 1.00000, 0.00000 ,
    1.00000, 1.00000, 0.00000 ,
    1.00000, 1.00000, 0.00000 ,
    1.00000, 1.00000, 0.00000 ,
    1.00000, 1.00000, 0.06667 ,
    1.00000, 1.00000, 0.06667 ,
    1.00000, 1.00000, 0.13333 ,
    1.00000, 1.00000, 0.13333 ,
    1.00000, 1.00000, 0.20000 ,
    1.00000, 1.00000, 0.20000 ,
    1.00000, 1.00000, 0.26667 ,
    1.00000, 1.00000, 0.26667 ,
    1.00000, 1.00000, 0.33333 ,
    1.00000, 1.00000, 0.33333 ,
    1.00000, 1.00000, 0.40000 ,
    1.00000, 1.00000, 0.40000 ,
    1.00000, 1.00000, 0.46667 ,
    1.00000, 1.00000, 0.46667 ,
    1.00000, 1.00000, 0.53333 ,
    1.00000, 1.00000, 0.53333 ,
    1.00000, 1.00000, 0.60000 ,
    1.00000, 1.00000, 0.60000 ,
    1.00000, 1.00000, 0.66667 ,
    1.00000, 1.00000, 0.66667 ,
    1.00000, 1.00000, 0.73333 ,
    1.00000, 1.00000, 0.73333 ,
    1.00000, 1.00000, 0.80000 ,
    1.00000, 1.00000, 0.80000 ,
    1.00000, 1.00000, 0.86667 ,
    1.00000, 1.00000, 1.00000 ,
};

static Colour_struct rainbow[LUT_SIZE] =
{
    0.00000, 0.00000, 0.01176 ,
    0.00000, 0.00000, 0.02745 ,
    0.00000, 0.00000, 0.04314 ,
    0.00000, 0.00000, 0.05882 ,
    0.00000, 0.00000, 0.07451 ,
    0.00000, 0.00000, 0.09020 ,
    0.00000, 0.00000, 0.10588 ,
    0.00000, 0.00000, 0.12157 ,
    0.00000, 0.00000, 0.13725 ,
    0.00000, 0.00000, 0.15294 ,
    0.00000, 0.00000, 0.16863 ,
    0.00000, 0.00000, 0.18431 ,
    0.00000, 0.00000, 0.20000 ,
    0.00000, 0.00000, 0.21176 ,
    0.00000, 0.00000, 0.22745 ,
    0.00000, 0.00000, 0.24314 ,
    0.00000, 0.00000, 0.25882 ,
    0.00000, 0.00000, 0.27451 ,
    0.00000, 0.00000, 0.29020 ,
    0.00000, 0.00000, 0.30588 ,
    0.00000, 0.00000, 0.32157 ,
    0.00000, 0.00000, 0.33725 ,
    0.00000, 0.00000, 0.35294 ,
    0.00000, 0.00000, 0.36863 ,
    0.00000, 0.00000, 0.38431 ,
    0.00000, 0.00000, 0.40000 ,
    0.00000, 0.00000, 0.41176 ,
    0.00000, 0.00000, 0.42745 ,
    0.00000, 0.00000, 0.44314 ,
    0.00000, 0.00000, 0.45882 ,
    0.00000, 0.00000, 0.47451 ,
    0.00000, 0.00000, 0.49020 ,
    0.00000, 0.00000, 0.50588 ,
    0.00000, 0.00000, 0.52157 ,
    0.00000, 0.00000, 0.53725 ,
    0.00000, 0.00000, 0.55294 ,
    0.00000, 0.00000, 0.56863 ,
    0.00000, 0.00000, 0.58431 ,
    0.00000, 0.00000, 0.60000 ,
    0.00000, 0.00000, 0.61176 ,
    0.00000, 0.00000, 0.62745 ,
    0.00000, 0.00000, 0.64314 ,
    0.00000, 0.00000, 0.65882 ,
    0.00000, 0.00000, 0.67451 ,
    0.00000, 0.00000, 0.69020 ,
    0.00000, 0.00000, 0.70588 ,
    0.00000, 0.00000, 0.72157 ,
    0.00000, 0.00000, 0.73725 ,
    0.00000, 0.00000, 0.75294 ,
    0.00000, 0.00000, 0.76863 ,
    0.00000, 0.00000, 0.78431 ,
    0.00000, 0.00000, 0.80000 ,
    0.00000, 0.00000, 0.81176 ,
    0.00000, 0.00000, 0.82745 ,
    0.00000, 0.00000, 0.84314 ,
    0.00000, 0.00000, 0.85882 ,
    0.00000, 0.00000, 0.87451 ,
    0.00000, 0.00000, 0.89020 ,
    0.00000, 0.00000, 0.90588 ,
    0.00000, 0.00000, 0.92157 ,
    0.00000, 0.00000, 0.93725 ,
    0.00000, 0.00000, 0.95294 ,
    0.00000, 0.00000, 0.96863 ,
    0.00000, 0.00000, 0.98431 ,
    0.00000, 0.00000, 1.00000 ,
    0.00000, 0.03529, 1.00000 ,
    0.00000, 0.07059, 1.00000 ,
    0.00000, 0.10980, 1.00000 ,
    0.00000, 0.14510, 1.00000 ,
    0.00000, 0.18039, 1.00000 ,
    0.00000, 0.21961, 1.00000 ,
    0.00000, 0.25490, 1.00000 ,
    0.00000, 0.29412, 1.00000 ,
    0.00000, 0.32941, 1.00000 ,
    0.00000, 0.36471, 1.00000 ,
    0.00000, 0.40392, 1.00000 ,
    0.00000, 0.43922, 1.00000 ,
    0.00000, 0.47843, 1.00000 ,
    0.00000, 0.50196, 1.00000 ,
    0.00000, 0.52549, 1.00000 ,
    0.00000, 0.54902, 1.00000 ,
    0.00000, 0.57255, 1.00000 ,
    0.00000, 0.59608, 1.00000 ,
    0.00000, 0.61961, 1.00000 ,
    0.00000, 0.64314, 1.00000 ,
    0.00000, 0.66667, 1.00000 ,
    0.00000, 0.69020, 1.00000 ,
    0.00000, 0.71373, 1.00000 ,
    0.00000, 0.73725, 1.00000 ,
    0.00000, 0.76078, 1.00000 ,
    0.00000, 0.78431, 1.00000 ,
    0.00000, 0.80000, 1.00000 ,
    0.00000, 0.81569, 1.00000 ,
    0.00000, 0.83137, 1.00000 ,
    0.00000, 0.84706, 1.00000 ,
    0.00000, 0.86667, 1.00000 ,
    0.00000, 0.88235, 1.00000 ,
    0.00000, 0.89804, 1.00000 ,
    0.00000, 0.91373, 1.00000 ,
    0.00000, 0.93333, 1.00000 ,
    0.00000, 0.94902, 1.00000 ,
    0.00000, 0.96471, 1.00000 ,
    0.00000, 0.98039, 1.00000 ,
    0.00000, 1.00000, 1.00000 ,
    0.00000, 1.00000, 0.97647 ,
    0.00000, 1.00000, 0.95294 ,
    0.00000, 1.00000, 0.92941 ,
    0.00000, 1.00000, 0.90588 ,
    0.00000, 1.00000, 0.88627 ,
    0.00000, 1.00000, 0.86275 ,
    0.00000, 1.00000, 0.83922 ,
    0.00000, 1.00000, 0.81569 ,
    0.00000, 1.00000, 0.79608 ,
    0.00000, 1.00000, 0.77255 ,
    0.00000, 1.00000, 0.74902 ,
    0.00000, 1.00000, 0.72549 ,
    0.00000, 1.00000, 0.70588 ,
    0.00000, 1.00000, 0.65098 ,
    0.00000, 1.00000, 0.59608 ,
    0.00000, 1.00000, 0.54118 ,
    0.00000, 1.00000, 0.48627 ,
    0.00000, 1.00000, 0.43137 ,
    0.00000, 1.00000, 0.37647 ,
    0.00000, 1.00000, 0.32549 ,
    0.00000, 1.00000, 0.27059 ,
    0.00000, 1.00000, 0.21569 ,
    0.00000, 1.00000, 0.16078 ,
    0.00000, 1.00000, 0.10588 ,
    0.00000, 1.00000, 0.05098 ,
    0.00000, 1.00000, 0.00000 ,
    0.05098, 1.00000, 0.00000 ,
    0.10588, 1.00000, 0.00000 ,
    0.16078, 1.00000, 0.00000 ,
    0.21569, 1.00000, 0.00000 ,
    0.27059, 1.00000, 0.00000 ,
    0.32549, 1.00000, 0.00000 ,
    0.37647, 1.00000, 0.00000 ,
    0.43137, 1.00000, 0.00000 ,
    0.48627, 1.00000, 0.00000 ,
    0.54118, 1.00000, 0.00000 ,
    0.59608, 1.00000, 0.00000 ,
    0.65098, 1.00000, 0.00000 ,
    0.70588, 1.00000, 0.00000 ,
    0.72549, 1.00000, 0.00000 ,
    0.74902, 1.00000, 0.00000 ,
    0.77255, 1.00000, 0.00000 ,
    0.79608, 1.00000, 0.00000 ,
    0.81569, 1.00000, 0.00000 ,
    0.83922, 1.00000, 0.00000 ,
    0.86275, 1.00000, 0.00000 ,
    0.88627, 1.00000, 0.00000 ,
    0.90588, 1.00000, 0.00000 ,
    0.92941, 1.00000, 0.00000 ,
    0.95294, 1.00000, 0.00000 ,
    0.97647, 1.00000, 0.00000 ,
    1.00000, 1.00000, 0.00000 ,
    0.99608, 0.97647, 0.00000 ,
    0.99608, 0.95686, 0.00000 ,
    0.99608, 0.93333, 0.00000 ,
    0.99608, 0.91373, 0.00000 ,
    0.99216, 0.89412, 0.00000 ,
    0.99216, 0.87059, 0.00000 ,
    0.99216, 0.85098, 0.00000 ,
    0.99216, 0.82745, 0.00000 ,
    0.98824, 0.80784, 0.00000 ,
    0.98824, 0.78824, 0.00000 ,
    0.98824, 0.76471, 0.00000 ,
    0.98824, 0.74510, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98824, 0.70588, 0.00000 ,
    0.98824, 0.68627, 0.00000 ,
    0.98824, 0.66667, 0.00000 ,
    0.98824, 0.64706, 0.00000 ,
    0.99216, 0.62745, 0.00000 ,
    0.99216, 0.60784, 0.00000 ,
    0.99216, 0.58824, 0.00000 ,
    0.99216, 0.56863, 0.00000 ,
    0.99608, 0.54902, 0.00000 ,
    0.99608, 0.52941, 0.00000 ,
    0.99608, 0.50980, 0.00000 ,
    0.99608, 0.49020, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.43137, 0.00000 ,
    1.00000, 0.39608, 0.00000 ,
    1.00000, 0.36078, 0.00000 ,
    1.00000, 0.32549, 0.00000 ,
    1.00000, 0.28627, 0.00000 ,
    1.00000, 0.25098, 0.00000 ,
    1.00000, 0.21569, 0.00000 ,
    1.00000, 0.18039, 0.00000 ,
    1.00000, 0.14118, 0.00000 ,
    1.00000, 0.10588, 0.00000 ,
    1.00000, 0.07059, 0.00000 ,
    1.00000, 0.03529, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.05098 ,
    1.00000, 0.00000, 0.10588 ,
    1.00000, 0.00000, 0.16078 ,
    1.00000, 0.00000, 0.21569 ,
    1.00000, 0.00000, 0.27059 ,
    1.00000, 0.00000, 0.32549 ,
    1.00000, 0.00000, 0.37647 ,
    1.00000, 0.00000, 0.43137 ,
    1.00000, 0.00000, 0.48627 ,
    1.00000, 0.00000, 0.54118 ,
    1.00000, 0.00000, 0.59608 ,
    1.00000, 0.00000, 0.65098 ,
    1.00000, 0.00000, 0.70588 ,
    1.00000, 0.00000, 0.72549 ,
    1.00000, 0.00000, 0.74902 ,
    1.00000, 0.00000, 0.77255 ,
    1.00000, 0.00000, 0.79608 ,
    1.00000, 0.00000, 0.81569 ,
    1.00000, 0.00000, 0.83922 ,
    1.00000, 0.00000, 0.86275 ,
    1.00000, 0.00000, 0.88627 ,
    1.00000, 0.00000, 0.90588 ,
    1.00000, 0.00000, 0.92941 ,
    1.00000, 0.00000, 0.95294 ,
    1.00000, 0.00000, 0.97647 ,
    1.00000, 0.00000, 1.00000 ,
    1.00000, 0.03529, 1.00000 ,
    1.00000, 0.07059, 1.00000 ,
    1.00000, 0.10588, 1.00000 ,
    1.00000, 0.14118, 1.00000 ,
    1.00000, 0.18039, 1.00000 ,
    1.00000, 0.21569, 1.00000 ,
    1.00000, 0.25098, 1.00000 ,
    1.00000, 0.28627, 1.00000 ,
    1.00000, 0.32549, 1.00000 ,
    1.00000, 0.36078, 1.00000 ,
    1.00000, 0.39608, 1.00000 ,
    1.00000, 0.43137, 1.00000 ,
    1.00000, 0.47059, 1.00000 ,
    1.00000, 0.48627, 1.00000 ,
    1.00000, 0.50588, 1.00000 ,
    1.00000, 0.52157, 1.00000 ,
    1.00000, 0.54118, 1.00000 ,
    1.00000, 0.56078, 1.00000 ,
    1.00000, 0.57647, 1.00000 ,
    1.00000, 0.59608, 1.00000 ,
    1.00000, 0.61176, 1.00000 ,
    1.00000, 0.63137, 1.00000 ,
    1.00000, 0.65098, 1.00000 ,
    1.00000, 0.66667, 1.00000 ,
    1.00000, 0.68627, 1.00000 ,
    1.00000, 0.70588, 1.00000 ,
    1.00000, 0.74510, 1.00000 ,
    1.00000, 0.78824, 1.00000 ,
    1.00000, 0.83137, 1.00000 ,
    1.00000, 0.87059, 1.00000 ,
    1.00000, 0.91373, 1.00000 ,
    1.00000, 0.95686, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
};

static Colour_struct random[LUT_SIZE] =
{
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00392, 0.47059, 0.00392 ,
    0.00392, 0.47059, 0.00392 ,
    0.00392, 0.47059, 0.00392 ,
    0.00392, 0.47059, 0.00392 ,
    0.00392, 0.47059, 0.00392 ,
    0.00392, 0.47059, 0.00392 ,
    0.00392, 0.47059, 0.00392 ,
    0.00392, 0.47059, 0.00392 ,
    0.00392, 0.47059, 0.00392 ,
    0.00392, 0.47059, 0.00392 ,
    0.00392, 0.62745, 0.00392 ,
    0.00392, 0.62745, 0.00392 ,
    0.00392, 0.62745, 0.00392 ,
    0.00392, 0.62745, 0.00392 ,
    0.00392, 0.62745, 0.00392 ,
    0.00392, 0.62745, 0.00392 ,
    0.00392, 0.62745, 0.00392 ,
    0.00392, 0.62745, 0.00392 ,
    0.00392, 0.62745, 0.00392 ,
    0.00392, 0.62745, 0.00392 ,
    0.00392, 0.78431, 0.00392 ,
    0.00392, 0.78431, 0.00392 ,
    0.00392, 0.78431, 0.00392 ,
    0.00392, 0.78431, 0.00392 ,
    0.00392, 0.78431, 0.00392 ,
    0.00392, 0.78431, 0.00392 ,
    0.00392, 0.78431, 0.00392 ,
    0.00392, 0.78431, 0.00392 ,
    0.00392, 0.78431, 0.00392 ,
    0.00392, 0.78431, 0.00392 ,
    0.00392, 1.00000, 0.00392 ,
    0.00392, 1.00000, 0.00392 ,
    0.00392, 1.00000, 0.00392 ,
    0.00392, 1.00000, 0.00392 ,
    0.00392, 1.00000, 0.00392 ,
    0.00392, 1.00000, 0.00392 ,
    0.00392, 1.00000, 0.00392 ,
    0.00392, 1.00000, 0.00392 ,
    0.00392, 1.00000, 0.00392 ,
    0.00392, 1.00000, 0.00392 ,
    0.00392, 0.86275, 0.47059 ,
    0.00392, 0.86275, 0.47059 ,
    0.00392, 0.86275, 0.47059 ,
    0.00392, 0.86275, 0.47059 ,
    0.00392, 0.86275, 0.47059 ,
    0.00392, 0.86275, 0.47059 ,
    0.00392, 0.86275, 0.47059 ,
    0.00392, 0.86275, 0.47059 ,
    0.00392, 0.86275, 0.47059 ,
    0.00392, 0.86275, 0.47059 ,
    0.00000, 0.78431, 0.62745 ,
    0.00000, 0.78431, 0.62745 ,
    0.00000, 0.78431, 0.62745 ,
    0.00000, 0.78431, 0.62745 ,
    0.00000, 0.78431, 0.62745 ,
    0.00000, 0.78431, 0.62745 ,
    0.00000, 0.78431, 0.62745 ,
    0.00000, 0.78431, 0.62745 ,
    0.00000, 0.78431, 0.62745 ,
    0.00000, 0.78431, 0.62745 ,
    0.00000, 0.70588, 0.78431 ,
    0.00000, 0.70588, 0.78431 ,
    0.00000, 0.70588, 0.78431 ,
    0.00000, 0.70588, 0.78431 ,
    0.00000, 0.70588, 0.78431 ,
    0.00000, 0.70588, 0.78431 ,
    0.00000, 0.70588, 0.78431 ,
    0.00000, 0.70588, 0.78431 ,
    0.00000, 0.70588, 0.78431 ,
    0.00000, 0.70588, 0.78431 ,
    0.00000, 0.62745, 1.00000 ,
    0.00000, 0.62745, 1.00000 ,
    0.00000, 0.62745, 1.00000 ,
    0.00000, 0.62745, 1.00000 ,
    0.00000, 0.62745, 1.00000 ,
    0.00000, 0.62745, 1.00000 ,
    0.00000, 0.62745, 1.00000 ,
    0.00000, 0.62745, 1.00000 ,
    0.00000, 0.62745, 1.00000 ,
    0.00000, 0.62745, 1.00000 ,
    0.23529, 0.47059, 1.00000 ,
    0.23529, 0.47059, 1.00000 ,
    0.23529, 0.47059, 1.00000 ,
    0.23529, 0.47059, 1.00000 ,
    0.23529, 0.47059, 1.00000 ,
    0.23529, 0.47059, 1.00000 ,
    0.23529, 0.47059, 1.00000 ,
    0.23529, 0.47059, 1.00000 ,
    0.23529, 0.47059, 1.00000 ,
    0.23529, 0.47059, 1.00000 ,
    0.23529, 0.00392, 1.00000 ,
    0.23529, 0.00392, 1.00000 ,
    0.23529, 0.00392, 1.00000 ,
    0.23529, 0.00392, 1.00000 ,
    0.23529, 0.00392, 1.00000 ,
    0.23529, 0.00392, 1.00000 ,
    0.23529, 0.00392, 1.00000 ,
    0.23529, 0.00392, 1.00000 ,
    0.23529, 0.00392, 1.00000 ,
    0.23529, 0.00392, 1.00000 ,
    0.47059, 0.00392, 0.78431 ,
    0.47059, 0.00392, 0.78431 ,
    0.47059, 0.00392, 0.78431 ,
    0.47059, 0.00392, 0.78431 ,
    0.47059, 0.00392, 0.78431 ,
    0.47059, 0.00392, 0.78431 ,
    0.47059, 0.00392, 0.78431 ,
    0.47059, 0.00392, 0.78431 ,
    0.47059, 0.00392, 0.78431 ,
    0.47059, 0.00392, 0.78431 ,
    0.62745, 0.00392, 0.62745 ,
    0.62745, 0.00392, 0.62745 ,
    0.62745, 0.00392, 0.62745 ,
    0.62745, 0.00392, 0.62745 ,
    0.62745, 0.00392, 0.62745 ,
    0.62745, 0.00392, 0.62745 ,
    0.62745, 0.00392, 0.62745 ,
    0.62745, 0.00392, 0.62745 ,
    0.62745, 0.00392, 0.62745 ,
    0.62745, 0.00392, 0.62745 ,
    0.78431, 0.00392, 0.47059 ,
    0.78431, 0.00392, 0.47059 ,
    0.78431, 0.00392, 0.47059 ,
    0.78431, 0.00392, 0.47059 ,
    0.78431, 0.00392, 0.47059 ,
    0.78431, 0.00392, 0.47059 ,
    0.78431, 0.00392, 0.47059 ,
    0.78431, 0.00392, 0.47059 ,
    0.78431, 0.00392, 0.47059 ,
    0.78431, 0.00392, 0.47059 ,
    0.90196, 0.11765, 0.23529 ,
    0.90196, 0.11765, 0.23529 ,
    0.90196, 0.11765, 0.23529 ,
    0.90196, 0.11765, 0.23529 ,
    0.90196, 0.11765, 0.23529 ,
    0.90196, 0.11765, 0.23529 ,
    0.90196, 0.11765, 0.23529 ,
    0.90196, 0.11765, 0.23529 ,
    0.90196, 0.11765, 0.23529 ,
    0.90196, 0.11765, 0.23529 ,
    1.00000, 0.23529, 0.00000 ,
    1.00000, 0.23529, 0.00000 ,
    1.00000, 0.23529, 0.00000 ,
    1.00000, 0.23529, 0.00000 ,
    1.00000, 0.23529, 0.00000 ,
    1.00000, 0.23529, 0.00000 ,
    1.00000, 0.23529, 0.00000 ,
    1.00000, 0.23529, 0.00000 ,
    1.00000, 0.23529, 0.00000 ,
    1.00000, 0.23529, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    1.00000, 0.47059, 0.00000 ,
    0.99216, 0.59608, 0.00000 ,
    0.99216, 0.59608, 0.00000 ,
    0.99216, 0.59608, 0.00000 ,
    0.99216, 0.59608, 0.00000 ,
    0.99216, 0.59608, 0.00000 ,
    0.99216, 0.59608, 0.00000 ,
    0.99216, 0.59608, 0.00000 ,
    0.99216, 0.59608, 0.00000 ,
    0.99216, 0.59608, 0.00000 ,
    0.99216, 0.59608, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98431, 0.85098, 0.00000 ,
    0.98431, 0.85098, 0.00000 ,
    0.98431, 0.85098, 0.00000 ,
    0.98431, 0.85098, 0.00000 ,
    0.98431, 0.85098, 0.00000 ,
    0.98431, 0.85098, 0.00000 ,
    0.98431, 0.85098, 0.00000 ,
    0.98431, 0.85098, 0.00000 ,
    0.98431, 0.85098, 0.00000 ,
    0.98431, 0.85098, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.98039, 0.98039, 0.47059 ,
    0.98039, 0.98039, 0.47059 ,
    0.98039, 0.98039, 0.47059 ,
    0.98039, 0.98039, 0.47059 ,
    0.98039, 0.98039, 0.47059 ,
    0.98039, 0.98039, 0.47059 ,
    0.98039, 0.98039, 0.47059 ,
    0.98039, 0.98039, 0.47059 ,
    0.98039, 0.98039, 0.47059 ,
    0.98039, 0.98039, 0.47059 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
    1.00000, 1.00000, 1.00000 ,
};

static Colour_struct rgb[LUT_SIZE] =
{
    0.00000, 0.00000, 0.00000 ,
    0.01176, 0.00000, 0.00000 ,
    0.02745, 0.00000, 0.00000 ,
    0.04314, 0.00000, 0.00000 ,
    0.05882, 0.00000, 0.00000 ,
    0.07451, 0.00000, 0.00000 ,
    0.08627, 0.00000, 0.00000 ,
    0.10196, 0.00000, 0.00000 ,
    0.11765, 0.00000, 0.00000 ,
    0.13333, 0.00000, 0.00000 ,
    0.14902, 0.00000, 0.00000 ,
    0.16078, 0.00000, 0.00000 ,
    0.17647, 0.00000, 0.00000 ,
    0.19216, 0.00000, 0.00000 ,
    0.20784, 0.00000, 0.00000 ,
    0.22353, 0.00000, 0.00000 ,
    0.23529, 0.00000, 0.00000 ,
    0.25098, 0.00000, 0.00000 ,
    0.26667, 0.00000, 0.00000 ,
    0.28235, 0.00000, 0.00000 ,
    0.29804, 0.00000, 0.00000 ,
    0.30980, 0.00000, 0.00000 ,
    0.32549, 0.00000, 0.00000 ,
    0.34118, 0.00000, 0.00000 ,
    0.35686, 0.00000, 0.00000 ,
    0.37255, 0.00000, 0.00000 ,
    0.38431, 0.00000, 0.00000 ,
    0.40000, 0.00000, 0.00000 ,
    0.41569, 0.00000, 0.00000 ,
    0.43137, 0.00000, 0.00000 ,
    0.44706, 0.00000, 0.00000 ,
    0.45882, 0.00000, 0.00000 ,
    0.47451, 0.00000, 0.00000 ,
    0.49020, 0.00000, 0.00000 ,
    0.50588, 0.00000, 0.00000 ,
    0.52157, 0.00000, 0.00000 ,
    0.53725, 0.00000, 0.00000 ,
    0.54902, 0.00000, 0.00000 ,
    0.56471, 0.00000, 0.00000 ,
    0.58039, 0.00000, 0.00000 ,
    0.59608, 0.00000, 0.00000 ,
    0.61176, 0.00000, 0.00000 ,
    0.62353, 0.00000, 0.00000 ,
    0.63922, 0.00000, 0.00000 ,
    0.65490, 0.00000, 0.00000 ,
    0.67059, 0.00000, 0.00000 ,
    0.68627, 0.00000, 0.00000 ,
    0.69804, 0.00000, 0.00000 ,
    0.71373, 0.00000, 0.00000 ,
    0.72941, 0.00000, 0.00000 ,
    0.74510, 0.00000, 0.00000 ,
    0.76078, 0.00000, 0.00000 ,
    0.77255, 0.00000, 0.00000 ,
    0.78824, 0.00000, 0.00000 ,
    0.80392, 0.00000, 0.00000 ,
    0.81961, 0.00000, 0.00000 ,
    0.83529, 0.00000, 0.00000 ,
    0.84706, 0.00000, 0.00000 ,
    0.86275, 0.00000, 0.00000 ,
    0.87843, 0.00000, 0.00000 ,
    0.89412, 0.00000, 0.00000 ,
    0.90980, 0.00000, 0.00000 ,
    0.92157, 0.00000, 0.00000 ,
    0.93725, 0.00000, 0.00000 ,
    0.95294, 0.00000, 0.00000 ,
    0.96863, 0.01176, 0.00000 ,
    0.98431, 0.02745, 0.00000 ,
    1.00000, 0.04314, 0.00000 ,
    0.98431, 0.05882, 0.00000 ,
    0.96863, 0.07451, 0.00000 ,
    0.95294, 0.09020, 0.00000 ,
    0.93725, 0.10588, 0.00000 ,
    0.92157, 0.12157, 0.00000 ,
    0.90196, 0.13725, 0.00000 ,
    0.88627, 0.15294, 0.00000 ,
    0.87059, 0.16863, 0.00000 ,
    0.85490, 0.18431, 0.00000 ,
    0.83922, 0.20000, 0.00000 ,
    0.82353, 0.21569, 0.00000 ,
    0.80392, 0.23137, 0.00000 ,
    0.78824, 0.24706, 0.00000 ,
    0.77255, 0.26275, 0.00000 ,
    0.75686, 0.27843, 0.00000 ,
    0.74118, 0.29412, 0.00000 ,
    0.72157, 0.30980, 0.00000 ,
    0.70588, 0.32549, 0.00000 ,
    0.69020, 0.34118, 0.00000 ,
    0.67451, 0.35686, 0.00000 ,
    0.65882, 0.37255, 0.00000 ,
    0.64314, 0.38824, 0.00000 ,
    0.62353, 0.40392, 0.00000 ,
    0.60784, 0.41961, 0.00000 ,
    0.59216, 0.43529, 0.00000 ,
    0.57647, 0.45098, 0.00000 ,
    0.56078, 0.46667, 0.00000 ,
    0.54118, 0.48235, 0.00000 ,
    0.52549, 0.49804, 0.00000 ,
    0.50980, 0.51373, 0.00000 ,
    0.49412, 0.52941, 0.00000 ,
    0.47843, 0.54510, 0.00000 ,
    0.46275, 0.56078, 0.00000 ,
    0.44314, 0.57647, 0.00000 ,
    0.42745, 0.59216, 0.00000 ,
    0.41176, 0.60784, 0.00000 ,
    0.39608, 0.62353, 0.00000 ,
    0.38039, 0.63922, 0.00000 ,
    0.36078, 0.65490, 0.00000 ,
    0.34510, 0.67059, 0.00000 ,
    0.32941, 0.68627, 0.00000 ,
    0.31373, 0.70196, 0.00000 ,
    0.29804, 0.71765, 0.00000 ,
    0.28235, 0.73333, 0.00000 ,
    0.26275, 0.74902, 0.00000 ,
    0.24706, 0.76471, 0.00000 ,
    0.23137, 0.78039, 0.00000 ,
    0.21569, 0.79608, 0.00000 ,
    0.20000, 0.81176, 0.00000 ,
    0.18039, 0.82745, 0.00000 ,
    0.16471, 0.84314, 0.00000 ,
    0.14902, 0.85882, 0.00000 ,
    0.13333, 0.87451, 0.00000 ,
    0.11765, 0.89020, 0.00000 ,
    0.10196, 0.90588, 0.00000 ,
    0.08235, 0.92157, 0.00000 ,
    0.06667, 0.93725, 0.00000 ,
    0.05098, 0.95294, 0.00000 ,
    0.03529, 0.96863, 0.00000 ,
    0.01961, 0.98431, 0.01176 ,
    0.00000, 1.00000, 0.02745 ,
    0.00000, 0.98431, 0.04314 ,
    0.00000, 0.96863, 0.05882 ,
    0.00000, 0.95294, 0.07451 ,
    0.00000, 0.93725, 0.09020 ,
    0.00000, 0.92157, 0.10588 ,
    0.00000, 0.90588, 0.11765 ,
    0.00000, 0.89020, 0.13333 ,
    0.00000, 0.87451, 0.14902 ,
    0.00000, 0.85882, 0.16471 ,
    0.00000, 0.84314, 0.18039 ,
    0.00000, 0.82745, 0.19608 ,
    0.00000, 0.81176, 0.21176 ,
    0.00000, 0.79608, 0.22353 ,
    0.00000, 0.78039, 0.23922 ,
    0.00000, 0.76471, 0.25490 ,
    0.00000, 0.74902, 0.27059 ,
    0.00000, 0.73333, 0.28627 ,
    0.00000, 0.71765, 0.30196 ,
    0.00000, 0.70196, 0.31765 ,
    0.00000, 0.68627, 0.33333 ,
    0.00000, 0.66667, 0.34510 ,
    0.00000, 0.65098, 0.36078 ,
    0.00000, 0.63529, 0.37647 ,
    0.00000, 0.61961, 0.39216 ,
    0.00000, 0.60392, 0.40784 ,
    0.00000, 0.58824, 0.42353 ,
    0.00000, 0.57255, 0.43922 ,
    0.00000, 0.55686, 0.45098 ,
    0.00000, 0.54118, 0.46667 ,
    0.00000, 0.52549, 0.48235 ,
    0.00000, 0.50980, 0.49804 ,
    0.00000, 0.49412, 0.51373 ,
    0.00000, 0.47843, 0.52941 ,
    0.00000, 0.46275, 0.54510 ,
    0.00000, 0.44706, 0.55686 ,
    0.00000, 0.43137, 0.57255 ,
    0.00000, 0.41569, 0.58824 ,
    0.00000, 0.40000, 0.60392 ,
    0.00000, 0.38431, 0.61961 ,
    0.00000, 0.36863, 0.63529 ,
    0.00000, 0.35294, 0.65098 ,
    0.00000, 0.33333, 0.66667 ,
    0.00000, 0.31765, 0.67843 ,
    0.00000, 0.30196, 0.69412 ,
    0.00000, 0.28627, 0.70980 ,
    0.00000, 0.27059, 0.72549 ,
    0.00000, 0.25490, 0.74118 ,
    0.00000, 0.23922, 0.75686 ,
    0.00000, 0.22353, 0.77255 ,
    0.00000, 0.20784, 0.78431 ,
    0.00000, 0.19216, 0.80000 ,
    0.00000, 0.17647, 0.81569 ,
    0.00000, 0.16078, 0.83137 ,
    0.00000, 0.14510, 0.84706 ,
    0.00000, 0.12941, 0.86275 ,
    0.00000, 0.11373, 0.87843 ,
    0.00000, 0.09804, 0.89020 ,
    0.00000, 0.08235, 0.90588 ,
    0.00000, 0.06667, 0.92157 ,
    0.00000, 0.05098, 0.93725 ,
    0.00000, 0.03529, 0.95294 ,
    0.00000, 0.01961, 0.96863 ,
    0.00000, 0.00000, 0.98431 ,
    0.00000, 0.00000, 1.00000 ,
    0.00000, 0.00000, 0.98431 ,
    0.00000, 0.00000, 0.96863 ,
    0.00000, 0.00000, 0.95294 ,
    0.00000, 0.00000, 0.93725 ,
    0.00000, 0.00000, 0.92157 ,
    0.00000, 0.00000, 0.90588 ,
    0.00000, 0.00000, 0.89020 ,
    0.00000, 0.00000, 0.87451 ,
    0.00000, 0.00000, 0.85882 ,
    0.00000, 0.00000, 0.84314 ,
    0.00000, 0.00000, 0.82745 ,
    0.00000, 0.00000, 0.81176 ,
    0.00000, 0.00000, 0.79608 ,
    0.00000, 0.00000, 0.78039 ,
    0.00000, 0.00000, 0.76471 ,
    0.00000, 0.00000, 0.74902 ,
    0.00000, 0.00000, 0.73333 ,
    0.00000, 0.00000, 0.71765 ,
    0.00000, 0.00000, 0.70196 ,
    0.00000, 0.00000, 0.68627 ,
    0.00000, 0.00000, 0.66667 ,
    0.00000, 0.00000, 0.65098 ,
    0.00000, 0.00000, 0.63529 ,
    0.00000, 0.00000, 0.61961 ,
    0.00000, 0.00000, 0.60392 ,
    0.00000, 0.00000, 0.58824 ,
    0.00000, 0.00000, 0.57255 ,
    0.00000, 0.00000, 0.55686 ,
    0.00000, 0.00000, 0.54118 ,
    0.00000, 0.00000, 0.52549 ,
    0.00000, 0.00000, 0.50980 ,
    0.00000, 0.00000, 0.49412 ,
    0.00000, 0.00000, 0.47843 ,
    0.00000, 0.00000, 0.46275 ,
    0.00000, 0.00000, 0.44706 ,
    0.00000, 0.00000, 0.43137 ,
    0.00000, 0.00000, 0.41569 ,
    0.00000, 0.00000, 0.40000 ,
    0.00000, 0.00000, 0.38431 ,
    0.00000, 0.00000, 0.36863 ,
    0.00000, 0.00000, 0.35294 ,
    0.00000, 0.00000, 0.33333 ,
    0.00000, 0.00000, 0.31765 ,
    0.00000, 0.00000, 0.30196 ,
    0.00000, 0.00000, 0.28627 ,
    0.00000, 0.00000, 0.27059 ,
    0.00000, 0.00000, 0.25490 ,
    0.00000, 0.00000, 0.23922 ,
    0.00000, 0.00000, 0.22353 ,
    0.00000, 0.00000, 0.20784 ,
    0.00000, 0.00000, 0.19216 ,
    0.00000, 0.00000, 0.17647 ,
    0.00000, 0.00000, 0.16078 ,
    0.00000, 0.00000, 0.14510 ,
    0.00000, 0.00000, 0.12941 ,
    0.00000, 0.00000, 0.11373 ,
    0.00000, 0.00000, 0.09804 ,
    0.00000, 0.00000, 0.08235 ,
    0.00000, 0.00000, 0.06667 ,
    0.00000, 0.00000, 0.05098 ,
    0.00000, 0.00000, 0.03529 ,
    0.00000, 0.00000, 0.01961 ,
    0.00000, 0.00000, 0.00000 ,
};

static Colour_struct smooth[LUT_SIZE] =
{
    0.00000, 0.00000, 1.00000 ,
    0.01569, 0.00000, 0.98431 ,
    0.03529, 0.00000, 0.96471 ,
    0.05098, 0.00000, 0.94902 ,
    0.06667, 0.00000, 0.93333 ,
    0.08627, 0.00000, 0.91373 ,
    0.10196, 0.00000, 0.89804 ,
    0.11765, 0.00000, 0.88235 ,
    0.13725, 0.00000, 0.86275 ,
    0.15294, 0.00000, 0.84706 ,
    0.16863, 0.00000, 0.83137 ,
    0.18824, 0.00000, 0.81176 ,
    0.20392, 0.00000, 0.79608 ,
    0.21961, 0.00000, 0.78039 ,
    0.23922, 0.00000, 0.76078 ,
    0.25490, 0.00000, 0.74510 ,
    0.27059, 0.00000, 0.72941 ,
    0.28627, 0.00000, 0.71373 ,
    0.30588, 0.00000, 0.69412 ,
    0.32157, 0.00000, 0.67843 ,
    0.33725, 0.00000, 0.66275 ,
    0.35686, 0.00000, 0.64314 ,
    0.37255, 0.00000, 0.62745 ,
    0.38824, 0.00000, 0.61176 ,
    0.40784, 0.00000, 0.59216 ,
    0.42353, 0.00000, 0.57647 ,
    0.43922, 0.00000, 0.56078 ,
    0.45882, 0.00000, 0.54118 ,
    0.47451, 0.00000, 0.52549 ,
    0.49020, 0.00000, 0.50980 ,
    0.50980, 0.00000, 0.49020 ,
    0.52549, 0.00000, 0.47451 ,
    0.54118, 0.00000, 0.45882 ,
    0.56078, 0.00000, 0.43922 ,
    0.57647, 0.00000, 0.42353 ,
    0.59216, 0.00000, 0.40784 ,
    0.61176, 0.00000, 0.38824 ,
    0.62745, 0.00000, 0.37255 ,
    0.64314, 0.00000, 0.35686 ,
    0.66275, 0.00000, 0.33725 ,
    0.67843, 0.00000, 0.32157 ,
    0.69412, 0.00000, 0.30588 ,
    0.71373, 0.00000, 0.28627 ,
    0.72941, 0.00000, 0.27059 ,
    0.74510, 0.00000, 0.25490 ,
    0.76078, 0.00000, 0.23922 ,
    0.78039, 0.00000, 0.21961 ,
    0.79608, 0.00000, 0.20392 ,
    0.81176, 0.00000, 0.18824 ,
    0.83137, 0.00000, 0.16863 ,
    0.84706, 0.00000, 0.15294 ,
    0.86275, 0.00000, 0.13725 ,
    0.88235, 0.00000, 0.11765 ,
    0.89804, 0.00000, 0.10196 ,
    0.91373, 0.00000, 0.08627 ,
    0.93333, 0.00000, 0.06667 ,
    0.94902, 0.00000, 0.05098 ,
    0.96471, 0.00000, 0.03529 ,
    0.98431, 0.00000, 0.01569 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.00000, 0.00000 ,
    1.00000, 0.01176, 0.00000 ,
    1.00000, 0.01961, 0.00000 ,
    1.00000, 0.03137, 0.00000 ,
    1.00000, 0.03922, 0.00000 ,
    1.00000, 0.05098, 0.00000 ,
    1.00000, 0.05882, 0.00000 ,
    1.00000, 0.07059, 0.00000 ,
    1.00000, 0.08235, 0.00000 ,
    1.00000, 0.09020, 0.00000 ,
    1.00000, 0.10196, 0.00000 ,
    1.00000, 0.10980, 0.00000 ,
    1.00000, 0.12157, 0.00000 ,
    1.00000, 0.12941, 0.00000 ,
    1.00000, 0.14118, 0.00000 ,
    0.99608, 0.15294, 0.00000 ,
    0.99608, 0.16078, 0.00000 ,
    0.99608, 0.17255, 0.00000 ,
    0.99608, 0.18039, 0.00000 ,
    0.99608, 0.19216, 0.00000 ,
    0.99608, 0.20392, 0.00000 ,
    0.99608, 0.21176, 0.00000 ,
    0.99608, 0.22353, 0.00000 ,
    0.99608, 0.23137, 0.00000 ,
    0.99608, 0.24314, 0.00000 ,
    0.99608, 0.25098, 0.00000 ,
    0.99608, 0.26275, 0.00000 ,
    0.99608, 0.27451, 0.00000 ,
    0.99608, 0.28235, 0.00000 ,
    0.99608, 0.29412, 0.00000 ,
    0.99608, 0.30196, 0.00000 ,
    0.99608, 0.31373, 0.00000 ,
    0.99608, 0.32157, 0.00000 ,
    0.99608, 0.33333, 0.00000 ,
    0.99608, 0.34510, 0.00000 ,
    0.99608, 0.35294, 0.00000 ,
    0.99608, 0.36471, 0.00000 ,
    0.99608, 0.37255, 0.00000 ,
    0.99608, 0.38431, 0.00000 ,
    0.99608, 0.39216, 0.00000 ,
    0.99608, 0.40392, 0.00000 ,
    0.99608, 0.41569, 0.00000 ,
    0.99608, 0.42353, 0.00000 ,
    0.99608, 0.43529, 0.00000 ,
    0.99608, 0.44314, 0.00000 ,
    0.99216, 0.45490, 0.00000 ,
    0.99216, 0.46667, 0.00000 ,
    0.99216, 0.47451, 0.00000 ,
    0.99216, 0.48627, 0.00000 ,
    0.99216, 0.49412, 0.00000 ,
    0.99216, 0.50588, 0.00000 ,
    0.99216, 0.51373, 0.00000 ,
    0.99216, 0.52549, 0.00000 ,
    0.99216, 0.53725, 0.00000 ,
    0.99216, 0.54510, 0.00000 ,
    0.99216, 0.55686, 0.00000 ,
    0.99216, 0.56471, 0.00000 ,
    0.99216, 0.57647, 0.00000 ,
    0.99216, 0.58431, 0.00000 ,
    0.99216, 0.59608, 0.00000 ,
    0.99216, 0.60000, 0.00000 ,
    0.99216, 0.60784, 0.00000 ,
    0.99216, 0.61176, 0.00000 ,
    0.99216, 0.61569, 0.00000 ,
    0.99216, 0.61961, 0.00000 ,
    0.99216, 0.62745, 0.00000 ,
    0.99216, 0.63137, 0.00000 ,
    0.99216, 0.63529, 0.00000 ,
    0.99216, 0.64314, 0.00000 ,
    0.98824, 0.64706, 0.00000 ,
    0.98824, 0.65098, 0.00000 ,
    0.98824, 0.65882, 0.00000 ,
    0.98824, 0.66275, 0.00000 ,
    0.98824, 0.66667, 0.00000 ,
    0.98824, 0.67451, 0.00000 ,
    0.98824, 0.67843, 0.00000 ,
    0.98824, 0.68235, 0.00000 ,
    0.98824, 0.68627, 0.00000 ,
    0.98824, 0.69412, 0.00000 ,
    0.98824, 0.69804, 0.00000 ,
    0.98824, 0.70196, 0.00000 ,
    0.98824, 0.70980, 0.00000 ,
    0.98824, 0.71373, 0.00000 ,
    0.98824, 0.71765, 0.00000 ,
    0.98824, 0.72549, 0.00000 ,
    0.98824, 0.72941, 0.00000 ,
    0.98824, 0.73333, 0.00000 ,
    0.98824, 0.73725, 0.00000 ,
    0.98824, 0.74510, 0.00000 ,
    0.98824, 0.74902, 0.00000 ,
    0.98431, 0.75294, 0.00000 ,
    0.98431, 0.76078, 0.00000 ,
    0.98431, 0.76471, 0.00000 ,
    0.98431, 0.76863, 0.00000 ,
    0.98431, 0.77255, 0.00000 ,
    0.98431, 0.78039, 0.00000 ,
    0.98431, 0.78431, 0.00000 ,
    0.98431, 0.78824, 0.00000 ,
    0.98431, 0.79608, 0.00000 ,
    0.98431, 0.80000, 0.00000 ,
    0.98431, 0.80392, 0.00000 ,
    0.98431, 0.81176, 0.00000 ,
    0.98431, 0.81569, 0.00000 ,
    0.98431, 0.81961, 0.00000 ,
    0.98431, 0.82745, 0.00000 ,
    0.98431, 0.83137, 0.00000 ,
    0.98431, 0.83529, 0.00000 ,
    0.98431, 0.83922, 0.00000 ,
    0.98431, 0.84706, 0.00000 ,
    0.98431, 0.85098, 0.00000 ,
    0.98039, 0.85490, 0.00000 ,
    0.98039, 0.86275, 0.00000 ,
    0.98039, 0.86667, 0.00000 ,
    0.98039, 0.87059, 0.00000 ,
    0.98039, 0.87843, 0.00000 ,
    0.98039, 0.88235, 0.00000 ,
    0.98039, 0.88627, 0.00000 ,
    0.98039, 0.89020, 0.00000 ,
    0.98039, 0.89804, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.98039, 0.90196, 0.00000 ,
    0.96471, 0.88627, 0.00000 ,
    0.94902, 0.87059, 0.00000 ,
    0.92941, 0.85490, 0.00000 ,
    0.91373, 0.83922, 0.00000 ,
    0.89804, 0.82745, 0.00000 ,
    0.88235, 0.81176, 0.00000 ,
    0.86275, 0.79608, 0.00000 ,
    0.84706, 0.78039, 0.00000 ,
    0.83137, 0.76471, 0.00000 ,
    0.81569, 0.74902, 0.00000 ,
    0.79608, 0.73333, 0.00000 ,
    0.78039, 0.71765, 0.00000 ,
    0.76471, 0.70196, 0.00000 ,
    0.74902, 0.68627, 0.00000 ,
    0.72941, 0.67451, 0.00000 ,
    0.71373, 0.65882, 0.00000 ,
    0.69804, 0.64314, 0.00000 ,
    0.68235, 0.62745, 0.00000 ,
    0.66275, 0.61176, 0.00000 ,
    0.64706, 0.59608, 0.00000 ,
    0.63137, 0.58039, 0.00000 ,
    0.61569, 0.56471, 0.00000 ,
    0.60000, 0.54902, 0.00000 ,
    0.58039, 0.53333, 0.00000 ,
    0.56471, 0.52157, 0.00000 ,
    0.54902, 0.50588, 0.00000 ,
    0.53333, 0.49020, 0.00000 ,
    0.51373, 0.47451, 0.00000 ,
    0.49804, 0.45882, 0.00000 ,
    0.48235, 0.44314, 0.00000 ,
    0.46667, 0.42745, 0.00000 ,
    0.44706, 0.41176, 0.00000 ,
    0.43137, 0.39608, 0.00000 ,
    0.41569, 0.38039, 0.00000 ,
    0.40000, 0.36863, 0.00000 ,
    0.38039, 0.35294, 0.00000 ,
    0.36471, 0.33725, 0.00000 ,
    0.34902, 0.32157, 0.00000 ,
    0.33333, 0.30588, 0.00000 ,
    0.31765, 0.29020, 0.00000 ,
    0.29804, 0.27451, 0.00000 ,
    0.28235, 0.25882, 0.00000 ,
    0.26667, 0.24314, 0.00000 ,
    0.25098, 0.22745, 0.00000 ,
    0.23137, 0.21569, 0.00000 ,
    0.21569, 0.20000, 0.00000 ,
    0.20000, 0.18431, 0.00000 ,
    0.18431, 0.16863, 0.00000 ,
    0.16471, 0.15294, 0.00000 ,
    0.14902, 0.13725, 0.00000 ,
    0.13333, 0.12157, 0.00000 ,
    0.11765, 0.10588, 0.00000 ,
    0.09804, 0.09020, 0.00000 ,
    0.08235, 0.07451, 0.00000 ,
    0.06667, 0.06275, 0.00000 ,
    0.05098, 0.04706, 0.00000 ,
    0.03137, 0.03137, 0.00000 ,
    0.01569, 0.01569, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
    0.00000, 0.00000, 0.00000 ,
};

static Colour_struct staircase[LUT_SIZE] =
{
    0.00392, 0.00392, 0.31373 ,
    0.00784, 0.00784, 0.31373 ,
    0.01176, 0.01176, 0.31373 ,
    0.01569, 0.01569, 0.31373 ,
    0.01961, 0.01961, 0.31373 ,
    0.02353, 0.02353, 0.31373 ,
    0.02745, 0.02745, 0.31373 ,
    0.03137, 0.03137, 0.31373 ,
    0.03529, 0.03529, 0.31373 ,
    0.03922, 0.03922, 0.31373 ,
    0.04314, 0.04314, 0.31373 ,
    0.04706, 0.04706, 0.31373 ,
    0.05098, 0.05098, 0.31373 ,
    0.05490, 0.05490, 0.31373 ,
    0.05882, 0.05882, 0.31373 ,
    0.06275, 0.06275, 0.31373 ,
    0.06667, 0.06667, 0.47059 ,
    0.07059, 0.07059, 0.47059 ,
    0.07451, 0.07451, 0.47059 ,
    0.07843, 0.07843, 0.47059 ,
    0.08235, 0.08235, 0.47059 ,
    0.08627, 0.08627, 0.47059 ,
    0.09020, 0.09020, 0.47059 ,
    0.09412, 0.09412, 0.47059 ,
    0.09804, 0.09804, 0.47059 ,
    0.10196, 0.10196, 0.47059 ,
    0.10588, 0.10588, 0.47059 ,
    0.10980, 0.10980, 0.47059 ,
    0.11373, 0.11373, 0.47059 ,
    0.11765, 0.11765, 0.47059 ,
    0.12157, 0.12157, 0.47059 ,
    0.12549, 0.12549, 0.47059 ,
    0.12941, 0.12941, 0.62745 ,
    0.13333, 0.13333, 0.62745 ,
    0.13725, 0.13725, 0.62745 ,
    0.14118, 0.14118, 0.62745 ,
    0.14510, 0.14510, 0.62745 ,
    0.14902, 0.14902, 0.62745 ,
    0.15294, 0.15294, 0.62745 ,
    0.15686, 0.15686, 0.62745 ,
    0.16078, 0.16078, 0.62745 ,
    0.16471, 0.16471, 0.62745 ,
    0.16863, 0.16863, 0.62745 ,
    0.17255, 0.17255, 0.62745 ,
    0.17647, 0.17647, 0.62745 ,
    0.18039, 0.18039, 0.62745 ,
    0.18431, 0.18431, 0.62745 ,
    0.18824, 0.18824, 0.62745 ,
    0.19216, 0.19216, 0.78431 ,
    0.19608, 0.19608, 0.78431 ,
    0.20000, 0.20000, 0.78431 ,
    0.20392, 0.20392, 0.78431 ,
    0.20784, 0.20784, 0.78431 ,
    0.21176, 0.21176, 0.78431 ,
    0.21569, 0.21569, 0.78431 ,
    0.21961, 0.21961, 0.78431 ,
    0.22353, 0.22353, 0.78431 ,
    0.22745, 0.22745, 0.78431 ,
    0.23137, 0.23137, 0.78431 ,
    0.23529, 0.23529, 0.78431 ,
    0.23922, 0.23922, 0.78431 ,
    0.24314, 0.24314, 0.78431 ,
    0.24706, 0.24706, 0.78431 ,
    0.25098, 0.25098, 0.78431 ,
    0.25490, 0.25490, 0.94118 ,
    0.25882, 0.25882, 0.94118 ,
    0.26275, 0.26275, 0.94118 ,
    0.26667, 0.26667, 0.94118 ,
    0.27059, 0.27059, 0.94118 ,
    0.27451, 0.27451, 0.94118 ,
    0.27843, 0.27843, 0.94118 ,
    0.28235, 0.28235, 0.94118 ,
    0.28627, 0.28627, 0.94118 ,
    0.29020, 0.29020, 0.94118 ,
    0.29412, 0.29412, 0.94118 ,
    0.29804, 0.29804, 0.94118 ,
    0.30196, 0.30196, 0.94118 ,
    0.30588, 0.30588, 0.94118 ,
    0.30980, 0.30980, 0.94118 ,
    0.31373, 0.31373, 0.94118 ,
    0.31765, 0.31765, 0.95294 ,
    0.32157, 0.32157, 0.96471 ,
    0.32549, 0.32549, 0.97647 ,
    0.32941, 0.32941, 0.98824 ,
    0.33333, 0.33333, 1.00000 ,
    0.00392, 0.31373, 0.00392 ,
    0.00784, 0.31373, 0.00784 ,
    0.01176, 0.31373, 0.01176 ,
    0.01569, 0.31373, 0.01569 ,
    0.01961, 0.31373, 0.01961 ,
    0.02353, 0.31373, 0.02353 ,
    0.02745, 0.31373, 0.02745 ,
    0.03137, 0.31373, 0.03137 ,
    0.03529, 0.31373, 0.03529 ,
    0.03922, 0.31373, 0.03922 ,
    0.04314, 0.31373, 0.04314 ,
    0.04706, 0.31373, 0.04706 ,
    0.05098, 0.31373, 0.05098 ,
    0.05490, 0.31373, 0.05490 ,
    0.05882, 0.31373, 0.05882 ,
    0.06275, 0.31373, 0.06275 ,
    0.06667, 0.47059, 0.06667 ,
    0.07059, 0.47059, 0.07059 ,
    0.07451, 0.47059, 0.07451 ,
    0.07843, 0.47059, 0.07843 ,
    0.08235, 0.47059, 0.08235 ,
    0.08627, 0.47059, 0.08627 ,
    0.09020, 0.47059, 0.09020 ,
    0.09412, 0.47059, 0.09412 ,
    0.09804, 0.47059, 0.09804 ,
    0.10196, 0.47059, 0.10196 ,
    0.10588, 0.47059, 0.10588 ,
    0.10980, 0.47059, 0.10980 ,
    0.11373, 0.47059, 0.11373 ,
    0.11765, 0.47059, 0.11765 ,
    0.12157, 0.47059, 0.12157 ,
    0.12549, 0.47059, 0.12549 ,
    0.12941, 0.62745, 0.12941 ,
    0.13333, 0.62745, 0.13333 ,
    0.13725, 0.62745, 0.13725 ,
    0.14118, 0.62745, 0.14118 ,
    0.14510, 0.62745, 0.14510 ,
    0.14902, 0.62745, 0.14902 ,
    0.15294, 0.62745, 0.15294 ,
    0.15686, 0.62745, 0.15686 ,
    0.16078, 0.62745, 0.16078 ,
    0.16471, 0.62745, 0.16471 ,
    0.16863, 0.62745, 0.16863 ,
    0.17255, 0.62745, 0.17255 ,
    0.17647, 0.62745, 0.17647 ,
    0.18039, 0.62745, 0.18039 ,
    0.18431, 0.62745, 0.18431 ,
    0.18824, 0.62745, 0.18824 ,
    0.19216, 0.78431, 0.19216 ,
    0.19608, 0.78431, 0.19608 ,
    0.20000, 0.78431, 0.20000 ,
    0.20392, 0.78431, 0.20392 ,
    0.20784, 0.78431, 0.20784 ,
    0.21176, 0.78431, 0.21176 ,
    0.21569, 0.78431, 0.21569 ,
    0.21961, 0.78431, 0.21961 ,
    0.22353, 0.78431, 0.22353 ,
    0.22745, 0.78431, 0.22745 ,
    0.23137, 0.78431, 0.23137 ,
    0.23529, 0.78431, 0.23529 ,
    0.23922, 0.78431, 0.23922 ,
    0.24314, 0.78431, 0.24314 ,
    0.24706, 0.78431, 0.24706 ,
    0.25098, 0.78431, 0.25098 ,
    0.25490, 0.94118, 0.25490 ,
    0.25882, 0.94118, 0.25882 ,
    0.26275, 0.94118, 0.26275 ,
    0.26667, 0.94118, 0.26667 ,
    0.27059, 0.94118, 0.27059 ,
    0.27451, 0.94118, 0.27451 ,
    0.27843, 0.94118, 0.27843 ,
    0.28235, 0.94118, 0.28235 ,
    0.28627, 0.94118, 0.28627 ,
    0.29020, 0.94118, 0.29020 ,
    0.29412, 0.94118, 0.29412 ,
    0.29804, 0.94118, 0.29804 ,
    0.30196, 0.94118, 0.30196 ,
    0.30588, 0.94118, 0.30588 ,
    0.30980, 0.94118, 0.30980 ,
    0.31373, 0.94118, 0.31373 ,
    0.31765, 0.95294, 0.31765 ,
    0.32157, 0.96471, 0.32157 ,
    0.32549, 0.97647, 0.32549 ,
    0.32941, 0.98824, 0.32941 ,
    0.33333, 1.00000, 0.33333 ,
    0.31373, 0.00392, 0.00392 ,
    0.31373, 0.00784, 0.00784 ,
    0.31373, 0.01176, 0.01176 ,
    0.31373, 0.01569, 0.01569 ,
    0.31373, 0.01961, 0.01961 ,
    0.31373, 0.02353, 0.02353 ,
    0.31373, 0.02745, 0.02745 ,
    0.31373, 0.03137, 0.03137 ,
    0.31373, 0.03529, 0.03529 ,
    0.31373, 0.03922, 0.03922 ,
    0.31373, 0.04314, 0.04314 ,
    0.31373, 0.04706, 0.04706 ,
    0.31373, 0.05098, 0.05098 ,
    0.31373, 0.05490, 0.05490 ,
    0.31373, 0.05882, 0.05882 ,
    0.31373, 0.06275, 0.06275 ,
    0.47059, 0.06667, 0.06667 ,
    0.47059, 0.07059, 0.07059 ,
    0.47059, 0.07451, 0.07451 ,
    0.47059, 0.07843, 0.07843 ,
    0.47059, 0.08235, 0.08235 ,
    0.47059, 0.08627, 0.08627 ,
    0.47059, 0.09020, 0.09020 ,
    0.47059, 0.09412, 0.09412 ,
    0.47059, 0.09804, 0.09804 ,
    0.47059, 0.10196, 0.10196 ,
    0.47059, 0.10588, 0.10588 ,
    0.47059, 0.10980, 0.10980 ,
    0.47059, 0.11373, 0.11373 ,
    0.47059, 0.11765, 0.11765 ,
    0.47059, 0.12157, 0.12157 ,
    0.47059, 0.12549, 0.12549 ,
    0.62745, 0.12941, 0.12941 ,
    0.62745, 0.13333, 0.13333 ,
    0.62745, 0.13725, 0.13725 ,
    0.62745, 0.14118, 0.14118 ,
    0.62745, 0.14510, 0.14510 ,
    0.62745, 0.14902, 0.14902 ,
    0.62745, 0.15294, 0.15294 ,
    0.62745, 0.15686, 0.15686 ,
    0.62745, 0.16078, 0.16078 ,
    0.62745, 0.16471, 0.16471 ,
    0.62745, 0.16863, 0.16863 ,
    0.62745, 0.17255, 0.17255 ,
    0.62745, 0.17647, 0.17647 ,
    0.62745, 0.18039, 0.18039 ,
    0.62745, 0.18431, 0.18431 ,
    0.62745, 0.18824, 0.18824 ,
    0.78431, 0.19216, 0.19216 ,
    0.78431, 0.19608, 0.19608 ,
    0.78431, 0.20000, 0.20000 ,
    0.78431, 0.20392, 0.20392 ,
    0.78431, 0.20784, 0.20784 ,
    0.78431, 0.21176, 0.21176 ,
    0.78431, 0.21569, 0.21569 ,
    0.78431, 0.21961, 0.21961 ,
    0.78431, 0.22353, 0.22353 ,
    0.78431, 0.22745, 0.22745 ,
    0.78431, 0.23137, 0.23137 ,
    0.78431, 0.23529, 0.23529 ,
    0.78431, 0.23922, 0.23922 ,
    0.78431, 0.24314, 0.24314 ,
    0.78431, 0.24706, 0.24706 ,
    0.78431, 0.25098, 0.25098 ,
    0.94118, 0.25490, 0.25490 ,
    0.94118, 0.25882, 0.25882 ,
    0.94118, 0.26275, 0.26275 ,
    0.94118, 0.26667, 0.26667 ,
    0.94118, 0.27059, 0.27059 ,
    0.94118, 0.27451, 0.27451 ,
    0.94118, 0.27843, 0.27843 ,
    0.94118, 0.28235, 0.28235 ,
    0.94118, 0.28627, 0.28627 ,
    0.94118, 0.29020, 0.29020 ,
    0.94118, 0.29412, 0.29412 ,
    0.94118, 0.29804, 0.29804 ,
    0.94118, 0.30196, 0.30196 ,
    0.94118, 0.30588, 0.30588 ,
    0.94118, 0.30980, 0.30980 ,
    0.94118, 0.31373, 0.31373 ,
    0.94902, 0.39216, 0.39216 ,
    0.96078, 0.52941, 0.52941 ,
    0.97255, 0.66667, 0.66667 ,
    0.98431, 0.80392, 0.80392 ,
    0.99216, 0.80000, 0.80000 ,
    1.00000, 1.00000, 1.00000 ,
};

static void colour_table (num_cells, reds, greens, blues, stride, x, y, lut)
unsigned int      num_cells;
unsigned short    *reds;
unsigned short    *greens;
unsigned short    *blues;
unsigned int      stride;
double            x;
double            y;
Colour_struct      lut[];
{

    unsigned int      pixel_count;
    int               m;
    double            shift, slope, xx, yy;

    if ((x < 0.0) || (x > 1.0) || (y < 0.0) || (y > 1.0))
    {
	shift = 0.5;
	slope = 1.0;
    }
    else {
	shift = x;
	slope = tan(1.57*y);
    }


    for (pixel_count = 1;  pixel_count <= num_cells ; pixel_count++)
    {
	xx = 1.0 / (float) (num_cells+1) * (pixel_count);
	yy = slope * (xx-shift) + 0.5;
	if (yy < 0.0)
	{
	    yy = 0.0;
	} else
	{
	    if (yy > 1.0)
	    {
		yy = 1.0;
	    }
	}

	m = ( (float) (LUT_SIZE-2) * yy + 1.5 );
	if (m >= LUT_SIZE) 
	m = LUT_SIZE-1;
	reds[(pixel_count-1) * stride]    = lut[m].red*MAX_INTENSITY;
	blues[(pixel_count-1) * stride]   = lut[m].blue*MAX_INTENSITY;
	greens[(pixel_count-1) * stride]  = lut[m].green*MAX_INTENSITY;
    }
}

/*PUBLIC_FUNCTION*/
void cf_background (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
{
    extern Colour_struct   background[];

    colour_table (num_cells, reds, greens, blues, stride, x, y, background);
}

/*PUBLIC_FUNCTION*/
void cf_heat (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
{
    extern Colour_struct   heat[];
  
    colour_table (num_cells, reds, greens, blues, stride, x, y, heat);
}

/*PUBLIC_FUNCTION*/
void cf_isophot (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
{
    extern Colour_struct   isophot[];

    colour_table (num_cells, reds, greens, blues, stride, x, y, isophot);
}

/*PUBLIC_FUNCTION*/
void cf_mono (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
{
    extern Colour_struct   mono[];

    colour_table (num_cells, reds, greens, blues, stride, x, y, mono);
}

/*PUBLIC_FUNCTION*/
void cf_mousse (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
{
    extern Colour_struct   mousse[];

    colour_table (num_cells, reds, greens, blues, stride, x, y, mousse);
}

/*PUBLIC_FUNCTION*/
void cf_rainbow (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
{
    extern Colour_struct   rainbow[];

    colour_table (num_cells, reds, greens, blues, stride, x, y, rainbow);
}

/*PUBLIC_FUNCTION*/
void cf_random (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
{
    extern Colour_struct   random[];

    colour_table (num_cells, reds, greens, blues, stride, x, y, random);
}

/*PUBLIC_FUNCTION*/
void cf_rgb (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
{
    extern Colour_struct   rgb[];

    colour_table (num_cells, reds, greens, blues, stride, x, y, rgb);
}

/*PUBLIC_FUNCTION*/
void cf_smooth (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
{
    extern Colour_struct   smooth[];

    colour_table (num_cells, reds, greens, blues, stride, x, y, smooth);
}

/*PUBLIC_FUNCTION*/
void cf_staircase (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
{
    extern Colour_struct   staircase[];

    colour_table (num_cells, reds, greens, blues, stride, x, y, staircase);
}

/*PUBLIC_FUNCTION*/
void cf_ronekers (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
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
}

static Colour_struct mirp[LUT_SIZE] =
{
     0.000000, 0.000000, 1.000000,
     0.000000, 0.060295, 1.000000,
     0.000000, 0.112233, 1.000000,
     0.000000, 0.157963, 1.000000,
     0.000000, 0.198902, 1.000000,
     0.000000, 0.236033, 1.000000,
     0.000000, 0.270066, 1.000000,
     0.000000, 0.301531, 1.000000,
     0.000000, 0.330834, 1.000000,
     0.000000, 0.358294, 1.000000,
     0.000000, 0.384164, 1.000000,
     0.000000, 0.408651, 1.000000,
     0.000000, 0.431924, 1.000000,
     0.000000, 0.454124, 1.000000,
     0.000000, 0.475368, 1.000000,
     0.000000, 0.495760, 1.000000,
     0.000000, 0.515384, 1.000000,
     0.000000, 0.534315, 1.000000,
     0.000000, 0.552619, 1.000000,
     0.000000, 0.570353, 1.000000,
     0.000000, 0.587566, 1.000000,
     0.000000, 0.604304, 1.000000,
     0.000000, 0.620605, 1.000000,
     0.000000, 0.636507, 1.000000,
     0.000000, 0.652040, 1.000000,
     0.000000, 0.667234, 1.000000,
     0.000000, 0.682114, 1.000000,
     0.000000, 0.696706, 1.000000,
     0.000000, 0.711030, 1.000000,
     0.000000, 0.725108, 1.000000,
     0.000000, 0.738957, 1.000000,
     0.000000, 0.752595, 1.000000,
     0.000000, 0.766038, 1.000000,
     0.000000, 0.779300, 1.000000,
     0.000000, 0.792396, 1.000000,
     0.000000, 0.805338, 1.000000,
     0.000000, 0.818139, 1.000000,
     0.000000, 0.830811, 1.000000,
     0.000000, 0.843364, 1.000000,
     0.000000, 0.855809, 1.000000,
     0.000000, 0.868156, 1.000000,
     0.000000, 0.880414, 1.000000,
     0.000000, 0.892592, 1.000000,
     0.000000, 0.904700, 1.000000,
     0.000000, 0.916746, 1.000000,
     0.000000, 0.928738, 1.000000,
     0.000000, 0.940684, 1.000000,
     0.000000, 0.952592, 1.000000,
     0.000000, 0.964470, 1.000000,
     0.000000, 0.976326, 1.000000,
     0.000000, 0.988167, 1.000000,
     0.000000, 1.000000, 1.000000,
     0.000000, 1.000000, 0.988167,
     0.000000, 1.000000, 0.976326,
     0.000000, 1.000000, 0.964470,
     0.000000, 1.000000, 0.952592,
     0.000000, 1.000000, 0.940684,
     0.000000, 1.000000, 0.928738,
     0.000000, 1.000000, 0.916746,
     0.000000, 1.000000, 0.904700,
     0.000000, 1.000000, 0.892592,
     0.000000, 1.000000, 0.880414,
     0.000000, 1.000000, 0.868156,
     0.000000, 1.000000, 0.855809,
     0.000000, 1.000000, 0.843364,
     0.000000, 1.000000, 0.830811,
     0.000000, 1.000000, 0.818139,
     0.000000, 1.000000, 0.805338,
     0.000000, 1.000000, 0.792396,
     0.000000, 1.000000, 0.779300,
     0.000000, 1.000000, 0.766038,
     0.000000, 1.000000, 0.752595,
     0.000000, 1.000000, 0.738957,
     0.000000, 1.000000, 0.725108,
     0.000000, 1.000000, 0.711030,
     0.000000, 1.000000, 0.696706,
     0.000000, 1.000000, 0.682114,
     0.000000, 1.000000, 0.667234,
     0.000000, 1.000000, 0.652040,
     0.000000, 1.000000, 0.636507,
     0.000000, 1.000000, 0.620605,
     0.000000, 1.000000, 0.604304,
     0.000000, 1.000000, 0.587566,
     0.000000, 1.000000, 0.570353,
     0.000000, 1.000000, 0.552619,
     0.000000, 1.000000, 0.534315,
     0.000000, 1.000000, 0.515384,
     0.000000, 1.000000, 0.495760,
     0.000000, 1.000000, 0.475368,
     0.000000, 1.000000, 0.454124,
     0.000000, 1.000000, 0.431924,
     0.000000, 1.000000, 0.408651,
     0.000000, 1.000000, 0.384164,
     0.000000, 1.000000, 0.358294,
     0.000000, 1.000000, 0.330834,
     0.000000, 1.000000, 0.301531,
     0.000000, 1.000000, 0.270066,
     0.000000, 1.000000, 0.236033,
     0.000000, 1.000000, 0.198902,
     0.000000, 1.000000, 0.157963,
     0.000000, 1.000000, 0.112233,
     0.000000, 1.000000, 0.060295,
     0.000000, 1.000000, 0.000000,
     0.060295, 1.000000, 0.000000,
     0.112233, 1.000000, 0.000000,
     0.157963, 1.000000, 0.000000,
     0.198902, 1.000000, 0.000000,
     0.236033, 1.000000, 0.000000,
     0.270066, 1.000000, 0.000000,
     0.301531, 1.000000, 0.000000,
     0.330834, 1.000000, 0.000000,
     0.358294, 1.000000, 0.000000,
     0.384164, 1.000000, 0.000000,
     0.408651, 1.000000, 0.000000,
     0.431924, 1.000000, 0.000000,
     0.454124, 1.000000, 0.000000,
     0.475368, 1.000000, 0.000000,
     0.495760, 1.000000, 0.000000,
     0.515384, 1.000000, 0.000000,
     0.534315, 1.000000, 0.000000,
     0.552619, 1.000000, 0.000000,
     0.570353, 1.000000, 0.000000,
     0.587566, 1.000000, 0.000000,
     0.604304, 1.000000, 0.000000,
     0.620605, 1.000000, 0.000000,
     0.636507, 1.000000, 0.000000,
     0.652040, 1.000000, 0.000000,
     0.667234, 1.000000, 0.000000,
     0.682114, 1.000000, 0.000000,
     0.696706, 1.000000, 0.000000,
     0.711030, 1.000000, 0.000000,
     0.725108, 1.000000, 0.000000,
     0.738957, 1.000000, 0.000000,
     0.752595, 1.000000, 0.000000,
     0.766038, 1.000000, 0.000000,
     0.779300, 1.000000, 0.000000,
     0.792396, 1.000000, 0.000000,
     0.805338, 1.000000, 0.000000,
     0.818139, 1.000000, 0.000000,
     0.830811, 1.000000, 0.000000,
     0.843364, 1.000000, 0.000000,
     0.855809, 1.000000, 0.000000,
     0.868156, 1.000000, 0.000000,
     0.880414, 1.000000, 0.000000,
     0.892592, 1.000000, 0.000000,
     0.904700, 1.000000, 0.000000,
     0.916746, 1.000000, 0.000000,
     0.928738, 1.000000, 0.000000,
     0.940684, 1.000000, 0.000000,
     0.952592, 1.000000, 0.000000,
     0.964470, 1.000000, 0.000000,
     0.976326, 1.000000, 0.000000,
     0.988167, 1.000000, 0.000000,
     1.000000, 1.000000, 0.000000,
     1.000000, 0.988167, 0.000000,
     1.000000, 0.976326, 0.000000,
     1.000000, 0.964470, 0.000000,
     1.000000, 0.952592, 0.000000,
     1.000000, 0.940684, 0.000000,
     1.000000, 0.928738, 0.000000,
     1.000000, 0.916746, 0.000000,
     1.000000, 0.904700, 0.000000,
     1.000000, 0.892592, 0.000000,
     1.000000, 0.880414, 0.000000,
     1.000000, 0.868156, 0.000000,
     1.000000, 0.855809, 0.000000,
     1.000000, 0.843364, 0.000000,
     1.000000, 0.830811, 0.000000,
     1.000000, 0.818139, 0.000000,
     1.000000, 0.805338, 0.000000,
     1.000000, 0.792396, 0.000000,
     1.000000, 0.779300, 0.000000,
     1.000000, 0.766038, 0.000000,
     1.000000, 0.752595, 0.000000,
     1.000000, 0.738957, 0.000000,
     1.000000, 0.725108, 0.000000,
     1.000000, 0.711030, 0.000000,
     1.000000, 0.696706, 0.000000,
     1.000000, 0.682114, 0.000000,
     1.000000, 0.667234, 0.000000,
     1.000000, 0.652040, 0.000000,
     1.000000, 0.636507, 0.000000,
     1.000000, 0.620605, 0.000000,
     1.000000, 0.604304, 0.000000,
     1.000000, 0.587566, 0.000000,
     1.000000, 0.570353, 0.000000,
     1.000000, 0.552619, 0.000000,
     1.000000, 0.534315, 0.000000,
     1.000000, 0.515384, 0.000000,
     1.000000, 0.495760, 0.000000,
     1.000000, 0.475368, 0.000000,
     1.000000, 0.454124, 0.000000,
     1.000000, 0.431924, 0.000000,
     1.000000, 0.408651, 0.000000,
     1.000000, 0.384164, 0.000000,
     1.000000, 0.358294, 0.000000,
     1.000000, 0.330834, 0.000000,
     1.000000, 0.301531, 0.000000,
     1.000000, 0.270066, 0.000000,
     1.000000, 0.236033, 0.000000,
     1.000000, 0.198902, 0.000000,
     1.000000, 0.157963, 0.000000,
     1.000000, 0.112233, 0.000000,
     1.000000, 0.060295, 0.000000,
     1.000000, 0.000000, 0.000000,
     1.000000, 0.000000, 0.060295,
     1.000000, 0.000000, 0.112233,
     1.000000, 0.000000, 0.157963,
     1.000000, 0.000000, 0.198902,
     1.000000, 0.000000, 0.236033,
     1.000000, 0.000000, 0.270066,
     1.000000, 0.000000, 0.301531,
     1.000000, 0.000000, 0.330834,
     1.000000, 0.000000, 0.358294,
     1.000000, 0.000000, 0.384164,
     1.000000, 0.000000, 0.408651,
     1.000000, 0.000000, 0.431924,
     1.000000, 0.000000, 0.454124,
     1.000000, 0.000000, 0.475368,
     1.000000, 0.000000, 0.495760,
     1.000000, 0.000000, 0.515384,
     1.000000, 0.000000, 0.534315,
     1.000000, 0.000000, 0.552619,
     1.000000, 0.000000, 0.570353,
     1.000000, 0.000000, 0.587566,
     1.000000, 0.000000, 0.604304,
     1.000000, 0.000000, 0.620605,
     1.000000, 0.000000, 0.636507,
     1.000000, 0.000000, 0.652040,
     1.000000, 0.000000, 0.667234,
     1.000000, 0.000000, 0.682114,
     1.000000, 0.000000, 0.696706,
     1.000000, 0.000000, 0.711030,
     1.000000, 0.000000, 0.725108,
     1.000000, 0.000000, 0.738957,
     1.000000, 0.000000, 0.752595,
     1.000000, 0.000000, 0.766038,
     1.000000, 0.000000, 0.779300,
     1.000000, 0.000000, 0.792396,
     1.000000, 0.000000, 0.805338,
     1.000000, 0.000000, 0.818139,
     1.000000, 0.000000, 0.830811,
     1.000000, 0.000000, 0.843364,
     1.000000, 0.000000, 0.855809,
     1.000000, 0.000000, 0.868156,
     1.000000, 0.000000, 0.880414,
     1.000000, 0.000000, 0.892592,
     1.000000, 0.000000, 0.904700,
     1.000000, 0.000000, 0.916746,
     1.000000, 0.000000, 0.928738,
     1.000000, 0.000000, 0.940684,
     1.000000, 0.000000, 0.952592,
     1.000000, 0.000000, 0.964470,
     1.000000, 0.000000, 0.976326,
     1.000000, 0.000000, 0.988167,
     1.000000, 0.000000, 1.000000,
};

static void colour_table_mirp (num_cells, reds, greens, blues, stride, x, y,
			       lut)
unsigned int      num_cells;
unsigned short    *reds;
unsigned short    *greens;
unsigned short    *blues;
unsigned int      stride;
double            x;
double            y;
Colour_struct      lut[];
{

    unsigned int      pixel_count;
    int               m;
    double            shift, slope, xx, yy;

    if ((x < 0.0) || (x > 1.0) || (y < 0.0) || (y > 1.0))
    {
	shift = 0.5;
	slope = 1.0;
    }
    else {
	shift = x;
	slope = tan(PI_ON_2*y);
    }


    for (pixel_count = 0;  pixel_count < num_cells ; pixel_count++)
    {
	xx = 1.0 / (float) (num_cells-1) * (pixel_count);
	yy = slope * (xx-shift) + 0.5;
	if (yy < 0.0)
	{
	    yy = 0.0;
	} else
	{
	    if (yy > 1.0)
	    {
		yy = 1.0;
	    }
	}

	m = ( (float)(LUT_SIZE-1) * yy + 0.5 );
	if (m >= LUT_SIZE) 
	m = LUT_SIZE-1;
	reds[pixel_count * stride]    = lut[m].red*MAX_INTENSITY;
	blues[pixel_count * stride]   = lut[m].blue*MAX_INTENSITY;
	greens[pixel_count * stride]  = lut[m].green*MAX_INTENSITY;
    }
}

/*PUBLIC_FUNCTION*/
void cf_mirp (num_cells, reds, greens, blues, stride, x, y, var_param)
unsigned int          num_cells;
unsigned short        *reds;
unsigned short        *greens;
unsigned short        *blues;
unsigned int          stride;
double                x;
double                y;
void                  *var_param;
{
    extern Colour_struct   mirp[];

    colour_table_mirp (num_cells, reds, greens, blues, stride, x, y, mirp);
}

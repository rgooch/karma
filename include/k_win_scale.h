/*
    Definition of  win_scale  structure.

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

#ifndef K_WIN_SCALE_H
#define K_WIN_SCALE_H

#ifndef KARMA_H
#  include <karma.h>
#endif

#define K_INTENSITY_SCALE_LINEAR (unsigned int) 0
#define K_INTENSITY_SCALE_LOGARITHMIC (unsigned int) 1
#define K_INTENSITY_SCALE_SQUARE_ROOT (unsigned int) 2
#define NUM_INTENSITY_SCALE_ALTERNATIVES (unsigned int) 3

#define KIMAGE_COMPLEX_CONV_REAL (unsigned int) 0
#define KIMAGE_COMPLEX_CONV_IMAG (unsigned int) 1
#define KIMAGE_COMPLEX_CONV_ABS (unsigned int) 2
#define KIMAGE_COMPLEX_CONV_SQUARE_ABS (unsigned int) 3
#define KIMAGE_COMPLEX_CONV_PHASE (unsigned int) 4
#define KIMAGE_COMPLEX_CONV_CONT_PHASE (unsigned int) 5
#define KIMAGE_NUM_COMPLEX_CONVERSIONS (unsigned int) 6

#define K_WIN_SCALE_MAGIC_NUMBER (unsigned int) 238943488

/*  This structure is rather, er, historic. I plan to slowly phase out this
    structure.
*/
struct win_scale_type
{
    unsigned int magic_number;
    int x_offset;                /* These fields should be seen as attibutes */
    int y_offset;                /* for the KWorldCanvas class               */
    int x_pixels;
    int y_pixels;
    unsigned long blank_pixel;   /* These fields relate to intensity mapping */
    unsigned long min_sat_pixel;
    unsigned long max_sat_pixel;
    double x_min;                /* And these relate to co-ordinate mapping  */
    double x_max;
    double y_min;
    double y_max;
    flag x_log;                  /* These 2 will be phased out               */
    flag y_log;
    double z_min;                /* Back to intensity mapping                */
    double z_max;
    unsigned int z_scale;
    unsigned int conv_type;
    flag flip_horizontal;       /* These fields are a convenience for the    */
    flag flip_vertical;         /* ViewableImage class to flip the image     */
};

struct scale_type
{
    double min;
    double max;
    unsigned int num_ticks;
    unsigned int num_major_ticks;
    int first_tick_num;
    double first_major_tick;
    double tick_interval;
};

#endif  /*  K_WIN_SCALE_H  */

/*  kview_2d.h

    Header file for  kview_2d  (X11 image display tool for Karma).

    Copyright (C) 1993  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*
    This Karma module will enable on-screen imaging of a single arrayfile of
    the general data structure format. The arrayfile must contain a 2
    dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   12-JUL-1993

    Last updated by Richard Gooch   28-SEP-1993


*/
#define MAX_ARRAYS 10
#define MAX_ARRAYFILES 100
#define MAX_RESTRICTIONS 100
#define MAX_ELEMENTS 10
#define DEFAULT_MAX_LOG_CYCLES (unsigned int) 8
#define DEFAULT_ORD_SCALE_FOR_ZEROS (double) 1e-25
#define MAX_DISPLAY_COLOURS (unsigned int) 256
#define NUM_PROTOCOLS (unsigned int) 3
#define DEFAULT_NUMBER_OF_COLOURS (unsigned int) 200
#define DEFAULT_CMAP_FILENAME "kview_2d.cmap"

#define CONV1_ABS_INV (NUM_CONV1_ALTERNATIVES + 1)

#define AXES_SCALE_LINEAR (unsigned int) 0
#define AXES_SCALE_LOGARITHMIC (unsigned int) 1
#define NUM_AXES_SCALE_ALTERNATIVES (unsigned int) 2

#define PROCESS_CHOICE_X_DISPLAY (unsigned int) 0
#define PROCESS_CHOICE_HARD_COPY (unsigned int) 1
#define PROCESS_CHOICE_FIND_EXTREMES (unsigned int) 2

#define PLOT_LAYOUT_PORTRAIT (unsigned int) 0
#define PLOT_LAYOUT_LANDSCAPE (unsigned int) 1
#define NUM_PLOT_LAYOUT_ALTERNATIVES (unsigned int) 2

#define DISPLAY_MODE_NORMAL (unsigned int) 0
#define DISPLAY_MODE_REPORT (unsigned int) 1
#define NUM_DISPLAY_MODES (unsigned int) 2

#define MOUSE_MODE_CROSSHAIR (unsigned int) 0
#define MOUSE_MODE_EDIT (unsigned int) 1
#define MOUSE_MODE_ANNOTATE (unsigned int) 2

struct colour_slider_type
{
    float min_red;
    float min_green;
    float min_blue;
    float max_red;
    float max_green;
    float max_blue;
};

struct shimmer_info_type
{
    float start_factor;
    float middle_factor;
    float end_factor;
    float middle_position;
};

/*  globals.c

    Globals file for  koords  (X11 co-ordinate generator tool for Karma).

    Copyright (C) 1996  Richard Gooch

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
    This Karma module will enable interactive generation of an astronomical
    co-ordinate system on a target image, using a reference image.
    This module runs on an X11 server.


    Written by      Richard Gooch   14-OCT-1996

    Last updated by Richard Gooch   28-OCT-1996


*/

#include "koords.h"

/*  Reference information  */
iarray ref_array = NULL;
KwcsAstro reference_ap = NULL;
unsigned int num_reference_points = 0;
double reference_ra[MAX_PAIRS];
double reference_dec[MAX_PAIRS];

/*  Target information  */
iarray tar_array = NULL;
KwcsAstro target_ap = NULL;
unsigned int num_target_points = 0;
double target_x[MAX_PAIRS];
double target_y[MAX_PAIRS];

flag last_click_was_reference = FALSE;

char title_name[STRING_LENGTH] = "Unknown";

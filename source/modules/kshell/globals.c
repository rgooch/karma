/*  globals.c

    Globals file for  kshell  (X11 ellipse integrator tool for Karma).

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
    This Karma module will enable interactive integration along the
    circumference of concentric ellipses.
    This module runs on an X11 server.


    Written by      Richard Gooch   24-SEP-1996: Copied from kpvslice module.

    Last updated by Richard Gooch   28-OCT-1996


*/

#include "kshell.h"

/*  Cube information  */
iarray    cube_arr = NULL;
KwcsAstro cube_ap = NULL;
iarray cube_ra_coords = NULL;
iarray cube_dec_coords = NULL;

/*  Loaded image information  */
iarray    image_arr = NULL; 
KwcsAstro loaded_image_ap = NULL;
ViewableImage image_vimage = NULL;

/*  Main canvas information  */
KwcsAstro main_ap = NULL;
flag      auto_update = TRUE;
unsigned int image_mode = IMAGE_MODE_LOADED;

/*  Moment maps  */
iarray    mom0_arr = NULL;
iarray    mom1_arr = NULL;
ViewableImage mom0_vimage = NULL;
ViewableImage mom1_vimage = NULL;

/*  Slice (aux) canvas information  */
KwcsAstro aux_ap = NULL;

char title_name[STRING_LENGTH] = "Unknown";

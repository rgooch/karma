/*  globals.c

    Globals file for  kview  (X11 image/movie display tool for Karma).

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
    This Karma module will enable on-screen imaging of a single arrayfile of
    the general data structure format. The arrayfile must contain a 2 or 3
    dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   17-SEP-1996

    Last updated by Richard Gooch   27-OCT-1996


*/

#include <karma.h>
#include <karma_wcs.h>


KwcsAstro astro_projection = NULL;
char title_name[STRING_LENGTH] = "Unknown";

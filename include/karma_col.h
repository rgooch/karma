/*  karma_col.h

    Header for  col_  package.

    Copyright (C) 1995  Richard Gooch

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

    This include file contains all the definitions and function declarations
  needed to interface to the col_ routines in the Karma library.


    Written by      Richard Gooch   29-DEC-1995

    Last updated by Richard Gooch   29-DEC-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_COL_H
#define KARMA_COL_H


/*  File:  main.c  */
EXTERN_FUNCTION (void col_hsb_slice_to_rgb_array,
		 (unsigned long rgb_array[65536],
		  unsigned long red_mask, unsigned long green_mask,
		  unsigned long blue_mask,
		  float min_brightness, float max_brightness,
		  float start_hue, float stop_hue, float saturation));


#endif /*  KARMA_COL_H  */

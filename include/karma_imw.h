/*  karma_imw.h

    Header for  imw_  package.

    Copyright (C) 1994  Richard Gooch

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
  needed to interface to the imw_ routines in the Karma library.


    Written by      Richard Gooch   8-AUG-1994

    Last updated by Richard Gooch   8-AUG-1994

*/

#ifndef KARMA_IMW_H
#define KARMA_IMW_H



#ifndef KARMA_H
#  include <karma.h>
#endif

#ifndef KARMA_DS_DEF_H
#  include <karma_ds_def.h>
#endif

#ifndef K_WIN_SCALE_H
#  include <k_win_scale.h>
#endif


/*  File:   imw_origsize.c   */
EXTERN_FUNCTION (flag imw_to8_o,
		 (unsigned char *out_image,
		  iaddr out_hstride, iaddr out_vstride,
		  int width, int height, CONST char *inp_image,
		  CONST iaddr *inp_hoffsets, CONST iaddr *inp_voffsets,
		  unsigned int inp_type, unsigned int conv_type,
		  unsigned int num_pixels, CONST unsigned char *pixel_values,
		  unsigned char blank_pixel,
		  unsigned char min_sat_pixel, unsigned char max_sat_pixel,
		  double i_min, double i_max) );


#endif /*  KARMA_IMW_H  */

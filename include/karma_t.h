/*  karma_t.h

    Header for  t_  package.

    Copyright (C) 1992,1993,1994,1995  Richard Gooch

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
  needed to interface to the t_ routines in the Karma library.


    Written by      Richard Gooch   19-OCT-1992

    Last updated by Richard Gooch   6-MAY-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_T_H
#define KARMA_T_H


#define KARMA_FFT_OK (unsigned int) 0
#define KARMA_FFT_BAD_LENGTH (unsigned int) 1
#define KARMA_FFT_BAD_TYPE (unsigned int) 2
#define KARMA_FFT_ALLOC_ERROR (unsigned int) 3
#define KARMA_FFT_METHOD_NOT_AVAILABLE (unsigned int) 4
#define KARMA_FFT_BAD_STRIDE (unsigned int) 5

/*  NOTE: do not change these without changing the Iarray library as well  */
#define KARMA_FFT_FORWARD 1
#define KARMA_FFT_INVERSE -1

/*  For the file: transform.c  */
EXTERN_FUNCTION (unsigned int t_c_to_c_1D_fft_float,
		 (float *real, float *imag, unsigned int length,
		  unsigned int stride, int direction));
EXTERN_FUNCTION (unsigned int t_c_to_c_many_1D_fft_float,
		 (float *real, float *imag, unsigned int length,
		  unsigned int elem_stride, unsigned int number,
		  unsigned int dim_stride, int direction) );
EXTERN_FUNCTION (flag t_check_power_of_2, (unsigned int number) );
EXTERN_FUNCTION (unsigned int t_r_to_c_many_1D_fft_float,
		 (float *array, unsigned int length, unsigned int elem_stride,
		  unsigned int number, unsigned int dim_stride,
		  int direction) );


#endif /*  KARMA_T_H  */

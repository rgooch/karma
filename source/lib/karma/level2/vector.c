/*LINTLIBRARY*/
/*PREFIX:"va_"*/
/*  vector.c

    This code provides some optimised vector arithmetic operations.

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

/*  This file contains various optimised routines which perform vector
  arithmetic operations.


    Written by      Richard Gooch   13-NOV-1992

    Updated by      Richard Gooch   2-DEC-1992

    Last updated by Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>


*/
#include <stdio.h>
#include <ctype.h>
#include <karma.h>
#include <karma_va.h>
#include <karma_a.h>


/*PUBLIC_FUNCTION*/
void va_scale_float (out, out_stride, inp, inp_stride, length, scale, offset)
/*  This routine will perform a scale and offset operation on an array of
    floating point numbers. The arithmetic form is as follows:
        out[i] = inp[i] * scale + offset
    The result array will be written to the storage pointed to by  out  .
    The stride (in floats) of consecutive elements in the output array must be
    given by  out_stride  .
    The input array must be pointed to by  inp_stride  .
    The stride (in floats) of consecutive elements in the input array must be
    given by  inp_stride  .
    The elements of the input and output arrays must be aligned on a float
    boundary.
    The length of the input and output arrays must be given by  length  .
    The scale factor must be given by  scale  .
    The offset value must be given by  offset  .
    The routine returns nothing.
*/
float *out;
int out_stride;
float *inp;
int inp_stride;
int length;
float scale;
float offset;
{
    static char function_name[] = "va_scale_float";

    if ( (int) out % sizeof *out != 0 )
    {
	(void) fprintf (stderr, "Output array not on a float boundary\n");
	a_prog_bug (function_name);
    }
    if ( (int) inp % sizeof *inp != 0 )
    {
	(void) fprintf (stderr, "Input array not on a float boundary\n");
	a_prog_bug (function_name);
    }
#ifdef ARCH_VXMVX
    asm__va_scale_float (out, sizeof *out * out_stride,
			 inp, sizeof *inp * inp_stride, length, scale, offset);
#else
    while (length-- > 0)
    {
	*out = *inp * scale + offset;
	inp += inp_stride;
	out += out_stride;
    }
#endif
}   /*  End Function va_scale_float  */

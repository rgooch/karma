/*LINTLIBRARY*/
/*  scale_float.c

    This code provides some optimised vector arithmetic operations.

    Copyright (C) 1992-1996  Richard Gooch

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

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   8-AUG-1994: Moved to full ASCII function
  declaration syntax. Had to do this because of the confused Dec Alpha compiler

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/va/scale_float.c

    Updated by      Richard Gooch   15-JUN-1995: Made use of IS_ALIGNED macro.

    Last updated by Richard Gooch   13-APR-1996: Changed to new documentation
  format.


*/
#include <stdio.h>
#include <ctype.h>
#include <karma.h>
#include <karma_va.h>
#include <karma_a.h>
#include <os.h>


#ifdef OS_VXMVX
EXTERN_FUNCTION (void asm__va_scale_float,
		 (float *out, int out_stride, float *inp, int inp_stride,
		  int length, float scale, float offset) );
#endif


/*PUBLIC_FUNCTION*/
void va_scale_float (float *out, int out_stride, float *inp, int inp_stride,
		     int length, float scale, float offset)
/*  [SUMMARY] Scale and offset an array of floats.
    [PURPOSE] This routine will perform a scale and offset operation on an
    array of floating point numbers. The arithmetic form is as follows:
        out[i] = inp[i] * scale + offset
    <out> The result array will be written here.
    <out_stride> The stride (in floats) of consecutive elements in the output
    array.
    <inp> The input array.
    <inp_stride> The stride (in floats) of consecutive elements in the input
    array.
    [NOTE] The elements of the input and output arrays must be aligned on a
    float boundary.
    <length> The length of the input and output arrays.
    <scale> The scale factor.
    <offset> The offset value.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "va_scale_float";

    if ( !IS_ALIGNED (out, sizeof *out) )
    {
	(void) fprintf (stderr, "Output array not on a float boundary\n");
	a_prog_bug (function_name);
    }
    if ( !IS_ALIGNED (inp, sizeof *inp) )
    {
	(void) fprintf (stderr, "Input array not on a float boundary\n");
	a_prog_bug (function_name);
    }
#ifdef OS_VXMVX
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

/*LINTLIBRARY*/
/*  fft.c

    This code provides simple Fourier Transform operations.

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

/*

    This file contains the various utility routines for performing simple
    Fourier Transforms.


    Written by      Richard Gooch   19-OCT-1992

    Updated by      Richard Gooch   1-DEC-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Last updated by Richard Gooch   26-NOV-1994: Moved to  packages/t/fft.c


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_t.h>


/*  Public routines follow  */

/*PUBLIC_FUNCTION*/
unsigned int t_c_to_c_1D_fft_float (real, imag, length, stride, direction)
/*  This routine will perform a complex to complex 1 dimensional FFT on an
    array of single precision complex data.
    The array of real components must be pointed to by  real  .
    The array of imaginary components must be pointed to by  imag  .
    The number of complex values in the array to transform must be given by
    length  .
    The stride (in bytes) of successive components must be given by  stride  .
    The routine will perform an inverse transform (with appropriate 1 / length
    scaling) if  direction  is KARMA_FFT_INVERSE.
    The routine performs the transform in situ.
    The routine returns a value indicating the success / failure status of the
    transform.
    This routine has been pinched from Patrick Jordan, who pinched and modified
    the Numerical Recipes in C routine.
*/
float *real;
float *imag;
unsigned int length;
unsigned int stride;
unsigned int direction;
{
    int mmax,mmax2,m,j,i;
    int ind,length_on_mmax2,ii,jj;
    int lengthstride;
    int mmax2stride;
    int int_stride;
    float wtemp,wr,wpr,wpi,wi,theta;
    float tempr,tempi;
    float swap;
    float *wrt,*wit;
    static int old_length;
    static flag first_time = TRUE;
    static float one_on_length;
    static float two_pi;
    static int *bit_rev;
    static float *wrti,*witi,*wrtf,*witf;

#ifdef NEEDS_MISALIGN_COMPILE
    /*  Check if data aligned  */
    if ( ( (int) real % sizeof (float) != 0 ) ||
	( (int) imag % sizeof (float) != 0 ) ||
	(stride % sizeof (float) != 0) )
    {
	return ( misalign__t_c_to_c_1D_fft_float (real, imag, length, stride,
						  direction) );
    }
#endif  /*  NEEDS_MISALIGN_COMPILE  */

    /*  Adjust  stride  */
    /*  Faster using a signed integer copy of the stride  */
    int_stride = stride / sizeof (float);
    lengthstride = length * int_stride;

    /*  Save some time if this fft is same length as previous one  */
    if (length != old_length)
    {
	int a,b;

	if (first_time == TRUE)
	{
	    first_time = FALSE;
	    /*  Calculate 2 * PI  */
	    two_pi = 8.0 * atan (1.0);
	}
	else
	{
	    free ( (char *) bit_rev );
	    free ( (char *) wrti );
	    free ( (char *) witi );
	    free ( (char *) wrtf );
	    free ( (char *) witf );
	}

	/*  Check if length is a power of 2  */
	if (t_check_power_of_2 (length) != TRUE)
	{
	    /*  Not a power of 2  */
	    return (KARMA_FFT_BAD_LENGTH);
	}

	one_on_length = 1.0 / (float) length;
	old_length = length;

	/*  Set up the bit reverse table  */

	bit_rev = (int *) calloc ( length, sizeof (int) );

	for (a = 1, b = length / 2; a < length; a *= 2, b /= 2)
	{
	    for (i = 0; i < length; i++)
	    {
		if ( ( (i / b) % 2 ) == 1 ) bit_rev[i] += a;
	    }
	}

	/*  Set up sin and cos table  */
	wrti = (float *) calloc ( length / 2, sizeof (float) );
	witi = (float *) calloc ( length / 2, sizeof (float) );
	wrtf = (float *) calloc ( length / 2, sizeof (float) );
	witf = (float *) calloc ( length / 2, sizeof (float) );

	/*  Create table for INVERSE  */
	theta = two_pi * one_on_length;
	wtemp = sin (0.5 * theta);
	wpr = -2.0 * wtemp * wtemp;
	wpi = sin (theta);
	wr = 1.0;
	wi = 0.0;
	for (m = 0; m < length / 2; m++)
	{
	    wrti[m] = wr;
	    witi[m] = wi;
	    wr = (wtemp = wr) * wpr - wi * wpi + wr;
	    wi = wi * wpr + wtemp * wpi + wi;
	}

	/*  Create table for FORWARD  */
	theta = - two_pi * one_on_length;
	wtemp = sin (0.5 * theta);
	wpr = -2.0 * wtemp * wtemp;
	wpi = sin (theta);
	wr = 1.0;
	wi = 0.0;
	for (m = 0; m < length / 2; m++)
	{
	    wrtf[m] = wr;
	    witf[m] = wi;
	    wr = (wtemp = wr) * wpr - wi * wpi + wr;
	    wi = wi * wpr + wtemp * wpi + wi;
	}
    }

    /*  Rearrange using bit reversal  */
    for (i = 0; i < length; i++)
    {
	j = bit_rev[i];
	if (j > i)
	{
	    swap = real[i * int_stride];
	    real[i * int_stride] = real[j * int_stride];
	    real[j * int_stride] = swap;
	    swap = imag[i * int_stride];
	    imag[i * int_stride] = imag[j * int_stride];
	    imag[j * int_stride] = swap;
	}
    }

    /*  Select appropriate sin and cos tables  */
    if (direction == KARMA_FFT_INVERSE)
    {
	wrt = wrti;
	wit = witi;
    }
    else
    {
	wrt = wrtf;
	wit = witf;
    }

    /*  Do the actual transform  */
    for (mmax = 1; mmax < length; mmax = mmax2)
    {
	mmax2 = mmax * 2;
	mmax2stride = mmax2 * int_stride;
	length_on_mmax2 = length / mmax2;
	for (m = 0; m < mmax; m++)
	{
	    ind = m * length_on_mmax2;
	    wr = wrt[ind];
	    wi = wit[ind];
	    for (ii = m * int_stride, jj = (m + mmax) * int_stride;
		 ii < lengthstride;
		 ii += mmax2stride, jj += mmax2stride)
	    {
		tempr = wr * real[jj] - wi * imag[jj];
		tempi = wr * imag[jj] + wi * real[jj];
		real[jj] = real[ii] - tempr;
		imag[jj] = imag[ii] - tempi;
		real[ii] += tempr;
		imag[ii] += tempi;
	    }
	}
    }

    /*  Scale by a factor of 1 / length for inverse fft  */
    if (direction == KARMA_FFT_INVERSE)
    {
	/*  No gain to be made using pointers instead of indexing on a Sparc */
	for (i = 0; i < length; i++)
	{
	    real[i * int_stride] *= one_on_length;
	    imag[i * int_stride] *= one_on_length;
	}
    }
    /*  Return OK  */
    return (KARMA_FFT_OK);
}   /*  End Function t_c_to_c_1D_fft_float  */

/*PUBLIC_FUNCTION*/
unsigned int t_c_to_c_many_1D_fft_float (real, imag, length, elem_stride,
					 number, dim_stride, direction)
/*  This routine will perform a number of complex to complex 1 dimensional FFTs
    on an array of single precision complex data.
    The array of real components must be pointed to by  real  .
    The array of imaginary components must be pointed to by  imag  .
    The number of complex values in the array to transform must be given by
    length  .
    The stride (in bytes) of successive components must be given by
    elem_stride  .
    The number of 1 dimensional FFTs to perform must be given by  number  .
    The stride (in bytes) between successive data sets must be given by
    dim_stride  .
    The routine will perform an inverse transform (with appropriate 1 / length
    scaling) if  direction  is KARMA_FFT_INVERSE.
    The routine performs the transform in situ.
    The routine returns a value indicating the success / failure status of the
    transform.
    This routine has been pinched from Patrick Jordan, who pinched and modified
    the Numerical Recipes in C routine.
*/
float *real;
float *imag;
unsigned int length;
unsigned int elem_stride;
unsigned int number;
unsigned int dim_stride;
unsigned int direction;
{
    int mmax,mmax2,m,j,i;
    int ind,length_on_mmax2,ii,jj;
    int lengthstride;
    int mmax2stride;
    int int_stride;
    unsigned int iter_count;
    float wtemp,wr,wpr,wpi,wi,theta;
    float tempr,tempi;
    float swap;
    float *real_ptr;
    float *imag_ptr;
    float *wrt,*wit;
    static int old_length;
    static flag first_time = TRUE;
    static float one_on_length;
    static float two_pi;
    static int *bit_rev;
    static float *wrti,*witi,*wrtf,*witf;

#ifdef NEEDS_MISALIGN_COMPILE
    /*  Check if data aligned  */
    if ( ( (int) real % sizeof (float) != 0 ) ||
	( (int) imag % sizeof (float) != 0 ) ||
	(elem_stride % sizeof (float) != 0) ||
	(dim_stride % sizeof (float) != 0) )
    {
	return ( misalign__t_c_to_c_many_1D_fft_float (real, imag,
						       length, elem_stride,
						       number, dim_stride,
						       direction) );
    }
#endif  /*  NEEDS_MISALIGN_COMPILE  */

    /*  Adjust  elem_stride  */
    /*  Faster using a signed integer copy of the stride  */
    int_stride = elem_stride / sizeof (float);
    lengthstride = length * int_stride;
    dim_stride /= sizeof (float);

    /*  Save some time if this fft is same length as previous one  */
    if (length != old_length)
    {
	int a,b;

	if (first_time == TRUE)
	{
	    first_time = FALSE;
	    /*  Calculate 2 * PI  */
	    two_pi = 8.0 * atan (1.0);
	}
	else
	{
	    free ( (char *) bit_rev );
	    free ( (char *) wrti );
	    free ( (char *) witi );
	    free ( (char *) wrtf );
	    free ( (char *) witf );
	}

	/*  Check if length is a power of 2  */
	if (t_check_power_of_2 (length) != TRUE)
	{
	    /*  Not a power of 2  */
	    return (KARMA_FFT_BAD_LENGTH);
	}

	one_on_length = 1.0 / (float) length;
	old_length = length;

	/*  Set up the bit reverse table  */

	bit_rev = (int *) calloc ( length, sizeof (int) );

	for (a = 1, b = length / 2; a < length; a *= 2, b /= 2)
	{
	    for (i = 0; i < length; i++)
	    {
		if ( ( (i / b) % 2 ) == 1 ) bit_rev[i] += a;
	    }
	}

	/*  Set up sin and cos table  */
	wrti = (float *) calloc ( length / 2, sizeof (float) );
	witi = (float *) calloc ( length / 2, sizeof (float) );
	wrtf = (float *) calloc ( length / 2, sizeof (float) );
	witf = (float *) calloc ( length / 2, sizeof (float) );

	/*  Create table for INVERSE  */
	theta = two_pi * one_on_length;
	wtemp = sin (0.5 * theta);
	wpr = -2.0 * wtemp * wtemp;
	wpi = sin (theta);
	wr = 1.0;
	wi = 0.0;
	for (m = 0; m < length / 2; m++)
	{
	    wrti[m] = wr;
	    witi[m] = wi;
	    wr = (wtemp = wr) * wpr - wi * wpi + wr;
	    wi = wi * wpr + wtemp * wpi + wi;
	}

	/*  Create table for FORWARD  */
	theta = - two_pi * one_on_length;
	wtemp = sin (0.5 * theta);
	wpr = -2.0 * wtemp * wtemp;
	wpi = sin (theta);
	wr = 1.0;
	wi = 0.0;
	for (m = 0; m < length / 2; m++)
	{
	    wrtf[m] = wr;
	    witf[m] = wi;
	    wr = (wtemp = wr) * wpr - wi * wpi + wr;
	    wi = wi * wpr + wtemp * wpi + wi;
	}
    }

    /*  Rearrange using bit reversal  */
    for (i = 0; i < length; i++)
    {
	j = bit_rev[i];
	if (j > i)
	{
	    for (iter_count = 0, real_ptr = real, imag_ptr = imag;
		 iter_count < number;
		 ++iter_count, real_ptr += dim_stride, imag_ptr += dim_stride)
	    {
		swap = real_ptr[i * int_stride];
		real_ptr[i * int_stride] = real_ptr[j * int_stride];
		real_ptr[j * int_stride] = swap;
		swap = imag_ptr[i * int_stride];
		imag_ptr[i * int_stride] = imag_ptr[j * int_stride];
		imag_ptr[j * int_stride] = swap;
	    }
	}
    }

    /*  Select appropriate sin and cos tables  */
    if (direction == KARMA_FFT_INVERSE)
    {
	wrt = wrti;
	wit = witi;
    }
    else
    {
	wrt = wrtf;
	wit = witf;
    }

    /*  Do the actual transform  */
    for (mmax = 1; mmax < length; mmax = mmax2)
    {
	mmax2 = mmax * 2;
	mmax2stride = mmax2 * int_stride;
	length_on_mmax2 = length / mmax2;
	for (m = 0; m < mmax; m++)
	{
	    ind = m * length_on_mmax2;
	    wr = wrt[ind];
	    wi = wit[ind];
	    for (ii = m * int_stride, jj = (m + mmax) * int_stride;
		 ii < lengthstride;
		 ii += mmax2stride, jj += mmax2stride)
	    {
		for (iter_count = 0, real_ptr = real, imag_ptr = imag;
		     iter_count < number;
		     ++iter_count, real_ptr += dim_stride,
		     imag_ptr += dim_stride)
		{
		    tempr = wr * real_ptr[jj] - wi * imag_ptr[jj];
		    tempi = wr * imag_ptr[jj] + wi * real_ptr[jj];
		    real_ptr[jj] = real_ptr[ii] - tempr;
		    imag_ptr[jj] = imag_ptr[ii] - tempi;
		    real_ptr[ii] += tempr;
		    imag_ptr[ii] += tempi;
		}
	    }
	}
    }

    /*  Scale by a factor of 1 / length for inverse fft  */
    if (direction == KARMA_FFT_INVERSE)
    {
	for (i = 0; i < length; i++)
	{
	    for (iter_count = 0, real_ptr = real, imag_ptr = imag;
		 iter_count < number;
		 ++iter_count, real_ptr += dim_stride, imag_ptr += dim_stride)
	    {
		/*  No gain to be made using pointers instead of indexing
		    on a Sparc  */
		real_ptr[i * int_stride] *= one_on_length;
		imag_ptr[i * int_stride] *= one_on_length;
	    }
	}
    }
    /*  Return OK  */
    return (KARMA_FFT_OK);
}   /*  End Function t_c_to_c_many_1D_fft_float  */

/*PUBLIC_FUNCTION*/
flag t_check_power_of_2 (number)
/*  This routine will check if a number is a power of 2.
    The number must be given by  number  .
    The routine returns TRUE if the number is a power of 2,
    else it returns FALSE.
*/
unsigned int number;
{
    unsigned int i;
    static flag prev_result = FALSE;
    static unsigned int prev_number = 0;

    if (number == prev_number)
    {
	return (prev_result);
    }
    prev_number = number;
    for (i = 1; i < number; i *= 2);
    if (i == number)
    {
	/*  Power of 2  */
	prev_result = TRUE;
    }
    else
    {
	/*  Not a power of 2  */
	prev_result = FALSE;
    }
    return (prev_result);
}   /*  End Function t_check_power_of_2  */

/*PUBLIC_FUNCTION*/
unsigned int t_r_to_c_many_1D_fft_float (a, length, elem_stride, number,
					 dim_stride, direction)
/*  This routine will perform a real to complex or complex to real FFT.
    The array must be pointed to by  a  .
    The number of elements to transform must be given by  n  .
    The stride (in floats) of sucessive data elements must be given by
    elem_stride  .
    The number of transforms to perform must be given by  number  .
    The stride (in floats) between sucessive data sets must be given by
    dim_stride  .
    The routine will perform the forward, real to complex FFT if  direction  is
    KARMA_FFT_FORWARD  else it will perform the inverse, complex to real FFT.
    The routine returns a value indicating the success / failure status of the
    transform.
*/
float *a;
unsigned int length;
unsigned int elem_stride;
unsigned int number;
unsigned int dim_stride;
unsigned int direction;
{
    int i1,i2,i3,i4,count,err;
    float c1=0.5,c2=0.5,h1r,h1i,h2r,h2i;
    float wr,wi,wpr,wpi,wtemp,theta;
    int stride2=2*elem_stride;
    float *aa=a;
  
    theta=PI/(float) length;

    if (direction == KARMA_FFT_FORWARD)
    {
	c2 = -c2;
	theta = -theta;
	err=t_c_to_c_many_1D_fft_float(&a[0],&a[elem_stride],length,4*stride2,number,
				       4*dim_stride,direction);
	if(err!=0)
	printf("ERROR: (r_to_c_fft_fp_many) : Karma fft returned %d\n",err);
    }
  
    for(count=0;count<number;count++,aa+=dim_stride)
    {
	wtemp=sin(0.5*theta);
	wr=1.0+(wpr = -2.0*wtemp*wtemp);
	wi=wpi=sin(theta);

	for (i1=stride2,i2=3*elem_stride,i3=(2*length-2)*elem_stride,i4=(2*length-1)*elem_stride;
	     i1<=i3;
	     i1+=stride2,i2+=stride2,i3-=stride2,i4-=stride2)
	{
	    h1r=c1*(aa[i1]+aa[i3]);
	    h1i=c1*(aa[i2]-aa[i4]);
	    h2r = -c2*(aa[i2]+aa[i4]);
	    h2i=c2*(aa[i1]-aa[i3]);
	    aa[i1]=h1r+wr*h2r-wi*h2i;
	    aa[i2]=h1i+wr*h2i+wi*h2r;
	    aa[i3]=h1r-wr*h2r+wi*h2i;
	    aa[i4]= -h1i+wr*h2i+wi*h2r;
	    wr=(wtemp=wr)*wpr-wi*wpi+wr;
	    wi=wi*wpr+wtemp*wpi+wi;
	}

	aa[0] = (h1r= aa[0])+ aa[elem_stride];
	aa[elem_stride] = h1r- aa[elem_stride];

	if (direction == KARMA_FFT_INVERSE)
	{
	    aa[0] *= 0.5;
	    aa[elem_stride] *= 0.5;
	}
    }

    if (direction == KARMA_FFT_INVERSE)
    {
	err=t_c_to_c_many_1D_fft_float(&a[0],&a[elem_stride],length,4*stride2,number,
				       4*dim_stride,direction);
	if(err!=0)
	printf("ERROR: (r_to_c_fft_fp_many) : Karma fft returned %d\n",err);
    }
    /*  Return OK  */
    return (KARMA_FFT_OK);
}   /*  End Function t_r_to_c_many_1D_fft_float  */

/*LINTLIBRARY*/
/*PREFIX:"n_"*/
/*  numbers.c

    This code provides random number generation routines.

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

/*

    This file contains the various Karma utility routines which generate random
  numbers.


    Written by      Richard Gooch   3-OCT-1992

    Updated by      Richard Gooch   14-NOV-1992

    Last updated by Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>


*/
#include <stdio.h>
#ifdef ARCH_MS_DOS_386
#include <time.h>
#define random rand
#else
#include <sys/time.h>
#endif
#include <karma.h>
#include <karma_n.h>

/*PUBLIC_FUNCTION*/
double n_gaussian ()
/*  This routine will compute a random number with a Gaussian distribution.
    The mean is 0.0 and the variance is 1.0
    The routine will return the number.
*/
{   
    static int call1 = TRUE;
#ifdef ARCH_MS_DOS_386
    static unsigned int one_i = 0x8000;
    time_t ntime;
    int rand ();
#else
    static unsigned long one_l = 0x80000000;
    struct timeval tv;		/* gettimeofday() returns time value */
    struct timezone tz;		/*  ... & time zone                  */
    long random();
#endif
    double val = -6.0;
    static double one;
    int i;

    if (call1)
    {
	call1 = FALSE; 
#ifdef ARCH_MS_DOS_386
	one = one_i;
	time (&ntime);
	srand (ntime);
#else
	one = one_l;
	gettimeofday(&tv,&tz);
	srandom(tv.tv_sec);
#endif
    }

    for (i = 0; i < 12; i++) val += random () / one;

    return (val);
}   /*  End Function n_gaussian  */

/*PUBLIC_FUNCTION*/
double n_uniform ()
/*  This routine will compute a random number with a Uniform distribution.
    The range is from 0.0 to 1.0
    The routine will return the number.
*/
{   
    static int call1 = TRUE;
#ifdef ARCH_MS_DOS_386
    static unsigned int one_i = 0x8000;
    time_t ntime;
    int rand ();
#else
    static unsigned long one_l = 0x80000000;
    struct timeval tv;		/* gettimeofday() returns time value */
    struct timezone tz;		/*  ... & time zone                  */
    long random();
#endif
    double val;
    static double one;
    int i;

    if (call1)
    {
	call1 = FALSE; 
#ifdef ARCH_MS_DOS_386
	one = one_i;
	time (&ntime);
	srand (ntime);
#else
	one = one_l;
	gettimeofday(&tv,&tz);
	srandom(tv.tv_sec);
#endif
    }

    val = random () / one;
    return (val);
}   /*  End Function n_uniform  */

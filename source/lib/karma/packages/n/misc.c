/*LINTLIBRARY*/
/*  misc.c

    This code provides random number generation routines.

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

    This file contains the various Karma utility routines which generate random
  numbers.


    Written by      Richard Gooch   3-OCT-1992

    Updated by      Richard Gooch   14-NOV-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   23-AUG-1994: XOred in microseconds time
  value when computing seed.

    Updated by      Richard Gooch   4-OCT-1994: Changed to  ?rand48  routines
  in order to avoid having to link with buggy UCB compatibility library in
  Slowaris 2.3

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/n/misc.c

    Last updated by Richard Gooch   25-JAN-1995: Added #ifdef OS_ConvexOS


*/
#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <karma.h>
#include <karma_n.h>

extern double drand48 ();

/*PUBLIC_FUNCTION*/
double n_gaussian ()
/*  This routine will compute a random number with a Gaussian distribution.
    The mean is 0.0 and the variance is 1.0
    The routine will return the number.
*/
{   
    double val = -6.0;
    int i;

    for (i = 0; i < 12; i++) val += n_uniform ();
    return (val);
}   /*  End Function n_gaussian  */

/*PUBLIC_FUNCTION*/
double n_uniform ()
/*  This routine will compute a random number with a Uniform distribution.
    The range is from 0.0 to 1.0
    The routine will return the number.
*/
{   
    struct timeval tv;
    struct timezone tz;
#ifdef OS_ConvexOS
    long val = 0x7fffffff;
#endif
    static int first_time = TRUE;

    if (first_time)
    {
	first_time = FALSE; 
	gettimeofday (&tv, &tz);
	tv.tv_sec ^= tv.tv_usec;
#ifdef OS_ConvexOS
	srandom (tv.tv_sec);
#else
	srand48 (tv.tv_sec);
#endif
    }
#ifdef OS_ConvexOS
    return ( (double) random () / (double) val );
#else
    return ( drand48 () );
#endif
}   /*  End Function n_uniform  */

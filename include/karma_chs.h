/*  karma_chs.h

    Header for  chs_  package.

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

    This include file contains all the definitions and function declarations
  needed to interface to the chs_ routines in the Karma library.


    Written by      Richard Gooch   3-DEC-1992

    Last updated by Richard Gooch   21-MAY-1994

*/

#ifndef KARMA_CHS_H
#define KARMA_CHS_H


#ifndef KARMA_H
#  include <karma.h>
#endif

#ifndef KARMA_CH_DEF_H
#  include <karma_ch_def.h>
#endif


/*  File:   chs.c   */
EXTERN_FUNCTION (unsigned int chs_get_value, (Channel channel, char *string,
					      unsigned int length) );
EXTERN_FUNCTION (double chs_get_float, (Channel channel) );
EXTERN_FUNCTION (int chs_get_int, (Channel channel) );
EXTERN_FUNCTION (flag chs_get_line, (Channel channel, char *buffer,
				     unsigned int length) );


#endif /*  KARMA_CHS_H  */

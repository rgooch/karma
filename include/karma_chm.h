/*  karma_chm.h

    Header for  chm_  package.

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

    This include file contains all the definitions and function declarations
  needed to interface to the chm_ routines in the Karma library.


    Written by      Richard Gooch   13-SEP-1992

    Last updated by Richard Gooch   7-OCT-1992

*/

#ifndef KARMA_CHM_H
#define KARMA_CHM_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#include <karma_ch.h>

/*  File:   chm.c   */
EXTERN_FUNCTION (flag chm_manage, (Channel channel, void *info,
				   flag (*input_func) (Channel channel,
						       void **info),
				   void (*close_func) (void *info),
				   flag (*output_func) (Channel channel,
							void **info),
				   flag (*exception_func) (Channel channel,
							   void **info) ) );
EXTERN_FUNCTION (void chm_unmanage, (Channel channel) );
EXTERN_FUNCTION (void chm_poll, (long timeout) );


#endif /*  KARMA_CHM_H  */

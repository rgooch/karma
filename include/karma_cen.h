/*  karma_cen.h

    Header for  cen_  package.

    Copyright (C) 1994,1995  Richard Gooch

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
  needed to interface to the cen_ routines in the Karma library.

    Written by      Richard Gooch   6-APR-1994

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#if !defined(KARMA_CH_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ch_def.h>
#endif

#if !defined(KARMA_EN_H) || defined(MAKEDEPEND)
#  include <karma_en.h>
#endif

#ifndef KARMA_CEN_H
#define KARMA_CEN_H


/*  File:  cen_idea.c  */
EXTERN_FUNCTION (ChConverter cen_idea,
		 (Channel channel, char read_key[EN_IDEA_KEY_SIZE],
		  char read_init_vector[EN_IDEA_BLOCK_SIZE],
		  char write_key[EN_IDEA_KEY_SIZE],
		  char write_init_vector[EN_IDEA_BLOCK_SIZE], flag clear) );


#endif /*  KARMA_CEN_H  */

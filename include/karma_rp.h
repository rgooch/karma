/*  karma_rp.h

    Header for  rp_  package.

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
  needed to interface to the rp_ routines in the Karma library.


    Written by      Richard Gooch   24-AUG-1994

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_RP_H
#define KARMA_RP_H


typedef struct randpool_type * RandPool;


/*  File:  randpool.c  */
EXTERN_FUNCTION (RandPool rp_create,
		 ( unsigned int size, unsigned int hash_digest_size,
		  unsigned int hash_block_size, void (*hash_func) () ) );
EXTERN_FUNCTION (void rp_add_bytes,
		 (RandPool rp, CONST unsigned char *buf,
		  unsigned int length) );
EXTERN_FUNCTION (void rp_get_bytes,
		 (RandPool rp, unsigned char *buf, unsigned int length) );
EXTERN_FUNCTION (void rp_destroy, (RandPool rp) );
EXTERN_FUNCTION (void rp_destroy_all, () );
EXTERN_FUNCTION (void rp_add_time_noise, (RandPool rp) );
EXTERN_FUNCTION (void rp_register_destroy_func,
		 (RandPool rp, void (*destroy_func) (), void *info) );


#endif /*  KARMA_RP_H  */


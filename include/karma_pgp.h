/*  karma_pgp.h

    Header for  pgp_  package.

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
  needed to interface to the pgp_ routines in the Karma library.


    Written by      Richard Gooch   2-DEC-1994

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_PGP_H
#define KARMA_PGP_H



/*  File:   decrypt.c   */
EXTERN_FUNCTION (char *pgp_decrypt,
		 (CONST char *ciphertext, unsigned int ciphertext_length,
		  unsigned int *plaintext_length) );


/*  File:   encrypt.c  */
EXTERN_FUNCTION (char *pgp_encrypt,
		 (CONST char *plaintext, unsigned int plaintext_length,
		  CONST char **recipients, unsigned int num_recipients,
		  unsigned int *ciphertext_length, flag ascii_armour) );


#endif /*  KARMA_PGP_H  */

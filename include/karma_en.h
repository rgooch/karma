/*  karma_en.h

    Header for  en_  package.

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
  needed to interface to the en_ routines in the Karma library.

    Written by      Richard Gooch   6-APR-1994

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_EN_H
#define KARMA_EN_H


#define EN_IDEA_KEY_SIZE 16
#define EN_IDEA_BLOCK_SIZE 8


typedef struct idea_cipher_status_type * idea_status;


/*  File:  en_idea.c  */
EXTERN_FUNCTION (idea_status en_idea_init,
		 (char key[EN_IDEA_KEY_SIZE], flag decrypt,
		  char init_vector[EN_IDEA_BLOCK_SIZE], flag clear) );
EXTERN_FUNCTION (void en_idea_cfb, (idea_status status, char *buffer,
				    unsigned int length) );
EXTERN_FUNCTION (void en_idea_close, (idea_status status) );


#endif /*  KARMA_EN_H  */

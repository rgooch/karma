/*  karma_m.h

    Header for  m_  package.

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
  needed to interface to the m_ routines in the Karma library.


    Written by      Richard Gooch   12-SEP-1992

    Last updated by Richard Gooch   25-MAY-1993

*/

#ifndef KARMA_M_H
#define KARMA_M_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#ifndef KARMA_H
#  include <karma.h>
#endif
#ifndef NULL
#define NULL 0
#endif

/*  File:   mem_alloc.c   */
EXTERN_FUNCTION (char *m_alloc, (unsigned int size) );
EXTERN_FUNCTION (void m_free, (char * ptr) );
EXTERN_FUNCTION (void m_error_notify, (char *function_name, char *purpose) );
EXTERN_FUNCTION (void m_abort, (char *name, char *reason) );

/*  File:   memory.c   */
EXTERN_FUNCTION (void m_clear, (char *memory, unsigned int length) );
EXTERN_FUNCTION (void m_copy, (char *dest, char *source,
			       unsigned int length) );
EXTERN_FUNCTION (void m_copy_blocks, (char *dest, char *source,
				      unsigned int dest_stride,
				      unsigned int source_stride,
				      unsigned int block_size,
				      unsigned int num_blocks) );
EXTERN_FUNCTION (void m_fill, (char *dest, unsigned int stride,
			       char *source, unsigned int size,
			       unsigned int num) );
EXTERN_FUNCTION (flag m_cmp, (char *block1, char *block2,
			      unsigned int length) );


#endif /*  KARMA_M_H  */

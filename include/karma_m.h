/*  karma_m.h

    Header for  m_  package.

    Copyright (C) 1992-1996  Richard Gooch

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

    Last updated by Richard Gooch   15-JUN-1996

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif
#ifndef NULL
#  define NULL 0
#endif

#ifndef KARMA_M_H
#define KARMA_M_H



/*  File:   mem_alloc.c   */
EXTERN_FUNCTION (char *m_alloc, (uaddr size) );
EXTERN_FUNCTION (void m_free, (char *ptr) );
EXTERN_FUNCTION (void m_error_notify, (char *function_name, char *purpose) );
EXTERN_FUNCTION (void m_abort, (char *name, char *reason) );
EXTERN_FUNCTION (unsigned int m_verify_memory_integrity, (flag force) );

/*  File:   memory.c   */
EXTERN_FUNCTION (void m_clear, (char *memory, uaddr length) );
EXTERN_FUNCTION (void m_copy, (char *dest, CONST char *source,
			       uaddr length) );
EXTERN_FUNCTION (void m_copy_blocks, (char *dest, CONST char *source,
				      unsigned int dest_stride,
				      unsigned int source_stride,
				      unsigned int block_size,
				      unsigned int num_blocks) );
EXTERN_FUNCTION (void m_copy_and_swap_blocks,
		 (char *dest, CONST char *source, uaddr dest_stride,
		  uaddr source_stride, uaddr block_size, uaddr num_blocks) );
EXTERN_FUNCTION (void m_fill, (char *dest, uaddr stride,
			       CONST char *source, uaddr size,
			       unsigned int num) );
EXTERN_FUNCTION (flag m_cmp, (CONST char *block1, CONST char *block2,
			      uaddr length) );
EXTERN_FUNCTION (char *m_dup, (CONST char *original, uaddr size) );
EXTERN_FUNCTION (char *m_alloc_scratch, (uaddr size, char *function_name) );
EXTERN_FUNCTION (void m_free_scratch, () );


#endif /*  KARMA_M_H  */

/*LINTLIBRARY*/
/*  u32.c

    This code provides conversion between host and cannonical data formats.

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

    This file contains various Karma utility routines which perform conversion
    between host and cannonical data formats.


    Written by      Richard Gooch   12-AUG-1992

    Updated by      Richard Gooch   14-NOV-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   13-FEB-1993: Added  p_write_buf64  and
  p_read_buf64

    Updated by      Richard Gooch   29-JUL-1993: Fixed bugs in  p_write_buf16s
  and  p_read_buf16s  which manifested themselves on byte swapper machines.

    Updated by      Richard Gooch   15-APR-1994: Added code to handle
  misaligned data for those platforms which care.

    Updated by      Richard Gooch   7-JUL-1994: Added support for 64 bit read
  and write for Alpha.

    Updated by      Richard Gooch   6-AUG-1994: Changed from using *  and  /
  to  <<  and  >>  to get proper sign extension.

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Last updated by Richard Gooch   26-NOV-1994: Split and moved to
  packages/p/u32.c


*/
#include <stdio.h>
#include <karma.h>
#include <os.h>
#include <karma_p.h>

#define BYTE_MASK 0xff

/*PUBLIC_FUNCTION*/
flag p_write_buf32 (buffer, data)
/*  This routine will write 32 bits of data to the buffer pointed to by
    buffer  .
    The data must be given by  data  .This must be in host natural byte order.
    The data will be converted to network byte order prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *buffer;
unsigned long data;
{
    static char function_name[] = "p_write_buf32";

#if defined(BLOCK_TRANSFER) && !defined(NEEDS_MISALIGN_COMPILE)
    *(unsigned long *) buffer = data;
    return (TRUE);
#else
#  if defined(BLOCK_TRANSFER) && defined(NEEDS_MISALIGN_COMPILE)
    if ( (int) buffer & 0x3 == 0 )
    {
	*(unsigned long *) buffer = data;
	return (TRUE);
    }
#  endif
    if (data > NET_UINT_MAX)
    {
	(void) fprintf (stderr,
			"WARNING: %s: data: %lu is outside range for network format: clipping\n",
			function_name, data);
	data = NET_UINT_MAX;
    }
    /*  Convert data to network byte order  */
    /*  Have to do this the hard way  */
    /*  Byte 3 (LSB)  */
    *(unsigned char *) (buffer + 3) = data & BYTE_MASK;
    /*  Byte 2  */
    *(unsigned char *) (buffer + 2) = (data >> 8) & BYTE_MASK;
    /*  Byte 1  */
    *(unsigned char *) (buffer + 1) = (data >> 16) & BYTE_MASK;
    /*  Byte 0 (MSB)  */
    *(unsigned char *) buffer = (data >> 24) & BYTE_MASK;
    return (TRUE);
#endif
}   /*  End Function p_write_buf32  */

/*PUBLIC_FUNCTION*/
flag p_read_buf32 (buffer, data)
/*  This routine will read 32 bits of data from the buffer pointed to by
    buffer  .This must be at least 4 bytes long.
    The data will be written to the storage pointed to by  data  .This will be
    in host natural byte order.
    The data will be converted from network byte order after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *buffer;
unsigned long *data;
{
#if defined(BLOCK_TRANSFER) && !defined(NEEDS_MISALIGN_COMPILE)
    *data = *(unsigned long *) buffer;
    return (TRUE);
#else
#  if defined(BLOCK_TRANSFER) && defined(NEEDS_MISALIGN_COMPILE)
    if ( (int) buffer & 0x3 == 0 )
    {
	*data = *(unsigned long *) buffer;
	return (TRUE);
    }
#  endif
    /*  Have to do this the hard way  */
    /*  Byte 0 (MSB)  */
    *data = (unsigned long) *(unsigned char *) buffer << 24;
    /*  Byte 1  */
    *data |= (unsigned long) *(unsigned char *) (buffer + 1) << 16;
    /*  Byte 2  */
    *data |= (unsigned long) *(unsigned char *) (buffer + 2) << 8;
    /*  Byte 3 (LSB)  */
    *data |= *(unsigned char *) (buffer + 3);
    return (TRUE);
#endif
}   /*  End Function p_read_buf32  */

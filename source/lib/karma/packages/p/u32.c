/*LINTLIBRARY*/
/*  u32.c

    This code provides conversion between host and cannonical data formats.

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

    Updated by      Richard Gooch   26-NOV-1994: Split and moved to
  packages/p/u32.c

    Updated by      Richard Gooch   27-FEB-1994: Improved portability.

    Last updated by Richard Gooch   9-JUN-1995: Changed to NEED_ALIGNED_DATA
  and removed assumption that all little endian platforms have a 32 bit
  unsigned integer.


*/
#include <stdio.h>
#include <karma.h>
#include <os.h>
#include <karma_p.h>

#define BYTE_MASK 0xff

#undef ALWAYS_DONE

/*PUBLIC_FUNCTION*/
flag p_write_buf32 (char *buffer, unsigned long data)
/*  [PURPOSE] This routine will write 32 bits of unsigned data to a buffer,
    <buffer> A pointer to the buffer. This buffer must be at least 4 bytes long
    <data> The data. This must be in host natural byte order. The data will be
    converted to network byte order prior to writing.
    [RETURNS] TRUE on success, else FALSE.
*/
{
#if defined(MACHINE_LITTLE_ENDIAN) && defined(Kword32u)
    Kword32u value;
    char *data_ptr = (char *) &value;
#endif

    static char function_name[] = "p_write_buf32";

    if (data > NET_UINT_MAX)
    {
	(void) fprintf (stderr,
			"WARNING: %s: data: %lu is outside network format range: clipping\n",
			function_name, data);
	data = NET_UINT_MAX;
    }
#ifdef Kword32u
#  if defined(MACHINE_BIG_ENDIAN) && !defined(NEED_ALIGNED_DATA)
#    define ALWAYS_DONE
    *(Kword32u *) buffer = data;
    return (TRUE);
#  endif
#  if defined(MACHINE_BIG_ENDIAN) && defined(NEED_ALIGNED_DATA)
    if ( ( (int) buffer & 0x3 ) == 0 )
    {
	*(Kword32u *) buffer = data;
	return (TRUE);
    }
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
#    define ALWAYS_DONE
    value = data;
    buffer[0] = data_ptr[3];
    buffer[1] = data_ptr[2];
    buffer[2] = data_ptr[1];
    buffer[3] = data_ptr[0];
    return (TRUE);
#  endif
#endif
#ifndef ALWAYS_DONE
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

#undef ALWAYS_DONE

/*PUBLIC_FUNCTION*/
flag p_read_buf32 (char *buffer, unsigned long *data)
/*  [PURPOSE] This routine will read 32 bits of unsigned data from a buffer.
    <buffer> A pointer to the buffer. This buffer must be at least 4 bytes long
    <data> The output data will be written here. This will be in host natural
    byte order. The data will be converted from network byte order after
    reading.
    [RETURNS] TRUE on success, else FALSE.
*/
{
#ifdef Kword32u
#  if defined(MACHINE_BIG_ENDIAN) && !defined(NEED_ALIGNED_DATA)
#    define ALWAYS_DONE
    *data = *(Kword32u *) buffer;
    return (TRUE);
#  endif
#  if defined(MACHINE_BIG_ENDIAN) && defined(NEED_ALIGNED_DATA)
    if ( ( (int) buffer & 0x3 ) == 0 )
    {
	*data = *(Kword32u *) buffer;
	return (TRUE);
    }
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
#    define ALWAYS_DONE
    Kword32u value;
    char *data_ptr = (char *) &value;

    data_ptr[0] = buffer[3];
    data_ptr[1] = buffer[2];
    data_ptr[2] = buffer[1];
    data_ptr[3] = buffer[0];
    *data = value;
    return (TRUE);
#  endif
#endif
#ifndef ALWAYS_DONE
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

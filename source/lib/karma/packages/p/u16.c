/*LINTLIBRARY*/
/*  u16.c

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


    Written by      Richard Gooch   27-FEB-1994: Copied from  s16.c

    Updated by      Richard Gooch   27-FEB-1994

    Last updated by Richard Gooch   9-JUN-1995: Changed to NEED_ALIGNED_DATA
  and removed assumption that all little endian platforms have a 16 bit
  unsigned integer.


*/
#include <stdio.h>
#include <karma.h>
#include <os.h>
#include <karma_p.h>

#define BYTE_MASK 0xff

#undef ALWAYS_DONE

/*PUBLIC_FUNCTION*/
flag p_write_buf16 (char *buffer, unsigned long data)
/*  [PURPOSE] This routine will write 16 bits of unsigned data to a buffer,
    <buffer> A pointer to the buffer. This buffer must be at least 2 bytes long
    <data> The data. This must be in host natural byte order. The data will be
    converted to network byte order prior to writing.
    [RETURNS] TRUE on success, else FALSE.
*/
{
#if defined(MACHINE_LITTLE_ENDIAN) && defined(Kword16u)
    Kword16u value;
    char *data_ptr = (char *) &value;
#endif

    static char function_name[] = "p_write_buf16";

    if (data > NET_USHORT_MAX)
    {
	(void) fprintf (stderr,
			"WARNING: %s: data: %lu is outside network format range: clipping\n",
			function_name, data);
	data = NET_USHORT_MAX;
    }
#ifdef Kword16u
#  if defined(MACHINE_BIG_ENDIAN) && !defined(NEED_ALIGNED_DATA)
#    define ALWAYS_DONE
    *(Kword16u *) buffer = data;
    return (TRUE);
#  endif
#  if defined(MACHINE_BIG_ENDIAN) && defined(NEED_ALIGNED_DATA)
    if ( ( (int) buffer & 0x1 ) == 0 )
    {
	*(Kword16u *) buffer = data;
	return (TRUE);
    }
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
#    define ALWAYS_DONE
    value = data;
    buffer[0] = data_ptr[1];
    buffer[1] = data_ptr[0];
    return (TRUE);
#  endif
#endif
#ifndef ALWAYS_DONE
    /*  Have to do this the hard way  */
    /*  Byte 1 (LSB) */
    *(unsigned char *) (buffer + 1) = data & BYTE_MASK;
    /*  Byte 0 (MSB)  */
    *(unsigned char *) buffer = (data >> 8) & BYTE_MASK;
    return (TRUE);
#endif
}   /*  End Function p_write_buf16  */

#undef ALWAYS_DONE

/*PUBLIC_FUNCTION*/
flag p_read_buf16 (char *buffer, unsigned long *data)
/*  [PURPOSE] This routine will read 16 bits of unsigned data from a buffer.
    <buffer> A pointer to the buffer. This buffer must be at least 2 bytes long
    <data> The output data will be written here. This will be in host natural
    byte order. The data will be converted from network byte order after
    reading.
    [RETURNS] TRUE on success, else FALSE.
*/
{
#ifdef Kword16u
#  if defined(MACHINE_BIG_ENDIAN) && !defined(NEED_ALIGNED_DATA)
#    define ALWAYS_DONE
    *data = *(Kword16u *) buffer;
    return (TRUE);
#  endif
#  if defined(MACHINE_BIG_ENDIAN) && defined(NEED_ALIGNED_DATA)
    if ( ( (int) buffer & 0x1 ) == 0 )
    {
	*data = *(Kword16u *) buffer;
	return (TRUE);
    }
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
#    define ALWAYS_DONE
    Kword16u value;
    char *data_ptr = (char *) &value;

    data_ptr[0] = buffer[1];
    data_ptr[1] = buffer[0];
    *data = value;
    return (TRUE);
#  endif
#endif
#ifndef ALWAYS_DONE
    /*  Have to do this the hard way  */
    /*  Byte 0 (MSB)  */
    *data = (unsigned long) *(unsigned char *) buffer << 8;
    /*  Byte 1 (LSB)  */
    *data |= *(unsigned char *) (buffer + 1);
    return (TRUE);
#endif
}   /*  End Function p_read_buf16  */

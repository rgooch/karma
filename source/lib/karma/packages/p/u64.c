/*LINTLIBRARY*/
/*  u64.c

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
  packages/p/u64.c

    Updated by      Richard Gooch   23-FEB-1995: Improved portability.

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Last updated by Richard Gooch   9-JUN-1995: Changed to NEED_ALIGNED_DATA.


*/
#include <stdio.h>
#include <karma.h>
#include <os.h>
#include <karma_p.h>

#define BYTE_MASK 0xff

/*PUBLIC_FUNCTION*/
flag p_write_buf64 (buffer, data)
/*  This routine will write 64 bits of data to the buffer pointed to by
    buffer  .
    The data must be given by  data  .This must be in host natural byte order.
    The data will be converted to network byte order prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *buffer;
unsigned long data;
{
#undef NO_FALLBACK
#if defined(Kword64u) && defined(MACHINE_BIG_ENDIAN)
#  ifdef NEED_ALIGNED_DATA
    if ( ( (int) buffer & 0x7 ) == 0 )
    {
	*(Kword64u *) buffer = data;
	return (TRUE);
    }
#  else
    *(Kword64u *) buffer = data;
    return (TRUE);
#    define NO_FALLBACK
#  endif
#else
#  if defined(Kword32u) && defined(MACHINE_BIG_ENDIAN)
#    ifdef NEED_ALIGNED_DATA
    if ( ( (int) buffer & 0x3 ) == 0 )
    {
	*(Kword32u *) buffer = 0;
	buffer += 4;
	*(Kword32u *) buffer = data;
	return (TRUE);
    }
#    else
    *(Kword32u *) buffer = 0;
    buffer += 4;
    *(Kword32u *) buffer = data;
    return (TRUE);
#      define NO_FALLBACK
#    endif
#  endif
#endif
#ifndef NO_FALLBACK
    /*  Convert data to network byte order  */
    /*  Have to do this the hard way  */
    /*  Byte 7 (LSB)  */
    *(unsigned char *) (buffer + 7) = data & BYTE_MASK;
    /*  Byte 6  */
    *(unsigned char *) (buffer + 6) = (data >> 8) & BYTE_MASK;
    /*  Byte 5  */
    *(unsigned char *) (buffer + 5) = (data >> 16) & BYTE_MASK;
    /*  Byte 4  */
    *(unsigned char *) (buffer + 4) = (data >> 24) & BYTE_MASK;
#  ifdef Kword64u
    /*  Byte 3  */
    *(unsigned char *) (buffer + 3) = (data >> 32) & BYTE_MASK;
    /*  Byte 2  */
    *(unsigned char *) (buffer + 2) = (data >> 40) & BYTE_MASK;
    /*  Byte 1  */
    *(unsigned char *) (buffer + 1) = (data >> 48) & BYTE_MASK;
    /*  Byte 0 (MSB)  */
    *(unsigned char *) buffer = (data >> 56) & BYTE_MASK;
#  else
    /*  Byte 3  */
    *(unsigned char *) (buffer + 3) = 0;
    /*  Byte 2  */
    *(unsigned char *) (buffer + 2) = 0;
    /*  Byte 1  */
    *(unsigned char *) (buffer + 1) = 0;
    /*  Byte 0 (MSB)  */
    *(unsigned char *) buffer = 0;
#  endif
    return (TRUE);
#endif
}   /*  End Function p_write_buf64  */

/*PUBLIC_FUNCTION*/
flag p_read_buf64 (buffer, data)
/*  This routine will read 64 bits of data from the buffer pointed to by
    buffer  .This must be at least 8 bytes long.
    The data will be written to the storage pointed to by  data  .This will be
    in host natural byte order.
    The data will be converted from network byte order after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *buffer;
unsigned long *data;
{
#undef NO_FALLBACK
#ifndef Kword64u
    int count, num;

    if (sizeof (unsigned long) < 8)
    {
	/*  Check to make sure data will fit  */
	num = 8 - sizeof (unsigned long);
	for (count = 0; count < num; ++count)
	{
	    if (buffer[count] != 0)
	    {
		(void) fprintf(stderr,"Overflow on reading 64 bit quantity\n");
		(void) fprintf (stderr, "val[0]: %lx  val[1]: %lx\n",
				*(unsigned long *) buffer,
				*(unsigned long *) (buffer + 4));
		return (FALSE);
	    }
	}
    }
#endif
#if defined(Kword64u) && defined(MACHINE_BIG_ENDIAN)
#  ifdef NEED_ALIGNED_DATA
    if ( ( (int) buffer & 0x7 ) == 0 )
    {
	*data = *(Kword64u *) buffer;
	return (TRUE);
    }
#  else
    *data = *(Kword64u *) buffer;
    return (TRUE);
#    define NO_FALLBACK
#  endif
#else
#  if defined(Kword32u) && defined(MACHINE_BIG_ENDIAN)
#    ifdef NEED_ALIGNED_DATA
    if ( ( (int) buffer & 0x3 ) == 0 )
    {
	buffer += 4;
	*data = *(Kword32u *) buffer;
	return (TRUE);
    }
#    else
    buffer += 4;
    *data = *(Kword32u *) buffer;
    return (TRUE);
#      define NO_FALLBACK
#    endif
#  endif
#endif
#ifndef NO_FALLBACK
    /*  Have to do this the hard way  */
#  ifdef Kword64u
    /*  Byte 0 (MSB)  */
    *data = (unsigned long) *(unsigned char *) buffer << 56;
    /*  Byte 1  */
    *data |= (unsigned long) *(unsigned char *) (buffer + 1) << 48;
    /*  Byte 2  */
    *data |= (unsigned long) *(unsigned char *) (buffer + 2) << 40;
    /*  Byte 3  */
    *data |= (unsigned long) *(unsigned char *) (buffer + 3) << 32;
#  else
    *data = 0;
#  endif
    /*  Byte 4  */
    *data |= (unsigned long) *(unsigned char *) (buffer + 4) << 24;
    /*  Byte 5  */
    *data |= (unsigned long) *(unsigned char *) (buffer + 5) << 16;
    /*  Byte 6  */
    *data |= (unsigned long) *(unsigned char *) (buffer + 6) << 8;
    /*  Byte 7 (LSB)  */
    *data |= *(unsigned char *) (buffer + 7);
    return (TRUE);
#endif
}   /*  End Function p_read_buf64  */

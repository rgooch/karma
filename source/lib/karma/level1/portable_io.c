/*LINTLIBRARY*/
/*PREFIX:"p_"*/
/*  portable_io.c

    This code provides conversion between host and cannonical data formats.

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

    This file contains various Karma utility routines which perform conversion
    between host and cannonical data formats.


    Written by      Richard Gooch   12-AUG-1992

    Updated by      Richard Gooch   14-NOV-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   13-FEB-1993: Added  p_write_buf64  and
  p_read_buf64

    Last updated by Richard Gooch   29-JUL-1993: Fixed bugs in  p_write_buf16s
  and  p_read_buf16s  which manifested themselves on byte swapper machines.


*/
#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <karma.h>
#include <os.h>
#include <karma_p.h>

#define BYTE_FACTOR 256
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
    static char function_name[] = "p_write_buf64";

    /*  Temporary  */
#ifdef MATCHING_SIZES
    *(unsigned long *) buffer = 0;
    buffer += 4;
#else
    (void) fprintf (stderr, "64 bit quantities not supported yet\n");
    return (FALSE);
#endif
#ifdef BLOCK_TRANSFER
    *(unsigned long *) buffer = data;
    return (TRUE);
#else
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
    data /= BYTE_FACTOR;
    *(unsigned char *) (buffer + 2) = data & BYTE_MASK;
    /*  Byte 1  */
    data /= BYTE_FACTOR;
    *(unsigned char *) (buffer + 1) = data & BYTE_MASK;
    /*  Byte 0 (MSB)  */
    data /= BYTE_FACTOR;
    *(unsigned char *) buffer = data & BYTE_MASK;
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
    /*  Temporary  */
#ifdef MATCHING_SIZES
    if (*(unsigned long *) buffer != 0)
    {
	(void) fprintf (stderr, "Reading 64 bit quantities not finished\n");
	(void) fprintf (stderr, "Upper 32 bits: %lu\n",
			*(unsigned long *) buffer);
	return (FALSE);
    }
    buffer += 4;
#else
    (void) fprintf (stderr, "64 bit quantities not supported yet\n");
    return (FALSE);
#endif
#ifdef BLOCK_TRANSFER
    *data = *(unsigned long *) buffer;
    return (TRUE);
#else
    /*  Have to do this the hard way  */
    /*  Byte 0 (MSB)  */
    *data = *(unsigned char *) buffer;
    *data *= BYTE_FACTOR;
    /*  Byte 1  */
    *data += *(unsigned char *) (buffer + 1);
    *data *= BYTE_FACTOR;
    /*  Byte 2  */
    *data += *(unsigned char *) (buffer + 2);
    *data *= BYTE_FACTOR;
    /*  Byte 3 (LSB)  */
    *data += *(unsigned char *) (buffer + 3);
    return (TRUE);
#endif
}   /*  End Function p_read_buf64  */

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

#ifdef BLOCK_TRANSFER
    *(unsigned long *) buffer = data;
    return (TRUE);
#else
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
    data /= BYTE_FACTOR;
    *(unsigned char *) (buffer + 2) = data & BYTE_MASK;
    /*  Byte 1  */
    data /= BYTE_FACTOR;
    *(unsigned char *) (buffer + 1) = data & BYTE_MASK;
    /*  Byte 0 (MSB)  */
    data /= BYTE_FACTOR;
    *(unsigned char *) buffer = data & BYTE_MASK;
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
#ifdef BLOCK_TRANSFER
    *data = *(unsigned long *) buffer;
    return (TRUE);
#else
    /*  Have to do this the hard way  */
    /*  Byte 0 (MSB)  */
    *data = *(unsigned char *) buffer;
    *data *= BYTE_FACTOR;
    /*  Byte 1  */
    *data += *(unsigned char *) (buffer + 1);
    *data *= BYTE_FACTOR;
    /*  Byte 2  */
    *data += *(unsigned char *) (buffer + 2);
    *data *= BYTE_FACTOR;
    /*  Byte 3 (LSB)  */
    *data += *(unsigned char *) (buffer + 3);
    return (TRUE);
#endif
}   /*  End Function p_read_buf32  */

/*PUBLIC_FUNCTION*/
flag p_write_buf16 (buffer, data)
/*  This routine will write 16 bits of data to the buffer pointed to by
    buffer  .
    The data must be given by  data  .This must be in host natural byte order.
    The data will be converted to network byte order prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *buffer;
unsigned long data;
{
    static char function_name[] = "p_write_buf16";

    if (data > NET_USHORT_MAX)
    {
	(void) fprintf (stderr,
			"WARNING: %s: data: %lu is outside range for network format: clipping\n",
			function_name, data);
	data = NET_USHORT_MAX;
    }
#ifdef BLOCK_TRANSFER
    *(unsigned short *) buffer = data;
    return (TRUE);
#else
    /*  Have to do this the hard way  */
    /*  Byte 1 (LSB) */
    *(unsigned char *) (buffer + 1) = data & BYTE_MASK;
    /*  Byte 0 (MSB)  */
    data /= BYTE_FACTOR;
    *(unsigned char *) buffer = data & BYTE_MASK;
    return (TRUE);
#endif
}   /*  End Function p_write_buf16  */

/*PUBLIC_FUNCTION*/
flag p_read_buf16 (buffer, data)
/*  This routine will read 16 bits of data from the buffer pointed to by
    buffer  .This must be at least 2 bytes long.
    The data will be written to the storage pointed to by  data  .This will be
    in host natural byte order.
    The data will be converted from network byte order after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *buffer;
unsigned long *data;
{
#ifdef BLOCK_TRANSFER
    *data = *(unsigned short *) buffer;
    return (TRUE);
#else
    /*  Have to do this the hard way  */
    /*  Byte 0 (MSB)  */
    *data = *(unsigned char *) buffer;
    *data *= BYTE_FACTOR;
    /*  Byte 1 (LSB)  */
    *data += *(unsigned char *) (buffer + 1);
    return (TRUE);
#endif
}   /*  End Function p_read_buf16  */

/*PUBLIC_FUNCTION*/
flag p_write_buf32s (buffer, data)
/*  This routine will write 32 bits of signed data to the buffer pointed to by
    buffer  .
    The data must be given by  data  .This must be in host natural byte order.
    The data will be converted to network byte order prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *buffer;
long data;
{
    static char function_name[] = "p_write_buf32s";

#ifdef BLOCK_TRANSFER
    *(long *) buffer = data;
    return (TRUE);
#else
# ifdef BYTE_SWAPPER
    char *data_ptr = (char *) &data;

    buffer[0] = data_ptr[3];
    buffer[1] = data_ptr[2];
    buffer[2] = data_ptr[1];
    buffer[3] = data_ptr[0];
    return (TRUE);
# else
    if (data < NET_INT_MIN)
    {
	(void) fprintf (stderr,
			"WARNING: %s: data: %ld is outside network format range: clipping\n",
			function_name, data);
	data = NET_INT_MIN;
    }
    else if (data > NET_INT_MAX)
    {
	(void) fprintf (stderr,
			"WARNING: data: %ld is outside network format range: clipping\n",
			function_name, data);
	data = NET_INT_MAX;
    }
    /*  Convert data to network byte order  */
    /*  Have to do this the hard way  */
    /*  Byte 3 (LSB)  */
    *(buffer + 3) = data & BYTE_MASK;
    /*  Byte 2  */
    data /= BYTE_FACTOR;
    *(buffer + 2) = data & BYTE_MASK;
    /*  Byte 1  */
    data /= BYTE_FACTOR;
    *(buffer + 1) = data & BYTE_MASK;
    /*  Byte 0 (MSB)  */
    data /= BYTE_FACTOR;
    *buffer = data & BYTE_MASK;
    return (TRUE);
# endif
#endif
}   /*  End Function p_write_buf32s  */

/*PUBLIC_FUNCTION*/
flag p_read_buf32s (buffer, data)
/*  This routine will read 32 bits of signed data from the buffer pointed to by
    buffer  .This must be at least 4 bytes long.
    The data will be written to the storage pointed to by  data  .This will be
    in host natural byte order.
    The data will be converted from network byte order after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *buffer;
long *data;
{
#ifdef BLOCK_TRANSFER
    *data = *(long *) buffer;
    return (TRUE);
#else
# ifdef BYTE_SWAPPER
    char *data_ptr = (char *) data;

    data_ptr[0] = buffer[3];
    data_ptr[1] = buffer[2];
    data_ptr[2] = buffer[1];
    data_ptr[3] = buffer[0];
    return (TRUE);
# else
    /*  Have to do this the hard way  */
    /*  Byte 0 (MSB)  */
    *data = *buffer;
    *data *= BYTE_FACTOR;
    /*  Byte 1  */
    *data += *(buffer + 1);
    *data *= BYTE_FACTOR;
    /*  Byte 2  */
    *data += *(buffer + 2);
    *data *= BYTE_FACTOR;
    /*  Byte 3 (LSB)  */
    *data += *(buffer + 3);
    return (TRUE);
# endif
#endif
}   /*  End Function p_read_buf32s  */

/*PUBLIC_FUNCTION*/
flag p_write_buf16s (buffer, data)
/*  This routine will write 16 bits of signed data to the buffer pointed to by
    buffer  .
    The data must be given by  data  .This must be in host natural byte order.
    The data will be converted to network byte order prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *buffer;
long data;
{
#ifdef BYTE_SWAPPER
    short value;
    char *data_ptr = (char *) &value;
#endif

    static char function_name[] = "p_write_buf16s";

    if (data < NET_SHORT_MIN)
    {
	(void) fprintf (stderr,
			"WARNING: %s: data: %ld is outside network format range: clipping\n",
			function_name, data);
	data = NET_SHORT_MIN;
    }
    else if (data > NET_SHORT_MAX)
    {
	(void) fprintf (stderr,
			"WARNING: %s: data: %ld is outside network format range: clipping\n",
			function_name, data);
	data = NET_SHORT_MAX;
    }
#ifdef BLOCK_TRANSFER
    *(short *) buffer = data;
    return (TRUE);
#else
# ifdef BYTE_SWAPPER
    value = data;
    buffer[0] = data_ptr[1];
    buffer[1] = data_ptr[0];
    return (TRUE);
# else
    /*  Have to do this the hard way  */
    /*  Byte 1 (LSB) */
    *(buffer + 1) = data & BYTE_MASK;
    /*  Byte 0 (MSB)  */
    data /= BYTE_FACTOR;
    *buffer = data & BYTE_MASK;
    return (TRUE);
# endif
#endif
}   /*  End Function p_write_buf16s  */

/*PUBLIC_FUNCTION*/
flag p_read_buf16s (buffer, data)
/*  This routine will read 16 bits of signed data from the buffer pointed to by
    buffer  .This must be at least 2 bytes long.
    The data will be written to the storage pointed to by  data  .This will be
    in host natural byte order.
    The data will be converted from network byte order after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
char *buffer;
long *data;
{
#ifdef BLOCK_TRANSFER
    *data = *(short *) buffer;
    return (TRUE);
#else
# ifdef BYTE_SWAPPER
    short value;
    char *data_ptr = (char *) &value;

    data_ptr[0] = buffer[1];
    data_ptr[1] = buffer[0];
    *data = value;
    return (TRUE);
# else
    /*  Have to do this the hard way  */
    /*  Byte 0 (MSB)  */
    *data = *buffer;
    *data *= BYTE_FACTOR;
    /*  Byte 1 (LSB)  */
    *data += *(buffer + 1);
    return (TRUE);
# endif
#endif
}   /*  End Function p_read_buf16s  */

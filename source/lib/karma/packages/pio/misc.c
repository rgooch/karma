/*LINTLIBRARY*/
/*  misc.c

    This code provides portable data IO routines.

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

/*  This file contains all routines needed for the portable transfer of data to
  and from channel objects.


    Written by      Richard Gooch   13-SEP-1992

    Updated by      Richard Gooch   8-OCT-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   13-FEB-1993: Added  pio_write64  and
  pio_read64

    Updated by      Richard Gooch   15-APR-1994: Improved documentation for
  string reading and writing routines.

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Updated by      Richard Gooch   7-JUL-1994: Added support for float and
  double read and write for Alpha.

    Updated by      Richard Gooch   6-AUG-1994: Moved to full ASCII function
  declaration syntax. Had to do this because of the confused Dec Alpha compiler

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/pio/misc.c

    Last updated by Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>


*/
#include <stdio.h>
#include <ctype.h>
#include <errno.h>
#include <karma.h>
#include <karma_pio.h>
#include <karma_ch.h>
#include <karma_m.h>
#include <karma_p.h>
#include <karma_a.h>
#include <os.h>

/*PUBLIC_FUNCTION*/
flag pio_write64 (Channel channel, unsigned long data)
/*  This routine will write 64 bits of data to the channel object given by
    channel  .
    The data must be given by  data  .This must be in host natural byte order.
    The data will be converted to network byte order prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    char buffer[8];
    extern char *sys_errlist[];

    if (p_write_buf64 (buffer, data) != TRUE)
    {
	return (FALSE);
    }
    if (ch_write (channel, buffer, 8) < 8)
    {
	(void) fprintf (stderr, "Error writing 8 bytes of data\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function pio_write64  */

/*PUBLIC_FUNCTION*/
flag pio_read64 (Channel channel, unsigned long *data)
/*  This routine will read 64 bits of data from the channel object given by
    channel  .
    The data will be written to the storage pointed to by  data  .This will be
    in host natural byte order.
    The data will be converted from network byte order after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    char buffer[8];
    extern char *sys_errlist[];

    if (ch_read (channel, buffer, 8) < 8)
    {
	(void) fprintf (stderr, "Error reading 8 bytes of data\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return ( p_read_buf64 (buffer, data) );
}   /*  End Function pio_read64  */

/*PUBLIC_FUNCTION*/
flag pio_write32 (Channel channel, unsigned long data)
/*  This routine will write 32 bits of data to the channel object given by
    channel  .
    The data must be given by  data  .This must be in host natural byte order.
    The data will be converted to network byte order prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    char buffer[4];
    extern char *sys_errlist[];

    if (p_write_buf32 (buffer, data) != TRUE)
    {
	return (FALSE);
    }
    if (ch_write (channel, buffer, 4) < 4)
    {
	(void) fprintf (stderr, "Error writing 4 bytes of data\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function pio_write32  */

/*PUBLIC_FUNCTION*/
flag pio_read32 (Channel channel, unsigned long *data)
/*  This routine will read 32 bits of data from the channel object given by
    channel  .
    The data will be written to the storage pointed to by  data  .This will be
    in host natural byte order.
    The data will be converted from network byte order after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    char buffer[4];
    extern char *sys_errlist[];

    if (ch_read (channel, buffer, 4) < 4)
    {
	(void) fprintf (stderr, "Error reading 4 bytes of data\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return ( p_read_buf32 (buffer, data) );
}   /*  End Function pio_read32  */

/*PUBLIC_FUNCTION*/
flag pio_write16 (Channel channel, unsigned long data)
/*  This routine will write 16 bits of data to the channel object given by
    channel  .
    The data must be given by  data  .This must be in host natural byte order.
    The data will be converted to network byte order prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    char buffer[2];
    extern char *sys_errlist[];

    if (p_write_buf16 (buffer, data) != TRUE)
    {
	return (FALSE);
    }
    if (ch_write (channel, buffer, 2) < 2)
    {
	(void) fprintf (stderr, "Error writing 2 bytes of data\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function pio_write16  */

/*PUBLIC_FUNCTION*/
flag pio_read16 (Channel channel, unsigned long *data)
/*  This routine will read 16 bits of data from the channel object given by
    channel  .
    The data will be written to the storage pointed to by  data  .This will be
    in host natural byte order.
    The data will be converted from network byte order after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    char buffer[2];
    extern char *sys_errlist[];

    if (ch_read (channel, buffer, 2) < 2)
    {
	(void) fprintf (stderr, "Error reading 2 bytes of data\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return ( p_read_buf16 (buffer, data) );
}   /*  End Function pio_read16  */

#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag pio_write_float (Channel channel, float data)
/*  This routine will write a floating point number to the channel object
    given by  channel  .
    The data must be given by  data  .This must be in host natural format.
    The data will be converted to IEEE network format prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    extern char *sys_errlist[];
    static char function_name[] = "pio_write_float";

    /*  All machines which have the network data format
	must be implemented here  */
#ifdef BLOCK_TRANSFER
#define CONVERSION_SUPPORTED
    if (ch_write (channel, (char *) &data, NET_FLOAT_SIZE) < NET_FLOAT_SIZE)
    {
	(void) fprintf (stderr, "Error writing %d bytes of data\t%s\n",
			NET_FLOAT_SIZE, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
#endif

    /*  Machines which only need byte swapping  */
#if defined(BYTE_SWAPPER) || defined(MACHINE_alpha)
#define CONVERSION_SUPPORTED
    return ( pio_write_swap (channel, (char *) &data, (unsigned int) NET_FLOAT_SIZE) );
#endif

    /*  Machines which require conversion routines  */
#ifdef machine_cray
#define CONVERSION_SUPPORTED
    char *data_ptr;
    char *out_ptr;
    char buffer[NET_FLOAT_SIZE];

    if (CRAY2IEG (2, 1, buffer, 0, (char *) &data, 1) < 0)
    {
	(void) fprintf (stderr, "Error converting Cray float to IEEE float\n");
	return (FALSE);
    }
    if (ch_write (channel, buffer, NET_FLOAT_SIZE) < NET_FLOAT_SIZE)
    {
	(void) fprintf (stderr, "Error writing: %d bytes of data\t%s\n",
			NET_FLOAT_SIZE, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
#endif

#ifndef CONVERSION_SUPPORTED
    (void) fprintf (stderr,
		    "Error: conversion to IEEE network format not supported\n");
    a_prog_bug (function_name);
    return (FALSE);
#endif
}   /*  End Function pio_write_float  */

#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag pio_read_float (Channel channel, float *data)
/*  This routine will read a floating point number from the channel object
    given by  channel  .
    The data will be written to the storage pointed to by  data  .This will be
    in host natural format.
    The data will be converted from IEEE network format after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    extern char *sys_errlist[];
    static char function_name[] = "pio_read_float";

    /*  All machines which have the network data format
	must be implemented here  */
#ifdef BLOCK_TRANSFER
#define CONVERSION_SUPPORTED
    if (ch_read (channel, (char *) data, NET_FLOAT_SIZE) < NET_FLOAT_SIZE)
    {
	(void) fprintf (stderr, "Error reading: %d bytes of data\t%s\n",
			NET_FLOAT_SIZE, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
#endif

    /*  Machines which only need byte swapping  */
#if defined(BYTE_SWAPPER) || defined(MACHINE_alpha)
#define CONVERSION_SUPPORTED
    return ( pio_read_swap (channel, (char *) data, (unsigned int) NET_FLOAT_SIZE) );
#endif

    /*  Machines which require conversion routines  */
#ifdef machine_cray
#define CONVERSION_SUPPORTED
    char buffer[NET_FLOAT_SIZE];

    /*  Read into buffer  */
    if (ch_read (channel, buffer, NET_FLOAT_SIZE) < NET_FLOAT_SIZE)
    {
	(void) fprintf (stderr, "Error reading: %d bytes of data\t%s\n",
			NET_FLOAT_SIZE, sys_errlist[errno]);
	return (FALSE);
    }
    if (IEG2CRAY (2, 1, buffer, 0, data_ptr, 1) < 0)
    {
	(void) fprintf (stderr, "Error converting IEEE float to Cray float\n");
	return (FALSE);
    }
    return (TRUE);
#endif

#ifndef CONVERSION_SUPPORTED
    (void) fprintf (stderr,
		    "Error: conversion to IEEE network format not supported\n");
    a_prog_bug (function_name);
    return (FALSE);
#endif
}   /*  End Function pio_read_float  */

#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag pio_write_double (Channel channel, double data)
/*  This routine will write a double precision floating point number to the
    channel object given by  channel  .
    The data must be given by  data  .This must be in host natural format.
    The data will be converted to IEEE network format prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    extern char *sys_errlist[];
    static char function_name[] = "pio_write_double";

#ifdef BLOCK_TRANSFER
#define CONVERSION_SUPPORTED
    if (ch_write (channel, (char *) &data, NET_DOUBLE_SIZE) < NET_DOUBLE_SIZE)
    {
	(void) fprintf (stderr, "Error writing %d bytes of data\t%s\n",
			NET_DOUBLE_SIZE, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
#endif

    /*  Machines which only need byte swapping  */
#if defined(BYTE_SWAPPER) || defined(MACHINE_alpha)
#define CONVERSION_SUPPORTED
    return ( pio_write_swap (channel, (char *) &data,(unsigned int) NET_DOUBLE_SIZE) );
#endif

    /*  Machines which require conversion routines  */
#ifdef machine_cray
#define CONVERSION_SUPPORTED
    char buffer[NET_DOUBLE_SIZE];

    if (sizeof data == 8)
    {
	if (CRAY2IEG (8, 1, buffer, 0, data_ptr, 1) < 0)
	{
	    (void) fprintf (stderr,
			    "Error converting Cray double to IEEE double\n");
	    return (FALSE);
	}
    }
    else
    {
	if (CRAY2IEG (3, 1, buffer, 0, data_ptr, 1) < 0)
	{
	    (void) fprintf (stderr,
			    "Error converting Cray double to IEEE double\n");
	    return (FALSE);
	}
    }
    if (ch_write (channel, buffer, NET_DOUBLE_SIZE) < NET_DOUBLE_SIZE)
    {
	(void) fprintf (stderr, "Error writing %d bytes of data\t%s\n",
			NET_DOUBLE_SIZE, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
#endif

#ifndef CONVERSION_SUPPORTED
    (void) fprintf (stderr,
		    "Error: conversion to IEEE network format not supported\n");
    a_prog_bug (function_name);
    return (FALSE);
#endif
}   /*  End Function pio_write_double  */

#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag pio_read_double (Channel channel, double *data)
/*  This routine will read a double precision floating point number from the
    channel object given by  channel  .
    The data will be written to the storage pointed to by  data  .This will be
    in host natural format.
    The data will be converted from IEEE network format after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    extern char *sys_errlist[];
    static char function_name[] = "pio_read_double";

    /*  All machines which have the network data format
	must be implemented here  */
#ifdef BLOCK_TRANSFER
#define CONVERSION_SUPPORTED
    if (ch_read (channel, (char *) data, NET_DOUBLE_SIZE) < NET_DOUBLE_SIZE)
    {
	(void) fprintf (stderr, "Error reading: %d bytes of data\t%s\n",
			NET_DOUBLE_SIZE, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
#endif

    /*  Machines which only need byte swapping  */
#if defined(BYTE_SWAPPER) || defined(MACHINE_alpha)
#define CONVERSION_SUPPORTED
    return ( pio_read_swap (channel, (char *) data, (unsigned int) NET_DOUBLE_SIZE) );
#endif

    /*  Machines which require conversion routines  */
#ifdef machine_cray
#define CONVERSION_SUPPORTED
    char buffer[NET_DOUBLE_SIZE];

    /*  Read into buffer  */
    if (ch_read (channel, buffer, NET_DOUBLE_SIZE) < NET_DOUBLE_SIZE)
    {
	(void) fprintf (stderr, "Error reading: %d bytes of data\t%s\n",
			NET_DOUBLE_SIZE, sys_errlist[errno]);
	return (FALSE);
    }
    if (sizeof *data == 8)
    {
	if (IEG2CRAY (8, 1, buffer, 0, data_ptr, 1) < 0)
	{
	    (void) fprintf (stderr,
			    "Error converting IEEE double to Cray double\n");
	    return (FALSE);
	}
    }
    else
    {
	if (IEG2CRAY (3, 1, buffer, 0, data_ptr, 1) < 0)
	{
	    (void) fprintf (stderr,
			    "Error converting IEEE double to Cray double\n");
	    return (FALSE);
	}
    }
    return (TRUE);
#endif

#ifndef CONVERSION_SUPPORTED
    (void) fprintf (stderr,
		    "Error: conversion to IEEE network format not supported\n");
    a_prog_bug (function_name);
    return (FALSE);
#endif
}   /*  End Function pio_read_double  */

/*PUBLIC_FUNCTION*/
flag pio_write32s (Channel channel, long data)
/*  This routine will write 32 bits of signed data to the channel object
    given by  channel  .
    The data must be given by  data  .This must be in host natural byte order.
    The data will be converted to network byte order prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    char buffer[4];
    extern char *sys_errlist[];
    static char function_name[] = "pio_write32s";

    if (p_write_buf32s (buffer, data) != TRUE)
    {
	return (FALSE);
    }
    if (ch_write (channel, buffer, 4) < 4)
    {
	(void) fprintf (stderr, "Error writing 4 bytes of data\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function pio_write32s  */

/*PUBLIC_FUNCTION*/
flag pio_read32s (Channel channel, long *data)
/*  This routine will read 32 bits of data from the channel object given by
    channel  .
    The data will be written to the storage pointed to by  data  .This will be
    in host natural byte order.
    The data will be converted from network byte order after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    char buffer[4];
    extern char *sys_errlist[];
    static char function_name[] = "pio_read32s";

    if (ch_read (channel, buffer, 4) < 4)
    {
	(void) fprintf (stderr, "Error reading 4 bytes of data\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return ( p_read_buf32s (buffer, data) );
}   /*  End Function pio_read32s  */

/*PUBLIC_FUNCTION*/
flag pio_write16s (Channel channel, long data)
/*  This routine will write 16 bits of signed data to the channel object
    given by  channel  .
    The data must be given by  data  .This must be in host natural byte order.
    The data will be converted to network byte order prior to writing.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    char buffer[2];
    extern char *sys_errlist[];
    static char function_name[] = "pio_write16s";

    if (p_write_buf16s (buffer, data) != TRUE)
    {
	return (FALSE);
    }
    if (ch_write (channel, buffer, 2) < 2)
    {
	(void) fprintf (stderr, "Error writing 2 bytes of data\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function pio_write16s  */

/*PUBLIC_FUNCTION*/
flag pio_read16s (Channel channel, long *data)
/*  This routine will read 16 bits of data from the channel object given by
    channel  .
    The data will be written to the storage pointed to by  data  .This will be
    in host natural byte order.
    The data will be converted from network byte order after reading.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    char buffer[2];
    extern char *sys_errlist[];
    static char function_name[] = "pio_read16s";

    if (ch_read (channel, buffer, 2) < 2)
    {
	(void) fprintf (stderr, "Error reading 2 bytes of data\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    return ( p_read_buf16s (buffer, data) );
}   /*  End Function pio_read16s  */

/*PUBLIC_FUNCTION*/
flag pio_write_swap (Channel channel, char *data, unsigned int length)
/*  This routine will a swap a number of bytes of data and write them to the
    channel object given by  channel  .
    The data to be written must be pointed to by  data  .
    The number of bytes to swap and then write must be given by  length  .
    Note that the routine will not swap the data in the original buffer.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int byte_count;
    extern char *sys_errlist[];
    static unsigned int buf_len = 0;
    static char *buffer = NULL;
    static char function_name[] = "pio_write_swap";

    /*  Test to see if old buffer is big enough  */
    if (length > buf_len)
    {
	/*  Old buffer too small  */
	if (buffer != NULL)
	{
	    /*  Deallocate old buffer  */
	    m_free (buffer);
	}
	if ( ( buffer = m_alloc (length) ) == NULL )
	{
	    m_error_notify (function_name, "swap buffer");
	    return (FALSE);
	}
	buf_len = length;
    }
    /*  Copy and swap bytes  */
    for (byte_count = 0, --data; byte_count < length; ++byte_count)
    {
	buffer[byte_count] = data[length - byte_count];
    }
    /*  Write buffer  */
    if (ch_write (channel, buffer, length) < length)
    {
	(void) fprintf (stderr, "Error writing: %u bytes of data\t%s\n",
			length, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function pio_write_swap  */

/*PUBLIC_FUNCTION*/
flag pio_read_swap (Channel channel, char *data, unsigned int length)
/*  This routine will read a number of bytes from the channel object given by
    channel  and will then swap them.
    The data will be written to the storage pointed to by  data  .
    The number of bytes to read and then swap must be given by  length  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned int byte_count;
    extern char *sys_errlist[];
    static unsigned int buf_len = 0;
    static char *buffer = NULL;
    static char function_name[] = "pio_read_swap";

    /*  Test to see if old buffer is big enough  */
    if (length > buf_len)
    {
	/*  Old buffer too small  */
	if (buffer != NULL)
	{
	    /*  Deallocate old buffer  */
	    m_free (buffer);
	}
	if ( ( buffer = m_alloc (length) ) == NULL )
	{
	    m_error_notify (function_name, "swap buffer");
	    return (FALSE);
	}
	buf_len = length;
    }
    /*  Read buffer  */
    if (ch_read (channel, buffer, length) < length)
    {
	(void) fprintf (stderr, "Error reading: %u bytes of data\t%s\n",
			length, sys_errlist[errno]);
	return (FALSE);
    }
    /*  Copy and swap bytes  */
    for (byte_count = 0, --data; byte_count < length; ++byte_count)
    {
	data[length - byte_count] = buffer[byte_count];
    }
    return (TRUE);
}   /*  End Function pio_read_swap  */

/*PUBLIC_FUNCTION*/
flag pio_write_string (Channel channel, char *string)
/*  This routine will write a string to a channel.
    The channel object must be given by  channel  .
    The string must be pointed to by  string  .If this is NULL, a zero length
    string is written.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    unsigned long length;
    static char function_name[] = "pio_write_string";

    if (string == NULL)
    {
	length = 0;
    }
    else
    {
	length = strlen (string);
    }
    if (pio_write32 (channel, length) != TRUE)
    {
	return (FALSE);
    }
    if (length < 1)
    {
	return (TRUE);
    }
    if (ch_write (channel, string, (unsigned int) length) < length)
    {
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function pio_write_string  */

/*PUBLIC_FUNCTION*/
char *pio_read_string (Channel channel, unsigned int *length)
/*  This routine will read a string from a channel.
    The channel object must be given by  channel  .
    The length of the string will be written to the storage pointed to by
    length  .If this is NULL, nothing is written here.
    The routine returns a pointer to a dynamically allocated string on success,
    else it returns NULL. If a zero length string was read, the returned buffer
    is one character long and contains the '\0' character.
*/
{
    unsigned long len;
    char *string;
    extern char *sys_errlist[];
    static char function_name[] = "pio_read_string";

    if (pio_read32 (channel, &len) != TRUE)
    {
	a_func_abort (function_name, "Error reading string length");
	return (NULL);
    }
    if ( ( string = m_alloc (len + 1) ) == NULL )
    {
	m_error_notify (function_name, "string");
	return (NULL);
    }
    if (len < 1)
    {
	/*  Empty string  */
	string[0] = '\0';
	if (length != NULL)
	{
	    *length = len;
	}
	return (string);
    }
    /*  Read string  */
    if (ch_read (channel, string, (unsigned int) len) < len)
    {
	(void) fprintf (stderr, "Error reading: %u bytes\t%s\n",
			(unsigned int) len, sys_errlist[errno]);
	a_func_abort (function_name, "could not read string");
	m_free (string);
	return (NULL);
    }
    string[len] = '\0';
    if (length != NULL)
    {
	*length = len;
    }
    return (string);
}   /*  End Function pio_read_string  */

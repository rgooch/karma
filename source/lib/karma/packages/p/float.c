/*LINTLIBRARY*/
/*  float.c

    This code provides conversion between host and cannonical data formats.

    Copyright (C) 1995  Richard Gooch

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


    Written by      Richard Gooch   31-JUL-1995: Copied hard bits from
  pio/misc.c

    Last updated by Richard Gooch   31-JUL-1995


*/
#include <stdio.h>
#include <karma.h>
#include <os.h>
#include <karma_p.h>

#ifdef MACHINE_crayPVP
fortran int IEG2CRAY (int, int, char *, int, char *, int);
fortran int CRAY2IEG (int, int, char *, int, char *, int);
#endif


#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag p_write_buf_float (char *buffer, float data)
/*  [PURPOSE] This routine will write a floating point number to a buffer.
    <buffer> A pointer to the buffer. This buffer must be at least 4 bytes long
    <data> The data. This must be in host natural byte order. The data will be
    converted to IEEE network format prior to writing.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char *ptr = (char *) &data;
    extern char *sys_errlist[];

#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
#    define CONVERSION_SUPPORTED
    buffer[0] = ptr[0];
    buffer[1] = ptr[1];
    buffer[2] = ptr[2];
    buffer[3] = ptr[3];
    return (TRUE);
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
#    define CONVERSION_SUPPORTED
    buffer[3] = ptr[0];
    buffer[2] = ptr[1];
    buffer[1] = ptr[2];
    buffer[0] = ptr[3];
    return (TRUE);
#  endif
#endif
    /*  Machines which require conversion routines  */
#ifdef MACHINE_crayPVP
#  define CONVERSION_SUPPORTED
    char buf[NET_FLOAT_SIZE];

    if (CRAY2IEG (2, 1, buf, 0, ptr, 1) < 0)
    {
	(void) fprintf (stderr, "Error converting Cray float to IEEE float\n");
	return (FALSE);
    }
    buffer[0] = buf[0];
    buffer[1] = buf[1];
    buffer[2] = buf[2];
    buffer[3] = buf[3];
    return (TRUE);
#endif

#ifndef CONVERSION_SUPPORTED
    (void) fprintf (stderr,
		    "Error: conversion to IEEE network format not supported\n");
    return (FALSE);
#endif
}   /*  End Function p_write_buf_float  */

#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag p_read_buf_float (char *buffer, float *data)
/*  [PURPOSE] This routine will read a floating point number from a buffer.
    <buffer> A pointer to the buffer. This buffer must be at least 4 bytes long
    <data> The output data will be written here. This will be in host natural
    byte order. The data will be converted from network byte order after
    reading.
    [NOTES] The data will be converted from IEEE network format after reading.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char *ptr = (char *) data;
    extern char *sys_errlist[];

#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
#    define CONVERSION_SUPPORTED
    ptr[0] = buffer[0];
    ptr[1] = buffer[1];
    ptr[2] = buffer[2];
    ptr[3] = buffer[3];
    return (TRUE);
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
#    define CONVERSION_SUPPORTED
    ptr[3] = buffer[0];
    ptr[2] = buffer[1];
    ptr[1] = buffer[2];
    ptr[0] = buffer[3];
    return (TRUE);
#  endif
#endif
    /*  Machines which require conversion routines  */
#ifdef MACHINE_crayPVP
#  define CONVERSION_SUPPORTED
    char buf[NET_FLOAT_SIZE];

    buf[0] = buffer[0];
    buf[1] = buffer[1];
    buf[2] = buffer[2];
    buf[3] = buffer[3];
    if (IEG2CRAY (2, 1, buf, 0, ptr, 1) < 0)
    {
	(void) fprintf (stderr, "Error converting IEEE float to Cray float\n");
	return (FALSE);
    }
    return (TRUE);
#endif

#ifndef CONVERSION_SUPPORTED
    (void) fprintf (stderr,
		    "Error: conversion to IEEE network format not supported\n");
    return (FALSE);
#endif
}   /*  End Function p_read_buf_float  */

#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag p_write_buf_double (char *buffer, double data)
/*  [PURPOSE] This routine will write a double precision floating point number
    to a buffer.
    <buffer> A pointer to the buffer. This buffer must be at least 8 bytes long
    <data> The data. This must be in host natural byte order. The data will be
    converted to IEEE network format prior to writing.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char *ptr = (char *) &data;
    extern char *sys_errlist[];

#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
#    define CONVERSION_SUPPORTED
    buffer[0] = ptr[0];
    buffer[1] = ptr[1];
    buffer[2] = ptr[2];
    buffer[3] = ptr[3];
    buffer[4] = ptr[4];
    buffer[5] = ptr[5];
    buffer[6] = ptr[6];
    buffer[7] = ptr[7];
    return (TRUE);
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
#    define CONVERSION_SUPPORTED
    buffer[7] = ptr[0];
    buffer[6] = ptr[1];
    buffer[5] = ptr[2];
    buffer[4] = ptr[3];
    buffer[3] = ptr[4];
    buffer[2] = ptr[5];
    buffer[1] = ptr[6];
    buffer[0] = ptr[7];
    return (TRUE);
#  endif
#endif
    /*  Machines which require conversion routines  */
#ifdef MACHINE_crayPVP
#  define CONVERSION_SUPPORTED
    char buf[NET_DOUBLE_SIZE];

    if (sizeof data == 8)
    {
	if (CRAY2IEG (8, 1, buf, 0, ptr, 1) < 0)
	{
	    (void) fprintf (stderr,
			    "Error converting Cray double to IEEE double\n");
	    return (FALSE);
	}
    }
    else
    {
	if (CRAY2IEG (3, 1, buf, 0, ptr, 1) < 0)
	{
	    (void) fprintf (stderr,
			    "Error converting Cray double to IEEE double\n");
	    return (FALSE);
	}
    }
    buffer[0] = buf[0];
    buffer[1] = buf[1];
    buffer[2] = buf[2];
    buffer[3] = buf[3];
    buffer[4] = buf[4];
    buffer[5] = buf[5];
    buffer[6] = buf[6];
    buffer[7] = buf[7];
    return (TRUE);
#endif

#ifndef CONVERSION_SUPPORTED
    (void) fprintf (stderr,
		    "Error: conversion to IEEE network format not supported\n");
    return (FALSE);
#endif
}   /*  End Function p_write_buf_double  */

#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag p_read_buf_double (char *buffer, double *data)
/*  [PURPOSE] This routine will read a double precision floating point number
    from a buffer.
    <buffer> A pointer to the buffer. This buffer must be at least 8 bytes long
    <data> The output data will be written here. This will be in host natural
    byte order. The data will be converted from network byte order after
    reading.
    [NOTES] The data will be converted from IEEE network format after reading.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char *ptr = (char *) data;
    extern char *sys_errlist[];

#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
#    define CONVERSION_SUPPORTED
    ptr[0] = buffer[0];
    ptr[1] = buffer[1];
    ptr[2] = buffer[2];
    ptr[3] = buffer[3];
    ptr[4] = buffer[4];
    ptr[5] = buffer[5];
    ptr[6] = buffer[6];
    ptr[7] = buffer[7];
    return (TRUE);
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
#    define CONVERSION_SUPPORTED
    ptr[7] = buffer[0];
    ptr[6] = buffer[1];
    ptr[5] = buffer[2];
    ptr[4] = buffer[3];
    ptr[3] = buffer[4];
    ptr[2] = buffer[5];
    ptr[1] = buffer[6];
    ptr[0] = buffer[7];
    return (TRUE);
#  endif
#endif
    /*  Machines which require conversion routines  */
#ifdef MACHINE_crayPVP
#  define CONVERSION_SUPPORTED
    char buf[NET_DOUBLE_SIZE];

    buf[0] = buffer[0];
    buf[1] = buffer[1];
    buf[2] = buffer[2];
    buf[3] = buffer[3];
    buf[4] = buffer[4];
    buf[5] = buffer[5];
    buf[6] = buffer[6];
    buf[7] = buffer[7];
    if (sizeof *data == 8)
    {
	if (IEG2CRAY (8, 1, buffer, 0, ptr, 1) < 0)
	{
	    (void) fprintf (stderr,
			    "Error converting IEEE double to Cray double\n");
	    return (FALSE);
	}
    }
    else
    {
	if (IEG2CRAY (3, 1, buffer, 0, ptr, 1) < 0)
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
    return (FALSE);
#endif
}   /*  End Function p_read_buf_double  */

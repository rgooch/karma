/*LINTLIBRARY*/
/*  float.c

    This code provides conversion between host and cannonical data formats.

    Copyright (C) 1995-1996  Richard Gooch

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

    Updated by      Richard Gooch   31-JUL-1995

    Updated by      Richard Gooch   12-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   12-SEP-1996: Finished routines to convert
  floats and doubles and check for NaNs.


*/
#include <stdio.h>
#include <karma.h>
#include <os.h>
#include <karma_p.h>

#ifdef MACHINE_crayPVP
fortran int IEG2CRAY (int, int, char *, int, char *, int);
fortran int CRAY2IEG (int, int, char *, int, char *, int);
#endif


#define NUM_NANS 3

/*  Big-endian float NaNs  */
static unsigned char fnans_be[NUM_NANS][NET_FLOAT_SIZE] =
{
    {0x7f, 0x80, 0x0f, 0},    /*  Signal NaN  */
    {0x7f, 0xc0, 0, 0},       /*  Quiet NaN. It is also indefinite(?)  */
    {0xff, 0xff, 0xff, 0xff}
};

/*  Big-endian double NaNs  */
static unsigned char dnans_be[NUM_NANS][NET_DOUBLE_SIZE] =
{
    {0x7f, 0xf0, 0, 0, 0, 0x0f, 0, 0},  /*  Signal NaN  */
    {0x7f, 0xf8, 0, 0, 0, 0, 0, 0}, /*  Quiet NaN, It is also indefinite(?)  */
    {0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff, 0xff}
};


#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag p_write_buf_float (char *buffer, float data)
/*  [SUMMARY] Write a floating point number to a buffer.
    <buffer> A pointer to the buffer. This buffer must be at least 4 bytes long
    <data> The data. This must be in host natural byte order. The data will be
    converted to IEEE network format prior to writing.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char *ptr = (char *) &data;

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
	fprintf (stderr, "Error converting Cray float to IEEE float\n");
	return (FALSE);
    }
    buffer[0] = buf[0];
    buffer[1] = buf[1];
    buffer[2] = buf[2];
    buffer[3] = buf[3];
    return (TRUE);
#endif

#ifndef CONVERSION_SUPPORTED
    fprintf (stderr,
	     "Error: conversion to IEEE network format not supported\n");
    return (FALSE);
#endif
}   /*  End Function p_write_buf_float  */

#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag p_read_buf_float (CONST char *buffer, float *data)
/*  [SUMMARY] Read a floating point number from a buffer.
    <buffer> A pointer to the buffer. This buffer must be at least 4 bytes long
    <data> The output data will be written here. This will be in host natural
    byte order. The data will be converted from network byte order after
    reading.
    [NOTE] The data will be converted from IEEE network format after reading.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char *ptr = (char *) data;

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
	fprintf (stderr, "Error converting IEEE float to Cray float\n");
	return (FALSE);
    }
    return (TRUE);
#endif

#ifndef CONVERSION_SUPPORTED
    fprintf (stderr,
	     "Error: conversion to IEEE network format not supported\n");
    return (FALSE);
#endif
}   /*  End Function p_read_buf_float  */

#undef CONVERSION_SUPPORTED

/*EXPERIMENTAL_FUNCTION*/
flag p_read_buf_floats (CONST char *buffer, uaddr num_values, float *data,
			uaddr *num_nan)
/*  [SUMMARY] Read floating point data from a buffer, trapping NaNs.
    [PURPOSE] This routine will read many floating point numbers from a buffer
    and will optionally trap IEEE Not-A-Number (NaN) values. Trapped NaNs are
    converted to the TOOBIG value.
    <buffer> A pointer to the buffer. This buffer must be at least
    4 * <<num_values>> bytes long
    <num_values> The number of values to read.
    <data> The data will be written here. This will be written in host natural
    format. The data will be converted from IEEE network format prior to
    writing.
    <num_nan> The number of NaN values found will be written here. If this is
    NULL then NaN values are not trapped.
    [RETURNS] The number of values read and converted.
*/
{
    flag foundnan;
#ifdef Kword32u
    Kword32u nan0, nan1, nan2;
#else
    flag equal;
    unsigned int byte_count, nan_count;
#endif
    uaddr num_nan_local = 0;
    char *b_ptr;
#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
    float *f_ptr;
#  endif
#endif
#ifdef Kword32u
    static char function_name[] = "p_read_buf_floats";
#endif

#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
#    define CONVERSION_SUPPORTED
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
#    define CONVERSION_SUPPORTED
#  endif
#endif
    if (num_nan == NULL)
    {
	/*  No NaN trap: try to speed things up  */
#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
	for (f_ptr = (float *) buffer; num_values > 0;
	     --num_values, ++f_ptr, ++data) *data = *f_ptr;
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
	for (b_ptr = (char *) data; num_values > 0;
	     --num_values, buffer += NET_FLOAT_SIZE, b_ptr += NET_FLOAT_SIZE)
	{
	    b_ptr[0] = buffer[3];
	    b_ptr[1] = buffer[1];
	    b_ptr[2] = buffer[2];
	    b_ptr[3] = buffer[0];
	}
#  endif
#endif
#ifndef CONVERSION_SUPPORTED
	for (; num_values > 0; buffer += NET_FLOAT_SIZE, ++data)
	{
	    if ( !p_read_buf_float (buffer, data) ) return (FALSE);
	}
#endif
	return (TRUE);
    }
    /*  Must trap for NaNs  */
#ifdef Kword32u
    if (NUM_NANS != 3)
    {
	fprintf (stderr, "%s: insufficient NaN checking. Library bug.\n",
		 function_name);
	abort ();
    }
    nan0 = *(Kword32u *) fnans_be[0];
    nan1 = *(Kword32u *) fnans_be[1];
    nan2 = *(Kword32u *) fnans_be[2];
#endif
    /*  Convert and possibly NaN trap  */
    for ( ; num_values > 0; --num_values, buffer += NET_FLOAT_SIZE, ++data)
    {
	/*  Convert this value and trap for NaNs  */
	foundnan = FALSE;
#ifdef Kword32u
	/*  Can do a fast test  */
	if ( (nan0 == *(Kword32u *) buffer) ||
	     (nan1 == *(Kword32u *) buffer) ||
	     (nan2 == *(Kword32u *) buffer) ) foundnan = TRUE;
#else
	for (nan_count = 0; nan_count < NUM_NANS; ++nan_count)
	{
	    for (byte_count = 0, equal = TRUE;
		 equal && (byte_count < NET_FLOAT_SIZE);
		 ++byte_count)
	    {
		if (*(unsigned char *) (buffer + byte_count) !=
		    fnans_be[nan_count][byte_count])
		{
		    equal = FALSE;
		}
	    }
	    if (equal) foundnan = TRUE;
	}
#endif
	if (foundnan)
	{
	    *data = TOOBIG;
	    ++num_nan_local;
	    continue;
	}
	/*  Not a NaN: convert. Optimise if possible  */
#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
	*data = *(float *) buffer;
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
	*(char *) data = buffer[3];
	*( (char *) data + 1 ) = buffer[2];
	*( (char *) data + 2 ) = buffer[1];
	*( (char *) data + 3 ) = buffer[0];
#  endif
#endif
#ifndef CONVERSION_SUPPORTED
	if ( !p_read_buf_float (buffer, data) ) return (FALSE);
#endif
    }
    *num_nan = num_nan_local;
    return (TRUE);
}   /*  End Function p_read_buf_floats  */

#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag p_write_buf_double (char *buffer, double data)
/*  [SUMMARY] Write a double precision floating point number to a buffer.
    <buffer> A pointer to the buffer. This buffer must be at least 8 bytes long
    <data> The data. This must be in host natural byte order. The data will be
    converted to IEEE network format prior to writing.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char *ptr = (char *) &data;

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
	    fprintf (stderr, "Error converting Cray double to IEEE double\n");
	    return (FALSE);
	}
    }
    else
    {
	if (CRAY2IEG (3, 1, buf, 0, ptr, 1) < 0)
	{
	    fprintf (stderr, "Error converting Cray double to IEEE double\n");
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
    fprintf (stderr,
	     "Error: conversion to IEEE network format not supported\n");
    return (FALSE);
#endif
}   /*  End Function p_write_buf_double  */

#undef CONVERSION_SUPPORTED

/*PUBLIC_FUNCTION*/
flag p_read_buf_double (CONST char *buffer, double *data)
/*  [SUMMARY] Read a double precision floating point number from a buffer.
    <buffer> A pointer to the buffer. This buffer must be at least 8 bytes long
    <data> The output data will be written here. This will be in host natural
    byte order. The data will be converted from network byte order after
    reading.
    [NOTE] The data will be converted from IEEE network format after reading.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char *ptr = (char *) data;

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
	    fprintf (stderr, "Error converting IEEE double to Cray double\n");
	    return (FALSE);
	}
    }
    else
    {
	if (IEG2CRAY (3, 1, buffer, 0, ptr, 1) < 0)
	{
	    fprintf (stderr, "Error converting IEEE double to Cray double\n");
	    return (FALSE);
	}
    }
    return (TRUE);
#endif

#ifndef CONVERSION_SUPPORTED
    fprintf (stderr,
	     "Error: conversion to IEEE network format not supported\n");
    return (FALSE);
#endif
}   /*  End Function p_read_buf_double  */

#undef CONVERSION_SUPPORTED

/*EXPERIMENTAL_FUNCTION*/
flag p_read_buf_doubles (CONST char *buffer, uaddr num_values, double *data,
			 uaddr *num_nan)
/*  [SUMMARY] Read double floating point data from a buffer, trapping NaNs.
    [PURPOSE] This routine will read many double precision floating point
    numbers from a buffer and will optionally trap IEEE Not-A-Number (NaN)
    values. Trapped NaNs are converted to the TOOBIG value.
    <buffer> A pointer to the buffer. This buffer must be at least
    8 * <<num_values>> bytes long
    <num_values> The number of values to read.
    <data> The data will be written here. This will be written in host natural
    format. The data will be converted from IEEE network format prior to
    writing.
    <num_nan> The number of NaN values found will be written here. If this is
    NULL then NaN values are not trapped.
    [RETURNS] The number of values read and converted.
*/
{
    flag foundnan;
#ifdef Kword64u
    Kword64u nan0, nan1, nan2;
#else
    flag equal;
    unsigned int byte_count, nan_count;
#endif
    uaddr num_nan_local = 0;
    char *b_ptr;
#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
    double *d_ptr;
#  endif
#endif
#ifdef Kword64u
    static char function_name[] = "p_read_buf_doubles";
#endif

#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
#    define CONVERSION_SUPPORTED
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
#    define CONVERSION_SUPPORTED
#  endif
#endif
    if (num_nan == NULL)
    {
	/*  No NaN trap: try to speed things up  */
#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
	for (d_ptr = (double *) buffer; num_values > 0;
	     --num_values, ++d_ptr, ++data) *data = *d_ptr;
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
	for (b_ptr = (char *) data; num_values > 0;
	     --num_values, buffer += NET_DOUBLE_SIZE, b_ptr += NET_DOUBLE_SIZE)
	{
	    b_ptr[0] = buffer[7];
	    b_ptr[1] = buffer[6];
	    b_ptr[2] = buffer[5];
	    b_ptr[3] = buffer[4];
	    b_ptr[4] = buffer[3];
	    b_ptr[5] = buffer[1];
	    b_ptr[6] = buffer[2];
	    b_ptr[7] = buffer[0];
	}
#  endif
#endif
#ifndef CONVERSION_SUPPORTED
	for (; num_values > 0; buffer += NET_DOUBLE_SIZE, ++data)
	{
	    if ( !p_read_buf_double (buffer, data) ) return (FALSE);
	}
#endif
	return (TRUE);
    }
    /*  Must trap for NaNs  */
#ifdef Kword64u
    if (NUM_NANS != 3)
    {
	fprintf (stderr, "%s: insufficient NaN checking. Library bug.\n",
		 function_name);
	abort ();
    }
    nan0 = *(Kword64u *) dnans_be[0];
    nan1 = *(Kword64u *) dnans_be[1];
    nan2 = *(Kword64u *) dnans_be[2];
#endif
    /*  Convert and possibly NaN trap  */
    for ( ; num_values > 0; --num_values, buffer += NET_DOUBLE_SIZE, ++data)
    {
	/*  Convert this value and trap for NaNs  */
	foundnan = FALSE;
#ifdef Kword64u
	/*  Can do a fast test  */
	if ( (nan0 == *(Kword64u *) buffer) ||
	     (nan1 == *(Kword64u *) buffer) ||
	     (nan2 == *(Kword64u *) buffer) ) foundnan = TRUE;
#else
	for (nan_count = 0; nan_count < NUM_NANS; ++nan_count)
	{
	    for (byte_count = 0, equal = TRUE;
		 equal && (byte_count < NET_DOUBLE_SIZE);
		 ++byte_count)
	    {
		if (buffer[byte_count] !=
		    dnans_be[nan_count][byte_count])
		{
		    equal = FALSE;
		}
	    }
	    if (equal) foundnan = TRUE;
	}
#endif
	if (foundnan)
	{
	    *data = TOOBIG;
	    ++num_nan_local;
	    continue;
	}
	/*  Not a NaN: convert. Optimise if possible  */
#ifdef HAS_IEEE
#  ifdef MACHINE_BIG_ENDIAN
	*data = *(double *) buffer;
#  endif
#  ifdef MACHINE_LITTLE_ENDIAN
	*(char *) data = buffer[7];
	*( (char *) data + 1 ) = buffer[6];
	*( (char *) data + 2 ) = buffer[5];
	*( (char *) data + 3 ) = buffer[4];
	*( (char *) data + 4 ) = buffer[3];
	*( (char *) data + 5 ) = buffer[2];
	*( (char *) data + 6 ) = buffer[1];
	*( (char *) data + 7 ) = buffer[0];
#  endif
#endif
#ifndef CONVERSION_SUPPORTED
	if ( !p_read_buf_double (buffer, data) ) return (FALSE);
#endif
    }
    *num_nan = num_nan_local;
    return (TRUE);
}   /*  End Function p_read_buf_doubles  */

#undef CONVERSION_SUPPORTED

/*
    Various definitions for Karma.

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


/*-----------------------------------------------------------*
 *     This file contains some useful macros, definitions    *
 *     and functions that no program should be without       *
 *     (at least in my opinion)                              *
 *-----------------------------------------------------------*/

#ifndef KARMA_H
#define KARMA_H

/*-----------------------------------------------------------*
 *     Logical variables and definitions                     *
 *-----------------------------------------------------------*/

typedef int flag;

#define TRUE 1
#define FALSE 0

/*-----------------------------------------------------------*
 *     Definition of NULL                                    *
 *-----------------------------------------------------------*/

#ifndef NULL
#define NULL (void *)0
#endif

/*-----------------------------------------------------------*
 *     ERRNO_TYPE                                            *
 *-----------------------------------------------------------*/

#ifdef __ZTC__
#define ERRNO_TYPE extern volatile int
#else
#define ERRNO_TYPE extern int
#endif

/*-----------------------------------------------------------*
 *     PI Definitions                                        *
 *-----------------------------------------------------------*/

#ifndef PI
#define PI 3.14159265358979
#endif
#define PI_ON_2 1.57079632679489
#define TWO_PI 6.28318530717958
#define PION180 (PI/180.0)
#define TWOPI TWO_PI

/*-----------------------------------------------------------*
 *   Some other numerical definitions                        *
 *-----------------------------------------------------------*/

#define TOOBIG  1.0e30

/*-----------------------------------------------------------*
 *  Some useful lengths                                      *
 *-----------------------------------------------------------*/

#define STRING_LENGTH 255
#define K_SIZE 1024
#define M_SIZE 1048576
#define SPACE_LENGTH 21
#define TABWIDTH 8
#define PAGE_LENGTH 60

/*-----------------------------------------------------------*
 *  These are the standard return values (returned by  exit) *
 *-----------------------------------------------------------*/

#define RV_OK 0
#define RV_BAD_PARAM 1
#define RV_MISSING_PARAM 2
#define RV_TOO_MANY_PARAM 3
#define RV_BAD_OPTION 4
#define RV_FILE_NOT_FOUND 5
#define RV_CANNOT_OPEN 6
#define RV_READ_ERROR 7
#define RV_WRITE_ERROR 8
#define RV_MEM_ERROR 9
#define RV_SYS_ERROR 10
#define RV_BAD_DATA 11
#define RV_PROGRAM_BUG 12
#define RV_BAD_FILE_TYPE 13
#define RV_CONTROL_C 14
#define RV_ABORT 15
#define RV_UNDEF_ERROR 1023

/*-----------------------------------------------------------*
 *     Wait for keypress                                     *
 *-----------------------------------------------------------*/

#define PAUSE do{char i[20];gets(i);}while(0)


/*-----------------------------------------------------------*
 *     Wait for keypress                                     *
 *-----------------------------------------------------------*/

#define FLAG_VERIFY(flag) if ( (flag != TRUE) && (flag != FALSE) ) \
                          {(void) fprintf (stderr, "%s: Bad flag value: %d\n",\
					   function_name, flag); \
			   (void) fprintf (stderr, "Aborting.%c\n", BEL);  \
			   exit (RV_UNDEF_ERROR);  \
		          }

/*-----------------------------------------------------------*
 *     Define some ASCII values                              *
 *-----------------------------------------------------------*/

#define BEL 7
#define BS 8
#define HT 9
#define LF 10
#define VT 11
#define FF 12
#define CR 13
#define ESC 27
#define DEL 127

/*-----------------------------------------------------------*/

#endif  /*  KARMA_H  */

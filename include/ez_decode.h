/*
    Definition of structure used to describe variables to ez_decode

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

struct variable_description
{
  char *name;     /*  name of variable                                     */
  char *units;    /*  units of this variable                               */
  int type;       /*  one of INT, FLOAT, STRING, FUNCTION, etc.            */
  char *val;      /*  pointer to value (pointer to pointer for strings)    */
  int dim;        /*  0 or 1 for a simple variable, >1 for an array    
		   *  For a CHOICE variable, dim is dimension of choice
		   */
  int *count;     /*  pointer to count of values in `val'              
		   *  or pointer to list of choices (e.g. char *choose[])
		   */
  char *format;   /*  print out format                                     */
};

typedef  char *(*CFUNC)();
typedef  void (*VFUNC)();

char *ez_decode_fp ();

#ifndef NUMTYPES

#include <karma_ds_def.h>

#endif

#if NUMTYPES > 30000
    !!!! ERROR !!!!    **** NUMTYPES is greater than 30000 ****
#endif

#define  STRING        30001
#define  FUNCTION      30002
#define  ONOFF         30003
#define  GROUP         30004
#define  ROWS          30005
#define  COLS          30006
#define  CHOICE        30007
#define  CFUNCTION     30008
#define  LAST_VARIABLE 30009

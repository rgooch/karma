/*  karma_dsproc.h

    Header for  dsproc_  package.

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

    This include file contains all the definitions and function declarations
  needed to interface to the dsproc_ routines in the Karma library.


    Written by      Richard Gooch   16-OCT-1992

    Last updated by Richard Gooch   29-NOV-1993

*/

#ifndef KARMA_DSPROC_H
#define KARMA_DSPROC_H


#include <karma_ds_def.h>

/*  File:   dsproc.c   */
EXTERN_FUNCTION (void dsproc_object, (char *object, char *array_names[],
				      unsigned int num_arrays,
				      flag save_unproc_data,
				      flag (*pre_process) (),
				      flag (*process_array) (),
				      flag (*post_process) (),
				      unsigned int mmap_option) );


#endif /*  KARMA_DSPROC_H  */

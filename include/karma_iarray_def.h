/*  karma_iarray_def.h

    Header for  iarray_  package. This file ONLY contains the object
    definitions.

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

    This include file contains all the definitions for the iarray_ routines
    in the Karma library.


    Written by      Richard Gooch   24-DEC-1995

    Last updated by Richard Gooch   24-DEC-1995

*/

#ifndef KARMA_IARRAY_DEF_H
#define KARMA_IARRAY_DEF_H

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#if !defined(KARMA_C_DEF_H) || defined(MAKEDEPEND)
#  include <karma_c_def.h>
#endif

#if !defined(KARMA_DS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ds_def.h>
#endif


/*  Structure declarations  */
typedef struct
{
    char *data;             /*  Pointer to start of array  */
    uaddr **offsets;        /*  Array of offset pointers for each dimension  */
    uaddr *lengths;         /*  Array of dimension lengths                   */
    flag *contiguous;
    packet_desc *top_pack_desc;
    char **top_packet;
    multi_array *multi_desc;
    array_desc *arr_desc;
    unsigned int array_num;
    int boundary_width;
    unsigned int elem_index;
    unsigned int num_dim;
    unsigned int *orig_dim_indices;
    unsigned int *restrictions;
    unsigned int magic_number;
    KCallbackList destroy_callbacks;
} *iarray;


#endif /*  KARMA_IARRAY_DEF_H  */

/*  karma_dmp.h

    Header for  dmp_  package.

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

    This include file contains all the definitions and function declarations
  needed to interface to the dmp_ routines in the Karma library.


    Written by      Richard Gooch   3-OCT-1992

    Last updated by Richard Gooch   7-APR-1995

*/

#ifndef FILE
#  include <stdio.h>
#endif

#if !defined(KARMA_DS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ds_def.h>
#endif

#ifndef KARMA_DMP_H
#define KARMA_DMP_H


/*  File:   diagnose.c  */
EXTERN_FUNCTION (void dmp_multi_desc, (FILE *fp,
				       multi_array *multi_desc,
				       flag comments) );
EXTERN_FUNCTION (void dmp_packet_desc, (FILE *fp,
					packet_desc *pack_desc,
					flag comments) );
EXTERN_FUNCTION (void dmp_element_desc, (FILE *fp, unsigned int type,
					 char *desc, flag comments) );
EXTERN_FUNCTION (void dmp_array_desc, (FILE *fp,
				       array_desc *arr_desc,
				       flag comments) );
EXTERN_FUNCTION (void dmp_dim_desc, (FILE *fp, dim_desc *dimension,
				     flag comments) );
EXTERN_FUNCTION (void dmp_multi_data, (FILE *fp,
				       multi_array *multi_desc,
				       flag comments) );
EXTERN_FUNCTION (void dmp_packet, (FILE *fp, packet_desc *pack_desc,
				   char *packet, flag comments) );
EXTERN_FUNCTION (void dmp_element, (FILE *fp, unsigned int type, char *desc,
				    char *element, flag comments) );
EXTERN_FUNCTION (void dmp_array, (FILE *fp, array_desc *arr_desc,
				  char *array, flag comments) );
EXTERN_FUNCTION (void dmp_list, (FILE *fp, packet_desc *pack_desc,
				 list_header *list_head,
				 flag comments) );
EXTERN_FUNCTION (void dmp_flag, (FILE *fp, flag logical, char comment_string[],
				 flag comments) );


#endif /*  KARMA_DMP_H  */

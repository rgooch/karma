/*  karma_dsra.h

    Header for  dsra_  package.

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
  needed to interface to the dsra_ routines in the Karma library.


    Written by      Richard Gooch   24-MAR-1993

    Last updated by Richard Gooch   7-APR-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#if !defined(KARMA_DS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ds_def.h>
#endif

#if !defined(KARMA_CH_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ch_def.h>
#endif

#ifndef KARMA_DSRA_H
#define KARMA_DSRA_H


/*  File:   dsra.c   */
EXTERN_FUNCTION (multi_array *dsra_multi_desc, (Channel channel) );
EXTERN_FUNCTION (packet_desc *dsra_packet_desc, (Channel channel) );
EXTERN_FUNCTION (flag dsra_element_desc, (Channel channel, unsigned int *type,
					  char name[]) );
EXTERN_FUNCTION (array_desc *dsra_array_desc, (Channel channel,
					       unsigned int type) );
EXTERN_FUNCTION (dim_desc *dsra_dim_desc, (Channel channel) );
EXTERN_FUNCTION (flag dsra_multi_data, (Channel channel,
					multi_array *multi_desc) );
EXTERN_FUNCTION (flag dsra_packet, (Channel channel, packet_desc *descriptor,
				    char *packet) );
EXTERN_FUNCTION (flag dsra_element, (Channel channel, unsigned int type,
				     char *desc, char *element) );
EXTERN_FUNCTION (flag dsra_array, (Channel channel, array_desc *descriptor,
				   char *array) );
EXTERN_FUNCTION (flag dsra_list, (Channel channel, packet_desc *descriptor,
				  list_header *header) );
EXTERN_FUNCTION (flag dsra_flag, (Channel channel, flag *logical) );
EXTERN_FUNCTION (flag dsra_type, (Channel channel, unsigned int *type) );
EXTERN_FUNCTION (flag dsra_uint, (Channel channel, unsigned int *value) );
EXTERN_FUNCTION (flag dsra_int, (Channel channel, int *value) );
EXTERN_FUNCTION (flag dsra_float, (Channel channel, float *value) );
EXTERN_FUNCTION (flag dsra_double, (Channel channel, double *value) );


#endif /*  KARMA_DSRA_H  */

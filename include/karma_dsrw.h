/*  karma_dsrw.h

    Header for  dsrw_  package.

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
  needed to interface to the dsrw_ routines in the Karma library.


    Written by      Richard Gooch   14-SEP-1992

    Last updated by Richard Gooch   28-MAY-1993

*/

#ifndef KARMA_DSRW_H
#define KARMA_DSRW_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#include <karma_ds_def.h>
#include <karma_ch.h>


/*  File:   dsrw.c   */
EXTERN_FUNCTION (void dsrw_write_multi_desc,
		 (Channel channel, multi_array *multi_desc) );
EXTERN_FUNCTION (void dsrw_write_packet_desc,
		 (Channel channel, packet_desc *pack_desc) );
EXTERN_FUNCTION (void dsrw_write_element_desc, (Channel channel,
						unsigned int type,
						char *desc) );
EXTERN_FUNCTION (void dsrw_write_array_desc, (Channel channel,
					      array_desc *arr_desc) );
EXTERN_FUNCTION (void dsrw_write_dim_desc, (Channel channel,
					    dim_desc *dimension) );
EXTERN_FUNCTION (void dsrw_write_multi_data,
		 (Channel channel, multi_array *multi_desc) );
EXTERN_FUNCTION (void dsrw_write_packet, (Channel channel,
					  packet_desc *pack_desc,
					  char *packet) );
EXTERN_FUNCTION (void dsrw_write_element, (Channel channel, unsigned int type,
					   char *desc, char *element) );
EXTERN_FUNCTION (void dsrw_write_array, (Channel channel,
					 array_desc *arr_desc,
					 char *element, flag pad) );
EXTERN_FUNCTION (void dsrw_write_list, (Channel channel,
					packet_desc *pack_desc,
					list_header *list_header) );
EXTERN_FUNCTION (void dsrw_write_flag, (Channel channel, flag logical) );
EXTERN_FUNCTION (multi_array *dsrw_read_multi, (Channel channel) );
EXTERN_FUNCTION (packet_desc *dsrw_read_packet_desc, (Channel channel) );
EXTERN_FUNCTION (array_desc *dsrw_read_array_desc, (Channel channel,
						    unsigned int type) );
EXTERN_FUNCTION (dim_desc *dsrw_read_dim_desc, (Channel channel) );
EXTERN_FUNCTION (flag dsrw_read_packet, (Channel channel,
					 packet_desc *descriptor,
					 char *packet) );
EXTERN_FUNCTION (flag dsrw_read_element, (Channel channel, unsigned int type,
					  char *desc, char *element) );
EXTERN_FUNCTION (flag dsrw_read_array, (Channel channel,
					array_desc *descriptor,
					char *element, flag pad) );
EXTERN_FUNCTION (flag dsrw_read_list, (Channel channel,
				       packet_desc *descriptor,
				       list_header *header) );
EXTERN_FUNCTION (flag dsrw_read_flag, (Channel channel, flag *logical) );
EXTERN_FUNCTION (flag dsrw_read_type, (Channel channel, unsigned int *type) );


#endif /*  KARMA_DSRW_H  */

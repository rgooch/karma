/*  karma_dsrw.h

    Header for  dsrw_  package.

    Copyright (C) 1992-1996  Richard Gooch

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

    Last updated by Richard Gooch   16-AUG-1996

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

#ifndef KARMA_DSRW_H
#define KARMA_DSRW_H


/*  File:   dsrw.c   */
EXTERN_FUNCTION (flag dsrw_write_multi,
		 (Channel channel, CONST multi_array *multi_desc) );
EXTERN_FUNCTION (flag dsrw_write_multi_header,
		 (Channel channel, CONST multi_array *multi_desc) );
EXTERN_FUNCTION (flag dsrw_write_packet_desc,
		 (Channel channel, CONST packet_desc *pack_desc) );
EXTERN_FUNCTION (flag dsrw_write_element_desc,
		 (Channel channel, unsigned int type, CONST char *desc) );
EXTERN_FUNCTION (flag dsrw_write_array_desc,
		 (Channel channel, CONST array_desc *arr_desc) );
EXTERN_FUNCTION (flag dsrw_write_dim_desc,
		 (Channel channel, CONST dim_desc *dimension) );
EXTERN_FUNCTION (flag dsrw_write_packet,
		 (Channel channel, CONST packet_desc *pack_desc,
		  CONST char *packet) );
EXTERN_FUNCTION (flag dsrw_write_element,
		 (Channel channel, unsigned int type, CONST char *desc,
		  CONST char *element) );
EXTERN_FUNCTION (flag dsrw_write_array,
		 (Channel channel, CONST array_desc *arr_desc,
		  CONST char *element, flag pad) );
EXTERN_FUNCTION (flag dsrw_write_list,
		 (Channel channel, CONST packet_desc *pack_desc,
		  CONST list_header *list_head) );
EXTERN_FUNCTION (flag dsrw_write_packets,
		 (Channel channel, CONST packet_desc *descriptor,
		  CONST char *source, unsigned long num_packets) );
EXTERN_FUNCTION (flag dsrw_write_flag, (Channel channel, flag logical) );
EXTERN_FUNCTION (multi_array *dsrw_read_multi, (Channel channel) );
EXTERN_FUNCTION (multi_array *dsrw_read_multi_header, (Channel channel) );
EXTERN_FUNCTION (packet_desc *dsrw_read_packet_desc, (Channel channel) );
EXTERN_FUNCTION (array_desc *dsrw_read_array_desc, (Channel channel,
						    unsigned int type) );
EXTERN_FUNCTION (dim_desc *dsrw_read_dim_desc, (Channel channel) );
EXTERN_FUNCTION (flag dsrw_read_packet,
		 (Channel channel, CONST packet_desc *descriptor,
		  char *packet) );
EXTERN_FUNCTION (flag dsrw_read_element, (Channel channel, unsigned int type,
					  char *desc, char *element) );
EXTERN_FUNCTION (flag dsrw_read_array,
		 (Channel channel, CONST array_desc *descriptor,
		  char *element, flag pad) );
EXTERN_FUNCTION (flag dsrw_read_list,
		 (Channel channel, CONST packet_desc *descriptor,
		  list_header *header) );
EXTERN_FUNCTION (flag dsrw_read_packets,
		 (Channel channel, CONST packet_desc *descriptor,
		  char *dest, unsigned long num_packets) );
EXTERN_FUNCTION (flag dsrw_read_flag, (Channel channel, flag *logical) );
EXTERN_FUNCTION (flag dsrw_read_type, (Channel channel, unsigned int *type) );


#endif /*  KARMA_DSRW_H  */

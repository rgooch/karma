/*  karma_ch.h

    Header for  ch_  package.

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
  needed to interface to the ch_ routines in the Karma library.


    Written by      Richard Gooch   12-SEP-1992

    Last updated by Richard Gooch   27-SEP-1995

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#if !defined(KARMA_CH_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ch_def.h>
#endif

#if !defined(KARMA_C_DEF_H) || defined(MAKEDEPEND)
#  include <karma_c_def.h>
#endif

#ifndef KARMA_CH_H
#define KARMA_CH_H


/*  File:  channel.c  */
EXTERN_FUNCTION (Channel ch_open_file, (CONST char *filename,
					CONST char *type) );
EXTERN_FUNCTION (Channel ch_map_disc, (CONST char *filename,
				       unsigned int option,
				       flag writeable, flag update_on_write) );
EXTERN_FUNCTION (Channel ch_open_connection, (unsigned long host_addr,
					      unsigned int port_number) );
EXTERN_FUNCTION (Channel ch_open_memory, (char *buffer, unsigned int size) );
EXTERN_FUNCTION (Channel ch_accept_on_dock, (Channel dock,
					     unsigned long *addr) );
EXTERN_FUNCTION (Channel *ch_alloc_port, (unsigned int *port_number,
					  unsigned int retries,
					  unsigned int *num_docks) );
EXTERN_FUNCTION (flag ch_close, (Channel channel) );
EXTERN_FUNCTION (flag ch_flush, (Channel channel) );
EXTERN_FUNCTION (unsigned int ch_read, (Channel channel, char *buffer,
					unsigned int length) );
EXTERN_FUNCTION (unsigned int ch_write, (Channel channel, CONST char *buffer,
					 unsigned int length) );
EXTERN_FUNCTION (void ch_close_all_channels, () );
EXTERN_FUNCTION (flag ch_seek, (Channel channel, unsigned long position) );
EXTERN_FUNCTION (int ch_get_bytes_readable, (Channel channel) );
EXTERN_FUNCTION (int ch_get_descriptor, (Channel channel) );
EXTERN_FUNCTION (void ch_open_stdin, () );
EXTERN_FUNCTION (flag ch_test_for_io, (Channel channel) );
EXTERN_FUNCTION (flag ch_test_for_asynchronous, (Channel channel) );
EXTERN_FUNCTION (flag ch_test_for_connection, (Channel channel) );
EXTERN_FUNCTION (flag ch_test_for_local_connection, (Channel channel) );
EXTERN_FUNCTION (Channel ch_attach_to_asynchronous_descriptor, (int fd) );
EXTERN_FUNCTION (flag ch_test_for_mmap, (Channel channel) );
EXTERN_FUNCTION (flag ch_tell, (Channel channel, unsigned long *read_pos,
				unsigned long *write_pos) );
EXTERN_FUNCTION (char *ch_get_mmap_addr, (Channel channel) );
EXTERN_FUNCTION (unsigned int ch_get_mmap_access_count, (Channel channel) );
EXTERN_FUNCTION (ChConverter ch_register_converter,
		 (Channel channel, unsigned int (*size_func) (),
		  unsigned int (*read_func) (), unsigned int (*write_func) (),
		  flag (*flush_func) (), void (*close_func) (), void *info) );
EXTERN_FUNCTION (void ch_unregister_converter, (ChConverter converter) );
EXTERN_FUNCTION (flag ch_create_pipe, (Channel *read_ch, Channel *write_ch) );
EXTERN_FUNCTION (Channel ch_create_sink, () );
EXTERN_FUNCTION (KCallbackFunc ch_tap_io_events,
		 ( void (*tap_func) (), void *info ) );


/*  File:  ch_misc.c  */
EXTERN_FUNCTION (Channel ch_open_and_fill_memory, (char **strings) );
EXTERN_FUNCTION (flag ch_gets, (Channel channel, char *buffer,
				unsigned int length) );
EXTERN_FUNCTION (flag ch_getl, (Channel channel, char *buffer,
				unsigned int length) );
EXTERN_FUNCTION (flag ch_puts, (Channel channel, CONST char *string,
				flag newline) );
EXTERN_FUNCTION (unsigned int ch_drain,
		 (Channel channel, unsigned int length) );
EXTERN_FUNCTION (flag ch_printf, (Channel channel, CONST char *format, ...) );


/*  File:  ch_globals.h  */
extern Channel ch_stdin;
extern Channel ch_stdout;
extern Channel ch_stderr;
extern unsigned int ch_mmap_control;


#endif /*  KARMA_CH_H  */

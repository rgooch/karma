/*  karma_r.h

    Header for  r_  package.

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
  needed to interface to the r_ routines in the Karma library.


    Written by      Richard Gooch   12-SEP-1992

    Last updated by Richard Gooch   13-APR-1996

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_R_H
#define KARMA_R_H


#define CONN_MAX_INSTANCES 10
#define KFTYPE_DISC (unsigned int) 0
#define KFTYPE_CHARACTER (unsigned int) 1
#define KFTYPE_FIFO (unsigned int) 2
#define KFTYPE_UNIX_CONNECTION (unsigned int) 3
#define KFTYPE_LOCAL_tcpIP_CONNECTION (unsigned int) 4
#define KFTYPE_REMOTE_tcpIP_CONNECTION (unsigned int) 5


/*  For the file: connections.c  */
EXTERN_FUNCTION (int *r_alloc_port, (unsigned int *port_number,
				     unsigned int retries,
				     unsigned int *num_docks) );
EXTERN_FUNCTION (void r_close_dock, (int dock) );
EXTERN_FUNCTION (int r_connect_to_port, (unsigned long addr,
					 unsigned int port_number,
					 flag *local) );
EXTERN_FUNCTION (int r_accept_connection_on_dock, (int dock,
						   unsigned long *addr,
						   flag *local) );
EXTERN_FUNCTION (flag r_close_connection, (int connection) );
EXTERN_FUNCTION (int r_get_bytes_readable, (int connection) );
EXTERN_FUNCTION (unsigned long r_get_inet_addr_from_host,
		 (CONST char *host, flag *local) );
EXTERN_FUNCTION (int r_read, (int fd, char *buf, int nbytes) );
EXTERN_FUNCTION (int r_write, (int fd, CONST char *buf, int nbytes) );
EXTERN_FUNCTION (flag r_test_input_event, (int connection) );
EXTERN_FUNCTION (int r_open_stdin, (flag *disc) );
EXTERN_FUNCTION (char *r_getenv, (CONST char *name) );
EXTERN_FUNCTION (int r_setenv, (CONST char *env_name, CONST char *env_value) );
EXTERN_FUNCTION (void r_gethostname, (char *name, unsigned int namelen) );
EXTERN_FUNCTION (flag r_get_fq_hostname, (char *name, unsigned int namelen) );
EXTERN_FUNCTION (int r_getppid, () );
EXTERN_FUNCTION (int r_open_file, (CONST char *filename, int flags, int mode,
				   unsigned int *type,
				   unsigned int *blocksize) );
EXTERN_FUNCTION (int r_create_pipe, (int *read_fd, int *write_fd) );



/*  For the file: port_number.c  */
EXTERN_FUNCTION (char *r_get_karmabase, () );
EXTERN_FUNCTION (int r_get_service_number, (CONST char *module_name) );
EXTERN_FUNCTION (char *r_get_host_from_display, (CONST char *display) );
EXTERN_FUNCTION (int r_get_display_num_from_display, (CONST char *display) );
EXTERN_FUNCTION (int r_get_screen_num_from_display, (CONST char *display) );
EXTERN_FUNCTION (int r_get_def_port,
		 (CONST char *module_name, CONST char *display) );


#endif /*  KARMA_R_H  */

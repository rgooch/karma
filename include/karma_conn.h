/*  karma_conn.h

    Header for  conn_  package.

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
  needed to interface to the conn_ routines in the Karma library.


    Written by      Richard Gooch   18-SEP-1992

    Last updated by Richard Gooch   26-SEP-1993

*/

#ifndef KARMA_CONN_H
#define KARMA_CONN_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#include <karma_ch.h>

#ifndef CONNECTION_DEFINED
#define CONNECTION_DEFINED
typedef void * Connection;
#endif

/*  File:   connections.c   */
EXTERN_FUNCTION (void conn_register_managers,
		 (flag (*manage_func) (), void (*unmanage_func) (),
		  void (*exit_schedule_func) () ) );
EXTERN_FUNCTION (void conn_register_server_protocol,
		 (char *protocol_name, unsigned int version,
		  unsigned int max_connections,
		  flag (*open_func) (), flag (*read_func) (),
		  void (*close_func) () ) );
EXTERN_FUNCTION (void conn_register_client_protocol,
		 (char *protocol_name, unsigned int version,
		  unsigned int max_connections,
		  flag (*validate_func) (), flag (*open_func) (),
		  flag (*read_func) (), void (*close_func) () ) );
EXTERN_FUNCTION (Channel conn_get_channel, (Connection connection) );
EXTERN_FUNCTION (flag conn_attempt_connection, (char *hostname,
						unsigned int port_number,
						char *protocol_name) );
EXTERN_FUNCTION (flag conn_close, (Connection connection) );
EXTERN_FUNCTION (flag conn_become_server, (unsigned int *port_number,
					   unsigned int retries) );
EXTERN_FUNCTION (unsigned int conn_get_num_serv_connections,
		 (char *protocol_name) );
EXTERN_FUNCTION (unsigned int conn_get_num_client_connections,
		 (char *protocol_name) );
EXTERN_FUNCTION (Connection conn_get_serv_connection, (char *protocol_name,
						       unsigned int number) );
EXTERN_FUNCTION (Connection conn_get_client_connection,
		 (char *protocol_name, unsigned int number) );
EXTERN_FUNCTION (void *conn_get_connection_info, (Connection connection) );
EXTERN_FUNCTION (flag conn_controlled_by_cm_tool, () );
EXTERN_FUNCTION (char *conn_get_connection_module_name,
		 (Connection connection) );
EXTERN_FUNCTION (void conn_register_cm_quiescent_func, (void (*func) () ) );


#endif /*  KARMA_CONN_H  */

/*
    Codes sent from the Connection Management tool to the conn_ library

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

/*  Codes sent from the Connection Management tool or shell to the module  */
#define CM_TOOL_ATTEMPT_CONNECTION  (unsigned long) 0
#define CM_TOOL_CLOSE_CONNECTION    (unsigned long) 1
#define CM_TOOL_EXIT_MODULE         (unsigned long) 2
#define CM_TOOL_NOW_QUIESCENT       (unsigned long) 3

/*  Codes sent from the conn_ library to the Connection Management tool  */
#define CM_LIB_PORT_NUMBER        (unsigned long) 1000
#define CM_LIB_NEW_CONNECTION     (unsigned long) 1001
#define CM_LIB_CONNECTION_CLOSED  (unsigned long) 1002


/*  Information sent to Connection Management tool by  conn_  library  */

/*  Info sent when conn_ library connects (attempt_connection_to_cm)
PROTOCOL: conn_mngr_control
 *  module name
 *  hostname
 *  x position
 *  y position
 *  PID
PROTOCOL: conn_mngr_stdio
 *  hostname
 *  PID

*/

/*  Info sent when becoming server (conn_become_server)

 *  CM_LIB_PORT_NUMBER
 *  port number

*/

/*  Info sent when client connection succeeds

 *  CM_LIB_NEW_CONNECTION
 *  protocol name
 *  host Internet address
 *  port number
 *  Connection ID

*/

/*  Info sent when client connection closes

 *  CM_LIB_CONNECTION_CLOSED
 *  Connection ID

*/


/*  Information sent to  conn_  library by Connection Management tool  */

/*  Info sent when connection is to be attempted

 *  CM_TOOL_ATTEMPT_CONNECTION
 *  hostname
 *  port number
 *  protocol name

*/

/*  Info sent when connection is to be closed

 *  CM_TOOL_CLOSE_CONNECTION
 *  Connection ID

*/

/*  Info sent when module should exit cleanly

 *  CM_TOOL_EXIT_MODULE

*/

/*  Info sent when module should exit immediately

 *  CM_TOOL_KILL_MODULE

*/


/*  Information sent to the Connection Management tool by the slave on
    startup

 *  hostname

*/


/*  Information sent to the slave by the Connection Management tool whenever
    a new module is to be started

 *  module name
 *  x position
 *  y position

*/

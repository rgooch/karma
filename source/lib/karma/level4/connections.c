/*LINTLIBRARY*/
/*PREFIX:"conn_"*/
/*  connections.c

    This code provides high level connection control/ management routines.

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

    This file contains the various utility routines for managing connections.


    Written by      Richard Gooch   14-SEP-1992

    Updated by      Richard Gooch   27-OCT-1992

    Updated by      Richard Gooch   29-DEC-1992: Added connection management
  tool support and complete read processing.

    Updated by      Richard Gooch   1-JAN-1993: Took account of change to
  ch_gets  .

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   7-MAR-1993: Added revision number
  transmission.

    Updated by      Richard Gooch   26-MAR-1993: Made use of new routine:
  ch_open_and_fill_memory  .

    Updated by      Richard Gooch   4-APR-1993: Added version number parameter
  to protocol registration routines.

    Updated by      Richard Gooch   24-JUL-1993: Changed to  ch_open_file  .

    Updated by      Richard Gooch   3-AUG-1993: Fixed bug in
  conn_register_client_protocol which did not limit the maximum number of
  connections to that in the services file.

    Updated by      Richard Gooch   25-AUG-1993: Improved documentation for
  conn_get_serv_connection  and  conn_get_client_connection  .

    Updated by      Richard Gooch   27-AUG-1993: Added draining of input data
  in  serv_connection_input_func  and  conn_attempt_connection  .

    Updated by      Richard Gooch   30-AUG-1993: Fixed introduced bug in
  serv_connection_input_func  which froze server until client sent new data.

    Updated by      Richard Gooch   1-SEP-1993: Added test to ensure read
  callbacks are draining channel data.

    Updated by      Richard Gooch   5-SEP-1993: Removed need for protocols file

    Updated by      Richard Gooch   14-SEP-1993: Made diagnostic for input data
  not being read more helpful.

    Updated by      Richard Gooch   26-SEP-1993: Created
  conn_register_cm_quiescent_func  function.

    Last updated by Richard Gooch   27-SEP-1993: Improved error message in
  cm_command_func  .


*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <strings.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#ifdef HAS_SOCKETS
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#include <karma.h>
#include <karma_pio.h>
#include <karma_st.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_r.h>
#include <k_cm_codes.h>
#include <os.h>

typedef struct connection_type * Connection;

#define CONNECTION_DEFINED
#include <karma_conn.h>


#if defined(HAS_SOCKETS) || defined(HAS_COMMUNICATIONS_EMULATION)
#define COMMUNICATIONS_AVAILABLE
#endif

#define AUTH_PASSWORD_LENGTH 128
#define CONN_MAGIC_NUMBER (unsigned long) 245987237
#define REVISION_NUMBER (unsigned long) 2
#define SETUP_MESSAGE_LENGTH (AUTH_PASSWORD_LENGTH + PROTOCOL_NAME_LENGTH + 4 + 4 + 4)
#define OBJECT_MAGIC_NUMBER (unsigned int) 1794359023
#define PROTOCOL_NAME_LENGTH 80

/*  Raw protocol
    Stage 1: Client connects to server and sends:
      Magic number
      Revision number
      Protocol name [PROTOCOL_NAME_LENGTH]
      Protocol version number
      Password [AUTH_PASSWORD_LENGTH]
      Module name
    Stage 2: Server replies:
      Verification string (NULL for success)
      Module name (on success only)
    Stage 3: Server reads module name
*/

/*  Hard-coded protocol information  */
static char *protocol_lines[] =
{
    "# Protocol name		Number	Max Incoming	Max Outgoing",
    "server_exit		3	1		1",
    "ping_server		9	0		1",
    "experimental		10	0		0",
    NULL
};

/*  Generic connection structure (server and client)  */
struct connection_type
{
    unsigned int magic_number;
    flag client;
    Channel channel;
    char *protocol_name;             /*  Pointer copy: do not free  */
    unsigned int *connection_count;
    flag (*read_func) ();
    void (*close_func) ();
    void *info;
    char *module_name;
    unsigned long inet_addr;
    Connection prev;
    Connection next;
    Connection *list_start;
};

/*  Server protocol structures  */
struct serv_protocol_list_type
{
    char *protocol_name;  /*  This is just a pointer copy  */
    unsigned int version;
    unsigned int connection_count;
    unsigned int max_connections;
    flag (*open_func) ();
    flag (*read_func) ();
    void (*close_func) ();
    struct serv_protocol_list_type *next;
    struct auth_host_list_type *authorised_hosts;
};

struct auth_host_list_type
{
    struct host_list_type *host_ptr;
    struct auth_host_list_type *prev;
    struct auth_host_list_type *next;
};

struct host_list_type
{
    unsigned long inet_addr;
    char *hostname;
    struct host_list_type *next;
};

/*  Client protocol structure  */
struct client_protocol_list_type
{
    char *protocol_name;  /*  This is just a pointer copy  */
    unsigned int version;
    unsigned int connection_count;
    unsigned int max_connections;
    flag (*validate_func) ();
    flag (*open_func) ();
    flag (*read_func) ();
    void (*close_func) ();
    struct client_protocol_list_type *next;
};


/*  Private data  */
/*  Connection information  */
static flag ran_become_server = FALSE;
static Connection serv_connections = NULL;
static Connection client_connections = NULL;
static struct serv_protocol_list_type *serv_protocol_list = NULL;
static struct client_protocol_list_type *client_protocol_list = NULL;
static struct host_list_type *host_list = NULL;

/*  Connection to Connection Management tool  */
static Channel cm_channel = NULL;

/*  Pointers to channel management code  */
static flag (*manage_channel) () = NULL;
static void (*unmanage_channel) () = NULL;

static void (*exit_schedule_function) () = NULL;

/*  Function to call when CM tool or shell is quiescent  */
static void (*quiescent_function) () = NULL;


/*  Local functions  */
static flag dock_input_func ();
static flag serv_connection_input_func ();
static flag client_connection_input_func ();
static void connection_close_func ();
static void dealloc_connection ();
static struct serv_protocol_list_type *verify_connection ();
static struct serv_protocol_list_type *get_serv_protocol_info ();
static struct client_protocol_list_type *get_client_protocol_info ();
static char *get_password_for_protocol ();
static flag check_host_access ();
static flag write_protocol ();
static flag respond_to_ping_server_from_client ();
static flag register_server_exit ();
static Connection get_numbered_connection ();
static flag cm_command_func ();
static void cm_close_func ();


/*  Private functions follow  */

static flag dock_input_func (dock, info)
/*   This routine is called when new input occurs on a dock channel (ie. there
     is a connection request on a dock).
     The channel object is given by  dock  .
     An arbitrary pointer may be written to the storage pointed to by  info  .
     The pointer written here will persist until the channel is unmanaged
     (or a subsequent callback routine changes it).
     The routine returns TRUE if the connection is to remain managed,
     else it returns FALSE (indicating that the connection is to be
     unmanaged).
*/
Channel dock;
void **info;
{
    Connection new_connection;
    Connection last_entry;
    extern Connection serv_connections;
    extern flag (*manage_channel) ();
    static char function_name[] = "dock_input_func";

    /*  Allocate new connection object  */
    if ( ( new_connection = (Connection) m_alloc (sizeof *new_connection) )
	== NULL )
    {
	m_error_notify (function_name, "connection object");
	/*  Do not close dock  */
	return (TRUE);
    }
    if ( ( (*new_connection).channel =
	  ch_accept_on_dock (dock, &(*new_connection).inet_addr) )
	== NULL )
    {
	a_func_abort (function_name, "could not accept connection on dock");
	return (FALSE);
    }
    if ( (*manage_channel) ( (*new_connection).channel,
			    (void *) new_connection,
			    serv_connection_input_func,
			    connection_close_func,
			    ( flag (*) () ) NULL,
			    ( flag (*) () ) NULL ) != TRUE )
    {
	(void) ch_close ( (*new_connection).channel );
	m_free ( (char *) new_connection );
	a_func_abort (function_name, "could not manage channel");
	return (FALSE);
    }
    (*new_connection).magic_number = OBJECT_MAGIC_NUMBER;
    (*new_connection).client = FALSE;
    (*new_connection).connection_count = NULL;
    (*new_connection).read_func = NULL;
    (*new_connection).close_func = NULL;
    (*new_connection).info = NULL;
    (*new_connection).prev = NULL;
    (*new_connection).next = NULL;
    (*new_connection).list_start = &serv_connections;
    /*  Add new connection to end of list  */
    if (serv_connections == NULL)
    {
	/*  No list yet  */
	serv_connections = new_connection;
	return (TRUE);
    }
    /*  Search for end of list  */
    for (last_entry = serv_connections; (*last_entry).next != NULL;
	 last_entry = (*last_entry).next);
    (*last_entry).next = new_connection;
    (*new_connection).prev = last_entry;
    return (TRUE);
}   /*  End Function dock_input_func  */

static flag serv_connection_input_func (channel, info)
/*  This routine is called when new input occurs on a channel.
    The channel object is given by  channel  .
    An arbitrary pointer may be written to the storage pointed to by  info
    The pointer written here will persist until the channel is unmanaged
    (or a subsequent callback routine changes it).
    The routine returns TRUE if the channel is to remain managed and
    open, else it returns FALSE (indicating that the channel is to be
    unmanaged and closed).
*/
Channel channel;
void **info;
{
    int bytes;
    unsigned long old_read_pos, new_read_pos, dummy;
    Connection connection;
    struct serv_protocol_list_type *protocol_info;
    static char function_name[] = "serv_connection_input_func";

    connection = (Connection) *info;
    if ( (*connection).magic_number != OBJECT_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid connection object\n");
	a_prog_bug (function_name);
    }
    if ( (*connection).connection_count == NULL )
    {
	/*  No protocol yet: connection setup information needed  */
	/*  Check if enough data on connection  */
	if ( ( bytes = ch_get_bytes_readable (channel) ) < 0 )
	{
	    (void) exit (RV_SYS_ERROR);
	}
	if (bytes < SETUP_MESSAGE_LENGTH)
	{
	    /*  Not enough info was sent  */
	    (void) pio_write_string (channel, "Bad setup message length");
	    (void) fprintf (stderr,
			    "Only: %d bytes of connection setup information sent\n",
			    bytes);
	    (void) fprintf (stderr,
			    "%d bytes are required: connection closed\n",
			    SETUP_MESSAGE_LENGTH);
	    return (FALSE);
	}
	if ( ( protocol_info = verify_connection (connection) ) == NULL )
	{
	    /*  Connection was not authorised  */
	    (void) ch_flush (channel);
	    return (FALSE);
	}
	/*  Read module name from client  */
	if ( ( (*connection).module_name =
	      pio_read_string (channel, (unsigned int *) NULL) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "Error reading module name from connection\n");
	    return (FALSE);
	}
	if (ch_flush (channel) != TRUE)
	{
	    return (FALSE);
	}
	(*connection).protocol_name = (*protocol_info).protocol_name;
	(*connection).connection_count = &(*protocol_info).connection_count;
	(*connection).read_func = (*protocol_info).read_func;
	++(*protocol_info).connection_count;
	if ( (*protocol_info).open_func != NULL )
	{
	    /*  Open function registered  */
	    if ( (* (*protocol_info).open_func ) (connection,
						  &(*connection).info)
		!= TRUE )
	    {
		/*  Failure  */
		return (FALSE);
	    }
	}
	(*connection).close_func = (*protocol_info).close_func;
    }
    if ( ( bytes = ch_get_bytes_readable (channel) ) < 0 )
    {
	(void) exit (RV_SYS_ERROR);
    }
    if ( (bytes > 0) && ( (*connection).read_func == NULL ) )
    {
	(void) fprintf (stderr,
			"Input on \"%s\" connection not being read (no callback)\n",
			(*connection).protocol_name);
	a_prog_bug (function_name);
    }
    if (ch_tell ( (*connection).channel, &old_read_pos, &dummy ) != TRUE )
    {
	(void) exit (RV_SYS_ERROR);
    }
    while (bytes > 0)
    {
	if ( (* (*connection).read_func ) (connection, &(*connection).info)
	    == FALSE )
	{
	    return (FALSE);
	}
	if (ch_tell ( (*connection).channel, &new_read_pos, &dummy ) != TRUE )
	{
	    (void) exit (RV_SYS_ERROR);
	}
	if (new_read_pos <= old_read_pos)
	{
	    (void) fprintf (stderr,
			    "Connection read callback for protocol: \"%s\" not draining\n",
			    (*connection).protocol_name);
	    a_prog_bug (function_name);
	}
	old_read_pos = new_read_pos;
	if ( ( bytes = ch_get_bytes_readable (channel) ) < 0 )
	{
	    (void) exit (RV_SYS_ERROR);
	}
    }
    return (TRUE);
}   /*  End Function serv_connection_input_func  */

static flag client_connection_input_func (channel, info)
/*  This routine is called when new input occurs on a channel.
    The channel object is given by  channel  .
    An arbitrary pointer may be written to the storage pointed to by  info
    The pointer written here will persist until the channel is unmanaged
    (or a subsequent callback routine changes it).
    The routine returns TRUE if the channel is to remain managed and
    open, else it returns FALSE (indicating that the channel is to be
    unmanaged and closed).
*/
Channel channel;
void **info;
{
    int bytes;
    unsigned long old_read_pos, new_read_pos, dummy;
    Connection connection;
    static char function_name[] = "client_connection_input_func";

    connection = (Connection) *info;
    if ( (*connection).magic_number != OBJECT_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid connection object\n");
	a_prog_bug (function_name);
    }
    if ( (*connection).read_func == NULL )
    {
	(void) fprintf (stderr,
			"Input on \"%s\" connection not being read (no callback)\n",
			(*connection).protocol_name);
	a_prog_bug (function_name);
    }
    bytes = 1;
    if (ch_tell ( (*connection).channel, &old_read_pos, &dummy ) != TRUE )
    {
	(void) exit (RV_SYS_ERROR);
    }
    while (bytes > 0)
    {
	if ( (* (*connection).read_func ) (connection, &(*connection).info)
	    == FALSE )
	{
	    return (FALSE);
	}
	if (ch_tell ( (*connection).channel, &new_read_pos, &dummy ) != TRUE )
	{
	    (void) exit (RV_SYS_ERROR);
	}
	if (new_read_pos <= old_read_pos)
	{
	    (void) fprintf (stderr,
			    "Connection read callback for protocol: \"%s\" not draining\n",
			    (*connection).protocol_name);
	    a_prog_bug (function_name);
	}
	old_read_pos = new_read_pos;
	if ( ( bytes = ch_get_bytes_readable (channel) ) < 0 )
	{
	    (void) exit (RV_SYS_ERROR);
	}
    }
    return (TRUE);
}   /*  End Function client_connection_input_func  */

static struct serv_protocol_list_type *verify_connection (connection)
/*  This routine will verify whether a connection is permitted or not.
    The connection object must be given by  connection  .
    The routine returns a pointer to the protocol information if the
    connection is authorised, else it returns NULL.
*/
Connection connection;
{
    unsigned long protocol_version;
    unsigned long magic_number;
    unsigned long revision_number;
    Channel channel;
    char *required_auth_password;
    struct serv_protocol_list_type *protocol_info;
    char password_buffer[AUTH_PASSWORD_LENGTH];
    char protocol_buffer[PROTOCOL_NAME_LENGTH + 1];
    extern char module_name[STRING_LENGTH + 1];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "verify_connection";

    channel = (*connection).channel;
    /*  Read connection setup information  */
    if (pio_read32 (channel, &magic_number) != TRUE)
    {
	a_func_abort (function_name,
		      "Error reading magic number from connection");
	return (NULL);
    }
    /*  Check magic number  */
    if (magic_number != CONN_MAGIC_NUMBER)
    {
	/*  Bad magic number  */
	(void) pio_write_string (channel, "Bad magic number");
	(void) fprintf (stderr,
			"WARNING: Connection attempted with bad magic number: %lu\n",
			magic_number);
	(void) fprintf (stderr, "Someone may be trying to stuff around\n");
	return (NULL);
    }
    /*  Get revision number  */
    if (pio_read32 (channel, &revision_number) != TRUE)
    {
	a_func_abort (function_name,
		      "Error reading revision number from connection");
	return (NULL);
    }
    /*  Check revision number  */
    if (revision_number != REVISION_NUMBER)
    {
	/*  Bad magic number  */
	(void) pio_write_string (channel, "Bad revision number");
	return (NULL);
    }
    /*  Read protocol name  */
    if (ch_read (channel, protocol_buffer, PROTOCOL_NAME_LENGTH)
	< PROTOCOL_NAME_LENGTH)
    {
	(void) fprintf (stderr,
			"Error reading protocol name from connection\n");
	return (NULL);
    }
    protocol_buffer[PROTOCOL_NAME_LENGTH] = '\0';
    /*  Get protocol version number  */
    if (pio_read32 (channel, &protocol_version) != TRUE)
    {
	(void) fprintf (stderr,
			"Error reading protocol version number from connection\n");
	return (NULL);
    }
    /*  Read password information (even if not needed)  */
    if (ch_read (channel, password_buffer, AUTH_PASSWORD_LENGTH) < AUTH_PASSWORD_LENGTH)
    {
	(void) fprintf (stderr,
			"Error reading password from connection\n");
	return (NULL);
    }
    /*  Compare protocol number with list of supported ones  */
    if ( ( protocol_info = get_serv_protocol_info (protocol_buffer) ) == NULL )
    {
	/*  Protocol is not supported  */
	/*  Write failure message back to client  */
	(void) pio_write_string (channel,
				 "Protocol is not supported by server");
	return (NULL);
    }
    if (protocol_version != (*protocol_info).version)
    {
	(void) pio_write_string (channel, "Protocol version mismatch");
	return (NULL);
    }
    /*  Protocol is supported: check if more connections allowed  */
    if ( ( (*protocol_info).max_connections > 0 ) &&
	( (*protocol_info).connection_count
	 >= (*protocol_info).max_connections ) )
    {
	(void) pio_write_string (channel,
				 "Connection limit reached for protocol");
	return (NULL);
    }
    /*  Protocol is supported: check if authorised  */
    required_auth_password = get_password_for_protocol (protocol_buffer);
    if ( (required_auth_password == NULL) ||
	(*required_auth_password == '\0') )
    {
	/*  No password required  */
	/*  Write verification message back to client  */
	(void) pio_write_string (channel, (char *) NULL);
	/*  Send module name back to client  */
	(void) pio_write_string (channel, module_name);
	return (protocol_info);
    }
    /*  Password required  */
    if (strncmp (required_auth_password, password_buffer, AUTH_PASSWORD_LENGTH)
	== 0)
    {
	/*  Password is OK  */
	/*  Write verification message back to client  */
	(void) pio_write_string (channel, (char *) NULL);
	/*  Send module name back to client  */
	(void) pio_write_string (channel, module_name);
	return (protocol_info);
    }
    /*  Bad password: check if host is authorised  */
    if (check_host_access ( (*connection).inet_addr, protocol_buffer ) == TRUE)
    {
	/*  Host is permitted for this protocol  */
	/*  Write verification message back to client  */
	(void) pio_write_string (channel, (char *) NULL);
	/*  Send module name back to client  */
	(void) pio_write_string (channel, module_name);
	return (protocol_info);
    }
    /*  Client is not allowed  */
    (void) fprintf (stderr,
		    "WARNING: connection attempt failed for protocol: %s\n",
		     protocol_buffer);
    (void) pio_write_string (channel, "Connection not authorised");
    return (NULL);
}   /*  End Function verify_connection  */

static struct serv_protocol_list_type *get_serv_protocol_info (protocol_name)
/*  This routine will search the list of supported server protocols for the
    protocol given by  protocol_name  .
    The routine will return a pointer to a protocol information structure if
    the protocol is found in the list, else it returns NULL.
*/
char *protocol_name;
{
    struct serv_protocol_list_type *entry;
    extern struct serv_protocol_list_type *serv_protocol_list;

    entry = serv_protocol_list;
    while (entry != NULL)
    {
	if (strcmp (protocol_name, (*entry).protocol_name) == 0)
	{
	    /*  Found it!  */
	    return (entry);
	}
	entry = (*entry).next;
    }
    /*  Not found  */
    return (NULL);
}   /*  End Function get_serv_protocol_info  */

static struct client_protocol_list_type *get_client_protocol_info (protocol_name)
/*  This routine will search the list of supported client protocols for the
    protocol given by  protocol_name  .
    The routine will return a pointer to a protocol information structure if
    the protocol is found in the list, else it returns NULL.
*/
char *protocol_name;
{
    struct client_protocol_list_type *entry;
    extern struct client_protocol_list_type *client_protocol_list;

    entry = client_protocol_list;
    while (entry != NULL)
    {
	if (strcmp (protocol_name, (*entry).protocol_name) == 0)
	{
	    /*  Found it!  */
	    return (entry);
	}
	entry = (*entry).next;
    }
    /*  Not found  */
    return (NULL);
}   /*  End Function get_client_protocol_info  */

static void connection_close_func (channel, info)
/*  This routine is called when a channel closes.
    The channel object is given by  channel  .
    The arbitrary pointer for the channel will be pointed to by  info  .
    Any unread buffered data in the channel will be lost upon closure. The
    call to this function is the last chance to read this buffered data.
    The routine returns nothing.
*/
Channel channel;
void *info;
{
    Connection connection;
    static char function_name[] = "connection_close_func";

    /*  Extract and verify connection  */
    connection = (Connection) info;
    if ( (*connection).magic_number != OBJECT_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid connection object\n");
	a_prog_bug (function_name);
    }
    dealloc_connection (connection);
}   /*  End Function connection_close_func  */

static void dealloc_connection (connection)
/*  This routine will deallocate a connection object.
    The routine will NOT unmanage or close the channel associated with the
    connection.
    The close function registered for the connection will be called prior to
    deallocation.
    The connection object must be given by  connection  .
    The routine returns nothing.
*/
Connection connection;
{
    extern Channel cm_channel;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    /*  First decrement connection count and remove from list,
	otherwise the close_func callback could query for a connection and
	get this one!  */
    if ( (*connection).connection_count != NULL )
    {
	/*  Decrement count of connections for this protocol  */
	--*(*connection).connection_count;
    }
    /*  Remove connection from list  */
    if ( (*connection).prev == NULL )
    {
	/*  There is no previous connection: this is the first in the list  */
	*(*connection).list_start = (*connection).next;
    }
    else
    {
	/*  There is a previous connection  */
	(* (*connection).prev ).next = (*connection).next;
    }
    if ( (*connection).next != NULL )
    {
	/*  There is a next connection  */
	(* (*connection).next ).prev = (*connection).prev;
    }
    /*  Call registered closure routine if it exists  */
    if ( (*connection).close_func != NULL )
    {
	(* (*connection).close_func ) (connection, (*connection).info);
    }
    (*connection).magic_number = 0;
    if ( (*connection).module_name != NULL )
    {
	m_free ( (*connection).module_name );
    }
    if ( (*connection).client && (cm_channel != NULL) )
    {
	/*  Have client connection and connected to
	    Connection Management tool  */
	if (pio_write32 (cm_channel, CM_LIB_CONNECTION_CLOSED) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing command value\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if (pio_write32 (cm_channel, (unsigned long) connection) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing Connection ID\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if (ch_flush (cm_channel) != TRUE)
	{
	    (void) fprintf (stderr, "Error flushing channel\n");
	    exit (RV_WRITE_ERROR);
	}
    }
    m_free ( (char *) connection );
}   /*  End Function dealloc_connection  */

struct auth_password_list_type
{
    char *protocol_name;
    char *password;
    struct auth_password_list_type *next;
};

static char *get_password_for_protocol (protocol)
/*  This routine will determine the password required to gain access to a Karma
    server using the protocol given by  protocol  .
    The routine returns the password required.
*/
char *protocol;
{
#ifdef COMMUNICATIONS_AVAILABLE
    FILE *fp;
    char *home;
    struct auth_password_list_type *entry;
    struct auth_password_list_type *end_entry;
    char auth_file[STRING_LENGTH];
    char txt[STRING_LENGTH + 1];
    char protocol_name[STRING_LENGTH];
    char password[STRING_LENGTH];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static flag read_list = FALSE;
    static struct auth_password_list_type *password_list;
    static char function_name[] = "get_password_for_protocol";

    if (read_list != TRUE)
    {
	read_list = TRUE;
	/*  Read in password list  */
	if ( ( home = r_getenv ("HOME") ) == NULL )
	{
	    (void) fprintf (stderr,
			    "Error getting environment variable: HOME\n");
	    /*  Return saying no passwords needed  */
	    password_list = NULL;
	    return (NULL);
	}
	(void) sprintf (auth_file, "%s/.KARMAauthority", home);
	if (access (auth_file, R_OK) != 0)
	{
	    /*  File not there  */
	    password_list = NULL;
	    /*  No passwords required  */
	    return (NULL);
	}
	/*  Read file  */
	if ( ( fp = fopen (auth_file, "r") ) == NULL )
	{
	    (void) fprintf (stderr, "Error opening file: %s\n", auth_file);
	    (void) exit (RV_CANNOT_OPEN);
	}
	while (fgets (txt, STRING_LENGTH, fp) != NULL)
	{
	    /*  Not end of file  */
	    if ( (txt[0] == '#') || (txt[0] == '\n') )
	    {
		/*  Comment or blank line: skip  */
		continue;
	    }
	    password[0] = '\0';
	    switch ( sscanf (txt, " %s %s", protocol_name, password) )
	    {
	      case 1:
	      case 2:
		/*  Found a valid line  */
		/*  Add password to list of passwords  */
		if ( ( entry = (struct auth_password_list_type *)
		      m_alloc (sizeof *entry) ) == NULL )
		{
		    m_abort (function_name, "protocol password entry");
		}
		(*entry).next = NULL;
		if ( ( (*entry).protocol_name = st_dup (protocol_name) )
		    == NULL )
		{
		    m_abort (function_name, "protocol name");
		}
		if (password[0] != '\0')
		{
		    /*  Some password to be recorded  */
		    if ( ( (*entry).password =
			  m_alloc (strlen (password) + 1) ) == NULL )
		    {
			m_abort (function_name, "protocol password");
		    }
		    (void) strcpy ( (*entry).password, password );
		}
		else
		{
		    (*entry).password = NULL;
		}
		/*  Add entry to list  */
		if (password_list == NULL)
		{
		    /*  Create list  */
		    password_list = entry;
		    end_entry = entry;
		}
		else
		{
		    /*  Append to list  */
		    (*end_entry).next = entry;
		    end_entry = entry;
		}
		break;
	      default:
		/*  Error converting line  */
		(void) fclose (fp);
		(void) fprintf (stderr, "Error in file: %s\n", auth_file);
		(void) exit (RV_BAD_DATA);
		break;
	    }
	}
	/*  End of file  */
	(void) fclose (fp);
    }
    /*  Get password from list  */
    if (password_list == NULL)
    {
	/*  No list: no passwords required  */
	return (NULL);
    }
    for (entry = password_list; entry != NULL; entry = (*entry).next)
    {
	if (strcmp (protocol, (*entry).protocol_name) == 0)
	{
	    return ( (*entry).password );
	}
    }
    return (NULL);

#else  /*  COMMUNICATIONS_AVAILABLE  */
    (void) fprintf (stderr,
		    "Operating system does not support communications\n");
    (void) exit (RV_UNDEF_ERROR);
#endif  /*  COMMUNICATIONS_AVAILABLE  */
}   /*  End Function get_password_for_protocol  */

static flag check_host_access (inet_addr, protocol)
/*  This routine will check if the host with Internet address given by
    inet_addr  is authorised to connect to the server with protocol given by
    protocol  .
    The routine returns TRUE if the host is permitted access,
    else it returns FALSE.
*/
unsigned long inet_addr;
char *protocol;
{
    struct serv_protocol_list_type *protocol_info;
    struct auth_host_list_type *auth_entry;
    extern struct serv_protocol_list_type *serv_protocol_list;
    static char function_name[] = "check_host_access";

    if (serv_protocol_list == NULL)
    {
	/*  Protocol not supprted  */
	(void) fprintf (stderr, "No protocols supported by server\n");
	a_prog_bug (function_name);
    }
    /*  Search for protocol  */
    if ( ( protocol_info = get_serv_protocol_info (protocol) ) == NULL )
    {
	/*  Protocol not supported  */
	(void) fprintf (stderr, "Protocol: %u not supported by server\n",
			protocol);
	a_prog_bug (function_name);
    }
    /*  Search if host is authorised  */
    for (auth_entry = (*protocol_info).authorised_hosts; auth_entry != NULL;
	 auth_entry = (*auth_entry).next)
    {
	if (inet_addr == (* (*auth_entry).host_ptr ).inet_addr)
	{
	    /*  Found it  */
	    return (TRUE);
	}
    }
    /*  Host is not authorised  */
    return (FALSE);
}   /*  End Function check_host_access  */

static flag write_protocol (channel, protocol_name, version)
/*  This routine will write an authentication message for a specified protocol
    to a channel.
    The channel object must be given by  channel  .
    The protocol name must be pointed to by  protocol_name  .
    The version number for the protocol must be given by  version  .
    The routine returns TRUE on success, else it returns FALSE.
*/
Channel channel;
char *protocol_name;
unsigned int version;
{
    char password_buffer[AUTH_PASSWORD_LENGTH];
    char protocol_buffer[PROTOCOL_NAME_LENGTH];
    char *password;
    extern char module_name[STRING_LENGTH + 1];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "write_protocol";

    /*  Write magic number  */
    if (pio_write32 (channel, CONN_MAGIC_NUMBER) != TRUE)
    {
	a_func_abort (function_name, "Error writing magic number to channel");
	return (FALSE);
    }
    /*  Write revision number  */
    if (pio_write32 (channel, REVISION_NUMBER) != TRUE)
    {
	a_func_abort (function_name,
		      "Error writing revision number to channel");
	return (FALSE);
    }
    /*  Write protocol name  */
    (void) strncpy (protocol_buffer, protocol_name, PROTOCOL_NAME_LENGTH);
    if (ch_write (channel, protocol_buffer, PROTOCOL_NAME_LENGTH)
	< PROTOCOL_NAME_LENGTH)
    {
	(void) fprintf (stderr, "Error writing protocol name\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  Write protocol version number  */
    if (pio_write32 (channel, (unsigned long) version) != TRUE)
    {
	a_func_abort (function_name,
		      "Error writing protocol version number to channel");
	return (FALSE);
    }
    /*  Get password  */
    password = get_password_for_protocol (protocol_name);
    if (password == NULL)
    {
	password_buffer[0] = '\0';
    }
    else
    {
	(void) strncpy (password_buffer, password, AUTH_PASSWORD_LENGTH);
    }
    if (ch_write (channel, password_buffer, AUTH_PASSWORD_LENGTH)
	< AUTH_PASSWORD_LENGTH)
    {
	(void) fprintf (stderr,
			"Error writing connection setup information\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    /*  Send module name to server  */
    (void) pio_write_string (channel, module_name);
    return ( ch_flush (channel) );
}   /*  End Function write_protocol  */

static flag respond_to_ping_server_from_client (connection, info)
/*  This routine will process input from a client on a  ping_server  connection
    The connection object must be given by  connection  .
    Any appropriate information will be written to the pointer pointed to by
    info  (unused).
    The routine returns TRUE on success, else it returns FALSE (indicating that
    the connection should be closed).
*/
Connection connection;
void **info;
{
#ifdef COMMUNICATIONS_AVAILABLE
    int bytes_readable;
    Channel channel;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static unsigned int buffer_length = 0;
    static char *buffer = NULL;
    static char function_name[] = "respond_to_ping_server_from_client";

    if ( (*connection).magic_number != OBJECT_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid connection object\n");
	a_prog_bug (function_name);
    }
    channel = (*connection).channel;
    if ( ( bytes_readable = ch_get_bytes_readable (channel) )
	< 0 )
    {
	(void) exit (RV_SYS_ERROR);
    }
    if (bytes_readable < 1)
    {
	(void) fprintf (stderr,
			"Connection has: %d bytes readable: should be at least 1\n",
			bytes_readable);
	a_prog_bug (function_name);
    }
    /*  Check if buffer is big enough  */
    if (buffer_length < (unsigned int) bytes_readable)
    {
	/*  Allocate bigger buffer  */
	if (buffer != NULL)
	{
	    m_free (buffer);
	}
	buffer_length = 0;
	if ( ( buffer = m_alloc ( (unsigned int) bytes_readable ) ) == NULL )
	{
	    m_error_notify (function_name, "buffer");
	    return (FALSE);
	}
	buffer_length = (unsigned int) bytes_readable;
    }
    /*  Read data  */
    if (ch_read (channel, buffer, (unsigned int) bytes_readable)
	< (unsigned int) bytes_readable)
    {
	(void) fprintf (stderr,
			"Error reading: %u bytes from descriptor\t%s\n",
			(unsigned int) bytes_readable, sys_errlist[errno]);
	return (FALSE);
    }
    /*  Send data back from whence it came  */
    if (ch_write (channel, buffer, (unsigned int) bytes_readable) 
	< (unsigned int) bytes_readable)
    {
	(void) fprintf (stderr,
			"Error writing: %u bytes to channel\t%s\n",
			(unsigned int) bytes_readable, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);

#else  /*  COMMUNICATIONS_AVAILABLE  */
    (void) fprintf (stderr,
		    "Operating system does not support communications\n");
    return (FALSE);
#endif
}   /*  End Function respond_to_ping_server_from_client   */

static flag register_server_exit (connection, info)
/*  This routine will process input from a client on a  "server_exit"
    connection.
    The connection object must be given by  connection  .
    Any appropriate information will be written to the pointer pointed to by
    info  (unused).
    The routine either returns FALSE (indicating that the connection should be
    closed), or the routine will never exit ( because it calls exit(3) ).
*/
Connection connection;
void **info;
{
    extern void (*exit_schedule_function) ();

    if (exit_schedule_function == NULL)
    {
	exit (RV_OK);
    }
    (*exit_schedule_function) ();
    return (FALSE);
}   /*  End Function respond_to_ping_server_from_client   */

static Connection get_numbered_connection (protocol_name, number, list)
/*  This routine will get Nth connection from a connection list with a
    specified protocol.
    The protocol name must be pointed to by  protocol_name  .
    The number of the connection to get must be given by  number  .
    The connection list must be given by  list  .
    The routine returns the connection on success, else it returns NULL.
*/
char *protocol_name;
unsigned int number;
Connection list;
{
    unsigned int conn_count;
    Connection curr_entry;
    static char function_name[] = "get_numbered_connection";

    for (conn_count = 0, curr_entry = list; curr_entry != NULL;
	 curr_entry = (*curr_entry).next)
    {
	if (strcmp (protocol_name, (*curr_entry).protocol_name) == 0)
	{
	    /*  Found a connection of the correct protocol  */
	    if (conn_count == number)
	    {
		/*  Found it  */
		return (curr_entry);
	    }
	    ++conn_count;
	}
    }
    /*  Not found  */
    return (NULL);
}   /*  End Function get_numbered_connection  */  

static void attempt_connection_to_cm ()
/*  This routine will attempt to connect to the Connection Management tool.
    The routine returns nothing.
*/
{
    int x;
    int y;
    int out_fd;
    unsigned int port_number;
    unsigned int length;
    unsigned long host_addr;
    Channel stdio;
    char *control_command;
    char *char_ptr;
    char *message;
    char my_hostname[STRING_LENGTH];
    char txt[STRING_LENGTH];
    extern Channel cm_channel;
    extern flag (*manage_channel) ();
    extern void (*unmanage_channel) ();
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    extern char module_name[STRING_LENGTH + 1];
    static char function_name[] = "attempt_connection_to_cm";

    if (cm_channel != NULL)
    {
	(void) fprintf (stderr,
			"Connection to Connection Management tool already open\n");
	a_prog_bug (function_name);
    }
    if ( ( control_command = r_getenv ("KARMA_CM_CONTROL_COMMAND") ) == NULL )
    {
	/*  Ignore  */
	return;
    }
#ifdef HAS_SOCKETS
    (void) sprintf ( txt, "/tmp/%s.log.%d.%d",
		    module_name, getuid (), getpid () );
    if ( ( out_fd = open (txt, O_CREAT | O_TRUNC | O_WRONLY,
			  S_IRUSR | S_IWUSR) ) < 0 )
    {
	(void) fprintf (stderr,
			"Error opening: \"%s\" for output\t%s\n",
			txt, sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Attatch log file to standard output  */
    if (dup2 (out_fd, 1) < 0)
    {
	(void) fprintf (stderr,
			"Error redirecting standard output\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Attatch log file to standard error  */
    if (dup2 (out_fd, 2) < 0)
    {
	(void) fprintf (stderr,
			"Error redirecting standard error\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    /*  Close temporary descriptor for log file  */
    if (close (out_fd) != 0)
    {
	(void) fprintf (stderr,
			"Error closing temporary descriptor for log file\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
#endif
    r_gethostname (my_hostname, STRING_LENGTH);
    /*  Decode command  */
    for (char_ptr = control_command; *char_ptr != '\0'; ++char_ptr)
    {
	if (*char_ptr == ':')
	{
	    /*  Convert ':' to ' '  */
	    *char_ptr = ' ';
	}
    }
    char_ptr = control_command;
    host_addr = ex_int (char_ptr, &char_ptr);
    port_number = ex_int (char_ptr, &char_ptr);
    x = ex_int (char_ptr, &char_ptr);
    y = ex_int (char_ptr, &char_ptr);
    if (host_addr == 0) (void) strcpy (my_hostname, "localhost");
#ifdef ARCH_VXMVX
    /*  Append ":vx" to hostname  */
    (void) strcat (my_hostname, ":vx");
#endif
    /*  Connect to Connection Management tool with control protocol  */
    /*  Open connection  */
    if ( ( cm_channel = ch_open_connection (host_addr, port_number) ) == NULL )
    {
	(void) fprintf (stderr,
			"Could not open connection to Connection Management tool");
	exit (RV_UNDEF_ERROR);
    }
    /*  Send protocol info  */
    if (write_protocol (cm_channel, "conn_mngr_control", 0) != TRUE)
    {
	(void) fprintf (stderr,
			"Error writing authentication information\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (cm_channel);
	exit (RV_SYS_ERROR);
    }
    /*  Get returned message  */
    if ( ( message = pio_read_string (cm_channel, &length) ) == NULL )
    {
	(void) fprintf (stderr,
			"Error reading return message\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (cm_channel);
	exit (RV_SYS_ERROR);
    }
    if (length > 0)
    {
	(void) fprintf (stderr,
			"\"conn_mngr_control\" connection refused. Reason: %s\n",
			message);
	m_free (message);
	(void) ch_close (cm_channel);
	exit (RV_UNDEF_ERROR);
    }
    m_free (message);
    /*  Get returned module name  */
    if ( ( message = pio_read_string (cm_channel, (unsigned int *) NULL) )
	== NULL )
    {
	(void) fprintf (stderr,
			"Error reading server module name\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (cm_channel);
	exit (RV_UNDEF_ERROR);
    }
    m_free (message);
    if ( (*manage_channel) (cm_channel, (void *) NULL,
			    cm_command_func,
			    cm_close_func,
			    ( flag (*) () ) NULL,
			    ( flag (*) () ) NULL ) != TRUE )
    {
	(void) ch_close (cm_channel);
	(void) fprintf (stderr, "Could not manage channel");
	exit (RV_UNDEF_ERROR);
    }
    /*  Send information back to Connection Management tool  */
    if (pio_write_string (cm_channel, module_name) != TRUE)
    {
	(void) fprintf (stderr, "Error writing module name\t%s\n",
			sys_errlist[errno]);
	exit (RV_WRITE_ERROR);
    }
    if (pio_write_string (cm_channel, my_hostname) != TRUE)
    {
	(void) fprintf (stderr, "Error writing hostname\t%s\n",
			sys_errlist[errno]);
	exit (RV_WRITE_ERROR);
    }
    if (pio_write32s (cm_channel, x) != TRUE)
    {
	(void) fprintf (stderr, "Error writing x position\t%s\n",
			sys_errlist[errno]);
	exit (RV_WRITE_ERROR);
    }
    if (pio_write32s (cm_channel, y) != TRUE)
    {
	(void) fprintf (stderr, "Error writing y position\t%s\n",
			sys_errlist[errno]);
	exit (RV_WRITE_ERROR);
    }
    if (pio_write32s ( cm_channel, getpid () ) != TRUE)
    {
	(void) fprintf (stderr, "Error writing PID\t%s\n",
			sys_errlist[errno]);
	exit (RV_WRITE_ERROR);
    }
    if (ch_flush (cm_channel) != TRUE)
    {
	(void) fprintf (stderr, "Error flushing channel\n");
	exit (RV_WRITE_ERROR);
    }
    /*  Connect to Connection Management tool with standard IO protocol  */
    /*  Open connection  */
    if ( ( stdio = ch_open_connection (host_addr, port_number) ) == NULL )
    {
	(void) fprintf (stderr,
			"Could not open connection to Connection Management tool");
	exit (RV_UNDEF_ERROR);
    }
    /*  Send protocol info  */
    if (write_protocol (stdio, "conn_mngr_stdio", 0) != TRUE)
    {
	(void) fprintf (stderr,
			"Error writing authentication information\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (stdio);
	exit (RV_SYS_ERROR);
    }
    /*  Get returned message  */
    if ( ( message = pio_read_string (stdio, &length) ) == NULL )
    {
	(void) fprintf (stderr,
			"Error reading return message\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (stdio);
	exit (RV_SYS_ERROR);
    }
    if (length > 0)
    {
	(void) fprintf (stderr,
			"\"conn_mngr_stdio\" connection refused. Reason: %s\n",
			message);
	m_free (message);
	(void) ch_close (stdio);
	exit (RV_UNDEF_ERROR);
    }
    m_free (message);
    /*  Get returned module name  */
    if ( ( message = pio_read_string (stdio, (unsigned int *) NULL) ) == NULL )
    {
	(void) fprintf (stderr,
			"Error reading server module name\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (stdio);
	exit (RV_UNDEF_ERROR);
    }
    m_free (message);
    if (pio_write_string (stdio, my_hostname) != TRUE)
    {
	(void) fprintf (stderr, "Error writing hostname\t%s\n",
			sys_errlist[errno]);
	exit (RV_WRITE_ERROR);
    }
    if (pio_write32s ( stdio, getpid () ) != TRUE)
    {
	(void) fprintf (stderr, "Error writing PID\t%s\n",
			sys_errlist[errno]);
	exit (RV_WRITE_ERROR);
    }
    if (ch_flush (stdio) != TRUE)
    {
	(void) fprintf (stderr, "Error flushing channel\n");
	exit (RV_WRITE_ERROR);
    }
    ch_stdin = stdio;
    ch_stdout = stdio;
    ch_stderr = stdio;
}   /*  End Function attempt_connection_to_cm  */

static flag cm_command_func (channel, info)
/*  This routine is called when new input occurs on the connection to the
    Connection Management tool.
    The channel object is given by  channel  .
    An arbitrary pointer may be written to the storage pointed to by  info
    The pointer written here will persist until the channel is unmanaged
    (or a subsequent callback routine changes it).
    The routine returns TRUE if the channel is to remain managed and
    open, else it returns FALSE (indicating that the channel is to be
    unmanaged and closed). This routine MUST NOT unmanage or close the
    channel given by  channel  .
*/
Channel channel;
void **info;
{
    unsigned long command;
    unsigned long conn_id;
    unsigned long port_number;
    char *hostname;
    char *protocol_name;
    extern flag clean_exit_request;
    extern Channel cm_channel;
    extern void (*exit_schedule_function) ();
    extern void (*quiescent_function) ();
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "cm_command_func";

    if (channel != cm_channel)
    {
	(void) fprintf (stderr, "Bad channel\n");
	a_prog_bug (function_name);
    }
    if (pio_read32 (channel, &command) != TRUE)
    {
	(void) fprintf (stderr, "Error reading command\t%s\n",
			sys_errlist[errno]);
	return (FALSE);
    }
    switch (command)
    {
      case CM_TOOL_ATTEMPT_CONNECTION:
	if ( ( hostname = pio_read_string (channel, (unsigned int *) NULL) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error reading hostname\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	if (pio_read32 (channel, &port_number) != TRUE)
	{
	    (void) fprintf (stderr, "Error reading port_number\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	if ( ( protocol_name = pio_read_string (channel,
						(unsigned int *) NULL) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error reading protocol name\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	if (conn_attempt_connection (hostname, (unsigned int) port_number,
				     protocol_name)
	    != TRUE)
	{
	    (void) fprintf (stderr, "Error attempting connection\n");
	}
	break;
      case CM_TOOL_CLOSE_CONNECTION:
	if (pio_read32 (channel, &conn_id) != TRUE)
	{
	    (void) fprintf (stderr, "Error reading command\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	if (conn_close ( (Connection) conn_id ) != TRUE)
	{
	    (void) fprintf (stderr, "Error closing connection\t%s\n",
			    sys_errlist[errno]);
	}
	break;
      case CM_TOOL_EXIT_MODULE:
	if (exit_schedule_function == NULL)
	{
	    exit (RV_OK);
	}
	else
	{
	    (*exit_schedule_function) ();
	}
	break;
      case CM_TOOL_NOW_QUIESCENT:
	if (quiescent_function != NULL)
	{
	    (*quiescent_function) ();
	}
	break;
      default:
	(void) fprintf (stderr, "%s: illegal command value from CM tool: %u\n",
			function_name, command);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function cm_command_func  */

static void cm_close_func (channel, info)
/*  This routine is called when the connection to the Connection Management
    tool closes.
    The channel object is given by  channel  .
    An arbitrary pointer may be written to the storage pointed to by  info
    The pointer written here will persist until the channel is unmanaged
    (or a subsequent callback routine changes it).
    The routine returns TRUE if the channel is to remain managed and
    open, else it returns FALSE (indicating that the channel is to be
    unmanaged and closed). This routine MUST NOT unmanage or close the
    channel given by  channel  .
*/
Channel channel;
void **info;
{
    extern Channel cm_channel;
    static char function_name[] = "cm_close_func";

    if (channel != cm_channel)
    {
	(void) fprintf (stderr, "Bad channel\n");
	a_prog_bug (function_name);
    }
    (void) fprintf (stderr,
		    "PANIC: lost connection to Connection Management tool\n");
    a_print_abort ();
}   /*  End Function cm_close_func  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void conn_register_managers (manage_func, unmanage_func, exit_schedule_func)
/*  This routine will register the channel management functions. This routine
    MUST be called prior to the routine:  conn_become_server  .
    See the  chm_  routines for information on channel management.
    The function pointer  manage_func  must refer to a function with the same
    interface definition as the  chm_manage  function.
    The function pointer  unmanage_func  must refer to a function with the same
    interface definition as the  chm_unmanage  function.
    The routine which should be called when a clean exit is to be scheduled
    must be pointed to by  exit_schedule_func  .
    The interface to this routine is as follows:

    void exit_schedule_func ()
    *   This routine is called when the  conn_  package wishes to schedule a
        clean exit from the module.
	This routine may return (ie. it need not exit).
	The routine returns nothing.
    *

    If this is NULL, then exit(3) is called instead.

    The routine returns nothing.
*/
flag (*manage_func) ();
void (*unmanage_func) ();
void (*exit_schedule_func) ();
{
    extern flag (*manage_channel) ();
    extern void (*unmanage_channel) ();
    extern void (*exit_schedule_function) ();
    static char function_name[] = "conn_register_managers";

    if ( (manage_channel != NULL) || (unmanage_channel != NULL) )
    {
	(void) fprintf (stderr, "Channel managers already registered\n");
	a_prog_bug (function_name);
    }
    manage_channel = manage_func;
    unmanage_channel = unmanage_func;
    exit_schedule_function = exit_schedule_func;
    attempt_connection_to_cm ();
}   /*  End Function conn_register_managers  */

/*PUBLIC_FUNCTION*/
void conn_register_server_protocol (protocol_name, version, max_connections,
				    open_func, read_func, close_func)
/*  This routine will register the support of a new Karma protocol for the
    Karma ports which are being managed by the various routines in this file.
    This routine may be called at any time.
    The name of the new protocol to support must be pointed to by
    protocol_name  .Note that this is only for incoming connections.
    The version number for the protocol must be given by  version  .When any
    changes to the protocol are made, this should be increased.
    The maximum number of incoming connections to this server must be given by
    max_connections  .If this is 0, an unlimited number of connections is
    permitted.
    The function which will register the opening of a connection must be
    pointed to by  open_func  .
    The interface to this function is given below:

    flag open_func (connection, info)
    *   This routine will register the opening of a connection.
        The connection will be given by  connection  .
	The routine will write any appropriate information to the pointer
	pointed to by  info  .
	The routine returns TRUE on successful registration,
	else it returns FALSE (indicating the connection should be closed).
	Note that the  close_func  will not be called if this routine returns
	FALSE.
    *
    Connection connection;
    void **info;

    The function which will read data from the connection must be pointed to by
    read_func  .
    The interface to this function is given below:

    flag read_func (connection, info)
    *   This routine will read in data from the connection given by  connection
        and will write any appropriate information to the pointer pointed to by
	info  .
	The routine returns TRUE on successful reading,
	else it returns FALSE (indicating the connection should be closed).
	Note that the  close_func  will be called if this routine returns FALSE
    *
    Connection connection;
    void **info;

    The function which will be called prior to closure of a connection must be
    pointed to by  close_func  .
    The interface to this function is given below:

    void close_func (connection, info)
    *   This routine will register the closure of a connection.
	When this routine is called, this is the last chance to read any
	buffered data from the channel associated with the connection object.
        The connection will be given by  connection  .
        The connection information pointer will be given by  info  .
	The routine returns nothing.
    *
    Connection connection;
    void *info;

    The routine returns nothing.
*/
char *protocol_name;
unsigned int version;
unsigned int max_connections;
flag (*open_func) ();
flag (*read_func) ();
void (*close_func) ();
{
    struct serv_protocol_list_type *entry;
    struct serv_protocol_list_type *last_entry;
    extern struct serv_protocol_list_type *serv_protocol_list;
    extern struct protocol_list_type *protocol_list;
    static char function_name[] = "conn_register_server_protocol";

    if (strlen (protocol_name) >= PROTOCOL_NAME_LENGTH)
    {
	(void) fprintf (stderr,
			"Protocol name: \"%s\" too long. Max: %u characters\n",
			protocol_name, PROTOCOL_NAME_LENGTH);
	a_prog_bug (function_name);
    }
    /*  Test if protocol already supported  */
    if (get_serv_protocol_info (protocol_name) != NULL)
    {
	/*  Already supported: error  */
	(void) fprintf (stderr, "Protocol: \"%s\" already supported\n",
			protocol_name);
	a_prog_bug (function_name);
    }
    /*  Allocate new entry  */
    if ( ( entry = (struct serv_protocol_list_type *) m_alloc (sizeof *entry) )
	== NULL )
    {
	m_abort (function_name, "new protocol list entry");
    }
    if ( ( (*entry).protocol_name = st_dup (protocol_name) ) == NULL )
    {
	m_abort (function_name, "protocol name");
    }
    (*entry).version = version;
    (*entry).max_connections = max_connections;
    (*entry).connection_count = 0;
    (*entry).open_func = open_func;
    (*entry).read_func = read_func;
    (*entry).close_func = close_func;
    (*entry).authorised_hosts = NULL;
    (*entry).next = NULL;
    if (serv_protocol_list == NULL)
    {
	/*  Create protocol list  */
	serv_protocol_list = entry;
    }
    else
    {
	/*  Append entry to protocol list  */
	for (last_entry = serv_protocol_list; (*last_entry).next != NULL;
	     last_entry = (*last_entry).next);
	(*last_entry).next = entry;
    }
    return;
}   /*  End Function conn_register_server_protocol  */

/*PUBLIC_FUNCTION*/
void conn_register_client_protocol (protocol_name, version, max_connections,
				    validate_func,
				    open_func, read_func, close_func)
/*  This routine will register the support of a new Karma protocol for outgoing
    (client) connections.
    This routine may be called at any time.
    The name of the new protocol to support must be pointed to by
    protocol_name  .Note that this is only for outgoing connections.
    The version number for the protocol must be given by  version  .When any
    changes to the protocol are made, this should be increased.
    The maximum number of outgoing connections from this client must be given
    by max_connections  .If this is 0, an unlimited number of connections is
    permitted.
    The function which will validate the opening of a new connection (prior to
    any attempts to connect to the server) must be pointed to by
    validate_func  .
    The interface to this function is given below:

    flag validate_func (info)
    *   This routine will validate whether it is appropriate to open a
        connection.
	The routine will write any appropriate information to the pointer
	pointed to by  info  .The pointer value written here will be passed
	to the other routines.
	The routine returns TRUE if the connection should be attempted,
	else it returns FALSE (indicating the connection should be aborted).
	NOTE: Even if this routine is called and returns TRUE, there is no
	guarantee that the connection will be subsequently opened.
    *
    void **info;

    The function which will register the opening of a connection must be
    pointed to by  open_func  .
    The interface to this function is given below:

    flag open_func (connection, info)
    *   This routine will register the opening of a connection.
        The connection will be given by  connection  .
	The routine will write any appropriate information to the pointer
	pointed to by  info  .
	The routine returns TRUE on successful registration,
	else it returns FALSE (indicating the connection should be closed).
	Note that the  close_func  will not be called if this routine returns
	FALSE.
    *
    Connection connection;
    void **info;

    The function which will read data from the connection must be pointed to by
    read_func  .
    The interface to this function is given below:

    flag read_func (connection, info)
    *   This routine will read in data from the connection given by  connection
        and will write any appropriate information to the pointer pointed to by
	info  .
	The routine returns TRUE on successful reading,
	else it returns FALSE (indicating the connection should be closed).
	Note that the  close_func  will be called if this routine returns FALSE
    *
    Connection connection;
    void **info;

    The function which will be called prior to closure of a connection must be
    pointed to by  close_func  .
    The interface to this function is given below:

    void close_func (connection, info)
    *   This routine will register a closure of a connection.
	When this routine is called, this is the last chance to read any
	buffered data from the channel associated with the connection object.
        The connection will be given by  connection  .
        The connection information pointer will be given by  info  .
	The routine returns nothing.
    *
    Connection connection;
    void *info;

    The routine returns nothing.
*/
char *protocol_name;
unsigned int version;
unsigned int max_connections;
flag (*validate_func) ();
flag (*open_func) ();
flag (*read_func) ();
void (*close_func) ();
{
    struct client_protocol_list_type *entry;
    struct client_protocol_list_type *last_entry;
    extern struct client_protocol_list_type *client_protocol_list;
    extern struct protocol_list_type *protocol_list;
    static char function_name[] = "conn_register_client_protocol";

    if (strlen (protocol_name) >= PROTOCOL_NAME_LENGTH)
    {
	(void) fprintf (stderr,
			"Protocol name: \"%s\" too long. Max: %u characters\n",
			protocol_name, PROTOCOL_NAME_LENGTH);
	a_prog_bug (function_name);
    }
    if (strcmp (protocol_name, "conn_mngr_control") == 0)
    {
	(void) fprintf (stderr, "Client protocol: \"%s\" reserved\n",
			protocol_name);
	a_prog_bug (function_name);
    }
    /*  Test if protocol already supported  */
    if (get_client_protocol_info (protocol_name) != NULL)
    {
	/*  Already supported: error  */
	(void) fprintf (stderr, "Protocol: \"%s\" already supported\n",
			protocol_name);
	a_prog_bug (function_name);
    }
    /*  Allocate new entry  */
    if ( ( entry = (struct client_protocol_list_type *)
	  m_alloc (sizeof *entry) )
	== NULL )
    {
	m_abort (function_name, "new protocol list entry");
    }
    if ( ( (*entry).protocol_name = st_dup (protocol_name) ) == NULL )
    {
	m_abort (function_name, "protocol name");
    }
    (*entry).version = version;
    (*entry).max_connections = max_connections;
    (*entry).connection_count = 0;
    (*entry).validate_func = validate_func;
    (*entry).open_func = open_func;
    (*entry).read_func = read_func;
    (*entry).close_func = close_func;
    (*entry).next = NULL;
    if (client_protocol_list == NULL)
    {
	/*  Create protocol list  */
	client_protocol_list = entry;
    }
    else
    {
	/*  Append entry to protocol list  */
	for (last_entry = client_protocol_list; (*last_entry).next != NULL;
	     last_entry = (*last_entry).next);
	(*last_entry).next = entry;
    }
    return;
}   /*  End Function conn_register_client_protocol  */

/*PUBLIC_FUNCTION*/
Channel conn_get_channel (connection)
/*  This routine will extract the channel object associated with a connection.
    The connection object must be given by  connection  .
    The routine returns the channel object.
*/
Connection connection;
{
    static char function_name[] = "conn_get_channel";

    if (connection == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (*connection).magic_number != OBJECT_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid connection object\n");
	a_prog_bug (function_name);
    }
    return ( (*connection).channel );
}   /*  End Function conn_get_channel  */

/*PUBLIC_FUNCTION*/
flag conn_attempt_connection (hostname, port_number, protocol_name)
/*  This routine will attempt to make a connection to a server. The routine
    always makes a connection using the most efficient transport layer
    available.
    The hostname of the machine on which the server is running must be pointed
    to by  hostname  .
    The Karma port number to connect to must be given by  port_number  .
    The protocol to connect with must be pointed to by  protocol_name  .
    The routine returns TRUE if the connection was successfull,
    else it returns FALSE. Note that this will cause the appropriate callback
    functions registered with  conn_register_client_protocol  to be called.
*/
char *hostname;
unsigned int port_number;
char *protocol_name;
{
    flag local;
    int bytes;
    unsigned int length;
    unsigned long host_addr;
    Connection new_connection;
    Connection last_entry;
    Channel channel;
    void *info = NULL;
    char *message;
    struct client_protocol_list_type *client_protocol_info;
    extern Channel cm_channel;
    extern Connection client_connections;
    extern struct client_protocol_list_type *client_protocol_list;
    extern flag (*manage_channel) ();
    extern void (*unmanage_channel) ();
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "conn_attempt_connection";

    if (strlen (protocol_name) >= PROTOCOL_NAME_LENGTH)
    {
	(void) fprintf (stderr,
			"Protocol name: \"%s\" too long. Max: %u characters\n",
			protocol_name, PROTOCOL_NAME_LENGTH);
	a_prog_bug (function_name);
    }
    if (manage_channel == NULL)
    {
	(void) fprintf (stderr, "Channel managers not registered\n");
	a_prog_bug (function_name);
    }
    if (strcmp (protocol_name, "conn_mngr_control") == 0)
    {
	(void) fprintf (stderr, "Client protocol: \"%s\" reserved\n",
			protocol_name);
	a_prog_bug (function_name);
    }
    if (strcmp (protocol_name, "conn_mngr_stdio") == 0)
    {
	(void) fprintf (stderr, "Client protocol: \"%s\" reserved\n",
			protocol_name);
	a_prog_bug (function_name);
    }
    if ( ( host_addr = r_get_inet_addr_from_host (hostname, &local) ) == 0 )
    {
	(void) fprintf (stderr, "Error getting host address for: \"%s\"\n",
			hostname);
	return (FALSE);
    }
    if (local)
    {
	host_addr = 0;
    }
    if ( ( client_protocol_info = get_client_protocol_info (protocol_name) )
	== NULL )
    {
	(void) fprintf (stderr, "Protocol: \"%s\" not supported\n",
			protocol_name);
	return (FALSE);
    }
    if ( ( (*client_protocol_info).max_connections > 0 ) &&
	( (*client_protocol_info).connection_count
	 >= (*client_protocol_info).max_connections ) )
    {
	(void) fprintf (stderr,
			"Maximum number of client connections reached for protocol: \"%s\"\n",
			protocol_name);
	return (FALSE);
    }
    /*  Validate  */
    if ( (*client_protocol_info).validate_func != NULL )
    {
	if ( (* (*client_protocol_info).validate_func ) (&info) != TRUE )
	{
	    return (FALSE);
	}
    }
    if ( ( new_connection = (Connection) m_alloc (sizeof *new_connection) )
	== NULL )
    {
	m_error_notify (function_name, "new connection object");
	return (FALSE);
    }
    (*new_connection).client = TRUE;
    (*new_connection).protocol_name = (*client_protocol_info).protocol_name;
    (*new_connection).connection_count =
    &(*client_protocol_info).connection_count;
    (*new_connection).read_func = (*client_protocol_info).read_func;
    (*new_connection).close_func = (*client_protocol_info).close_func;
    (*new_connection).info = info;
    (*new_connection).prev = NULL;
    (*new_connection).next = NULL;
    (*new_connection).list_start = &client_connections;
    /*  Open connection  */
    if ( ( channel = ch_open_connection (host_addr, port_number) ) == NULL )
    {
	a_func_abort (function_name, "Could not open connection");
	m_free ( (char *) new_connection );
	return (FALSE);
    }
    (*new_connection).channel = channel;
    /*  Send protocol info  */
    if (write_protocol (channel, protocol_name,(*client_protocol_info).version)
	!= TRUE)
    {
	(void) fprintf (stderr,
			"Error writing authentication information\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (channel);
	m_free ( (char *) new_connection );
	return (FALSE);
    }
    /*  Get returned message  */
    if ( ( message = pio_read_string (channel, &length) ) == NULL )
    {
	(void) fprintf (stderr,
			"Error reading return message\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (channel);
	m_free ( (char *) new_connection );
	return (FALSE);
    }
    if (length > 0)
    {
	(void) fprintf (stderr,
			"Connection refused for protocol: \"%s\". Reason: %s\n",
			protocol_name, message);
	m_free (message);
	(void) ch_close (channel);
	m_free ( (char *) new_connection );
	return (FALSE);
    }
    m_free (message);
    /*  Get returned module name  */
    if ( ( (*new_connection).module_name =
	  pio_read_string (channel, (unsigned int *) NULL) ) == NULL )
    {
	(void) fprintf (stderr,
			"Error reading server module name\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (channel);
	m_free ( (char *) new_connection );
	return (FALSE);
    }
    if ( (*manage_channel) (channel, (void *) new_connection,
			    client_connection_input_func,
			    connection_close_func,
			    ( flag (*) () ) NULL,
			    ( flag (*) () ) NULL ) != TRUE )
    {
	(void) ch_close (channel);
	m_free ( (char *) new_connection );
	a_func_abort (function_name, "Could not manage channel");
	return (FALSE);
    }
    ++*(*new_connection).connection_count;
    (*new_connection).magic_number = OBJECT_MAGIC_NUMBER;
    if ( (*client_protocol_info).open_func != NULL )
    {
	if ( (* (*client_protocol_info).open_func ) (new_connection,
						     &(*new_connection).info)
	    != TRUE )
	{
	    (*unmanage_channel) (channel);
	    (void) ch_close (channel);
	    --*(*new_connection).connection_count;
	    (*new_connection).magic_number = 0;
	    m_free ( (char *) new_connection );
	    return (FALSE);
	}
    }
    /*  Append connection to list  */
    if (client_connections == NULL)
    {
	/*  No list yet  */
	client_connections = new_connection;
    }
    else
    {
	/*  Search for end of list  */
	for (last_entry = client_connections; (*last_entry).next != NULL;
	     last_entry = (*last_entry).next);
	/*  Append entry  */
	(*last_entry).next = new_connection;
	(*new_connection).prev = last_entry;
    }
    /*  Drain any input on new connection  */
    if ( ( bytes = ch_get_bytes_readable (channel) ) < 0 )
    {
	(void) exit (RV_SYS_ERROR);
    }
    if ( (bytes > 0) && ( (*new_connection).read_func == NULL ) )
    {
	(void) fprintf (stderr, "Input on new connection not being read\n");
	a_prog_bug (function_name);
    }
    while (bytes > 0)
    {
	if ( (* (*new_connection).read_func ) (new_connection,
					       &(*new_connection).info)
	    == FALSE )
	{
	    return (FALSE);
	}
	if ( ( bytes = ch_get_bytes_readable (channel) ) < 0 )
	{
	    (void) exit (RV_SYS_ERROR);
	}
    }
    if (cm_channel != NULL)
    {
	/*  Connected to Connection Management tool  */
	if (pio_write32 (cm_channel, CM_LIB_NEW_CONNECTION) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing command value\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if (pio_write_string (cm_channel, protocol_name) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing protocol name\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if (pio_write32 (cm_channel, host_addr) != TRUE)
	{
	    (void) fprintf (stderr,
			    "Error writing host Internet address\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if (pio_write32 (cm_channel, (unsigned long) port_number) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing port number\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if (pio_write32 (cm_channel, (unsigned long) new_connection) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing Connection ID\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if (ch_flush (cm_channel) != TRUE)
	{
	    (void) fprintf (stderr, "Error flushing channel\n");
	    exit (RV_WRITE_ERROR);
	}
    }
    return (TRUE);
}   /*  End Function conn_attempt_connection  */

/*PUBLIC_FUNCTION*/
flag conn_close (connection)
/*  This routine will close a connection. This will cause the closure callback
    routine registered to be executed.
    The connection object must be given by  connection  .
    The routine returns TRUE on success, else it returns FALSE.
*/
Connection connection;
{
    Channel channel;
    extern void (*unmanage_channel) ();
    static char function_name[] = "conn_close";

    if (connection == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if (unmanage_channel == NULL)
    {
	(void) fprintf (stderr, "Channel managers not registered\n");
	a_prog_bug (function_name);
    }
    if ( (*connection).magic_number != OBJECT_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid connection object\n");
	a_prog_bug (function_name);
    }
    channel = (*connection).channel;
    dealloc_connection (connection);
    (*unmanage_channel) (channel);
    return ( ch_close (channel) );
}   /*  End Function conn_close  */

/*PUBLIC_FUNCTION*/
flag conn_become_server (port_number, retries)
/*  This routine will allocate a Karma port for the module so that it can
    operate as a server (able to receive network connections).
    The port number to allocate must be pointed to by  port_number  .The
    routine will write the actual port number allocated to this address. This
    must point to an address which lies on an  int  boundary.
    The number of succsessive port numbers to attempt to allocate before giving
    up must be given by  retries  .If this is 0, then the routine will give up
    immediately if the specified port number is in use.
    The routine returns TRUE on success, else it returns FALSE.
*/
unsigned int *port_number;
unsigned int retries;
{
    unsigned int num_docks;
    unsigned int dock_count;
    flag return_value = FALSE;
    Channel *port;
    extern Channel cm_channel;
    extern flag ran_become_server;
    extern flag (*manage_channel) ();
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "conn_become_server";

    if (manage_channel == NULL)
    {
	(void) fprintf (stderr, "Channel managers not registered\n");
	a_prog_bug (function_name);
    }
    if (ran_become_server != TRUE)
    {
	/*  Set up internal protocol support  */
	conn_register_server_protocol ("ping_server", 0, 0,
				       ( flag (*) () ) NULL,
				       respond_to_ping_server_from_client,
				       ( void (*) () ) NULL);
	conn_register_server_protocol ("server_exit", 0, 1,
				       register_server_exit,
				       ( flag (*) () ) NULL,
				       ( void (*) () ) NULL);
	ran_become_server = TRUE;
    }
    if ( ( port = ch_alloc_port (port_number, retries, &num_docks) ) == NULL )
    {
	a_func_abort (function_name, "Error becoming server");
	return (FALSE);
    }
    for (dock_count = 0; dock_count < num_docks; ++dock_count)
    {
	if ( (*manage_channel) (port[dock_count], (void *) NULL,
				dock_input_func,
				( void (*) () ) NULL,
				( flag (*) () ) NULL,
				( flag (*) () ) NULL) != TRUE )
	{
	    (void) fprintf (stderr, "Error managing dock: %u\n", dock_count);
	    (void) ch_close (port[dock_count]);
	}
	else
	{
	    /*  At least one dock was managed: call this success  */
	    return_value = TRUE;
	}
    }
    m_free ( (char *) port );
    if ( return_value && (cm_channel != NULL) )
    {
	/*  Have a dock and are connected to Connection Management tool  */
	if (pio_write32 (cm_channel, CM_LIB_PORT_NUMBER) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing command value\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if (pio_write32 (cm_channel, (unsigned long) *port_number) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing port number\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if (ch_flush (cm_channel) != TRUE)
	{
	    (void) fprintf (stderr, "Error flushing channel\n");
	    exit (RV_WRITE_ERROR);
	}
    }
    return (return_value);
}   /*  End Function conn_become_server  */

/*PUBLIC_FUNCTION*/
unsigned int conn_get_num_serv_connections (protocol_name)
/*  This routine will get the number of connections to the server with a
    specified protocol.
    The protocol name must be pointed to by  protocol_name  .
    The routine returns the number of current connections. A return value of
    0 indicates that the protocol is not supported or there are no current
    connections.
*/
char *protocol_name;
{
    struct serv_protocol_list_type *serv_protocol_info;
    static char function_name[] = "conn_get_num_serv_connections";

    if ( ( serv_protocol_info = get_serv_protocol_info (protocol_name) )
	== NULL )
    {
	/*  Protocol is not supported  */
	return (0);
    }
    return ( (*serv_protocol_info).connection_count );
}   /*  End Function conn_get_num_serv_connections  */

/*PUBLIC_FUNCTION*/
unsigned int conn_get_num_client_connections (protocol_name)
/*  This routine will get the number of connections from the client with a
    specified protocol.
    The protocol name must be pointed to by  protocol_name  .
    The routine returns the number of current connections. A return value of
    0 indicates that the protocol is not supported or there are no current
    connections.
*/
char *protocol_name;
{
    struct client_protocol_list_type *client_protocol_info;
    static char function_name[] = "conn_get_num_client_connections";

    if ( ( client_protocol_info = get_client_protocol_info (protocol_name) )
	== NULL )
    {
	/*  Protocol is not supported  */
	return (0);
    }
    return ( (*client_protocol_info).connection_count );
}   /*  End Function conn_get_num_client_connections  */

/*PUBLIC_FUNCTION*/
Connection conn_get_serv_connection (protocol_name, number)
/*  This routine will get Nth connection to the server with a specified
    protocol. The first connection is numbered 0.
    The protocol name must be pointed to by  protocol_name  .
    The number of the connection to get must be given by  number  .
    The routine returns the connection on success, else it returns NULL.
*/
char *protocol_name;
unsigned int number;
{
    extern Connection serv_connections;

    return ( get_numbered_connection (protocol_name, number,
				      serv_connections) );
}   /*  End Function conn_get_serv_connection  */

/*PUBLIC_FUNCTION*/
Connection conn_get_client_connection (protocol_name, number)
/*  This routine will get Nth connection from the client with a specified
    protocol. The first connection is numbered 0.
    The protocol name must be pointed to by  protocol_name  .
    The number of the connection to get must be given by  number  .
    The routine returns the connection on success, else it returns NULL.
*/
char *protocol_name;
unsigned int number;
{
    extern Connection client_connections;

    return ( get_numbered_connection (protocol_name, number,
				      client_connections) );
}   /*  End Function conn_get_client_connection  */

/*PUBLIC_FUNCTION*/
void *conn_get_connection_info (connection)
/*  This routine will get the arbitrary information for a connection.
    The connection must be given by  connection  .
    The routine aborts the process if  connection  is not valid.
    The routine returns a pointer to the arbitrary information. This may be a
    NULL pointer.
*/
Connection connection;
{
    static char function_name[] = "conn_get_connection_info";

    if (connection == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (*connection).magic_number != OBJECT_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid connection object\n");
	a_prog_bug (function_name);
    }
    return ( (*connection).info );
}   /*  End Function conn_get_connection_info  */

/*PUBLIC_FUNCTION*/
flag conn_controlled_by_cm_tool ()
/*  This routine will determine if the module is being controlled by the
    Connection Management tool.
    The routine returns TRUE if the module is controlled, else it returns FALSE
*/
{
    extern Channel cm_channel;

    return ( (cm_channel == NULL) ? FALSE : TRUE );
}   /*  End Function conn_controlled_by_cm_tool  */

/*PUBLIC_FUNCTION*/
char *conn_get_connection_module_name (connection)
/*  This routine will get the name of the module at the other end of a
    connection.
    The connection must be given by  connection  .
    The routine aborts the process if  connection  is not valid.
    The routine returns a pointer to the module name.
*/
Connection connection;
{
    static char function_name[] = "conn_get_connection_module_name";

    if (connection == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (*connection).magic_number != OBJECT_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid connection object\n");
	a_prog_bug (function_name);
    }
    if ( (*connection).module_name == NULL )
    {
	(void) fprintf (stderr, "Invalid connection module_name\n");
	a_prog_bug (function_name);
    }
    return ( (*connection).module_name );
}   /*  End Function conn_get_connection_module_name  */

/*PUBLIC_FUNCTION*/
void conn_register_cm_quiescent_func (func)
/*  This routine will register a callback function to be called when the
    Connection Management tool or shell is quiescent (ie. all modules have
    started and all initial connections made). The function is ONLY called if
    the module is running under the Connection Management tool or shell.
    Only one callback may be registered.
    The routine which should be called when the Connection Management tool or
    shell is quiescent must be pointed to by  func  .
    The interface to this routine is as follows:

    void func ()
    *   This routine is called when the Connection Management tool or
        shell is quiescent.
	The routine returns nothing.
    *

    The routine returns nothing.
*/
void (*func) ();
{
    extern void (*quiescent_function) ();
    static char function_name[] = "conn_register_cm_quiescent_func";

    if (conn_controlled_by_cm_tool () != TRUE)
    {
	(void) fprintf (stderr, "Not controlled by CM tool or shell\n");
	a_prog_bug (function_name);
    }
    if (quiescent_function != NULL)
    {
	(void) fprintf (stderr, "Quiescent callback already registered\n");
	a_prog_bug (function_name);
    }
    quiescent_function = func;
}   /*  End Function conn_register_cm_quiescent_func  */

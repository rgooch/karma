/*LINTLIBRARY*/
/*  main.c

    This code provides high level connection control/ management routines.

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

    Updated by      Richard Gooch   27-SEP-1993: Improved error message in
  cm_command_func  .

    Updated by      Richard Gooch   23-NOV-1993: Removed some dead externs.

    Updated by      Richard Gooch   3-MAY-1994: Added encryption support.

    Updated by      Richard Gooch   16-MAY-1994: Dropped encryption once basic
  setup information is transferred for local connections.

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>
  and created  conn_extract_protocols  .

    Updated by      Richard Gooch   26-AUG-1994: Moved  typedef  of Connection
  class to header.

    Updated by      Richard Gooch   4-OCT-1994: Changed to  m_cmp  routine in
    order to avoid having to link with buggy UCB compatibility library in
    Slowaris 2.3

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   25-NOV-1994: Switched to  cen_  package.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/conn/main.c

    Updated by      Richard Gooch   3-DEC-1994: Made use of  pgp_  package.

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>

    Updated by      Richard Gooch   1-JAN-1995: Added per-protocol encryption
  support. Raw protocol version number incremented.

    Updated by      Richard Gooch   7-JAN-1995: Changed from "PRIMARY_KEY" to
  "RAW_PROTOCOL" for raw protocol.

    Updated by      Richard Gooch   23-JAN-1995: Added some casts to keep
  IRIX compiler happy.

    Updated by      Richard Gooch   14-MAR-1995: Changed
  <attempt_connection_to_cm> to send fully qualified hostname if possible.

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Updated by      Richard Gooch   14-JUN-1995: Made use of <ex_uint>.

    Updated by      Richard Gooch   16-JUN-1995: Made use of
  <r_get_fq_hostname>.

    Updated by      Richard Gooch   10-AUG-1995: Removed printing of error
  message in <conn_attempt_connection> if raw connection failed.

    Last updated by Richard Gooch   30-NOV-1995: Changed from call to <_exit>
  to call to <exit> in <cm_close_func> for IRIX sproc() problems.


*/

#include <stdio.h>
#include <math.h>
#include <ctype.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <karma.h>
#include <karma_conn.h>
#include <karma_pio.h>
#include <karma_chs.h>
#include <karma_cen.h>
#include <karma_pgp.h>
#include <karma_ch.h>
#include <karma_st.h>
#include <karma_ex.h>
#include <karma_rp.h>
#include <karma_md.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_r.h>
#include <karma_c.h>
#include <k_cm_codes.h>
#include <os.h>


#if defined(HAS_SOCKETS) || defined(HAS_COMMUNICATIONS_EMULATION)
#define COMMUNICATIONS_AVAILABLE
#endif

#define OBJECT_MAGIC_NUMBER (unsigned int) 1794359023
#define VERIFY_CONNECTION(conn) {if (conn == NULL) \
{(void) fprintf (stderr, "NULL connection object\n"); \
 a_prog_bug (function_name); } \
if (conn->magic_number != OBJECT_MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid connection object\n"); \
 a_prog_bug (function_name); }}


#define AUTH_PASSWORD_LENGTH 128
#define CONN_MAGIC_NUMBER (unsigned long) 245987237
#define REVISION_NUMBER (unsigned long) 3
#define SETUP_MESSAGE_LENGTH (4 + 4)
#define PROTOCOL_NAME_LENGTH 80
#define PROTOCOL_DEFINITION_MESSAGE_LENGTH (PROTOCOL_NAME_LENGTH + 4)

#define RAW_PROTOCOL "RAW_PROTOCOL"
#define CM_PROTOCOL_VERSION (unsigned int) 1

#define SECURITY_TYPE_KEY (unsigned int) 0
#define SECURITY_TYPE_IDEA (unsigned int) 1
#define SECURITY_TYPE_PGPuserID_IDEA (unsigned int) 2
#define SECURITY_TYPE_DROP_ENCRYPTION (unsigned int) 3

#define SECURITY_STRING_KEY "key-only"
#define SECURITY_STRING_IDEA "IDEA"
#define SECURITY_STRING_PGPuserID_IDEA "PGPuserID-IDEA"
#define SECURITY_STRING_DROP_ENCRYPTION "drop-encryption"

/*  Definition of security types:
        key-only        IDEA used to verify authentication. Data unencrypted
	IDEA            IDEA used to verify authentication and data encrypted
	PGPuserID-IDEA  PGP used for session key transfer, then use IDEA mode
	drop-encryption drop data encryption for a specific protocol
*/

/*  Raw protocol
    Stage 0: Client connects to server.
    Stage 1: Client then sends:
      Magic number
      Revision number
    Stage 2:
      Client and server negotiate PRIMARY_KEY security if applicable
    Stage 3: Client sends:
      Protocol name [PROTOCOL_NAME_LENGTH]
      Protocol version number
    Stage 4:
      Client and server negotiate protocol security if applicable
    Stage 5: Server replies:
      Verification string (NULL for success)
      Module name (on success only)
    Stage 6: Client sends:
      Module name
    Stage 7: Server reads module name
*/

/*  Security negotiation sequence:
      key-only:
        Key data is used for IDEA session key. Client and server each send an
	IDEA block of random bytes and then write back the blocks. This both
	helps randomise the IDEA IV and also ensures that both ends are
	authenticated (i.e. a third party has not stepped in). Once both ends
	are authenticated, the encryption is dropped.
      IDEA:
        Same as key-only except that encryption is not dropped unless the
	connection is local
      PGPuserID-IDEA:
        PGP is used for IDEA session key transfer. After that IDEA mode is used
*/

/*  Generic connection structure (server and client)  */
struct connection_type
{
    unsigned int magic_number;
    flag client;
    /*  Server specific fields  */
    flag verified_raw;
    flag verified_security;
    flag verified_protocol_security;
    ChConverter pri_encryption_converter;
    ChConverter pro_encryption_converter;
    /*  General fields  */
    Channel channel;
    char *protocol_name;               /*  Pointer copy: do not free    */
    unsigned int *connection_count;
    flag (*read_func) ();
    void (*close_func) ();
    void *info;
    char *module_name;
    unsigned long inet_addr;
    Connection prev;
    Connection next;
    Connection *list_start;
    char idea_block[EN_IDEA_BLOCK_SIZE];
};

/*  Server protocol structures  */
struct serv_protocol_list_type
{
    char *protocol_name;
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
    char *protocol_name;
    unsigned int version;
    unsigned int connection_count;
    unsigned int max_connections;
    flag (*validate_func) ();
    flag (*open_func) ();
    flag (*read_func) ();
    void (*close_func) ();
    struct client_protocol_list_type *next;
};

struct auth_password_list_type
{
    char *protocol_name;
    char password[AUTH_PASSWORD_LENGTH];
    unsigned int security_type;
    struct auth_password_list_type *next;
};


/*  Private data  */
/*  Connection information  */
static flag ran_become_server = FALSE;
static Connection serv_connections = NULL;
static Connection client_connections = NULL;
static struct serv_protocol_list_type *serv_protocol_list = NULL;
static struct client_protocol_list_type *client_protocol_list = NULL;

/*  Connection to Connection Management tool  */
static Channel cm_channel = NULL;

/*  Pointers to channel management code  */
static flag (*manage_channel) () = NULL;
static void (*unmanage_channel) () = NULL;

static void (*exit_schedule_function) () = NULL;

/*  Function to call when CM tool or shell is quiescent  */
static void (*quiescent_function) () = NULL;

/*  Cryptographically strong Pseudo Random Number Generator  */
static RandPool main_randpool = NULL;


/*  Private functions  */
STATIC_FUNCTION (flag dock_input_func, (Channel dock, void **info) );
STATIC_FUNCTION (flag serv_connection_input_func,
		 (Channel channel, void **info) );
STATIC_FUNCTION (flag verify_raw, (Connection connection) );
STATIC_FUNCTION (flag verify_security, (Connection connection) );
STATIC_FUNCTION (flag verify_connection, (Connection conn) );
STATIC_FUNCTION (flag verify_protocol_security, (Connection connection) );
STATIC_FUNCTION (flag client_connection_input_func,
		 (Channel channel, void **info) );
STATIC_FUNCTION (void connection_close_func, (Channel channel, void *info) );
STATIC_FUNCTION (void dealloc_connection, (Connection connection) );
STATIC_FUNCTION (struct serv_protocol_list_type *get_serv_protocol_info,
		 (char *protocol_name) );
STATIC_FUNCTION (struct client_protocol_list_type *get_client_protocol_info,
		 (char *protocol_name) );
STATIC_FUNCTION (struct auth_password_list_type *get_authinfo,
		 (char *protocol) );
STATIC_FUNCTION (struct auth_password_list_type *get_password_list, () );
STATIC_FUNCTION (void convert_password_to_bin,
		 (unsigned char *binpassword, CONST char *asciipassword) );
#ifdef dummy
static flag check_host_access ();
#endif
STATIC_FUNCTION (char *write_protocol, (Channel channel, char *protocol_name,
					unsigned int version) );
static flag respond_to_ping_server_from_client ();
static flag register_server_exit ();
static Connection get_numbered_connection ();
static flag cm_command_func ();
static void cm_close_func ();
STATIC_FUNCTION (void setup_prng, () );
STATIC_FUNCTION (void randpool_destroy_func, (RandPool rp, void *info) );
STATIC_FUNCTION (ChConverter get_encryption,
		 (Channel channel, struct auth_password_list_type *authinfo,
		  flag server, char *rand_block) );


/*  Private functions follow  */

/*  Functions to process input on server connections  */

static flag dock_input_func (Channel dock, void **info)
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
{
    Connection new_conn;
    Connection last_entry;
    extern Connection serv_connections;
    extern flag (*manage_channel) ();
    static char function_name[] = "dock_input_func";

    /*  Allocate new connection object  */
    if ( ( new_conn = (Connection) m_alloc (sizeof *new_conn) )
	== NULL )
    {
	m_error_notify (function_name, "connection object");
	/*  Do not close dock  */
	return (TRUE);
    }
    if ( ( new_conn->channel =
	  ch_accept_on_dock (dock, &new_conn->inet_addr) )
	== NULL )
    {
	a_func_abort (function_name, "could not accept connection on dock");
	return (FALSE);
    }
    new_conn->magic_number = OBJECT_MAGIC_NUMBER;
    new_conn->client = FALSE;
    new_conn->verified_raw = FALSE;
    new_conn->verified_security = FALSE;
    new_conn->verified_protocol_security = FALSE;
    new_conn->pri_encryption_converter = NULL;
    new_conn->pro_encryption_converter = NULL;
    new_conn->protocol_name = NULL;
    new_conn->connection_count = NULL;
    new_conn->read_func = NULL;
    new_conn->close_func = NULL;
    new_conn->info = NULL;
    new_conn->module_name = NULL;
    new_conn->prev = NULL;
    new_conn->next = NULL;
    if ( !(*manage_channel) (new_conn->channel, (void *) new_conn,
			     serv_connection_input_func,
			     connection_close_func,
			     ( flag (*) () ) NULL, ( flag (*) () ) NULL) )
    {
	(void) ch_close (new_conn->channel);
	m_free ( (char *) new_conn );
	a_func_abort (function_name, "could not manage channel");
	return (FALSE);
    }
    new_conn->list_start = &serv_connections;
    /*  Add new connection to end of list  */
    if (serv_connections == NULL)
    {
	/*  No list yet  */
	serv_connections = new_conn;
	return (TRUE);
    }
    /*  Search for end of list  */
    for (last_entry = serv_connections; last_entry->next != NULL;
	 last_entry = last_entry->next);
    last_entry->next = new_conn;
    new_conn->prev = last_entry;
    return (TRUE);
}   /*  End Function dock_input_func  */

static flag serv_connection_input_func (Channel channel, void **info)
/*  This routine is called when new input occurs on a channel.
    The channel object is given by  channel  .
    An arbitrary pointer may be written to the storage pointed to by  info
    The pointer written here will persist until the channel is unmanaged
    (or a subsequent callback routine changes it).
    The routine returns TRUE if the channel is to remain managed and
    open, else it returns FALSE (indicating that the channel is to be
    unmanaged and closed).
*/
{
    int bytes;
    unsigned long old_read_pos, new_read_pos, dummy;
    Connection connection;
    struct serv_protocol_list_type *protocol_info;
    static char function_name[] = "serv_connection_input_func";

    connection = (Connection) *info;
    VERIFY_CONNECTION (connection);
    if (!connection->verified_raw)
    {
	if ( !verify_raw (connection) ) return (FALSE);
	return (TRUE);
    }
    /*  At this point we know that the other end wants to talk the Karma
	protocol.  */
    if (!connection->verified_security)
    {
	if ( !verify_security (connection) ) return (FALSE);
	return (TRUE);
    }
    /*  At this point we know any security requirements have been met for the
	raw protocol.  */
    if (connection->protocol_name == NULL)
    {
	/*  No protocol yet: connection setup information needed  */
	if ( !verify_connection (connection) )
	{
	    /*  Connection was not authorised  */
	    (void) ch_flush (channel);
	    return (FALSE);
	}
	return (TRUE);
    }
    if (!connection->verified_protocol_security)
    {
	if ( !verify_protocol_security (connection) ) return (FALSE);
	return (TRUE);
    }
    if (connection->module_name == NULL)
    {
	/*  Read module name from client  */
	if ( ( connection->module_name =
	      pio_read_string (channel, (unsigned int *) NULL) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "Error reading module name from connection\n");
	    return (FALSE);
	}
	protocol_info = get_serv_protocol_info (connection->protocol_name);
	if (protocol_info->open_func != NULL)
	{
	    /*  Open function registered  */
	    if ( !(*protocol_info->open_func) (connection, &connection->info) )
	    {
		/*  Failure  */
		return (FALSE);
	    }
	}
	connection->read_func = protocol_info->read_func;
	connection->close_func = protocol_info->close_func;
	return (TRUE);
    }
    if ( ( bytes = ch_get_bytes_readable (channel) ) < 0 )
    {
	(void) exit (RV_SYS_ERROR);
    }
    if ( (bytes > 0) && (connection->read_func == NULL) )
    {
	(void) fprintf (stderr,
			"Input on \"%s\" connection not being read (no callback)\n",
			connection->protocol_name);
	a_prog_bug (function_name);
    }
    if (ch_tell (connection->channel, &old_read_pos, &dummy) != TRUE )
    {
	(void) exit (RV_SYS_ERROR);
    }
    while (bytes > 0)
    {
	if ( !(*connection->read_func) (connection, &connection->info) )
	{
	    return (FALSE);
	}
	if (ch_tell ( connection->channel, &new_read_pos, &dummy ) != TRUE )
	{
	    (void) exit (RV_SYS_ERROR);
	}
	if (new_read_pos <= old_read_pos)
	{
	    (void) fprintf (stderr,
			    "Connection read callback for protocol: \"%s\" not draining\n",
			    connection->protocol_name);
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

static flag verify_raw (Connection connection)
/*  [PURPOSE] This routine will verify a raw Karma connection to ensure that
    the magic number and revision number are correct. It will then setup any
    required security.
    <connection> The connection.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int bytes;
    unsigned long magic_number;
    unsigned long revision_number;
    struct auth_password_list_type *authinfo;
    extern char *sys_errlist[];
    static char function_name[] = "verify_raw";

    VERIFY_CONNECTION (connection);
    /*  Check if enough data on connection  */
    if ( ( bytes = ch_get_bytes_readable (connection->channel) ) < 0 )
    {
	(void) exit (RV_SYS_ERROR);
    }
    if (bytes < SETUP_MESSAGE_LENGTH)
    {
	/*  Not enough info was sent  */
	(void) pio_write_string (connection->channel,
				 "Bad setup message length");
	(void) fprintf (stderr,
			"Only: %d bytes of connection setup information ",
			bytes);
	(void) fprintf (stderr,
			"sent\n%d bytes are required: connection closed\n",
			SETUP_MESSAGE_LENGTH);
	return (FALSE);
    }
    /*  Read connection setup information  */
    if ( !pio_read32 (connection->channel, &magic_number) )
    {
	a_func_abort (function_name,
		      "Error reading magic number from connection");
	return (FALSE);
    }
    /*  Check magic number  */
    if (magic_number != CONN_MAGIC_NUMBER)
    {
	/*  Bad magic number  */
	(void) pio_write_string (connection->channel, "Bad magic number");
	(void) fprintf (stderr,
			"WARNING: Connection attempted with bad magic number: %lu\n",
			magic_number);
	(void) fprintf (stderr, "Someone may be trying to stuff around\n");
	return (FALSE);
    }
    /*  Get revision number  */
    if ( !pio_read32 (connection->channel, &revision_number) )
    {
	a_func_abort (function_name,
		      "Error reading revision number from connection");
	return (FALSE);
    }
    /*  Check revision number  */
    if (revision_number != REVISION_NUMBER)
    {
	/*  Bad magic number  */
	(void) pio_write_string (connection->channel, "Bad revision number");
	return (FALSE);
    }
    connection->verified_raw = TRUE;
    authinfo = get_authinfo (RAW_PROTOCOL);
    if (authinfo == NULL)
    {
	/*  No security requirements  */
	connection->verified_security = TRUE;
	return (TRUE);
    }
    if (authinfo->security_type == SECURITY_TYPE_DROP_ENCRYPTION)
    {
	a_func_abort (function_name,
		      "security type: \"drop-encryption\" not valid for PRIMARY_KEY");
	exit (RV_BAD_DATA);
    }
    /*  Setup security requirements  */
    if ( ( connection->pri_encryption_converter =
	  get_encryption (connection->channel, authinfo, TRUE,
			  connection->idea_block) ) == NULL )
    {
	return (FALSE);
    }
    if ( !ch_flush (connection->channel) ) return (FALSE);
    /*  At some point the client will send some verification data  */
    connection->verified_security = FALSE;
    return (TRUE);
}   /*  End Function verify_raw  */

static flag verify_security (Connection connection)
/*  [PURPOSE] This routine will verify a raw Karma connection to ensure that
    the connection is authorised.
    <connection> The connection.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int bytes;
    struct auth_password_list_type *authinfo;
    char idea_blocks[EN_IDEA_BLOCK_SIZE * 2];
    static char function_name[] = "verify_security";

    VERIFY_CONNECTION (connection);
    /*  Check if enough data on connection  */
    if ( ( bytes = ch_get_bytes_readable (connection->channel) ) < 0 )
    {
	(void) exit (RV_SYS_ERROR);
    }
    if (bytes < EN_IDEA_BLOCK_SIZE * 2)
    {
	/*  Not enough info was sent  */
	(void) pio_write_string (connection->channel,
				 "Bad IDEA block message length");
	(void) fprintf (stderr,
			"Only: %d bytes of connection IDEA block data ",
			bytes);
	(void) fprintf (stderr,
			"sent\n%d bytes are required: connection closed\n",
			EN_IDEA_BLOCK_SIZE * 2);
	return (FALSE);
    }
    if (ch_read (connection->channel, idea_blocks, EN_IDEA_BLOCK_SIZE * 2) <
	EN_IDEA_BLOCK_SIZE * 2) return (FALSE);
    if ( !m_cmp (idea_blocks + EN_IDEA_BLOCK_SIZE, connection->idea_block,
		 EN_IDEA_BLOCK_SIZE) )
    {
	/*  Sorry, I'm not convinced  */
	return (FALSE);
    }
    if (ch_write (connection->channel, idea_blocks, EN_IDEA_BLOCK_SIZE) <
	EN_IDEA_BLOCK_SIZE) return (FALSE);
    if ( !ch_flush (connection->channel) ) return (FALSE);
    authinfo = get_authinfo (RAW_PROTOCOL);
    switch (authinfo->security_type)
    {
      case SECURITY_TYPE_KEY:
	ch_unregister_converter (connection->pri_encryption_converter);
	connection->pri_encryption_converter = NULL;
	break;
      case SECURITY_TYPE_IDEA:
      case SECURITY_TYPE_PGPuserID_IDEA:
	if ( ch_test_for_local_connection (connection->channel) )
	{
	    ch_unregister_converter (connection->pri_encryption_converter);
	    connection->pri_encryption_converter = NULL;
	}
	break;
      case SECURITY_TYPE_DROP_ENCRYPTION:
	a_func_abort (function_name,
		      "security type: \"drop-encryption\" not valid for PRIMARY_KEY\n");
	exit (RV_BAD_DATA);
/*
	break;
*/
    }
    connection->verified_security = TRUE;
    return (TRUE);
}   /*  End Function verify_security  */

static flag verify_connection (Connection connection)
/*  This routine will verify whether a connection is permitted or not.
    The connection object must be given by  connection  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    ChConverter converter;
    int bytes;
    unsigned long protocol_version;
    struct serv_protocol_list_type *protocol_info;
    struct auth_password_list_type *authinfo;
    char protocol_buffer[PROTOCOL_NAME_LENGTH + 1];
    extern char module_name[STRING_LENGTH + 1];
    extern char *sys_errlist[];
    static char function_name[] = "verify_connection";

    VERIFY_CONNECTION (connection);
    if ( ( bytes = ch_get_bytes_readable (connection->channel) ) < 0 )
    {
	(void) exit (RV_SYS_ERROR);
    }
    if (bytes < PROTOCOL_DEFINITION_MESSAGE_LENGTH)
    {
	/*  Not enough info was sent  */
	(void) pio_write_string (connection->channel,
				 "Bad protocol definition message length");
	(void) fprintf (stderr,
			"Only: %d bytes of connection setup information ",
			bytes);
	(void) fprintf (stderr,
			"sent\n%d bytes are required: connection closed\n",
			PROTOCOL_DEFINITION_MESSAGE_LENGTH);
	return (FALSE);
    }
    /*  Read protocol name  */
    if (ch_read (connection->channel, protocol_buffer, PROTOCOL_NAME_LENGTH)
	< PROTOCOL_NAME_LENGTH)
    {
	(void) fprintf (stderr,
			"Error reading protocol name from connection\n");
	return (FALSE);
    }
    protocol_buffer[PROTOCOL_NAME_LENGTH] = '\0';
    /*  Get protocol version number  */
    if (pio_read32 (connection->channel, &protocol_version) != TRUE)
    {
	(void) fprintf (stderr,
			"Error reading protocol version number from connection\n");
	return (FALSE);
    }
    /*  Compare protocol number with list of supported ones  */
    if ( ( protocol_info = get_serv_protocol_info (protocol_buffer) ) == NULL )
    {
	/*  Protocol is not supported  */
	/*  Write failure message back to client  */
	(void) pio_write_string (connection->channel,
				 "Protocol is not supported by server");
	return (FALSE);
    }
    if (protocol_version != protocol_info->version)
    {
	(void) pio_write_string (connection->channel,
				 "Protocol version mismatch");
	return (FALSE);
    }
    /*  Protocol is supported: check if more connections allowed  */
    if ( ( protocol_info->max_connections > 0 ) &&
	( protocol_info->connection_count
	 >= protocol_info->max_connections ) )
    {
	(void) pio_write_string (connection->channel,
				 "Connection limit reached for protocol");
	return (FALSE);
    }
    connection->protocol_name = protocol_info->protocol_name;
    connection->connection_count = &protocol_info->connection_count;
    ++protocol_info->connection_count;
    connection->verified_protocol_security = FALSE;
    authinfo = get_authinfo (protocol_buffer);
    if ( (authinfo != NULL) &&
	(authinfo->security_type == SECURITY_TYPE_DROP_ENCRYPTION) )
    {
	if (connection->pri_encryption_converter != NULL)
	{
	    ch_unregister_converter (connection->pri_encryption_converter);
	    connection->pri_encryption_converter = NULL;
	}
	authinfo = NULL;
    }
    if (authinfo == NULL)
    {
	connection->verified_protocol_security = TRUE;
	/*  Write verification message back to client  */
	(void) pio_write_string (connection->channel, (char *) NULL);
	/*  Send module name back to client  */
	(void) pio_write_string (connection->channel, module_name);
	if ( !ch_flush (connection->channel) ) return (FALSE);
	return (TRUE);
    }
    /*  Setup security requirements  */
    if ( ( converter = get_encryption (connection->channel, authinfo, TRUE,
				       connection->idea_block) )
	== NULL ) return (FALSE);
    if ( !ch_flush (connection->channel) ) return (FALSE);
    connection->pro_encryption_converter = converter;
    return (TRUE);
}   /*  End Function verify_connection  */

static flag verify_protocol_security (Connection connection)
/*  [PURPOSE] This routine will verify a Karma connection to ensure that
    the connection is authorised for the appropriate protocol.
    <connection> The connection.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int bytes;
    struct auth_password_list_type *authinfo;
    char idea_blocks[EN_IDEA_BLOCK_SIZE * 2];
    extern char module_name[STRING_LENGTH + 1];
    static char function_name[] = "verify_protocol_security";

    VERIFY_CONNECTION (connection);
    /*  Check if enough data on connection  */
    if ( ( bytes = ch_get_bytes_readable (connection->channel) ) < 0 )
    {
	(void) exit (RV_SYS_ERROR);
    }
    if (bytes < EN_IDEA_BLOCK_SIZE * 2)
    {
	/*  Not enough info was sent  */
	(void) pio_write_string (connection->channel,
				 "Bad IDEA block message length");
	(void) fprintf (stderr,
			"Only: %d bytes of connection IDEA block data ",
			bytes);
	(void) fprintf (stderr,
			"sent\n%d bytes are required: connection closed\n",
			EN_IDEA_BLOCK_SIZE * 2);
	return (FALSE);
    }
    if (ch_read (connection->channel, idea_blocks, EN_IDEA_BLOCK_SIZE * 2) <
	EN_IDEA_BLOCK_SIZE * 2) return (FALSE);
    if ( !m_cmp (idea_blocks + EN_IDEA_BLOCK_SIZE, connection->idea_block,
		 EN_IDEA_BLOCK_SIZE) )
    {
	/*  Client is not allowed  */
	(void) fprintf (stderr,
			"WARNING: connection attempt failed for protocol: %s\n",
			connection->protocol_name);
	(void) pio_write_string (connection->channel,
				 "Connection not authorised");
	if ( !ch_flush (connection->channel) ) return (FALSE);
	return (FALSE);
    }
    if (ch_write (connection->channel, idea_blocks, EN_IDEA_BLOCK_SIZE) <
	EN_IDEA_BLOCK_SIZE) return (FALSE);
    if ( !ch_flush (connection->channel) ) return (FALSE);
    if (connection->pri_encryption_converter != NULL)
    {
	ch_unregister_converter (connection->pri_encryption_converter);
	connection->pri_encryption_converter = NULL;
    }
    authinfo = get_authinfo (connection->protocol_name);
    switch (authinfo->security_type)
    {
      case SECURITY_TYPE_KEY:
	ch_unregister_converter (connection->pro_encryption_converter);
	connection->pro_encryption_converter = NULL;
	break;
      case SECURITY_TYPE_IDEA:
      case SECURITY_TYPE_PGPuserID_IDEA:
	if ( ch_test_for_local_connection (connection->channel) )
	{
	    ch_unregister_converter (connection->pro_encryption_converter);
	    connection->pro_encryption_converter = NULL;
	}
	break;
    }
    connection->verified_protocol_security = TRUE;
    /*  Write verification message back to client  */
    (void) pio_write_string (connection->channel, (char *) NULL);
    /*  Send module name back to client  */
    (void) pio_write_string (connection->channel, module_name);
    if ( !ch_flush (connection->channel) ) return (FALSE);
    return (TRUE);
}   /*  End Function verify_protocol_security  */

/*  Function to process client connection input  */

static flag client_connection_input_func (Channel channel, void **info)
/*  This routine is called when new input occurs on a channel.
    The channel object is given by  channel  .
    An arbitrary pointer may be written to the storage pointed to by  info
    The pointer written here will persist until the channel is unmanaged
    (or a subsequent callback routine changes it).
    The routine returns TRUE if the channel is to remain managed and
    open, else it returns FALSE (indicating that the channel is to be
    unmanaged and closed).
*/
{
    int bytes;
    unsigned long old_read_pos, new_read_pos, dummy;
    Connection connection;
    static char function_name[] = "client_connection_input_func";

    connection = (Connection) *info;
    VERIFY_CONNECTION (connection);
    if (connection->read_func == NULL)
    {
	(void) fprintf (stderr,
			"Input on \"%s\" connection not being read (no callback)\n",
			connection->protocol_name);
	a_prog_bug (function_name);
    }
    bytes = 1;
    if ( !ch_tell (channel, &old_read_pos, &dummy) )
    {
	(void) exit (RV_SYS_ERROR);
    }
    while (bytes > 0)
    {
	if ( !(*connection->read_func) (connection, &connection->info) )
	{
	    return (FALSE);
	}
	if ( !ch_tell (channel, &new_read_pos, &dummy) )
	{
	    (void) exit (RV_SYS_ERROR);
	}
	if (new_read_pos <= old_read_pos)
	{
	    (void) fprintf (stderr,
			    "Connection read callback for protocol: \"%s\" not draining\n",
			    connection->protocol_name);
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
	if (strcmp (protocol_name, entry->protocol_name) == 0)
	{
	    /*  Found it!  */
	    return (entry);
	}
	entry = entry->next;
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
	if (strcmp (protocol_name, entry->protocol_name) == 0)
	{
	    /*  Found it!  */
	    return (entry);
	}
	entry = entry->next;
    }
    /*  Not found  */
    return (NULL);
}   /*  End Function get_client_protocol_info  */

static void connection_close_func (Channel channel, void *info)
/*  This routine is called when a channel closes.
    The channel object is given by  channel  .
    The arbitrary pointer for the channel will be pointed to by  info  .
    Any unread buffered data in the channel will be lost upon closure. The
    call to this function is the last chance to read this buffered data.
    The routine returns nothing.
*/
{
    Connection connection;
    static char function_name[] = "connection_close_func";

    /*  Extract and verify connection  */
    connection = (Connection) info;
    VERIFY_CONNECTION (connection);
    dealloc_connection (connection);
}   /*  End Function connection_close_func  */

static void dealloc_connection (Connection connection)
/*  This routine will deallocate a connection object.
    The routine will NOT unmanage or close the channel associated with the
    connection.
    The close function registered for the connection will be called prior to
    deallocation.
    The connection object must be given by  connection  .
    The routine returns nothing.
*/
{
    extern Channel cm_channel;
    extern char *sys_errlist[];
    static char function_name[] = "dealloc_connection";

    VERIFY_CONNECTION (connection);
    /*  First decrement connection count and remove from list,
	otherwise the close_func callback could query for a connection and
	get this one!  */
    if (connection->connection_count != NULL)
    {
	/*  Decrement count of connections for this protocol  */
	--*connection->connection_count;
    }
    /*  Remove connection from list  */
    if (connection->prev == NULL)
    {
	/*  There is no previous connection: this is the first in the list  */
	*connection->list_start = connection->next;
    }
    else
    {
	/*  There is a previous connection  */
	connection->prev->next = connection->next;
    }
    if (connection->next != NULL)
    {
	/*  There is a next connection  */
	connection->next->prev = connection->prev;
    }
    /*  Call registered closure routine if it exists  */
    if (connection->close_func!= NULL)
    {
	(*connection->close_func) (connection, connection->info);
    }
    connection->magic_number = 0;
    if (connection->module_name != NULL) m_free (connection->module_name);
    if ( connection->client && (cm_channel != NULL) )
    {
	/*  Have client connection and connected to
	    Connection Management tool  */
	if ( !pio_write32 (cm_channel, CM_LIB_CONNECTION_CLOSED) )
	{
	    (void) fprintf (stderr, "Error writing command value\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if ( !pio_write32 (cm_channel,
			   (unsigned long) connection & 0xffffffff) )
	{
	    (void) fprintf (stderr, "Error writing Connection ID\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if ( !ch_flush (cm_channel) )
	{
	    (void) fprintf (stderr, "Error flushing channel\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
    }
    m_clear (connection->idea_block, EN_IDEA_BLOCK_SIZE);
    m_free ( (char *) connection );
}   /*  End Function dealloc_connection  */

static struct auth_password_list_type *get_authinfo (char *protocol)
/*  This routine will determine the authorisation required to gain access to a
    Karma server using the protocol given by  protocol  .
    The routine will write the security type to the storage pointed to by
    security_type  .
    The routine returns the authorisation information.
*/
{
#ifdef COMMUNICATIONS_AVAILABLE
    struct auth_password_list_type *entry;
    struct auth_password_list_type *password_list;
/*
    static char function_name[] = "get_authinfo";
*/

    password_list = get_password_list ();
    if (password_list == NULL)
    {
	/*  No list: no passwords required  */
	return (NULL);
    }
    for (entry = password_list; entry != NULL; entry = entry->next)
    {
	if (strcmp (protocol, entry->protocol_name) == 0)
	{
	    return (entry);
	}
    }
    return (NULL);

#else  /*  COMMUNICATIONS_AVAILABLE  */
    (void) fprintf (stderr,
		    "Operating system does not support communications\n");
    (void) exit (RV_UNDEF_ERROR);
#endif  /*  COMMUNICATIONS_AVAILABLE  */
}   /*  End Function get_authinfo  */

static struct auth_password_list_type *get_password_list ()
/*  [PURPOSE] This routine will process the authorisation file.
    [RETURNS] The password list.
*/
{
#ifdef COMMUNICATIONS_AVAILABLE
    Channel channel;
    flag need_bin_password;
    char *home;
    char *p, *security_name;
    char *password = NULL;
    struct auth_password_list_type *entry;
    char auth_file[STRING_LENGTH];
    char txt[STRING_LENGTH + 1];
    extern char *sys_errlist[];
    static flag read_list = FALSE;
    static struct auth_password_list_type *password_list = NULL;
    static char function_name[] = "get_password_list";

    if (read_list) return (password_list);
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
    if ( ( channel = ch_open_file (auth_file, "r") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening file: %s\n", auth_file);
	(void) exit (RV_CANNOT_OPEN);
    }
    while ( chs_get_line (channel, txt, STRING_LENGTH) )
    {
	/*  Not end of file  */
	need_bin_password = FALSE;
	if ( ( entry = (struct auth_password_list_type *)
	      m_alloc (sizeof *entry) ) == NULL )
	{
	    m_abort (function_name, "protocol password entry");
	}
	entry->next = NULL;
	if ( ( entry->protocol_name = ex_word (txt, &p) ) == NULL )
	{
	    /*  Error converting line  */
	    (void) ch_close (channel);
	    (void) fprintf (stderr, "Error in file: %s\n", auth_file);
	    (void) exit (RV_BAD_DATA);
	}
	if ( ( security_name = ex_word (p, &p) ) == NULL )
	{
	    (void) fprintf (stderr, "Missing security type\n");
	    (void) ch_close (channel);
	    (void) fprintf (stderr, "Error in file: %s\n", auth_file);
	    (void) exit (RV_BAD_DATA);
	}
	if (strcmp (security_name, SECURITY_STRING_KEY) == 0)
	{
	    entry->security_type = SECURITY_TYPE_KEY;
	}
	else if (strcmp (security_name, SECURITY_STRING_IDEA) == 0)
	{
	    entry->security_type = SECURITY_TYPE_IDEA;
	}
	else if (strcmp (security_name, SECURITY_STRING_PGPuserID_IDEA) == 0)
	{
	    entry->security_type = SECURITY_TYPE_PGPuserID_IDEA;
	}
	else if (strcmp (security_name, SECURITY_STRING_DROP_ENCRYPTION) == 0)
	{
	    entry->security_type = SECURITY_TYPE_DROP_ENCRYPTION;
	}
	else
	{
	    (void) fprintf (stderr, "Illegal security type: \"%s\"\n",
			    security_name);
	    (void) ch_close (channel);
	    (void) fprintf (stderr, "Error in file: %s\n", auth_file);
	    (void) exit (RV_BAD_DATA);
	}
	switch (entry->security_type)
	{
	  case SECURITY_TYPE_IDEA:
	  case SECURITY_TYPE_KEY:
	    setup_prng ();
	    need_bin_password = TRUE;
	    if ( ( password = ex_word (p, &p) ) == NULL )
	    {
		m_abort (function_name, "password");
	    }
	    if ( (int) strlen (password) / 2 <
		EN_IDEA_KEY_SIZE + EN_IDEA_BLOCK_SIZE )
	    {
		(void) fprintf (stderr, "IDEA password too short\n");
		(void) ch_close (channel);
		(void) fprintf (stderr, "Error in file: %s\n",
				auth_file);
		(void) exit (RV_BAD_DATA);
	    }
	    break;
	  case SECURITY_TYPE_PGPuserID_IDEA:
	    setup_prng ();
	    if ( ( password = ex_word (p, &p) ) == NULL )
	    {
		m_abort (function_name, "password");
	    }
	    break;
	  case SECURITY_TYPE_DROP_ENCRYPTION:
	    password = NULL;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal security type: %u\n",
			    entry->security_type);
	    a_prog_bug (function_name);
	    break;
	}
	if (need_bin_password)
	{
	    /*  Convert password in file from hex format to binary format  */
	    convert_password_to_bin ( (unsigned char *) entry->password,
				     password );
	    m_clear ( password, strlen (password) );
	    m_free (password);
	}
	else
	{
	    if (password != NULL)
	    {
		strcpy (entry->password, password);
		m_clear ( password, strlen (password) );
		m_free (password);
	    }
	}
	/*  Add entry to list  */
	entry->next = password_list;
	password_list = entry;
    }
    /*  End of file  */
    (void) ch_close (channel);
    m_clear (txt, STRING_LENGTH);
    return (password_list);

#else  /*  COMMUNICATIONS_AVAILABLE  */
    (void) fprintf (stderr,
		    "Operating system does not support communications\n");
    (void) exit (RV_UNDEF_ERROR);
#endif  /*  COMMUNICATIONS_AVAILABLE  */
}   /*  End Function get_password_list  */

static void convert_password_to_bin (unsigned char *binpassword,
				     CONST char *asciipassword)
/*  [PURPOSE] This routine will convert an ASCII password to a binary password.
    <binpassword> Pointer to the storage for the binary password.
    <asciipassword> Pointer to the ASCII password.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    int byte[2];

    m_clear ( (char *) binpassword, AUTH_PASSWORD_LENGTH );
    for (count = 0; count < AUTH_PASSWORD_LENGTH; ++count)
    {
	byte[0] = tolower (asciipassword[count * 2]);
	byte[1] = tolower (asciipassword[count * 2 + 1]);
	if ( (byte[0] == '\0') || (byte[1] == '\0') ) return;
	if ( !isxdigit (byte[0]) || !isxdigit (byte[1]) )
	{
	    (void) fprintf (stderr,
			    "Password: \"%s\" contains non-hex character\n",
			    asciipassword);
	    (void) fprintf (stderr, "Error in authorisation file\n");
	    (void) exit (RV_BAD_DATA);
	}
	if (byte[0] > '9') byte[0] = byte[0] - 'a' + 0x0a;
	else byte[0] -= '0';
	if (byte[1] > '9') byte[1] = byte[1] - 'a' + 0x0a;
	else byte[1] -= '0';
	binpassword[count] = (byte[0] << 4) + byte[1];
    }
    byte[0] = 0;
    byte[1] = 0;
}   /*  End Function convert_password_to_bin  */

#ifdef dummy
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
    for (auth_entry = protocol_info->authorised_hosts; auth_entry != NULL;
	 auth_entry = auth_entry->next)
    {
	if (inet_addr == auth_entry->host_ptr->inet_addr)
	{
	    /*  Found it  */
	    return (TRUE);
	}
    }
    /*  Host is not authorised  */
    return (FALSE);
}   /*  End Function check_host_access  */
#endif

static char *write_protocol (Channel channel, char *protocol_name,
			     unsigned int version)
/*  This routine will write an authentication message for a specified protocol
    to a channel.
    The channel object must be given by  channel  .
    The protocol name must be pointed to by  protocol_name  .
    The version number for the protocol must be given by  version  .
    The routine returns the module name of the server on success, else it
    returns NULL.
*/
{
    ChConverter pri_converter = NULL;
    ChConverter pro_converter = NULL;
    unsigned int length;
    char *message;
    char protocol_buffer[PROTOCOL_NAME_LENGTH];
    char idea_block_client[EN_IDEA_BLOCK_SIZE];
    char idea_block_serv[EN_IDEA_BLOCK_SIZE];
    struct auth_password_list_type *authinfo;
    extern char module_name[STRING_LENGTH + 1];
    extern char *sys_errlist[];
    static char function_name[] = "write_protocol";

    /*  Write magic number  */
    if (pio_write32 (channel, CONN_MAGIC_NUMBER) != TRUE)
    {
	a_func_abort (function_name, "Error writing magic number to channel");
	return (NULL);
    }
    /*  Write revision number  */
    if (pio_write32 (channel, REVISION_NUMBER) != TRUE)
    {
	a_func_abort (function_name,
		      "Error writing revision number to channel");
	return (NULL);
    }
    if ( !ch_flush (channel) )
    {
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    /*  Get primary password  */
    if ( ( authinfo = get_authinfo (RAW_PROTOCOL) ) != NULL )
    {
	if (authinfo->security_type == SECURITY_TYPE_DROP_ENCRYPTION)
	{
	    a_func_abort (function_name,
			  "security type: \"drop-encryption\" not valid for PRIMARY_KEY\n");
	    exit (RV_BAD_DATA);
	}
	if ( ( pri_converter = get_encryption (channel, authinfo, FALSE,
					       idea_block_client) ) == NULL )
	{
	    return (NULL);
	}
	if (ch_read (channel, idea_block_serv, EN_IDEA_BLOCK_SIZE) <
	    EN_IDEA_BLOCK_SIZE) return (NULL);
	if (ch_write (channel, idea_block_serv, EN_IDEA_BLOCK_SIZE) <
	    EN_IDEA_BLOCK_SIZE) return (NULL);
	if ( !ch_flush (channel) )
	{
	    (void) fprintf (stderr, "Error flushing channel\t%s\n",
			    sys_errlist[errno]);
	    return (NULL);
	}
	if (ch_read (channel, idea_block_serv, EN_IDEA_BLOCK_SIZE) <
	    EN_IDEA_BLOCK_SIZE) return (NULL);
	if ( !m_cmp (idea_block_client, idea_block_serv,
		     EN_IDEA_BLOCK_SIZE) ) return (NULL);
	switch (authinfo->security_type)
	{
	  case SECURITY_TYPE_KEY:
	    ch_unregister_converter (pri_converter);
	    pri_converter = NULL;
	    break;
	  case SECURITY_TYPE_IDEA:
	  case SECURITY_TYPE_PGPuserID_IDEA:
	    if ( ch_test_for_local_connection (channel) )
	    {
		ch_unregister_converter (pri_converter);
		pri_converter = NULL;
	    }
	    break;
	}
    }
    /*  Write protocol name  */
    (void) strncpy (protocol_buffer, protocol_name, PROTOCOL_NAME_LENGTH);
    if (ch_write (channel, protocol_buffer, PROTOCOL_NAME_LENGTH)
	< PROTOCOL_NAME_LENGTH)
    {
	(void) fprintf (stderr, "Error writing protocol name\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    /*  Write protocol version number  */
    if (pio_write32 (channel, (unsigned long) version) != TRUE)
    {
	a_func_abort (function_name,
		      "Error writing protocol version number to channel");
	return (NULL);
    }
    if ( !ch_flush (channel) )
    {
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    authinfo = get_authinfo (protocol_name);
    if ( (authinfo != NULL) &&
	(authinfo->security_type == SECURITY_TYPE_DROP_ENCRYPTION) )
    {
	if (pri_converter != NULL) ch_unregister_converter (pri_converter);
	authinfo = NULL;
    }
    if (authinfo != NULL)
    {
	if ( ( pro_converter = get_encryption (channel, authinfo, FALSE,
					       idea_block_client) ) == NULL )
	{
	    return (NULL);
	}
	if (ch_read (channel, idea_block_serv, EN_IDEA_BLOCK_SIZE) <
	    EN_IDEA_BLOCK_SIZE) return (NULL);
	if (ch_write (channel, idea_block_serv, EN_IDEA_BLOCK_SIZE) <
	    EN_IDEA_BLOCK_SIZE) return (NULL);
	if ( !ch_flush (channel) )
	{
	    (void) fprintf (stderr, "Error flushing channel\t%s\n",
			    sys_errlist[errno]);
	    return (NULL);
	}
	if (ch_read (channel, idea_block_serv, EN_IDEA_BLOCK_SIZE) <
	    EN_IDEA_BLOCK_SIZE) return (NULL);
	if ( !m_cmp (idea_block_client, idea_block_serv,
		     EN_IDEA_BLOCK_SIZE) ) return (NULL);
	if (pri_converter != NULL) ch_unregister_converter (pri_converter);
	switch (authinfo->security_type)
	{
	  case SECURITY_TYPE_KEY:
	    ch_unregister_converter (pro_converter);
	    pro_converter = NULL;
	    break;
	  case SECURITY_TYPE_IDEA:
	  case SECURITY_TYPE_PGPuserID_IDEA:
	    if ( ch_test_for_local_connection (channel) )
	    {
		ch_unregister_converter (pro_converter);
		pro_converter = NULL;
	    }
	    break;
	}
    }
    /*  Get returned message  */
    if ( ( message = pio_read_string (channel, &length) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading return message\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    if (length > 0)
    {
	(void) fprintf (stderr, "\"%s\" connection refused. Reason: %s\n",
			protocol_name, message);
	m_free (message);
	return (NULL);
    }
    m_free (message);
    /*  Send module name to server  */
    (void) pio_write_string (channel, module_name);
    if ( !ch_flush (channel) )
    {
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
	return (NULL);
    }
    /*  Get returned module name  */
    if ( ( message = pio_read_string (channel, (unsigned int *) NULL) )
	== NULL )
    {
	(void) fprintf (stderr, "Error reading server module name\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (cm_channel);
	exit (RV_UNDEF_ERROR);
    }
    return (message);
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
    extern char *sys_errlist[];
    static unsigned int buffer_length = 0;
    static char *buffer = NULL;
    static char function_name[] = "respond_to_ping_server_from_client";

    VERIFY_CONNECTION (connection);
    channel = connection->channel;
    if ( ( bytes_readable = ch_get_bytes_readable (channel) ) < 0 )
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
    static char function_name[] = "register_server_exit";

    VERIFY_CONNECTION (connection);
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

    if (list != NULL) VERIFY_CONNECTION (list);
    for (conn_count = 0, curr_entry = list; curr_entry != NULL;
	 curr_entry = curr_entry->next)
    {
	if (strcmp (protocol_name, curr_entry->protocol_name) == 0)
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
    Channel stdio;
    int x;
    int y;
    int out_fd;
    unsigned int port_number;
    unsigned long host_addr;
    char *control_command;
    char *char_ptr;
    char *message;
    char *keyword;
    char my_hostname[STRING_LENGTH];
    char txt[STRING_LENGTH];
    extern Channel cm_channel;
    extern flag (*manage_channel) ();
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
			  S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH) ) < 0 )
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
    /*  Get the fully qualified hostname  */
    if ( !r_get_fq_hostname (my_hostname, STRING_LENGTH) ) exit (RV_SYS_ERROR);
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
    host_addr = ex_uint (char_ptr, &char_ptr);
    port_number = ex_uint (char_ptr, &char_ptr);
    x = ex_int (char_ptr, &char_ptr);
    y = ex_int (char_ptr, &char_ptr);
    if (host_addr == 0) (void) strcpy (my_hostname, "localhost");
#ifdef OS_VXMVX
    /*  Append ":vx" to hostname  */
    (void) strcat (my_hostname, ":vx");
#endif
    /*  Connect to Connection Management tool with control protocol  */
    /*  Open connection  */
    if ( ( cm_channel = ch_open_connection (host_addr, port_number) ) == NULL )
    {
	(void) fprintf (stderr,
			"Could not open control connection to Connection Management tool\n");
	(void) fprintf (stderr, "on host addr: %lu  port: %u\n",
			host_addr, port_number);
	exit (RV_UNDEF_ERROR);
    }
    /*  Send protocol info  */
    if ( ( message = write_protocol (cm_channel, "conn_mngr_control", 
				     CM_PROTOCOL_VERSION) )
	== NULL )
    {
	(void) fprintf (stderr, "Error writing authentication information\n");
	(void) ch_close (cm_channel);
	exit (RV_SYS_ERROR);
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
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
	exit (RV_WRITE_ERROR);
    }
    /*  Connect to Connection Management tool with standard IO protocol  */
    /*  Open connection  */
    if ( ( stdio = ch_open_connection (host_addr, port_number) ) == NULL )
    {
	(void) fprintf (stderr,
			"Could not open stdio connection to Connection Management tool");
	exit (RV_UNDEF_ERROR);
    }
    /*  Send protocol info  */
    if ( ( message = write_protocol (stdio, "conn_mngr_stdio",
				     CM_PROTOCOL_VERSION) ) == NULL )
    {
	(void) fprintf (stderr, "Error writing authentication information\n");
	(void) ch_close (stdio);
	exit (RV_SYS_ERROR);
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
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
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
    extern Channel cm_channel;
    extern void (*exit_schedule_function) ();
    extern void (*quiescent_function) ();
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
	(void) fprintf (stderr,
			"%s: illegal command value from CM tool: %lu\n",
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
    /*  Used to call _exit() but because this leaves sproc() processes lying
	around under IRIX, changed to exit(). Hope this doesn't cause a nasty
	loop.  */
    exit (RV_OK);
}   /*  End Function cm_close_func  */

static void setup_prng ()
/*  This routine will setup the cryptographically strong random number
    generator(s). If already set up, the routine does nothing.
    The routine returns nothing.
*/
{
    KCallbackFunc cbk;
    extern RandPool main_randpool;
    static char function_name[] = "__conn_setup_prng";

    if (main_randpool == NULL)
    {
	if ( ( main_randpool = rp_create (512, 16, 64, md_md5_transform) )
	    == NULL )
	{
	    m_abort (function_name, "random pool of bytes");
	}
	cbk = ch_tap_io_events (rp_add_time_noise, main_randpool);
	rp_register_destroy_func (main_randpool, randpool_destroy_func, cbk);
    }
}   /*  End Function setup_prng  */

static void randpool_destroy_func (RandPool rp, void *info)
/*  This routine is called when a random pool is destroyed.
    The random pool will be given by  rp  .
    The arbitrary information pointer will be given by  info  .
    The routine returns nothing.
*/
{
    c_unregister_callback ( (KCallbackFunc) info );
}   /*  End Function randpool_destroy_func  */

static ChConverter get_encryption (Channel channel,
				   struct auth_password_list_type *authinfo,
				   flag server, char *rand_block)
/*  [PURPOSE] The routine will set the appropriate channel encryption for a
    connection.
    <channel> The channel for the connection.
    <authinfo> A pointer to the authorisation information for the desired
    protocol.
    <server> If TRUE, the connection is for a server, else a client.
    [RETURNS] An encryption converter function, NULL on failure.
*/
{
    ChConverter converter = NULL;
    flag idea_encryption = FALSE;
    flag clear_session = TRUE;
    unsigned int ciphertext_length, plaintext_length;
    unsigned long data;
    char *ciphertext, *plaintext;
    char *session_key = NULL;  /*  Initialised to keep compiler happy  */
    char *session_iv = NULL;   /*  Initialised to keep compiler happy  */
    char *recipient;
    char idea_session[EN_IDEA_KEY_SIZE + EN_IDEA_BLOCK_SIZE];
    extern RandPool main_randpool;
    extern char *sys_errlist[];
    static char function_name[] = "get_encryption";

    if ( (authinfo == NULL) || (rand_block == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    switch ( authinfo->security_type )
    {
      case SECURITY_TYPE_KEY:
      case SECURITY_TYPE_IDEA:
	idea_encryption = TRUE;
	session_key = authinfo->password;
	session_iv = authinfo->password + EN_IDEA_KEY_SIZE;
	clear_session = FALSE;
	break;
      case SECURITY_TYPE_PGPuserID_IDEA:
	idea_encryption = TRUE;
	if (server)
	{
	    rp_get_bytes (main_randpool, (unsigned char *) idea_session,
			  EN_IDEA_KEY_SIZE + EN_IDEA_BLOCK_SIZE);
	    recipient = authinfo->password;
	    (void) fprintf (stderr, "recipient: \"%s\"\n",
			    recipient);
	    if ( ( ciphertext =
		  pgp_encrypt (idea_session,
			       EN_IDEA_KEY_SIZE + EN_IDEA_BLOCK_SIZE,
			       (CONST char **) &recipient, 1,
			       &ciphertext_length, FALSE) ) == NULL )
	    {
		m_abort (function_name, "encrypted IDEA session key");
	    }
	    if ( !pio_write32 (channel, ciphertext_length) )
	    {
		(void) fprintf (stderr, "Error writing length\t%s\n",
				sys_errlist[errno]);
		m_clear (idea_session, EN_IDEA_KEY_SIZE + EN_IDEA_BLOCK_SIZE);
		m_clear (ciphertext, ciphertext_length);
		m_free (ciphertext);
		return (NULL);
	    }
	    if (ch_write (channel, ciphertext, ciphertext_length) <
		ciphertext_length)
	    {
		(void)fprintf(stderr,
			      "Error writing encrypted IDEA session key\t%s\n",
			      sys_errlist[errno]);
		m_clear (idea_session, EN_IDEA_KEY_SIZE + EN_IDEA_BLOCK_SIZE);
		m_clear (ciphertext, ciphertext_length);
		m_free (ciphertext);
		return (NULL);
	    }
	    m_clear (ciphertext, ciphertext_length);
	    m_free (ciphertext);
	    (void) fprintf (stderr, "Wrote encrypted IDEA session: %x\n",
			    *(unsigned int *) idea_session);
	}
	else
	{
	    /*  Client  */
	    if ( !pio_read32 (channel, &data) )
	    {
		(void) fprintf (stderr, "Error reading length\t%s\n",
				sys_errlist[errno]);
		return (NULL);
	    }
	    ciphertext_length = data;
	    if ( ( ciphertext = m_alloc (ciphertext_length) ) == NULL )
	    {
		m_abort (function_name, "ciphertext");
	    }
	    if (ch_read (channel, ciphertext, ciphertext_length) <
		ciphertext_length)
	    {
		(void) fprintf (stderr, "Error reading length\t%s\n",
				sys_errlist[errno]);
		m_free (ciphertext);
		return (NULL);
	    }
	    if ( ( plaintext = pgp_decrypt (ciphertext, ciphertext_length,
					    &plaintext_length) ) == NULL )
	    {
		(void) fprintf (stderr, "Error decrypting IDEA session key\n");
		m_free (ciphertext);
		return (NULL);
	    }
	    if (plaintext_length != EN_IDEA_KEY_SIZE + EN_IDEA_BLOCK_SIZE)
	    {
		(void) fprintf(stderr,
			       "Decrypted session key length: %u is not: %u\n",
				plaintext_length,
			       EN_IDEA_KEY_SIZE + EN_IDEA_BLOCK_SIZE);
		m_clear (plaintext, plaintext_length);
		m_free (plaintext);
		return (NULL);
	    }
	    m_copy (idea_session, plaintext, plaintext_length);
	    m_clear (plaintext, plaintext_length);
	    m_free (plaintext);
	    (void) fprintf (stderr, "Read encrypted IDEA session: %x\n",
			    *(unsigned int *) idea_session);
	}
	session_key = idea_session;
	session_iv = idea_session + EN_IDEA_KEY_SIZE;
	break;
      default:
	(void) fprintf (stderr, "Illegal security type: %u\n",
			authinfo->security_type);
	a_prog_bug (function_name);
	break;
    }
    if (idea_encryption)
    {
	if ( ( converter = cen_idea (channel, session_key, session_iv,
				     session_key, session_iv, clear_session) )
	    == NULL )
	{
	    m_abort (function_name, "IDEA encryption");
	}
	/*  Send some random data to get around constant IV problem  */
	rp_get_bytes (main_randpool, (unsigned char *) rand_block,
		      EN_IDEA_BLOCK_SIZE);
	if (ch_write (channel, rand_block, EN_IDEA_BLOCK_SIZE) <
	    EN_IDEA_BLOCK_SIZE)
	{
	    (void) fprintf (stderr, "Error writing random block\t%s\n",
			    sys_errlist[errno]);
	    m_clear (rand_block, EN_IDEA_BLOCK_SIZE);
	    return (NULL);
	}
    }
    return (converter);
}   /*  End Function get_encryption  */


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
    (void) get_password_list ();
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
    static char function_name[] = "conn_register_server_protocol";

    if ( (int) strlen (protocol_name) >= PROTOCOL_NAME_LENGTH )
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
    if ( ( entry->protocol_name = st_dup (protocol_name) ) == NULL )
    {
	m_abort (function_name, "protocol name");
    }
    entry->version = version;
    entry->max_connections = max_connections;
    entry->connection_count = 0;
    entry->open_func = open_func;
    entry->read_func = read_func;
    entry->close_func = close_func;
    entry->authorised_hosts = NULL;
    entry->next = NULL;
    if (serv_protocol_list == NULL)
    {
	/*  Create protocol list  */
	serv_protocol_list = entry;
    }
    else
    {
	/*  Append entry to protocol list  */
	for (last_entry = serv_protocol_list; last_entry->next != NULL;
	     last_entry = last_entry->next);
	last_entry->next = entry;
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
    static char function_name[] = "conn_register_client_protocol";

    if ( (int) strlen (protocol_name) >= PROTOCOL_NAME_LENGTH )
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
    if ( ( entry->protocol_name = st_dup (protocol_name) ) == NULL )
    {
	m_abort (function_name, "protocol name");
    }
    entry->version = version;
    entry->max_connections = max_connections;
    entry->connection_count = 0;
    entry->validate_func = validate_func;
    entry->open_func = open_func;
    entry->read_func = read_func;
    entry->close_func = close_func;
    entry->next = NULL;
    if (client_protocol_list == NULL)
    {
	/*  Create protocol list  */
	client_protocol_list = entry;
    }
    else
    {
	/*  Append entry to protocol list  */
	for (last_entry = client_protocol_list; last_entry->next != NULL;
	     last_entry = last_entry->next);
	last_entry->next = entry;
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

    VERIFY_CONNECTION (connection);
    return ( connection->channel );
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
    unsigned long host_addr;
    Connection new_connection;
    Connection last_entry;
    Channel channel;
    void *info = NULL;
    struct client_protocol_list_type *protocol_info;
    extern Channel cm_channel;
    extern Connection client_connections;
    extern struct client_protocol_list_type *client_protocol_list;
    extern flag (*manage_channel) ();
    extern void (*unmanage_channel) ();
    extern char *sys_errlist[];
    static char function_name[] = "conn_attempt_connection";

    if ( (int) strlen (protocol_name) >= PROTOCOL_NAME_LENGTH )
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
    if ( ( protocol_info = get_client_protocol_info (protocol_name) ) == NULL )
    {
	(void) fprintf (stderr, "Protocol: \"%s\" not supported\n",
			protocol_name);
	return (FALSE);
    }
    if ( (protocol_info->max_connections > 0) &&
	(protocol_info->connection_count >= protocol_info->max_connections) )
    {
	(void) fprintf (stderr,
			"Maximum number of client connections reached for protocol: \"%s\"\n",
			protocol_name);
	return (FALSE);
    }
    /*  Validate  */
    if (protocol_info->validate_func != NULL)
    {
	if ( (*protocol_info->validate_func) (&info) != TRUE )
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
    new_connection->client = TRUE;
    new_connection->protocol_name = protocol_info->protocol_name;
    new_connection->connection_count = &protocol_info->connection_count;
    new_connection->read_func = protocol_info->read_func;
    new_connection->close_func = protocol_info->close_func;
    new_connection->info = info;
    new_connection->prev = NULL;
    new_connection->next = NULL;
    new_connection->list_start = &client_connections;
    /*  Open connection  */
    if ( ( channel = ch_open_connection (host_addr, port_number) ) == NULL )
    {
	m_free ( (char *) new_connection );
	return (FALSE);
    }
    new_connection->channel = channel;
    /*  Send protocol info  */
    if ( ( new_connection->module_name =
	  write_protocol (channel, protocol_name,
			  protocol_info->version) ) == NULL )
    {
	(void) fprintf (stderr, "Error writing authentication information\n");
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
    ++*new_connection->connection_count;
    new_connection->magic_number = OBJECT_MAGIC_NUMBER;
    if (protocol_info->open_func != NULL)
    {
	if ( !(*protocol_info->open_func) (new_connection,
					   &new_connection->info) )
	{
	    (*unmanage_channel) (channel);
	    (void) ch_close (channel);
	    --*new_connection->connection_count;
	    new_connection->magic_number = 0;
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
	for (last_entry = client_connections; last_entry->next != NULL;
	     last_entry = last_entry->next);
	/*  Append entry  */
	last_entry->next = new_connection;
	new_connection->prev = last_entry;
    }
    /*  Drain any input on new connection  */
    if ( ( bytes = ch_get_bytes_readable (channel) ) < 0 )
    {
	(void) exit (RV_SYS_ERROR);
    }
    if (bytes > 0)
    {
	if (new_connection->read_func == NULL)
	{
	    (void) fprintf (stderr,
			    "Input on new connection not being read\n");
	    a_prog_bug (function_name);
	}
	if ( !client_connection_input_func (channel,
					    (void **) &new_connection) )
	{
	    dealloc_connection (new_connection);
	    (*unmanage_channel) (channel);
	    (void) ch_close (channel);
	    return (FALSE);
	}
    }
    if (cm_channel != NULL)
    {
	/*  Connected to Connection Management tool  */
	if ( !pio_write32 (cm_channel, CM_LIB_NEW_CONNECTION) )
	{
	    (void) fprintf (stderr, "Error writing command value\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if ( !pio_write_string (cm_channel, protocol_name) )
	{
	    (void) fprintf (stderr, "Error writing protocol name\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if ( !pio_write32 (cm_channel, host_addr) )
	{
	    (void) fprintf (stderr,
			    "Error writing host Internet address\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if ( !pio_write32 (cm_channel, (unsigned long) port_number) )
	{
	    (void) fprintf (stderr, "Error writing port number\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if ( !pio_write32 (cm_channel,
			   (unsigned long) new_connection & 0xffffffff) )
	{
	    (void) fprintf (stderr, "Error writing Connection ID\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_WRITE_ERROR);
	}
	if (ch_flush (cm_channel) != TRUE)
	{
	    (void) fprintf (stderr, "Error flushing channel\t%s\n",
			    sys_errlist[errno]);
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

    VERIFY_CONNECTION (connection);
    if (unmanage_channel == NULL)
    {
	(void) fprintf (stderr, "Channel managers not registered\n");
	a_prog_bug (function_name);
    }
    channel = connection->channel;
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
	    (void) fprintf (stderr, "Error flushing channel\t%s\n",
			    sys_errlist[errno]);
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
/*
    static char function_name[] = "conn_get_num_serv_connections";
*/

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
/*
    static char function_name[] = "conn_get_num_client_connections";
*/

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

    VERIFY_CONNECTION (connection);
    return (connection->info);
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

    VERIFY_CONNECTION (connection);
    if (connection->module_name == NULL)
    {
	(void) fprintf (stderr, "Invalid connection module_name\n");
	a_prog_bug (function_name);
    }
    return (connection->module_name);
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

#ifndef OS_VXMVX
/*PUBLIC_FUNCTION*/
char **conn_extract_protocols ()
/*  This routine will extract all the supported protocols and produces a
    dynamically allocated array of strings which may be later displayed.
    The routine returns a pointer to a dynamically allocated array of
    dynamically allocated strings on success, else it returns NULL. The end
    of the array is marked with a NULL string pointer.
*/
{
    unsigned int num_protocols;
    char **strings;
    char txt[STRING_LENGTH];
    char tmp1[STRING_LENGTH];
    char tmp2[STRING_LENGTH];
    struct client_protocol_list_type *client_entry;
    struct serv_protocol_list_type *serv_entry;
    extern struct client_protocol_list_type *client_protocol_list;
    extern struct serv_protocol_list_type *serv_protocol_list;
    static char function_name[] = "conn_extract_protocols";

    /*  Find total number of protocols  */
    /*  Loop through all server protocols  */
    for (serv_entry = serv_protocol_list, num_protocols = 0;
	 serv_entry != NULL;
	 serv_entry = serv_entry->next, ++num_protocols);
    /*  Loop through all client protocols  */
    for (client_entry = client_protocol_list; client_entry != NULL;
	 client_entry = client_entry->next)
    {
	if (get_serv_protocol_info (client_entry->protocol_name) == NULL )
	{
	    /*  Client protocol not in the server list  */
	    ++num_protocols;
	}
    }
    if ( ( strings = (char **)
	  m_alloc ( (num_protocols + 2) * sizeof *strings ) ) == NULL )
    {
	m_error_notify (function_name, "array of string pointers");
	return (NULL);
    }
    strings[0] = NULL;
    if ( ( strings[0] = st_dup
	  ("Protocol_name              serv_max  #serv     client_max #client")
	  ) == NULL )
    {
	m_error_notify (function_name, "first string");
	m_free ( (char *) strings );
	return (NULL);
    }
    strings[1] = NULL;
    /*  Process the server protocol list again  */
    for (serv_entry = serv_protocol_list, num_protocols = 0;
	 serv_entry != NULL;
	 serv_entry = serv_entry->next, ++num_protocols)
    {
	if ( serv_entry->max_connections == 0 )
	{
	    (void) strcpy (tmp1, "unlimited");
	}
	else
	{
	    (void) sprintf (tmp1, "%u", serv_entry->max_connections);
	}
	client_entry = get_client_protocol_info ( serv_entry->protocol_name);
	if (client_entry == NULL)
	{
	    (void) sprintf (txt, "%-27s%-10s%-10u-          -",
			    serv_entry->protocol_name,
			    tmp1,
			    serv_entry->connection_count);
	}
	else
	{
	    if ( client_entry->max_connections == 0 )
	    {
		(void) strcpy (tmp2, "unlimited");
	    }
	    else
	    {
		(void) sprintf (tmp2, "%u", client_entry->max_connections);
	    }
	    (void) sprintf (txt, "%-27s%-10s%-10u%-11s%u",
			    serv_entry->protocol_name,
			    tmp1,
			    serv_entry->connection_count,
			    tmp2,
			    client_entry->connection_count);
	}
	if ( ( strings[num_protocols + 1] = st_dup (txt) ) == NULL )
	{
	    m_error_notify (function_name, "protocol string information");
	    for (num_protocols = 0; strings[num_protocols] != NULL;
		 ++num_protocols) m_free (strings[num_protocols]);
	    m_free ( (char *) strings );
	    return (NULL);
	}
	strings[num_protocols + 2] = NULL;
    }
    /*  Process the client protocol list again  */
    for (client_entry = client_protocol_list; client_entry != NULL;
	 client_entry = client_entry->next)
    {
	if (get_serv_protocol_info ( client_entry->protocol_name ) != NULL )
	{
	    /*  Client protocol is in the server list  */
	    continue;
	}
	if ( client_entry->max_connections == 0 )
	{
	    (void) strcpy (tmp2, "unlimited");
	}
	else
	{
	    (void) sprintf (tmp2, "%u", client_entry->max_connections);
	}
	(void) sprintf (txt, "%-27s-         -         %-11s%u",
			client_entry->protocol_name,
			tmp2,
			client_entry->connection_count);
	if ( ( strings[num_protocols + 1] = st_dup (txt) ) == NULL )
	{
	    m_error_notify (function_name, "protocol string information");
	    for (num_protocols = 0; strings[num_protocols] != NULL;
		 ++num_protocols) m_free (strings[num_protocols]);
	    m_free ( (char *) strings );
	    return (NULL);
	}
	strings[++num_protocols + 1] = NULL;
    }
    return (strings);
}   /*  End Function conn_extract_protocols  */
#endif  /*  OS_VXMVX  */

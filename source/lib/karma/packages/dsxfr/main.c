/*LINTLIBRARY*/
/*  main.c

    This code provides high level data structure transfer routines.

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

    This file contains the various utility routines for transferring the
    general data structure supported in Karma to and from objects.


    Written by      Richard Gooch   19-SEP-1992

    Updated by      Richard Gooch   9-DEC-1992

    Updated by      Richard Gooch   21-DEC-1992: Closed disc channels after
  reading.

    Updated by      Richard Gooch   24-DEC-1992: Fixed test for "*_connection"
  in  dsxfr_put_multi  .

    Updated by      Richard Gooch   25-DEC-1992: Fixed bug in
  get_connection_num  .

    Updated by      Richard Gooch   2-JAN-1993: Added memory mapped array
  support.

    Updated by      Richard Gooch   6-MAR-1993: Added  first_time_data
  flag to read callback and added  dsxfr_register_close_func  .

    Updated by      Richard Gooch   7-MAR-1993: Added
  "connections:<module_name>"  syntax for object names to  dsxfr_put_multi  .

    Updated by      Richard Gooch   28-MAR-1993: Added explicit control for
  memory mapping to  dsxfr_get_multi  by adding parameter.

    Updated by      Richard Gooch   4-APR-1993: Took account of change to
  conn_register_server_protocol  and  conn_register_client_protocol  .

    Updated by      Richard Gooch   16-MAY-1993: Took account of replacement of
  dsrw_write_multi_desc  and  dsrw_write_multi_data  with  dsrw_write_multi  .

    Updated by      Richard Gooch   17-MAY-1993: Added  writeable  parameter to
  dsxfr_get_multi  .

    Updated by      Richard Gooch   24-JUL-1993: Changed to  ch_open_file  .

    Updated by      Richard Gooch   30-JUL-1993: Fixed bug in  dsxfr_put_multi
  which caused module specific transmission to be sent everywhere.

    Updated by      Richard Gooch   7-AUG-1993: Cleaned up attachments and the
  documentation for it.

    Updated by      Richard Gooch   11-AUG-1993: Fixed bug in implementation of
  attachment counting. Did not increment when reading "connection".

    Updated by      Richard Gooch   19-AUG-1993: Added 
  "connections:!<module_name>"  syntax for object names to  dsxfr_put_multi  .

    Updated by      Richard Gooch   22-AUG-1993: Further documented object name
  syntax for  dsxfr_put_multi  and fixed bug in  serv_read_func  .

    Updated by      Richard Gooch   4-SEP-1993: Fixed bug in  serv_close_func
  which caused false abort if  close_callback  tried to deallocate structure.

    Updated by      Richard Gooch   13-SEP-1993: Changed semantics of
  "<filename>_connection" object name for  dsxfr_put_multi  to transmit to all
  "multi_array" connections.

    Updated by      Richard Gooch   27-SEP-1993: Improved diagnostics.

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/dsxfr/main.c

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno

    Updated by      Richard Gooch   17-JAN-1995: Changed to  destroy_callbacks
  field for  multi_array  structure.

    Updated by      Richard Gooch   23-JAN-1995: Fixed bug in  serv_read_func
  which did not set callback function.

    Updated by      Richard Gooch   28-FEB-1995: Added extra parameter to
  read callback function.

    Last updated by Richard Gooch   5-MAY-1995: Placate SGI compiler.


*/

#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <karma.h>
#include <karma_dsxfr.h>
#include <karma_conn.h>
#include <karma_dsrw.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>
#include <karma_c.h>

#define MAGIC_NUMBER (unsigned int) 1541229803
#define PROTOCOL_VERSION (unsigned int) 0

/*  Private structures  */
struct cache_type
{
    unsigned int magic_number;
    char *filename;
    multi_array *multi_desc;
    Channel channel;
    flag mapped;
    flag writeable;
    struct cache_type *prev;
    struct cache_type *next;
};

struct conn_type
{
    multi_array *multi_desc;
    KCallbackFunc callback;
};


/*  Private data  */
static void (*read_callback) () = NULL;
static void (*close_callback) () = NULL;
static struct cache_type *cache_list = NULL;
static char *default_extension = ".kf";


/*  Private functions  */
STATIC_FUNCTION (flag serv_open_func, (Connection connection, void **info) );
STATIC_FUNCTION (flag serv_read_func, (Connection connection, void **info) );
STATIC_FUNCTION (void serv_close_func, (Connection connection, void *info) );
STATIC_FUNCTION (int get_connection_num, (CONST char *string) );
STATIC_FUNCTION (char *convert_object_to_filename, (CONST char *object_name) );
STATIC_FUNCTION (void remove_from_cache_list,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );
STATIC_FUNCTION (multi_array *get_cache_entry,
		 (char *filename, flag *mapped, flag *writeable) );
STATIC_FUNCTION (void add_mmap_destroy_func,
		 (multi_array *multi_desc, Channel channel) );
STATIC_FUNCTION (void close_mmap_channel,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );
STATIC_FUNCTION (void remove_conn_data,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );


/*  Private functions follow  */

static flag add_to_cache_list (char *filename, multi_array *multi_desc,
			       Channel channel, flag mapped, flag writeable)
/*  This routine will add a filename and data structure to the cache list.
    The filename must be pointed to by  filename  .
    The data structure must be pointed to by  multi_desc  .
    If a channel is to be closed when the data structure is destroyed, then it
    should be given by  channel  .If this is NULL, no channel will be closed.
    If the stucture was memory mapped, the value of  mapped  must be TRUE.
    If the structure is memory mapped and writeable, the value of  writeable
    must be TRUE.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    struct cache_type *new_entry;
    extern struct cache_type *cache_list;
    static char function_name[] = "add_to_cache_list";

    if ( (filename == NULL) || (multi_desc == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if ( ( new_entry = (struct cache_type *) m_alloc (sizeof *new_entry) )
	== NULL )
    {
	m_error_notify (function_name, "new cache entry");
	return (FALSE);
    }
    new_entry->magic_number = MAGIC_NUMBER;
    new_entry->prev = NULL;
    new_entry->next = NULL;
    if ( ( new_entry->filename = st_dup (filename) ) == NULL )
    {
	m_error_notify (function_name, "filename");
	m_free ( (char *) new_entry );
	return (FALSE);
    }
    new_entry->multi_desc = multi_desc;
    new_entry->channel = channel;
    new_entry->mapped = mapped;
    new_entry->writeable = writeable;
    c_register_callback (&multi_desc->destroy_callbacks,
			 ( flag (*) () ) remove_from_cache_list, new_entry,
			 NULL, FALSE, NULL, FALSE, FALSE);
    /*  Add entry to list  */
    if (cache_list == NULL)
    {
	cache_list = new_entry;
	return (TRUE);
    }
    /*  Insert entry at beginning of list  */
    new_entry->next = cache_list;
    (*cache_list).prev = new_entry;
    cache_list = new_entry;
    return (TRUE);
}   /*  End Function add_to_cache_list  */

static void remove_from_cache_list (void *object, void *client1_data,
				    void *call_data, void *client2_data)
/*  This routine will remove a cache list entry.
    The entry must be pointed to by  entry  .
    The routine returns nothing.
*/
{
    struct cache_type *entry = object;
    extern struct cache_type *cache_list;
    extern char *sys_errlist[];
    static char function_name[] = "remove_from_cache_list";

    if (entry->magic_number != MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Cache list entry is not valid\n");
	a_prog_bug (function_name);
    }
    entry->magic_number = 0;
    if (cache_list == entry)
    {
	cache_list = NULL;
	return;
    }
    if (entry->prev != NULL) entry->prev->next = entry->next;
    if (entry->next != NULL) entry->next->prev = entry->prev;
    if (entry->channel != NULL)
    {
	if ( !ch_close (entry->channel) )
	{
	    (void) fprintf (stderr,
			    "Error closing channel for data structure\t%s\n",
			    sys_errlist[errno]);
	}
    }
    m_free ( (char *) entry );
}   /*  End Function remove_from_cache_list  */

static multi_array *get_cache_entry (char *filename, flag *mapped,
				     flag *writeable)
/*  This function will extract the data structure associated with a filename
    from the cache list.
    The filename must be pointed to by  filename  .
    If the data structure is read memory mapped, the value TRUE will be
    written to the storage pointed to by  mapped  ,else FALSE will be
    written here.
    If the data structure is memory mapped and writeable, the value TRUE will
    be written to the storage pointed to by  writeable  ,else FALSE will be
    written here.
    The routine returns a pointed to the data structure on sucess,
    else it returns NULL (indicating that the file was not cached).
*/
{
    struct cache_type *curr_entry;
    extern struct cache_type *cache_list;

    for (curr_entry = cache_list; curr_entry != NULL;
	 curr_entry = (*curr_entry).next)
    {
	if (strcmp (filename, (*curr_entry).filename) == 0)
	{
	    *mapped = (*curr_entry).mapped;
	    *writeable = (*curr_entry).writeable;
	    return ( (*curr_entry).multi_desc );
	}
    }
    return (NULL);
}   /*  End Function get_cache_entry  */

static flag serv_open_func (Connection connection, void **info)
/*  [PURPOSE] This function will register the opening of a connection.
    <connection> The connection.
    <info> Pointer to storage for an arbitrary pointer.
    [RETURNS] TRUE on successful registration, else FALSE (indicating the
    connection should be closed).
    [NOTE] The  close_func  will not be called if this routine returns FALSE.
*/
{
    struct conn_type *new;
    static char function_name[] = "serv_open_func";

    if ( ( new = (struct conn_type *) m_alloc (sizeof *new) ) == NULL )
    {
	m_error_notify (function_name, "structure");
	return (FALSE);
    }
    new->multi_desc = NULL;
    new->callback = NULL;
    *info = new;
    return (TRUE);
}   /*  End Function serv_open_func  */

static flag serv_read_func (Connection connection, void **info)
/*  This routine will read in data from the connection given by  connection
    and will write any appropriate information to the pointer pointed to by
    info  .
    The routine returns TRUE on successful reading,
    else it returns FALSE (indicating the connection should be closed).
*/
{
    flag first_time_data = FALSE;
    Channel channel;
    unsigned int count, num;
    multi_array *new;
    struct conn_type *entry = *info;
    extern void (*read_callback) ();

    channel = conn_get_channel (connection);
    if ( ( new = dsrw_read_multi (channel) ) == NULL ) return (FALSE);
    if (entry->multi_desc == NULL)
    {
	/*  No initial data  */
	first_time_data = TRUE;
    }
    else
    {
	/*  Deallocate the multi array: first avoid the destroy function:
	    remove_conn_data  */
	c_unregister_callback (entry->callback);
	entry->callback = NULL;
	ds_dealloc_multi (entry->multi_desc);
	entry->multi_desc = NULL;
    }
    entry->multi_desc = new;
    entry->callback = c_register_callback (&new->destroy_callbacks,
					   ( flag (*) () ) remove_conn_data,
					   NULL, NULL, FALSE, NULL, FALSE,
					   FALSE);
    if (read_callback == NULL) return (TRUE);
    num = conn_get_num_serv_connections ("multi_array");
    for (count = 0; count < num; ++count)
    {
	if (conn_get_serv_connection ("multi_array", count) == connection)
	{
	    (*read_callback) (first_time_data, count);
	    return (TRUE);
	}
    }
    return (FALSE);
}   /*  End Function serv_read_func  */

static void serv_close_func (Connection connection, void *info)
/*  This routine will register the closure of a connection.
    The connection must be given by  connection  .
    Any associated data for the connection will be pointed to by  info  .
    The routine returns nothing.
*/
{
    flag data_deallocated = FALSE;
    struct conn_type *entry = info;
    extern void (*close_callback) ();

    if (entry->multi_desc != NULL)
    {
	/*  Deallocate the multi array: first avoid the destroy function:
	    remove_conn_data  */
	c_unregister_callback (entry->callback);
	entry->callback = NULL;
	ds_dealloc_multi (entry->multi_desc);
	entry->multi_desc = NULL;
	data_deallocated = TRUE;
    }
    m_free ( (char *) entry );
    if (close_callback != NULL)
    {
	(*close_callback) (data_deallocated);
    }
}   /*  End Function serv_close_func  */

static int get_connection_num (CONST char *string)
/*  This routine will extract the connection number from a string.
    The string must be pointed to by  string  .
    The routine returns the connection number on success, else it returns -1
*/
{
    long index;
    char *char_ptr;
    if (*string != '[')
    {
	(void) fprintf (stderr, "Left bracket ('[') missing\n");
	return (-1);
    }
    index = strtol (string + 1, &char_ptr, 10);
    if (*char_ptr != ']')
    {
	/*  Not terminated properly  */
	(void) fprintf (stderr, "Right bracket (']') missing\n");
	return (-1);
    }
    return (index);
}   /*  End Function get_connection_num  */

static char *convert_object_to_filename (CONST char *object_name)
/*  This routine will convert an object name to a Unix filename. This involves
    appending the default extension name to the object name. If the object name
    already contains the default extension name, no change is made.
    The object name must be pointed to by  object_name  .
    The routine returns a pointer to a dynamically allocated filename on
    success, else it returns NULL.
*/
{
    int name_length;
    int ext_length;
    char *filename;
    extern char *default_extension;
    static char function_name[] = "convert_object_to_filename";

    name_length = strlen (object_name);
    ext_length = strlen (default_extension);
    if (name_length >= ext_length)
    {
	if (strcmp (object_name + name_length - ext_length, default_extension)
	    == 0)
	{
	    /*  Extension already there  */
	    return ( st_dup (object_name) );
	}
    }
    /*  Must append default extension  */
    if ( ( filename = m_alloc (name_length + ext_length + 1) ) == NULL )
    {
	m_error_notify (function_name, "filename");
	return (NULL);
    }
    (void) strcpy (filename, object_name);
    (void) strcat (filename, default_extension);
    return (filename);
}   /*  End Function convert_object_to_filename  */

static void add_mmap_destroy_func (multi_array *multi_desc, Channel channel)
/*  This routine will add a destroy function to a data structure so that when
    the data structure is deallocated, the channel object is closed.
    The data structure must be pointed to by  multi_desc  .
    The channel object must be given by  channel  .
*/
{
/*
    static char function_name[] = "add_mmap_destroy_func";
*/

    c_register_callback (&multi_desc->destroy_callbacks,
			 ( flag (*) () ) close_mmap_channel,
			 channel, NULL, FALSE, NULL, FALSE, FALSE);
}   /*  End Function add_mmap_destroy_func  */

static void close_mmap_channel (void *object, void *client1_data,
				void *call_data, void *client2_data)
/*  This routine will close the memory mapped channel object associated with
    a data structure.
    The routine returns nothing.
*/
{
    Channel channel = object;
    extern char *sys_errlist[];

    if ( !ch_close (channel) )
    {
	(void) fprintf (stderr,
			"Error closing memory mapped channel for data structure\t%s\n",
			sys_errlist[errno]);
    }
}   /*  End Function close_mmap_channel  */

static void remove_conn_data (void *object, void *client1_data,
			      void *call_data, void *client2_data)
/*  This routine will process the destruction of a multi_array data structure
    which was received over a connection.
    The arbitrary destroy info will be pointed to by  info  .
    The routine returns nothing.
*/
{
    static char function_name[] = "remove_conn_data";

    (void) fprintf (stderr,
		    "Attempt to deallocate multi_array data structure received from a connection\n");
    a_prog_bug (function_name);
}   /*  End Function remove_conn_data  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void dsxfr_register_connection_limits (int max_incoming, int max_outgoing)
/*  [PURPOSE] This routine will register the maximum number of incoming
    (server) and outgoing (client) connections for the transfer of the general
    data structure. The protocol used is: "multi_array".
    <max_incoming> The maximum number of incoming connections. If this is less
    than 0, no connections are permitted. If this is 0, an unlimited number of
    connections is permitted.
    <max_outgoing> The maximum number of outgoing connections. If this is less
    than 0, no connections are permitted. If this is 0, an unlimited number of
    connections is permitted.
    [NOTE] This routine ONLY registers the server and client callback routines,
    the application must first call <conn_register_managers> in all cases and
    must call <conn_become_server> if it wishes to accept connections.
    [RETURNS] Nothing.
*/
{
    if (max_incoming > -1)
    {
	conn_register_server_protocol ("multi_array", PROTOCOL_VERSION,
				       max_incoming,
				       serv_open_func,
				       serv_read_func,
				       serv_close_func);
    }
    if (max_outgoing > -1)
    {
	conn_register_client_protocol ("multi_array", PROTOCOL_VERSION,
				       max_outgoing,
				       ( flag (*) () ) NULL,
				       ( flag (*) () ) NULL,
				       ( flag (*) () ) NULL,
				       ( void (*) () ) NULL);
    }
}   /*  End Function dsxfr_register_connection_limits  */

/*PUBLIC_FUNCTION*/
flag dsxfr_put_multi (CONST char *object, multi_array *multi_desc)
/*  [PURPOSE] This routine will put (write to disc, transmit over connection) a
    multi_desc general data structure to a named object.
    <object> The object name. If the object is named "connections" then the
    data will be transmitted to all available connections with the
    "multi_array" protocol. If the object is named "connections:{module_name}"
    then the data will be transmitted to all available connections to modules
    with the name {module_name} and with the "multi_array" protocol.
    If the object is named "connections:!{module_name}" then the data will be
    transmitted to all available connections with the "multi_array" protocol to
    all modules except those with the name {module_name}.
    If the object is named "connection[#]" then the data will be transmitted to
    the "multi_array" protocol connection with index: # (starting from 0).
    If the object is named "{filename}_connection" then the data will be
    transmitted to all available connections with the "multi_array" protocol.
    In all other cases the data will be written to a disc file. The routine
    will append a ".kf" extension if not already specified. If the disc file
    already exists, the old data file is first renamed to have a trailing '~'
    (tilde) character.
    <multi_desc> A pointer to the data structure.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag rename_file;
    flag all_modules_except;
    int connection_num;
    unsigned int connection_count;
    unsigned int num_connections;
    Connection connection;
    Channel channel;
    CONST char *char_ptr;
    char *filename;
    char *tilde_filename;
    CONST char *module_name;
    extern char *sys_errlist[];
    static int connection_name_length = -1;
    static char connection_name[] = "connection";
    static char function_name[] = "dsxfr_put_multi";

    if (object == NULL)
    {
	(void) fprintf (stderr, "NULL string pointer passed\n");
	a_prog_bug (function_name);
    }
    if (*object == '\0')
    {
	(void) fprintf (stderr, "Empty string passed\n");
	a_prog_bug (function_name);
    }
    if (multi_desc == NULL)
    {
	(void) fprintf (stderr, "NULL descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if (connection_name_length < 0)
    {
	connection_name_length = strlen (connection_name);
    }
    if (strncmp (object, "connections", 11) == 0)
    {
	/*  Write to many connections  */
	if (strcmp (object, "connections") == 0)
	{
	    /*  Write to all connections  */
	    module_name = NULL;
	    all_modules_except = FALSE;
	}
	else
	{
	    /*  Write to connections with specified module name  */
	    if ( ( char_ptr = strchr (object, ':') ) == NULL )
	    {
		(void) fprintf (stderr, "Bad name: \"%s\"\n", object);
		return (FALSE);
	    }
	    if (*++char_ptr == '\0')
	    {
		(void) fprintf (stderr, "Module name missing from: \"%s\"\n",
				object);
		return (FALSE);
	    }
	    if (*char_ptr == '!')
	    {
		/*  Send to all modules except named one  */
		all_modules_except = TRUE;
		++char_ptr;
	    }
	    else
	    {
		/*  Send only to named module  */
		all_modules_except = FALSE;
	    }
	    module_name = char_ptr;
	}
	num_connections = conn_get_num_client_connections ("multi_array");
	for (connection_count = 0; connection_count < num_connections;
	     ++connection_count)
	{
	    if ( ( connection = conn_get_client_connection ("multi_array",
							    connection_count) )
		== NULL )
	    {
		(void) fprintf (stderr, "Could not find connection: %u\n",
				connection_count);
		a_prog_bug (function_name);
	    }
	    if (module_name != NULL)
	    {
		/*  Check for module name match  */
		if (strcmp (conn_get_connection_module_name (connection),
			    module_name) == 0)
		{
		    /*  Module names matched  */
		    if (all_modules_except) continue;
		}
		else
		{
		    /*  Modules names did not match  */
		    if (!all_modules_except) continue;
		}
	    }
	    channel = conn_get_channel (connection);
	    dsrw_write_multi (channel, multi_desc);
	    if ( !ch_flush (channel) )
	    {
		(void) fprintf (stderr, "Error flushing channel\t%s\n",
				sys_errlist[errno]);
		return (FALSE);
	    }
	}
	return (TRUE);
    }
    if ( ( char_ptr = strrchr (object, '_') ) == NULL )
    {
	/*  '_' not found: look from start of string  */
	char_ptr = object;
    }
    else
    {
	/*  Point beyond '_'  */
	++char_ptr;
    }
    if (strcmp (char_ptr, connection_name) == 0)
    {
	/*  Send data over all connections  */
	return ( dsxfr_put_multi ("connections", multi_desc) );
    }
    if (strncmp (char_ptr, connection_name, connection_name_length) == 0)
    {
	if ( ( connection_num =
	      get_connection_num (char_ptr + connection_name_length) ) < 0 )
	{
	    (void) fprintf (stderr, "%s: bad object name: \"%s\"\n",
			    function_name, object);
	    return (FALSE);
	}
	/*  Send data over numbered connection  */
	if ( ( connection =
	      conn_get_client_connection ("multi_array",
					  (unsigned int) connection_num) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Could not find connection: %d\n",
			    connection_num);
	    return (FALSE);
	}
	channel = conn_get_channel (connection);
	dsrw_write_multi (channel, multi_desc);
	if ( !ch_flush (channel) )
	{
	    (void) fprintf (stderr, "Error flushing channel\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	return (TRUE);
    }
    /*  Multi array must be written to an arrayfile (disc), not a connection */
    if ( ( filename = convert_object_to_filename (object) ) == NULL )
    {
	m_error_notify (function_name, "filename");
	return (FALSE);
    }
    if (access (filename, F_OK) == 0)
    {
	/*  Old file exists  */
	rename_file = TRUE;
    }
    else
    {
	/*  Error accessing old file  */
	if (errno != ENOENT)
	{
	    /*  Error, not simply missing file  */
	    (void) fprintf (stderr, "Error accessing file: \"%s\"\t%s\n",
			    filename, sys_errlist[errno]);
	    m_free (filename);
	    return (FALSE);
	}
	/*  File does not exist  */
	rename_file = FALSE;
    }
    if (rename_file == TRUE)
    {
	if ( ( tilde_filename = m_alloc (strlen (filename) + 2) ) == NULL )
	{
	    m_error_notify (function_name, "tilde filename");
	    m_free (filename);
	    return (FALSE);
	}
	(void) strcpy (tilde_filename, filename);
	(void) strcat (tilde_filename, "~");
	if (rename (filename, tilde_filename) != 0)
	{
	    (void) fprintf (stderr, "Error renaming file: \"%s\"\t%s\n",
			    filename, sys_errlist[errno]);
	    m_free (tilde_filename);
	    m_free (filename);
	    return (FALSE);
	}
	m_free (tilde_filename);
    }
    if ( ( channel = ch_open_file (filename, "w") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening file: \"%s\" for output\t%s\n",
			filename, sys_errlist[errno]);
	m_free (filename);
	return (FALSE);
    }
    m_free (filename);
    dsrw_write_multi (channel, multi_desc);
    if ( !ch_close (channel) )
    {
	(void) fprintf (stderr, "Error closing channel\n");
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function dsxfr_put_multi  */

/*PUBLIC_FUNCTION*/
multi_array *dsxfr_get_multi (CONST char *object, flag cache,
			      unsigned int mmap_option, flag writeable)
/*  [PURPOSE] This routine will get (read from disc, read from connection) a
    multi_desc general data structure from a named object.
    <object> The object name. If the object is named "connection[#]" then
    whatever data has been previously sent over the "multi_array" protocol
    connection with index: # (starting from 0) will be returned.
    In all other cases the data will be read from a disc file. The routine will
    append a ".kf" extension if not already specified.
    <cache> If TRUE and the data is read from a disc, the data structure and
    filename relationship is cached. This means that a subsequent attempt to
    read the data will not require the disc to be accessed. This relationship
    is lost if the data structure is destroyed. Also, in both this case and the
    case where the data structure is "read" from a connection, the attachment
    count for the data structure is incremented *every time* this routine is
    called. Read the documentation for the <ds_dealloc_multi> routine for
    information on attachment counts. The attachment count is *not* incremented
    when reading a disc file without adding it to the cache list.
    <mmap_option> Option to control memory mapping when reading from disc.
    Legal values are:
    [<pre>]
        K_CH_MAP_NEVER           Never map
	K_CH_MAP_LARGE_LOCAL     Map if local filesystem and file size > 1MB
	K_CH_MAP_LOCAL           Map if local filesystem
	K_CH_MAP_LARGE           Map if file over 1 MByte
	K_CH_MAP_IF_AVAILABLE    Map if operating system supports it
	K_CH_MAP_ALWAYS          Always map, fail if not supported.
    [</pre>]
    <writeable> If TRUE, the mapped structure will be writeable. When the data
    structure data is modified these changes will be reflected in the disc
    file. The shape of the data structure cannot be changed though mapping.
    If FALSE and the structure is written to, a segmentation fault occurs.
    [RETURNS] A pointer to the data structure on success, else NULL.
    [NOTE] Reading from a connection with this routine does *not* block, if no
    prior data was transmitted, the routine returns NULL. Multiple calls to
    this routine will return the same data structure *until* new data is
    received over the connection.
*/
{
    flag mapped = FALSE;
    flag prev_mapped;
    flag prev_writeable;
    int connection_num;
    Connection connection;
    Channel channel;
    void *info;
    CONST char *char_ptr;
    char *filename;
    multi_array *multi_desc;
    struct conn_type *entry;
    extern char *sys_errlist[];
    static int connection_name_length = -1;
    static char connection_name[] = "connection";
    static char function_name[] = "dsxfr_get_multi";

    if (object == NULL)
    {
	(void) fprintf (stderr, "NULL string pointer passed\n");
	a_prog_bug (function_name);
    }
    if (*object == '\0')
    {
	(void) fprintf (stderr, "Empty string passed\n");
	a_prog_bug (function_name);
    }
    FLAG_VERIFY (writeable);
    if (connection_name_length < 0)
    {
	connection_name_length = strlen (connection_name);
    }
    if ( ( char_ptr = strrchr (object, '_') ) == NULL )
    {
	/*  '_' not found: look from start of string  */
	char_ptr = object;
    }
    if (strcmp (char_ptr, connection_name) == 0)
    {
	/*  Read data from first connection  */
	if ( ( connection = conn_get_serv_connection ("multi_array", 0) )
	    == NULL )
	{
	    return (NULL);
	}
	if ( ( info = conn_get_connection_info (connection) ) == NULL )
	{
	    (void) fprintf (stderr, "No data yet on connection: 0\n");
	    return (NULL);
	}
	entry = (struct conn_type *) info;
	/*  Increment the attachment count  */
	++entry->multi_desc->attachments;
	return (entry->multi_desc);
    }
    if (strncmp (char_ptr, connection_name, connection_name_length) == 0)
    {
	if ( ( connection_num =
	      get_connection_num (char_ptr + connection_name_length) ) < 0 )
	{
	    (void) fprintf (stderr, "%s: bad object name: \"%s\"\n",
			    function_name, object);
	    return (NULL);
	}
	/*  Read data from numbered connection  */
	if ( ( connection =
	      conn_get_serv_connection ("multi_array",
					(unsigned int) connection_num) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Could not find connection: %d\n",
			    connection_num);
	    return (NULL);
	}
	if ( ( info = conn_get_connection_info (connection) ) == NULL )
	{
	    (void) fprintf (stderr, "No data yet on connection: %d\n",
			    connection_num);
	    return (NULL);
	}
	entry = (struct conn_type *) info;
	/*  Increment the attachment count  */
	++entry->multi_desc->attachments;
	return (entry->multi_desc);
    }
    /*  Multi array must be read from an arrayfile (disc), not a connection */
    if ( ( filename = convert_object_to_filename (object) ) == NULL )
    {
	m_error_notify (function_name, "filename");
	return (NULL);
    }
    if ( ( multi_desc = get_cache_entry (filename, &prev_mapped,
					 &prev_writeable) )
	!= NULL )
    {
	if (!prev_mapped || prev_writeable || !writeable)
	{
	    /* Not previously mapped read-only or want read-only: no dangers */
	    m_free (filename);
	    /*  Increment the attachment count  */
	    ++multi_desc->attachments;
	    return (multi_desc);
	}
	/*  Was previously mapped read-only and want write access: get a new
	    one  */
    }
    if ( ( channel = ch_map_disc (filename, mmap_option, writeable, TRUE) )
	== NULL )
    {
	(void) fprintf (stderr, "Error opening file: \"%s\" for input\t%s\n",
			filename, sys_errlist[errno]);
	m_free (filename);
	return (NULL);
    }
    if ( ( multi_desc = dsrw_read_multi (channel) ) == NULL )
    {
	if ( !ch_close (channel) )
	{
	    (void) fprintf (stderr, "Error closing disc channel: \"%s\"\t%s\n",
			    filename, sys_errlist[errno]);
	}
	m_free (filename);
	return (NULL);
    }
    if ( ch_test_for_mmap (channel) )
    {
	/*  Channel was mapped: see if mapping used  */
	if (ch_get_mmap_access_count (channel) > 0)
	{
	    mapped = TRUE;
	}
    }
    if (cache)
    {
	if ( !add_to_cache_list (filename, multi_desc, mapped ? channel : NULL,
			       mapped, writeable) )
	{
	    (void) fprintf (stderr, "Error cacheing data structure\n");
	}
	/*  Increment the attachment count  */
	++multi_desc->attachments;
    }
    else
    {
	if (mapped)
	{
	    /*  Do not cache: add different destroy func  */
	    add_mmap_destroy_func (multi_desc, channel);
	}
    }
    if (!mapped)
    {
	if ( !ch_close (channel) )
	{
	    (void) fprintf (stderr, "Error closing disc channel: \"%s\"\t%s\n",
			    filename, sys_errlist[errno]);
	}
    }
    m_free (filename);
    return (multi_desc);
}   /*  End Function dsxfr_get_multi  */

/*PUBLIC_FUNCTION*/
void dsxfr_register_read_func (void (*read_func) ())
/*  [PURPOSE] This routine will register a function which is to be called when
    new data arrives on a "multi_array" connection.
    <read_func> A pointer to the function. The interface to this function is
    given below:

    void read_func (flag first_time_data, unsigned int connection_num)
    *   This routine is called when new data arrives on any "multi_array"
        connection.
	If data appears on a connection for the first time, the value of
	first_time_data  will be TRUE. Any subsqeuent data that appears on a
	connection will not set this flag.
	The index number of the connection will be given by  connection_num  .
	The routine returns nothing.
    *

    [RETURNS] Nothing.
*/
{
    extern void (*read_callback) ();
    static char function_name[] = "dsxfr_register_read_func";

    if (read_callback != NULL)
    {
	(void) fprintf (stderr, "Read callback function already registered\n");
	a_prog_bug (function_name);
    }
    read_callback = read_func;
}   /*  End Function dsxfr_register_read_func  */

/*PUBLIC_FUNCTION*/
void dsxfr_register_close_func (void (*close_func) ())
/*  [PURPOSE] This routine will register a function which is to be called when
    a "multi_array" connection closes.
    <close_func> A pointer to the function. The interface to this function is
    given below:

    void close_func (data_deallocated)
    *   This routine is called when any "multi_array" connection closes.
        If there was a multi_array data structure already received over the
	connection, it is deallocated and  data_deallocated  will be TRUE.
	The routine returns nothing.
    *
    flag data_deallocated;

    [RETURNS] Nothing.
*/
{
    extern void (*close_callback) ();
    static char function_name[] = "dsxfr_register_close_func";

    if (close_callback != NULL)
    {
	(void) fprintf (stderr,
			"Closure callback function already registered\n");
	a_prog_bug (function_name);
    }
    close_callback = close_func;
}   /*  End Function dsxfr_register_close_func  */

/*  tcplog.c

    Source file for  tcplog  (TCP/IP logging facility).

    Copyright (C) 1994-1996  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This Karma module will support TCP/IP connection logging. This is useful
  when determining exactly which bytes are transferred across a TCP/IP link.


    Written by      Richard Gooch   14-AUG-1994

    Last updated by Richard Gooch   1-JUN-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <errno.h>
#include <karma.h>
#include <karma_chm.h>
#include <karma_ch.h>
#include <karma_im.h>
#include <karma_r.h>
#include <karma_m.h>
#include <karma_a.h>


/*  Private functions  */
STATIC_FUNCTION (flag dock_input_func, (Channel dock, void **info) );
STATIC_FUNCTION (flag input_func, (Channel dock, void **info) );
STATIC_FUNCTION (void close_func, (Channel dock, void *info) );


struct conn_info_type
{
    Channel other;
    Channel log;
};


/*  Private data  */
static char *remote_spec = NULL;
static Channel txlog = NULL;
static Channel rxlog = NULL;

int main (int argc, char **argv)
{
    Channel *port;
    int req_port;
    unsigned int dock_count;
    unsigned int num_docks;
    unsigned int allocated_port;
    extern Channel txlog;
    extern Channel rxlog;
    extern char module_name[STRING_LENGTH + 1];
    extern char *remote_spec;
    extern char *sys_errlist[];

    im_register_module_name ("tcplog");
    if (argc != 4)
    {
	(void) fprintf (stderr, "Usage:\ttcplog remote_spec txlog rxlog\n");
	exit (RV_BAD_PARAM);
    }
    remote_spec = argv[1];
    /*  Allocate a port  */
    if ( ( req_port = r_get_def_port ( module_name, r_getenv ("DISPLAY") ) )
	< 0 )
    {
	(void) fprintf (stderr, "Error getting port number\n");
	exit (RV_UNDEF_ERROR);
    }
    allocated_port = req_port;
    if ( ( port = ch_alloc_port (&allocated_port, 0, &num_docks) ) == NULL )
    {
	(void) fprintf (stderr, "Error allocating port\n");
	exit (RV_UNDEF_ERROR);
    }
    (void) fprintf (stderr, "Port number: %u\n", allocated_port + 6200);
    for (dock_count = 0; dock_count < num_docks; ++dock_count)
    {
	if (chm_manage (port[dock_count], (void *) NULL, dock_input_func,
			( void (*) () ) NULL,
			( flag (*) () ) NULL,
			( flag (*) () ) NULL) != TRUE)
	{
	    (void) fprintf (stderr, "Error managing dock: %u\n", dock_count);
	    exit (RV_UNDEF_ERROR);
	}
    }
    m_free ( (char *) port );
    if ( ( txlog = ch_open_file (argv[2], "w") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening txlogfile: \"%s\"\t%s\n",
			argv[2], sys_errlist[errno]);
	exit (RV_CANNOT_OPEN);
    }
    if ( ( rxlog = ch_open_file (argv[3], "w") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening rxlogfile: \"%s\"\t%s\n",
			argv[3], sys_errlist[errno]);
	exit (RV_CANNOT_OPEN);
    }
    while (TRUE) chm_poll (-1);
}   /*  End Function main  */

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
    Channel new_conn, remote;
    unsigned long inet_addr;
    struct conn_info_type *con_local, *con_remote;
    extern char *remote_spec;
    extern char *sys_errlist[];
    static char function_name[] = "dock_input_func";

    if ( ( new_conn = ch_accept_on_dock (dock, &inet_addr) ) == NULL )
    {
	a_func_abort (function_name, "could not accept connection on dock");
	return (FALSE);
    }
    if ( ( con_local = (struct conn_info_type *) m_alloc (sizeof *con_local) )
	== NULL )
    {
	m_abort (function_name, "info structure");
    }
    if ( ( con_remote = (struct conn_info_type *)
	  m_alloc (sizeof *con_remote) ) == NULL )
    {
	m_abort (function_name, "info structure");
    }
    if ( ( remote = ch_open_file (remote_spec, "r+") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening remote\t%s\n",
			sys_errlist[errno]);
	(void) ch_close (new_conn);
	return (TRUE);
    }
    (*con_local).other = remote;
    (*con_local).log = txlog;
    if (chm_manage (new_conn, (void *) con_local,
		    input_func, close_func,
		    ( flag (*) () ) NULL,
		    ( flag (*) () ) NULL) != TRUE)
    {
	(void) ch_close (new_conn);
	a_func_abort (function_name, "could not manage channel");
	return (FALSE);
    }
    (*con_remote).other = new_conn;
    (*con_remote).log = rxlog;
    if (chm_manage (remote, (void *) con_remote,
		    input_func, close_func,
		    ( flag (*) () ) NULL,
		    ( flag (*) () ) NULL) != TRUE)
    {
	(void) ch_close (new_conn);
	a_func_abort (function_name, "could not manage channel");
	return (FALSE);
    }
    (void) fprintf (stderr, "New connection being logged\n");
    return (TRUE);
}   /*  End Function dock_input_func  */

static flag input_func (Channel channel, void **info)
/*   This routine is called when new input occurs on a channel.
     The channel object is given by  channel  .
     An arbitrary pointer may be written to the storage pointed to by  info  .
     The pointer written here will persist until the channel is unmanaged
     (or a subsequent callback routine changes it).
     The routine returns TRUE if the connection is to remain managed,
     else it returns FALSE (indicating that the connection is to be
     unmanaged).
*/
{
    int bytes_readable;
    struct conn_info_type *con;
    extern char *sys_errlist[];
    static uaddr buf_size = 0;
    static char *buffer = NULL;
    static char function_name[] = "input_func";

    con = (struct conn_info_type *) *info;
    if ( ( bytes_readable = ch_get_bytes_readable (channel) ) < 0 )
    {
	(void) fprintf (stderr, "Error getting bytes readable\n");
	exit (RV_UNDEF_ERROR);
    }
    if (bytes_readable > buf_size)
    {
	if (buffer != NULL) m_free (buffer);
	if ( ( buffer = m_alloc (bytes_readable) ) == NULL )
	{
	    m_abort (function_name, "buffer");
	}
	buf_size = bytes_readable;
    }
    if (ch_read (channel, buffer, (unsigned int) bytes_readable) <
	bytes_readable)
    {
	(void) fprintf (stderr, "Error reading channel\t%s\n",
			sys_errlist[errno]);
	exit (RV_UNDEF_ERROR);
    }
    if (ch_write ( (*con).other, buffer, (unsigned int) bytes_readable) <
	bytes_readable)
    {
	(void) fprintf (stderr, "Error writing to channel\t%s\n",
			sys_errlist[errno]);
	exit (RV_UNDEF_ERROR);
    }
    if ( !ch_flush ( (*con).other ) )
    {
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
	exit (RV_UNDEF_ERROR);
    }
    if (ch_write ( (*con).log, buffer, (unsigned int) bytes_readable) <
	bytes_readable)
    {
	(void) fprintf (stderr, "Error writing to channel\t%s\n",
			sys_errlist[errno]);
	exit (RV_UNDEF_ERROR);
    }
    if ( !ch_flush ( (*con).log ) )
    {
	(void) fprintf (stderr, "Error flushing channel\t%s\n",
			sys_errlist[errno]);
	exit (RV_UNDEF_ERROR);
    }
    return (TRUE);
}   /*  End Function input_func  */

static void close_func (Channel channel, void *info)
/*  This routine is called when a channel closes.
    The channel object is given by  channel  .
    The arbitrary pointer for the channel will be pointed to by  info  .
    Any unread buffered data in the channel will be lost upon closure. The
    call to this function is the last chance to read this buffered data.
    The routine returns nothing.
*/
{
    struct conn_info_type *con;
    /*static char function_name[] = "close_func";*/

    con = (struct conn_info_type *) info;
    chm_unmanage ( (*con).other );
    (void) ch_close ( (*con).other );
    m_free ( (char *) con );
    (void) fprintf (stderr, "Connection closed\n");
}   /*  End Function close_func  */

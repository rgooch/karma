/*  kftpd.c

    Source file for  kftpd  (file transfer daemon module).

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

/*  This Karma module will service  file_transfer  protocol requests.


    Written by      Richard Gooch   3-MAY-1994

    Updated by      Richard Gooch   5-MAY-1994

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Updated by      Richard Gooch   1-JUN-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   12-JUL-1996: Switched to utime() call.


*/
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <utime.h>
#include <karma.h>
#include <karma_conn.h>
#include <karma_chm.h>
#include <karma_pio.h>
#include <karma_ch.h>
#include <karma_im.h>
#include <karma_a.h>
#include <karma_r.h>
#include <karma_d.h>
#include <karma_m.h>
#include "kftp.h"


#define VERSION "1.1"

#define BUF_SIZE 1048576

STATIC_FUNCTION (flag process_request, (Connection connection, void **info) );
STATIC_FUNCTION (flag copy_channels, (Channel dest, Channel source,
				      unsigned int length) );


int main (int argc, char **argv)
{
    int def_port_number;
    unsigned int server_port_number;
    extern char *sys_errlist[];
    extern char module_name[STRING_LENGTH + 1];
    /*static char function_name[] = "main";*/

    /*  Fork  */
    switch ( fork () )
    {
      case 0:
	/*  Child  */
	break;
      case -1:
	/*  Error  */
	(void) fprintf (stderr, "Error forking\t%s\n", sys_errlist[errno]);
	exit (RV_SYS_ERROR);
/*
	break;
*/
      default:
	/*  Parent  */
	exit (RV_OK);
/*
	break;
*/
    }
    /*  Initialise module  */
    im_register_module_name ("kftpd");
    im_register_module_version_date (VERSION);
    conn_register_managers (chm_manage, chm_unmanage, ( void (*) () ) NULL);
    conn_register_server_protocol ("file_transfer", 0, 1,
				   ( flag (*) () ) NULL,
				   process_request,
				   ( void (*) () ) NULL);
    if ( ( def_port_number = r_get_def_port (module_name, NULL) ) < 0 )
    {
	(void) fprintf (stderr, "Could not get default port number\n");
	exit (RV_UNDEF_ERROR);
    }
    server_port_number = def_port_number;
    if ( !conn_become_server (&server_port_number, CONN_MAX_INSTANCES) )
    {
	(void) fprintf (stderr, "Error becoming server\n");
	exit (RV_UNDEF_ERROR);
    }
    def_port_number = server_port_number;
    (void) fprintf (stderr, "Port allocated: %d\n", def_port_number);
    d_enter_daemon_mode ();
    while (TRUE) chm_poll (-1);
}   /*  End Function main   */


/*  Private functions follow  */

static flag process_request (connection, info)
/*  This routine will read in data from the connection given by  connection
    and will write any appropriate information to the pointer pointed to by
    info  .
    The routine returns TRUE on successful reading,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will be called upon connection closure.
*/
Connection connection;
void **info;
{
    Channel channel;
    Channel fch;
    unsigned long command;
    unsigned long length;
    unsigned long mode;
    unsigned long mtime;
    unsigned int name_len;
    char dummy = '\0';
    char *filename;
    struct stat statbuf;
    struct utimbuf ut;

    channel = conn_get_channel (connection);
    if ( !pio_read32 (channel, &command) ) return (FALSE);
    switch (command)
    {
      case KFTP_REQUEST_SEND:
	if ( ( filename = pio_read_string (channel, &name_len) )
	    == NULL ) return (FALSE);
	if ( !pio_read32 (channel, &length) )
	{
	    m_free (filename);
	    return (FALSE);
	}
	if ( !pio_read32 (channel, &mode) )
	{
	    m_free (filename);
	    return (FALSE);
	}
	if ( !pio_read32 (channel, &mtime) )
	{
	    m_free (filename);
	    return (FALSE);
	}
	if ( ( fch = ch_open_file (filename, "w") ) == NULL )
	{
	    m_free (filename);
	    if ( !pio_write32 (channel, KFTP_RESPONSE_FAILED) ) return (FALSE);
	    if ( !pio_write32 (channel, errno) ) return (FALSE);
	    return ( ch_flush (channel) );
	}
	if (fchmod (ch_get_descriptor (fch), mode) != 0)
	{
	    if ( !pio_write32 (channel, KFTP_RESPONSE_FAILED) ) return (FALSE);
	    if ( !pio_write32 (channel, errno) ) return (FALSE);
	    return ( ch_flush (channel) );
	}
	if ( !pio_write32 (channel, KFTP_RESPONSE_OK) ) return (FALSE);
	if ( !ch_flush (channel) ) return (FALSE);
	if ( !copy_channels (fch, channel, length) ) return (FALSE);
	(void) ch_close (fch);
	ut.actime = mtime;
	ut.modtime = mtime;
	(void) utime (filename, &ut);
	if (ch_write (channel, &dummy, 1) < 1) return (FALSE);
	if ( !ch_flush (channel) ) return (FALSE);
	break;
      case KFTP_REQUEST_GET:
	if ( ( filename = pio_read_string (channel, &name_len) )
	    == NULL ) return (FALSE);
	if (stat (filename, &statbuf) != 0)
	{
	    m_free (filename);
	    if ( !pio_write32 (channel, KFTP_RESPONSE_FAILED) ) return (FALSE);
	    if ( !pio_write32 (channel, errno) ) return (FALSE);
	    return ( ch_flush (channel) );
	}
	length = statbuf.st_size;
	if ( ( fch = ch_open_file (filename, "r") ) == NULL )
	{
	    m_free (filename);
	    if ( !pio_write32 (channel, KFTP_RESPONSE_FAILED) ) return (FALSE);
	    if ( !pio_write32 (channel, errno) ) return (FALSE);
	    return ( ch_flush (channel) );
	}
	if ( !pio_write32 (channel, KFTP_RESPONSE_OK) ) return (FALSE);
	if ( !pio_write32 (channel, length) ) return (FALSE);
	if ( !pio_write32 (channel, statbuf.st_mode) ) return (FALSE);
	if ( !pio_write32 (channel, statbuf.st_mtime) ) return (FALSE);
	if ( !copy_channels (channel, fch, length) ) return (FALSE);
	(void) ch_close (fch);
	break;
      default:
	return (FALSE);
/*
	break;
*/
    }	
    return ( ch_flush (channel) );
}   /*  End Function process_request  */

static flag copy_channels (dest, source, length)
/*  This routine will copy bytes from one channel to another.
    The destination channel must be given by  dest  .
    The source channel must be given by  source  .
    The number of bytes to copy must be given by  length  .
    The routine returns TRUE on success, else it returns FALSE.
*/
Channel dest;
Channel source;
unsigned int length;
{
    unsigned int blk_len;
    char buffer[BUF_SIZE];

    while (length > 0)
    {
	blk_len = (length > BUF_SIZE) ? BUF_SIZE : length;
	if (ch_read (source, buffer, blk_len) < blk_len) return (FALSE);
	if (ch_write (dest, buffer, blk_len) < blk_len) return (FALSE);
	length -= blk_len;
    }
    return (TRUE);
}   /*  End Function copy_channels  */

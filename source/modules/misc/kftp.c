/*  kftp.c

    Source file for  kftp  (file transfer module).

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

/*  This Karma module will transfer files using the  file_transfer  protocol.


    Written by      Richard Gooch   4-MAY-1994

    Updated by      Richard Gooch   4-MAY-1994

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Updated by      Richard Gooch   27-MAY-1994: Moved completion message.

    Updated by      Richard Gooch   4-SEP-1994: Supported "." as local filename
  on get and changed printing interval of '.' characters to 1/80th file size.

    Updated by      Richard Gooch   22-NOV-1994: Display transfer rate.

    Updated by      Richard Gooch   1-JUN-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   12-JUL-1996: Switched to utime() call.


*/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
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
#include "kftp.h"


#define VERSION "1.2"

#define BUF_SIZE 1048576

STATIC_FUNCTION (flag open_func, (Connection connection, void **info) );
STATIC_FUNCTION (flag copy_channels, (Channel dest, Channel source,
				      unsigned int length) );

static char localfile[STRING_LENGTH];
static char remotefile[STRING_LENGTH];
static unsigned int operation;
static flag failure = TRUE;


int main (int argc, char **argv)
{
    int port_number;
    char *source;
    char *dest;
    char *ptr;
    char hostname[STRING_LENGTH];
    static char usage_string[] ="Usage:\tkftp [-port port_number] source dest";
    /*static char function_name[] = "main";*/

    /*  Process arguments  */
    if ( (argc != 3) && (argc != 5) )
    {
	fprintf (stderr, "%s\n", usage_string);
	exit (RV_BAD_PARAM);
    }
    if (strcmp (argv[1], "-port") == 0)
    {
	port_number = atoi (argv[2]);
	source = argv[3];
	dest = argv[4];
    }
    else
    {
	if ( ( port_number = r_get_def_port ("kftpd", NULL) ) < 0 )
	{
	    fprintf (stderr, "Could not get default port number\n");
	    exit (RV_UNDEF_ERROR);
	}
	source = argv[1];
	dest = argv[2];
    }
    if ( ( ptr = strchr (source, ':') ) == NULL )
    {
	if ( ( ptr = strchr (dest, ':') ) == NULL )
	{
	    fprintf (stderr,
			    "Either source or destination must be remote\n");
	    exit (RV_BAD_PARAM);
	}
	operation = KFTP_REQUEST_SEND;
	strcpy (localfile, source);
	strcpy (remotefile, ptr + 1);
	strncpy (hostname, dest, ptr - dest);
	hostname[ptr - dest] = '\0';
    }
    else
    {
	operation = KFTP_REQUEST_GET;
	strcpy (localfile, dest);
	strcpy (remotefile, ptr + 1);
	strncpy (hostname, source, ptr - source);
	hostname[ptr - source] = '\0';
    }
    if (strlen (remotefile) == 0) strcpy (remotefile, ".");
    fprintf (stderr, "l: \"%s\"  r: \"%s\"\n", localfile, remotefile);
    /*  Initialise module  */
    im_register_module_name ("kftp");
    im_register_module_version_date (VERSION);
    conn_register_managers (chm_manage, chm_unmanage, ( void (*) () ) NULL);
    conn_register_client_protocol ("file_transfer", 0, 1,
				   ( flag (*) () ) NULL,
				   open_func,
				   ( flag (*) () ) NULL,
				   ( void (*) () ) NULL);
    if (!conn_attempt_connection (hostname, port_number, "file_transfer") &&
	failure)
    {
	fprintf (stderr, "Error connecting to daemon\n");
	exit (RV_UNDEF_ERROR);
    }
    fprintf (stderr, "Transfer completed\n");
    return (RV_OK);
}   /*  End Function main   */


/*  Private functions follow  */

static flag open_func (connection, info)
/*  This routine will register the opening of a connection.
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on successful registration,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will not be called if this routine returns
    FALSE.
*/
Connection connection;
void **info;
{
    Channel channel;
    Channel fch;
    unsigned long response;
    unsigned long rerrno;
    unsigned long length;
    unsigned long mode;
    unsigned long mtime;
    char dummy;
    float wall_clock_time_taken;
    float transfer_rate = 0.0;
    struct stat statbuf;
    struct timeval start_time;
    struct timeval stop_time;
    char *ptr;
    struct utimbuf ut;
    extern flag failure;
    extern char *sys_errlist[];
    static struct timezone tz = {0, 0};

    channel = conn_get_channel (connection);
    if ( !pio_write32 (channel, operation) ) return (FALSE);
    switch (operation)
    {
      case KFTP_REQUEST_SEND:
	if ( !pio_write_string (channel, remotefile) ) return (FALSE);
	if (stat (localfile, &statbuf) != 0)
	{
	    fprintf (stderr,
			    "Error getting stats on file: \"%s\"\t%s\n",
			    localfile, sys_errlist[errno]);
	    return (FALSE);
	}
	length = statbuf.st_size;
	if ( ( fch = ch_open_file (localfile, "r") ) == NULL )
	{
	    fprintf (stderr, "Error opening file: \"%s\"\t%s\n",
			    localfile, sys_errlist[errno]);
	    return (FALSE);
	}
	if ( !pio_write32 (channel, length) )
	{
	    ch_close (fch);
	    return (FALSE);
	}
	if ( !pio_write32 (channel, statbuf.st_mode) )
	{
	    ch_close (fch);
	    return (FALSE);
	}
	if ( !pio_write32 (channel, statbuf.st_mtime) )
	{
	    ch_close (fch);
	    return (FALSE);
	}
	if ( !ch_flush (channel) )
	{
	    ch_close (fch);
	    return (FALSE);
	}
	if ( !pio_read32 (channel, &response) )
	{
	    ch_close (fch);
	    return (FALSE);
	}
	if (response != KFTP_RESPONSE_OK)
	{
	    ch_close (fch);
	    if ( !pio_read32 (channel, &rerrno) ) return (FALSE);
	    fprintf (stderr, "Remote error: %s\n",
			    sys_errlist[rerrno]);
	    return (FALSE);
	}
	if (gettimeofday (&start_time, &tz) != 0)
	{
	    fprintf (stderr, "Error getting time of day\t%s%c\n",
			    sys_errlist[errno], BEL);
	    exit (RV_SYS_ERROR);
	}
	if ( !copy_channels (channel, fch, length) )
	{
	    ch_close (fch);
	    return (FALSE);
	}
	ch_close (fch);
	ch_flush (channel);
	if (ch_read (channel, &dummy, 1) < 1)
	{
	    ch_close (fch);
	    return (FALSE);
	}
	if (gettimeofday (&stop_time, &tz) != 0)
	{
	    fprintf (stderr, "Error getting time of day\t%s%c\n",
			    sys_errlist[errno], BEL);
	    exit (RV_SYS_ERROR);
	}
	wall_clock_time_taken = (float)(stop_time.tv_usec -start_time.tv_usec);
	wall_clock_time_taken /= 1e3;
	wall_clock_time_taken += 1e3 * (stop_time.tv_sec - start_time.tv_sec);
	transfer_rate = (float) length / wall_clock_time_taken;
	failure = FALSE;
	break;
      case KFTP_REQUEST_GET:
	if ( !pio_write_string (channel, remotefile) ) return (FALSE);
	if ( !ch_flush (channel) ) return (FALSE);
	if ( !pio_read32 (channel, &response) ) return (FALSE);
	if (response != KFTP_RESPONSE_OK)
	{
	    if ( !pio_read32 (channel, &rerrno) ) return (FALSE);
	    fprintf (stderr, "Remote error: %s\n",
			    sys_errlist[rerrno]);
	    return (FALSE);
	}
	if ( !pio_read32 (channel, &length) ) return (FALSE);
	if ( !pio_read32 (channel, &mode) ) return (FALSE);
	if ( !pio_read32 (channel, &mtime) ) return (FALSE);
	if ( (stat (localfile, &statbuf) == 0) && S_ISDIR (statbuf.st_mode) )
	{
	    ptr = strrchr (remotefile, '/');
	    ptr = (ptr == NULL) ? remotefile : ptr + 1;
	    strcat (localfile, "/");
	    strcat (localfile, ptr);
	}
	if (strcmp (localfile, "/dev/null") == 0)
	{
	    if ( ( fch = ch_create_sink () ) == NULL )
	    {
		fprintf (stderr,"Error opening file: \"%s\" for writing\t%s\n",
			 localfile, sys_errlist[errno]);
		return (FALSE);
	    }
	}
	else
	{
	    if ( ( fch = ch_open_file (localfile, "w") ) == NULL )
	    {
		fprintf (stderr,
				"Error opening file: \"%s\" for writing\t%s\n",
				localfile, sys_errlist[errno]);
		return (FALSE);
	    }
	    if (fchmod (ch_get_descriptor (fch), mode) != 0)
	    {
		return (FALSE);
	    }
	}
	if (gettimeofday (&start_time, &tz) != 0)
	{
	    fprintf (stderr, "Error getting time of day\t%s%c\n",
			    sys_errlist[errno], BEL);
	    exit (RV_SYS_ERROR);
	}
	if ( !copy_channels (fch, channel, length) ) return (FALSE);
	if (gettimeofday (&stop_time, &tz) != 0)
	{
	    fprintf (stderr, "Error getting time of day\t%s%c\n",
			    sys_errlist[errno], BEL);
	    exit (RV_SYS_ERROR);
	}
	wall_clock_time_taken = (float)(stop_time.tv_usec -start_time.tv_usec);
	wall_clock_time_taken /= 1e3;
	wall_clock_time_taken += 1e3 * (stop_time.tv_sec - start_time.tv_sec);
	transfer_rate = (float) length / wall_clock_time_taken;
	ch_close (fch);
	ut.actime = mtime;
	ut.modtime = mtime;
	utime (localfile, &ut);
	failure = FALSE;
	break;
      default:
	break;
    }
    if (!failure)
    {
	fprintf (stderr, "%e kBytes/sec transferred\n", transfer_rate);
    }
    return (FALSE);
}   /*  End Function open_func  */

static flag copy_channels (dest, source, length)
/*  [PURPOSE] This routine will copy bytes from one channel to another.
    <dest> The destination channel.
    <source> The source channel.
    <length> The number of bytes to copy.
    [RETURNS] TRUE on success, else FALSE.
*/
Channel dest;
Channel source;
unsigned int length;
{
    unsigned int blk_len;
    float bytes_per_dot;
    unsigned int bytes_transferred;
    unsigned int dot_pos;
    char buffer[BUF_SIZE];

    bytes_per_dot = (float) length / 80.0;
    fprintf (stderr,
	     "Transferring: %u bytes. Each '.' is: %u bytes (1/80th file size).\n",
	     length, (unsigned int) bytes_per_dot);
    dot_pos = 0;
    bytes_transferred = 0;
    while (length > 0)
    {
	blk_len = (length > BUF_SIZE) ? BUF_SIZE : length;
	if ( blk_len > (int) (bytes_per_dot / 2.0) )
	{
	    blk_len = (bytes_per_dot / 2.0);
	}
	if (ch_read (source, buffer, blk_len) < blk_len) return (FALSE);
	if (ch_write (dest, buffer, blk_len) < blk_len) return (FALSE);
	length -= blk_len;
	bytes_transferred += blk_len;
	while ( (float) dot_pos * bytes_per_dot < bytes_transferred )
	{
	    fprintf (stderr, ".");
	    ++dot_pos;
	}
    }
    fprintf (stderr, "\n");
    return (TRUE);
}   /*  End Function copy_channels  */

/*LINTLIBRARY*/
/*PREFIX:"notify_chm_*/
/*  notify_chm.c

    This code provides Channel Management routines for the XView toolkit.

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

/*  This file contains all routines needed for the management of channel
  objects using the XView notifier.


    Written by      Richard Gooch   22-SEP-1992

    Updated by      Richard Gooch   4-DEC-1992

    Updated by      Richard Gooch   31-DEC-1992: Added call to  close_func  if
  event functions return FALSE.

    Last updated by Richard Gooch   24-JUL-1993: Made use of new
  ch_test_for_io  routine.


*/
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
#include <karma.h>
#include <karma_notify_chm.h>
#include <karma_ch.h>
#include <karma_m.h>
#include <karma_a.h>
#include <xview/xview.h>

#define NOTIFY_INPUT_CLIENT (Notify_client) 20
#define NOTIFY_OUTPUT_CLIENT (Notify_client) 21
#define NOTIFY_EXCEPTION_CLIENT (Notify_client) 22
#define NOTIFY_CHILD_CLIENT (Notify_client) 23

/*  Private functions  */
static Notify_value notify_sig_io_handler (/* client, fd */);
static Notify_value notify_sig_child_handler (/* client, pid, status,
						 rusage */);
static flag read_channel ();
static void close_channel ();

/*  Structure and private data to manage channels  */
struct managed_channel_type
{
    Channel channel;
    int fd;
    void *info;
    flag (*input_func) ();
    void (*close_func) ();
    flag (*output_func) ();
    flag (*exception_func) ();
    struct managed_channel_type *next;
    struct managed_channel_type *prev;
};
static struct managed_channel_type *managed_channel_list = NULL;

/*  Private functions which communicate with XView  */

static Notify_value notify_sig_io_handler (client, fd)
/*  This routine will handle  activity on a file descriptor.
    The Notifier client is given by  client  .
    The file descriptor is given by  fd  .
    The routine returns  NOTIFY_DONE  .
*/
Notify_client client;
int fd;
{
    int bytes_readable;
    struct managed_channel_type *entry;
    char drain_buffer[1];
    extern struct managed_channel_type *managed_channel_list;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "notify_sig_io_handler";

    /*  Now call routine which has an active descriptor  */
    for (entry = managed_channel_list; entry != NULL; entry = (*entry).next)
    {
	if (fd == (*entry).fd)
	{
	    /*  Found it  */
	    if (client == NOTIFY_EXCEPTION_CLIENT)
	    {
		/*  Exception occurred  */
		if ( (* (*entry).exception_func ) ( (*entry).channel,
						   &(*entry).info ) != TRUE )
		{
		    /*  Channel to be unmanaged and closed  */
		    close_channel (entry);
		    return (NOTIFY_DONE);
		}
	    }
	    else if (client == NOTIFY_INPUT_CLIENT)
	    {
		/*  Input/ closure occurred  */
		if (read_channel (entry) != TRUE)
		{
		    /*  Close and unmanage  */
		    close_channel (entry);
		    return (NOTIFY_DONE);
		}
	    }
	    else if (client == NOTIFY_OUTPUT_CLIENT)
	    {
		if ( (* (*entry).output_func ) ( (*entry).channel,
						&(*entry).info )
		    != TRUE )
		{
		    /*  Channel to be unmanaged and closed  */
		    close_channel (entry);
		    return (NOTIFY_DONE);
		}
	    }
	    return (NOTIFY_DONE);
	}
    }
    (void) fprintf (stderr, "Descriptor: %d not managed\n", fd);
    a_prog_bug (function_name);
}   /*  End Function notify_sig_io_handler  */

static flag read_channel (entry)
/*  This routine will read a channel (if appropriate) until all data is drained
    from the descriptor and the buffer.
    The channel entry must be pointed to by  entry  .
    The routine returns TRUE if the channel is to remain managed,
    else it returns FALSE.
*/
struct managed_channel_type *entry;
{
    int bytes_readable;
    char drain_buffer[1];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "read_channel";

    if ( (ch_test_for_io ( (*entry).channel ) == TRUE) ||
	( (*entry).close_func != NULL ) )
    {
	/*  Can test for input/ closure  */
	/*  Test descriptor, not channel, so as to ensure detection
	    of closure  */
	if ( ( bytes_readable =
	      r_get_bytes_readable ( (*entry).fd ) ) < 0 )
	{
	    (void) exit (RV_SYS_ERROR);
	}
    }
    else
    {
	/*  Fake "read" input  */
	bytes_readable = 1;
    }
    if (bytes_readable < 1)
    {
	/*  Closure occurred  */
	/*  Channel drainable  */
	/*  Drain data (close event)  */
	switch ( r_read ( (*entry).fd, drain_buffer, 1 ) )
	{
	  case -1:
	    /*  Error  */
	    (void) fprintf (stderr,
			    "Error draining descriptor\t%s\n",
			    sys_errlist[errno]);
	    break;
	  case 0:
	    /*  This is what we expect  */
	    break;
	  case 1:
	    (void) fprintf (stderr,
			    "Data readable on closed descriptor\n");
	    a_prog_bug (function_name);
	    break;
	}
	return (FALSE);
    }
    /*  Input/ closure event occurred  */
    if (ch_test_for_io ( (*entry).channel ) != TRUE)
    {
	/*  Not bufferable  */
	if ( (*entry).input_func == NULL )
	{
	    (void) fprintf (stderr,
			    "Input on asynchronous channel not being read\n");
	    a_prog_bug (function_name);
	}
	return ( (* (*entry).input_func ) ( (*entry).channel,
					   &(*entry).info ) );
    }
    /*  Buffered connection  */
    if ( (*entry).input_func == NULL )
    {
	(void) fprintf (stderr, "Input on channel not being read\n");
	a_prog_bug (function_name);
    }
    while (bytes_readable > 0)
    {
	/*  More input  */
	if ( (* (*entry).input_func ) ( (*entry).channel, &(*entry).info )
	    != TRUE )
	{
	    /*  Channel to be unmanaged and closed  */
	    return (FALSE);
	}
	/*  Test channel, not descriptor, to ensure channel read buffer as
	    well as descriptor drained  */
	if ( ( bytes_readable =
	      ch_get_bytes_readable ( (*entry).channel ) ) < 0 )
	{
	    (void) exit (RV_SYS_ERROR);
	}
    }
    return (TRUE);
}   /*  End Function read_channel  */

static void close_channel (entry)
/*  This routine will call the registered  close_func  for a channel, and will
    then close and unmanage the channel.
    The channel entry must be pointed to by  entry  .
    The routine returns TRUE if the channel is to remain managed,
    else it returns FALSE.
*/
struct managed_channel_type *entry;
{
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "open_channel";

    if ( (*entry).close_func != NULL )
    {
	(* (*entry).close_func ) ( (*entry).channel,
				  (*entry).info );
    }
    (void) ch_close ( (*entry).channel );
    notify_chm_unmanage ( (*entry).channel );
}   /*  End Function close_channel  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag notify_chm_manage (channel, info, input_func, close_func, output_func,
			exception_func)
/*  This routine will manage a channel for activity by registering callback
    routines.
    The channel object to manage must be given by  channel  .
    An arbitrary pointer may associated with the managed channel. This pointer
    may be modified by the callback routines. The initial value of this pointer
    must be given by  info  .
    The routine which is called when new input occurrs on the channel must
    be pointed to by  input_func  .If this is NULL, no callback routine is
    installed. The interface to this routine is as follows:

    flag input_func (channel, info)
    *   This routine is called when new input occurs on a channel.
        The channel object is given by  channel  .
	An arbitrary pointer may be written to the storage pointed to by  info
	The pointer written here will persist until the channel is unmanaged
	(or a subsequent callback routine changes it).
	The routine returns TRUE if the channel is to remain managed and
	open, else it returns FALSE (indicating that the channel is to be
	unmanaged and closed). This routine MUST NOT unmanage or close the
	channel given by  channel  .
	Note that the  close_func  will be called if this routine returns FALSE
    *
    Channel channel;
    void **info;

    The routine which is called when the channel closes must be pointed to by
    close_func  .If this is NULL, no callback routine is installed. The
    interface to this routine is as follows:

    void close_func (channel, info)
    *   This routine is called when a channel closes.
        The channel object is given by  channel  .The channel object MUST be
        capable of detecting closure if this routine is supplied (ie. this
	routine cannot be supplied for dock channels).
        The arbitrary pointer for the channel will be pointed to by  info  .
	This routine MUST NOT unmanage the channel pointed to by  channel  ,
	the channel will be automatically unmanaged and deleted upon closure
	(even if no close_func is specified).
	Any unread buffered data in the channel will be lost upon closure. The
	call to this function is the last chance to read this buffered data.
	The routine returns nothing.
    *
    Channel channel;
    void *info;

    The routine which is called when the channel becomes ready for output
    must be pointed to by  output_func  .If this is NULL, no callback routine
    is installed. The interface to this routine is as follows:

    flag output_func (channel, info)
    *   This routine is called when a channel becomes ready for writing.
	The channel object is given by  channel  .
	An arbitrary pointer may be written to the storage pointed to by  info
	The pointer written here will persist until the channel is unmanaged
	(or a subsequent callback routine changes it).
	The routine returns TRUE if the channel is to remain managed and
	open, else it returns FALSE (indicating that the channel is to be
	unmanaged and closed). This routine MUST NOT unmanage or close the
	channel given by  channel  .
	Note that the  close_func  will be called if this routine returns FALSE
    *
    Channel channel;
    void **info;

    The routine which is called when exceptions occurr on the channel must
    be pointed to by  exception_func  .If this is NULL, no callback routine is
    installed. The interface to this routine is as follows:

    flag exception_func (channel, info)
    *   This routine is called when an exception occurrs on channel.
	The channel object is given by  channel  .
	An arbitrary pointer may be written to the storage pointed to by  info
	The pointer written here will persist until the channel is unmanaged
	(or a subsequent callback routine changes it).
	The routine returns TRUE if the channel is to remain managed and
	open, else it returns FALSE (indicating that the channel is to be
	unmanaged and closed). This routine MUST NOT unmanage or close the
	channel given by  channel  .
	Note that the  close_func  will be called if this routine returns FALSE
    *
    Channel channel;
    void **info;

    The routine returns TRUE on success, else it returns FALSE.
*/
Channel channel;
void *info;
flag (*input_func) ();
void (*close_func) ();
flag (*output_func) ();
flag (*exception_func) ();
{
    int fd_flags;
    int fd;
    struct managed_channel_type *entry;
    struct managed_channel_type *new_entry;
    struct managed_channel_type *last_entry;
    extern struct managed_channel_type *managed_channel_list;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "notify_chm_manage";

    /*  Check if channel is a connection  */
    if (ch_test_for_asynchronous (channel) != TRUE)
    {
	(void) fprintf (stderr,
			"Cannot manage a channel if it is not a connection\n");
	a_prog_bug (function_name);
    }
    /*  Get file descriptor  */
    if ( ( fd = ch_get_descriptor (channel) ) < 0 )
    {
	(void) fprintf (stderr,
			"Error getting file descriptor for channel object\n");
	return (FALSE);
    }
    if (close_func != NULL)
    {
	/*  Check if closure detectable  */
	if (r_get_bytes_readable (fd) < 0)
	{
	    (void) fprintf (stderr,
			    "close_func  supplied and closure not detectable\n");
	    a_prog_bug (function_name);
	}
    }
    /*  See if channel is already managed  */
    for (entry = managed_channel_list; entry != NULL; entry = (*entry).next)
    {
	if (channel == (*entry).channel)
	{
	    (void) fprintf (stderr, "Channel: %x is already managed\n",
			    channel);
	    a_prog_bug (function_name);
	}
	last_entry = entry;
    }
    /*  Channel is not already managed  */
    /*  Allocate new entry  */
    if ( ( new_entry =
	  (struct managed_channel_type *) m_alloc (sizeof *new_entry) )
	== NULL )
    {
	m_error_notify (function_name, "new managed descriptor entry");
	return (FALSE);
    }
    /*  Fill in entry  */
    (*new_entry).channel = channel;
    (*new_entry).fd = fd;
    (*new_entry).info = info;
    (*new_entry).input_func = input_func;
    (*new_entry).close_func = close_func;
    (*new_entry).output_func = output_func;
    (*new_entry).exception_func = exception_func;
    (*new_entry).next = NULL;
    (*new_entry).prev = NULL;
    if (managed_channel_list == NULL)
    {
	/*  Create new list  */
	managed_channel_list = new_entry;
    }
    else
    {
	/*  Append to end of list  */
	(*last_entry).next = new_entry;
	(*new_entry).prev = last_entry;
    }
    /*  Tell the Notifier  */
    if ( (input_func != NULL) || (close_func != NULL) )
    {
	notify_set_input_func (NOTIFY_INPUT_CLIENT, notify_sig_io_handler, fd);
    }
    if (output_func != NULL)
    {
	notify_set_output_func (NOTIFY_OUTPUT_CLIENT, notify_sig_io_handler,
				fd);
    }
    if (exception_func != NULL)
    {
	notify_set_exception_func (NOTIFY_EXCEPTION_CLIENT,
				   notify_sig_io_handler, fd);
    }
    return (TRUE);
}   /*  End Function notify_chm_manage  */

/*PUBLIC_FUNCTION*/
void notify_chm_unmanage (channel)
/*  This routine will terminate the management of a channel for activity.
    The channel to unmanage must be given by  channel  .
    The routine will NOT close the channel (nor does it assume the channel is
    open).
    The routine returns nothing.
*/
Channel channel;
{
    struct managed_channel_type *entry;
    extern struct managed_channel_type *managed_channel_list;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "notify_chm_unmanage";

    for (entry = managed_channel_list; entry != NULL; entry = (*entry).next)
    {
	if (channel == (*entry).channel)
	{
	    /*  Remove entry  */
	    notify_set_input_func (NOTIFY_INPUT_CLIENT, NOTIFY_FUNC_NULL,
				   (*entry).fd);
	    notify_set_output_func (NOTIFY_OUTPUT_CLIENT, NOTIFY_FUNC_NULL,
				    (*entry).fd);
	    notify_set_exception_func (NOTIFY_EXCEPTION_CLIENT,
				       NOTIFY_FUNC_NULL, (*entry).fd);
	    if ( (*entry).prev == NULL )
	    {
		/*  Entry is first in the list  */
		managed_channel_list = (*entry).next;
	    }
	    else
	    {
		/*  Previous entry exists  */
		(* (*entry).prev ).next = (*entry).next;
	    }
	    if ( (*entry).next != NULL )
	    {
		/*  Next entry exists  */
		(* (*entry).next ).prev = (*entry).prev;
	    }
	    m_free ( (char *) entry );
	    return;
	}
    }
    /*  Channel not found  */
    (void) fprintf (stderr, "Channel: %x not managed\n", channel);
    a_prog_bug (function_name);
}   /*  End Function notify_chm_unmanage  */

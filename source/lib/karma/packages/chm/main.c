/*LINTLIBRARY*/
/*  main.c

    This code provides Channel Management routines.

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

/*  This file contains all routines needed for the management of channel
  objects.


    Written by      Richard Gooch   12-SEP-1992

    Updated by      Richard Gooch   14-NOV-1992

    Updated by      Richard Gooch   15-DEC-1992: Added complete read processing

    Updated by      Richard Gooch   19-DEC-1992: Tested for  chm_poll re-entry.

    Updated by      Richard Gooch   28-DEC-1992: Added closure detection for
  asynchronous channels.

    Updated by      Richard Gooch   31-DEC-1992: Added call to  close_func  if
  event functions return FALSE.

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   24-JAN-1993: Disabled code which sets
  descriptor to send SIGIO, since it was not required.

    Updated by      Richard Gooch   24-JUL-1993: Made use of new
  ch_test_for_io  routine.

    Updated by      Richard Gooch   3-OCT-1993: Improved diagnostics in
  read_channel  .

    Updated by      Richard Gooch   2-NOV-1994: Changed from  rs6000  to
  rs6000_AIX

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/chs/main.c

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno

    Updated by      Richard Gooch   2-JAN-1995: Fixed some comments.

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Updated by      Richard Gooch   1-APR-1996: Moved remaing functions to new
  documentation style.

    Last updated by Richard Gooch   2-DEC-1996: Added scheduling of work
  functions.


*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <errno.h>
#include <os.h>
#ifdef HAS_SOCKETS
#  include <sys/stat.h>
#  include <sys/time.h>
#  include <fcntl.h>
#  include <sys/ioctl.h>
#  ifdef OS_AIX
#    include <sys/select.h>
#  endif
#endif
#ifdef OS_VXMVX
#  include <vx/vx.h>
#endif
#include <karma.h>
#include <karma_chm.h>
#include <karma_ch.h>
#include <karma_wf.h>
#include <karma_m.h>
#include <karma_a.h>
#include <karma_r.h>
#include <karma_e.h>

#if defined(HAS_SOCKETS) || defined(HAS_COMMUNICATIONS_EMULATION)
#define COMMUNICATIONS_AVAILABLE
#endif

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


/*  Private functions  */
static flag read_channel ();
static void close_channel ();


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag chm_manage ( Channel channel, void *info, flag (*input_func) (),
		  void (*close_func) (), flag (*output_func) (),
		  flag (*exception_func) () )
/*  [SUMMARY] Manage a channel for activity by registering callback routines.
    <channel> The channel object to manage.
    <info> The arbitrary information pointer associated with the managed
    channel. This pointer may be modified by the callback routines.
    <input_func> This routine is called when new input occurs on the channel.
    If this is NULL, no callback routine is installed. The prototype function
    is [<CHM_PROTO_input_func>].
    <close_func> This routine is called when the channel closes. If this is
    NULL, no callback routine is installed. The prototype function is
    [<CHM_PROTO_close_func>]. The channel object MUST be capable of detecting
    closure if this routine is supplied (i.e. this routine cannot be supplied
    for dock channels). Any unread buffered data in the channel will be lost
    upon closure. The call to this function is the last chance to read this
    buffered data.
    <output_func> This routine is called when the channel becomes ready for
    output. If this is NULL, no callback routine is installed. The prototype
    function is [<CHM_PROTO_output_func>].
    <exception_func> This routine is called when exceptions occur on the
    channel. If this is NULL, no callback routine is installed. The prototype
    function is [<CHM_PROTO_exception_func>].
    [RETURNS] TRUE on success, else FALSE.
*/
{
#ifdef COMMUNICATIONS_AVAILABLE
#  ifdef HAS_SOCKETS___dummy
    int fd_flags;
#  endif
    int fd;
    struct managed_channel_type *entry;
    struct managed_channel_type *new_entry;
    struct managed_channel_type *last_entry = NULL; /*  Init. for gcc -Wall  */
    extern struct managed_channel_type *managed_channel_list;
    /*extern char *sys_errlist[];*/
    static char function_name[] = "chm_manage";

    /*  Check if channel is a connection  */
    if (ch_test_for_asynchronous (channel) != TRUE)
    {
	fprintf (stderr,"Cannot manage a channel if it is not asynchronous\n");
	a_prog_bug (function_name);
    }
    /*  Get file descriptor  */
    if ( ( fd = ch_get_descriptor (channel) ) < 0 )
    {
	fprintf (stderr, "Error getting file descriptor for channel object\n");
	return (FALSE);
    }
    if (close_func != NULL)
    {
	/*  Check if closure detectable  */
	if (r_get_bytes_readable (fd) < 0)
	{
	    fprintf (stderr,
		     "close_func  supplied and closure not detectable\n");
	    a_prog_bug (function_name);
	}
    }
    /*  See if channel is already managed  */
    for (entry = managed_channel_list; entry != NULL; entry = entry->next)
    {
	if (channel == entry->channel)
	{
	    fprintf (stderr, "Channel: %p is already managed\n", channel);
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
    new_entry->channel = channel;
    new_entry->fd = fd;
    new_entry->info = info;
    new_entry->input_func = input_func;
    new_entry->close_func = close_func;
    new_entry->output_func = output_func;
    new_entry->exception_func = exception_func;
    new_entry->next = NULL;
    new_entry->prev = NULL;
#  ifdef HAS_SOCKETS___dummy
    /*  Set process ID which will receive SIGIO signals  */
    if (fcntl ( fd, F_SETOWN, getpid () ) == -1)
    {
	fprintf (stderr, "Error setting owner PID for descriptor: %d\t%s\n",
		 fd, sys_errlist[errno]);
	m_free ( (char *) new_entry );
	return (FALSE);
    }
    /*  Set descriptor to send SIGIO events  */
    if ( ( fd_flags = fcntl (fd, F_GETFL, NULL) ) == -1 )
    {
	fprintf (stderr, "Error getting flags for descriptor: %d\t%s\n",
		 fd, sys_errlist[errno]);
	m_free ( (char *) new_entry );
	return (FALSE);
    }
    fd_flags |= FASYNC;
    if (fcntl (fd, F_SETFL, fd_flags) == -1)
    {
	fprintf (stderr, "Error setting flags for descriptor: %d\t%s\n",
		 fd, sys_errlist[errno]);
	m_free ( (char *) new_entry );
	return (FALSE);
    }
#  endif  /*  HAS_SOCKETS  */
    /*  Everything fine: add to list  */
    if (managed_channel_list == NULL)
    {
	/*  Create new list  */
	managed_channel_list = new_entry;
    }
    else
    {
	/*  Append to end of list  */
	last_entry->next = new_entry;
	new_entry->prev = last_entry;
    }
    return (TRUE);

#else  /*  COMMUNICATIONS_AVAILABLE  */
    fprintf (stderr, "Communications support not available\n");
    return (FALSE);
#endif  /*  COMMUNICATIONS_AVAILABLE  */
}   /*  End Function chm_manage  */

/*PUBLIC_FUNCTION*/
void chm_unmanage (Channel channel)
/*  [SUMMARY] Terminate the management of a channel for activity.
    <channel> The channel object to unmanage.
    [NOTE] This routine will NOT close the channel (nor does it assume the
    channel is open).
    [RETURNS] Nothing.
*/
{
    struct managed_channel_type *entry;
    extern struct managed_channel_type *managed_channel_list;
    static char function_name[] = "chm_unmanage";

    for (entry = managed_channel_list; entry != NULL; entry = entry->next)
    {
	if (channel == entry->channel)
	{
	    /*  Remove entry  */
	    if (entry->prev == NULL)
	    {
		/*  Entry is first in the list  */
		managed_channel_list = entry->next;
	    }
	    else
	    {
		/*  Previous entry exists  */
		entry->prev->next = entry->next;
	    }
	    if (entry->next != NULL)
	    {
		/*  Next entry exists  */
		entry->next->prev = entry->prev;
	    }
	    m_free ( (char *) entry );
	    return;
	}
    }
    /*  Channel not found  */
    fprintf (stderr, "Channel: %p not managed\n", channel);
    a_prog_bug (function_name);
}   /*  End Function chm_unmanage  */

/*PUBLIC_FUNCTION*/
void chm_poll (long timeout_ms)
/*  [SUMMARY] Poll all managed channels for any activity.
    <timeout_ms> The time (in milliseconds) to poll. If this is less than 0 the
    routine will poll forever (until some activity occurs or a signal is
    caught).
    [RETURNS] Nothing.
*/
{
#ifdef COMMUNICATIONS_AVAILABLE
#  ifdef HAS_SOCKETS
    fd_set input_fds, output_fds, exception_fds;
    struct timeval timeout;
    struct timeval *timeout_ptr;
#  endif  /*  HAS_SOCKETS  */
    struct managed_channel_type *entry;
    struct managed_channel_type *next_entry;
    extern struct managed_channel_type *managed_channel_list;
    extern char *sys_errlist[];
    static flag locked = FALSE;
    static char function_name[] = "chm_poll";

    if (locked)
    {
	fprintf (stderr, "Code non-reentrant\n");
	a_prog_bug (function_name);
    }
    locked = TRUE;
    if ( wf_work_to_be_done () )
    {
	timeout_ms = 0;
	wf_do_work ();
    }
#  ifdef HAS_SOCKETS
    FD_ZERO (&input_fds);
    FD_ZERO (&output_fds);
    FD_ZERO (&exception_fds);
    for (entry = managed_channel_list; entry != NULL; entry = entry->next)
    {
	/*  Add descriptor to list to check  */
	/*  Always monitor input  */
	FD_SET (entry->fd, &input_fds);
	if (entry->output_func != NULL)
	{
	    /*  Monitor output  */
	    FD_SET (entry->fd, &output_fds);
	}
	if (entry->exception_func != NULL)
	{
	    /*  Monitor exceptions  */
	    FD_SET (entry->fd, &exception_fds);
	}
    }
    /*  Now call  select(2)  routine  */
    if (timeout_ms < 0)
    {
	/*  Wait forever for something to happen  */
	timeout_ptr = NULL;
    }
    else
    {
	/*  Block for a finite time  */
	timeout_ptr = &timeout;
	timeout.tv_sec = (unsigned long) timeout_ms / 1000;
	timeout_ms -= (long) (timeout.tv_sec * 1000);
	timeout.tv_usec = (unsigned long) timeout_ms * 1000;
    }
    switch ( select (FD_SETSIZE, &input_fds, &output_fds, &exception_fds,
		     timeout_ptr) )
    {
      case 0:
	locked = FALSE;
	return;
	/*break;*/
      case -1:
	if (errno == EINTR)
	{
	    locked = FALSE;
	    e_unix_dispatch_events (DISPATCH_SYNCHRONOUS);
	    return;
	}
	/*  Failure  */
	fprintf (stderr, "Error calling  select(2)\t%s\n", sys_errlist[errno]);
	/*return;*/
	break;
      default:
	/*  Success  */
	break;
    }
#  endif  /*  HAS_SOCKETS  */

    /*  Now see which descriptor is active  */
    for (entry = managed_channel_list; entry != NULL; entry = next_entry)
    {
	next_entry = entry->next;
#  ifdef HAS_SOCKETS
	if (entry->exception_func != NULL)
	{
	    /*  Monitor exceptions  */
	    if (FD_ISSET (entry->fd, &exception_fds) != 0)
	    {
		/*  Exception occurred  */
		if ( !(*entry->exception_func) (entry->channel, &entry->info) )
		{
		    /*  Channel to be unmanaged and closed  */
		    close_channel (entry);
		    continue;
		}
	    }
	}
#  endif  /*  HAS_SOCKETS  */
	/*  Monitor input  */
#  ifdef HAS_SOCKETS
	if (FD_ISSET (entry->fd, &input_fds) != 0)
#  else  /*  HAS_SOCKETS  */
	if ( r_test_input_event (entry->fd) )
#  endif  /*  HAS_SOCKETS  */
	{
	    /*  Input/ closure occurred  */
	    if (read_channel (entry) != TRUE)
	    {
		/*  Close and unmanage  */
		close_channel (entry);
		continue;
	    }
	}
#  ifdef HAS_SOCKETS
	if (entry->output_func != NULL)
	{
	    /*  Monitor output  */
	    if (FD_ISSET (entry->fd, &output_fds) != 0)
	    {
		if ( !(*entry->output_func) (entry->channel, &entry->info) )
		{
		    /*  Channel to be unmanaged and closed  */
		    close_channel (entry);
		    continue;
		}
	    }
	}
#  endif  /*  HAS_SOCKETS  */
    }
    /*  Processed all activity (we hope)  */
#  ifdef OS_VXMVX
    if (timeout_ms > 0)
    {
	task_sleep (timeout_ms);
    }
#  endif  /*  OS_VXMVX  */

#else  /*  COMMUNICATIONS_AVAILABLE  */
    fprintf (stderr, "Operating system does not support communications\n");
#endif  /*  COMMUNICATIONS_AVAILABLE  */
    locked = FALSE;
}   /*  End Function chm_poll  */


/*  Private functions follow  */

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
    extern char *sys_errlist[];
    static char function_name[] = "read_channel";

    if ( ch_test_for_io (entry->channel) || (entry->close_func != NULL) )
    {
	/*  Can test for input/ closure  */
	/*  Test descriptor, not channel, so as to ensure detection
	    of closure  */
	if ( ( bytes_readable = r_get_bytes_readable (entry->fd) ) < 0 )
	{
	    exit (RV_SYS_ERROR);
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
	switch ( r_read (entry->fd, drain_buffer, 1) )
	{
	  case -1:
	    /*  Error  */
	    fprintf (stderr, "Error draining descriptor\t%s\n",
			    sys_errlist[errno]);
	    break;
	  case 0:
	    /*  This is what we expect  */
	    break;
	  case 1:
	    fprintf (stderr, "Data readable on closed descriptor\n");
	    a_prog_bug (function_name);
	    break;
	}
	return (FALSE);
    }
    /*  Input/ closure event occurred  */
    if (ch_test_for_io (entry->channel) != TRUE)
    {
	/*  Not bufferable  */
	if (entry->input_func == NULL)
	{
	    fprintf (stderr,
			    "Input on asynchronous channel but no callback\n");
	    a_prog_bug (function_name);
	}
	return ( (*entry->input_func) (entry->channel, &entry->info) );
    }
    /*  Buffered connection  */
    if (entry->input_func == NULL)
    {
	fprintf (stderr, "Input on channel not being drained\n");
	a_prog_bug (function_name);
    }
    while (bytes_readable > 0)
    {
	/*  More input  */
	if ( !(*entry->input_func) (entry->channel, &entry->info) )
	{
	    /*  Channel to be unmanaged and closed  */
	    return (FALSE);
	}
	/*  Test channel, not descriptor, to ensure channel read buffer as
	    well as descriptor drained  */
	if ( ( bytes_readable = ch_get_bytes_readable (entry->channel) ) < 0 )
	{
	    exit (RV_SYS_ERROR);
	}
    }
    return (TRUE);
}   /*  End Function read_channel  */

static void close_channel (entry)
/*  This routine will call the registered  close_func  for a channel, and will
    then close and unmanage the channel.
    The channel entry must be pointed to by  entry  .
    The routine returns nothing.
*/
struct managed_channel_type *entry;
{
    /*static char function_name[] = "close_channel";*/

    if (entry->close_func != NULL)
    {
	(*entry->close_func) (entry->channel, entry->info);
    }
    ch_close (entry->channel);
    chm_unmanage (entry->channel);
}   /*  End Function close_channel  */

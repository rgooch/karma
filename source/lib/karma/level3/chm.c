/*LINTLIBRARY*/
/*PREFIX:"chm_"*/
/*  chm.c

    This code provides Channel Management routines.

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

    Last updated by Richard Gooch   24-JUL-1993: Made use of new
  ch_test_for_io  routine.


*/
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <os.h>
#ifdef HAS_SOCKETS
#  include <sys/stat.h>
#  include <sys/time.h>
#  include <errno.h>
#  include <fcntl.h>
#  include <sys/ioctl.h>
#  ifdef ARCH_rs6000
#    include <sys/select.h>
#  endif
#endif
#ifdef ARCH_VXMVX
#  include <vx/vx.h>
#endif
#include <karma.h>
#include <karma_chm.h>
#include <karma_ch.h>
#include <karma_m.h>
#include <karma_a.h>

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
flag chm_manage (channel, info, input_func, close_func, output_func,
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
#ifdef COMMUNICATIONS_AVAILABLE
    int fd_flags;
    int fd;
    struct managed_channel_type *entry;
    struct managed_channel_type *new_entry;
    struct managed_channel_type *last_entry;
    extern struct managed_channel_type *managed_channel_list;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "chm_manage";

    /*  Check if channel is a connection  */
    if (ch_test_for_asynchronous (channel) != TRUE)
    {
	(void) fprintf (stderr,
			"Cannot manage a channel if it is not asynchronous\n");
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
#  ifdef HAS_SOCKETS___dummy
    /*  Set process ID which will receive SIGIO signals  */
    if (fcntl ( fd, F_SETOWN, getpid () ) == -1)
    {
	(void) fprintf (stderr,
			"Error setting owner PID for descriptor: %d\t%s\n",
			fd, sys_errlist[errno]);
	m_free ( (char *) new_entry );
	return (FALSE);
    }
    /*  Set descriptor to send SIGIO events  */
    if ( ( fd_flags = fcntl (fd, F_GETFL, NULL) ) == -1 )
    {
	(void) fprintf (stderr, "Error getting flags for descriptor: %d\t%s\n",
			fd, sys_errlist[errno]);
	m_free ( (char *) new_entry );
	return (FALSE);
    }
    fd_flags |= FASYNC;
    if (fcntl (fd, F_SETFL, fd_flags) == -1)
    {
	(void) fprintf (stderr, "Error setting flags for descriptor: %d\t%s\n",
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
	(*last_entry).next = new_entry;
	(*new_entry).prev = last_entry;
    }
    return (TRUE);

#else  /*  COMMUNICATIONS_AVAILABLE  */
    (void) fprintf (stderr, "Communications support not available\n");
    return (FALSE);
#endif  /*  COMMUNICATIONS_AVAILABLE  */
}   /*  End Function chm_manage  */

/*PUBLIC_FUNCTION*/
void chm_unmanage (channel)
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
    static char function_name[] = "chm_unmanage";

    for (entry = managed_channel_list; entry != NULL; entry = (*entry).next)
    {
	if (channel == (*entry).channel)
	{
	    /*  Remove entry  */
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
}   /*  End Function chm_unmanage  */

/*PUBLIC_FUNCTION*/
void chm_poll (timeout_ms)
/*  This routine will poll all managed channels for any activity.
    The time (in milliseconds) to poll must be given by  timeout_ms  .If this
    is less than 0 the routine will poll forever (until some activity occurs
    or a signal is caught).
    The routine returns nothing.
*/
long timeout_ms;
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
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static flag locked = FALSE;
    static char function_name[] = "chm_poll";

    if (locked)
    {
	(void) fprintf (stderr, "Code non-reentrant\n");
	a_prog_bug (function_name);
    }
#  ifdef HAS_SOCKETS
    FD_ZERO (&input_fds);
    FD_ZERO (&output_fds);
    FD_ZERO (&exception_fds);
    for (entry = managed_channel_list; entry != NULL; entry = (*entry).next)
    {
	/*  Add descriptor to list to check  */
	/*  Always monitor input  */
	FD_SET ( (*entry).fd, &input_fds );
	if ( (*entry).output_func != NULL )
	{
	    /*  Monitor output  */
	    FD_SET ( (*entry).fd, &output_fds );
	}
	if ( (*entry).exception_func != NULL )
	{
	    /*  Monitor exceptions  */
	    FD_SET ( (*entry).fd, &exception_fds );
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
	return;
	break;
      case -1:
	if (errno == EINTR)
	{
	    return;
	}
	/*  Failure  */
	(void) fprintf (stderr, "Error calling  select(2)\t%s\n",
			sys_errlist[errno]);
	return;
	break;
      default:
	/*  Success  */
	break;
    }
#  endif  /*  HAS_SOCKETS  */

    /*  Now see which descriptor is active  */
    for (entry = managed_channel_list; entry != NULL; entry = next_entry)
    {
	next_entry = (*entry).next;
#  ifdef HAS_SOCKETS
	if ( (*entry).exception_func != NULL )
	{
	    /*  Monitor exceptions  */
	    if (FD_ISSET ( (*entry).fd, &exception_fds ) != 0)
	    {
		/*  Exception occurred  */
		if ( (* (*entry).exception_func ) ( (*entry).channel,
						   &(*entry).info ) != TRUE )
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
	if (FD_ISSET ( (*entry).fd, &input_fds ) != 0)
#  else  /*  HAS_SOCKETS  */
	if ( r_test_input_event ( (*entry).fd ) )
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
	if ( (*entry).output_func != NULL )
	{
	    /*  Monitor output  */
	    if (FD_ISSET ( (*entry).fd, &output_fds ) != 0)
	    {
		if ( (* (*entry).output_func ) ( (*entry).channel,
						&(*entry).info )
		    != TRUE )
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
#  ifdef ARCH_VXMVX
    if (timeout_ms > 0)
    {
	task_sleep (timeout_ms);
    }
#  endif  /*  ARCH_VXMVX  */

#else  /*  COMMUNICATIONS_AVAILABLE  */
    (void) fprintf (stderr,
		    "Operating system does not support communications\n");
#endif  /*  COMMUNICATIONS_AVAILABLE  */
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
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "read_channel";

    if ( ch_test_for_io ( (*entry).channel ) ||
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
    static char function_name[] = "close_channel";

    if ( (*entry).close_func != NULL )
    {
	(* (*entry).close_func ) ( (*entry).channel,
				  (*entry).info );
    }
    (void) ch_close ( (*entry).channel );
    chm_unmanage ( (*entry).channel );
}   /*  End Function close_channel  */

/*LINTLIBRARY*/
/*  manage.c

    This code provides file descriptor Management routines.

    Copyright (C) 1995-1996  Richard Gooch

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

/*  This file contains all routines needed for the management of file
  descriptors.


    Written by      Richard Gooch   3-OCT-1995

    Updated by      Richard Gooch   7-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   3-JUN-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
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
#include <karma_dm.h>
#include <karma_m.h>
#include <karma_a.h>
#include <karma_r.h>

#if defined(HAS_SOCKETS) /* || defined(HAS_COMMUNICATIONS_EMULATION) */
#  define COMMUNICATIONS_AVAILABLE
#endif

/*  Structure and private data to manage descriptors  */
struct managed_fd_type
{
    int fd;
    void *info;
    flag (*input_func) ();
    void (*close_func) ();
    flag (*output_func) ();
    flag (*exception_func) ();
    struct managed_fd_type *next;
    struct managed_fd_type *prev;
};
static struct managed_fd_type *managed_fd_list = NULL;


/*  Private functions  */
STATIC_FUNCTION (flag read_fd, (struct managed_fd_type *entry) );
STATIC_FUNCTION (void close_fd, (struct managed_fd_type *entry) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag dm_manage ( int fd, void *info, flag (*input_func) (),
		void (*close_func) (), flag (*output_func) (),
		flag (*exception_func) () )
/*  [SUMMARY] Manage a file descriptor for events.
    [PURPOSE] This routine will manage a file descriptor for activity by
    registering callback routines. Only one set of callbacks may be registered
    per file descriptor.
    <fd> The file descriptor to manage.
    <info> The initial value of the arbitrary pointer associated with the
    managed descriptor. This pointer may be modified by the callback routines.
    <input_func> The routine which is called when new input occurrs on the
    descriptor. If this is NULL, no callback routine is installed. The
    prototype function is [<DM_PROTO_input_func>].
    <close_func> The routine which is called when the descriptor closes. If
    this is NULL, no callback routine is installed. The prototype function is
    [<DM_PROTO_close_func>].
    <output_func> The routine which is called when the descriptor becomes ready
    for output. If this is NULL, no callback routine is installed. The
    prototype function is [<DM_PROTO_output_func>].
    <exception_func> The routine which is called when exceptions occurr on the
    descriptor. If this is NULL, no callback routine is installed. The
    prototype function is [<DM_PROTO_exception_func>].
    [RETURNS] TRUE on success, else FALSE.
*/
{
#ifdef COMMUNICATIONS_AVAILABLE
#  ifdef HAS_SOCKETS___dummy
    int fd_flags;
#  endif
    struct managed_fd_type *entry;
    struct managed_fd_type *new_entry;
    struct managed_fd_type *last_entry = NULL; /*  Init. for gcc -Wall  */
    extern struct managed_fd_type *managed_fd_list;
    /*extern char *sys_errlist[];*/
    static char function_name[] = "dm_manage";

    /*  Check if descriptor is a connection  */
/*
    if ( !r_test_for_asynchronous (fd) )
    {
	(void) fprintf (stderr,
			"Cannot manage a descriptor if it is not asynchronous\n");
	a_prog_bug (function_name);
    }
*/
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
    /*  See if descriptor is already managed  */
    for (entry = managed_fd_list; entry != NULL; entry = entry->next)
    {
	if (fd == entry->fd)
	{
	    (void) fprintf (stderr, "Descriptor: %d is already managed\n",
			    fd);
	    a_prog_bug (function_name);
	}
	last_entry = entry;
    }
    /*  Descriptor is not already managed  */
    /*  Allocate new entry  */
    if ( ( new_entry =
	  (struct managed_fd_type *) m_alloc (sizeof *new_entry) )
	== NULL )
    {
	m_error_notify (function_name, "new managed descriptor entry");
	return (FALSE);
    }
    /*  Fill in entry  */
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
    if (managed_fd_list == NULL)
    {
	/*  Create new list  */
	managed_fd_list = new_entry;
    }
    else
    {
	/*  Append to end of list  */
	last_entry->next = new_entry;
	new_entry->prev = last_entry;
    }
    return (TRUE);

#else  /*  COMMUNICATIONS_AVAILABLE  */
    (void) fprintf (stderr, "Communications support not available\n");
    return (FALSE);
#endif  /*  COMMUNICATIONS_AVAILABLE  */
}   /*  End Function dm_manage  */

/*PUBLIC_FUNCTION*/
void dm_unmanage (int fd)
/*  [SUMMARY] Terminate the management of a file descriptor for activity.
    [NOTE] The routine will NOT close the descriptor (nor does it assume the
    descriptor is open).
    <fd> The descriptor to unmanage.
    [RETURNS] Nothing.
*/
{
    struct managed_fd_type *entry;
    extern struct managed_fd_type *managed_fd_list;
    static char function_name[] = "dm_unmanage";

    for (entry = managed_fd_list; entry != NULL; entry = entry->next)
    {
	if (fd == entry->fd)
	{
	    /*  Remove entry  */
	    if (entry->prev == NULL)
	    {
		/*  Entry is first in the list  */
		managed_fd_list = entry->next;
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
    /*  Descriptor not found  */
    (void) fprintf (stderr, "Descriptor: %d not managed\n", fd);
    a_prog_bug (function_name);
}   /*  End Function dm_unmanage  */

/*PUBLIC_FUNCTION*/
void dm_poll (long timeout_ms)
/*  [SUMMARY] Poll all managed descriptors for any activity.
    <timeout_ms> The time (in milliseconds) to poll. If this is less than 0
    the routine will poll forever (until some activity occurs or a signal is
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
    struct managed_fd_type *entry;
    struct managed_fd_type *next_entry;
    extern struct managed_fd_type *managed_fd_list;
    extern char *sys_errlist[];
    static flag locked = FALSE;
    static char function_name[] = "dm_poll";

    if (locked)
    {
	(void) fprintf (stderr, "Code non-reentrant\n");
	a_prog_bug (function_name);
    }
    locked = TRUE;
#  ifdef HAS_SOCKETS
    FD_ZERO (&input_fds);
    FD_ZERO (&output_fds);
    FD_ZERO (&exception_fds);
    for (entry = managed_fd_list; entry != NULL; entry = entry->next)
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
/*
	break;
*/
      case -1:
	if (errno == EINTR)
	{
	    locked = FALSE;
	    return;
	}
	/*  Failure  */
	(void) fprintf (stderr, "Error calling  select(2)\t%s\n",
			sys_errlist[errno]);
	locked = FALSE;
	return;
/*
	break;
*/
      default:
	/*  Success  */
	break;
    }
#  endif  /*  HAS_SOCKETS  */

    /*  Now see which descriptor is active  */
    for (entry = managed_fd_list; entry != NULL; entry = next_entry)
    {
	next_entry = entry->next;
#  ifdef HAS_SOCKETS
	if (entry->exception_func != NULL)
	{
	    /*  Monitor exceptions  */
	    if (FD_ISSET (entry->fd, &exception_fds) != 0)
	    {
		/*  Exception occurred  */
		if ( !(*entry->exception_func) (entry->fd, &entry->info) )
		{
		    /*  Descriptor to be unmanaged and closed  */
		    close_fd (entry);
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
	    if ( !read_fd (entry) )
	    {
		/*  Close and unmanage  */
		close_fd (entry);
		continue;
	    }
	}
#  ifdef HAS_SOCKETS
	if (entry->output_func != NULL)
	{
	    /*  Monitor output  */
	    if (FD_ISSET (entry->fd, &output_fds) != 0)
	    {
		if ( !(*entry->output_func) (entry->fd, &entry->info) )
		{
		    /*  Descriptor to be unmanaged and closed  */
		    close_fd (entry);
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
    (void) fprintf (stderr,
		    "Operating system does not support communications\n");
#endif  /*  COMMUNICATIONS_AVAILABLE  */
    locked = FALSE;
}   /*  End Function dm_poll  */


/*  Private functions follow  */

static flag read_fd (struct managed_fd_type *entry)
/*  [PURPOSE] This routine will read a descriptor (if appropriate) until all
    data is drained from the descriptor.
    <entry> The descriptor entry.
    [RETURNS] TRUE if the descriptor is to remain managed, else FALSE.
*/
{
    int bytes_readable;
    char drain_buffer[1];
    extern char *sys_errlist[];
    static char function_name[] = "read_fd";

    if ( /*ch_test_for_io (entry->fd) ||*/ (entry->close_func != NULL) )
    {
	/*  Can test for input/ closure  */
	/*  Test descriptor, not descriptor, so as to ensure detection
	    of closure  */
	if ( ( bytes_readable = r_get_bytes_readable (entry->fd) ) < 0 )
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
	/*  Descriptor drainable  */
	/*  Drain data (close event)  */
	switch ( r_read (entry->fd, drain_buffer, 1) )
	{
	  case -1:
	    /*  Error  */
	    (void) fprintf (stderr, "Error draining descriptor\t%s\n",
			    sys_errlist[errno]);
	    break;
	  case 0:
	    /*  This is what we expect  */
	    break;
	  case 1:
	    (void) fprintf (stderr, "Data readable on closed descriptor\n");
	    a_prog_bug (function_name);
	    break;
	}
	return (FALSE);
    }
    /*  Input/ closure event occurred  */
#ifdef NOT_IMPLEMENTED
    if ( !ch_test_for_io (entry->fd) )
    {
	/*  Not bufferable  */
	if (entry->input_func == NULL)
	{
	    (void) fprintf (stderr,
			    "Input on asynchronous descriptor but no callback\n");
	    a_prog_bug (function_name);
	}
	return ( (*entry->input_func) (entry->fd, &entry->info) );
    }
#endif
    /*  Buffered connection  */
    if (entry->input_func == NULL)
    {
	(void) fprintf (stderr, "Input on descriptor not being drained\n");
	a_prog_bug (function_name);
    }
    while (bytes_readable > 0)
    {
	/*  More input  */
	if ( !(*entry->input_func) (entry->fd, &entry->info) )
	{
	    /*  Descriptor to be unmanaged and closed  */
	    return (FALSE);
	}
	/*  Test descriptor, not descriptor, to ensure descriptor read buffer as
	    well as descriptor drained  */
	if ( ( bytes_readable = r_get_bytes_readable (entry->fd) ) < 0 )
	{
	    (void) exit (RV_SYS_ERROR);
	}
    }
    return (TRUE);
}   /*  End Function read_fd  */

static void close_fd (struct managed_fd_type *entry)
/*  [PURPOSE] This routine will call the registered <<close_func>> for a
    descriptor, and will then close and unmanage the descriptor.
    <entry> The descriptor entry.
    [RETURNS] Nothing.
*/
{
    /*static char function_name[] = "close_fd";*/

    if (entry->close_func != NULL)
    {
	(*entry->close_func) (entry->fd, entry->info);
    }
    (void) close (entry->fd);
    dm_unmanage (entry->fd);
}   /*  End Function close_fd  */

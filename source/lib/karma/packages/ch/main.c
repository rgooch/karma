/*LINTLIBRARY*/
/*  main.c

    This code provides Channel objects.

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

/*  This file contains all routines needed for the creation, manipulation and
  deletion of channel objects.


    Written by      Richard Gooch   14-AUG-1992

    Updated by      Richard Gooch   13-DEC-1992

    Updated by      Richard Gooch   22-DEC-1992: Fixed bug in  ch_read_disc
  which hangs when reading zero length files.

    Updated by      Richard Gooch   1-JAN-1993: Added memory mapped disc files
  and changed parameters to  ch_gets  .

    Updated by      Richard Gooch   2-JAN-1993: Added  ch_tell  support for
  disc and connection channels.

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   1-MAR-1993: Added support for Ultrix
  mmap(2)

    Updated by      Richard Gooch   22-MAR-1993: Made reference to  conn_
  package in documentation on  ch_open_connection  .

    Updated by      Richard Gooch   28-MAR-1993: Created  ch_map_disc  routine.

    Updated by      Richard Gooch   12-APR-1993: Fixed  ch_read_connection  so
  that closure during read does not abort process.

    Updated by      Richard Gooch   10-MAY-1993: Added patch for SGImips.

    Updated by      Richard Gooch   24-JUL-1993: Added support for opening
  character special devices and FIFOs in  ch_open_file  and created the
  ch_test_for_io  routine.

    Updated by      Richard Gooch   26-JUL-1993: Fixed bug in  ch_tell  .

    Updated by      Richard Gooch   20-AUG-1993: Moved  ch_gets  and  ch_puts
  to  ch_misc.c

    Updated by      Richard Gooch   16-SEP-1993: Fixed memory mapping for
  Convex.

    Updated by      Richard Gooch   3-OCT-1993: Improved diagnostic messages
  when reading from closed asynchronous I/O channel.

    Updated by      Richard Gooch   19-FEB-1994: Made  ch_flush  more robust,
  prevented  ch_close  from flushing for asynchronous descriptor type and fixed
  bug with checking for NFS/local filesystems under Linux.

    Updated by      Richard Gooch   1-APR-1994: Added  ch_register_converters

    Updated by      Richard Gooch   13-APR-1994: Redefined interface for
  ch_register_converters  and changed implementation.

    Updated by      Richard Gooch   16-APR-1994: Disabled converter code for
  the VX/MVX because of some obscure bug (not mine!).

    Updated by      Richard Gooch   11-MAY-1994: Added un-named pipe support by
  creating  ch_create_pipe  .

    Updated by      Richard Gooch   16-MAY-1994: Added data sink support by
  creating  ch_create_sink  .

    Updated by      Richard Gooch   20-MAY-1994: Added  CONST  declaration
  where appropriate.

    Updated by      Richard Gooch   7-JUL-1994: Used  MAP_VARIABLE  in
  ch_map_disc  if available (ie. on Alpha/OSF1).

    Updated by      Richard Gooch   14-AUG-1994: Extended functionality of
  ch_open_file  .

    Updated by      Richard Gooch   24-AUG-1994: Created  ch_tap_events  .

    Updated by      Richard Gooch   12-SEP-1994: Completed implementation of
  ch_tap_events  .

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   14-NOV-1994: Supported disc channels in
  ch_seek  .

    Updated by      Richard Gooch   15-NOV-1994: Added  #include <unistd.h>  .

    Updated by      Richard Gooch   21-NOV-1994: Tidied up  VERIFY_CHANNEL
  macro.

    Updated by      Richard Gooch   25-NOV-1994: Renamed  ch_tap_events  to
  ch_tap_io_events  .

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/ch/main.c

    Updated by      Richard Gooch   27-NOV-1994: Made use of  c_  package.

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>

    Updated by      Richard Gooch   25-DEC-1994: Removed group-write enable
  when writing files.

    Updated by      Richard Gooch   31-JAN-1995: Fixed descriptor leak in
  <ch_map_disc> when channels not mapped.

    Updated by      Richard Gooch   5-MAY-1995: Placate SGI compiler.

    Updated by      Richard Gooch   15-JUN-1995: Made use of IS_ALIGNED macro.

    Updated by      Richard Gooch   31-MAR-1996: Changed documentation style.

    Updated by      Richard Gooch   15-JUN-1996: Inlined memory copy for small
  transfers in <ch_read_disc> and <ch_read_memory>. Created
  <ch_read_and_swap_blocks>.

    Updated by      Richard Gooch   29-JUN-1996: Created
  <ch_swap_and_write_blocks>.

    Last updated by Richard Gooch   10-AUG-1996: Moved
  <ch_read_and_swap_blocks> and <ch_swap_and_write_blocks> routines to misc.c.


*/

#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>
#include <karma.h>
#include <karma_ch.h>
#include <karma_m.h>
#include <karma_r.h>
#include <karma_a.h>
#include <karma_c.h>
#include <os.h>
#ifdef HAS_MMAP
#  include <sys/mman.h>
#  ifdef OS_ULTRIX
caddr_t mmap ();
#  endif
#  ifdef OS_Linux
#    include <sys/vfs.h>
     /*  This is in  linux/nfs_fs.h  but contains declarations for Kernel
	 internals. Hope this will be fixed one day.  */
#    define NFS_SUPER_MAGIC 0x6969
#  endif
#endif

#ifdef OS_VXMVX
/*  Some obscure bug with the VX/MVX software prevents me from enabling this.
    It's not so much that this software is faulty, but that there is some limit
    on programs for the VX/MVX (but not a pure size limit).
    Basically, I don't know what is going on, but cutting code has fixed it
    for now. This is a MAJOR problem!
    In fact, cutting code has *not* fixed it, but I can't be bothered removing
    #ifdef's just now.
*/
#  define DISABLE_CONVERTERS
#endif



#if __STDC__ == 1
#  define CHANNEL_MAGIC_NUMBER 3498653274U
#  define CONVERTER_MAGIC_NUMBER 2495849834U
#else
#  define CHANNEL_MAGIC_NUMBER (unsigned int) 3498653274
#  define CONVERTER_MAGIC_NUMBER (unsigned int) 2495849834
#endif

#define VERIFY_CHANNEL(ch) if (ch == NULL) \
{(void) fprintf (stderr, "NULL channel passed\n"); \
 a_prog_bug (function_name); } \
if (ch->magic_number != CHANNEL_MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid channel object\n"); \
 a_prog_bug (function_name); }

#define CONNECTION_BUF_SIZE (unsigned int) 4096
#define DEFAULT_BLOCK_SIZE (unsigned int) 4096
#define CONV_BUF_SIZE (unsigned int) 4096

#define MMAP_LARGE_SIZE 1048576

#define CHANNEL_TYPE_DISC (unsigned int) 0
#define CHANNEL_TYPE_CONNECTION (unsigned int) 1
#define CHANNEL_TYPE_MEMORY (unsigned int) 2
#define CHANNEL_TYPE_DOCK (unsigned int) 3
#define CHANNEL_TYPE_ASYNCHRONOUS (unsigned int) 4
#define CHANNEL_TYPE_MMAP (unsigned int) 5
#define CHANNEL_TYPE_CHARACTER (unsigned int) 6
#define CHANNEL_TYPE_FIFO (unsigned int) 7
#define CHANNEL_TYPE_SINK (unsigned int) 8
#define CHANNEL_TYPE_UNDEFINED (unsigned int) 9

/*  Internal definition of ChConverter object structure type  */
struct _converter_struct
{
    unsigned int magic_number;
    Channel channel;
    unsigned int (*size_func) ();
    unsigned int (*read_func) ();
    unsigned int (*write_func) ();
    flag (*flush_func) ();
    void (*close_func) ();
    void *info;
    struct _converter_struct *prev;
    struct _converter_struct *next;
};

/*  Internal definition of Channel object structure type  */
struct channel_type
{
    unsigned int magic_number;
    unsigned int type;
    int fd;
    int ch_errno;
    char *read_buffer;
    unsigned int read_buf_len;
    unsigned int read_buf_pos;
    unsigned int bytes_read;
    char *write_buffer;
    unsigned int write_buf_len;
    unsigned int write_buf_pos;
    unsigned int write_start_pos;
    char *memory_buffer;
    unsigned int mem_buf_len;
    unsigned int mem_buf_read_pos;
    unsigned int mem_buf_write_pos;
    flag mem_buf_allocated;
    flag local;
    unsigned int mmap_access_count;
    unsigned int abs_read_pos;
    unsigned int abs_write_pos;
    ChConverter top_converter;
    ChConverter next_converter;
    struct channel_type *prev;
    struct channel_type *next;
};


/*  Private data follows  */
static Channel first_channel = NULL;
static flag registered_exit_func = FALSE;
static KCallbackList tap_list = NULL;


/*  Private functions  */
STATIC_FUNCTION (Channel ch_alloc, () );
STATIC_FUNCTION (unsigned int ch_read_disc,
		 (Channel channel, char *buffer, unsigned int length) );
STATIC_FUNCTION (unsigned int ch_read_connection,
		 (Channel channel, char *buffer, unsigned int length) );
STATIC_FUNCTION (unsigned int ch_read_memory,
		 (Channel channel, char *buffer, unsigned int length) );
#ifdef DISABLED
STATIC_FUNCTION (unsigned int ch_read_memory_and_swap,
		 (Channel channel, char *buffer, unsigned int num_blocks,
		  unsigned int block_size) );
#endif
STATIC_FUNCTION (unsigned int ch_write_descriptor,
		 (Channel channel, CONST char *buffer, unsigned int length) );
STATIC_FUNCTION (int mywrite_raw,
		 (Channel channel, CONST char *buffer, unsigned int length) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
Channel ch_open_file (CONST char *filename, CONST char *type)
/*  [SUMMARY] Open a file.
    [PURPOSE] This routine will open a file channel. The file may be a regular
    disc file, a named FIFO, a character special device, a Unix domain
    connection or a TCP/IP connection. The channel may be later tested
    to determine what the true channel type is by calling routines such as:
    [<ch_test_for_asynchronous>] and [<ch_test_for_io>].
    <filename> The pathname of the file to open. This parameter has the same
    meaning as the first parameter to <<open(2)>>. Filenames of the form
    "//tcpIP/<hostname>:<port>" indicate a connection to a TCP/IP port on host
    <<hostname>> with raw port number <<port>> is requested.
    <type> The mode of the file. See [<CH_FILE_MODES>] for a list of allowable
    modes.
    [NOTE] For character special files and named FIFOs, these modes
    degenerate into read-write, read-only and write-only.
    [RETURNS] A channel object on success, else NULL.
*/
{
    flag read_flag = FALSE;
    flag write_flag = FALSE;
    int flags = 0;  /*  Initialised to keep compiler happy  */
    int mode;
    unsigned int filetype;
    unsigned int blocksize;
    Channel channel;
    static char function_name[] = "ch_open_file";

    if ( (filename == NULL) || (type == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if ( ( channel = ch_alloc () ) == NULL )
    {
	return (NULL);
    }
    /*  Determine access mode  */
    if (strcmp (type, "r") == 0)
    {
	flags = O_RDONLY;
	read_flag = TRUE;
    }
    else if (strcmp (type, "w") == 0)
    {
	flags = O_CREAT | O_TRUNC | O_WRONLY;
	write_flag = TRUE;
    }
    else if (strcmp (type, "a") == 0)
    {
	flags = O_CREAT | O_APPEND | O_WRONLY;
	write_flag = TRUE;
    }
    else if (strcmp (type, "r+") == 0)
    {
	flags = O_RDWR;
	read_flag = TRUE;
	write_flag = TRUE;
    }
    else if (strcmp (type, "w+") == 0)
    {
	flags = O_CREAT | O_TRUNC | O_RDWR;
	read_flag = TRUE;
	write_flag = TRUE;
    }
    else if (strcmp (type, "a+") == 0)
    {
	flags = O_CREAT | O_APPEND | O_RDWR;
	read_flag = TRUE;
	write_flag = TRUE;
    }
    else if (strcmp (type, "W") == 0)
    {
	flags = O_WRONLY;
	write_flag = TRUE;
    }
    else
    {
	(void) fprintf (stderr, "Illegal access mode: \"%s\"\n", type);
	a_prog_bug (function_name);
    }
#ifdef S_IRUSR
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
#else
    mode = 0664;  /*  Octal!  */
#endif
    /*  Set sticky bit if defined  */
#ifdef S_ISVTX
    mode |= S_ISVTX;
#endif
    /*  Open file descriptor  */
    if ( ( channel->fd = r_open_file (filename, flags, mode, &filetype,
				      &blocksize) ) < 0 )
    {
	(void) ch_close (channel);
	return (NULL);
    }
    /*  Set channel type  */
    switch (filetype)
    {
      case KFTYPE_DISC:
	channel->type = CHANNEL_TYPE_DISC;
	break;
      case KFTYPE_CHARACTER:
	channel->type = CHANNEL_TYPE_CHARACTER;
	break;
      case KFTYPE_FIFO:
	channel->type = CHANNEL_TYPE_FIFO;
	break;
      case KFTYPE_UNIX_CONNECTION:
	channel->type = CHANNEL_TYPE_CONNECTION;
	blocksize = CONNECTION_BUF_SIZE;
	break;
      case KFTYPE_LOCAL_tcpIP_CONNECTION:
	channel->type = CHANNEL_TYPE_CONNECTION;
	channel->local = TRUE;
	blocksize = CONNECTION_BUF_SIZE;
	break;
      case KFTYPE_REMOTE_tcpIP_CONNECTION:
	channel->type = CHANNEL_TYPE_CONNECTION;
	channel->local = FALSE;
	blocksize = CONNECTION_BUF_SIZE;
	break;
      default:
	(void) fprintf (stderr, "Illegal filetype: %u\n", filetype);
	a_prog_bug (function_name);
	break;
    }
    if (blocksize == 0) blocksize = DEFAULT_BLOCK_SIZE;
    if (read_flag)
    {
	/*  Allocate read buffer  */
	if ( ( channel->read_buffer = m_alloc (blocksize) ) == NULL )
	{
	    m_error_notify (function_name, "read buffer");
	    (void) ch_close (channel);
	    return (NULL);
	}
	channel->read_buf_len = blocksize;
	channel->read_buf_pos = 0;
	channel->bytes_read = 0;
    }
    if (write_flag)
    {
	/*  Allocate write buffer  */
	if ( ( channel->write_buffer = m_alloc (blocksize) ) == NULL )
	{
	    m_error_notify (function_name, "write buffer");
	    (void) ch_close (channel);
	    return (NULL);
	}
	channel->write_buf_len = blocksize;
	channel->write_buf_pos = 0;
	channel->write_start_pos = 0;
    }
    return (channel);
}   /*  End Function ch_open_file  */

/*PUBLIC_FUNCTION*/
Channel ch_map_disc (CONST char *filename, unsigned int option, flag writeable,
		     flag update_on_write)
/*  [SUMMARY] Map a disc file.
    [PURPOSE] This routine will open a memory channel with the memory pages
    being mapped from a disc file. The disc file must already exist.
    <filename> The pathname of the file to open.
    <option> Control value which determines whether the channel is opened as an
    ordinary disc file or is mapped. See [<CH_MAP_CONTROLS>] for legal values.
    If the file is not mapped then the routine will attempt to open an ordinary
    disc channel. If the file is opened as a disc channel the access mode is:
    "r".
    <writable> If the mapped pages are to be writeable, this must be TRUE. If
    this is FALSE and the memory pages are written to, a segmentation fault
    occurs.
    <update_on_write> If the disc file should be updated when the memory pages
    are written to, this must be TRUE. If this is FALSE, then a write to a
    memory page causes the page to be copied into swap space and the process
    retains a private copy of the page from this point on.
    [NOTE] If <<update_on_write>> is FALSE and <<writeable>> is TRUE, then some
    systems require the allocation of normal virtual memory equal to the size
    of the disc file at the time of mapping, while others will dynamically
    allocate memory from swap space as pages are written into. In the latter
    case, some systems will cause a segmentation fault if swap space is
    exhausted, while other systems wait for space to be freed.
    [NOTE] The channel may be queried to determine if it has been memory mapped
    using the call <<ch_test_for_mmap>>.
    [RETURNS] A channel object on success, else NULL.
*/
{
    int open_flags;
#ifdef HAS_MMAP
    flag memory_map = FALSE;
    int mmap_flags = 0;
    int mmap_prot = PROT_READ;
#  ifdef OS_ConvexOS
    unsigned int map_len;
#  endif
#  ifdef OS_Linux
    struct statfs statfs_buf;
#  endif
#endif
    struct stat statbuf;
    Channel channel;
    extern char *sys_errlist[];
    static char function_name[] = "ch_map_disc";

    if (filename == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
#ifdef HAS_MMAP
    /*  Can memory map  */
    if ( ( channel = ch_alloc () ) == NULL )
    {
	return (NULL);
    }
    /*  Open disc file descriptor  */
    if (writeable && update_on_write)
    {
	open_flags = O_RDWR;
    }
    else
    {
	open_flags = O_RDONLY;
    }
    if ( ( channel->fd = open (filename, open_flags, 0) ) < 0 )
    {
	(void) ch_close (channel);
	return (NULL);
    }
    /*  Set the type to disc so that premature closure closes the descriptor */
    channel->type = CHANNEL_TYPE_DISC;
    /*  Get stats on file  */
    if (fstat (channel->fd, &statbuf) != 0)
    {
	(void) fprintf (stderr, "Error getting file stats\n");
	(void) ch_close (channel);
	return (NULL);
    }
#  ifdef OS_Linux
    if (fstatfs (channel->fd, &statfs_buf) != 0)
    {
	(void) fprintf (stderr, "Error getting filesystem stats\n");
	(void) close (channel->fd);
	(void) ch_close (channel);
	return (NULL);
    }
#  endif
    /*  Determine if file should be memory mapped  */
    switch (option)
    {
      case K_CH_MAP_NEVER:
	break;
      case K_CH_MAP_LARGE_LOCAL:
	/*  Check if large, local file  */
#  ifdef OS_Linux
	if ( (statbuf.st_size >= MMAP_LARGE_SIZE) &&
	    (statfs_buf.f_type != NFS_SUPER_MAGIC) )
#  else
	if ( (statbuf.st_size >= MMAP_LARGE_SIZE) &&
	    (statbuf.st_dev >= 0) )
#  endif
	{
	    memory_map = TRUE;
	}
	break;
      case K_CH_MAP_LOCAL:
        /*  Check if local  */
#  ifdef OS_Linux
        if (statfs_buf.f_type != NFS_SUPER_MAGIC)
#  else
	if (statbuf.st_dev >= 0)
#  endif
	{
	    memory_map = TRUE;
	}
	break;
      case K_CH_MAP_LARGE:
	/*  Check if large  */
	if (statbuf.st_size >= MMAP_LARGE_SIZE)
	{
	    memory_map = TRUE;
	}
	break;
      case K_CH_MAP_IF_AVAILABLE:
      case K_CH_MAP_ALWAYS:
	memory_map = TRUE;
	break;
      default:
	(void) fprintf (stderr, "Illegal value of: option : %u\n", option);
	a_prog_bug (function_name);
	break;
    }
    if (!memory_map)
    {
	/*  Mapping not required  */
	(void) ch_close (channel);
	return ( ch_open_file (filename, "r") );
    }
    /*  Memory mapping desired  */
#  ifdef MAP_VARIABLE
    mmap_flags |= MAP_VARIABLE;
#  endif
    if (writeable)
    {
	/*  Write enable pages  */
	mmap_prot |= PROT_WRITE;
	if (update_on_write)
	{
	    /*  Shared  */
#  ifdef MAP_SHARED
	    mmap_flags |= MAP_SHARED;
#  else
	    if (option != K_CH_MAP_ALWAYS)
	    {
		return ( ch_open_file (filename, "r") );
	    }
	    (void) fprintf (stderr, "Share-on-write mapping not supported\n");
	    return (NULL);
#  endif
	}
	else
	{
	    /*  Copy-on-write mode required  */
#  ifdef MAP_PRIVATE
	    mmap_flags |= MAP_PRIVATE;
#  else
	    if (option != K_CH_MAP_ALWAYS)
	    {
		return ( ch_open_file (filename, "r") );
	    }
	    (void) fprintf (stderr, "Copy-on-write mapping not supported\n");
	    return (NULL);
#  endif
	}
    }
    else
    {
	/*  Read only  */
#  ifdef MAP_SHARED
	mmap_flags |= MAP_SHARED;
#  endif
#  ifdef MAP_FILE
	mmap_flags |= MAP_FILE;
#  endif
    }
#  ifdef OS_ConvexOS
    map_len = statbuf.st_size;
#endif
    if ( ( channel->memory_buffer = mmap ( (caddr_t ) NULL,
#  ifdef OS_ConvexOS  /*  Why the fuck to they have to be different?  */
					  &map_len,
#  else
					  (size_t) statbuf.st_size,
#  endif
					  mmap_prot, mmap_flags,
					  channel->fd, (off_t) 0 ) )
	== (caddr_t) -1 )
    {
	/*  Failed  */
	(void) fprintf (stderr, "Error memory mapping file: \"%s\"\t%s\n",
			filename, sys_errlist[errno]);
	(void) ch_close (channel);
	return (NULL);
    }
    /*  Set channel type  */
    channel->type = CHANNEL_TYPE_MMAP;
    channel->mem_buf_allocated = FALSE;
    channel->mem_buf_len = statbuf.st_size;
    return (channel);
#else  /*  HAS_MMAP  */
    if (option != K_CH_MAP_ALWAYS) return ( ch_open_file (filename, "r") );
    (void) fprintf (stderr, "Memory mapping not supported\n");
    return (NULL);
#endif  /*  HAS_MMAP  */
}   /*  End Function ch_map_disc  */

/*PUBLIC_FUNCTION*/
Channel ch_open_connection (unsigned long host_addr, unsigned int port_number)
/*  [SUMMARY] Open a connection.
    [PURPOSE] This routine will open a full-duplex connection channel to a
    server running on another host machine.
    <host_addr> The Internet address of the host machine. If this is 0 the
    connection is made to a server running on the same machine using the most
    efficient transport available.
    <port_number> The port number to connect to. This should not be confused
    with Internet port numbers.
    [NOTE] The use of this routine is not recommended, see the [<conn>]
    package for more powerful functionality.
    [RETURNS] A channel object on success, else NULL.
*/
{
    Channel channel;
    static char function_name[] = "ch_open_connection";

    if ( ( channel = ch_alloc () ) == NULL )
    {
	return (NULL);
    }
    /*  Set channel type  */
    channel->type = CHANNEL_TYPE_CONNECTION;
    /*  Open connection descriptor  */
    if ( ( channel->fd = r_connect_to_port (host_addr, port_number,
					    &channel->local) ) < 0 )
    {
	(void) ch_close (channel);
	return (NULL);
    }
    /*  Allocate read buffer  */
    if ( ( channel->read_buffer = m_alloc (CONNECTION_BUF_SIZE) ) == NULL )
    {
	m_error_notify (function_name, "read buffer");
	(void) ch_close (channel);
	return (NULL);
    }
    channel->read_buf_len = CONNECTION_BUF_SIZE;
    channel->read_buf_pos = 0;
    channel->bytes_read = 0;
    /*  Allocate write buffer  */
    if ( ( channel->write_buffer = m_alloc (CONNECTION_BUF_SIZE) ) == NULL )
    {
	m_error_notify (function_name, "write buffer");
	(void) ch_close (channel);
	return (NULL);
    }
    channel->write_buf_len = CONNECTION_BUF_SIZE;
    channel->write_buf_pos = 0;
    channel->write_start_pos = 0;
    return (channel);
}   /*  End Function ch_open_connection  */

/*PUBLIC_FUNCTION*/
Channel ch_open_memory (char *buffer, unsigned int size)
/*  [SUMMARY] Open a memory channel.
    [PURPOSE] This routine will open a memory channel. A memory channel behaves
    like a disc channel with a limited (specified) file (device) size. Data is
    undefined when reading before writing has occurred.
    <buffer> The buffer to use. If this is NULL, the routine will allocate a
    buffer of the specified size which is automatically deallocated upon
    closure of the channel.
    <size> The size of the buffer to allocate.
    [RETURNS] A channel object on success, else NULL.
*/
{
    Channel channel;
    static char function_name[] = "ch_open_memory";

    if ( ( channel = ch_alloc () ) == NULL )
    {
	return (NULL);
    }
    /*  Set channel type  */
    channel->type = CHANNEL_TYPE_MEMORY;
    if (buffer == NULL)
    {
	/*  Must allocate a buffer  */
	if ( ( channel->memory_buffer = m_alloc (size) ) == NULL )
	{
	    m_error_notify (function_name, "channel memory buffer");
	    (void) ch_close (channel);
	    return (NULL);
	}
	channel->mem_buf_allocated = TRUE;
    }
    else
    {
	/*  Use specified buffer  */
	channel->memory_buffer = buffer;
	channel->mem_buf_allocated = FALSE;
    }
    channel->mem_buf_len = size;
    return (channel);
}   /*  End Function ch_open_memory  */

/*PUBLIC_FUNCTION*/
Channel ch_accept_on_dock (Channel dock, unsigned long *addr)
/*  [SUMMARY] Accept a connection.
    [PURPOSE] This routine will open a full-duplex connection channel to the
    first connection on a waiting dock.
    <dock> The dock.
    <addr> The address of the host connecting to the dock will be written here.
    [RETURNS] A channel object on success, else NULL.
*/
{
    Channel channel;
    static char function_name[] = "ch_accept_on_dock";

    VERIFY_CHANNEL (dock);
    if (addr == NULL)
    {
	(void) fprintf (stderr, "NULL address pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is a dock  */
    if (dock->type != CHANNEL_TYPE_DOCK)
    {
	(void) fprintf (stderr, "Channel is not a dock: %s\n",
			function_name);
	return (NULL);
    }
    if ( ( channel = ch_alloc () ) == NULL )
    {
	return (NULL);
    }
    /*  Set channel type  */
    channel->type = CHANNEL_TYPE_CONNECTION;
    /*  Accept connection  */
    if ( ( channel->fd = r_accept_connection_on_dock (dock->fd, addr,
						      &channel->local) )
	< 0 )
    {
	(void) fprintf (stderr, "Error accepting connection\n");
	(void) ch_close (channel);
	return (NULL);
    }
    /*  Allocate read buffer  */
    if ( ( channel->read_buffer = m_alloc (CONNECTION_BUF_SIZE) ) == NULL )
    {
	m_error_notify (function_name, "read buffer");
	(void) ch_close (channel);
	return (NULL);
    }
    channel->read_buf_len = CONNECTION_BUF_SIZE;
    channel->read_buf_pos = 0;
    channel->bytes_read = 0;
    /*  Allocate write buffer  */
    if ( ( channel->write_buffer = m_alloc (CONNECTION_BUF_SIZE) ) == NULL )
    {
	m_error_notify (function_name, "write buffer");
	(void) ch_close (channel);
	return (NULL);
    }
    channel->write_buf_len = CONNECTION_BUF_SIZE;
    channel->write_buf_pos = 0;
    channel->write_start_pos = 0;
    return (channel);
}   /*  End Function ch_accept_on_dock  */

/*PUBLIC_FUNCTION*/
Channel *ch_alloc_port (unsigned int *port_number, unsigned int retries,
			unsigned int *num_docks)
/*  [SUMMARY] Allocate a port.
    [PURPOSE] This routine will allocate a Karma port for the module so that it
    can operate as a server (able to receive network connections).
    <port_number> A pointer to the port number to allocate. The routine will
    write the actual port number allocated to this address. This must point to
    an address which lies on an <<int>> boundary.
    <retries> The number of succsessive port numbers to attempt to allocate
    before giving up. If this is 0, then the routine will give up immediately
    if the specified port number is in use.
    <num_docks> The routine will create a number of docks for one port. Each
    dock is an alternative access point for other modules to connect to this
    port. The number of docks allocated will be written here. This must point
    to an address which lies on an <<int>> boundary.
    [NOTE] The close-on-exec flags of the docks are set such that the docks
    will close on a call to execve(2V).
    [NOTE] The docks are placed into blocking mode.
    [RETURNS] A pointer to an array of channel docks on success, else NULL.
*/
{
    unsigned int dock_count;
    int *docks;
    Channel *ch_docks;
    static char function_name[] = "ch_alloc_port";

    if ( (port_number == NULL) || (num_docks == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Check alignment of  port_number  */
    if ( !IS_ALIGNED (port_number, sizeof *port_number) )
    {
	(void) fprintf (stderr,
			"Pointer to port number storage does not lie on an  int  boundary\n");
	a_prog_bug (function_name);
    }
    /*  Check alignment of  num_docks  */
    if ( !IS_ALIGNED (num_docks, sizeof *num_docks) )
    {
	(void) fprintf (stderr,
			"Pointer to number of docks storage does not lie on an  int  boundary\n");
	a_prog_bug (function_name);
    }
    if ( ( docks = r_alloc_port (port_number, retries, num_docks) )
	== NULL )
    {
	return (NULL);
    }
    /*  Allocate array of channel pointers  */
    if ( ( ch_docks = (Channel *) m_alloc (sizeof *ch_docks * *num_docks) )
	== NULL )
    {
	m_error_notify (function_name, "array of channel pointers");
	/*  Close docks  */
	for (dock_count = 0; dock_count < *num_docks; ++dock_count)
	{
	    r_close_dock (docks[dock_count]);
	}
	return (NULL);
    }
    /*  Create a channel object for each dock  */
    for (dock_count = 0; dock_count < *num_docks; ++dock_count)
    {
	if ( ( ch_docks[dock_count] = ch_alloc () ) == NULL )
	{
	    /*  Close docks  */
	    for (; dock_count > 0; --dock_count)
	    {
		(void) ch_close (ch_docks[dock_count - 1]);
	    }
	    m_free ( (char *) ch_docks );
	    return (NULL);
	}
	ch_docks[dock_count]->fd = docks[dock_count];
	ch_docks[dock_count]->type = CHANNEL_TYPE_DOCK;
    }
    return (ch_docks);
}   /*  End Function ch_alloc_port  */

/*PUBLIC_FUNCTION*/
flag ch_close (Channel channel)
/*  [SUMMARY] Close a channel.
    [PURPOSE] This routine will close a channel object. The write buffer will
    be flushed prior to closure.
    <channel> The channel object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    flag return_value = TRUE;
    ChConverter converter, next_converter;
    extern Channel first_channel;
    extern char *sys_errlist[];
    static char function_name[] = "ch_close";

    VERIFY_CHANNEL (channel);
#ifndef DISABLE_CONVERTERS
    for (converter = channel->top_converter; converter != NULL;
	 converter = converter->next)
    {
	/*  Call the flush routine  */
	channel->next_converter = converter->next;
	if ( !(*converter->flush_func) (channel, &converter->info) )
	{
	    channel->next_converter = channel->top_converter;
	    return (FALSE);
	}
    }
    channel->next_converter = channel->top_converter;
#endif
    /*  Close descriptor if open  */
    if (channel->fd > -1)
    {
	/*  Have open file descriptor: flush buffer and close descriptor  */
	switch (channel->type)
	{
	  case CHANNEL_TYPE_MMAP:
#ifdef HAS_MMAP
	    if (munmap (channel->memory_buffer, channel->mem_buf_len)
		!= 0)
	    {
		(void) fprintf (stderr, "Error unmapping\t%s\n",
				sys_errlist[errno]);
		exit (RV_SYS_ERROR);
	    }
	    channel->memory_buffer = NULL;
#else
	    (void) fprintf (stderr,
			    "Channel memory mapped but platform does not support\n");
	    a_prog_bug (function_name);
#endif
	    /*  Fall through for closure of descriptor  */
	  case CHANNEL_TYPE_DISC:
	  case CHANNEL_TYPE_CHARACTER:
	  case CHANNEL_TYPE_FIFO:
	    if (ch_flush (channel) != TRUE)
	    {
		return_value = FALSE;
	    }
	    if (close (channel->fd) != 0)
	    {
		(void) fprintf (stderr, "Error closing descriptor: %d\t%s\n",
				channel->fd, sys_errlist[errno]);
		return_value = FALSE;
	    }
	    break;
	  case CHANNEL_TYPE_CONNECTION:
	    if (ch_flush (channel) != TRUE)
	    {
		return_value = FALSE;
	    }
	    return_value = r_close_connection (channel->fd);
	    break;
	  case CHANNEL_TYPE_DOCK:
	    r_close_dock (channel->fd);
	    return_value = TRUE;
	    break;
	  case CHANNEL_TYPE_ASYNCHRONOUS:
	    return_value = TRUE;
	    break;
	  case CHANNEL_TYPE_UNDEFINED:
	    return_value = TRUE;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal channel type: %u\n",
			    channel->type);
	    a_prog_bug (function_name);
	    break;
	}
    }
#ifndef DISABLE_CONVERTERS
    for (converter = channel->top_converter; converter != NULL;
	 converter = next_converter)
    {
	/*  Call the close routine  */
	channel->next_converter = converter->next;
	if (converter->close_func != NULL)
	{
	    (*converter->close_func) (converter->info);
	}
	next_converter = converter->next;
	converter->magic_number = 0;
	m_free ( (char *) converter );
    }
#endif
    /*  Deallocate buffers  */
    if (channel->read_buffer != NULL)
    {
	m_free (channel->read_buffer);
    }
    if (channel->write_buffer != NULL)
    {
	m_free (channel->write_buffer);
    }
    if ( (channel->memory_buffer != NULL) && channel->mem_buf_allocated )
    {
	m_free (channel->memory_buffer);
    }
    /*  Remove channel object from list  */
    if (channel->next != NULL)
    {
	/*  Another entry further in the list  */
	channel->next->prev = channel->prev;
    }
    if (channel->prev != NULL)
    {
	/*  Another entry previous in the list  */
	channel->prev->next = channel->next;
    }
    if (channel == first_channel)
    {
	/*  Channel is first in list: make next entry the first  */
	first_channel = channel->next;
    }
    /*  Kill magic number entry and everything else for security  */
    m_clear ( (char *) channel, sizeof *channel );
    /*  Deallocate channel object now that there are no more references  */
    m_free ( (char *) channel );
    return (return_value);
}   /*  End Function ch_close  */

/*PUBLIC_FUNCTION*/
flag ch_flush (Channel channel)
/*  [SUMMARY] Flush the write buffer of a channel object.
    <channel> The channel object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int bytes_to_write;
    ChConverter converter;
    static char function_name[] = "ch_flush";

    /*  THIS ROUTINE MUST NOT BE CALLED FROM ANY OF THE  ch_write  SUPPORT
	ROUTINES  */
    VERIFY_CHANNEL (channel);
#ifndef DISABLE_CONVERTERS
    for (converter = channel->top_converter; converter != NULL;
	 converter = converter->next)
    {
	/*  Call the flush routine  */
	channel->next_converter = converter->next;
	if ( !(*converter->flush_func) (channel, &converter->info) )
	{
	    channel->next_converter = channel->top_converter;
	    return (FALSE);
	}
    }
    channel->next_converter = channel->top_converter;
#endif
    if (channel->fd < 0)
    {
	return (TRUE);
    }
    switch (channel->type)
    {
      case CHANNEL_TYPE_DISC:
	break;
      case CHANNEL_TYPE_MEMORY:
	(void) fprintf (stderr,
			"Non-descriptor type should already be discarded\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_MMAP:
	return (TRUE);
/*
	break;
*/
      case CHANNEL_TYPE_CONNECTION:
      case CHANNEL_TYPE_CHARACTER:
      case CHANNEL_TYPE_FIFO:
	break;
      case CHANNEL_TYPE_DOCK:
	(void) fprintf (stderr,
			"Flush operation not permitted on dock channel\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_ASYNCHRONOUS:
	(void) fprintf (stderr,
			"Flush operation not permitted on raw asynchronous\n");
	a_prog_bug (function_name);
	break;
      default:
	(void) fprintf (stderr, "Invalid channel type\n");
	a_prog_bug (function_name);
	break;
    }
    if (channel->write_buf_pos > channel->write_start_pos)
    {
	/*  Flush write buffer  */
	bytes_to_write = channel->write_buf_pos - channel->write_start_pos;
	if (mywrite_raw (channel, channel->write_buffer, bytes_to_write)
	    < bytes_to_write)
	{
	    channel->write_start_pos = 0;
	    return (FALSE);
	}
	channel->write_start_pos = 0;
	channel->write_buf_pos = 0;
    }
    return (TRUE);
}   /*  End Function ch_flush  */

/*PUBLIC_FUNCTION*/
unsigned int ch_read (Channel channel, char *buffer, unsigned int length)
/*  [SUMMARY] Read from a channel.
    [PURPOSE] This routine will read a number of bytes from a channel and
    places them into a buffer.
    <channel> The channel object.
    <buffer> The buffer to write the data into.
    <length> The number of bytes to write into the buffer.
    [NOTE] If the channel is a connection and the number of bytes readable from
    the connection is equal to or more than <<length>> the routine will NOT
    block.
    [RETURNS] The number of bytes read.
*/
{
    unsigned int num_read = 0;  /*  Initialised to keep compiler happy  */
    ChConverter converter, old_converter;
    static char function_name[] = "ch_read";

    VERIFY_CHANNEL (channel);
    if (buffer == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
#ifndef DISABLE_CONVERTERS
    if ( (converter = channel->next_converter) != NULL )
    {
	/*  Pass it on to a read converter  */
	old_converter = channel->next_converter;
	channel->next_converter = converter->next;
	if ( ( num_read = (*converter->read_func) (channel, buffer, length,
						   &converter->info) )
	    < length )
	{
	    channel->next_converter = channel->top_converter;
	    channel->abs_write_pos += num_read;
	    return (num_read);
	}
	/*  Read converter has produced all data  */
	if (converter == channel->top_converter)
	{
	    channel->abs_read_pos += num_read;
	}
	channel->next_converter = old_converter;
	return (num_read);
    }
#endif
    switch (channel->type)
    {
      case CHANNEL_TYPE_DISC:
	num_read = ch_read_disc (channel, buffer, length);
	break;
      case CHANNEL_TYPE_CONNECTION:
      case CHANNEL_TYPE_CHARACTER:
      case CHANNEL_TYPE_FIFO:
	num_read = ch_read_connection (channel, buffer, length);
	break;
      case CHANNEL_TYPE_MEMORY:
      case CHANNEL_TYPE_MMAP:
	num_read = ch_read_memory (channel, buffer, length);
	break;
      case CHANNEL_TYPE_DOCK:
	(void) fprintf (stderr, "Read operation not permitted on dock\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_ASYNCHRONOUS:
	(void) fprintf (stderr,
			"Read operation not permitted on raw asynchronous\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_SINK:
	errno = 0;
	return (0);
/*
	break;
*/
      default:
	(void) fprintf (stderr, "Invalid channel type\n");
	a_prog_bug (function_name);
	break;
    }
    channel->abs_read_pos += num_read;
    return (num_read);
}   /*  End Function ch_read  */

/*PUBLIC_FUNCTION*/
unsigned int ch_write (Channel channel, CONST char *buffer,unsigned int length)
/*  [SUMMARY] Write to a channel.
    [PURPOSE] This routine will write a number of bytes from a buffer to a
    channel.
    <channel> The channel object.
    <buffer> The buffer to read the data from.
    <length> The number of bytes to read from the buffer and write to the
    channel.
    [RETURNS] The number of bytes written.
*/
{
    ChConverter converter, old_converter;
    unsigned int bytes_to_write, bytes_written;
    unsigned int num_written = 0;  /*  Initialised to keep compiler happy  */
    static char function_name[] = "ch_write";

    VERIFY_CHANNEL (channel);
    if (buffer == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
#ifndef DISABLE_CONVERTERS
    if ( (converter = channel->next_converter) != NULL )
    {
	/*  Pass it on to a write converter  */
	char tmp_buf[CONV_BUF_SIZE];

	old_converter = channel->next_converter;
	channel->next_converter = converter->next;
	for (num_written = 0; length > 0;
	     length -= bytes_written, buffer += bytes_written,
	     num_written += bytes_written)
	{
	    bytes_to_write = (length > CONV_BUF_SIZE) ? CONV_BUF_SIZE : length;
	    m_copy (tmp_buf, buffer, bytes_to_write);
	    if ( ( bytes_written =
		  (*converter->write_func) (channel, tmp_buf, bytes_to_write,
					    &converter->info) )
		< bytes_to_write )
	    {
		channel->next_converter = channel->top_converter;
		channel->abs_write_pos += num_written;
		return (num_written);
	    }
	}
	/*  Write converter has consumed all data  */
	if (converter == channel->top_converter)
	{
	    channel->abs_write_pos += num_written;
	}
	channel->next_converter = old_converter;
	return (num_written);
    }
#endif
    switch (channel->type)
    {
      case CHANNEL_TYPE_DISC:
      case CHANNEL_TYPE_CONNECTION:
      case CHANNEL_TYPE_CHARACTER:
      case CHANNEL_TYPE_FIFO:
	num_written = ch_write_descriptor (channel, buffer, length);
	break;
      case CHANNEL_TYPE_MEMORY:
      case CHANNEL_TYPE_MMAP:
	num_written = mywrite_raw (channel, buffer, length);
	break;
      case CHANNEL_TYPE_DOCK:
	(void) fprintf (stderr, "Write operation not permitted on dock\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_ASYNCHRONOUS:
	(void) fprintf (stderr,
			"Write operation not permitted on raw asynchronous\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_SINK:
	num_written = length;
	break;
      default:
	(void) fprintf (stderr, "Invalid channel type\n");
	a_prog_bug (function_name);
	break;
    }
    if (channel->top_converter == NULL)
    {
	channel->abs_write_pos += num_written;
    }
    return (num_written);
}   /*  End Function ch_write  */

/*PUBLIC_FUNCTION*/
void ch_close_all_channels ()
/*  [SUMMARY] Close all open channels.
    [PURPOSE] The routine will close all open channels. The routine is meant to
    be called from the exit(3) function.
    [RETURNS] Nothing.
*/
{
    extern Channel first_channel;

    while (first_channel != NULL)
    {
	(void) ch_close (first_channel);
    }
}   /*  End Function ch_close_all_channels  */

/*PUBLIC_FUNCTION*/
flag ch_seek (Channel channel, unsigned long position)
/*  [SUMMARY] Move read/write pointer.
    [PURPOSE] This routine will position the read and write pointers for a
    channel.
    <channel> The channel object.
    <position> The position (relative to the start of the channel data).
    [NOTE] This routine cannot be used for connection channels.
    [RETURNS] TRUE on success, else FALSE (indicating a seek request beyond the
    channel limits)
*/
{
    char dummy;
    unsigned long newpos, block_pos, block_len, count;
    static char function_name[] = "ch_seek";

    VERIFY_CHANNEL (channel);
#ifndef DISABLE_CONVERTERS
    if (channel->top_converter != NULL)
    {
	(void) fprintf (stderr,
			"Cannot seek channels with converter functions\n");
	a_prog_bug (function_name);
    }
#endif
    switch (channel->type)
    {
      case CHANNEL_TYPE_DISC:
	if (channel->ch_errno > 0) return (FALSE);
	if ( !ch_flush (channel) ) return (FALSE);
	if ( (block_len = channel->read_buf_len) > 0 )
	{
	    if ( (channel->write_buffer != NULL) &&
		(block_len != channel->write_buf_len) )
	    {
		(void) fprintf (stderr,
				"Read buffer length: %u not equal to write ",
				channel->read_buf_len);
		(void) fprintf (stderr, "buffer length: %u\n",
				channel->write_buf_len);
		a_prog_bug (function_name);
	    }
	}
	else block_len = channel->write_buf_len;
	block_pos = position % block_len;
	newpos = position - block_pos;
	if (lseek (channel->fd, newpos, SEEK_SET) == -1) return (FALSE);
	if (channel->read_buffer != NULL)
	{
	    channel->read_buf_pos = 0;
	    channel->bytes_read = 0;
	    channel->abs_read_pos = newpos;
	    for (count = 0; count < block_pos; ++count)
	    {
		if ( !ch_read (channel, &dummy, 1) ) return (FALSE);
	    }
	    if (position != channel->abs_read_pos)
	    {
		(void) fprintf (stderr, "Position missmatch: %lu  and  %u\n",
				position, channel->abs_read_pos);
		a_prog_bug (function_name);
	    }
	}
	if (channel->write_buffer != NULL)
	{
	    channel->write_buf_pos = block_pos;
	    channel->write_start_pos = block_pos;
	    channel->abs_write_pos = position;
	}
	channel->ch_errno = 0;
	break;
      case CHANNEL_TYPE_CONNECTION:
	(void) fprintf (stderr,
			"Seek operation not permitted on connection channel\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_CHARACTER:
	(void) fprintf (stderr,
			"Seek operation not permitted on character special channel\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_FIFO:
	(void) fprintf (stderr,
			"Seek operation not permitted on FIFO channel\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_MEMORY:
      case CHANNEL_TYPE_MMAP:
	if (position > channel->mem_buf_len)
	{
	    /*  Illegal seek position  */
	    return (FALSE);
	}
	channel->mem_buf_read_pos = position;
	channel->mem_buf_write_pos = position;
	channel->abs_read_pos = position;
	channel->abs_write_pos = position;
	break;
      case CHANNEL_TYPE_DOCK:
	(void) fprintf (stderr,
			"Seek operation not permitted on dock channel\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_ASYNCHRONOUS:
	(void) fprintf (stderr,
			"Seek operation not permitted on raw asynchronous channel\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_SINK:
	(void) fprintf (stderr,
			"Seek operation not permitted on data sink channel\n");
	a_prog_bug (function_name);
	break;
      default:
	(void) fprintf (stderr, "Invalid channel type\n");
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function ch_seek  */

/*PUBLIC_FUNCTION*/
int ch_get_bytes_readable (Channel channel)
/*  [SUMMARY] Count unread bytes.
    [PURPOSE] This routine will determine the number of bytes currently
    readable on a connection channel. This is equal to the maximum number of
    bytes that could be read from the channel at this time without blocking
    waiting for more input to be transmitted from the other end of the
    connection.
    <channel> The channel object.
    [RETURNS] The number of bytes readable on success, else -1 indicating error
*/
{
    ChConverter converter;
    int bytes_available;
    int conv_bytes = 0;
    static char function_name[] = "ch_get_bytes_readable";

    VERIFY_CHANNEL (channel);
    switch (channel->type)
    {
      case CHANNEL_TYPE_DISC:
	(void) fprintf (stderr,
			"Function: %s not appropriate for disc channel\n",
			function_name);
	return (-1);
/*
	break;
*/
      case CHANNEL_TYPE_CONNECTION:
      case CHANNEL_TYPE_CHARACTER:
      case CHANNEL_TYPE_FIFO:
	break;
      case CHANNEL_TYPE_ASYNCHRONOUS:
	(void) fprintf (stderr,
			"Function: %s not appropriate for an asynchronous channel\n",
			function_name);
	return (-1);
/*
	break;
*/
      case CHANNEL_TYPE_MEMORY:
	(void) fprintf (stderr,
			"Function: %s not appropriate for memory channel\n",
			function_name);
	return (-1);
/*
	break;
*/
      case CHANNEL_TYPE_DOCK:
	(void) fprintf (stderr,
			"Function: %s not appropriate for dock channel\n",
			function_name);
	return (-1);
/*
	break;
*/
      case CHANNEL_TYPE_SINK:
	(void) fprintf (stderr,
			"Function: %s not appropriate for data sink channel\n",
			function_name);
	return (-1);
/*
	break;
*/
      default:
	(void) fprintf (stderr, "Invalid channel type: %u\n", channel->type);
	return (-1);
/*
	break;
*/
    }
    if (channel->ch_errno != 0)
    {
	errno = channel->ch_errno;
	return (-1);
    }
#ifndef DISABLE_CONVERTERS
    for (converter = channel->top_converter, conv_bytes = 0;
	 converter != NULL;
	 converter = converter->next)
    {
	/*  Call the close routine  */
	channel->next_converter = converter->next;
	conv_bytes += (*converter->size_func) (channel, &converter->info);
    }
    channel->next_converter = channel->top_converter;
#endif
    if ( ( bytes_available = r_get_bytes_readable (channel->fd) ) < 0 )
    {
	channel->ch_errno = errno;
	return (-1);
    }
    return (conv_bytes + (int) channel->bytes_read -
	    (int) channel->read_buf_pos + bytes_available);
}   /*  End Function ch_get_bytes_readable  */

/*PUBLIC_FUNCTION*/
int ch_get_descriptor (Channel channel)
/*  [SUMMARY] Get the file descriptor associated with a channel.
    <channel> The channel object.
    [RETURNS] The file descriptor on success, else -1 indicating error.
*/
{
    static char function_name[] = "ch_get_descriptor";

    VERIFY_CHANNEL (channel);
    return (channel->fd);
}   /*  End Function ch_get_descriptor  */

/*PUBLIC_FUNCTION*/
void ch_open_stdin ()
/*  [SUMMARY] Create starndard input channel.
    [PURPOSE] This routine will create a channel object for the standard input
    descriptor (typically 0 on Unix systems).
    [NOTE] The standard input channel will be written to the external variable
    <<ch_stdin>>.
    [RETURNS] Nothing.
*/
{
    flag disc;
    static char function_name[] = "ch_open_stdin";

    if (ch_stdin != NULL)
    {
	(void) fprintf (stderr, "ch_stdin already open\n");
	a_prog_bug (function_name);
    }
    if ( ( ch_stdin = ch_alloc () ) == NULL )
    {
	m_abort (function_name, "ch_stdin");
    }
    if ( ( ch_stdin->fd = r_open_stdin (&disc) ) < 0 )
    {
	(void) fprintf (stderr, "Error getting input descriptor\n");
	exit (RV_UNDEF_ERROR);
    }
    /*  Allocate read buffer  */
    if ( ( ch_stdin->read_buffer = m_alloc (CONNECTION_BUF_SIZE) )
	== NULL )
    {
	m_abort (function_name, "read buffer");
    }
    ch_stdin->read_buf_len = CONNECTION_BUF_SIZE;
    /*  Tag type  */
    ch_stdin->type = (disc) ? CHANNEL_TYPE_DISC : CHANNEL_TYPE_CHARACTER;
}   /*  End Function ch_open_stdin  */

/*PUBLIC_FUNCTION*/
flag ch_test_for_io (Channel channel)
/*  [SUMMARY] Test if I/O possible on channel.
    [PURPOSE] This routine will test if a channel object is capable of
    supporting reading and writing operations. Most channels fall under this
    category. The notable exceptions are the dock channel and channels created
    by a call to <<ch_attach_to_asynchronous_descriptor>>.
    <channel> The channel object.
    [RETURNS] TRUE if the channel is capable of reading and writing, else FALSE
*/
{
    flag return_value;
    static char function_name[] = "ch_test_for_io";

    VERIFY_CHANNEL (channel);
    switch (channel->type)
    {
      case CHANNEL_TYPE_DOCK:
      case CHANNEL_TYPE_ASYNCHRONOUS:
	return_value = FALSE;
	break;
      default:
	return_value = TRUE;
	break;
    }
    return (return_value);
}   /*  End Function ch_test_for_io  */

/*PUBLIC_FUNCTION*/
flag ch_test_for_asynchronous (Channel channel)
/*  [SUMMARY] Test if a channel object is an asynchronous channel.
    [PURPOSE] This routine will test if a channel object is an asynchronous
    channel, i.e. a character special file, named FIFO, connection, a dock
    channel or one created by a call to
    <<ch_attach_to_asynchronous_descriptor>> or <<ch_create_pipe>>.
    <channel> The channel object.
    [RETURNS] TRUE if the channel is asynchronous, else FALSE.
*/
{
    flag return_value;
    static char function_name[] = "ch_test_for_asynchronous";

    VERIFY_CHANNEL (channel);
    switch (channel->type)
    {
      case CHANNEL_TYPE_CONNECTION:
      case CHANNEL_TYPE_DOCK:
      case CHANNEL_TYPE_ASYNCHRONOUS:
      case CHANNEL_TYPE_CHARACTER:
      case CHANNEL_TYPE_FIFO:
	return_value = TRUE;
	break;
      default:
	return_value = FALSE;
	break;
    }
    return (return_value);
}   /*  End Function ch_test_for_asynchronous  */

/*PUBLIC_FUNCTION*/
flag ch_test_for_connection (Channel channel)
/*  [SUMMARY] Test if a channel object is a connection channel.
    <channel> The channel object.
    [RETURNS] TRUE if the channel object is a connection, else FALSE.
*/
{
    static char function_name[] = "ch_test_for_connection";

    VERIFY_CHANNEL (channel);
    if (channel->type == CHANNEL_TYPE_CONNECTION)
    {
	return (TRUE);
    }
    else
    {
	return (FALSE);
    }
}   /*  End Function ch_test_for_connection  */

/*PUBLIC_FUNCTION*/
flag ch_test_for_local_connection (Channel channel)
/*  [SUMMARY] Test if a connection channel object is a local connection.
    <channel> The channel object.
    [RETURNS] TRUE if the channel object is a local connection, else FALSE.
*/
{
    static char function_name[] = "ch_test_for_local_connection";

    VERIFY_CHANNEL (channel);
    if (channel->type != CHANNEL_TYPE_CONNECTION)
    {
	return (FALSE);
    }
    return (channel->local);
}   /*  End Function ch_test_for_local_connection  */

/*PUBLIC_FUNCTION*/
Channel ch_attach_to_asynchronous_descriptor (int fd)
/*  [SUMMARY] Create a channel object from an asynchronous descriptor.
    <fd> The descriptor.
    [RETURNS] A channel object on success, else NULL.
*/
{
    Channel channel;
/*
    static char function_name[] = "ch_attach_to_asynchronous_descriptor";
*/

    if ( ( channel = ch_alloc () ) == NULL )
    {
	return (NULL);
    }
    /*  Set channel type  */
    channel->type = CHANNEL_TYPE_ASYNCHRONOUS;
    channel->fd = fd;
    return (channel);
}   /*  End Function ch_attach_to_asynchronous_descriptor  */

/*PUBLIC_FUNCTION*/
flag ch_test_for_mmap (Channel channel)
/*  [SUMMARY] Test if a channel object is a memory mapped disc channel.
    <channel> The channel object.
    [RETURNS] TRUE if the channel object is memory mapped, else FALSE.
*/
{
    static char function_name[] = "ch_test_for_mmap";

    VERIFY_CHANNEL (channel);
    if (channel->type == CHANNEL_TYPE_MMAP)
    {
	return (TRUE);
    }
    else
    {
	return (FALSE);
    }
}   /*  End Function ch_test_for_mmap  */

/*PUBLIC_FUNCTION*/
flag ch_tell (Channel channel, unsigned long *read_pos,
	      unsigned long *write_pos)
/*  [SUMMARY] Get the read and write pointers for a channel.
    <channel> The channel object.
    <read_pos> The read position (relative to the start of the channel data)
    will be written here.
    <write_pos> The write position (relative to the start of the channel data)
    will be written here.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    static char function_name[] = "ch_tell";

    VERIFY_CHANNEL (channel);
    switch (channel->type)
    {
      case CHANNEL_TYPE_DISC:
      case CHANNEL_TYPE_MEMORY:
      case CHANNEL_TYPE_MMAP:
      case CHANNEL_TYPE_CONNECTION:
      case CHANNEL_TYPE_CHARACTER:
      case CHANNEL_TYPE_FIFO:
      case CHANNEL_TYPE_SINK:
	*read_pos = channel->abs_read_pos;
	*write_pos = channel->abs_write_pos;
	break;
      case CHANNEL_TYPE_DOCK:
	(void) fprintf (stderr,
			"Tell operation not permitted on dock channel\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_ASYNCHRONOUS:
	(void) fprintf (stderr,
			"Tell operation not permitted on raw asynchronous channel\n");
	a_prog_bug (function_name);
	break;
      default:
	(void) fprintf (stderr, "Invalid channel type\n");
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function ch_tell  */

/*PUBLIC_FUNCTION*/
char *ch_get_mmap_addr (Channel channel)
/*  [SUMMARY] Get memory mapped address.
    [PURPOSE] This routine will get the starting address of the data for a
    memory mapped disc channel. The channel MUST be a memory mapped disc
    channel.
    <channel> The channel object.
    [RETURNS] The address of the memory mapped data.
    [NOTE] If the memory mapped address space is read-only, any attempt to
    write to this address space will cause a segmentation fault.
*/
{
    static char function_name[] = "ch_get_mmap_addr";

    VERIFY_CHANNEL (channel);
    if (channel->type != CHANNEL_TYPE_MMAP)
    {
	(void) fprintf (stderr, "Channel is not a memory mapped disc file\n");
	a_prog_bug (function_name);
    }
    ++channel->mmap_access_count;
    return (channel->memory_buffer);
}   /*  End Function ch_get_mmap_addr  */

/*PUBLIC_FUNCTION*/
unsigned int ch_get_mmap_access_count (Channel channel)
/*  [SUMMARY] Get memory mapped access count.
    [PURPOSE] This routine will get the number of times a memory mapped disc
    channel has been queried for the mapping address using <<ch_get_mmap_addr>>
    <channel> The channel object.
    [NOTE] The channel MUST be a memory mapped disc channel.
    [RETURNS] The number of address queries.
*/
{
    static char function_name[] = "ch_get_mmap_access_count";

    VERIFY_CHANNEL (channel);
    if (channel->type != CHANNEL_TYPE_MMAP)
    {
	(void) fprintf (stderr, "Channel is not a memory mapped disc file\n");
	a_prog_bug (function_name);
    }
    return (channel->mmap_access_count);
}   /*  End Function ch_get_mmap_access_count  */

/*PUBLIC_FUNCTION*/
ChConverter ch_register_converter (Channel channel,
				   unsigned int (*size_func) (),
				   unsigned int (*read_func) (),
				   unsigned int (*write_func) (),
				   flag (*flush_func) (),
				   void (*close_func) (),
				   void *info)
/*  [SUMMARY] Register channel converter function.
    [PURPOSE] This routine will register a set of converter functions which
    will be called when channel data is read or written. The operation of these
    routines is transparent. Converter functions are useful for automatic
    compression and encryption of data streams.
    It is permissable to register multiple converter functions with a channel.
    Converter functions are pushed down from the top (application) level. In
    other words, the last converter functions registered are called first.
    [NOTE] Converters may only be registered for disc, connection, character
    special and FIFO channels (ie. those opened with <<ch_open_file>>), and
    impose restrictions on channel operations (ie. <<ch_seek>> cannot be
    called).
    [NOTE] Converter functions are expected to provide their own buffering as
    needed.
    <channel> The channel object.
    <size_func> The function which will determine (approximately) how many
    bytes would be returned by the <<read_func>>. This routine is called when
    [<ch_get_bytes_readable>] is called for the channel. The prototype function
    is [<CH_PROTO_size_func>].
    <read_func> The function which will convert data when reading.
    The prototype function is [<CH_PROTO_read_func>].
    <write_func> The function which will convert data when writing. If this is
    NULL, no write conversion is performed. The prototype function is
    [<CH_PROTO_write_func>].
    <flush_func> The function which is called when the channel is to be flushed
    The prototype function is [<CH_PROTO_flush_func>].
    <close_func> The function which is called when the channel is closed. If
    this is NULL, no special action is taken upon channel closure.
    The prototype function is [<CH_PROTO_close_func>].
    [NOTE] The <<flush_func>> will be called prior to the <<close_func>> upon
    channel closure.
    [NOTE]
    The sequence of events when the application level calls [<ch_write>] is:
      The last registered write converter is popped from the stack and called.
      This write converter may buffer some or all of the data. It may call
        [<ch_write>] with some converted data.
      When [<ch_write>] is called from a write converter, the next last
        registered write converter is popped from the stack and called.
      This sequence is continued until data is actually transferred into the
      channel write buffer.
    A similar sequence of events occurs when  ch_read  is called.
    The sequence of events when the application level calls  ch_flush  is:
      The last registered flush converter is popped from the stack and called.
      This flush converter MUST write all data in it's buffer using  ch_write
      When  ch_write  is called from a flush converter, the next last
        registered write converter is popped from the stack and called.
      When the last registered flush converter returns, the sequence is
      repeated with the next last flush converter, and so on, until all data
      in all write buffers is flushed, including the channel write buffer.
    [RETURNS] A ChConverter object on success (which may be used in a call to
    [<ch_unregister_converter>]), else NULL.
*/
{
    ChConverter new;
    static char function_name[] = "ch_register_converter";

    VERIFY_CHANNEL (channel);
#ifdef DISABLE_CONVERTERS
    (void) fprintf (stderr,
		    "Converters cannot be registered on this platform\n");
    a_prog_bug (function_name);
    return (NULL);
#else
    switch (channel->type)
    {
      case CHANNEL_TYPE_DISC:
      case CHANNEL_TYPE_CONNECTION:
      case CHANNEL_TYPE_CHARACTER:
      case CHANNEL_TYPE_FIFO:
	break;
      case CHANNEL_TYPE_MEMORY:
	(void) fprintf (stderr,
			"Conversion not permitted for memory channels\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_MMAP:
	(void) fprintf (stderr,
			"Conversion not permitted for mapped channels\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_DOCK:
	(void) fprintf (stderr,
			"Conversion not permitted for dock channels\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_ASYNCHRONOUS:
	(void) fprintf (stderr,
			"Conversion not permitted for raw asynchronous channels\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_SINK:
	(void) fprintf (stderr,
			"Conversion not permitted for data sink channels\n");
	a_prog_bug (function_name);
	break;
      default:
	(void) fprintf (stderr, "Invalid channel type\n");
	a_prog_bug (function_name);
	break;
    }
    if ( (size_func == NULL) || (read_func == NULL) || (write_func == NULL) ||
	(flush_func == NULL) )
    {
	(void) fprintf (stderr, "NULL functions pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if ( ( new = (ChConverter) m_alloc (sizeof *new) ) == NULL )
    {
	m_error_notify (function_name, "converter structure");
	return (NULL);
    }
    new->magic_number = CONVERTER_MAGIC_NUMBER;
    new->channel = channel;
    new->size_func = size_func;
    new->read_func = read_func;
    new->write_func = write_func;
    new->flush_func = flush_func;
    new->close_func = close_func;
    new->info = info;
    new->prev = NULL;
    new->next = channel->top_converter;
    if (channel->top_converter != NULL)
    {
	channel->top_converter->prev = new;
    }
    channel->top_converter = new;
    channel->next_converter = channel->top_converter;
    return (new);
#endif  /*  DISABLE_CONVERTERS  */
}   /*  End Function ch_register_converter  */

/*PUBLIC_FUNCTION*/
void ch_unregister_converter (ChConverter converter)
/*  [SUMMARY] Unregister converter.
    [PURPOSE] This routine will unregister a set of converter functions
    previously registered with [<ch_register_converter>]. This will cause the
    registered flush and close functions to be called.
    <converter> The ChConverter object.
    [RETURNS] Nothing.
*/
{
    Channel channel;
    ChConverter old_converter;
    static char function_name[] = "ch_unregister_converter";

#ifdef DISABLE_CONVERTERS
    (void) fprintf (stderr,
		    "Converters cannot be registered on this platform\n");
/*    a_prog_bug (function_name);*/
#else
    if (converter == NULL)
    {
	(void) fprintf (stderr, "NULL converter passed\n");
	a_prog_bug (function_name);
    }
    if (converter->magic_number != CONVERTER_MAGIC_NUMBER)
    {
	(void) fprintf (stderr, "Invalid converter object\n");
	a_prog_bug (function_name);
    }
    channel = converter->channel;
    /*  Preserve the stack  */
    old_converter = channel->next_converter;
    /*  Call flush and close functions  */
    channel->next_converter = converter->next;
    (void) (*converter->flush_func) (channel, &converter->info);
    if (converter->close_func != NULL)
    {
	(*converter->close_func) (converter->info);
    }
    /*  Restore the stack  */
    channel->next_converter = old_converter;
    /*  Unlink from list  */
    if (converter->prev != NULL)
    {
	converter->prev->next = converter->next;
    }
    if (converter->next != NULL)
    {
	converter->next->prev = converter->prev;
    }
    if (converter == channel->next_converter)
    {
	channel->next_converter = converter->next;
    }
    if (converter == channel->top_converter)
    {
	channel->top_converter = converter->next;
    }
    converter->magic_number = 0;
    m_free ( (char *) converter );
#endif  /*  DISABLE_CONVERTERS  */
}   /*  End Function ch_unregister_converter  */

/*PUBLIC_FUNCTION*/
flag ch_create_pipe (Channel *read_ch, Channel *write_ch)
/*  [SUMMARY] Create a pipe.
    [PURPOSE] This routine will create an un-named pipe (see <<pipe(2)>> for
    details on un-named pipes).
    <read_ch> The channel corresponding to the read end of the pipe will be
    written here.
    <write_ch> The channel corresponding to the write end of the pipe will be
    written here.
    [RETURNS] TRUE on success, else FALSE and sets <<errno>> with the error
    code.
*/
{
    Channel rch, wch;
    int read_fd;
    int write_fd;
    static char function_name[] = "ch_create_pipe";

    if (r_create_pipe (&read_fd, &write_fd) < 0) return (FALSE);
    errno = 0;
    if ( ( rch = ch_alloc () ) == NULL )
    {
	(void) close (read_fd);
	(void) close (write_fd);
	return (FALSE);
    }
    rch->type = CHANNEL_TYPE_FIFO;
    rch->fd = read_fd;
    if ( ( wch = ch_alloc () ) == NULL )
    {
	(void) ch_close (rch);
	(void) close (write_fd);
	return (FALSE);
    }
    wch->type = CHANNEL_TYPE_FIFO;
    wch->fd = write_fd;
    /*  Read channel  */
    /*  Allocate read buffer  */
    if ( ( rch->read_buffer = m_alloc (CONNECTION_BUF_SIZE) ) == NULL )
    {
	m_error_notify (function_name, "read buffer");
	(void) ch_close (rch);
	(void) ch_close (wch);
	return (FALSE);
    }
    rch->read_buf_len = CONNECTION_BUF_SIZE;
    rch->read_buf_pos = 0;
    rch->bytes_read = 0;
    rch->write_buffer = NULL;
    rch->write_buf_len = 0;
    rch->write_buf_pos = 0;
    rch->write_start_pos = 0;
    /*  Write channel  */
    wch->read_buffer = NULL;
    wch->read_buf_len = 0;
    wch->read_buf_pos = 0;
    wch->bytes_read = 0;
    /*  Allocate write buffer  */
    if ( ( wch->write_buffer = m_alloc (CONNECTION_BUF_SIZE) ) == NULL )
    {
	m_error_notify (function_name, "write buffer");
	(void) ch_close (rch);
	(void) ch_close (wch);
	return (FALSE);
    }
    wch->write_buf_len = CONNECTION_BUF_SIZE;
    wch->write_buf_pos = 0;
    *read_ch = rch;
    *write_ch = wch;
    return (TRUE);
}   /*  End Function ch_create_pipe  */

/*PUBLIC_FUNCTION*/
Channel ch_create_sink ()
/*  [SUMMARY] Create data sink.
    [PURPOSE] This routine will create a data sink channel. All writes to the
    channel are discarded (and reported as having succeeded) and all reads
    return an End-Of-File condition. Read and write operations modify the
    absolute read and write pointers (obtainable with [<ch_tell>]) as expected.
    [RETURNS] The channel object on succes, else NULL.
*/
{
    Channel channel;
/*
    static char function_name[] = "ch_create_sink";
*/

    if ( ( channel = ch_alloc () ) == NULL )
    {
	return (NULL);
    }
    channel->type = CHANNEL_TYPE_SINK;
    return (channel);
}   /*  End Function ch_create_sink  */

/*PUBLIC_FUNCTION*/
KCallbackFunc ch_tap_io_events (void (*tap_func) (), void *info)
/*  [SUMMARY] Register I/O tap function.
    [PURPOSE] This routine will tap I/O events by calling a registered function
    whenever data is transferred to/from a disc, connection or FIFO channel.
    Reading and writing memory mapped or memory channels does *not* constitute
    an I/O event.
    Multiple tap functions may be registered, with the first one registered
    being the first one called upon a channel I/O event.
    <tap_func> The function which is called when I/O occurs. The prototype
    function is [<CH_PROTO_tap_func>].
    <info> The arbitrary information passed to the tap function. This may be
    NULL.
    [RETURNS] A KCallbackFunc. On failure, the process aborts.
*/
{
    extern KCallbackList tap_list;

    return ( c_register_callback (&tap_list, ( flag (*) () ) tap_func,
				  info, NULL, FALSE, NULL, FALSE, FALSE) );
}   /*  End Function ch_tap_io_events  */


/*  Private functions follow  */

static Channel ch_alloc ()
/*  This routine will allocate and initialise a channel data structure.
    The routine returns a channel object structure on success, 
    else it returns NULL.
*/
{
    Channel channel;
    extern flag registered_exit_func;
    extern Channel first_channel;
    static char function_name[] = "ch_alloc";

    if (registered_exit_func != TRUE)
    {
	/*  Register function to be called upon processing of  exit(3)  */
#ifdef HAS_ATEXIT
	atexit (ch_close_all_channels);
#endif
#ifdef HAS_ON_EXIT
	on_exit (ch_close_all_channels, (caddr_t) NULL);
#endif
	registered_exit_func = TRUE;
    }

    /*  Allocate channel structure  */
    if ( ( channel = (Channel) m_alloc ( (unsigned int) sizeof *channel ) )
	== NULL )
    {
	m_error_notify (function_name, "channel object");
	return (NULL);
    }
    channel->magic_number = CHANNEL_MAGIC_NUMBER;
    channel->type = CHANNEL_TYPE_UNDEFINED;
    channel->fd = -1;
    channel->ch_errno = 0;
    channel->read_buffer = NULL;
    channel->read_buf_len = 0;
    channel->read_buf_pos = 0;
    channel->bytes_read = 0;
    channel->write_buffer = NULL;
    channel->write_buf_len = 0;
    channel->write_buf_pos = 0;
    channel->write_start_pos = 0;
    channel->memory_buffer = NULL;
    channel->mem_buf_len = 0;
    channel->mem_buf_read_pos = 0;
    channel->mem_buf_write_pos = 0;
    channel->mmap_access_count = 0;
    channel->abs_read_pos = 0;
    channel->abs_write_pos = 0;
    channel->top_converter = NULL;
    channel->next_converter = NULL;
    /*  Place channel object into list  */
    channel->prev = NULL;
    channel->next = first_channel;
    if (first_channel != NULL)
    {
	first_channel->prev = channel;
    }
    first_channel = channel;
    return (channel);
}   /*  End Function ch_alloc  */

static unsigned int ch_read_disc (Channel channel, char *buffer,
				  unsigned int length)
/*  This routine will read a number of bytes from a disc channel and places
    them into a buffer.
    The channel object must be given by  channel  .
    The buffer to write the data into must be pointed to by  buffer  .
    The number of bytes to read into the buffer must be pointed to by  length
    The routine returns the number of bytes read.
*/
{
    unsigned int read_pos = 0;
    unsigned int count;
    int bytes_to_read;
    int bytes_read;
    CONST char *source;
    extern KCallbackList tap_list;
    /*static char function_name[] = "ch_read_disc";*/

    if ( (channel->bytes_read - channel->read_buf_pos) >= length )
    {
	/*  Read buffer has enough data in it  */
	source = channel->read_buffer + channel->read_buf_pos;
	if (length < 16)
	{
	    /*  Avoid function call overhead  */
	    for (count = 0; count < length; ++count) *buffer++ = *source++;
	}
	else m_copy (buffer, source, length);
	channel->read_buf_pos += length;
	return (length);
    }
    /*  Need to read from disc  */
    /*  Copy what is left in the read buffer  */
    read_pos = channel->bytes_read - channel->read_buf_pos;
    m_copy (buffer, channel->read_buffer + channel->read_buf_pos,
	    read_pos);
    /*  Test if an error occurred  */
    if (channel->ch_errno != 0)
    {
	errno = (channel->ch_errno > 0) ? channel->ch_errno : 0;
	channel->read_buf_pos = 0;
	channel->bytes_read = 0;
	return (read_pos);
    }
    /*  Test if that's all there is in the file  */
    if ( (channel->bytes_read > 0) &&
	(channel->bytes_read < channel->read_buf_len) )
    {
	/*  Must have hit EOF  */
	return (read_pos);
    }
    channel->read_buf_pos = 0;
    channel->bytes_read = 0;
    bytes_to_read = length - read_pos;
    if (bytes_to_read > channel->read_buf_len)
    {
	/*  Call any tap functions that were registered  */
	c_call_callbacks (tap_list, NULL);
	/*  Read an intregal number of blocks directly  */
	bytes_to_read -= bytes_to_read % (int) channel->read_buf_len;
	if ( ( bytes_read = read (channel->fd, buffer + read_pos,
				  bytes_to_read) )
	    < 0 )
	{
	    /*  Error occurred  */
	    channel->ch_errno = errno;
	    if (read_pos > 0)
	    {
		errno = 0;
	    }
	    return (read_pos);
	}
	if (bytes_read < bytes_to_read)
	{
	    /*  Only read some bytes: hit End-Of-File  */
	    read_pos += bytes_read;
	    return (read_pos);
	}
	read_pos += bytes_read;
    }
    /*  Call any tap functions that were registered  */
    c_call_callbacks (tap_list, NULL);
    /*  Read one block into read buffer  */
    bytes_to_read = channel->read_buf_len;
    if ( ( bytes_read = read (channel->fd, channel->read_buffer,
			      bytes_to_read) )
	< 0 )
    {
	/*  Error reading complete block  */
	channel->ch_errno = errno;
    }
    if (bytes_read < bytes_to_read)
    {
	channel->ch_errno = -1;
    }
    channel->bytes_read = bytes_read;
    /*  Read what is needed from the read buffer  */
    read_pos += ch_read_disc (channel, buffer + read_pos, length - read_pos);
    errno = 0;
    return (read_pos);
}   /*  End Function ch_read_disc  */

static unsigned int ch_read_connection (Channel channel, char *buffer,
					unsigned int length)
/*  This routine will read a number of bytes from a connection channel and
    places them into a buffer.
    The channel object must be given by  channel  .
    The buffer to write the data into must be pointed to by  buffer  .
    The number of bytes to read into the buffer must be pointed to by  length
    If the number of bytes readable from the connection is equal to or more
    than  length  the routine will NOT block.
    The routine returns the number of bytes read.
*/
{
    flag was_error = TRUE;
    int bytes_available;
    unsigned int read_pos = 0;
    int bytes_to_read;
    int bytes_read;
    extern KCallbackList tap_list;
    extern char *sys_errlist[];
    static char function_name[] = "ch_read_connection";

    if ( (channel->bytes_read - channel->read_buf_pos) >= length )
    {
	/*  Read buffer has enough data in it  */
	m_copy (buffer, channel->read_buffer + channel->read_buf_pos,
		length);
	channel->read_buf_pos += length;
	return (length);
    }
    /*  Will need to read from connection  */
    /*  Copy what is left in the read buffer  */
    read_pos = channel->bytes_read - channel->read_buf_pos;
    m_copy (buffer, channel->read_buffer + channel->read_buf_pos,
	    read_pos);
    channel->read_buf_pos = 0;
    channel->bytes_read = 0;
    /*  Test if an error occurred  */
    if (channel->ch_errno > 0)
    {
	errno = channel->ch_errno;
	return (read_pos);
    }
    /*  Call any tap functions that were registered  */
    c_call_callbacks (tap_list, NULL);
    /*  Read the remainder directly  */
    bytes_to_read = length - read_pos;
    if ( ( bytes_read = r_read (channel->fd, buffer + read_pos,
				bytes_to_read) )
	< 0 )
    {
	/*  Error reading  */
	channel->ch_errno = errno;
	return (read_pos);
    }
    if (bytes_read < bytes_to_read)
    {
	/*  Only read some bytes  */
	switch (channel->type)
	{
	  case CHANNEL_TYPE_CONNECTION:
	    (void) fprintf (stderr,
			    "Connection channel closed while reading\n");
	    break;
	  case CHANNEL_TYPE_CHARACTER:
	    (void) fprintf (stderr,
			    "Character special channel closed while reading\n");
	    break;
	  case CHANNEL_TYPE_FIFO:
	    was_error = FALSE;
	    break;
	  default:
	    (void) fprintf (stderr, "Bad channel type: %u\n", channel->type);
	    a_prog_bug (function_name);
	    break;
	}
	if (was_error) (void) fprintf (stderr,
				       "Wanted: %d bytes  got: %d bytes\n",
				       bytes_to_read, bytes_read);
	return (read_pos);
    }
    read_pos += bytes_read;
    /*  Drain data on connection into buffer  */
    if ( ( bytes_available = r_get_bytes_readable (channel->fd) ) < 0 )
    {
	channel->ch_errno = errno;
	errno = 0;
	return (read_pos);
    }
    /*  Call any tap functions that were registered  */
    c_call_callbacks (tap_list, NULL);
    bytes_to_read = ( (bytes_available <= channel->read_buf_len) ?
		     bytes_available : channel->read_buf_len );
    if ( ( bytes_read = r_read (channel->fd, channel->read_buffer,
				bytes_to_read) )
	< bytes_to_read )
    {
	/*  Only read some bytes  */
	switch (channel->type)
	{
	  case CHANNEL_TYPE_CONNECTION:
	    (void) fprintf (stderr,
			    "Connection channel closed while draining\t%s\n",
			    sys_errlist[errno]);
	    break;
	  case CHANNEL_TYPE_CHARACTER:
	    (void) fprintf (stderr,
			    "Character special channel closed while draining\t%s\n",
			    sys_errlist[errno]);
	    break;
	  case CHANNEL_TYPE_FIFO:
	    (void) fprintf (stderr,
			    "FIFO channel closed while draining\t%s\n",
			    sys_errlist[errno]);
	    break;
	  default:
	    (void) fprintf (stderr, "Bad channel type: %u\n", channel->type);
	    a_prog_bug (function_name);
	    break;
	}
	exit (RV_SYS_ERROR);
    }
    channel->bytes_read = bytes_read;
    errno = 0;
    return (read_pos);
}   /*  End Function ch_read_connection  */

static unsigned int ch_read_memory (Channel channel, char *buffer,
				    unsigned int length)
/*  This routine will read a number of bytes from a memory channel and
    places them into a buffer.
    The channel object must be given by  channel  .
    The buffer to write the data into must be pointed to by  buffer  .
    The number of bytes to read into the buffer must be pointed to by  length
    The routine returns the number of bytes read.
*/
{
    unsigned int count, bytes_to_read;
    unsigned int bytes_left;
    CONST char *source;
/*
    static char function_name[] = "ch_read_memory";
*/

    bytes_left = channel->mem_buf_len - channel->mem_buf_read_pos;
    bytes_to_read = (length <= bytes_left) ? length : bytes_left;
    source = channel->memory_buffer + channel->mem_buf_read_pos;
    if (bytes_to_read < 16)
    {
	/*  Avoid function call overhead  */
	for (count = 0; count < bytes_to_read; ++count) *buffer++ = *source++;
    }
    else m_copy (buffer, source, bytes_to_read);
    channel->mem_buf_read_pos += bytes_to_read;
    return (bytes_to_read);
}   /*  End Function ch_read_memory  */

#ifdef DISABLED
/*  The following routine was an experiment that failed. It turns it to be
    faster to read/copy data into a buffer and then go back and swap the bytes,
    rather then read/copy and swap on the fly. Since twice the memory is being
    hit in a short time, cache utilisation drops. I also expect that for
    larger-than-ram buffers things will get much worse, as the disc head will
    have to travel from storage for the file to storage for the buffer and back
    at a great rate (causing expensive head moves). Reading the data in one go
    and then swapping requires the same data to be read but with much less head
    movement. One day if I have the time I'll test this.
*/
static unsigned int ch_read_memory_and_swap (Channel channel, char *buffer,
					     unsigned int num_blocks,
					     unsigned int block_size)
/*  [SUMMARY] Read blocks from a memory channel and swap bytes.
    [PURPOSE] This routine will read a number of blocks from a channel and
    places them into a buffer after swapping (reversing the order).
    <channel> The channel object.
    <buffer> The buffer to write the data into.
    <num_blocks> The number of blocks to read.
    <block_size> The size (in bytes) of each block.
    [RETURNS] The number of bytes read. Errors may cause partial blocks to be
    read.
*/
{
    unsigned int bytes_to_read, blocks_to_read;
    unsigned int bytes_left;
    unsigned int block_bytes_to_read;
    unsigned int bytes_to_read_in_block;
    unsigned int length = num_blocks * block_size;
    CONST char *source;
/*
    static char function_name[] = "ch_read_memory_and_swap";
*/

    bytes_left = channel->mem_buf_len - channel->mem_buf_read_pos;
    bytes_to_read = (length <= bytes_left) ? length : bytes_left;
    blocks_to_read = bytes_to_read / block_size;
    block_bytes_to_read = blocks_to_read * block_size;
    bytes_to_read_in_block = bytes_to_read - block_bytes_to_read;
    /*  Copy and swap blocks  */
    source = channel->memory_buffer + channel->mem_buf_read_pos;
    m_copy_and_swap_blocks (buffer, source, block_size, block_size, block_size,
			    blocks_to_read);
    /*  Plain copy of remaining bytes  */
    m_copy (buffer + block_bytes_to_read, source + block_bytes_to_read,
	    bytes_to_read_in_block);
    channel->mem_buf_read_pos += bytes_to_read;
    return (bytes_to_read);
}   /*  End Function ch_read_memory_and_swap  */
#endif

static unsigned int ch_write_descriptor (Channel channel, CONST char *buffer,
					 unsigned int length)
/*  This routine will write a number of bytes from a buffer to a descriptor
    channel.
    The channel object must be given by  channel  .
    The buffer to read the data from must be pointed to by  buffer  .
    The number of bytes to write from the buffer must be pointed to by  length
    The routine returns the number of bytes written.
*/
{
    int bytes_written;
    int bytes_to_write;
    unsigned int write_pos = 0;
/*
    static char function_name[] = "ch_write_descriptor";
*/

    if (channel->ch_errno != 0)
    {
	/*  Error occurred previously during writing  */
	errno = channel->ch_errno;
	return (0);
    }
    if ( length < (channel->write_buf_len - channel->write_buf_pos) )
    {
	/*  Write buffer will not be filled up  */
	m_copy (channel->write_buffer + channel->write_buf_pos,
		buffer, length);
	channel->write_buf_pos += length;
	return (length);
    }
    /*  Copy over as much as possible into write buffer  */
    write_pos = channel->write_buf_len - channel->write_buf_pos;
    m_copy (channel->write_buffer + channel->write_buf_pos, buffer,
	    write_pos);
    channel->write_buf_pos += write_pos;
    /*  Flush write buffer  */
    bytes_to_write = channel->write_buf_pos - channel->write_start_pos;
    if ( ( bytes_written = mywrite_raw (channel, channel->write_buffer,
					bytes_to_write) )
	< bytes_to_write )
    {
	channel->write_start_pos = 0;
	return (FALSE);
    }
    channel->write_start_pos = 0;
    /*  Write integral number of blocks directly  */
    bytes_to_write = length - write_pos;
    bytes_to_write -= bytes_to_write % (int) channel->write_buf_len;
    if ( ( bytes_written = mywrite_raw (channel, buffer + write_pos,
					bytes_to_write) )
	< bytes_to_write )
    {
	/*  Error writing  */
	channel->ch_errno = errno;
	return (write_pos + bytes_written);
    }
    write_pos += bytes_written;
    /*  Place remainder into write buffer  */
    m_copy (channel->write_buffer, buffer + write_pos, length - write_pos);
    channel->write_buf_pos = length - write_pos;
    return (length);
}  /*  End Function ch_write_descriptor  */

static int mywrite_raw (Channel channel, CONST char *buffer,
			unsigned int length)
/*  This routine will write a number of bytes from a buffer to a channel. This
    routine takes no account of buffering, nor a  write_converter_func  .
    The channel object must be given by  channel  .
    The buffer to read the data from must be pointed to by  buffer  .
    The number of bytes to write from the buffer must be pointed to by  length
    The routine returns the number of bytes written.
*/
{
    int bytes_to_write;
    unsigned int bytes_left;
    extern KCallbackList tap_list;
    static char function_name[] = "mywrite_raw";

    VERIFY_CHANNEL (channel);
    if (buffer == NULL)
    {
	(void) fprintf (stderr, "NULL buffer pointer passed\n");
	a_prog_bug (function_name);
    }
    if (length < 1) return (0);
    switch (channel->type)
    {
      case CHANNEL_TYPE_DISC:
	/*  Call any tap functions that were registered  */
	c_call_callbacks (tap_list, NULL);
	return ( write (channel->fd, buffer, length) );
/*
	break;
*/
      case CHANNEL_TYPE_CONNECTION:
      case CHANNEL_TYPE_CHARACTER:
      case CHANNEL_TYPE_FIFO:
	/*  Call any tap functions that were registered  */
	c_call_callbacks (tap_list, NULL);
	return ( r_write (channel->fd, buffer, length) );
/*
	break;
*/
      case CHANNEL_TYPE_MEMORY:
      case CHANNEL_TYPE_MMAP:
	bytes_left = channel->mem_buf_len - channel->mem_buf_write_pos;
	bytes_to_write = (length <= bytes_left) ? length : bytes_left;
	m_copy (channel->memory_buffer + channel->mem_buf_write_pos,
		buffer, bytes_to_write);
	channel->mem_buf_write_pos += bytes_to_write;
	return (bytes_to_write);
/*
	break;
*/
      case CHANNEL_TYPE_DOCK:
	(void) fprintf (stderr,
			"Write operation not permitted on dock channel\n");
	a_prog_bug (function_name);
	break;
      case CHANNEL_TYPE_ASYNCHRONOUS:
	(void) fprintf (stderr,
			"Write operation not permitted on raw asynchronous\n");
	a_prog_bug (function_name);
	break;
      default:
	(void) fprintf (stderr, "Invalid channel type\n");
	a_prog_bug (function_name);
	break;
    }
    return (-1);
}   /*  End Function mywrite_raw  */

/*LINTLIBRARY*/
/*PREFIX:"ch_"*/
/*  channel.c

    This code provides Channel objects.

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

    Last updated by Richard Gooch   16-SEP-1993: Fixed memory mapping for
  Convex.


*/
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <karma.h>
#include <karma_m.h>
#include <karma_r.h>
#include <karma_a.h>
#include <os.h>
#ifdef HAS_MMAP
#include <sys/mman.h>
#  ifdef ARCH_dec
caddr_t mmap ();
#  endif
#endif

/*  Internal definition of Channel object  */
typedef struct channel_type * Channel;

#define CHANNEL_DEFINED
#include <karma_ch.h>

#if __STDC__ == 1
#  define MAGIC_NUMBER 3498653274U
#else
#  define MAGIC_NUMBER (unsigned int) 3498653274
#endif
#define CONNECTION_BUF_SIZE (unsigned int) 4096
#define DEFAULT_BLOCK_SIZE (unsigned int) 4096

#define MMAP_LARGE_SIZE 1048576

#define CHANNEL_TYPE_DISC (unsigned int) 0
#define CHANNEL_TYPE_CONNECTION (unsigned int) 1
#define CHANNEL_TYPE_MEMORY (unsigned int) 2
#define CHANNEL_TYPE_DOCK (unsigned int) 3
#define CHANNEL_TYPE_ASYNCHRONOUS (unsigned int) 4
#define CHANNEL_TYPE_MMAP (unsigned int) 5
#define CHANNEL_TYPE_CHARACTER (unsigned int) 6
#define CHANNEL_TYPE_FIFO (unsigned int) 7
#define CHANNEL_TYPE_UNDEFINED (unsigned int) 8

struct channel_type
{
    unsigned int magic_number;
    unsigned int type;
    int fd;
    int errno;
    char *read_buffer;
    unsigned int read_buf_len;
    unsigned int read_buf_pos;
    unsigned int bytes_read;
    char *write_buffer;
    unsigned int write_buf_len;
    unsigned int write_buf_pos;
    char *memory_buffer;
    unsigned int mem_buf_len;
    unsigned int mem_buf_read_pos;
    unsigned int mem_buf_write_pos;
    flag mem_buf_allocated;
    flag local;
    unsigned int mmap_access_count;
    unsigned int abs_read_pos;
    unsigned int abs_write_pos;
    struct channel_type *prev;
    struct channel_type *next;
};

/*  Private data follows  */
static Channel first_channel = NULL;
static flag registered_exit_func = FALSE;

/*  Declarations of private functions follow  */

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
	atexit (ch_close_all_channels, NULL);
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
    (*channel).magic_number = MAGIC_NUMBER;
    (*channel).type = CHANNEL_TYPE_UNDEFINED;
    (*channel).fd = -1;
    (*channel).errno = 0;
    (*channel).read_buffer = NULL;
    (*channel).read_buf_len = 0;
    (*channel).read_buf_pos = 0;
    (*channel).bytes_read = 0;
    (*channel).write_buffer = NULL;
    (*channel).write_buf_len = 0;
    (*channel).write_buf_pos = 0;
    (*channel).memory_buffer = NULL;
    (*channel).mem_buf_len = 0;
    (*channel).mem_buf_read_pos = 0;
    (*channel).mem_buf_write_pos = 0;
    (*channel).mmap_access_count = 0;
    (*channel).abs_read_pos = 0;
    (*channel).abs_write_pos = 0;
    /*  Place channel object into list  */
    (*channel).prev = NULL;
    (*channel).next = first_channel;
    if (first_channel != NULL)
    {
	(*first_channel).prev = channel;
    }
    first_channel = channel;
    return (channel);
}   /*  End Function ch_alloc  */

static unsigned int ch_read_disc (channel, buffer, length)
/*  This routine will read a number of bytes from a disc channel and places
    them into a buffer.
    The channel object must be given by  channel  .
    The buffer to write the data into must be pointed to by  buffer  .
    The number of bytes to read into the buffer must be pointed to by  length
    The routine returns the number of bytes read.
*/
Channel channel;
char *buffer;
unsigned int length;
{
    unsigned int read_pos = 0;
    int bytes_to_read;
    int bytes_read;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "ch_read_disc";

    if ( ( (*channel).bytes_read - (*channel).read_buf_pos ) >= length )
    {
	/*  Read buffer has enough data in it  */
	m_copy (buffer, (*channel).read_buffer + (*channel).read_buf_pos,
		length);
	(*channel).read_buf_pos += length;
	return (length);
    }
    /*  Need to read from disc  */
    /*  Copy what is left in the read buffer  */
    read_pos = (*channel).bytes_read - (*channel).read_buf_pos;
    m_copy (buffer, (*channel).read_buffer + (*channel).read_buf_pos,
	    read_pos);
    /*  Test if an error occurred  */
    if ( (*channel).errno != 0 )
    {
	errno = ( (*channel).errno > 0 ) ? (*channel).errno : 0;
	(*channel).read_buf_pos = 0;
	(*channel).bytes_read = 0;
	return (read_pos);
    }
    /*  Test if that's all there is in the file  */
    if ( ( (*channel).bytes_read > 0 ) &&
	( (*channel).bytes_read < (*channel).read_buf_len ) )
    {
	/*  Must have hit EOF  */
	return (read_pos);
    }
    (*channel).read_buf_pos = 0;
    (*channel).bytes_read = 0;
    bytes_to_read = length - read_pos;
    if (bytes_to_read > (*channel).read_buf_len)
    {
	/*  Read an intregal number of blocks directly  */
	bytes_to_read -= bytes_to_read % (int) (*channel).read_buf_len;
	if ( ( bytes_read = read ( (*channel).fd, buffer + read_pos,
				  bytes_to_read ) )
	    < 0 )
	{
	    /*  Error occurred  */
	    (*channel).errno = errno;
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
    /*  Read one block into read buffer  */
    bytes_to_read = (*channel).read_buf_len;
    if ( ( bytes_read = read ( (*channel).fd, (*channel).read_buffer,
			      bytes_to_read ) )
	< 0 )
    {
	/*  Error reading complete block  */
	(*channel).errno = errno;
    }
    if (bytes_read < bytes_to_read)
    {
	(*channel).errno = -1;
    }
    (*channel).bytes_read = bytes_read;
    /*  Read what is needed from the read buffer  */
    read_pos += ch_read_disc (channel, buffer + read_pos, length - read_pos);
    errno = 0;
    return (read_pos);
}   /*  End Function ch_read_disc  */

static unsigned int ch_read_connection (channel, buffer, length)
/*  This routine will read a number of bytes from a connection channel and
    places them into a buffer.
    The channel object must be given by  channel  .
    The buffer to write the data into must be pointed to by  buffer  .
    The number of bytes to read into the buffer must be pointed to by  length
    If the number of bytes readable from the connection is equal to or more
    than  length  the routine will NOT block.
    The routine returns the number of bytes read.
*/
Channel channel;
char *buffer;
unsigned int length;
{
    int bytes_available;
    unsigned int read_pos = 0;
    int bytes_to_read;
    int bytes_read;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "ch_read_connection";

    if ( ( (*channel).bytes_read - (*channel).read_buf_pos ) >= length )
    {
	/*  Read buffer has enough data in it  */
	m_copy (buffer, (*channel).read_buffer + (*channel).read_buf_pos,
		length);
	(*channel).read_buf_pos += length;
	return (length);
    }
    /*  Will need to read from connection  */
    /*  Copy what is left in the read buffer  */
    read_pos = (*channel).bytes_read - (*channel).read_buf_pos;
    m_copy (buffer, (*channel).read_buffer + (*channel).read_buf_pos,
	    read_pos);
    (*channel).read_buf_pos = 0;
    (*channel).bytes_read = 0;
    /*  Test if an error occurred  */
    if ( (*channel).errno < 0 )
    {
	errno = (*channel).errno;
	return (read_pos);
    }
    /*  Read the remainder directly  */
    bytes_to_read = length - read_pos;
    if ( ( bytes_read = r_read ( (*channel).fd, buffer + read_pos,
				bytes_to_read ) )
	< 0 )
    {
	/*  Error reading  */
	(*channel).errno = errno;
	return (read_pos);
    }
    if (bytes_read < bytes_to_read)
    {
	/*  Only read some bytes  */
	(void) fprintf (stderr,
			"Connection channel closed while reading\n");
	(void) fprintf (stderr, "Wanted: %d bytes  got: %d bytes\n",
			bytes_to_read, bytes_read);
	return (read_pos);
    }
    read_pos += bytes_read;
    /*  Drain data on connection into buffer  */
    if ( ( bytes_available = r_get_bytes_readable ( (*channel).fd ) ) < 0 )
    {
	(*channel).errno = errno;
	errno = 0;
	return (read_pos);
    }
    bytes_to_read = ( (bytes_available <= (*channel).read_buf_len) ?
		     bytes_available : (*channel).read_buf_len );
    if ( ( bytes_read = r_read ( (*channel).fd, (*channel).read_buffer,
				bytes_to_read ) )
	< 0 )
    {
	(*channel).errno = errno;
	errno = 0;
	return (read_pos);
    }
    if (bytes_read < bytes_to_read)
    {
	/*  Only read some bytes  */
	(void) fprintf (stderr,
			"Connection channel closed while reading\n");
	(void) fprintf (stderr, "Wanted: %d bytes  got: %d bytes\n",
			bytes_to_read, bytes_read);
	return (read_pos);
    }
    (*channel).bytes_read = bytes_read;
    errno = 0;
    return (read_pos);
}   /*  End Function ch_read_connection  */

static unsigned int ch_read_memory (channel, buffer, length)
/*  This routine will read a number of bytes from a memory channel and
    places them into a buffer.
    The channel object must be given by  channel  .
    The buffer to write the data into must be pointed to by  buffer  .
    The number of bytes to read into the buffer must be pointed to by  length
    The routine returns the number of bytes read.
*/
Channel channel;
char *buffer;
unsigned int length;
{
    unsigned int bytes_to_read;
    unsigned int bytes_left;
    static char function_name[] = "ch_read_memory";

    bytes_left = (*channel).mem_buf_len - (*channel).mem_buf_read_pos;
    bytes_to_read = (length <= bytes_left) ? length : bytes_left;
    m_copy (buffer,
	    (*channel).memory_buffer + (*channel).mem_buf_read_pos,
	    bytes_to_read);
    (*channel).mem_buf_read_pos += bytes_to_read;
    return (bytes_to_read);
}   /*  End Function ch_read_memory  */

static unsigned int ch_write_descriptor (channel, buffer, length)
/*  This routine will write a number of bytes from a buffer to a descriptor
    channel.
    The channel object must be given by  channel  .
    The buffer to read the data from must be pointed to by  buffer  .
    The number of bytes to write from the buffer must be pointed to by  length
    The routine returns the number of bytes written.
*/
Channel channel;
char *buffer;
unsigned int length;
{
    int bytes_written;
    int bytes_to_write;
    unsigned int write_pos = 0;
    int (*write_func) ();
    int write ();
    ERRNO_TYPE errno;
    static char function_name[] = "ch_write_descriptor";

    if ( (*channel).type == CHANNEL_TYPE_DISC )
    {
	write_func = write;
    }
    else
    {
	write_func = r_write;
    }
    if ( (*channel).errno != 0 )
    {
	/*  Error occurred previously during writing  */
	errno = (*channel).errno;
	return (0);
    }
    if ( length < ( (*channel).write_buf_len - (*channel).write_buf_pos ) )
    {
	/*  Write buffer will not be filled up  */
	m_copy ( (*channel).write_buffer + (*channel).write_buf_pos,
		buffer, length );
	(*channel).write_buf_pos += length;
	return (length);
    }
    /*  Copy over as much as possible into write buffer  */
    write_pos = (*channel).write_buf_len - (*channel).write_buf_pos;
    m_copy ( (*channel).write_buffer + (*channel).write_buf_pos, buffer,
	    write_pos );
    (*channel).write_buf_pos += write_pos;
    if (ch_flush (channel) != TRUE)
    {
	/*  Error flushing buffer  */
	return (write_pos);
    }
    /*  Write integral number of blocks directly  */
    bytes_to_write = length - write_pos;
    bytes_to_write -= bytes_to_write % (int) (*channel).write_buf_len;
    if ( ( bytes_written = (*write_func) ( (*channel).fd,
					  buffer + write_pos,
					  bytes_to_write ) )
	< 0 )
    {
	/*  Error writing  */
	(*channel).errno = errno;
	return (write_pos);
    }
    if (bytes_written < bytes_to_write)
    {
	(void) fprintf (stderr,
			"Write did not write all data but did not return error\n");
	a_prog_bug (function_name);
    }
    write_pos += bytes_written;
    /*  Place remainder into write buffer  */
    m_copy ( (*channel).write_buffer, buffer + write_pos, length - write_pos );
    (*channel).write_buf_pos = length - write_pos;
    return (length);
}  /*  End Function ch_write_descriptor  */

static unsigned int ch_write_memory (channel, buffer, length)
/*  This routine will write a number of bytes from a buffer to a memory channel
    The channel object must be given by  channel  .
    The buffer to read the data from must be pointed to by  buffer  .
    The number of bytes to write into the channel must be pointed to by  length
    The routine returns the number of bytes written.
*/
Channel channel;
char *buffer;
unsigned int length;
{
    unsigned int bytes_to_write;
    unsigned int bytes_left;
    static char function_name[] = "ch_write_memory";

    bytes_left = (*channel).mem_buf_len - (*channel).mem_buf_write_pos;
    bytes_to_write = (length <= bytes_left) ? length : bytes_left;
    m_copy ( (*channel).memory_buffer + (*channel).mem_buf_write_pos,
	    buffer, bytes_to_write );
    (*channel).mem_buf_write_pos += bytes_to_write;
    return (bytes_to_write);
}   /*  End Function ch_write_memory  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
Channel ch_open_file (filename, type)
/*  This routine will open a file channel. The file may be a regular disc file,
    a named FIFO or a character special device. The channel may be later tested
    to determine what the true channel type is by calling routines such as:
    ch_test_for_asynchronous  and  ch_test_for_io  .
    The pathname of the file to open must be given by  filename  .
    The mode of the file must be pointed to by  type  .The following modes are
    allowed:
        "r"         open for reading
	"w"         open (truncate) or create for writing
	"a"         open or create for writing at end of file (append)
	"r+"        open for update (reading and writing)
	"w+"        open for reading and writing after truncation
	"a+"        open or create for update (reading and writing) at EOF
    Note that for character special files and named FIFOs, these modes
    degenerate into read-write, read-only and write-only.
    The routine returns a channel object on success, else it returns NULL.
*/
char *filename;
char *type;
{
    flag read_flag = FALSE;
    flag write_flag = FALSE;
    int flags;
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
    else
    {
	(void) fprintf (stderr, "Illegal access mode: \"%s\"\n", type);
	a_prog_bug (function_name);
    }
#ifdef S_IRUSR
    mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IWGRP | S_IROTH;
#else
    mode = 0664;  /*  Octal!  */
#endif
    /*  Set sticky bit if defined  */
#ifdef S_ISVTX
    mode |= S_ISVTX;
#endif
    /*  Open file descriptor  */
    if ( ( (*channel).fd = r_open_file (filename, flags, mode, &filetype,
					&blocksize) ) < 0 )
    {
	(void) ch_close (channel);
	return (NULL);
    }
    /*  Set channel type  */
    switch (filetype)
    {
      case KFTYPE_DISC:
	(*channel).type = CHANNEL_TYPE_DISC;
	break;
      case KFTYPE_CHARACTER:
	(*channel).type = CHANNEL_TYPE_CHARACTER;
	break;
      case KFTYPE_FIFO:
	(*channel).type = CHANNEL_TYPE_FIFO;
	break;
      default:
	(void) fprintf (stderr, "Illegal filetype: %u\n", filetype);
	a_prog_bug (function_name);
	break;
    }
    if (blocksize == 0) blocksize = DEFAULT_BLOCK_SIZE;
    if (read_flag == TRUE)
    {
	/*  Allocate read buffer  */
	if ( ( (*channel).read_buffer = m_alloc (blocksize) ) == NULL )
	{
	    m_error_notify (function_name, "read buffer");
	    (void) ch_close (channel);
	    return (NULL);
	}
	(*channel).read_buf_len = blocksize;
	(*channel).read_buf_pos = 0;
	(*channel).bytes_read = 0;
    }
    if (write_flag == TRUE)
    {
	/*  Allocate write buffer  */
	if ( ( (*channel).write_buffer = m_alloc (blocksize) ) == NULL )
	{
	    m_error_notify (function_name, "write buffer");
	    (void) ch_close (channel);
	    return (NULL);
	}
	(*channel).write_buf_len = blocksize;
	(*channel).write_buf_pos = 0;
    }
    return (channel);
}   /*  End Function ch_open_file  */

/*PUBLIC_FUNCTION*/
Channel ch_map_disc (filename, option, writeable, update_on_write)
/*  This routine will open a memory channel with the memory pages being mapped
    from a disc file. The disc file must already exist.
    The pathname of the file to open must be given by  filename  .
    The channel may be opened as an ordinary disc file depending on the value
    of  option  .Legal values for this are:
        K_CH_MAP_NEVER           Never map
	K_CH_MAP_LARGE_LOCAL     Map if local filesystem and file size > 1MB
	K_CH_MAP_LOCAL           Map if local filesystem
	K_CH_MAP_LARGE           Map if file over 1 MByte
	K_CH_MAP_IF_AVAILABLE    Map if operating system supports it
	K_CH_MAP_ALWAYS          Always map, fail if not supported.
    If the file is not mapped then the routine will attempt to open an ordinary
    disc channel, provided  option  is not equal to  K_CH_MAP_ALWAYS  .
    If the file is opened as a disc channel the access mode is: "r".
    If the mapped pages are to be writeable, the value of  writeable  must be
    TRUE. If this is FALSE and the memory pages are written to, a segmentation
    fault occurs.
    If the disc file should be updated when the memory pages are written to,
    the value of  update_on_write  must be TRUE. If this is FALSE, then a write
    to a memory page causes the page to be copied into swap space and the
    process retains a private copy of the page from this point on.
    If  update_on_write  is FALSE and  writeable  is TRUE, then some systems
    require the allocation of normal virtual memory equal to the size of the
    disc file at the time of mapping, while others will dynamically allocate
    memory from swap space as pages are written into.
    In the latter case, some systems will cause a segmentation fault if swap
    space is exhausted, while other systems wait for space to be freed.
    The channel may be queried to determine if it has been memory mapped using
    the call:  ch_test_for_mmap
    The routine returns a channel object on success, else it returns NULL.
*/
char *filename;
unsigned int option;
flag writeable;
flag update_on_write;
{
    int open_flags;
#ifdef HAS_MMAP
    flag memory_map = FALSE;
    int mmap_flags = 0;
    int mmap_prot = PROT_READ;
#  ifdef ARCH_convex
    unsigned int map_len;
#  endif
#endif
    struct stat statbuf;
    Channel channel;
    ERRNO_TYPE errno;
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
    if ( ( (*channel).fd = open (filename, open_flags, 0) ) < 0 )
    {
	(void) ch_close (channel);
	return (NULL);
    }
    /*  Get stats on file  */
    if (fstat ( (*channel).fd, &statbuf ) != 0)
    {
	(void) fprintf (stderr, "Error getting file stats\n");
	(void) ch_close (channel);
	return (NULL);
    }
    /*  Determine if file should be memory mapped  */
    switch (option)
    {
      case K_CH_MAP_NEVER:
	break;
      case K_CH_MAP_LARGE_LOCAL:
	/*  Check if large, local file  */
	if ( (statbuf.st_size >= MMAP_LARGE_SIZE) &&
	    (statbuf.st_dev >= 0) )
	{
	    memory_map = TRUE;
	}
	break;
      case K_CH_MAP_LOCAL:
	/*  Check if local  */
	if (statbuf.st_dev >= 0)
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
#  ifdef ARCH_convex
    map_len = statbuf.st_size;
#endif
    if ( ( (*channel).memory_buffer = mmap ( (caddr_t ) NULL,
#  ifdef ARCH_convex  /*  Why the fuck to they have to be different?  */
					    &map_len,
#  else
					    (size_t) statbuf.st_size,
#  endif
					    mmap_prot, mmap_flags,
					    (*channel).fd, (off_t) 0 ) )
	== (caddr_t) -1 )
    {
	/*  Failed  */
	(void) fprintf (stderr, "Error memory mapping file: \"%s\"\t%s\n",
			filename, sys_errlist[errno]);
	(void) ch_close (channel);
	return (NULL);
    }
    /*  Set channel type  */
    (*channel).type = CHANNEL_TYPE_MMAP;
    (*channel).mem_buf_allocated = FALSE;
    (*channel).mem_buf_len = statbuf.st_size;
    return (channel);
#else  /*  HAS_MMAP  */
    if (option != K_CH_MAP_ALWAYS) return ( ch_open_file (filename, "r") );
    (void) fprintf (stderr, "Memory mapping not supported\n");
    return (NULL);
#endif  /*  HAS_MMAP  */
}   /*  End Function ch_map_disc  */

/*PUBLIC_FUNCTION*/
Channel ch_open_connection (host_addr, port_number)
/*  This routine will open a full-duplex connection channel to a server running
    on another host machine.
    The Internet address of the host machine must be given by  host_addr  .
    If this is 0 the connection is made to a server running on the same machine
    using the most efficient transport available.
    The port number to connect to must be given by  port_number  .This should
    not be confused with Internet port numbers.
    The use of this routine is not recommended: see the  conn_  package for
    more powerful functionality.
    The routine returns a channel object on success, else it returns NULL.
*/
unsigned long host_addr;
unsigned int port_number;
{
    Channel channel;
    static char function_name[] = "ch_open_connection";

    if ( ( channel = ch_alloc () ) == NULL )
    {
	return (NULL);
    }
    /*  Set channel type  */
    (*channel).type = CHANNEL_TYPE_CONNECTION;
    /*  Open connection descriptor  */
    if ( ( (*channel).fd = r_connect_to_port (host_addr, port_number,
					      &(*channel).local) ) < 0 )
    {
	(void) ch_close (channel);
	return (NULL);
    }
    /*  Allocate read buffer  */
    if ( ( (*channel).read_buffer = m_alloc (CONNECTION_BUF_SIZE) ) == NULL )
    {
	m_error_notify (function_name, "read buffer");
	(void) ch_close (channel);
	return (NULL);
    }
    (*channel).read_buf_len = CONNECTION_BUF_SIZE;
    (*channel).read_buf_pos = 0;
    (*channel).bytes_read = 0;
    /*  Allocate write buffer  */
    if ( ( (*channel).write_buffer = m_alloc (CONNECTION_BUF_SIZE) ) == NULL )
    {
	m_error_notify (function_name, "write buffer");
	(void) ch_close (channel);
	return (NULL);
    }
    (*channel).write_buf_len = CONNECTION_BUF_SIZE;
    (*channel).write_buf_pos = 0;
    return (channel);
}   /*  End Function ch_open_connection  */

/*PUBLIC_FUNCTION*/
Channel ch_open_memory (buffer, size)
/*  This routine will open a memory channel. A memory channel behaves like a
    disc channel with a limited (specified) file (device) size.
    Data is undefined when reading before writing has occurred.
    The buffer to use must be pointed to by  buffer  .If this is NULL, the
    routine will allocate a buffer of the specified size which is automatically
    deallocated upon closure of the channel.
    The size of the buffer to allocate must be given by  size  .
    The routine returns a channel object on success, else it returns NULL.
*/
char *buffer;
unsigned int size;
{
    Channel channel;
    static char function_name[] = "ch_open_memory";

    if ( ( channel = ch_alloc () ) == NULL )
    {
	return (NULL);
    }
    /*  Set channel type  */
    (*channel).type = CHANNEL_TYPE_MEMORY;
    if (buffer == NULL)
    {
	/*  Must allocate a buffer  */
	if ( ( (*channel).memory_buffer = m_alloc (size) ) == NULL )
	{
	    m_error_notify (function_name, "channel memory buffer");
	    (void) ch_close (channel);
	    return (NULL);
	}
	(*channel).mem_buf_allocated = TRUE;
    }
    else
    {
	/*  Use specified buffer  */
	(*channel).memory_buffer = buffer;
	(*channel).mem_buf_allocated = FALSE;
    }
    (*channel).mem_buf_len = size;
    return (channel);
}   /*  End Function ch_open_memory  */

/*PUBLIC_FUNCTION*/
Channel ch_accept_on_dock (dock, addr)
/*  This routine will open a full-duplex connection channel to the first
    connection to a waiting dock.
    The dock must be given by  dock  .
    The address of the host connecting to the dock will be written to the
    storage pointed to by  addr  .
    The routine returns a channel object on success, else it returns NULL.
*/
Channel dock;
unsigned long *addr;
{
    Channel channel;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "ch_accept_on_dock";

    if ( (dock == NULL) || (addr == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*dock).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is a dock  */
    if ( (*dock).type != CHANNEL_TYPE_DOCK )
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
    (*channel).type = CHANNEL_TYPE_CONNECTION;
    /*  Accept connection  */
    if ( ( (*channel).fd = r_accept_connection_on_dock ( (*dock).fd, addr,
							&(*channel).local ) )
	< 0 )
    {
	(void) fprintf (stderr, "Error accepting connection\n");
	(void) ch_close (channel);
	return (NULL);
    }
    /*  Allocate read buffer  */
    if ( ( (*channel).read_buffer = m_alloc (CONNECTION_BUF_SIZE) ) == NULL )
    {
	m_error_notify (function_name, "read buffer");
	(void) ch_close (channel);
	return (NULL);
    }
    (*channel).read_buf_len = CONNECTION_BUF_SIZE;
    (*channel).read_buf_pos = 0;
    (*channel).bytes_read = 0;
    /*  Allocate write buffer  */
    if ( ( (*channel).write_buffer = m_alloc (CONNECTION_BUF_SIZE) ) == NULL )
    {
	m_error_notify (function_name, "write buffer");
	(void) ch_close (channel);
	return (NULL);
    }
    (*channel).write_buf_len = CONNECTION_BUF_SIZE;
    (*channel).write_buf_pos = 0;
    return (channel);
}   /*  End Function ch_accept_on_dock  */

/*PUBLIC_FUNCTION*/
Channel *ch_alloc_port (port_number, retries, num_docks)
/*  This routine will allocate a Karma port for the module so that it can
    operate as a server (able to receive network connections).
    The port number to allocate must be pointed to by  port_number  .The
    routine will write the actual port number allocated to this address. This
    must point to an address which lies on an  int  boundary.
    The number of succsessive port numbers to attempt to allocate before giving
    up must be given by  retries  .If this is 0, then the routine will give up
    immediately if the specified port number is in use.
    The routine will create a number of docks for one port. Each dock is an
    alternative access point for other modules to connect to this port.
    The number of docks allocated will be written to the storage pointed to by
    num_docks  .This must point to an address which lies on an  int
    boundary.
    The close-on-exec flags of the docks are set such that the docks will
    close on a call to execve(2V).
    The docks are placed into blocking mode.
    The routine returns a pointer to an array of channel docks on success,
    else it returns NULL.
*/
unsigned int *port_number;
unsigned int retries;
unsigned int *num_docks;
{
    unsigned int dock_count;
    int *docks;
    Channel *ch_docks;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "ch_alloc_port";

    if ( (port_number == NULL) || (num_docks == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Check alignment of  port_number  */
    if ( (int) port_number % sizeof (int) != 0 )
    {
	(void) fprintf (stderr,
			"Pointer to port number storage does not lie on an  int  boundary\n");
	a_prog_bug (function_name);
    }
    /*  Check alignment of  num_docks  */
    if ( (int) num_docks % sizeof (int) != 0 )
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
	(*ch_docks[dock_count]).fd = docks[dock_count];
	(*ch_docks[dock_count]).type = CHANNEL_TYPE_DOCK;
    }
    return (ch_docks);
}   /*  End Function ch_alloc_port  */

/*PUBLIC_FUNCTION*/
flag ch_close (channel)
/*  This routine will close a channel object. The write buffer will be flushed
    prior to closure.
    The channel object must be given by  channel  .
    The routine returns TRUE on success, else it returns FALSE.
*/
Channel channel;
{
    flag return_value = TRUE;
    extern Channel first_channel;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "ch_close";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    /*  Close descriptor if open  */
    if ( (*channel).fd > -1 )
    {
	/*  Open file descriptor: flush buffer and close descriptor  */
	if (ch_flush (channel) != TRUE)
	{
	    return_value = FALSE;
	}
	switch ( (*channel).type )
	{
	  case CHANNEL_TYPE_MMAP:
#ifdef HAS_MMAP
	    if (munmap ( (*channel).memory_buffer, (*channel).mem_buf_len )
		!= 0)
	    {
		(void) fprintf (stderr, "Error unmapping\t%s\n",
				sys_errlist[errno]);
		exit (RV_SYS_ERROR);
	    }
	    (*channel).memory_buffer = NULL;
#else
	    (void) fprintf (stderr,
			    "Channel memory mapped but platform does not support\n");
	    a_prog_bug (function_name);
#endif
	    /*  Fall through for closure of descriptor  */
	  case CHANNEL_TYPE_DISC:
	  case CHANNEL_TYPE_CHARACTER:
	  case CHANNEL_TYPE_FIFO:
	    if (close ( (*channel).fd ) != 0)
	    {
		(void) fprintf (stderr, "Error closing descriptor: %d\t%s\n",
				(*channel).fd, sys_errlist[errno]);
		return_value = FALSE;
	    }
	    break;
	  case CHANNEL_TYPE_CONNECTION:
	    return_value = r_close_connection ( (*channel).fd );
	    break;
	  case CHANNEL_TYPE_DOCK:
	    r_close_dock ( (*channel).fd );
	    return_value = TRUE;
	    break;
	  default:
	    return_value = TRUE;
	    break;
	}
    }
    /*  Deallocate buffers  */
    if ( (*channel).read_buffer != NULL )
    {
	m_free ( (*channel).read_buffer );
    }
    if ( (*channel).write_buffer != NULL )
    {
	m_free ( (*channel).write_buffer );
    }
    if ( ( (*channel).memory_buffer != NULL ) &&
	( (*channel).mem_buf_allocated == TRUE ) )
    {
	m_free ( (*channel).memory_buffer );
    }
    /*  Remove channel object from list  */
    if ( (*channel).next != NULL )
    {
	/*  Another entry further in the list  */
	(* (*channel).next ).prev = (*channel).prev;
    }
    if ( (*channel).prev != NULL )
    {
	/*  Another entry previous in the list  */
	(* (*channel).prev ).next = (*channel).next;
    }
    if (channel == first_channel)
    {
	/*  Channel is first in list: make next entry the first  */
	first_channel = (*channel).next;
    }
    /*  Kill magic number entry for security  */
    (*channel).magic_number = 0;
    /*  Deallocate channel object now that there are no more references  */
    m_free ( (char *) channel );
    return (return_value);
}   /*  End Function ch_close  */

/*PUBLIC_FUNCTION*/
flag ch_flush (channel)
/*  This routine will flush the write buffer of a channel object.
    The channel object must be given by  channel  .
    The routine returns TRUE on success, else it returns FALSE.
*/
Channel channel;
{
    int bytes_to_write;
    int bytes_written;
    int (*write_func) ();
    int write ();
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "ch_flush";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    if ( (*channel).fd < 0 )
    {
	return (TRUE);
    }
    if ( (*channel).type == CHANNEL_TYPE_MMAP )
    {
	return (TRUE);
    }
    if ( (*channel).type == CHANNEL_TYPE_DISC )
    {
	write_func = write;
    }
    else
    {
	write_func = r_write;
    }
    if ( (*channel).write_buf_pos > 0 )
    {
	/*  Flush write buffer  */
	bytes_to_write = (*channel).write_buf_pos;
	if ( ( bytes_written = (*write_func) ( (*channel).fd,
					      (*channel).write_buffer,
					      bytes_to_write ) ) < 0 )
	{
	    (void) fprintf (stderr, "Error flushing buffer\t%s\n",
			    sys_errlist[errno]);
	    return (FALSE);
	}
	if (bytes_written < bytes_to_write)
	{
	    (void) fprintf (stderr,
			    "Write did not write all data but did not return error\n");
	    a_prog_bug (function_name);
	}
	(*channel).write_buf_pos = 0;
    }
    return (TRUE);
}   /*  End Function ch_flush  */

/*PUBLIC_FUNCTION*/
unsigned int ch_read (channel, buffer, length)
/*  This routine will read a number of bytes from a channel and places them
    into a buffer.
    The channel object must be given by  channel  .
    The buffer to write the data into must be pointed to by  buffer  .
    The number of bytes to read into the buffer must be pointed to by  length
    If the channel is a connection and the number of bytes readable from the
    connection is equal to or more than  length  the routine will NOT block.
    The routine returns the number of bytes read.
*/
Channel channel;
char *buffer;
unsigned int length;
{
    unsigned int num_read;
    static char function_name[] = "ch_read";

    if ( (channel == NULL) || (buffer == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    switch ( (*channel).type )
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
      default:
	(void) fprintf (stderr, "Invalid channel type\n");
	a_prog_bug (function_name);
	break;
    }
    (*channel).abs_read_pos += num_read;
    return (num_read);
}   /*  End Function ch_read  */

/*PUBLIC_FUNCTION*/
unsigned int ch_write (channel, buffer, length)
/*  This routine will write a number of bytes from a buffer to a channel.
    The channel object must be given by  channel  .
    The buffer to read the data from must be pointed to by  buffer  .
    The number of bytes to write from the buffer must be pointed to by  length
    The routine returns the number of bytes written.
*/
Channel channel;
char *buffer;
unsigned int length;
{
    unsigned int num_written;
    static char function_name[] = "ch_write";

    if ( (channel == NULL) || (buffer == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    switch ( (*channel).type )
    {
      case CHANNEL_TYPE_DISC:
      case CHANNEL_TYPE_CONNECTION:
      case CHANNEL_TYPE_CHARACTER:
      case CHANNEL_TYPE_FIFO:
	num_written = ch_write_descriptor (channel, buffer, length);
	break;
      case CHANNEL_TYPE_MEMORY:
      case CHANNEL_TYPE_MMAP:
	num_written = ch_write_memory (channel, buffer, length);
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
      default:
	(void) fprintf (stderr, "Invalid channel type\n");
	a_prog_bug (function_name);
	break;
    }
    (*channel).abs_write_pos += num_written;
    return (num_written);
}   /*  End Function ch_write  */

/*PUBLIC_FUNCTION*/
void ch_close_all_channels ()
/*  This routine will close all open channels.
    The routine is meant to be called from the exit(3) function.
    The routine returns nothing.
*/
{
    extern Channel first_channel;

    while (first_channel != NULL)
    {
	(void) ch_close (first_channel);
    }
}   /*  End Function ch_close_all_channels  */

/*PUBLIC_FUNCTION*/
flag ch_seek (channel, position)
/*  This routine will position the read and write pointers for a channel.
    The channel object must be given by  channel  .
    The position (relative to the start of the channel data) must be given
    by  position  .
    This routine cannot be used for connection channels.
    The routine returns TRUE on success,
    else it returns FALSE (indicating a seek request beyond the channel limits)
*/
Channel channel;
unsigned long position;
{
    static char function_name[] = "ch_seek";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    switch ( (*channel).type )
    {
      case CHANNEL_TYPE_DISC:
	(void) fprintf (stderr,
			"Seek operation not finished for disc channel\n");
	return (FALSE);
/*
	break;
*/
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
	if (position > (*channel).mem_buf_len)
	{
	    /*  Illegal seek position  */
	    return (FALSE);
	}
	(*channel).mem_buf_read_pos = position;
	(*channel).mem_buf_write_pos = position;
	(*channel).abs_read_pos = position;
	(*channel).abs_write_pos = position;
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
      default:
	(void) fprintf (stderr, "Invalid channel type\n");
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function ch_seek  */

/*PUBLIC_FUNCTION*/
int ch_get_bytes_readable (channel)
/*  This routine will determine the number of bytes currently readable on
    a connection channel. This is equal to the maximum number of bytes that
    could be read from the channel at this time without blocking waiting for
    more input to be transmitted from the other end of the connection.
    The channel object must be given by  channel  .
    The routine returns the number of bytes readable on success,
    else it returns -1 indicating error.
*/
Channel channel;
{
    int bytes_available;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "ch_get_bytes_readable";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    switch ( (*channel).type )
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
      default:
	(void) fprintf (stderr, "Invalid channel type: %u\n", (*channel).type);
	return (-1);
/*
	break;
*/
    }
    if ( (*channel).errno != 0 )
    {
	errno = (*channel).errno;
	return (-1);
    }
    if ( ( bytes_available = r_get_bytes_readable ( (*channel).fd ) ) < 0 )
    {
	(*channel).errno = errno;
	return (-1);
    }
    return ( (*channel).bytes_read - (*channel).read_buf_pos
	    + (unsigned int) bytes_available );
}   /*  End Function ch_get_bytes_readable  */

/*PUBLIC_FUNCTION*/
int ch_get_descriptor (channel)
/*  This routine will get the file descriptor associated with a channel.
    The channel object must be given by  channel  .
    The routine returns the file descriptor on success,
    else it returns -1 indicating error.
*/
Channel channel;
{
    static char function_name[] = "ch_get_descriptor";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    return ( (*channel).fd );
}   /*  End Function ch_get_descriptor  */

/*PUBLIC_FUNCTION*/
void ch_open_stdin ()
/*  This routine will create a channel object for the standard input descriptor
    0
    The standard input channel will be written to the external variable
    ch_stdin  .
    The routine returns nothing.
*/
{
    flag disc;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
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
    if ( ( (*ch_stdin).fd = r_open_stdin (&disc) ) < 0 )
    {
	(void) fprintf (stderr, "Error getting input descriptor\n");
	exit (RV_UNDEF_ERROR);
    }
    /*  Allocate read buffer  */
    if ( ( (*ch_stdin).read_buffer = m_alloc (CONNECTION_BUF_SIZE) )
	== NULL )
    {
	m_abort (function_name, "read buffer");
    }
    (*ch_stdin).read_buf_len = CONNECTION_BUF_SIZE;
    /*  Tag type  */
    (*ch_stdin).type = (disc) ? CHANNEL_TYPE_DISC : CHANNEL_TYPE_CHARACTER;
}   /*  End Function ch_open_stdin  */

/*PUBLIC_FUNCTION*/
flag ch_test_for_io (channel)
/*  This routine will test if a channel object is capable of supporting reading
    and writing operations. Most channels fall under this category. The notable
    exceptions are the dock channel and channels created by a call to
    ch_attach_to_asynchronous_descriptor  .
    The channel object must be given by  channel  .
    The routine returns TRUE if the channel is capable of reading and writing,
    else it returns FALSE.
*/
Channel channel;
{
    flag return_value;
    static char function_name[] = "ch_test_for_io";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    switch ( (*channel).type )
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
flag ch_test_for_asynchronous (channel)
/*  This routine will test if a channel object is an asynchronous channel,
    ie. a character special file, named FIFO, connection, a dock channel or one
    created by a call to  ch_attach_to_asynchronous_descriptor  .
    The channel object must be given by  channel  .
    The routine returns TRUE if the channel is asynchronous,
    else it returns FALSE.
*/
Channel channel;
{
    flag return_value;
    static char function_name[] = "ch_test_for_asynchronous";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    switch ( (*channel).type )
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
flag ch_test_for_connection (channel)
/*  This routine will test if a channel object is a connection channel.
    The channel object must be given by  channel  .
    The routine returns TRUE if the channel object is a connection,
    else it returns FALSE.
*/
Channel channel;
{
    static char function_name[] = "ch_test_for_connection";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    if ( (*channel).type == CHANNEL_TYPE_CONNECTION )
    {
	return (TRUE);
    }
    else
    {
	return (FALSE);
    }
}   /*  End Function ch_test_for_connection  */

/*PUBLIC_FUNCTION*/
flag ch_test_for_local_connection (channel)
/*  This routine will test if a connection channel object is a local
    connection.
    The channel object must be given by  channel  .
    The routine returns TRUE if the channel object is a local connection,
    else it returns FALSE.
*/
Channel channel;
{
    static char function_name[] = "ch_test_for_local_connection";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    if ( (*channel).type != CHANNEL_TYPE_CONNECTION )
    {
	return (FALSE);
    }
    return ( (*channel).local );
}   /*  End Function ch_test_for_local_connection  */

/*PUBLIC_FUNCTION*/
Channel ch_attach_to_asynchronous_descriptor (fd)
/*  This routine will create a channel object from an asynchronous descriptor.
    The descriptor must be given by  fd  .
    The routine returns a channel object on success, else it returns NULL.
*/
int fd;
{
    Channel channel;
    static char function_name[] = "ch_attach_to_asynchronous_descriptor";

    if ( ( channel = ch_alloc () ) == NULL )
    {
	return (NULL);
    }
    /*  Set channel type  */
    (*channel).type = CHANNEL_TYPE_ASYNCHRONOUS;
    (*channel).fd = fd;
    return (channel);
}   /*  End Function ch_attach_to_asynchronous_descriptor  */

/*PUBLIC_FUNCTION*/
flag ch_test_for_mmap (channel)
/*  This routine will test if a channel object is a memory mapped disc channel.
    The channel object must be given by  channel  .
    The routine returns TRUE if the channel object is memory mapped,
    else it returns FALSE.
*/
Channel channel;
{
    static char function_name[] = "ch_test_for_mmap";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    if ( (*channel).type == CHANNEL_TYPE_MMAP )
    {
	return (TRUE);
    }
    else
    {
	return (FALSE);
    }
}   /*  End Function ch_test_for_mmap  */

/*PUBLIC_FUNCTION*/
flag ch_tell (channel, read_pos, write_pos)
/*  This routine will determine the read and write pointers for a channel.
    The channel object must be given by  channel  .
    The read position (relative to the start of the channel data) will be
    written to the storage pointed to by  read_pos  .
    The write position (relative to the start of the channel data) will be
    written to the storage pointed to by  write_pos  .
    The routine returns TRUE on success, else it returns FALSE.
*/
Channel channel;
unsigned long *read_pos;
unsigned long *write_pos;
{
    static char function_name[] = "ch_tell";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    switch ( (*channel).type )
    {
      case CHANNEL_TYPE_DISC:
      case CHANNEL_TYPE_MEMORY:
      case CHANNEL_TYPE_MMAP:
      case CHANNEL_TYPE_CONNECTION:
      case CHANNEL_TYPE_CHARACTER:
      case CHANNEL_TYPE_FIFO:
	*read_pos = (*channel).abs_read_pos;
	*write_pos = (*channel).abs_write_pos;
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
char *ch_get_mmap_addr (channel)
/*  This routine will get the starting address of the data for a memory mapped
    disc channel. The channel MUST be a memory mapped disc channel.
    The routine returns the address of the memory mapped data.
    NOTE: the if memory mapped address space is read-only, any attempt to write
    to this address space will cause a segmentation fault.
*/
Channel channel;
{
    static char function_name[] = "ch_get_mmap_addr";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    if ( (*channel).type != CHANNEL_TYPE_MMAP )
    {
	(void) fprintf (stderr, "Channel is not a memory mapped disc file\n");
	a_prog_bug (function_name);
    }
    ++(*channel).mmap_access_count;
    return ( (*channel).memory_buffer );
}   /*  End Function ch_get_mmap_addr  */

/*PUBLIC_FUNCTION*/
unsigned int ch_get_mmap_access_count (channel)
/*  This routine will get the number of times a memory mapped disc channel has
    been queried for the mapping address (using  ch_get_mmap_addr  ).
    The channel MUST be a memory mapped disc channel.
    The routine returns the number of address queries.
*/
Channel channel;
{
    static char function_name[] = "ch_get_mmap_access_count";

    if (channel == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if channel is valid  */
    if ( (*channel).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid channel object\n");
	a_prog_bug (function_name);
    }
    if ( (*channel).type != CHANNEL_TYPE_MMAP )
    {
	(void) fprintf (stderr, "Channel is not a memory mapped disc file\n");
	a_prog_bug (function_name);
    }
    return ( (*channel).mmap_access_count );
}   /*  End Function ch_get_mmap_access_count  */

/*LINTLIBRARY*/
/*PREFIX:"r_"*/
/*  connections.c
    This code implements low level communications.

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
/*

    This file contains the various utility routines for manipulating Karma
    connections.


    Written by      Richard Gooch   14-AUG-1992

    Updated by      Richard Gooch   29-OCT-1992

    Updated by      Richard Gooch   16-DEC-1992: Added  r_getenv  and  r_setenv

    Updated by      Richard Gooch   25-DEC-1992: Added  r_gethostname  .

    Updated by      Richard Gooch   30-DEC-1992: Added  r_getppid  .

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   24-JAN-1993: Declared  connect_to_port  at
  top of file (inside #ifdef).

    Updated by      Richard Gooch   12-APR-1993: Improved documentation for
  r_read  and added blocking of  SIGPIPE  to  r_write  .

    Updated by      Richard Gooch   20-JUL-1993: Added support for disc channel
  for Standard Input under VX/ MVX.

    Updated by      Richard Gooch   24-JUL-1993: Added support for opening
  character special devices and FIFOs by creating  r_open_file  .

    Updated by      Richard Gooch   29-JUL-1993: Added Linux support in
  r_setenv  .

    Updated by      Richard Gooch   20-NOV-1993: Made  alloc_port  tolerant of
  Linux kernels without TCP/IP support.

    Last updated by Richard Gooch   23-NOV-1993: Fixed bug introduced on 20-NOV
  which failed to call  htons(3)  prior to binding in  bind_inet  .


*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <os.h>
#ifdef HAS_SOCKETS
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/param.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#endif
#ifdef CAN_FORK
#include <signal.h>
#endif
#ifdef ARCH_VXMVX
#include <vx/vx.h>
#endif
#include <karma.h>
#include <karma_r.h>

#define NUM_DOCKS (unsigned int) 2

#if defined(HAS_SOCKETS) || defined(HAS_COMMUNICATIONS_EMULATION)
#define COMMUNICATIONS_AVAILABLE
#endif

#ifdef HAS_SOCKETS
#define TCP_PORT_OFFSET 6200
#define UNIX_SOCKET_DIR "/tmp/.KARMA_connections"
#define UNIX_SOCKET_FILE "KARMA"
#define INDEX_UNIX_DOCK 0
#define INDEX_INTERNET_DOCK 1
#define READ read
#define WRITE write
#define GETHOSTNAME gethostname
#define GETPPID getppid
#endif

#ifdef HAS_COMMUNICATIONS_EMULATION
#define READ read_from_connection
#define WRITE write_to_connection
#define GETHOSTNAME get_hostname
#define GETPPID get_ppid
#endif


#ifdef ARCH_VXMVX
#define MAX_DESCRIPTORS 100
#define DESCRIPTOR_OFFSET 11000
#define HR_ALLOC_PORT 0
#define HR_CONNECT_TO_PORT 1
#define HR_GET_INET_ADDR 2
#define HR_OPEN_STDIN 3
#define HR_GET_HOSTNAME 4
#endif

/*  Structure definitions  */
#ifdef ARCH_VXMVX
struct descriptor_type
{
    flag open;
    flag received_closure;
    int incoming_message_queue;
    int outgoing_message_queue;
    char *incoming_msg;
    int msg_pos;
    int msg_len;
};
#endif


/*  Private functions  */
static void prog_bug ();
#ifdef COMMUNICATIONS_AVAILABLE
static int *alloc_port ();
#  ifdef HAS_SOCKETS
static flag bind_unix (/* sock, port_number */);
static flag bind_inet (/* sock, port_number */);
#  endif
static void close_dock ();
static int connect_to_port ();
static int accept_connection_on_dock ();
static flag close_connection ();
static int get_bytes_readable ();
static unsigned long get_inet_addr_from_host ();
static unsigned long conv_hostname_to_addr ();
static int open_stdin ();
static int read_from_connection ();
static int write_to_connection ();
static int get_hostname ();
#endif
#ifdef HAS_COMMUNICATIONS_EMULATION
static void init_control_connection ();
static int find_spare_descriptor ();
#endif


/*  Private data  */
static int tcp_port_offset = -1;
static int docks[NUM_DOCKS];
static unsigned int num_docks_open = 0;
static unsigned int allocated_port_number;
#ifdef ARCH_VXMVX
static int parent_pid = -1;
static int incoming_control_message_queue = -1;
static int outgoing_control_message_queue = -1;
static struct descriptor_type descriptors[MAX_DESCRIPTORS];
#endif

#ifndef HAS_ENVIRON
char **environ = NULL;
#endif

/*  Public routines follow  */

/*PUBLIC_FUNCTION*/
int *r_alloc_port (port_number, retries, num_docks)
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
    The docks are placed into non-blocking mode.
    The routine returns a pointer to a statically allocated array of docks on
    success, else it returns NULL.
*/
unsigned int *port_number;
unsigned int retries;
unsigned int *num_docks;
{
    extern unsigned int num_docks_open;
    static char function_name[] = "r_alloc_port";

    if (num_docks_open > 0)
    {
	(void) fprintf (stderr, "Port has already been allocated\n");
	prog_bug (function_name);
    }
    /*  Check alignment of  port_number  */
    if ( (int) port_number % sizeof (int) != 0 )
    {
	(void) fputs ("Pointer to port number storage does not lie on an",
		      stderr);
	(void) fputs ("  int  boundary\n", stderr);
	prog_bug (function_name);
    }
    /*  Check alignment of  num_docks  */
    if ( (int) num_docks % sizeof (int) != 0 )
    {
	(void) fputs ("Pointer to number of docks storage does not lie on",
		      stderr);
	(void) fputs (" an  int  boundary\n", stderr);
	prog_bug (function_name);
    }
#ifdef COMMUNICATIONS_AVAILABLE
    return ( alloc_port (port_number, retries, num_docks) );

#else
    (void) fprintf (stderr, "No communications support\n");
    return (NULL);
#endif
}   /*  End Function r_alloc_port  */

/*PUBLIC_FUNCTION*/
void r_close_dock (dock)
/*  This routine will close a dock. If the dock was the last open dock for the
    port, then the entire port is closed and a new port may be allocated.
    The dock to close must be given by  dock  .
    The routine returns nothing.
*/
int dock;
{
    unsigned int dock_count;
    extern int docks[NUM_DOCKS];
    extern unsigned int num_docks_open;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "r_close_dock";

    if (num_docks_open < 1)
    {
	(void) fprintf (stderr, "No docks are open\n");
	prog_bug (function_name);
    }
#ifdef COMMUNICATIONS_AVAILABLE
    for (dock_count = 0; dock_count < NUM_DOCKS; ++dock_count)
    {
	if (dock == docks[dock_count])
	{
	    /*  Found the dock  */
	    close_dock (dock_count);
	    docks[dock_count] = -1;
	    --num_docks_open;
	    return;
	}
    }
    (void) fprintf (stderr, "Dock: %d does not exist\n", dock);
    prog_bug (function_name);

#else
    (void) fprintf (stderr, "No communications support\n");
    return;
#endif
}  /*  End Function r_close_dock  */

/*PUBLIC_FUNCTION*/
int r_connect_to_port (addr, port_number, local)
/*  This routine will connect to a server module running on the machine
    with Internet address given by  addr  .
    If the value of 0 is supplied for the address, the connection is made to a
    Karma server running on the local machine.
    The port number to connect to must given by  port_number  .
    If the connection is made to a port on the local host, then the value TRUE
    will be written to the storage pointed to by  local  ,else the value FALSE
    will be written here.
    The close-on-exec flags of the socket is set such that the socket will
    close on a call to execve(2V).
    The routine returns the file descriptor of the opened connection on
    success, else it returns -1
*/
unsigned long addr;
unsigned int port_number;
flag *local;
{
#ifdef COMMUNICATIONS_AVAILABLE
    flag local_flag;
    int fd;
    extern unsigned int allocated_port_number;
    extern unsigned int num_docks_open;
    static char function_name[] = "r_connect_to_port";

    /*  Check if self-connect  */
    if (addr == 0)
    {
	/*  Unix domain socket/ connection  */
	if ( (port_number == allocated_port_number) &&
	    (num_docks_open > 0) )
	{
	    (void) fprintf (stderr, "Attempt to connect to oneself!\n");
	    prog_bug (function_name);
	}
	local_flag = TRUE;
    }
    else
    {
	/*  Internet/ TCP domain socket/ connection  */
	if (r_get_inet_addr_from_host ( (char *) NULL, (flag *) NULL ) == addr)
	{
	    /*  Connecting locally  */
	    local_flag = TRUE;
	    if ( (port_number == allocated_port_number) &&
		(num_docks_open > 0) )
	    {
		(void) fprintf (stderr, "Attempt to connect to oneself!\n");
		prog_bug (function_name);
	    }
	}
	else
	{
	    /*  Not connecting locally  */
	    local_flag = FALSE;
	}
    }
    if ( ( fd = connect_to_port (addr, port_number) ) > -1 )
    {
	*local = local_flag;
    }
    return (fd);

#else
    (void) fprintf (stderr, "No communications support\n");
    return (-1);
#endif
}   /*  End Function r_connect_to_port  */

/*PUBLIC_FUNCTION*/
int r_accept_connection_on_dock (dock, addr, local)
/*  This routine will accept a connection on a dock.
    The dock must be given by  dock  .
    The address of the host connecting to the dock will be written to the
    storage pointed to by  addr  .
    If the connection is a local connection, then the routine will write the
    value TRUE to the storage pointed to by  local  ,else it will write
    the value FALSE.
    The routine returns a connection on success, else it returns -1
*/
int dock;
unsigned long *addr;
flag *local;
{
#ifdef COMMUNICATIONS_AVAILABLE
    unsigned int dock_count;
    extern unsigned int num_docks_open;
    static char function_name[] = "r_accept_connection_on_dock";

    if (num_docks_open < 1)
    {
	(void) fprintf (stderr, "No docks are open\n");
	prog_bug (function_name);
    }
    dock_count = 0;
    while ( (dock != docks[dock_count]) && (dock_count < NUM_DOCKS) )
    {
	++dock_count;
    }
    if (dock_count >= NUM_DOCKS)
    {
	(void) fprintf (stderr, "Dock: %d does not exist\n", dock);
	prog_bug (function_name);
    }
    return ( accept_connection_on_dock (dock_count, addr, local) );

#else
    (void) fprintf (stderr, "No communications support\n");
    return (-1);
#endif
}  /*  End Function r_accept_connection_on_dock  */

/*PUBLIC_FUNCTION*/
flag r_close_connection (connection)
/*  This routine will close a connection.
    The connection to close must be given by  connection  .
    The routine returns TRUE on success, else it returns FALSE.
*/
int connection;
{
#ifdef COMMUNICATIONS_AVAILABLE
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "r_close_connection";

    return ( close_connection (connection) );

#else
    (void) fprintf (stderr, "No communications support\n");
    return (FALSE);
#endif
}  /*  End Function r_close_connection  */

/*PUBLIC_FUNCTION*/
int r_get_bytes_readable (connection)
/*  This routine will determine the minimum number of bytes readable on a
    connection. There may be more bytes readable than indicated.
    The connection should be given by  connection  .
    The routine returns the number of bytes readable on success,
    else it returns -1
*/
int connection;
{
#ifdef COMMUNICATIONS_AVAILABLE
    return ( get_bytes_readable (connection) );

#else
    (void) fprintf (stderr, "No communications support\n");
    return (-1);
#endif
}   /*  End Function r_get_bytes_readable  */

/*PUBLIC_FUNCTION*/
unsigned long r_get_inet_addr_from_host (host, local)
/*  This routine will get the first listed Internet address from the hostname
    string pointed to by  host  .
    If the specified host is the local machine, then the routine will write
    the value TRUE to the storage pointed to by  local  ,else it will write
    the value FALSE here. If this is NULL, nothing is written here.
    The routine returns the Internet address on success (in host byte order),
    else it returns 0.
*/
char *host;
flag *local;
{
#ifdef COMMUNICATIONS_AVAILABLE
    return ( get_inet_addr_from_host (host, local) );

#else
    (void) fprintf (stderr, "No communications support\n");
    return (0);
#endif
}   /*  End Function r_get_inet_addr_from_host  */

/*PUBLIC_FUNCTION*/
int r_read (fd, buf, nbytes)
/*  This routine is similar to the system  read(2)  call, except that the
    number of bytes requested is always returned (except on error or closure).
    Hence, if the descriptor references a socket, the routine will read as much
    data as was requested, rather than a lesser amount due to packetisation or
    interrupted system calls.
    The file descriptor to read from must be given by  fd  .This descriptor
    must NOT be set to non-blocking IO.
    The buffer in which to write the data must be pointed to by  buf  .
    The number of bytes to read must be given by  nbytes  .
    The routine returns the number of bytes requested on success,
    the number of bytes read on end of file (or closure) and -1 on error.
*/
int fd;
char *buf;
int nbytes;
{
#ifdef COMMUNICATIONS_AVAILABLE
    int total_bytes_read = 0;
    int bytes_read;
    ERRNO_TYPE errno;

    while (total_bytes_read < nbytes)
    {
	errno = 0;
	if ( ( bytes_read = READ (fd, buf + total_bytes_read,
				  nbytes - total_bytes_read) )
	    < 0 )
	{
	    /*  Error  */
#  ifdef EINTR
	    if (errno != EINTR)
	    {
		/*  Unrecoverable error  */
		return (-1);
	    }
#  else
	    return (-1);
#  endif
	}
	else
	{
	    /*  No error condition  */
	    if (bytes_read == 0)
	    {
		/*  End-of-file  */
		return (total_bytes_read);
	    }
	    total_bytes_read += bytes_read;
	}
    }
    return (total_bytes_read);

#else
    (void) fprintf (stderr, "No communications support\n");
    return (-1);
#endif
}   /*  End Function r_read  */

/*PUBLIC_FUNCTION*/
int r_write (fd, buf, nbytes)
/*  This routine is similar to the system  write(2)  call, except that the
    number of bytes requested is always returned (except on error). Hence, if
    the descriptor references a socket, the routine will write as much data as
    was requested, rather than a lesser amount due to packetisation or
    interrupted system calls.
    The file descriptor to write from must be given by  fd  .This descriptor
    must NOT be set to non-blocking IO.
    The buffer in which to write the data must be pointed to by  buf  .
    The number of bytes to write must be given by  nbytes  .
    NOTE: the routine will force  SIGPIPE  to be ignored.
    The routine returns the number of bytes requested on success,
    else it returns -1 indicating error.
*/
int fd;
char *buf;
int nbytes;
{
#ifdef COMMUNICATIONS_AVAILABLE
    int total_bytes_written = 0;
    int bytes_written;
    ERRNO_TYPE errno;
#  ifdef SIGPIPE
    static flag must_ignore_sigpipe = TRUE;
#  endif
    static char function_name[] = "r_write";

#  ifdef SIGPIPE
    if (must_ignore_sigpipe)
    {
	(void) signal (SIGPIPE, SIG_IGN);
	must_ignore_sigpipe = FALSE;
    }
#  endif
    while (total_bytes_written < nbytes)
    {
	errno = 0;
	if ( ( bytes_written = WRITE (fd, buf + total_bytes_written,
				      nbytes - total_bytes_written) )
	    < 0 )
	{
	    /*  Error  */
#  ifdef EINTR
	    if (errno != EINTR)
	    {
		/*  Unrecoverable error  */
		return (-1);
	    }
#  else
	    return (-1);
#  endif
	}
	else
	{
	    /*  No error condition  */
	    total_bytes_written += bytes_written;
	}
    }
    return (total_bytes_written);

#else
    (void) fprintf (stderr, "No communications support\n");
    return (-1);
#endif
}   /*  End Function r_write  */

/*PUBLIC_FUNCTION*/
flag r_test_input_event (connection)
/*  This routine will test if there is input activity on a connection. This
    activity also covers the case of connection closure.
    The connection descriptor must be given by  connection  .
    NOTE: this routine is only available on platforms which emulate the
    communications facilities of Unix. It is NOT available on standard Unix
    systems.
    The routine returns TRUE if there is some input activity,
    else it returns FALSE.
*/
int connection;
{
    static char function_name[] = "r_test_input_event";
#ifdef ARCH_VXMVX
    void *msg;
    extern struct descriptor_type descriptors[MAX_DESCRIPTORS];

    connection -= DESCRIPTOR_OFFSET;
    if ( (connection < 0) || (connection >= MAX_DESCRIPTORS) )
    {
	(void) fprintf (stderr, "Bad descriptor: %d\n",
			connection + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if (descriptors[connection].open != TRUE)
    {
	(void) fprintf (stderr, "Descriptor: %d not open\n",
			connection + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if (descriptors[connection].incoming_msg != NULL)
    {
	return (TRUE);
    }
    if ( ( msg = task_recv_msg (descriptors[connection].incoming_message_queue,
				0, 0) ) == NULL )
    {
	return (FALSE);
    }
    descriptors[connection].incoming_msg = msg;
    descriptors[connection].msg_len = task_sizeof_msg (msg);
    descriptors[connection].msg_pos = 0;
    if (descriptors[connection].msg_len == 0)
    {
	descriptors[connection].received_closure = TRUE;
    }
    return (TRUE);

#else  /*  ARCH_VXMVX  */
    (void) fprintf (stderr, "No communications emulation\n");
    prog_bug (function_name);
    return (FALSE);
#endif  /*  ARCH_VXMVX  */
}   /*  End Function r_test_input_event  */

/*PUBLIC_FUNCTION*/
int r_open_stdin (disc)
/*  This routine will open the standard input.
    The routine will write the value TRUE to the storage pointed to by  disc
    if the standard input is a disc, else it will write FALSE.
    The routine returns the descriptor on success, else it returns -1
*/
flag *disc;
{
#ifdef COMMUNICATIONS_AVAILABLE
    return ( open_stdin (disc) );

#else
    (void) fprintf (stderr, "No communications support\n");
    return (-1);
#endif
}   /*  End Function r_open_stdin  */

/*PUBLIC_FUNCTION*/
char *r_getenv (name)
/*  This routine will get the value of the environment variable with name
    pointed to by  name  .
    The routine returns a pointer to the value string if present,
    else it returns NULL.
*/
char *name;
{
#ifdef HAS_ENVIRON
    return ( getenv (name) );

#else
    int length;
    char **env;
    char cmp[STRING_LENGTH];
    extern char **environ;

    init_control_connection ();
    if (environ == NULL)
    {
	return (NULL);
    }
    (void) strcpy (cmp, name);
    length = strlen (cmp);
    if (cmp[length] != '=')
    {
	cmp[length] = '=';
	cmp[length + 1] = '\0';
	++length;
    }
    for (env = environ; *env != NULL; ++env)
    {
	if (strncmp (cmp, *env, length) == 0)
	{
	    return (*env + length);
	}
    }
    return (NULL);
#endif
}   /*  End Function r_getenv  */

/*PUBLIC_FUNCTION*/
int r_setenv (env_name, env_value)
/*  This routine will interface to the "standard" C library routines:
    putenv  or  setenv  (depending on the particular standard C library
    supplied with the operating system).
    The environment variable to create or change must be named by  env_name  .
    The string value to set the variable to must be pointed to by  env_value  .
    The routine returns 0 on success, else it returns -1.
*/
char *env_name;
char *env_value;
{
    static char function_name[] = "r_setenv";

#if defined(ARCH_SUNsparc) || defined(ARCH_rs6000) || defined(ARCH_MS_DOS_386) || defined(ARCH_linux)
    char *str;
    char env_string[STRING_LENGTH];

    (void) sprintf (env_string, "%s=%s", env_name, env_value);
    if ( ( str = strdup (env_string) ) == NULL )
    {
	(void) fprintf (stderr, "%s: Error allocating string\n",
			function_name);
	exit (RV_MEM_ERROR);
    }
    return ( putenv (str) );
#endif

#ifdef ARCH_convex
    return ( setenv (env_name, env_value, 1) );
#endif

#ifdef ARCH_dec
    return ( setenv (env_name, env_value, 1) );
#endif

#ifndef HAS_ENVIRON
    char *str;
    char env_string[STRING_LENGTH];
    extern char **environ;

    (void) sprintf (env_string, "%s=%s", env_name, env_value);
    if ( ( str = strdup (env_string) ) == NULL )
    {
	(void) fprintf (stderr, "%s: Error allocating string\n",
			function_name);
	exit (RV_MEM_ERROR);
    }
    exit (RV_UNDEF_ERROR);
#endif
}   /*  End Function r_setenv  */

/*PUBLIC_FUNCTION*/
void r_gethostname (name, namelen)
/*  This routine will determine the local hostname.
    The buffer to write the hostname to must be pointed to by  name  .
    The length of the buffer must be given by  namelen  .
    The buffer is guaranteed to be null terminated.
    The routine returns nothing.
*/
char *name;
unsigned int namelen;
{
#ifdef COMMUNICATIONS_AVAILABLE
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    if (GETHOSTNAME (name, namelen - 1) != 0)
    {
	(void) fprintf (stderr, "Error getting hostname\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    name[namelen - 1] = '\0';

#else
    (void) fprintf (stderr, "No communications support\n");
    exit (RV_UNDEF_ERROR);
#endif
}   /*  End Function r_gethostname  */

/*PUBLIC_FUNCTION*/
int r_getppid ()
/*  This routine will determine the parent process ID.
    The routine returns the parent process ID.
*/
{
#ifdef COMMUNICATIONS_AVAILABLE
    int ppid;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    if ( ( ppid = GETPPID () ) < 0 )
    {
	(void) fprintf (stderr, "Error getting parent process ID\t%s\n",
			sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    return (ppid);

#else
    (void) fprintf (stderr, "No communications support\n");
    exit (RV_UNDEF_ERROR);
#endif
}   /*  End Function r_getppid  */

int r_open_file (filename, flags, mode, filetype, blocksize)
/*  This routine will open a file. The file may be a regular disc file,
    a named FIFO or a character special device. This routine provides an
    enhanced interface to the  open(2)  routine.
    The pathname of the file to open must be given by  filename  .This
    parameter has the same meaning as the first parameter to  open(2).
    The access flags must be given by  flags  .This parameter has the same
    meaning as the second parameter to  open(2).
    If the file is created, the file protection modes will be determined by the
    combination of the process  umask  and the value of  mode  .This parameter
    has the same meaning as the third parameter to  open(2).
    The type of the file will be written to the storage pointed to by  filetype
    The blocksize (in bytes) of the file will be written to the storage pointed
    to by  blocksize  (this is only valid for a disc file). If the blocksize
    cannot be determined, the value 0 will be written here.
    The routine returns the file descriptor on success, else it returns -1 and
    sets  errno  with the error code.
*/
char *filename;
int flags;
int mode;
unsigned int *filetype;
unsigned int *blocksize;
{
    int fd;
    struct stat statbuf;

    /*  Open file descriptor  */
    if ( ( fd = open (filename, flags, mode) ) < 0 )
    {
	return (-1);
    }
    /*  Get stats on file  */
    if (fstat (fd, &statbuf ) != 0)
    {
	(void) fprintf (stderr, "Error getting file stats\n");
	return (-1);
    }
    if ( S_ISREG (statbuf.st_mode) )
    {
	*filetype = KFTYPE_DISC;
#ifdef ARCH_SGImips
	*blocksize = 0;
#else
	*blocksize = statbuf.st_blksize;
#endif
    }
    else if ( S_ISCHR (statbuf.st_mode) )
    {
	*filetype = KFTYPE_CHARACTER;
	*blocksize = 0;
    }
    else if ( S_ISFIFO (statbuf.st_mode) )
    {
	*filetype = KFTYPE_FIFO;
	*blocksize = 0;
    }
    else
    {
	(void) fprintf (stderr, "Illegal file mode: %d\n", statbuf.st_mode);
	return (-1);
    }
    return (fd);
}   /*  End Function r_open_file  */


/*  Private functions follow  */

#ifdef HAS_SOCKETS
static unsigned long get_inet_addr_from_host (host, local)
/*  This routine will get the first listed Internet address from the hostname
    string pointed to by  host  .
    IF the specified host is the local machine, then the routine will write
    the value TRUE to the storage pointed to by  local  ,else it will write
    the value FALSE here. If this is NULL, nothing is written here.
    The routine returns the Internet address on success (in host byte order),
    else it returns 0.
*/
char *host;
flag *local;
{
    unsigned long addr;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static unsigned long local_addr = 0;
    static char local_hostname[MAXHOSTNAMELEN + 1];

    if (local_addr < 1)
    {
	/*  Get my hostname  */
	if (gethostname (local_hostname, MAXHOSTNAMELEN + 1) != 0)
	{
	    (void) fprintf (stderr, "Error getting local hostname\t%s",
			    sys_errlist[errno]);
	    return (0);
	}
	/*  Get my host address  */
	if ( ( local_addr = conv_hostname_to_addr (local_hostname) ) == 0 )
	{
	    (void) fprintf (stderr, "Error getting local host address\n");
	    return (0);
	}
    }
    if ( (host == NULL) || (strcmp (host, "unix") == 0) ||
	(strcmp (host, "localhost") == 0) )
    {
	/*  Local machine  */
	if (local != NULL)
	{
	    *local = TRUE;
	}
	return (local_addr);
    }
    if ( ( addr = conv_hostname_to_addr (host) ) == 0 )
    {
	(void) fprintf (stderr, "Error getting host address\n");
	return (0);
    }
    if (addr == local_addr)
    {
	if (local != NULL)
	{
	    *local = TRUE;
	}
    }
    else
    {
	if (local != NULL)
	{
	    *local = FALSE;
	}
    }
    return (addr);
}   /*  End Function get_inet_addr_from_host  */

static unsigned long conv_hostname_to_addr (host)
/*  This routine will get the first listed Internet address from the hostname
    string pointed to by  host  .This MUST be a valid hostname.
    The routine returns the Internet address on success (in host byte order),
    else it returns 0.
*/
char *host;
{
    struct hostent *host_ptr;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    if ( isascii (host[0]) && isdigit (host[0]) )
    {
	/*  Numeric Internet address  */
	return ( ntohl ( inet_addr (host) ) );
    }
    /*  Get host info  */
    if ( ( host_ptr = gethostbyname (host) ) == NULL )
    {
	(void) fprintf (stderr, "Host: \"%s\" not in database\n",
			host);
	return (0);
    }
    return ( ntohl ( *(unsigned long *) (*host_ptr).h_addr_list[0]) );
}   /*  End Function conv_hostname_to_addr  */

static int *alloc_port (port_number, retries, num_docks)
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
    The docks are placed into non-blocking mode.
    The routine returns a pointer to a statically allocated array of docks on
    success, else it returns NULL.
*/
unsigned int *port_number;
unsigned int retries;
unsigned int *num_docks;
{
    flag bound;
    unsigned int retry_number;
    struct servent *service_entry;
    extern int tcp_port_offset;
    extern int docks[NUM_DOCKS];
    extern unsigned int num_docks_open;
    extern unsigned int allocated_port_number;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "alloc_port";

    /*  Get Karma port number for tcp/ ip  */
    if (tcp_port_offset < 0)
    {
	/*  Get tcp port offset  */
	if ( ( service_entry = getservbyname ("KARMA", "tcp") ) == NULL )
	{
	    tcp_port_offset = TCP_PORT_OFFSET;
	}
	else
	{
	    tcp_port_offset = ntohs ( (*service_entry).s_port );
	    if (TCP_PORT_OFFSET != tcp_port_offset)
	    {
		(void) fprintf (stderr,
				"WARNING: NIS sevices database lists Karma");
		(void) fprintf (stderr, " as having port number: %d\n",
				tcp_port_offset);
		(void) fprintf (stderr,
				"whereas the hardcoded default is: %d\n",
				TCP_PORT_OFFSET);
	    }
	}
    }
    /*  Create Unix domain socket  */
    if ( ( docks[INDEX_UNIX_DOCK] = socket (AF_UNIX, SOCK_STREAM, 0) ) < 0 )
    {
	(void) fprintf (stderr, "Error creating Unix socket\t%s\n",
			sys_errlist[errno]);
	(void) exit (RV_SYS_ERROR);
    }
    /*  Create Internet domain socket  */
    if ( ( docks[INDEX_INTERNET_DOCK] = socket (AF_INET, SOCK_STREAM, 0) )
	< 0 )
    {
#ifdef ARCH_linux
	if (errno == EINVAL)
	{
	    (void) fprintf (stderr, "No kernel support for TCP/IP\t%s\n",
			    sys_errlist[errno]);
	    (void) fprintf (stderr, "Internet dock not created\n");
	    docks[INDEX_INTERNET_DOCK] = -1;
	}
	else
#endif
	{
	    (void) fprintf (stderr, "Error creating Internet socket\t%s\n",
			    sys_errlist[errno]);
	    if (close (docks[INDEX_UNIX_DOCK]) != 0)
	    {
		(void) fprintf (stderr, "Error closing Unix socket\t%s\n",
				sys_errlist[errno]);
	    }
	    (void) exit (RV_SYS_ERROR);
	}    
    }
    /*  Try to bind to port  */
    for (retry_number = 0, bound = FALSE, allocated_port_number = *port_number;
	 (retry_number <= retries) && !bound;
	 ++retry_number)
    {
	/*  Try to bind to another port number  */
	if (docks[INDEX_INTERNET_DOCK] < 0)
	{
	    /*  No Internet dock: try Unix dock  */
	    bound = bind_unix (docks[INDEX_UNIX_DOCK], allocated_port_number);
	}
	else
	{
	    /*  Have Internet dock: try to bind  */
	    bound = bind_inet (docks[INDEX_INTERNET_DOCK],
			       allocated_port_number + tcp_port_offset);
	}
	if (!bound) ++allocated_port_number;
    }
    if (!bound)
    {
	/*  All slots occupied  */
	if (close (docks[INDEX_UNIX_DOCK]) != 0)
	{
	    (void) fprintf (stderr, "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	    (void) exit (RV_SYS_ERROR);
	}
	if (close (docks[INDEX_INTERNET_DOCK]) != 0)
	{
	    (void) fprintf (stderr, "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	    (void) exit (RV_SYS_ERROR);
	}
	return (NULL);
    }
    if (docks[INDEX_INTERNET_DOCK] > -1)
    {
	/*  Have bound Internet dock: now bind Unix dock  */
	if (bind_unix (docks[INDEX_UNIX_DOCK], allocated_port_number) != TRUE)
	{
	    (void) fprintf (stderr,
			    "TCP dock bound but could not bind Unix dock\n");
	    exit (RV_SYS_ERROR);
	}
    }
    *port_number = allocated_port_number;
    if (docks[INDEX_INTERNET_DOCK] < 0)
    {
	num_docks_open = NUM_DOCKS - 1;
    }
    else
    {
	num_docks_open = NUM_DOCKS;
    }
    *num_docks = num_docks_open;
    return (docks);
}   /*  End Function alloc_port  */

static flag bind_unix (sock, port_number)
/*  This routine will bind a unix socket to a particular Karma port number.
    The socket must be given by  sock  .
    The port number must be given by  port_number  .
    The routine returns TRUE on success, else it returns FALSE (indicating the
    address is in use).
*/
int sock;
unsigned int port_number;
{
    struct sockaddr_un un_addr;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    un_addr.sun_family = AF_UNIX;
    (void) sprintf (un_addr.sun_path, "%s/%s%d", UNIX_SOCKET_DIR,
		    UNIX_SOCKET_FILE, port_number);
    (void) mkdir (UNIX_SOCKET_DIR, (mode_t) 0777);
    (void) chmod (UNIX_SOCKET_DIR, (mode_t) 0777);
    (void) unlink (un_addr.sun_path);
    /*  Try to bind to Unix port  */
    if (bind (sock, (struct sockaddr *) &un_addr, (int) sizeof un_addr) != 0)
    {
	/*  Could not bind to port number  */
	if (errno != EADDRINUSE)
	{
	    (void) fprintf (stderr, "Error binding Unix socket\t%s\n",
			    sys_errlist[errno]);
	    if (close (sock) != 0)
	    {
		(void) fprintf (stderr, "Error closing Unix socket\t%s\n",
				sys_errlist[errno]);
	    }
	    (void) exit (RV_SYS_ERROR);
	}
	/*  Port already in use: say so  */
	return (FALSE);
    }
    /*  Bound  */
    /*  Set close-on-exec flag  */
    if (fcntl (sock, F_SETFD, 1) == -1)
    {
	(void) fprintf (stderr,
			"Error setting close-on-exec flag for Unix socket\t%s\n",
			sys_errlist[errno]);
	if (close (sock) != 0)
	{
	    (void) fprintf (stderr, "Error closing Unix socket\t%s\n",
			    sys_errlist[errno]);
	}
	(void) exit (RV_SYS_ERROR);
    }
    if (listen (sock, 2) != 0)
    {
	(void) fprintf (stderr, "Error listening to Unix dock\t%s\n",
			sys_errlist[errno]);
	if (close (sock) != 0)
	{
	    (void) fprintf (stderr, "Error closing Unix socket\t%s\n",
			    sys_errlist[errno]);
	}
	(void) exit (RV_SYS_ERROR);
    }
    return (TRUE);
}   /*  End Function bind_unix  */

static flag bind_inet (sock, port_number)
/*  This routine will bind an Internet socket to a particular Karma port number
    The socket must be given by  sock  .
    The port number must be given by  port_number  .
    The routine returns TRUE on success, else it returns FALSE (indicating the
    address is in use).
*/
int sock;
unsigned int port_number;
{
    int sock_opt = SO_REUSEADDR;
    struct sockaddr_in in_addr;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = INADDR_ANY;
    /*  Try to bind to port number  */
    in_addr.sin_port = htons (port_number);
    if (bind (sock, (struct sockaddr *) &in_addr, (int) sizeof in_addr) != 0)
    {
	/*  Could not bind to port number  */
	if (errno != EADDRINUSE)
	{
	    (void) fprintf (stderr, "Error binding Internet socket\t%s\n",
			    sys_errlist[errno]);
	    if (close (sock) != 0)
	    {
		(void) fprintf (stderr, "Error closing Internet socket\t%s\n",
				sys_errlist[errno]);
	    }
	    (void) exit (RV_SYS_ERROR);
	}
	/*  Port already in use: say so  */
	return (FALSE);
    }
    /*  Bound  */
    /*  Set socket options  */
#  ifdef TCP_NODELAY
    sock_opt |= TCP_NODELAY;
    (void) fprintf (stderr, "TCP_NODELAY\n");
#  endif
    /*  The following code exercises a bug in the Convex OS kernel.
	This bug causes the system to crash. DO NOT USE until kernel patched.
    if (setsockopt (sock, SOL_SOCKET, sock_opt, (caddr_t) 0, 0)
	!= 0)
    {
        (void) fprintf (stderr,
	                "Error setting Internet socket options\t%s\n",
			sys_errlist[errno]);
	(void) exit (RV_SYS_ERROR);
    }
*/
    /*  Set close-on-exec flag  */
    if (fcntl (sock, F_SETFD, 1) == -1)
    {
	(void) fprintf (stderr,
			"Error setting close-on-exec flag for Internet socket\t%s\n",
			sys_errlist[errno]);
	if (close (sock) != 0)
	{
	    (void) fprintf (stderr,
			    "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	}
	(void) exit (RV_SYS_ERROR);
    }
    if (listen (sock, 2) != 0)
    {
	(void) fprintf (stderr, "Error listening to Internet dock\t%s\n",
			sys_errlist[errno]);
	if (close (sock) != 0)
	{
	    (void) fprintf (stderr, "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	}
	(void) exit (RV_SYS_ERROR);
    }
    return (TRUE);
}   /*  End Function bind_inet  */

static void close_dock (dock_index)
/*  This routine will close a dock.
    The dock index to close must be given by  dock_index  .
    The routine returns nothing.
*/
int dock_index;
{
    struct sockaddr_un un_addr;
    extern unsigned int allocated_port_number;
    extern int docks[NUM_DOCKS];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    if (close (docks[dock_index]) != 0)
    {
	(void) fprintf (stderr,
			"Error closing dock: %d\t%s\n",
			docks[dock_index], sys_errlist[errno]);
	exit (RV_SYS_ERROR);
    }
    if (dock_index == INDEX_UNIX_DOCK)
    {
	(void) sprintf (un_addr.sun_path, "%s/%s%d", UNIX_SOCKET_DIR,
			UNIX_SOCKET_FILE, allocated_port_number);
	(void) unlink (un_addr.sun_path);
    }
}  /*  End Function close_dock  */

static int connect_to_port (addr, port_number)
/*  This routine will connect to a server module running on the machine
    with Internet address given by  addr  .
    If the value of 0 is supplied for the address, the connection is made to a
    Karma server running on the local machine.
    The port number to connect to must given by  port_number  .
    The close-on-exec flags of the socket is set such that the socket will
    close on a call to execve(2V).
    The routine returns the file descriptor of the opened connection on
    success, else it returns -1
*/
unsigned long addr;
unsigned int port_number;
{
    int sock;
    struct sockaddr_un un_addr;
    struct sockaddr_in in_addr;
    struct servent *service_entry;
    extern int tcp_port_offset;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "connect_to_port";

    /*  Get port number for tcp/ ip  */
    if (tcp_port_offset < 0)
    {
	/*  Get tcp port offset  */
	if ( ( service_entry = getservbyname ("KARMA", "tcp") ) == NULL )
	{
	    tcp_port_offset = TCP_PORT_OFFSET;
	}
	else
	{
	    tcp_port_offset = ntohs ( (*service_entry).s_port );
	    if (TCP_PORT_OFFSET != tcp_port_offset)
	    {
		(void) fprintf (stderr,
				"WARNING: NIS sevices database lists Karma");
		(void) fprintf (stderr, " as having port number: %d\n",
				tcp_port_offset);
		(void) fprintf (stderr,
				"whereas the hardcoded default is: %d\n",
				TCP_PORT_OFFSET);
	    }
	}
    }
    /*  Try to connect to specified host  */
    if (addr == 0)
    {
	/*  Unix domain socket/ connection  */
	/*  Create socket  */
	if ( ( sock = socket (AF_UNIX, SOCK_STREAM, 0) ) < 0 )
	{
	    (void) fprintf (stderr, "Error creating socket\t%s\n",
			    sys_errlist[errno]);
	    (void) exit (RV_SYS_ERROR);
	}
	/*  Set close-on-exec flag  */
	if (fcntl (sock, F_SETFD, 1) == -1)
	{
	    (void) fprintf (stderr,
			    "Error setting close-on-exec flag for Unix socket\t%s\n",
			    sys_errlist[errno]);
	    if (close (sock) != 0)
	    {
		(void) fprintf (stderr,
				"Error closing Unix socket\t%s\n",
				sys_errlist[errno]);
	    }
	    (void) exit (RV_SYS_ERROR);
	}
	/*  Set up addressing info  */
	un_addr.sun_family = AF_UNIX;
	(void) sprintf (un_addr.sun_path, "%s/%s%d", UNIX_SOCKET_DIR,
			UNIX_SOCKET_FILE, port_number);
	/*  Connect to other end (server module)  */
	if (connect (sock, (struct sockaddr *) &un_addr, (int) sizeof un_addr)
	    != 0)
	{
	    /*  No connection made  */
	    if ( (errno != ECONNREFUSED) && (errno != ENOENT) )
	    {
		(void) fprintf (stderr,
				"Error connecting to Unix socket\t%s\n",
				sys_errlist[errno]);
		exit (RV_SYS_ERROR);
	    }
	    if (close (sock) != 0)
	    {
		(void) fprintf (stderr,
				"Error closing unix socket: %d\t%s\n",
				sock, sys_errlist[errno]);
	    }
	    return (-1);
	}
	/*  Return the connection descriptor  */
	return (sock);
    }
    /*  Internet/ TCP domain socket/ connection  */
    /*  Create socket  */
    if ( ( sock = socket (AF_INET, SOCK_STREAM, 0) ) < 0 )
    {
	(void) fprintf (stderr, "Error creating socket\t%s\n",
			sys_errlist[errno]);
	(void) exit (RV_SYS_ERROR);
    }
    /*  Set close-on-exec flag  */
    if (fcntl (sock, F_SETFD, 1) == -1)
    {
	(void) fprintf (stderr,
			"Error setting close-on-exec flag for Internet socket\t%s\n",
			sys_errlist[errno]);
	if (close (sock) != 0)
	{
	    (void) fprintf (stderr,
			    "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	}
	(void) exit (RV_SYS_ERROR);
    }
    /*  Set up addressing info  */
    in_addr.sin_family = AF_INET;
    in_addr.sin_port = htons (port_number + tcp_port_offset);
    in_addr.sin_addr.s_addr = htonl (addr);
    /*  Connect to other end (server module)  */
    if (connect (sock, (struct sockaddr *) &in_addr, (int) sizeof in_addr)
	!= 0)
    {
	/*  No connection made  */
	if (errno != ECONNREFUSED)
	{
	    (void) fprintf (stderr,
			    "Error connecting to Internet socket\t%s\n",
			    sys_errlist[errno]);
	    exit (RV_SYS_ERROR);
	}
	if (close (sock) != 0)
	{
	    (void) fprintf (stderr, "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	}
	return (-1);
    }
    /*  Return the connection descriptor  */
    return (sock);
}   /*  End Function connect_to_port  */

static int accept_connection_on_dock (dock_index, addr, local)
/*  This routine will accept a connection on a dock.
    The dock index must be given by  dock_index  .
    The address of the host connecting to the dock will be written to the
    storage pointed to by  addr  .
    If the connection is a local connection, then the routine will write the
    value TRUE to the storage pointed to by  local  ,else it will write
    the value FALSE.
    The routine returns a connection on success, else it returns -1
*/
int dock_index;
unsigned long *addr;
flag *local;
{
    /*  Unix dock info  */
    int un_addr_len;
    struct sockaddr_un un_addr;
    /* Internet dock info  */
    int in_addr_len;
    struct sockaddr_in in_addr;
    /*  General info  */
    int fd;
    extern int docks[NUM_DOCKS];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "accept_connection_on_dock";

    switch (dock_index)
    {
      case INDEX_UNIX_DOCK:
	un_addr_len = sizeof un_addr;
	(void) bzero ( (char *) &un_addr, un_addr_len );
	/*  Listen for connections  */
	if ( ( fd = accept (docks[dock_index], (struct sockaddr *) &un_addr,
			    &un_addr_len) )
	    < 0 )
	{
	    (void) fprintf (stderr, "Error accepting unix connection\t%s\n",
			    sys_errlist[errno]);
	    (void) exit (RV_SYS_ERROR);
	}
	*addr = r_get_inet_addr_from_host ( (char *) NULL, (flag *) NULL );
	break;
      case INDEX_INTERNET_DOCK:
	in_addr_len = sizeof in_addr;
	(void) bzero ( (char *) &in_addr, in_addr_len );
	/*  Listen for connections  */
	if ( ( fd = accept (docks[dock_index], (struct sockaddr *) &in_addr,
			    &in_addr_len) )
	    < 0 )
	{
	    (void) fprintf (stderr, "Error accepting connection\t%s\t\n",
			    sys_errlist[errno]);
	    (void) exit (RV_SYS_ERROR);
	}
	*addr = in_addr.sin_addr.s_addr;
	break;
      default:
	(void) fprintf (stderr, "Unknown dock type\n");
	prog_bug (function_name);
	break;
    }
    if (r_get_inet_addr_from_host ( (char *) NULL, (flag *) NULL ) == *addr)
    {
	*local = TRUE;
    }
    else
    {
	*local = FALSE;
    }
    /*  Set close-on-exec flag  */
    if (fcntl (fd, F_SETFD, 1) == -1)
    {
	(void) fprintf (stderr,
			"Error setting close-on-exec flag for descriptor: %d\t%s\n",
			fd, sys_errlist[errno]);
	(void) close (fd);
	return (-1);
    }
    return (fd);
}  /*  End Function accept_connection_on_dock  */

static flag close_connection (connection)
/*  This routine will close a connection.
    The connection to close must be given by  connection  .
    The routine returns TRUE on success, else it returns FALSE.
*/
int connection;
{
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    if (close (connection) != 0)
    {
	(void) fprintf (stderr, "Error closing descriptor: %d\t%s\n",
			connection, sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}  /*  End Function close_connection  */

static int get_bytes_readable (connection)
/*  This routine will determine the minimum number of bytes readable on a
    connection. There may be more bytes readable than indicated.
    The connection should be given by  connection  .
    The routine returns the number of bytes readable on success,
    else it returns -1
*/
int connection;
{
    int bytes_available = 0;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    if (ioctl (connection, (int) FIONREAD, (char *) &bytes_available ) != 0)
    {
	(void) fprintf (stderr,
			"Error getting number of bytes readable on descriptor: %d\t%s\n",
			connection, sys_errlist[errno]);
	return (-1);
    }
    return (bytes_available);
}  /*  End Function get_bytes_readable  */

static int open_stdin (disc)
/*  This routine will open the standard input.
    The routine will write the value TRUE to the storage pointed to by  disc
    if the standard input is a disc, else it will write FALSE.
    The routine returns the descriptor on success, else it returns -1
*/
flag *disc;
{
    struct stat statbuf;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    if (fstat (0, &statbuf) != 0)
    {
	(void) fprintf (stderr,
			"Error getting stats on input descriptor\t%s\n",
			sys_errlist[errno]);
	return (-1);
    }
    if ( S_ISREG (statbuf.st_mode) || S_ISCHR (statbuf.st_mode) ||
	S_ISFIFO (statbuf.st_mode) )
    {
	/*  Legal object  */
	/*  Tag type  */
	if ( S_ISREG (statbuf.st_mode) )
	{
	    *disc = TRUE;
	}
	else
	{
	    *disc = FALSE;
	}
    }
    else
    {
	/*  Illegal object  */
	(void) fprintf (stderr, "Illegal input descriptor\n");
	return (-1);
    }
    return (0);
}   /*  End Function open_stdin  */


#endif  /*  HAS_SOCKETS  */

#ifdef HAS_COMMUNICATIONS_EMULATION

#  ifdef ARCH_VXMVX
static void init_control_connection ()
/*  This routine will initialise the control connection to the host service
    process.
    This routine may be called repeatedly, only on the first call does it do
    anything.
    The routine returns nothing.
*/
{
    int count;
    int num_env;
    char *ptr;
    void *msg;
    extern int parent_pid;
    extern int incoming_control_message_queue;
    extern int outgoing_control_message_queue;
    extern char **environ;
    static flag initialised = FALSE;

    if (initialised) return;
    initialised = TRUE;
    if ( ( msg = task_recv_msg (0, 0, 1) ) == NULL )
    {
	(void) fprintf (stderr, "Error receiving first message\n");
	exit (RV_UNDEF_ERROR);
    }
    parent_pid = *(int *) msg;
    outgoing_control_message_queue = *( (int *) msg + 1 );
    /*  Get environment  */
    num_env = *( (int *) msg + 2 );
    ptr = (char *) msg + 3 * sizeof outgoing_control_message_queue;
    if ( ( environ = (char **) malloc ( sizeof *environ * (num_env + 1) ) )
	== NULL )
    {
	(void) fprintf (stderr,
			"Error allocating space for: %d environment strings\n",
			num_env);
	exit (RV_MEM_ERROR);
    }
    for (count = 0; count < num_env; ++count)
    {
	if ( ( environ[count] = strdup (ptr) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "Error allocating space for environment string: \"%s\"\n",
			    ptr);
	    exit (RV_MEM_ERROR);
	}
	ptr += strlen (ptr) + 1;
    }
    environ[count] = NULL;
    task_free_msg (msg);
    if ( ( incoming_control_message_queue = task_create_msg_q () ) < 0 )
    {
	(void) fprintf (stderr, "Error creating control message queue\n");
	exit (RV_UNDEF_ERROR);
    }
    if ( ( msg = task_alloc_msg (sizeof incoming_control_message_queue, 1) )
	== NULL )
    {
	(void) fprintf (stderr, "Error allocating message buffer\n");
	exit (RV_MEM_ERROR);
    }
    *(int *) msg = incoming_control_message_queue;
    task_send_msg (outgoing_control_message_queue, msg);
    /*  Install remote memory copy facility  */
    remote_memcpy_install ();
}   /*  End Function init_control_connection  */

static int *alloc_port (port_number, retries, num_docks)
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
    The docks are placed into non-blocking mode.
    The routine returns a pointer to a statically allocated array of docks on
    success, else it returns NULL.
*/
unsigned int *port_number;
unsigned int retries;
unsigned int *num_docks;
{
    flag bound;
    int connect_queue;
    int fd;
    unsigned int retry_number;
    unsigned int dock_count;
    void *msg;
    extern int docks[NUM_DOCKS];
    extern unsigned int num_docks_open;
    extern unsigned int allocated_port_number;
    extern int incoming_control_message_queue;
    extern int outgoing_control_message_queue;
    extern struct descriptor_type descriptors[MAX_DESCRIPTORS];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "alloc_port";

    init_control_connection ();
    if ( ( msg = task_alloc_msg (3 * sizeof (unsigned int), 1) ) == NULL )
    {
	(void) fprintf (stderr, "Error allocating message queue\n");
	exit (RV_UNDEF_ERROR);
    }
    *(unsigned int *) msg = HR_ALLOC_PORT;
    *( (unsigned int *) msg + 1) = *port_number;
    *( (unsigned int *) msg + 2) = retries;
    task_send_msg (outgoing_control_message_queue, msg);
    if ( ( msg = task_recv_msg (incoming_control_message_queue, 0, 1) )
	== NULL )
    {
	(void) fprintf (stderr, "Error receiving control message\n");
	exit (RV_UNDEF_ERROR);
    }
    if ( (num_docks_open = *(unsigned int *) msg) < 1 )
    {
	return (NULL);
    }
    allocated_port_number = *( (unsigned int *) msg + 1 );
    for (dock_count = 0; dock_count < num_docks_open; ++dock_count)
    {
	if ( ( fd = find_spare_descriptor () ) < 0 )
	{
	    (void) fprintf (stderr, "Descriptor limit reached\n");
	    exit (RV_UNDEF_ERROR);
	}
	docks[dock_count] = fd + DESCRIPTOR_OFFSET;
	descriptors[fd].open = TRUE;
	descriptors[fd].received_closure = FALSE;
	if ( ( descriptors[fd].incoming_message_queue =
	      task_create_msg_q () ) < 0 )
	{
	    (void) fprintf (stderr, "Error creating message queue\n");
	    exit (RV_UNDEF_ERROR);
	}
	descriptors[fd].outgoing_message_queue = *( (int *) msg + dock_count
						   + 2);
	descriptors[fd].incoming_msg = NULL;
	descriptors[fd].msg_pos = 0;
	descriptors[fd].msg_len = 0;
    }
    task_free_msg (msg);
    if ( ( msg = task_alloc_msg (num_docks_open * sizeof (int), 1) ) == NULL )
    {
	(void) fprintf (stderr, "Error allocating message queue\n");
	exit (RV_UNDEF_ERROR);
    }
    for (dock_count = 0; dock_count < num_docks_open; ++dock_count)
    {
	fd = docks[dock_count] - DESCRIPTOR_OFFSET;
	*( (int *) msg + dock_count) = descriptors[fd].incoming_message_queue;
    }
    task_send_msg (outgoing_control_message_queue, msg);
    *num_docks = num_docks_open;
    *port_number = allocated_port_number;
    return (docks);
}   /*  End Function alloc_port  */

static void close_dock (dock)
/*  This routine will close a dock.
    The dock to close must be given by  dock  .
    The routine returns nothing.
*/
int dock;
{
    void *msg;
    extern int incoming_control_message_queue;
    extern int outgoing_control_message_queue;
    extern struct descriptor_type descriptors[MAX_DESCRIPTORS];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "close_dock";

    init_control_connection ();
    dock -= DESCRIPTOR_OFFSET;
    if ( (dock < 0) || (dock >= MAX_DESCRIPTORS) )
    {
	(void) fprintf (stderr, "Bad descriptor: %d\n",
			dock + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if (descriptors[dock].open != TRUE)
    {
	(void) fprintf (stderr, "Descriptor: %d not open\n",
			dock + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if ( ( msg = task_alloc_msg (0, 1) ) == NULL )
    {
	(void) fprintf (stderr, "Error allocating message queue\n");
	exit (RV_UNDEF_ERROR);
    }
    task_send_msg (descriptors[dock].outgoing_message_queue, msg);
}  /*  End Function close_dock  */

static int connect_to_port (addr, port_number)
/*  This routine will connect to a server module running on the machine
    with Internet address given by  addr  .
    If the value of 0 is supplied for the address, the connection is made to a
    Karma server running on the local machine.
    The port number to connect to must given by  port_number  .
    The close-on-exec flags of the socket is set such that the socket will
    close on a call to execve(2V).
    The routine returns the file descriptor of the opened connection on
    success, else it returns -1
*/
unsigned long addr;
unsigned int port_number;
{
    int fd;
    int errnum;
    void *msg;
    extern int incoming_control_message_queue;
    extern int outgoing_control_message_queue;
    extern struct descriptor_type descriptors[MAX_DESCRIPTORS];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "connect_to_port";

    init_control_connection ();
    if ( ( fd = find_spare_descriptor () ) < 0 )
    {
	(void) fprintf (stderr, "Descriptor limit reached\n");
	return (-1);
    }
    if ( ( descriptors[fd].incoming_message_queue = task_create_msg_q () )
	< 0 )
    {
	(void) fprintf (stderr, "Error creating message queue\n");
	exit (RV_UNDEF_ERROR);
    }
    if ( ( msg = task_alloc_msg (4 * sizeof (unsigned int), 1) ) == NULL )
    {
	(void) fprintf (stderr, "Error allocating message\n");
	exit (RV_UNDEF_ERROR);
    }
    *(unsigned int *) msg = HR_CONNECT_TO_PORT;
    *( (unsigned int *) msg + 1 ) = addr;
    *( (unsigned int *) msg + 2 ) = port_number;
    *( (int *) msg + 3 ) = descriptors[fd].incoming_message_queue;
    task_send_msg (outgoing_control_message_queue, msg);
    if ( ( msg = task_recv_msg (incoming_control_message_queue, 0, 1) )
	== NULL )
    {
	(void) fprintf (stderr, "Error receiving control message\n");
	exit (RV_UNDEF_ERROR);
    }
    if ( (errnum = *(int *) msg) != 0 )
    {
	/*  Could not connect  */
	if (errnum != ECONNREFUSED)
	{
	    /*  Error  */
	    (void) fprintf (stderr, "Error connecting\t%s\n",
			    sys_errlist[errnum]);
	}
	task_free_msg (msg);
	return (-1);
    }
    descriptors[fd].open = TRUE;
    descriptors[fd].received_closure = FALSE;
    descriptors[fd].outgoing_message_queue = *( (int *) msg + 1 );
    descriptors[fd].incoming_msg = NULL;
    descriptors[fd].msg_pos = 0;
    descriptors[fd].msg_len = 0;
    task_free_msg (msg);
    /*  Return the connection descriptor  */
    return (fd + DESCRIPTOR_OFFSET);
}   /*  End Function connect_to_port  */

static int accept_connection_on_dock (dock_index, addr, local)
/*  This routine will accept a connection on a dock.
    The dock index must be given by  dock_index  .
    The address of the host connecting to the dock will be written to the
    storage pointed to by  addr  .
    If the connection is a local connection, then the routine will write the
    value TRUE to the storage pointed to by  local  ,else it will write
    the value FALSE.
    The routine returns a connection on success, else it returns -1
*/
int dock_index;
unsigned long *addr;
flag *local;
{
    int fd;
    int errnum;
    int dock;
    int accept_message_queue;
    void *msg;
    void *in_msg;
    extern int incoming_control_message_queue;
    extern int outgoing_control_message_queue;
    extern int docks[NUM_DOCKS];
    extern struct descriptor_type descriptors[MAX_DESCRIPTORS];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "accept_connection_on_dock";

    init_control_connection ();
    dock = docks[dock_index];
    dock -= DESCRIPTOR_OFFSET;
    if ( (dock < 0) || (dock >= MAX_DESCRIPTORS) )
    {
	(void) fprintf (stderr, "Bad descriptor: %d\n",
			dock + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if (descriptors[dock].open != TRUE)
    {
	(void) fprintf (stderr, "Descriptor: %d not open\n",
			dock + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if ( (in_msg = descriptors[dock].incoming_msg) == NULL )
    {
	(void) fprintf (stderr, "No request on dock\n");
	prog_bug (function_name);
    }
    accept_message_queue = *(int *) in_msg;
    descriptors[dock].incoming_msg = NULL;
    descriptors[dock].msg_pos = 0;
    descriptors[dock].msg_len = 0;
    if ( ( msg = task_alloc_msg (sizeof (int), 1) ) == NULL )
    {
	(void) fprintf (stderr, "Error allocating message\n");
	exit (RV_UNDEF_ERROR);
    }
    if ( ( fd = find_spare_descriptor () ) < 0 )
    {
	(void) fprintf (stderr, "Descriptor limit reached\n");
	*(int *) msg = -1;
	task_send_msg (accept_message_queue, msg);
	task_free_msg (in_msg);
	return (-1);
    }
    if ( ( descriptors[fd].incoming_message_queue = task_create_msg_q () )
	< 0 )
    {
	(void) fprintf (stderr, "Error creating message queue\n");
	exit (RV_UNDEF_ERROR);
    }
    *(int *) msg = descriptors[fd].incoming_message_queue;
    task_send_msg (accept_message_queue, msg);
    descriptors[fd].open = TRUE;
    descriptors[fd].received_closure = FALSE;
    descriptors[fd].outgoing_message_queue = *( (int *) in_msg + 1 );
    descriptors[fd].incoming_msg = NULL;
    descriptors[fd].msg_pos = 0;
    descriptors[fd].msg_len = 0;
    *local = *( (flag *) in_msg + 2 );
    *addr = *( (unsigned long *) in_msg + 3 );
    task_free_msg (in_msg);
    /*  Return the connection descriptor  */
    return (fd + DESCRIPTOR_OFFSET);
}   /*  End Function accept_connection_on_dock  */

static flag close_connection (connection)
/*  This routine will close a connection.
    The connection to close must be given by  connection  .
    The routine returns TRUE on success, else it returns FALSE.
*/
int connection;
{
    void *msg;
    extern struct descriptor_type descriptors[MAX_DESCRIPTORS];
    static char function_name[] = "close_connection";

    connection -= DESCRIPTOR_OFFSET;
    if ( (connection < 0) || (connection >= MAX_DESCRIPTORS) )
    {
	(void) fprintf (stderr, "Bad descriptor: %d\n",
			connection + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if (descriptors[connection].open != TRUE)
    {
	(void) fprintf (stderr, "Descriptor: %d not open\n",
			connection + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    descriptors[connection].open = FALSE;
    if ( ( msg = descriptors[connection].incoming_msg ) != NULL )
    {
	/*  There was some previous message  */
	task_free_msg (msg);
	descriptors[connection].incoming_message_queue = NULL;
	descriptors[connection].msg_pos = 0;
    }
    if (descriptors[connection].received_closure)
    {
	/*  Closure message was received: go no further  */
	(void) fprintf (stderr, "%s: Close remote\n", function_name);
	return (TRUE);
    }
    (void) fprintf (stderr, "%s: Close local: send 0 bytes\n", function_name);
    if (descriptors[connection].outgoing_message_queue > -1)
    {
	if ( ( msg = task_alloc_msg (0, 1) ) == NULL )
	{
	    (void) fprintf (stderr, "Error allocating message\n");
	    exit (RV_UNDEF_ERROR);
	}
	task_send_msg (descriptors[connection].outgoing_message_queue, msg);
    }
    return (TRUE);
}   /*  End Function close_connection  */

static int find_spare_descriptor ()
/*  This routine will find a spare descriptor number.
    The routine returns the descriptor on success, else it returns -1
*/
{
    unsigned int count;
    extern struct descriptor_type descriptors[MAX_DESCRIPTORS];

    for (count = 0; count < MAX_DESCRIPTORS; ++count)
    {
	if (descriptors[count].open != TRUE)
	{
	    return (count);
	}
    }
    return (-1);
}   /*  End Function find_spare_descriptor  */

static int read_from_connection (fd, buf, nbytes)
/*  This routine is similar to the system  read(2)  call, except that the
    number of bytes requested is always returned (except on error). Hence, if
    the descriptor references a socket, the routine will read as much data as
    was requested, rather than a lesser amount due to packetisation or
    interrupted system calls.
    The file descriptor to read from must be given by  fd  .This descriptor
    must NOT be set to non-blocking IO.
    The buffer in which to write the data must be pointed to by  buf  .
    The number of bytes to read must be given by  nbytes  .
    The routine returns the number of bytes requested on success,
    the number of bytes read on end of file and -1 on error.
*/
int fd;
char *buf;
int nbytes;
{
    int byte_count;
    int bytes_left;
    char *tmp = buf;
    void *msg;
    char *ptr;
    extern struct descriptor_type descriptors[MAX_DESCRIPTORS];
    static char function_name[] = "read_from_connection";

    fd -= DESCRIPTOR_OFFSET;
    if ( (fd < 0) || (fd >= MAX_DESCRIPTORS) )
    {
	(void) fprintf (stderr, "Bad descriptor: %d\n",
			fd + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if (descriptors[fd].open != TRUE)
    {
	(void) fprintf (stderr, "Descriptor: %d not open\n",
			fd + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if (descriptors[fd].incoming_msg == NULL)
    {
	if ( ( msg =
	      task_recv_msg (descriptors[fd].incoming_message_queue, 0,
			     1) ) == NULL )
	{
	    (void) fprintf (stderr, "Error reading message\n");
	    exit (RV_UNDEF_ERROR);
	}
	descriptors[fd].incoming_msg = msg;
	descriptors[fd].msg_len = task_sizeof_msg (msg);
	if (descriptors[fd].msg_len == 0)
	{
	    descriptors[fd].received_closure = TRUE;
	}
	descriptors[fd].msg_pos = 0;
    }
    msg = descriptors[fd].incoming_msg;
    bytes_left = (descriptors[fd].msg_len -
		 descriptors[fd].msg_pos);
    if (nbytes >= bytes_left)
    {
	nbytes = bytes_left;
	for (byte_count = 0, ptr = (char *) msg + descriptors[fd].msg_pos;
	     byte_count < nbytes;
	     ++byte_count, ++ptr, ++buf)
	{
	    *buf = *ptr;
	}
	descriptors[fd].incoming_msg = NULL;
	descriptors[fd].msg_pos = 0;
	descriptors[fd].msg_len = 0;
	task_free_msg (msg);
    }
    else
    {
	for (byte_count = 0, ptr = (char *) msg + descriptors[fd].msg_pos;
	     byte_count < nbytes;
	     ++byte_count, ++ptr, ++buf)
	{
	    *buf = *ptr;
	}
	descriptors[fd].msg_pos += nbytes;
    }
    return (nbytes);
}   /*  End Function read_from_connection  */

static int write_to_connection (fd, buf, nbytes)
/*  This routine is similar to the system  write(2)  call, except that the
    number of bytes requested is always returned (except on error). Hence, if
    the descriptor references a socket, the routine will write as much data as
    was requested, rather than a lesser amount due to packetisation or
    interrupted system calls.
    The file descriptor to write from must be given by  fd  .This descriptor
    must NOT be set to non-blocking IO.
    The buffer in which to write the data must be pointed to by  buf  .
    The number of bytes to write must be given by  nbytes  .
    The routine returns the number of bytes requested on success,
    else it returns -1 indicating error.
*/
int fd;
char *buf;
int nbytes;
{
    int byte_count;
    void *msg;
    char *ptr;
    extern struct descriptor_type descriptors[MAX_DESCRIPTORS];
    static char function_name[] = "write_to_connection";

    fd -= DESCRIPTOR_OFFSET;
    if ( (fd < 0) || (fd >= MAX_DESCRIPTORS) )
    {
	(void) fprintf (stderr, "Bad descriptor: %d\n",
			fd + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if (descriptors[fd].open != TRUE)
    {
	(void) fprintf (stderr, "Descriptor: %d not open\n",
			fd + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if ( ( msg = task_alloc_msg (nbytes, 1) ) == NULL )
    {
	(void) fprintf (stderr, "Error allocating message\n");
	exit (RV_UNDEF_ERROR);
    }
    ptr = (char *) msg;
    byte_count = nbytes;
    while (byte_count-- > 0)
    {
	*ptr++ = *buf++;
    }
    task_send_msg (descriptors[fd].outgoing_message_queue, msg);
    return (nbytes);
}   /*  End Function write_to_connection  */

static int get_bytes_readable (connection)
/*  This routine will determine the minimum number of bytes readable on a
    connection. There may be more bytes readable than indicated.
    The connection should be given by  connection  .
    The routine returns the number of bytes readable on success,
    else it returns -1
*/
int connection;
{
    int bytes_available;
    void *msg;
    extern struct descriptor_type descriptors[MAX_DESCRIPTORS];
    static char function_name[] = "get_bytes_readable";

    connection -= DESCRIPTOR_OFFSET;
    if ( (connection < 0) || (connection >= MAX_DESCRIPTORS) )
    {
	(void) fprintf (stderr, "Bad descriptor: %d\n",
			connection + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    if (descriptors[connection].open != TRUE)
    {
	(void) fprintf (stderr, "Descriptor: %d not open\n",
			connection + DESCRIPTOR_OFFSET);
	prog_bug (function_name);
    }
    msg = descriptors[connection].incoming_msg;
    if (msg == NULL)
    {
	if ( ( msg =
	      task_recv_msg (descriptors[connection].incoming_message_queue, 0,
			     0) ) == NULL )
	{
	    descriptors[connection].incoming_msg = NULL;
	    descriptors[connection].msg_len = 0;
	    descriptors[connection].msg_pos = 0;
	}
	else
	{
	    descriptors[connection].incoming_msg = msg;
	    descriptors[connection].msg_len = task_sizeof_msg (msg);
	    descriptors[connection].msg_pos = 0;
	    if (descriptors[connection].msg_len == 0)
	    {
		descriptors[connection].received_closure = TRUE;
	    }
	}
    }
    if (msg == NULL)
    {
	return (0);
    }
    return (descriptors[connection].msg_len - descriptors[connection].msg_pos);
}  /*  End Function get_bytes_readable  */

static unsigned long get_inet_addr_from_host (host, local)
/*  This routine will get the first listed Internet address from the hostname
    string pointed to by  host  .
    IF the specified host is the local machine, then the routine will write
    the value TRUE to the storage pointed to by  local  ,else it will write
    the value FALSE here. If this is NULL, nothing is written here.
    The routine returns the Internet address on success (in host byte order),
    else it returns 0.
*/
char *host;
flag *local;
{
    unsigned long addr;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static unsigned long local_addr = 0;

    if (local_addr < 1)
    {
	/*  Get my host address  */
	if ( ( local_addr = conv_hostname_to_addr ("localhost") ) == 0 )
	{
	    (void) fprintf (stderr, "Error getting local host address\n");
	    return (0);
	}
    }
    if ( (host == NULL) || (strcmp (host, "unix") == 0) ||
	(strcmp (host, "localhost") == 0) )
    {
	/*  Local machine  */
	if (local != NULL)
	{
	    *local = TRUE;
	}
	return (local_addr);
    }
    if ( ( addr = conv_hostname_to_addr (host) ) == 0 )
    {
	(void) fprintf (stderr, "Error getting host address\n");
	return (0);
    }
    if (addr == local_addr)
    {
	if (local != NULL)
	{
	    *local = TRUE;
	}
    }
    else
    {
	if (local != NULL)
	{
	    *local = FALSE;
	}
    }
    return (addr);
}   /*  End Function get_inet_addr_from_host  */

static unsigned long conv_hostname_to_addr (host)
/*  This routine will get the first listed Internet address from the hostname
    string pointed to by  host  .
    The routine returns the Internet address on success (in host byte order),
    else it returns 0.
*/
char *host;
{
    int length;
    unsigned long addr;
    void *msg;
    extern int incoming_control_message_queue;
    extern int outgoing_control_message_queue;

    init_control_connection ();
    length = sizeof (unsigned int) + strlen (host) + 1;
    if ( ( msg = task_alloc_msg (length, 1) ) == NULL )
    {
	(void) fprintf (stderr, "Error allocating message\n");
	exit (RV_UNDEF_ERROR);
    }
    *(unsigned int *) msg = HR_GET_INET_ADDR;
    (void) strcpy ( (char *) msg + sizeof (unsigned int), host );
    task_send_msg (outgoing_control_message_queue, msg);
    if ( ( msg = task_recv_msg (incoming_control_message_queue, 0, 1) )
	== NULL )
    {
	(void) fprintf (stderr, "Error receiving control message\n");
	exit (RV_UNDEF_ERROR);
    }
    addr = *(unsigned long *) msg;
    task_free_msg (msg);
    return (addr);
}   /*  End Function conv_hostname_to_addr  */

static int open_stdin (disc)
/*  This routine will open the standard input.
    The routine will write the value TRUE to the storage pointed to by  disc
    if the standard input is a disc, else it will write FALSE.
    The routine returns the descriptor on success, else it returns -1
*/
flag *disc;
{
    int fd;
    int errnum;
    int conn_message_queue;
    void *msg;
    extern int incoming_control_message_queue;
    extern int outgoing_control_message_queue;
    extern struct descriptor_type descriptors[MAX_DESCRIPTORS];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "open_stdin";

    init_control_connection ();
    if ( ( fd = find_spare_descriptor () ) < 0 )
    {
	(void) fprintf (stderr, "Descriptor limit reached\n");
	return (-1);
    }
    if ( ( conn_message_queue = task_create_msg_q () ) < 0 )
    {
	(void) fprintf (stderr, "Error creating message queue\n");
	exit (RV_UNDEF_ERROR);
    }
    if ( ( msg = task_alloc_msg (2 * sizeof (unsigned int), 1) ) == NULL )
    {
	(void) fprintf (stderr, "Error allocating message\n");
	exit (RV_UNDEF_ERROR);
    }
    *(unsigned int *) msg = HR_OPEN_STDIN;
    *( (int *) msg + 1 ) = conn_message_queue;
    task_send_msg (outgoing_control_message_queue, msg);
    if ( ( msg = task_recv_msg (incoming_control_message_queue, 0, 1) )
	== NULL )
    {
	(void) fprintf (stderr, "Error receiving control message\n");
	exit (RV_UNDEF_ERROR);
    }
    if ( (errnum = *(int *) msg) == -1 )
    {
	/*  Standard input is a disc  */
	*disc = TRUE;
	task_free_msg (msg);
	return (0);
    }
    if (errnum != 0)
    {
	(void) fprintf (stderr, "Error connecting\t%s\n", sys_errlist[errnum]);
	task_free_msg (msg);
	return (-1);
    }
    descriptors[fd].open = TRUE;
    descriptors[fd].received_closure = FALSE;
    descriptors[fd].incoming_message_queue = conn_message_queue;
    descriptors[fd].outgoing_message_queue = -1;
    descriptors[fd].incoming_msg = NULL;
    descriptors[fd].msg_pos = 0;
    descriptors[fd].msg_len = 0;
    task_free_msg (msg);
    /*  Return the connection descriptor  */
    *disc = FALSE;
    return (fd + DESCRIPTOR_OFFSET);
}   /*  End Function open_stdin  */

int get_hostname (name, namelen)
/*  This routine will determine the local hostname.
    The buffer to write the hostname to must be pointed to by  name  .
    The length of the buffer must be given by  namelen  .
    The routine returns 0. On failure, the routine exits the process.
*/
char *name;
unsigned int namelen;
{
    void *msg;
    extern int incoming_control_message_queue;
    extern int outgoing_control_message_queue;

    init_control_connection ();
    if ( ( msg = task_alloc_msg (sizeof (unsigned int), 1) ) == NULL )
    {
	(void) fprintf (stderr, "Error allocating message\n");
	exit (RV_UNDEF_ERROR);
    }
    *(unsigned int *) msg = HR_GET_HOSTNAME;
    task_send_msg (outgoing_control_message_queue, msg);
    if ( ( msg = task_recv_msg (incoming_control_message_queue, 0, 1) )
	== NULL )
    {
	(void) fprintf (stderr, "Error receiving control message\n");
	exit (RV_UNDEF_ERROR);
    }
    (void) strncpy (name, msg, namelen);
    task_free_msg (msg);
    return (0);
}   /*  End Function get_hostname  */

int get_ppid ()
/*  This routine will determine the parent process ID.
    The routine returns the parent process ID.
*/
{
    extern int parent_pid;

    init_control_connection ();
    return (parent_pid);
}   /*  End Function get_ppid  */

#  endif  /*  ARCH_VXMVX  */
#endif  /*  HAS_COMMUNICATIONS_EMULATION  */

static void prog_bug (function_name)
char *function_name;
{
    (void) fprintf (stderr, "Program bug noted in function: %s\n",
		    function_name);
    (void) fprintf (stderr, "Aborting.%c\n", BEL);
    exit (RV_PROGRAM_BUG);
}   /*  End Function prog_bug   */

/*  vxkarma_cm_slave.c

    VX slave process setup file for Connection Management tool and shell.

    Copyright (C) 1992,1993,1994  Richard Gooch

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

/*  This Karma programme will connect to a Connection Management tool or shell
    in order to process module creation commands for a VX/MVX processor.


    Written by      Richard Gooch   26-DEC-1992

    Updated by      Richard Gooch   11-JAN-1993

    Updated by      Richard Gooch   31-JUL-1993: Improved data transfer rate
  over Karma connections from Unix world to VX/MVX

    Updated by      Richard Gooch   15-APR-1994: Added support for passing of
  arguments to modules.

    Updated by      Richard Gooch   19-APR-1994: Added support for specifying
  VX video mode (none, mono video or stereo video).

    Last updated by Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>


    Usage:   vxkarma_cm_slave host port display
               [-novideo | -mono1152x900 | -stereo1152x900]

*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/param.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <vx/vx.h>
#include <karma.h>
#include <karma_conn.h>
#include <karma_chm.h>
#include <karma_pio.h>
#include <karma_ch.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_r.h>

#define MAX_VXNODES 8
#define BUFFER_SIZE 65536

#define HR_ALLOC_PORT 0
#define HR_CONNECT_TO_PORT 1
#define HR_GET_INET_ADDR 2
#define HR_OPEN_STDIN 3
#define HR_GET_HOSTNAME 4

#define TCP_PORT_OFFSET 6200
#define UNIX_SOCKET_DIR "/tmp/.KARMA_connections"
#define UNIX_SOCKET_FILE "KARMA"
#define INDEX_UNIX_DOCK 0
#define INDEX_INTERNET_DOCK 1
#define NUM_DOCKS 2

/*  External functions  */
EXTERN_FUNCTION (int slave_setup, (int argc, char *argv[],
				   unsigned int *cm_port_number,
				   unsigned long *cm_host_addr,
				   flag (*open_func) (), flag (*read_func) (),
				   void (*close_func) () ) );


/*  Structure defining "child" process (VX process)  */
typedef struct
{
    flag running;
    int vx_task_id;
    int service_task_id;
    int request_queue;
    int response_queue;
    int port_num;
    Vxh *handle;
    struct _pseudo_connection *docks[NUM_DOCKS];
    char name[STRING_LENGTH];
} vxchild;

/*  Structure defining VX process channel connections which converse with the
    Unix world
*/
typedef struct _pseudo_connection
{
    Channel channel;
    int incoming_message_queue;
    int outgoing_message_queue;
    vxchild *child;
    struct _pseudo_connection *prev;
    struct _pseudo_connection *next;
} pseudo_connection;



/*  Private functions  */
static void myexit ();
static void attach_nodes ();
static flag open_func ();
static flag read_func ();
static void close_func ();

static void service_routine ();
static void poll_children ();
static void poll_connections ();

static int get_spare_node ();
static vxchild *get_child_of_port ();

static void run_module ();

static void close_connection ();
static void close_all_connections ();

static void open_stdin ();
static void get_inet_addr ();
static void connect_to_port ();
static void alloc_port ();
static void get_hostname ();

static flag con_input_func ();
static void con_close_func ();
static flag dock_input_func ();

static int *alloc_raw_port ();
static Channel *alloc_ch_port ();
static Channel accept_on_dock ();
static Channel open_connection ();


/*  Private data  */
static int tcp_port_offset = -1;
static unsigned long cm_host_addr = 0;
static unsigned int cm_port_number;
static vxchild children[MAX_VXNODES];
static pseudo_connection *connections = NULL;
static pseudo_connection *pseudo_docks = NULL;


void main (argc, argv)
int argc;
char *argv[];
{
    flag stereo = FALSE;
    int argcount;
    int video_mode = -1;
    extern vxchild children[MAX_VXNODES];
    extern int task_fbcacheaddr;
    extern unsigned int cm_port_number;
    extern unsigned long cm_host_addr;
    static char function_name[] = "main";

    argcount = slave_setup (argc, argv, &cm_port_number, &cm_host_addr,
			    open_func, read_func, close_func);
    task_fbcacheaddr = RESERVE_NONE;
    while (argcount < argc)
    {
	if (strcmp (argv[argcount], "-novideo") == 0)
	{
	    task_fbcacheaddr = RESERVE_NONE;
	    video_mode = -1;
	}
	else if (strcmp (argv[argcount], "-mono1152x900") == 0)
	{
	    task_fbcacheaddr = RESERVE_MONO;
	    video_mode = VX_VIDEO_1152_66HZ;
	}
	else if (strcmp (argv[argcount], "-stereo1152x900") == 0)
	{
	    task_fbcacheaddr = RESERVE_STEREO;
	    video_mode = VX_VIDEO_1152_66HZ_S;
	    stereo = TRUE;
	}
	else
	{
	    (void) fprintf (stderr, "Bad parameter: \"%s\"\n", argv[argcount]);
	    exit (RV_BAD_PARAM);
	}
	++argcount;
    }
    attach_nodes (children, video_mode, stereo);
    /*  Set environement variable which a VX process may use to determine the
	video mode it has been given.  */
    if (video_mode < 0)
    {
	r_setenv ("VX_VIDEO_MODE", "NONE");
    }
    else
    {
	if (stereo)
	{
	    r_setenv ("VX_VIDEO_MODE", "STEREO");
	}
	else
	{
	    r_setenv ("VX_VIDEO_MODE", "MONO");
	}
    }
    /*  Primary event loop  */
    while (TRUE)
    {
	chm_poll (0);
	poll_children ();
	poll_connections ();
	task_yield ();
    }
}   /*  End Function main  */

static void myexit (status)
/*  This routine is called in place of exit(3) (note: cannot register this
    function with  on_exit(3)  for some reason.
    The routine returns  status  .
*/
int status;
{
    unsigned int count;
    extern pseudo_connection *connections;
    extern pseudo_connection *pseudo_docks;
    extern vxchild children[MAX_VXNODES];

    /*  Close all connections  */
    close_all_connections (&connections);
    task_sleep (1000);
    /*  Kill all children  */
    for (count = 0; count < MAX_VXNODES; ++count)
    {
	if (children[count].running)
	{
	    (void) fprintf (stderr, "Killing tasks for: %s\n",
			    children[count].name);
	    task_signal (children[count].service_task_id, TASK_DELETE);
	    task_signal (children[count].vx_task_id, TASK_DELETE);
	    children[count].running = FALSE;
	    task_yield ();
	}
    }
    exit (status);
}   /*  End Function myexit  */

static void attach_nodes (children, video_mode, stereo)
/*  This routine will attach to as many VX nodes as possible.
    The routine will initialise the list of children pointed to by  children  .
    The video mode to use must be given by  video_mode  .
    If the value of  stereo  is TRUE, then the mode is a stereo video mode.
    The routine returns nothing.
*/
vxchild children[MAX_VXNODES];
int video_mode;
flag stereo;
{
    flag no_nodes = TRUE;
    int screen_width, screen_height;
    int right_offset;
    unsigned int count;
    Vxh *vx;
    static char function_name[] = "attach_nodes";

    for (count = 0; count < MAX_VXNODES; ++count)
    {
	(void) sprintf (children[count].name, "/dev/vx0:%u", count);
	children[count].running = FALSE;
	children[count].port_num = -1;
	if ( ( children[count].handle = (Vxh *)
	      vx_attach (children[count].name) ) != NULL )
	{
	    no_nodes = FALSE;
	    (void) fprintf (stderr, "SLAVE: Attached: %s\n",
			    children[count].name);

    
	}
	else
	{
	    (void) fprintf (stderr, "SLAVE: Failed: %s\n",
			    children[count].name);
	}
    }
    if (no_nodes)
    {
	(void) fprintf (stderr,
			"SLAVE: Failed attaching to any VX/MVX node\n");
	myexit (RV_UNDEF_ERROR);
    }
    if ( (vx = children[0].handle) == NULL )
    {
	(void) fprintf (stderr,
			"SLAVE: Failed attaching to VX node\n");
	myexit (RV_UNDEF_ERROR);
    }
    /*  VX node: set video display mode  */
    if (video_mode < 0)
    {
	/*  No video: set GX only  */
	vx_win_mgr_init (vx, VX_VIDEO_DIRECT);
	vx_direct_video_mode_set (vx, VX_DISPLAY_GX_ONLY);
    }
    else
    {
	vx_win_mgr_init (vx, VX_VIDEO_DIRECT);
	vx_direct_video_mode_set (vx, VX_DISPLAY_VX_ONLY);
	vx_direct_video_key_set (vx, 0,
				 VX_DISPLAY_CHANNEL, VX_DISPLAY_TRUE_COLOR,
				 VX_COLOR_MAP, 0,
				 VX_SHOW_OVERLAY, FALSE,
				 NULL);
	vx_direct_video_control_set (vx,
				     VX_VIDEO_FORMAT, video_mode,
				     NULL);
	vx_videoformat (vx, &screen_width, &screen_height);
	right_offset = stereo ? screen_width * screen_height * 4 : 0;
	vx_direct_video_control_set (vx,
				     VX_VIEW, 0, right_offset,
				     NULL);
    }
}   /*  End Function attach_nodes  */


/*  The following functions manage the connection to the Connection Management
    tool.
*/

static flag open_func (connection, info)
/*  This routine will register the opening of the connection to the Connection
    Management tool.
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on successful registration,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will not be called upon connection closure.
*/
Connection connection;
void **info;
{
    Channel channel;
    char my_hostname[STRING_LENGTH + 4];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    /*  Get my host information  */
    if (gethostname (my_hostname, STRING_LENGTH) != 0)
    {
	(void) fprintf (stderr, "SLAVE: Error getting hostname\t%s\n",
			sys_errlist[errno]);
	myexit (RV_SYS_ERROR);
    }
    my_hostname[STRING_LENGTH] = NULL;
    (void) strcat (my_hostname, ":vx");
    channel = conn_get_channel (connection);
    if (pio_write_string (channel, my_hostname) != TRUE)
    {
	(void) fprintf (stderr, "SLAVE: Error writing hostname\t%s\n",
			sys_errlist[errno]);
	myexit (RV_WRITE_ERROR);
    }
    if (ch_flush (channel) != TRUE)
    {
	(void) fprintf (stderr, "SLAVE: Error flushing channel\t%s\n",
			sys_errlist[errno]);
	myexit (RV_WRITE_ERROR);
    }
    return (TRUE);
}   /*  End Function open_func  */

static flag read_func (connection, info)
/*  This routine will read from the connection to the Connection Management
    tool.
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on success,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will not be called upon connection closure.
*/
Connection connection;
void **info;
{
    long x;
    long y;
    int child_pid;
    Channel channel;
    char *module_name;
    char *args;
    extern unsigned int cm_port_number;
    extern unsigned long cm_host_addr;
    extern vxchild children[MAX_VXNODES];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    channel = conn_get_channel (connection);
    if ( ( module_name = pio_read_string (channel, (unsigned int *) NULL) )
	== NULL )
    {
	(void) fprintf (stderr, "SLAVE: Error reading module name\t%s\n",
			sys_errlist[errno]);
	myexit (RV_READ_ERROR);
    }
    if (pio_read32s (channel, &x) != TRUE)
    {
	(void) fprintf (stderr, "SLAVE: Error reading x position\t%s\n",
			sys_errlist[errno]);
	myexit (RV_READ_ERROR);
    }
    if (pio_read32s (channel, &y) != TRUE)
    {
	(void) fprintf (stderr, "SLAVE: Error reading y position\t%s\n",
			sys_errlist[errno]);
	myexit (RV_READ_ERROR);
    }
    if ( ( args = pio_read_string (channel, (unsigned int *) NULL) )
	== NULL )
    {
	(void) fprintf (stderr, "SLAVE: Error reading arguments\t%s\n",
			sys_errlist[errno]);
	exit (RV_READ_ERROR);
    }
    if (args[0] == '\0')
    {
	m_free (args);
	args = NULL;
    }
    if ( ( child_pid = get_spare_node (children) ) < 0 )
    {
	(void) fprintf (stderr, "SLAVE: No spare nodes left\n");
    }
    run_module (&children[child_pid], module_name, cm_host_addr,
		cm_port_number, (int) x, (int) y, args);
    m_free (module_name);
    if (args != NULL) m_free (args);
    return (TRUE);
}   /*  End Function read_func  */

static void close_func (connection, info)
/*  This routine will register the closing of the connection to the Connection
    Management tool.
    The connection will be given by  connection  .
    The connection information pointer will be given by  info  .
    The routine returns nothing.
*/
Connection connection;
void *info;
{
    (void) fprintf (stderr,
		    "SLAVE: Lost connection to Connection Management tool\n");
    myexit (RV_OK);
}   /*  End Function close_func  */


/*  The following three routines service events from the VX processes.  */

static void service_routine (child)
vxchild *child;
{
    int i;

    while (1)
    {
	if ( vxhpc_service ( (*child).handle, &i ) )
	{
	    (*child).running = FALSE;
	    return;
	}
	task_yield ();
    }
}   /*  End Function service_routine  */

static void poll_children ()
/*  This routine will poll the request queues of all running VX processes and
    will service them.
    The routine returns nothing.
*/
{
    unsigned int count;
    vxchild *child;
    void *msg;
    char *info;
    extern vxchild children[MAX_VXNODES];

    for (count = 0; count < MAX_VXNODES; ++count)
    {
	if (children[count].handle == NULL) continue;
	if (!children[count].running) continue;
	child = &children[count];
	if ( ( msg = task_recv_msg ( (*child).request_queue, 0, 0 ) ) != NULL )
	{
	    /*  Have a request  */
	    info = (char *) msg + sizeof (unsigned int);
	    switch (*(unsigned int *) msg)
	    {
	      case HR_ALLOC_PORT:
		alloc_port (child, info);
		break;
	      case HR_CONNECT_TO_PORT:
		connect_to_port (child, info);
		break;
	      case HR_GET_INET_ADDR:
		get_inet_addr (child, info);
		break;
	      case HR_OPEN_STDIN:
		open_stdin (child, info);
		break;
	      case HR_GET_HOSTNAME:
		get_hostname (child, info);
		break;
	      default:
		(void) fprintf (stderr,
				"SLAVE: Illegal request: %u  from VX task on: %s\n",
				*(unsigned int *) msg, (*child).name);
		myexit (RV_BAD_DATA);
	    }
	    task_free_msg (msg);
	}
    }
}   /*  End Function poll_children  */

static void poll_connections ()
/*  This routine will poll all connections to the task being serviced for
    activity from the VX processes.
    The routine returns nothing.
*/
{
    flag port_closed;
    int fd;
    unsigned int length;
    unsigned int count;
    vxchild *child;
    void *msg;
    pseudo_connection *curr_con;
    pseudo_connection *next_con;
    extern pseudo_connection *connections;
    extern pseudo_connection *pseudo_docks;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    /*  Poll "real" connections (ie. data transfer connections)  */
    for (curr_con = connections; curr_con != NULL; curr_con = next_con)
    {
	next_con = (*curr_con).next;
	if ( ( msg = task_recv_msg ( (*curr_con).incoming_message_queue,
				    0, 0 ) )
	    != NULL )
	{
	    /*  Have a message  */
	    length = task_sizeof_msg (msg);
	    if (length == 0)
	    {
		/*  Connection is to be closed  */
		(*curr_con).outgoing_message_queue = -1;
		close_connection (&connections, curr_con);
		task_free_msg (msg);
		continue;
	    }
	    fd = ch_get_descriptor ( (*curr_con).channel );
	    if (r_write (fd, msg, length) < length)
	    {
		(void) fprintf (stderr, "SLAVE: Error writing message\t%s\n",
				sys_errlist[errno]);
		myexit (RV_SYS_ERROR);
	    }
	    task_free_msg (msg);
	}
    }
    /*  Poll dock connections  */
    for (curr_con = pseudo_docks; curr_con != NULL; curr_con = next_con)
    {
	next_con = (*curr_con).next;
	if ( ( msg = task_recv_msg ( (*curr_con).incoming_message_queue,
				    0, 0 ) )
	    != NULL )
	{
	    (void) fprintf (stderr, "SLAVE: Closing dock for task\n");
	    /*  Have a message: close dock  */
	    child = (*curr_con).child;
	    for (count = 0, port_closed = TRUE; count < NUM_DOCKS; ++count)
	    {
		if ( (*child).docks[count] == curr_con )
		{
		    /*  Remove dock entry for child  */
		    (*child).docks[count] = NULL;
		}
		else
		{
		    if ( (*child).docks[count] != NULL )
		    {
			port_closed = FALSE;
		    }
		}
	    }
	    (*curr_con).outgoing_message_queue = -1;
	    close_connection (&pseudo_docks, curr_con);
	    if (port_closed) (*child).port_num = -1;
	    task_free_msg (msg);
	    continue;
	}
    }
}   /*   End Function poll_connections  */

    
/*  The following routines get information on VX processes  */

static int get_spare_node (children)
/*  This routine will try to get a spare VX node ID which is runnable and not
    used.
    The node information must be pointed to by  children  .
    The routine returns the ID of the node on success, else it returns -1.
*/
vxchild children[MAX_VXNODES];
{
    int count;

    for (count = 0; count < MAX_VXNODES; ++count)
    {
	if ( (children[count].handle != NULL) && (!children[count].running) )
	{
	    /*  Spare node  */
	    return (count);
	}
    }
    return (-1);
}   /*  End Function get_spare_node  */

static vxchild *get_child_of_port (port_number)
/*  This routine will get the VX process information for a Karma port.
    The port number must be given by  port_number  .
    The routine returns a pointer to the VX process if that process owns the
    port, else it returns NULL.
*/
unsigned int port_number;
{
    unsigned int count;
    extern vxchild children[MAX_VXNODES];

    for (count = 0; count < MAX_VXNODES; ++count)
    {
	if ( children[count].running &&
	    ( children[count].port_num == port_number ) )
	{
	    /*  Got it!  */
	    return (&children[count]);
	}
    }
    return (NULL);
}   /*  End Function get_child_of_port  */


static void run_module (child, module_name, cm_host_addr, cm_port, x, y, args)
/*  This routine will run a module on a VX node.
    The node information must be pointed to by  child  .
    The name of the module to run must be pointed to by  module_name  .
    The Internet address of the machine on which the Connection Management Tool
    is running must be given by  cm_host_addr  .
    The Karma port number to connect to must be given by  cm_port  .
    The window co-ordinates of the icon for the Connection Management Tool must
    be given by  x  and  y  .
    The optional arguments to pass to the new process must be pointed to by
    args  .If this is NULL, no arguments are passed.
    The routine returns nothing.
*/
vxchild *child;
char *module_name;
unsigned long cm_host_addr;
unsigned int cm_port;
int x;
int y;
char *args;
{
    int env_count;
    int env_size;
    char *ptr;
    char **env;
    void *msg;
    char control_env[STRING_LENGTH];
    extern int vxhpc_trace;
    extern int pager_trace;
    extern char **environ;
    static char function_name[] = "run_module";

    (void) fprintf (stderr, "vxkarma_cm_slave: vxhpc_trace: %d\n",vxhpc_trace);
/*
    vxhpc_trace = 1;
*/
    (void) fprintf (stderr, "vxkarma_cm_slave: pager_trace: %d\n",pager_trace);
/*
    pager_trace = 1;
*/
    (void) sprintf (control_env, "KARMA_CM_CONTROL_COMMAND=%lu:%u:%d:%d",
		    cm_host_addr, cm_port, x, y);
    (*child).port_num = -1;
    if ( ( (*child).request_queue = task_create_msg_q () ) < 0 )
    {
	(void) fprintf (stderr, "SLAVE: Error creating request queue\n");
	return;
    }
    /*  Create the service task  */
    if ( ( (*child).service_task_id = task_create (service_routine, 0, 1,
						   child) )
	== 0 )
    {
	(void) fprintf (stderr, "SLAVE: Could not create service task\n");
	return;
    }
    /*  Run the module  */
    if ( ( (*child).vx_task_id = task_exec ( (*child).name, module_name ) )
	== 0 )
    {
	(void) fprintf (stderr,
			"SLAVE: Error executing: \"%s\" on node: \"%s\"\n",
			module_name, (*child).name);
	task_signal ( (*child).service_task_id, TASK_DELETE );
	return;
    }
    /*  Determine size of environment  */
    for (env_size = strlen (control_env) + 1, env_count = 0, env = environ;
	 *env != NULL; ++env)
    {
	++env_count;
	env_size += strlen (*env) + 1;
    }
    if ( ( msg = task_alloc_msg (3 * sizeof (int) + env_size, 1) )
	== NULL )
    {
	(void) fprintf (stderr, "SLAVE: Error allocating message\n");
	myexit (1);
    }
    *(int *) msg = getpid ();
    *( (int *) msg + 1 ) = (*child).request_queue;
    *( (int *) msg + 2 ) = env_count + 1;
    ptr = (char *) msg + 3 * sizeof (*child).request_queue;
    for (env = environ; *env != NULL; ++env)
    {
	(void) strcpy (ptr, *env);
	ptr += strlen (*env) + 1;
    }
    /*  Copy over control environment variable  */
    (void) strcpy (ptr, control_env);
    task_send_msg ( (*child).vx_task_id, msg );
    if ( ( msg = task_recv_msg ( (*child).request_queue, 0, 1 ) ) == NULL )
    {
	(void) fprintf (stderr,
			"SLAVE: Error getting response queue message\n");
	myexit (RV_UNDEF_ERROR);
    }
    (*child).response_queue = *(int *) msg;
    task_free_msg (msg);
    (*child).running = TRUE;
}   /*  End Function run_module  */

static void close_connection (connections, con)
/*  This routine will close and remove a connection from a list of connections
    and will deallocate the connection.
    The connection list must be pointed to by  connections  .
    The connection to be removed must be pointed to by  con  .
    The routine returns nothing.
*/
pseudo_connection **connections;
pseudo_connection *con;
{
    void *msg;
    static char function_name[] = "close_connection";

    /*  Unmanage and close channel if it exists  */
    if ( (*con).channel != NULL )
    {
	chm_unmanage ( (*con).channel );
	(void) close ( ch_get_descriptor ( (*con).channel ) );
	(void) ch_close ( (*con).channel );
    }
    /*  Tell VX task that connection is closed  */
    if ( (*con).outgoing_message_queue > -1 )
    {
	if ( ( msg = task_alloc_msg (0, 1) ) == NULL )
	{
	    m_abort (function_name, "message");
	}
	task_send_msg ( (*con).outgoing_message_queue, msg );
    }
    /*  Remove connection from list  */
    if ( (*con).next != NULL )
    {
	/*  Current connection is not last in the list  */
	(* (*con).next ).prev = (*con).prev;
    }
    if ( (*con).prev == NULL )
    {
	/*  First in list  */
	*connections = (*con).next;
    }
    else
    {
	/*  Current connection is not first in the list  */
	(* (*con).prev ).next = (*con).next;
    }
    m_free ( (char *) con );
}   /*  End Function close_connection  */

static void close_all_connections (connections)
/*  This routine will close and remove all connection from a list of
    connections and will deallocate each connection.
    The connection list must be pointed to by  connections  .
    The routine returns nothing.
*/
pseudo_connection **connections;
{
    while (*connections != NULL)
    {
	close_connection (connections, *connections);
    }
}   /*  End Function close_all_connection  */


/*  The following routines service  r_  routines called by the VX process.  */

static void open_stdin (child, info)
/*  This routine will enable a VX process to open the standard input.
    Since the VX module must be running under the control of the Connection
    Management tool, this is a trap function.
    The VX process information must be pointed to by  child  .
    The routine returns nothing.
*/
vxchild *child;
char *info;
{
    (void) fprintf (stderr,
		    "SLAVE: Module tried to open standard IO!\n");
    myexit (RV_BAD_DATA);
}   /*  End Function open_stdin  */

static void get_inet_addr (child, info)
/*  This routine will enable a VX process to determine the Internet address of
    the host.
    The VX process information must be pointed to by  child  .
    The routine returns nothing.
*/
vxchild *child;
char *info;
{
    unsigned long addr;
    void *msg;
    static char function_name[] = "get_inet_addr";

    addr = r_get_inet_addr_from_host (info, (flag *) NULL);
    if ( ( msg = task_alloc_msg (sizeof addr, 1) ) == NULL )
    {
	(void) fprintf (stderr,
			"SLAVE: Error allocating message\n");
	myexit (RV_UNDEF_ERROR);
    }
    *(unsigned long *) msg = addr;
    task_send_msg ( (*child).response_queue, msg );
}   /*  End Function get_inet_addr  */

static void connect_to_port (child, info)
/*  This routine will enable a VX process to connect to a Karma port.
    The VX process information must be pointed to by  child  .
    The routine returns nothing.
*/
vxchild *child;
char *info;
{
    int queue;
    unsigned int port_number;
    unsigned long addr;
    unsigned long my_addr;
    void *msg;
    pseudo_connection *con;
    vxchild *vxproc;
    extern pseudo_connection *connections;
    static char function_name[] = "connect_to_port";

    addr = *(unsigned long *) info;
    port_number = *(unsigned int *) (info + sizeof addr);
    if ( ( my_addr = r_get_inet_addr_from_host (NULL, (flag *) NULL) ) == 0 )
    {
	(void) fprintf (stderr,
			"SLAVE: Error getting my Internet address\n");
	myexit (RV_UNDEF_ERROR);
    }
    if ( ( con =
	  (pseudo_connection *) m_alloc (sizeof *con) )
	== NULL )
    {
	m_abort (function_name, "pseudo connection");
    }
    (*con).child = NULL;
    (*con).outgoing_message_queue = *(int *) (info + sizeof addr +
					      sizeof port_number);
    if ( (addr == 0) || (my_addr == addr) )
    {
	/*  Local connection  */
	if ( ( vxproc = get_child_of_port (port_number) ) != NULL )
	{
	    /*  Connect directly to VX task  */
	    queue = (*(*vxproc).docks[INDEX_UNIX_DOCK]).outgoing_message_queue;
	    (void) fprintf (stderr,
			    "SLAVE: Direct connection to queue ID: %d\n",
			    queue);
	    if ( ( msg = task_alloc_msg (4 * sizeof (int), 1) ) == NULL )
	    {
		(void) fprintf (stderr, "SLAVE: Error allocating message\n");
		myexit (RV_UNDEF_ERROR);
	    }
	    *(int *) msg = (*child).request_queue;
	    *( (int *) msg + 1 ) = (*con).outgoing_message_queue;
	    *( (flag *) msg + 2 ) = TRUE;
	    *( (unsigned long *) msg + 3 ) = addr;
	    task_send_msg (queue, msg);
	    if ( ( msg = task_recv_msg ( (*child).request_queue, 0, 1 ) )
		== NULL )
	    {
		(void) fprintf (stderr,
				"SLAVE: Error getting response queue message\n");
		myexit (RV_UNDEF_ERROR);
	    }
	    if ( ( queue = *(int *) msg ) < 0 )
	    {
		(void) fprintf (stderr,
				"SLAVE: %s: VX task could not create descriptor\n",
				function_name);
		myexit (RV_UNDEF_ERROR);
	    }
	    task_free_msg (msg);
	    if ( ( msg = task_alloc_msg (2 * sizeof (int), 1) ) == NULL )
	    {
		(void) fprintf (stderr, "SLAVE: Error allocating message\n");
		myexit (RV_UNDEF_ERROR);
	    }
	    *(int *) msg = 0;
	    *( (int *) msg + 1 ) = queue;
	    task_send_msg ( (*child).response_queue, msg );
	    m_free ( (char *) con );
	    return;
	}
    }
    /*  Connect using Unix IPC  */
    if ( ( (*con).incoming_message_queue = task_create_msg_q () ) < 0 )
    {
	(void) fprintf (stderr, "SLAVE: Error creating message queue\n");
	myexit (RV_UNDEF_ERROR);
    }
    if ( ( (*con).channel = open_connection (addr, port_number) ) == NULL )
    {
	if ( ( msg = task_alloc_msg (sizeof (int), 1) ) == NULL )
	{
	    (void) fprintf (stderr, "SLAVE: Error allocating message\n");
	    myexit (RV_UNDEF_ERROR);
	}
	*(int *) msg = ECONNREFUSED;
	task_send_msg ( (*child).response_queue, msg );
	m_free ( (char *) con );
	(void) fprintf (stderr, "ECONNREFUSED\n");
	return;
    }
    if (chm_manage ( (*con).channel, (void *) con,
		    con_input_func, con_close_func,
		    ( flag (*) () ) NULL, ( flag (*) () ) NULL)
	!= TRUE)
    {
	(void) fprintf (stderr, "SLAVE: %s: Error managing channel\n",
			function_name);
	myexit (RV_UNDEF_ERROR);
    }
    if ( ( msg = task_alloc_msg (2 * sizeof (int), 1) ) == NULL )
    {
	(void) fprintf (stderr, "SLAVE: Error allocating message\n");
	myexit (RV_UNDEF_ERROR);
    }
    *(int *) msg = 0;
    *( (int *) msg + 1 ) = (*con).incoming_message_queue;
    task_send_msg ( (*child).response_queue, msg );
    (*con).prev = NULL;
    /*  Insert connection at start of list  */
    if (connections == NULL)
    {
	(*con).next = NULL;
    }
    else
    {
	(*con).next = connections;
	(*connections).prev = con;
    }
    connections = con;
}   /*  End Function connect_to_port  */

static void alloc_port (child, info)
/*  This routine will enable a VX process to allocate a Karma port.
    The VX process information must be pointed to by  child  .
    The routine returns nothing.
*/
vxchild *child;
char *info;
{
    int queue;
    unsigned int port_number;
    unsigned int retries;
    unsigned int num_docks;
    unsigned int dock_count;
    void *msg;
    Channel *docks;
    pseudo_connection *con;
    extern pseudo_connection *pseudo_docks;
    static pseudo_connection *last_con;
    static char function_name[] = "alloc_port";

    if ( (*child).port_num > -1 )
    {
	(void) fprintf (stderr, "SLAVE: Task is asking for a second port!\n");
	myexit (RV_BAD_DATA);
    }
    port_number = *(unsigned int *) info;
    retries = *( (unsigned int *) info + 1 );
    if ( ( docks = alloc_ch_port (&port_number, retries, &num_docks) )
	== NULL )
    {
	/*  Could not allocate port  */
	if ( ( msg = task_alloc_msg (sizeof (unsigned int), 1) ) == NULL )
	{
	    (void) fprintf (stderr, "SLAVE: Error allocating message\n");
	    myexit (RV_UNDEF_ERROR);
	}
	*(unsigned int *) msg = 0;
	task_send_msg ( (*child).response_queue, msg );
	return;
    }
    if ( ( msg = task_alloc_msg (sizeof (unsigned int) * (2 + num_docks), 1) )
	== NULL )
    {
	(void) fprintf (stderr, "SLAVE: Error allocating message\n");
	myexit (RV_UNDEF_ERROR);
    }
    *(unsigned int *) msg = num_docks;
    *( (unsigned int *) msg + 1 ) = port_number;
    for (dock_count = 0; dock_count < num_docks; ++dock_count)
    {
	if ( ( con = (pseudo_connection *) m_alloc (sizeof *con) ) == NULL )
	{
	    m_abort (function_name, "pseudo connection");
	}
	if ( ( queue = task_create_msg_q () ) < 0 )
	{
	    (void) fprintf (stderr, "SLAVE: Error creating message queue\n");
	    myexit (RV_UNDEF_ERROR);
	}
	*( (unsigned int *) msg + 2 + dock_count ) = queue;
	(*con).channel = docks[dock_count];
	(*con).incoming_message_queue = queue;
	(*con).outgoing_message_queue = -1;
	(*con).child = child;
	(*con).next = NULL;
	if (pseudo_docks == NULL)
	{
	    /*  Create list  */
	    pseudo_docks = con;
	    (*con).prev = NULL;
	}
	else
	{
	    /*  Append to end of list  */
	    (*last_con).next = con;
	    (*con).prev = last_con;
	}
	*( (unsigned int *) msg + 2 + dock_count ) = queue;
	if (chm_manage ( (*con).channel, (void *) con,
			dock_input_func, ( void (*) () ) NULL,
			( flag (*) () ) NULL, ( flag (*) () ) NULL)
	    != TRUE)
	{
	    (void) fprintf (stderr, "SLAVE: %s: Error managing channel\n",
			    function_name);
	    myexit (RV_UNDEF_ERROR);
	}
	last_con = con;
	(*child).port_num = port_number;
	(*child).docks[dock_count] = con;
    }
    task_send_msg ( (*child).response_queue, msg );
    if ( ( msg = task_recv_msg ( (*child).request_queue, 0, 1 ) ) == NULL )
    {
	(void) fprintf (stderr,
			"SLAVE: Error getting response queue message\n");
	myexit (RV_UNDEF_ERROR);
    }
    for (dock_count = 0; dock_count < num_docks; ++dock_count)
    {
	con = (*child).docks[dock_count];
	(*con).outgoing_message_queue = *( (int *) msg + dock_count );
    }
    task_free_msg (msg);
}   /*  End Function alloc_port  */

static void get_hostname (child, info)
/*  This routine will enable a VX process to get the hostname of the host it
    is running on.
    The VX process information must be pointed to by  child  .
    The routine returns nothing.
*/
vxchild *child;
char *info;
{
    char name[STRING_LENGTH];
    void *msg;
    static char function_name[] = "get_hostname";

    r_gethostname (name, STRING_LENGTH);
    if ( ( msg = task_alloc_msg (strlen (name) + 1, 1) ) == NULL )
    {
	(void) fprintf (stderr, "SLAVE: Error allocating message\n");
	myexit (RV_UNDEF_ERROR);
    }
    (void) strcpy (msg, name);
    task_send_msg ( (*child).response_queue, msg );
}   /*  End Function get_hostname  */


/*  The following two routines service VX process channel connections and
    docks to the Unix world.
*/

static flag con_input_func (channel, info)
/*  This routine is called when new input occurs on a channel.
    The channel object is given by  channel  .
    An arbitrary pointer may be written to the storage pointed to by  info
    The pointer written here will persist until the channel is unmanaged
    (or a subsequent callback routine changes it).
    The routine returns TRUE if the channel is to remain managed and
    open, else it returns FALSE (indicating that the channel is to be
    unmanaged and closed). This routine MUST NOT unmanage or close the
    channel given by  channel  .
*/
Channel channel;
void **info;
{
    int bytes_in_buffer;
    int bytes_available;
    int bytes_to_read;
    int fd;
    pseudo_connection *con;
    void *msg;
    char buffer[BUFFER_SIZE];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "con_input_func";

    con = (pseudo_connection *) *info;
    fd = ch_get_descriptor (channel);
    /*  Loop through buffer iterations  */
    if ( ( bytes_available = r_get_bytes_readable (fd) ) < 1 ) return (FALSE);
    while (bytes_available > 0)
    {
	/*  Try to fill buffer  */
	bytes_in_buffer = 0;
	while ( (bytes_available > 0) && (bytes_in_buffer < BUFFER_SIZE) )
	{
	    /*  Loop through trying to fill buffer  */
	    if (bytes_in_buffer + bytes_available > BUFFER_SIZE)
	    {
		/*  Too much for buffer: take a bit only  */
		bytes_to_read = BUFFER_SIZE - bytes_in_buffer;
	    }
	    else
	    {
		bytes_to_read = bytes_available;
	    }
	    if (r_read (fd, buffer + bytes_in_buffer, bytes_to_read)
		< bytes_to_read)
	    {
		(void) fprintf (stderr, "SLAVE: Error reading data\t%s\n",
				sys_errlist[errno]);
		return (FALSE);
	    }
	    bytes_in_buffer += bytes_to_read;
	    bytes_available = r_get_bytes_readable (fd);
	}
	if (bytes_in_buffer < 1) return (TRUE);
	/*  Send the buffer  */
	if ( ( msg = task_alloc_msg (bytes_in_buffer, 1) ) == NULL )
	{
	    m_abort (function_name, "message");
	}
	m_copy ( (char *) msg, buffer, bytes_in_buffer );
	task_send_msg ( (*con).outgoing_message_queue, msg );
    }
    return (TRUE);
}   /*  End Function con_input_func  */

static void con_close_func (channel, info)
/*  This routine is called when a channel closes.
    The channel object is given by  channel  .
    The arbitrary pointer for the channel will be pointed to by  info  .
    This routine MUST NOT unmanage the channel pointed to by  channel  ,
    the channel will be automatically unmanaged and deleted upon closure
    (even if no close_func is specified).
    Any unread buffered data in the channel will be lost upon closure. The
    call to this function is the last chance to read this buffered data.
    The routine returns nothing.
*/
Channel channel;
void *info;
{
    pseudo_connection *con;
    extern pseudo_connection *connections;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "con_close_func";

    con = (pseudo_connection *) info;
    (void) close ( ch_get_descriptor (channel) );
    (*con).channel = NULL;
    close_connection (&connections, con);
}   /*  End Function con_close_func  */


/*  The following routine processes connection requests on a Unix dock opened
    on behalf of a VX process.
*/

static flag dock_input_func (dock, info)
/*  This routine is called when new input occurs on a channel.
    The channel object is given by  dock  .
    An arbitrary pointer may be written to the storage pointed to by  info
    The pointer written here will persist until the channel is unmanaged
    (or a subsequent callback routine changes it).
    The routine returns TRUE if the channel is to remain managed and
    open, else it returns FALSE (indicating that the channel is to be
    unmanaged and closed). This routine MUST NOT unmanage or close the
    channel given by  channel  .
*/
Channel dock;
void **info;
{
    flag local;
    unsigned long addr;
    pseudo_connection *dock_con;
    pseudo_connection *con;
    vxchild *child;
    void *msg;
    extern pseudo_connection *connections;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "dock_input_func";

    dock_con = (pseudo_connection *) *info;
    child = (*dock_con).child;
    if ( ( con = (pseudo_connection *) m_alloc (sizeof *con) ) == NULL )
    {
	m_abort (function_name, "pseudo connection");
    }
    (*con).child = NULL;
    if ( ( (*con).channel = accept_on_dock (dock_con, &addr, &local) )
	== NULL )
    {
	return (FALSE);
    }
    if ( ( (*con).incoming_message_queue = task_create_msg_q () ) < 0 )
    {
	(void) fprintf (stderr, "SLAVE: Error creating message queue\n");
	myexit (RV_UNDEF_ERROR);
    }
    if (chm_manage ( (*con).channel, (void *) con,
		    con_input_func, con_close_func,
		    ( flag (*) () ) NULL, ( flag (*) () ) NULL)
	!= TRUE)
    {
	(void) fprintf (stderr, "SLAVE: %s: Error managing channel\n",
			function_name);
	myexit (RV_UNDEF_ERROR);
    }
    if ( ( msg = task_alloc_msg (4 * sizeof (int), 1) ) == NULL )
    {
	(void) fprintf (stderr, "SLAVE: Error allocating message\n");
	myexit (RV_UNDEF_ERROR);
    }
    *(int *) msg = (*child).request_queue;
    *( (int *) msg + 1 ) = (*con).incoming_message_queue;
    *( (flag *) msg + 2 ) = local;
    *( (unsigned long *) msg + 3 ) = addr;
    task_send_msg ( (*dock_con).outgoing_message_queue, msg );
    if ( ( msg = task_recv_msg ( (*child).request_queue, 0, 1 ) ) == NULL )
    {
	(void) fprintf (stderr,
			"SLAVE: Error getting response queue message\n");
	myexit (RV_UNDEF_ERROR);
    }
    if ( ( (*con).outgoing_message_queue = *(int *) msg ) < 0 )
    {
	(void) fprintf (stderr,
			"SLAVE: %s: Task could not create descriptor\n",
			function_name);
	myexit (RV_UNDEF_ERROR);
    }
    task_free_msg (msg);
    (*con).prev = NULL;
    /*  Insert connection at beginning of list  */
    if (connections == NULL)
    {
	(*con).next = NULL;
    }
    else
    {
	(*con).next = connections;
	(*connections).prev = con;
    }
    connections = con;
    return (TRUE);
}   /*  End Function dock_input_func  */


/*  The following four routines emulate the  ch_alloc_port  and  r_alloc_port
    routines in the Karma library. These have been modified in order to
    allow more than one dock to be allocated by the process.
*/

static Channel *alloc_ch_port (port_number, retries, num_docks)
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
    static char function_name[] = "alloc_ch_port";

    if ( (port_number == NULL) || (num_docks == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    if ( ( docks = alloc_raw_port (port_number, retries, num_docks) )
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
	    (void) close (docks[dock_count]);
	}
	m_free ( (char *) docks );
	return (NULL);
    }
    /*  Create a channel object for each dock  */
    for (dock_count = 0; dock_count < *num_docks; ++dock_count)
    {
	if ( ( ch_docks[dock_count] =
	      ch_attach_to_asynchronous_descriptor (docks[dock_count]) )
	      == NULL )
	{
	    /*  Close docks  */
	    for (; dock_count > 0; --dock_count)
	    {
		(void) ch_close (ch_docks[dock_count - 1]);
		(void) close (docks[dock_count - 1]);
	    }
	    m_free ( (char *) ch_docks );
	    m_free ( (char *) docks );
	    return (NULL);
	}
    }
    m_free ( (char *) docks );
    return (ch_docks);
}   /*  End Function alloc_ch_port  */

static int *alloc_raw_port (port_number, retries, num_docks)
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
    unsigned int allocated_port_number;
    int sock_opt = SO_REUSEADDR;
    unsigned int retry_number;
    struct sockaddr_un un_addr;
    struct sockaddr_in in_addr;
    int *docks = NULL;
    struct servent *service_entry;
    extern int tcp_port_offset;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "alloc_raw_port";

    if ( ( docks = (int *) m_alloc (sizeof *docks * NUM_DOCKS) ) == NULL )
    {
	m_abort (function_name, "array of docks");
    }
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
    /*  Create Internet domain socket  */
    if ( ( docks[INDEX_INTERNET_DOCK] = socket (AF_INET, SOCK_STREAM, 0) )
	< 0 )
    {
	(void) fprintf (stderr, "Error creating Internet socket\t%s\n",
			sys_errlist[errno]);
	myexit (RV_SYS_ERROR);
    }
    /*  Set socket options  */
#  ifdef TCP_NODELAY
    sock_opt |= TCP_NODELAY;
    (void) fprintf (stderr, "TCP_NODELAY\n");
#  endif
    /*  The following code exercises a bug in the Convex OS kernal.
	This bug causes the system to crash. DO NOT USE until kernal patched.
    if (setsockopt (docks[INDEX_INTERNET_DOCK], SOL_SOCKET, sock_opt,
        (caddr_t) 0, 0)
	!= 0)
    {
	(void) fprintf (stderr,
	                "Error setting Internet socket options\t%s\n",
			sys_errlist[errno]);
	myexit (RV_SYS_ERROR);
    }
    /*  Set close-on-exec flag  */
    if (fcntl (docks[INDEX_INTERNET_DOCK], F_SETFD, 1) == -1)
    {
	(void) fprintf (stderr,
			"Error setting close-on-exec flag for Internet socket\t%s\n",
			sys_errlist[errno]);
	if (close (docks[INDEX_INTERNET_DOCK]) != 0)
	{
	    (void) fprintf (stderr,
			    "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	}
	myexit (RV_SYS_ERROR);
    }
    in_addr.sin_family = AF_INET;
    in_addr.sin_addr.s_addr = INADDR_ANY;
    /*  Try to bind to Internet/ TCP port  */
    for (retry_number = 0, bound = FALSE, allocated_port_number = *port_number;
	 (retry_number <= retries) && (bound == FALSE);
	 ++retry_number)
    {
	/*  Try to bind to port number  */
	in_addr.sin_port = htons (allocated_port_number + tcp_port_offset);
	if (get_child_of_port (allocated_port_number) != NULL)
	{
	    ++allocated_port_number;
	}
	else if (bind (docks[INDEX_INTERNET_DOCK],
		       (struct sockaddr *) &in_addr,
		       (int) sizeof in_addr) != 0)
	{
	    /*  Could not bind to port number  */
	    if (errno != EADDRINUSE)
	    {
		(void) fprintf (stderr,
				"Error binding Internet socket\t%s\n",
				sys_errlist[errno]);
		if (close (docks[INDEX_INTERNET_DOCK]) != 0)
		{
		    (void) fprintf (stderr,
				    "Error closing Internet socket\t%s\n",
				    sys_errlist[errno]);
		}
		myexit (RV_SYS_ERROR);
	    }
	    /*  Port already in use: go to next one  */
	    ++allocated_port_number;
	}
	else
	{
	    /*  Managed to bind  */
	    bound = TRUE;
	}
    }
    if (bound != TRUE)
    {
	/*  All slots occupied  */
	if (close (docks[INDEX_INTERNET_DOCK]) != 0)
	{
	    (void) fprintf (stderr, "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	    myexit (RV_SYS_ERROR);
	}
	m_free ( (char *) docks );
	return (NULL);
    }
    if (listen (docks[INDEX_INTERNET_DOCK], 2) != 0)
    {
	(void) fprintf (stderr, "Error listening to Internet dock\t%s\n",
			sys_errlist[errno]);
	if (close (docks[INDEX_INTERNET_DOCK]) != 0)
	{
	    (void) fprintf (stderr, "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	}
	myexit (RV_SYS_ERROR);
    }
    /*  Create Unix domain socket  */
    if ( ( docks[INDEX_UNIX_DOCK] = socket (AF_UNIX, SOCK_STREAM, 0) ) < 0 )
    {
	(void) fprintf (stderr, "Error creating Unix socket\t%s\n",
			sys_errlist[errno]);
	if (close (docks[INDEX_INTERNET_DOCK]) != 0)
	{
	    (void) fprintf (stderr, "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	}
	myexit (RV_SYS_ERROR);
    }
    /*  Set close-on-exec flag  */
    if (fcntl (docks[INDEX_UNIX_DOCK], F_SETFD, 1) == -1)
    {
	(void) fprintf (stderr,
			"Error setting close-on-exec flag for Unix socket\t%s\n",
			sys_errlist[errno]);
	if (close (docks[INDEX_UNIX_DOCK]) != 0)
	{
	    (void) fprintf (stderr,
			    "Error closing Unix socket\t%s\n",
			    sys_errlist[errno]);
	}
	if (close (docks[INDEX_INTERNET_DOCK]) != 0)
	{
	    (void) fprintf (stderr, "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	}
	myexit (RV_SYS_ERROR);
    }
    un_addr.sun_family = AF_UNIX;
    (void) sprintf (un_addr.sun_path, "%s/%s%d", UNIX_SOCKET_DIR,
		    UNIX_SOCKET_FILE, allocated_port_number);
    (void) mkdir (UNIX_SOCKET_DIR, (mode_t) 0777);
    (void) chmod (UNIX_SOCKET_DIR, (mode_t) 0777);
    (void) unlink (un_addr.sun_path);
    /*  Try to bind to Unix port  */
    if (bind (docks[INDEX_UNIX_DOCK], (struct sockaddr *) &un_addr,
	      (int) sizeof un_addr) != 0)
    {
	/*  Could not bind to port number  */
	(void) fprintf (stderr, "Error binding Unix socket\t%s\n",
			sys_errlist[errno]);
	if (close (docks[INDEX_INTERNET_DOCK]) != 0)
	{
	    (void) fprintf (stderr,
			    "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	}
	if (close (docks[INDEX_UNIX_DOCK]) != 0)
	{
	    (void) fprintf (stderr,
			    "Error closing Unix socket\t%s\n",
			    sys_errlist[errno]);
	}
	myexit (RV_SYS_ERROR);
    }
    if (listen (docks[INDEX_UNIX_DOCK], 2) != 0)
    {
	(void) fprintf (stderr, "Error listening to Unix dock\t%s\n",
			sys_errlist[errno]);
	if (close (docks[INDEX_INTERNET_DOCK]) != 0)
	{
	    (void) fprintf (stderr,
			    "Error closing Internet socket\t%s\n",
			    sys_errlist[errno]);
	}
	if (close (docks[INDEX_UNIX_DOCK]) != 0)
	{
	    (void) fprintf (stderr,
			    "Error closing Unix socket\t%s\n",
			    sys_errlist[errno]);
	}
	myexit (RV_SYS_ERROR);
    }
    *port_number = allocated_port_number;
    *num_docks = NUM_DOCKS;
    return (docks);
}   /*  End Function alloc_raw_port  */

static Channel accept_on_dock (doc_con, addr, local)
/*  This routine will open an asynchronous channel to the first connection to a
    waiting dock.
    The dock must be pointed to by  doc_con  .
    The address of the host connecting to the dock will be written to the
    storage pointed to by  addr  .
    If the connection comes from a local machine, the value TRUE will be
    written to the storage pointed to by  local  ,else the value FALSE will be
    written here.
    The routine returns a channel object on success, else it returns NULL.
*/
pseudo_connection *doc_con;
unsigned long *addr;
flag *local;
{
    Channel channel;
    Channel dock;
    unsigned int dock_index;
    unsigned int dock_count;
    vxchild *child;
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
    static char function_name[] = "accept_on_dock";

    dock = (*doc_con).channel;
    child = (*doc_con).child;
    /*  Find dock index of child  */
    for (dock_count = 0, dock_index = NUM_DOCKS; dock_count < NUM_DOCKS;
	 ++dock_count)
    {
	if (doc_con == (*child).docks[dock_count])
	{
	    dock_index = dock_count;
	}
    }
    switch (dock_index)
    {
      case INDEX_UNIX_DOCK:
	un_addr_len = sizeof un_addr;
	(void) bzero ( (char *) &un_addr, un_addr_len );
	/*  Listen for connections  */
	if ( ( fd = accept (ch_get_descriptor (dock),
			    (struct sockaddr *) &un_addr, &un_addr_len) )
	    < 0 )
	{
	    (void) fprintf (stderr, "Error accepting unix connection\t%s\n",
			    sys_errlist[errno]);
	    myexit (RV_SYS_ERROR);
	}
	*addr = r_get_inet_addr_from_host ( (char *) NULL, (flag *) NULL );
	break;
      case INDEX_INTERNET_DOCK:
	in_addr_len = sizeof in_addr;
	(void) bzero ( (char *) &in_addr, in_addr_len );
	/*  Listen for connections  */
	if ( ( fd = accept (ch_get_descriptor (dock),
			    (struct sockaddr *) &in_addr, &in_addr_len) )
	    < 0 )
	{
	    (void) fprintf (stderr, "Error accepting connection\t%s\t\n",
			    sys_errlist[errno]);
	    myexit (RV_SYS_ERROR);
	}
	*addr = in_addr.sin_addr.s_addr;
	break;
      default:
	(void) fprintf (stderr, "Unknown dock type\n");
	a_prog_bug (function_name);
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
	return (NULL);
    }
    /*  Make connection into channel  */
    if ( ( channel = ch_attach_to_asynchronous_descriptor (fd) ) == NULL )
    {
	m_abort (function_name, "asnchronous channel");
    }
    return (channel);
}   /*  End Function accept_on_dock  */

static Channel open_connection (host_addr, port_number)
/*  This routine will open an asychronous channel to a server running on
    another host machine.
    The Internet address of the host machine must be given by  host_addr  .
    If this is 0 the connection is made to a server running on the same machine
    using the most efficient transport available.
    The port number to connect to must be given by  port_number  .This should
    not be confused with Internet port numbers.
    The routine returns a channel object on success, else it returns NULL.
*/
unsigned long host_addr;
unsigned int port_number;
{
    Channel channel;
    int sock;
    struct sockaddr_un un_addr;
    struct sockaddr_in in_addr;
    struct servent *service_entry;
    extern int tcp_port_offset;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "open_connection";

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
    if (host_addr == 0)
    {
	/*  Unix domain socket/ connection  */
	/*  Create socket  */
	if ( ( sock = socket (AF_UNIX, SOCK_STREAM, 0) ) < 0 )
	{
	    (void) fprintf (stderr, "Error creating socket\t%s\n",
			    sys_errlist[errno]);
	    myexit (RV_SYS_ERROR);
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
	    myexit (RV_SYS_ERROR);
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
		myexit (RV_SYS_ERROR);
	    }
	    if (close (sock) != 0)
	    {
		(void) fprintf (stderr,
				"Error closing unix socket: %d\t%s\n",
				sock, sys_errlist[errno]);
	    }
	    return (NULL);
	}
    }
    else
    {
	/*  Internet/ TCP domain socket/ connection  */
	/*  Create socket  */
	if ( ( sock = socket (AF_INET, SOCK_STREAM, 0) ) < 0 )
	{
	    (void) fprintf (stderr, "Error creating socket\t%s\n",
			    sys_errlist[errno]);
	    myexit (RV_SYS_ERROR);
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
	    myexit (RV_SYS_ERROR);
	}
	/*  Set up addressing info  */
	in_addr.sin_family = AF_INET;
	in_addr.sin_port = htons (port_number + tcp_port_offset);
	in_addr.sin_addr.s_addr = htonl (host_addr);
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
		myexit (RV_SYS_ERROR);
	    }
	    if (close (sock) != 0)
	    {
		(void) fprintf (stderr, "Error closing Internet socket\t%s\n",
				sys_errlist[errno]);
	    }
	    return (NULL);
	}
    }
    /*  Make connection into channel  */
    if ( ( channel = ch_attach_to_asynchronous_descriptor (sock) ) == NULL )
    {
	m_abort (function_name, "asnchronous channel");
    }
    return (channel);
}   /*  End Function open_connection  */

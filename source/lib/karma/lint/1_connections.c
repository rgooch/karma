#include <karma_r.h>
# 1 "" 
int *r_alloc_port (port_number, retries, num_docks)
unsigned int *port_number;
unsigned int retries;
unsigned int *num_docks;
{
    return ( (int *) 0 );
}
void r_close_dock (dock)
int dock;
{
}
int r_connect_to_port (addr, port_number, local)
unsigned long addr;
unsigned int port_number;
flag *local;
{
    return ( (int) 0 );
}
int r_accept_connection_on_dock (dock, addr, local)
int dock;
unsigned long *addr;
flag *local;
{
    return ( (int) 0 );
}
flag r_close_connection (connection)
int connection;
{
    return ( (flag) 0 );
}
int r_get_bytes_readable (connection)
int connection;
{
    return ( (int) 0 );
}
unsigned long r_get_inet_addr_from_host (host, local)
char *host;
flag *local;
{
    return ( (unsigned long) 0 );
}
int r_read (fd, buf, nbytes)
int fd;
char *buf;
int nbytes;
{
    return ( (int) 0 );
}
int r_write (fd, buf, nbytes)
int fd;
char *buf;
int nbytes;
{
    return ( (int) 0 );
}
flag r_test_input_event (connection)
int connection;
{
    return ( (flag) 0 );
}
int r_open_stdin (disc)
flag *disc;
{
    return ( (int) 0 );
}
char *r_getenv (name)
char *name;
{
    return ( (char *) 0 );
}
int r_setenv (env_name, env_value)
char *env_name;
char *env_value;
{
    return ( (int) 0 );
}
void r_gethostname (name, namelen)
char *name;
unsigned int namelen;
{
}
int r_getppid ()
{
    return ( (int) 0 );
}

#include <karma_ch.h>
# 1 "" 
Channel ch_open_file (filename, type)
char *filename;
char *type;
{
    return ( (Channel) 0 );
}
Channel ch_map_disc (filename, option, writeable, update_on_write)
char *filename;
unsigned int option;
flag writeable;
flag update_on_write;
{
    return ( (Channel) 0 );
}
Channel ch_open_connection (host_addr, port_number)
unsigned long host_addr;
unsigned int port_number;
{
    return ( (Channel) 0 );
}
Channel ch_open_memory (buffer, size)
char *buffer;
unsigned int size;
{
    return ( (Channel) 0 );
}
Channel ch_accept_on_dock (dock, addr)
Channel dock;
unsigned long *addr;
{
    return ( (Channel) 0 );
}
Channel *ch_alloc_port (port_number, retries, num_docks)
unsigned int *port_number;
unsigned int retries;
unsigned int *num_docks;
{
    return ( (Channel *) 0 );
}
flag ch_close (channel)
Channel channel;
{
    return ( (flag) 0 );
}
flag ch_flush (channel)
Channel channel;
{
    return ( (flag) 0 );
}
unsigned int ch_read (channel, buffer, length)
Channel channel;
char *buffer;
unsigned int length;
{
    return ( (unsigned int) 0 );
}
unsigned int ch_write (channel, buffer, length)
Channel channel;
char *buffer;
unsigned int length;
{
    return ( (unsigned int) 0 );
}
void ch_close_all_channels ()
{
}
flag ch_seek (channel, position)
Channel channel;
unsigned long position;
{
    return ( (flag) 0 );
}
int ch_get_bytes_readable (channel)
Channel channel;
{
    return ( (int) 0 );
}
int ch_get_descriptor (channel)
Channel channel;
{
    return ( (int) 0 );
}
void ch_open_stdin ()
{
}
flag ch_test_for_io (channel)
Channel channel;
{
    return ( (flag) 0 );
}
flag ch_test_for_asynchronous (channel)
Channel channel;
{
    return ( (flag) 0 );
}
flag ch_test_for_connection (channel)
Channel channel;
{
    return ( (flag) 0 );
}
flag ch_test_for_local_connection (channel)
Channel channel;
{
    return ( (flag) 0 );
}
Channel ch_attach_to_asynchronous_descriptor (fd)
int fd;
{
    return ( (Channel) 0 );
}
flag ch_test_for_mmap (channel)
Channel channel;
{
    return ( (flag) 0 );
}
flag ch_tell (channel, read_pos, write_pos)
Channel channel;
unsigned long *read_pos;
unsigned long *write_pos;
{
    return ( (flag) 0 );
}
char *ch_get_mmap_addr (channel)
Channel channel;
{
    return ( (char *) 0 );
}
unsigned int ch_get_mmap_access_count (channel)
Channel channel;
{
    return ( (unsigned int) 0 );
}

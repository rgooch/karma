#include <karma_conn.h>
# 1 "" 
void conn_register_managers (manage_func, unmanage_func, exit_schedule_func)
flag (*manage_func) ();
void (*unmanage_func) ();
void (*exit_schedule_func) ();
{
}
void conn_register_server_protocol (protocol_name, version, max_connections,
				    open_func, read_func, close_func)
char *protocol_name;
unsigned int version;
unsigned int max_connections;
flag (*open_func) ();
flag (*read_func) ();
void (*close_func) ();
{
}
void conn_register_client_protocol (protocol_name, version, max_connections,
				    validate_func,
				    open_func, read_func, close_func)
char *protocol_name;
unsigned int version;
unsigned int max_connections;
flag (*validate_func) ();
flag (*open_func) ();
flag (*read_func) ();
void (*close_func) ();
{
}
Channel conn_get_channel (connection)
Connection connection;
{
    return ( (Channel) 0 );
}
flag conn_attempt_connection (hostname, port_number, protocol_name)
char *hostname;
unsigned int port_number;
char *protocol_name;
{
    return ( (flag) 0 );
}
flag conn_close (connection)
Connection connection;
{
    return ( (flag) 0 );
}
flag conn_become_server (port_number, retries)
unsigned int *port_number;
unsigned int retries;
{
    return ( (flag) 0 );
}
unsigned int conn_get_num_serv_connections (protocol_name)
char *protocol_name;
{
    return ( (unsigned int) 0 );
}
unsigned int conn_get_num_client_connections (protocol_name)
char *protocol_name;
{
    return ( (unsigned int) 0 );
}
Connection conn_get_serv_connection (protocol_name, number)
char *protocol_name;
unsigned int number;
{
    return ( (Connection) 0 );
}
Connection conn_get_client_connection (protocol_name, number)
char *protocol_name;
unsigned int number;
{
    return ( (Connection) 0 );
}
void *conn_get_connection_info (connection)
Connection connection;
{
    return ( (void *) 0 );
}
flag conn_controlled_by_cm_tool ()
{
    return ( (flag) 0 );
}
char *conn_get_connection_module_name (connection)
Connection connection;
{
    return ( (char *) 0 );
}

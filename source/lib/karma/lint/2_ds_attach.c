#include <karma_ds.h>
# 1 "" 
flag ds_put_unique_named_value (pack_desc, packet, name, type, value, update)
packet_desc *pack_desc;
char **packet;
char *name;
unsigned int type;
double *value;
flag update;
{
    return ( (flag) 0 );
}
flag ds_put_unique_named_string (pack_desc, packet, name, string, update)
packet_desc *pack_desc;
char **packet;
char *name;
char *string;
flag update;
{
    return ( (flag) 0 );
}
flag ds_get_unique_named_value (pack_desc, packet, name, type, value)
packet_desc *pack_desc;
char *packet;
char *name;
unsigned int *type;
double *value;
{
    return ( (flag) 0 );
}
char *ds_get_unique_named_string (pack_desc, packet, name)
packet_desc *pack_desc;
char *packet;
char *name;
{
    return ( (char *) 0 );
}

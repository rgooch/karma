#include <karma_dsra.h>
# 1 "" 
multi_array *dsra_multi_desc (channel)
Channel channel;
{
    return ( (multi_array *) 0 );
}
packet_desc *dsra_packet_desc (channel)
Channel channel;
{
    return ( (packet_desc *) 0 );
}
flag dsra_element_desc (channel, type, name)
Channel channel;
unsigned int *type;
char name[];
{
    return ( (flag) 0 );
}
array_desc *dsra_array_desc (channel, type)
Channel channel;
unsigned int type;
{
    return ( (array_desc *) 0 );
}
dim_desc *dsra_dim_desc (channel)
Channel channel;
{
    return ( (dim_desc *) 0 );
}
flag dsra_multi_data (channel, multi_desc)
Channel channel;
multi_array *multi_desc;
{
    return ( (flag) 0 );
}
flag dsra_packet (channel, descriptor, packet)
Channel channel;
packet_desc *descriptor;
char *packet;
{
    return ( (flag) 0 );
}
flag dsra_element (channel, type, desc, element)
Channel channel;
unsigned int type;
char *desc;
char *element;
{
    return ( (flag) 0 );
}
flag dsra_array (channel, descriptor, array)
Channel channel;
array_desc *descriptor;
char *array;
{
    return ( (flag) 0 );
}
flag dsra_list (channel, descriptor, header)
Channel channel;
packet_desc *descriptor;
list_header *header;
{
    return ( (flag) 0 );
}
flag dsra_flag (channel, logical)
Channel channel;
flag *logical;
{
    return ( (flag) 0 );
}
flag dsra_type (channel, type)
Channel channel;
unsigned int *type;
{
    return ( (flag) 0 );
}
flag dsra_uint (channel, value)
Channel channel;
unsigned int *value;
{
    return ( (flag) 0 );
}
flag dsra_int (channel, value)
Channel channel;
int *value;
{
    return ( (flag) 0 );
}
flag dsra_float (channel, value)
Channel channel;
float *value;
{
    return ( (flag) 0 );
}
flag dsra_double (channel, value)
Channel channel;
double *value;
{
    return ( (flag) 0 );
}

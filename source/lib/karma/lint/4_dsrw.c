#include <karma_dsrw.h>
# 1 "" 
void dsrw_write_multi (channel, multi_desc)
Channel channel;
multi_array *multi_desc;
{
}
void dsrw_write_packet_desc (channel, pack_desc)
Channel channel;
packet_desc *pack_desc;
{
}
void dsrw_write_element_desc (channel, type, desc)
Channel channel;
unsigned int type;
char *desc;
{
}
void dsrw_write_array_desc (channel, arr_desc)
Channel channel;
array_desc *arr_desc;
{
}
void dsrw_write_dim_desc (channel, dimension)
Channel channel;
dim_desc *dimension;
{
}
void dsrw_write_packet (channel, pack_desc, packet)
Channel channel;
packet_desc *pack_desc;
char *packet;
{
}
void dsrw_write_element (channel, type, desc, element)
Channel channel;
unsigned int type;
char *desc;
char *element;
{
}
void dsrw_write_array (channel, arr_desc, element, pad)
Channel channel;
array_desc *arr_desc;
char *element;
flag pad;
{
}
void dsrw_write_list (channel, pack_desc, list_head)
Channel channel;
packet_desc *pack_desc;
list_header *list_head;
{
}
void dsrw_write_flag (channel, logical)
Channel channel;
flag logical;
{
}
multi_array *dsrw_read_multi (channel)
Channel channel;
{
    return ( (multi_array *) 0 );
}
packet_desc *dsrw_read_packet_desc (channel)
Channel channel;
{
    return ( (packet_desc *) 0 );
}
array_desc *dsrw_read_array_desc (channel, type)
Channel channel;
unsigned int type;
{
    return ( (array_desc *) 0 );
}
dim_desc *dsrw_read_dim_desc (channel)
Channel channel;
{
    return ( (dim_desc *) 0 );
}
flag dsrw_read_packet (channel, descriptor, packet)
Channel channel;
packet_desc *descriptor;
char *packet;
{
    return ( (flag) 0 );
}
flag dsrw_read_element (channel, type, desc, element)
Channel channel;
unsigned int type;
char *desc;
char *element;
{
    return ( (flag) 0 );
}
flag dsrw_read_array (channel, descriptor, element, pad)
Channel channel;
array_desc *descriptor;
char *element;
flag pad;
{
    return ( (flag) 0 );
}
flag dsrw_read_list (channel, descriptor, header)
Channel channel;
packet_desc *descriptor;
list_header *header;
{
    return ( (flag) 0 );
}
flag dsrw_read_flag (channel, logical)
Channel channel;
flag *logical;
{
    return ( (flag) 0 );
}
flag dsrw_read_type (channel, type)
Channel channel;
unsigned int *type;
{
    return ( (flag) 0 );
}

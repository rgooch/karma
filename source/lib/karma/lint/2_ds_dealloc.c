#include <karma_ds.h>
# 1 "" 
void ds_dealloc_multi (multi_desc)
multi_array *multi_desc;
{
}
void ds_dealloc_packet (pack_desc, data)
packet_desc *pack_desc;
char *data;
{
}
void ds_dealloc_data (pack_desc, packet)
packet_desc *pack_desc;
char *packet;
{
}
void ds_dealloc_packet_subdata (pack_desc, packet)
packet_desc *pack_desc;
char *packet;
{
}
void ds_dealloc_array_desc (arr_desc)
array_desc *arr_desc;
{
}
void ds_dealloc_list (list_desc, list_head)
packet_desc *list_desc;
list_header *list_head;
{
}
void ds_dealloc_list_entries (list_desc, list_head)
packet_desc *list_desc;
list_header *list_head;
{
}
void ds_dealloc2_list (list_head)
list_header *list_head;
{
}
void ds_dealloc_array (arr_desc, arr_element)
array_desc *arr_desc;
char *arr_element;
{
}

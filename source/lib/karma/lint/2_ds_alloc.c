#include <karma_ds.h>
# 1 "" 
multi_array *ds_alloc_multi (num_arrays)
unsigned int num_arrays;
{
    return ( (multi_array *) 0 );
}
packet_desc *ds_alloc_packet_desc (num_elem)
unsigned int num_elem;
{
    return ( (packet_desc *) 0 );
}
char *ds_alloc_data (pack_desc, clear, array_alloc)
packet_desc *pack_desc;
flag clear;
flag array_alloc;
{
    return ( (char *) 0 );
}
flag ds_alloc_packet_subdata (pack_desc, packet, clear, array_alloc)
packet_desc *pack_desc;
char *packet;
flag clear;
flag array_alloc;
{
    return ( (flag) 0 );
}
char *ds_alloc_packet (pack_descriptor)
packet_desc *pack_descriptor;
{
    return ( (char *) 0 );
}
array_desc *ds_alloc_array_desc (num_dimensions, num_levels)
unsigned int num_dimensions;
unsigned int num_levels;
{
    return ( (array_desc *) 0 );
}
flag ds_alloc_tiling_info (arr_desc, num_levels)
array_desc *arr_desc;
unsigned int num_levels;
{
    return ( (flag) 0 );
}
dim_desc *ds_alloc_dim_desc (dim_name, length, min, max, regular)
char *dim_name;
unsigned int length;
double min;
double max;
flag regular;
{
    return ( (dim_desc *) 0 );
}
list_header *ds_alloc_list_head ()
{
    return ( (list_header *) 0 );
}
list_entry *ds_alloc_list_entry (list_desc, array_alloc)
packet_desc *list_desc;
flag array_alloc;
{
    return ( (list_entry *) 0 );
}
flag ds_alloc_array (arr_desc, element, clear, array_alloc)
array_desc *arr_desc;
char *element;
flag clear;
flag array_alloc;
{
    return ( (flag) 0 );
}
char *ds_easy_alloc_array (multi_desc, num_dim, lengths, minima, maxima, names,
			   data_type, data_name)
multi_array **multi_desc;
unsigned int num_dim;
unsigned int *lengths;
double *minima;
double *maxima;
char **names;
unsigned int data_type;
char *data_name;
{
    return ( (char *) 0 );
}
flag ds_alloc_contiguous_list (list_desc, list_head, length, clear,array_alloc)
packet_desc *list_desc;
list_header *list_head;
unsigned int length;
flag clear;
flag array_alloc;
{
    return ( (flag) 0 );
}

#include <karma_ds.h>
# 1 "" 
flag ds_copy_packet (pack_desc, dest_packet, source_packet)
packet_desc *pack_desc;
char *dest_packet;
char *source_packet;
{
    return ( (flag) 0 );
}
packet_desc *ds_copy_desc_until (inp_desc, name)
packet_desc *inp_desc;
char *name;
{
    return ( (packet_desc *) 0 );
}
array_desc *ds_copy_array_desc_until (inp_desc, name)
array_desc *inp_desc;
char *name;
{
    return ( (array_desc *) 0 );
}
dim_desc *ds_copy_dim_desc (inp_desc)
dim_desc *inp_desc;
{
    return ( (dim_desc *) 0 );
}
flag ds_copy_data (inp_desc, inp_data, out_desc, out_data)
packet_desc *inp_desc;
char *inp_data;
packet_desc *out_desc;
char *out_data;
{
    return ( (flag) 0 );
}
flag ds_copy_array (inp_desc, inp_data, out_desc, out_data)
array_desc *inp_desc;
char *inp_data;
array_desc *out_desc;
char *out_data;
{
    return ( (flag) 0 );
}
flag ds_copy_list (inp_desc, inp_head, out_desc, out_head)
packet_desc *inp_desc;
list_header *inp_head;
packet_desc *out_desc;
list_header *out_head;
{
    return ( (flag) 0 );
}
multi_array *ds_select_arrays (array_list, num_in_list, multi_desc,
				      save_unproc, index_list)
char **array_list;
unsigned int num_in_list;
multi_array *multi_desc;
flag save_unproc;
unsigned int **index_list;
{
    return ( (multi_array *) 0 );
}

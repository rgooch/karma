#include <karma_ds.h>
# 1 "" 
flag ds_remove_dim_desc (arr_desc, dim_name)
array_desc *arr_desc;
char *dim_name;
{
    return ( (flag) 0 );
}
flag ds_append_dim_desc (arr_desc, dimension)
array_desc *arr_desc;
dim_desc *dimension;
{
    return ( (flag) 0 );
}
flag ds_prepend_dim_desc (arr_desc, dimension)
array_desc *arr_desc;
dim_desc *dimension;
{
    return ( (flag) 0 );
}
flag ds_compute_array_offsets (arr_desc)
array_desc *arr_desc;
{
    return ( (flag) 0 );
}
void ds_remove_tiling_info (arr_desc)
array_desc *arr_desc;
{
}
flag ds_append_gen_struct (multi_desc, pack_desc, packet, existing_arrayname,
			   append_arrayname)
multi_array *multi_desc;
packet_desc *pack_desc;
char *packet;
char *existing_arrayname;
char *append_arrayname;
{
    return ( (flag) 0 );
}

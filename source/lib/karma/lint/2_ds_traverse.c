#include <karma_ds.h>
# 1 "" 
flag ds_reorder_array (arr_desc, order_list, array, mod_desc)
array_desc *arr_desc;
unsigned int order_list[];
char *array;
flag mod_desc;
{
    return ( (flag) 0 );
}
flag ds_foreach_occurrence (pack_desc, packet, item, as_whole, function)
packet_desc *pack_desc;
char *packet;
char *item;
flag as_whole;
flag (*function) ();
{
    return ( (flag) 0 );
}
flag ds_foreach_in_array (arr_desc, array, item, as_whole, function)
array_desc *arr_desc;
char *array;
char *item;
flag as_whole;
flag (*function) ();
{
    return ( (flag) 0 );
}
flag ds_foreach_in_list (list_desc, list_head, item, as_whole, function)
packet_desc *list_desc;
list_header *list_head;
char *item;
flag as_whole;
flag (*function) ();
{
    return ( (flag) 0 );
}
flag ds_traverse_and_process (inp_desc, inp_data, out_desc, out_data, as_whole,
			      function)
packet_desc *inp_desc;
char *inp_data;
packet_desc *out_desc;
char *out_data;
flag as_whole;
flag (*function) ();
{
    return ( (flag) 0 );
}
flag ds_traverse_array (inp_desc, inp_data, out_desc, out_data, as_whole,
			function)
array_desc *inp_desc;
char *inp_data;
array_desc *out_desc;
char *out_data;
flag as_whole;
flag (*function) ();
{
    return ( (flag) 0 );
}
flag ds_traverse_list (inp_desc, inp_head, out_desc, out_head, as_whole,
		       function)
packet_desc *inp_desc;
list_header *inp_head;
packet_desc *out_desc;
list_header *out_head;
flag as_whole;
flag (*function) ();
{
    return ( (flag) 0 );
}

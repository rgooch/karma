#include <karma_ds.h>
# 1 "" 
unsigned int ds_get_handle_in_packet (pack_desc, packet, item_name,
				      restr_names, restr_values, num_restr,
				      parent_desc, parent, parent_type, index)
packet_desc *pack_desc;
char *packet;
char *item_name;
char *restr_names[];
double *restr_values;
unsigned int num_restr;
char **parent_desc;
char **parent;
unsigned int *parent_type;
unsigned int *index;
{
    return ( (unsigned int) 0 );
}
unsigned int ds_get_handle_in_array (arr_desc, array, item_name,
				     restr_names, restr_values, num_restr,
				     parent_desc, parent, parent_type, index)
array_desc *arr_desc;
char *array;
char *item_name;
char *restr_names[];
double *restr_values;
unsigned int num_restr;
char **parent_desc;
char **parent;
unsigned int *parent_type;
unsigned int *index;
{
    return ( (unsigned int) 0 );
}
unsigned int ds_get_handle_in_list (list_desc, list_head, item_name,
				    restr_names, restr_values, num_restr,
				    parent_desc, parent, parent_type, index)
packet_desc *list_desc;
list_header *list_head;
char *item_name;
char *restr_names[];
double *restr_values;
unsigned int num_restr;
char **parent_desc;
char **parent;
unsigned int *parent_type;
unsigned int *index;
{
    return ( (unsigned int) 0 );
}

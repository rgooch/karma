#include <karma_iedit.h>
# 1 "" 
packet_desc *iedit_create_desc ()
{
    return ( (packet_desc *) 0 );
}
list_entry *iedit_create_generic_instruction (edit_list_desc,
					      instruction_code,
					      coords, num_coords, intensity)
packet_desc *edit_list_desc;
unsigned int instruction_code;
edit_coord *coords;
unsigned int num_coords;
double intensity[2];
{
    return ( (list_entry *) 0 );
}
edit_coord *iedit_alloc_edit_coords (num_coords)
unsigned int num_coords;
{
    return ( (edit_coord *) 0 );
}
flag iedit_get_edit_coords (list_desc, list_head, coords)
packet_desc *list_desc;
list_header *list_head;
edit_coord **coords;
{
    return ( (flag) 0 );
}

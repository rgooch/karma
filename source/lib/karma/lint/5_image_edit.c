#include <karma_iedit.h>
# 1 "" 
KImageEditList iedit_create_list (add_func, loss_func, apply_func, info)
void (*add_func) ();
void (*loss_func) ();
void (*apply_func) ();
void *info;
{
    return ( (KImageEditList) 0 );
}
packet_desc *iedit_get_instruction_desc ()
{
    return ( (packet_desc *) 0 );
}
edit_coord *iedit_alloc_edit_coords (num_coords)
unsigned int num_coords;
{
    return ( (edit_coord *) 0 );
}
flag iedit_get_edit_coords (list_head, coords)
list_header *list_head;
edit_coord **coords;
{
    return ( (flag) 0 );
}
flag iedit_add_instruction (ilist, instruction_code, coords, num_coords,
			    intensity)
KImageEditList ilist;
unsigned int instruction_code;
edit_coord *coords;
unsigned int num_coords;
double intensity[2];
{
    return ( (flag) 0 );
}
flag iedit_remove_instructions (ilist, num_to_remove)
KImageEditList ilist;
unsigned int num_to_remove;
{
    return ( (flag) 0 );
}
flag iedit_apply_instructions (ilist)
KImageEditList ilist;
{
    return ( (flag) 0 );
}
list_header *iedit_get_list (ilist)
KImageEditList ilist;
{
    return ( (list_header *) 0 );
}
void iedit_make_list_default_master (ilist)
KImageEditList ilist;
{
}
void iedit_make_list_default_slave (ilist)
KImageEditList ilist;
{
}

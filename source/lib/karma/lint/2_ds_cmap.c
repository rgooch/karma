#include <karma_ds.h>
# 1 "" 
unsigned short *ds_cmap_alloc_colourmap (size, multi_desc, pack_desc, packet)
unsigned int size;
multi_array **multi_desc;
packet_desc **pack_desc;
char **packet;
{
    return ( (unsigned short *) 0 );
}
unsigned short *ds_cmap_find_colourmap (top_pack_desc, top_packet, size,
					reordering_done,
					restr_names, restr_values, num_restr)
packet_desc *top_pack_desc;
char *top_packet;
unsigned int *size;
flag *reordering_done;
char *restr_names[];
double *restr_values;
unsigned int num_restr;
{
    return ( (unsigned short *) 0 );
}
unsigned int *ds_cmap_get_all_colourmaps (multi_desc, num_found,
					  reordering_done,
					  restr_names, restr_values, num_restr)
multi_array *multi_desc;
unsigned int *num_found;
flag *reordering_done;
char *restr_names[];
double *restr_values;
unsigned int num_restr;
{
    return ( (unsigned int *) 0 );
}

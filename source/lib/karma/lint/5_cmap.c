#include <karma_kcmap.h>
# 1 "" 
void kcmap_init (alloc_func, free_func, store_func, location_func)
unsigned int (*alloc_func) ();
void (*free_func) ();
void (*store_func) ();
void (*location_func) ();
{
}
void kcmap_add_RGB_func (name, func, min_cells, max_cells)
char *name;
void (*func) ();
unsigned int min_cells;
unsigned int max_cells;
{
}
Kcolourmap kcmap_create (name, num_cells, tolerant, dpy_handle)
char *name;
unsigned int num_cells;
flag tolerant;
Kdisplay dpy_handle;
{
    return ( (Kcolourmap) 0 );
}
void kcmap_register_resize_func (cmap, resize_func, info)
Kcolourmap cmap;
void (*resize_func) ();
void *info;
{
}
flag kcmap_change (cmap, new_name, num_cells, tolerant)
Kcolourmap cmap;
char *new_name;
unsigned int num_cells;
flag tolerant;
{
    return ( (flag) 0 );
}
void kcmap_modify (cmap, x, y, var_param)
Kcolourmap cmap;
double x;
double y;
void *var_param;
{
}
char **kcmap_list_funcs ()
{
    return ( (char **) 0 );
}
char *kcmap_get_active_func (cmap)
Kcolourmap cmap;
{
    return ( (char *) 0 );
}
unsigned int kcmap_get_pixels (cmap, pixel_values)
Kcolourmap cmap;
unsigned long **pixel_values;
{
    return ( (unsigned int) 0 );
}
unsigned long kcmap_get_pixel (cmap, index)
Kcolourmap cmap;
unsigned int index;
{
    return ( (unsigned long) 0 );
}
void kcmap_prepare_for_slavery (cmap)
Kcolourmap cmap;
{
}
flag kcmap_copy_to_struct (cmap, top_pack_desc, top_packet)
Kcolourmap cmap;
packet_desc **top_pack_desc;
char **top_packet;
{
    return ( (flag) 0 );
}
flag kcmap_copy_from_struct (cmap, top_pack_desc, top_packet)
Kcolourmap cmap;
packet_desc *top_pack_desc;
char *top_packet;
{
    return ( (flag) 0 );
}

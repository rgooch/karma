#include <karma_vc.h>
# 1 "" 
Kdisplay vc_get_dpy_handle ()
{
    return ( (Kdisplay) 0 );
}
unsigned int vc_alloc_colours (num_cells, pixel_values, min_cells, dpy_handle)
unsigned int num_cells;
unsigned long *pixel_values;
unsigned int min_cells;
Kdisplay dpy_handle;
{
    return ( (unsigned int) 0 );
}
void vc_free_colours (num_cells, pixel_values, dpy_handle)
unsigned int num_cells;
unsigned long *pixel_values;
Kdisplay dpy_handle;
{
}
void vc_store_colours (num_cells, pixel_values, reds, greens, blues, stride,
		       dpy_handle)
unsigned int num_cells;
unsigned long *pixel_values;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
Kdisplay dpy_handle;
{
}
void vc_store_colours_24bit (num_cells, pixel_values, reds, greens, blues,
			     stride, dpy_handle)
unsigned int num_cells;
unsigned long *pixel_values;
unsigned short *reds;
unsigned short *greens;
unsigned short *blues;
unsigned int stride;
Kdisplay dpy_handle;
{
}
void vc_get_location (dpy_handle, serv_hostaddr, serv_display_num)
Kdisplay dpy_handle;
unsigned long *serv_hostaddr;
unsigned long *serv_display_num;
{
}
void vc_set_visual (dpy_handle, pseudo_colour)
Kdisplay dpy_handle;
flag pseudo_colour;
{
}

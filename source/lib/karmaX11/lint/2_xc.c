#include <karma_xc.h>
# 1 "" 
Kdisplay xc_get_dpy_handle (display, cmap)
Display *display;
Colormap cmap;
{
    return ( (Kdisplay) 0 );
}
unsigned int xc_alloc_colours (num_cells, pixel_values, min_cells, dpy_handle)
unsigned int num_cells;
unsigned long *pixel_values;
unsigned int min_cells;
Kdisplay dpy_handle;
{
    return ( (unsigned int) 0 );
}
void xc_free_colours (num_cells, pixel_values, dpy_handle)
unsigned int num_cells;
unsigned long *pixel_values;
Kdisplay dpy_handle;
{
}
void xc_store_colours (num_cells, pixel_values, reds, greens, blues, stride,
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
void xc_get_location (dpy_handle, serv_hostaddr, serv_display_num)
Kdisplay dpy_handle;
unsigned long *serv_hostaddr;
unsigned long *serv_display_num;
{
}

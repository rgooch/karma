#include <karma_kwin.h>
# 1 "" 
KPixCanvas kwin_create_x (display, window, gc, xoff, yoff, width, height)
Display *display;
Window window;
GC gc;
int xoff;
int yoff;
int width;
int height;
{
    return ( (KPixCanvas) 0 );
}
void kwin_set_gc_x (canvas, gc)
KPixCanvas canvas;
GC gc;
{
}
KPixCanvas kwin_create_vx (pseudo_colour, xoff, yoff, width, height)
flag pseudo_colour;
int xoff;
int yoff;
int width;
int height;
{
    return ( (KPixCanvas) 0 );
}
void kwin_register_refresh_func (canvas, refresh_func, info)
KPixCanvas canvas;
void (*refresh_func) ();
void *info;
{
}
void kwin_register_position_event_func (canvas, position_func, f_info)
KPixCanvas canvas;
flag (*position_func) ();
void *f_info;
{
}
flag kwin_resize (canvas, clear, xoff, yoff, width, height)
KPixCanvas canvas;
flag clear;
int xoff;
int yoff;
int width;
int height;
{
    return ( (flag) 0 );
}
flag kwin_process_position_event (canvas, x, y, clip, event_code, event_info)
KPixCanvas canvas;
int x;
int y;
flag clip;
unsigned int event_code;
void *event_info;
{
    return ( (flag) 0 );
}
flag kwin_draw_image (canvas, arr_desc, slice, hdim, vdim, elem_index,
		      num_pixels, pixel_values, win_scale, cache_ptr)
KPixCanvas canvas;
array_desc *arr_desc;
char *slice;
unsigned int hdim;
unsigned int vdim;
unsigned int elem_index;
unsigned int num_pixels;
unsigned long *pixel_values;
struct win_scale_type *win_scale;
KPixCanvasImageCache *cache_ptr;
{
    return ( (flag) 0 );
}
flag kwin_draw_cached_image (cache, x_off, y_off)
KPixCanvasImageCache cache;
int x_off;
int y_off;
{
    return ( (flag) 0 );
}
void kwin_draw_point (canvas, x, y, pixel_value)
KPixCanvas canvas;
int x;
int y;
unsigned long pixel_value;
{
}
void kwin_draw_line (canvas, x0, y0, x1, y1, pixel_value)
KPixCanvas canvas;
int x0;
int y0;
int x1;
int y1;
unsigned long pixel_value;
{
}
void kwin_fill_ellipse (canvas, cx, cy, rx, ry, pixel_value)
KPixCanvas canvas;
int cx;
int cy;
int rx;
int ry;
unsigned long pixel_value;
{
}
flag kwin_fill_polygon (canvas, point_x, point_y, num_vertices, pixel_value,
			convex)
KPixCanvas canvas;
int *point_x;
int *point_y;
unsigned int num_vertices;
unsigned long pixel_value;
flag convex;
{
    return ( (flag) 0 );
}
void kwin_get_size (canvas, width, height)
KPixCanvas canvas;
int *width;
int *height;
{
}
void kwin_free_cache_data (cache)
KPixCanvasImageCache cache;
{
}
flag kwin_convert_to_canvas_coord (canvas, xin, yin, xout, yout)
KPixCanvas canvas;
int xin;
int yin;
int *xout;
int *yout;
{
    return ( (flag) 0 );
}

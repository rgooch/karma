#include <karma_canvas.h>
# 1 "" 
KWorldCanvas canvas_create (pixcanvas, cmap, win_scale)
KPixCanvas pixcanvas;
Kcolourmap cmap;
struct win_scale_type *win_scale;
{
    return ( (KWorldCanvas) 0 );
}
void canvas_register_refresh_func (canvas, refresh_func, info)
KWorldCanvas canvas;
void (*refresh_func) ();
void *info;
{
}
void canvas_register_size_control_func (canvas, size_control_func, info)
KWorldCanvas canvas;
void (*size_control_func) ();
void *info;
{
}
void canvas_register_position_event_func (canvas, position_func, f_info)
KWorldCanvas canvas;
flag (*position_func) ();
void *f_info;
{
}
flag canvas_resize (canvas, win_scale, always_clear)
KWorldCanvas canvas;
struct win_scale_type *win_scale;
flag always_clear;
{
    return ( (flag) 0 );
}
flag canvas_draw_image (canvas, arr_desc, slice, hdim, vdim, elem_index,
			cache_ptr)
KWorldCanvas canvas;
array_desc *arr_desc;
char *slice;
unsigned int hdim;
unsigned int vdim;
unsigned int elem_index;
KPixCanvasImageCache *cache_ptr;
{
    return ( (flag) 0 );
}
void canvas_draw_point (canvas, x, y, value)
KWorldCanvas canvas;
double x;
double y;
double value[2];
{
}
void canvas_draw_line (canvas, x0, y0, x1, y1, value)
KWorldCanvas canvas;
double x0;
double y0;
double x1;
double y1;
double value[2];
{
}
void canvas_get_size (canvas, width, height, win_scale)
KWorldCanvas canvas;
int *width;
int *height;
struct win_scale_type *win_scale;
{
}
flag canvas_convert_to_canvas_coord (canvas, xin, yin, xout, yout)
KWorldCanvas canvas;
int xin;
int yin;
double *xout;
double *yout;
{
    return ( (flag) 0 );
}
flag canvas_convert_from_canvas_coord (canvas, xin, yin, xout, yout)
KWorldCanvas canvas;
double xin;
double yin;
int *xout;
int *yout;
{
    return ( (flag) 0 );
}
void canvas_fill_ellipse (canvas, centre_x, centre_y, radius_x, radius_y,value)
KWorldCanvas canvas;
double centre_x;
double centre_y;
double radius_x;
double radius_y;
double value[2];
{
}
flag canvas_fill_polygon (canvas, coords, num_vertices, value, convex)
KWorldCanvas canvas;
edit_coord *coords;
unsigned int num_vertices;
double value[2];
flag convex;
{
    return ( (flag) 0 );
}

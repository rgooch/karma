#include <karma_viewimg.h>
# 1 "" 
ViewableImage viewimg_create (canvas, multi_desc, arr_desc, slice,
			      hdim, vdim, elem_index)
KWorldCanvas canvas;
multi_array *multi_desc;
array_desc *arr_desc;
char *slice;
unsigned int hdim;
unsigned int vdim;
unsigned int elem_index;
{
    return ( (ViewableImage) 0 );
}
ViewableImage viewimg_create_from_iarray (canvas, array, swap)
KWorldCanvas canvas;
iarray array;
flag swap;
{
    return ( (ViewableImage) 0 );
}
flag viewimg_make_active (vimage)
ViewableImage vimage;
{
    return ( (flag) 0 );
}
void viewimg_control_autoscaling (canvas, auto_x, auto_y, auto_v, int_x, int_y,
				  maintain_aspect_ratio)
KWorldCanvas canvas;
flag auto_x;
flag auto_y;
flag auto_v;
flag int_x;
flag int_y;
flag maintain_aspect_ratio;
{
}
flag viewimg_register_data_change (vimage)
ViewableImage vimage;
{
    return ( (flag) 0 );
}
void viewimg_destroy (vimage)
ViewableImage vimage;
{
}
ViewableImage viewimg_get_active (canvas)
KWorldCanvas canvas;
{
    return ( (ViewableImage) 0 );
}
void viewimg_register_position_event_func (canvas, position_func, f_info)
KWorldCanvas canvas;
flag (*position_func) ();
void *f_info;
{
}
flag viewimg_fill_ellipse (vimage, centre_x, centre_y, radius_x,radius_y,value)
ViewableImage vimage;
double centre_x;
double centre_y;
double radius_x;
double radius_y;
double value[2];
{
    return ( (flag) 0 );
}
flag viewimg_fill_polygon (vimage, coords, num_vertices, value)
ViewableImage vimage;
edit_coord *coords;
unsigned int num_vertices;
double value[2];
{
    return ( (flag) 0 );
}

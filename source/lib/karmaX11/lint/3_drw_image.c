#include <karma_drw.h>
# 1 "" 
flag drw_single_plane (ximage, num_pixels, pixel_values,
		       data, elem_type, conv_type,
		       abs_dim_desc, abs_dim_stride,
		       ord_dim_desc, ord_dim_stride, win_scale)
XImage *ximage;
unsigned int num_pixels;
unsigned long *pixel_values;
char *data;
unsigned int elem_type;
unsigned int conv_type;
dim_desc *abs_dim_desc;
unsigned int abs_dim_stride;
dim_desc *ord_dim_desc;
unsigned int ord_dim_stride;
struct win_scale_type *win_scale;
{
    return ( (flag) 0 );
}

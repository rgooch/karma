#include <karma_ds.h>
# 1 "" 
flag ds_draw_ellipse (array, elem_type, abs_dim_desc, abs_stride,
		      ord_dim_desc, ord_stride,
		      centre_abs, centre_ord, radius_abs, radius_ord, value)
char *array;
unsigned int elem_type;
dim_desc *abs_dim_desc;
unsigned int abs_stride;
dim_desc *ord_dim_desc;
unsigned int ord_stride;
double centre_abs;
double centre_ord;
double radius_abs;
double radius_ord;
double *value;
{
    return ( (flag) 0 );
}
flag ds_draw_polygon (array, elem_type, abs_dim_desc, abs_stride, ord_dim_desc,
		      ord_stride, coords, num_points, value)
char *array;
unsigned int elem_type;
dim_desc *abs_dim_desc;
unsigned int abs_stride;
dim_desc *ord_dim_desc;
unsigned int ord_stride;
edit_coord *coords;
unsigned int num_points;
double *value;
{
    return ( (flag) 0 );
}

#include <karma_iarray.h>
# 1 "" 
iarray iarray_read_nD (object, cache, arrayname, num_dim, dim_names, elem_name,
		       mmap_option)
char *object;
flag cache;
char *arrayname;
unsigned int num_dim;
char **dim_names;
char *elem_name;
unsigned int mmap_option;
{
    return ( (iarray) 0 );
}
flag iarray_write (array, arrayfile)
iarray array;
char *arrayfile;
{
    return ( (flag) 0 );
}
iarray iarray_create (type, num_dim, dim_names, dim_lengths, elem_name,
		      old_array)
unsigned int type;
unsigned int num_dim;
char **dim_names;
unsigned int *dim_lengths;
char *elem_name;
iarray old_array;
{
    return ( (iarray) 0 );
}
iarray iarray_create_1D (xlen, type)
unsigned int xlen;
unsigned int type;
{
    return ( (iarray) 0 );
}
iarray iarray_create_2D (ylen, xlen, type)
unsigned int ylen;
unsigned int xlen;
unsigned int type;
{
    return ( (iarray) 0 );
}
iarray iarray_create_3D (zlen, ylen, xlen, type)
unsigned int zlen;
unsigned int ylen;
unsigned int xlen;
unsigned int type;
{
    return ( (iarray) 0 );
}
iarray iarray_get_from_multi_array (multi_desc, arrayname, num_dim, dim_names,
				    elem_name)
multi_array *multi_desc;
char *arrayname;
unsigned int num_dim;
char **dim_names;
char *elem_name;
{
    return ( (iarray) 0 );
}
void iarray_dealloc (array)
iarray array;
{
}
flag iarray_put_named_value (array, name, type, value)
iarray array;
char *name;
unsigned int type;
double *value;
{
    return ( (flag) 0 );
}
flag iarray_get_named_value (array, name, type, value)
iarray array;
char *name;
unsigned int *type;
double *value;
{
    return ( (flag) 0 );
}
flag iarray_put_float (array, name, value)
iarray array;
char *name;
float value;
{
    return ( (flag) 0 );
}
flag iarray_put_int (array, name, value)
iarray array;
char *name;
int value;
{
    return ( (flag) 0 );
}
float iarray_get_float (array, name)
iarray array;
char *name;
{
    return ( (float) 0 );
}
int iarray_get_int (array, name)
iarray array;
char *name;
{
    return ( (int) 0 );
}
flag iarray_copy_data (output, input, magnitude)
iarray output;
iarray input;
flag magnitude;
{
    return ( (flag) 0 );
}
char *iarray_get_element_1D (array, type, x)
iarray array;
unsigned int type;
int x;
{
    return ( (char *) 0 );
}
char *iarray_get_element_2D (array, type, y, x)
iarray array;
unsigned int type;
int y;
int x;
{
    return ( (char *) 0 );
}
char *iarray_get_element_3D (array, type, z, y, x)
iarray array;
unsigned int type;
int z;
int y;
int x;
{
    return ( (char *) 0 );
}
iarray iarray_get_sub_array_2D (array, starty, startx, ylen, xlen)
iarray array;
int starty;
int startx;
unsigned int ylen;
unsigned int xlen;
{
    return ( (iarray) 0 );
}
unsigned int iarray_dim_length (array, index)
iarray array;
unsigned int index;
{
    return ( (unsigned int) 0 );
}
flag iarray_fill (array, value)
iarray array;
double *value;
{
    return ( (flag) 0 );
}
flag iarray_fill_float (array, value)
iarray array;
float value;
{
    return ( (flag) 0 );
}
flag iarray_fill_int (array, value)
iarray array;
int value;
{
    return ( (flag) 0 );
}
flag iarray_min_max (array, conv_type, min, max)
iarray array;
unsigned int conv_type;
double *min;
double *max;
{
    return ( (flag) 0 );
}
flag iarray_min_max_float (array, min, max)
iarray array;
float *min;
float *max;
{
    return ( (flag) 0 );
}
flag iarray_min_max_int (array, min, max)
iarray array;
int *min;
int *max;
{
    return ( (flag) 0 );
}
flag iarray_scale_and_offset (out, inp, scale, offset, magnitude)
iarray out;
iarray inp;
double *scale;
double *offset;
flag magnitude;
{
    return ( (flag) 0 );
}
flag iarray_scale_and_offset_float (out, inp, scale, offset)
iarray out;
iarray inp;
float scale;
float offset;
{
    return ( (flag) 0 );
}
flag iarray_scale_and_offset_int (out, inp, scale, offset)
iarray out;
iarray inp;
int scale;
int offset;
{
    return ( (flag) 0 );
}
flag iarray_add_and_scale (out, inp1, inp2, scale, magnitude)
iarray out;
iarray inp1;
iarray inp2;
double *scale;
flag magnitude;
{
    return ( (flag) 0 );
}
flag iarray_sub_and_scale (out, inp1, inp2, scale, magnitude)
iarray out;
iarray inp1;
iarray inp2;
double *scale;
flag magnitude;
{
    return ( (flag) 0 );
}
char *iarray_dim_name (array, index)
iarray array;
unsigned int index;
{
    return ( (char *) 0 );
}
void iarray_remap_torus (array, boundary_width)
iarray array;
unsigned int boundary_width;
{
}

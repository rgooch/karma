#include <karma_iarray.h>
# 1 "" 
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

#include <karma_ds.h>
# 1 "" 
double ds_convert_atomic (datum, datum_type, real_out, imag_out)
char *datum;
unsigned int datum_type;
double *real_out;
double *imag_out;
{
    return ( (double) 0 );
}
double ds_get_coordinate (dimension, coord_num)
dim_desc *dimension;
unsigned int coord_num;
{
    return ( (double) 0 );
}
unsigned int ds_get_element_offset (pack_desc, elem_num)
packet_desc *pack_desc;
unsigned int elem_num;
{
    return ( (unsigned int) 0 );
}
unsigned int ds_get_packet_size (pack_desc)
packet_desc *pack_desc;
{
    return ( (unsigned int) 0 );
}
unsigned int ds_get_array_size (arr_desc)
array_desc *arr_desc;
{
    return ( (unsigned int) 0 );
}
flag ds_packet_all_data (pack_desc)
packet_desc *pack_desc;
{
    return ( (flag) 0 );
}
flag ds_element_is_atomic (element_type)
unsigned int element_type;
{
    return ( (flag) 0 );
}
flag ds_element_is_named (element_type)
unsigned int element_type;
{
    return ( (flag) 0 );
}
flag ds_element_is_legal (element_type)
unsigned int element_type;
{
    return ( (flag) 0 );
}
unsigned int ds_identify_name (multi_desc, name, encls_desc, index)
multi_array *multi_desc;
char *name;
char **encls_desc;
unsigned int *index;
{
    return ( (unsigned int) 0 );
}
unsigned int ds_f_array_name (multi_desc, name, encls_desc, index)
multi_array *multi_desc;
char *name;
char **encls_desc;
unsigned int *index;
{
    return ( (unsigned int) 0 );
}
unsigned int ds_f_name_in_packet (pack_desc, name, encls_desc, index)
packet_desc *pack_desc;
char *name;
char **encls_desc;
unsigned int *index;
{
    return ( (unsigned int) 0 );
}
unsigned int ds_f_name_in_array (arr_desc, name, encls_desc, index)
array_desc *arr_desc;
char *name;
char **encls_desc;
unsigned int *index;
{
    return ( (unsigned int) 0 );
}
unsigned int ds_f_elem_in_packet (pack_desc, name)
packet_desc *pack_desc;
char *name;
{
    return ( (unsigned int) 0 );
}
unsigned int ds_find_hole (inp_desc, out_desc, elem_num)
packet_desc *inp_desc;
packet_desc **out_desc;
unsigned int *elem_num;
{
    return ( (unsigned int) 0 );
}
flag ds_compare_packet_desc (desc1, desc2, recursive)
packet_desc *desc1;
packet_desc *desc2;
flag recursive;
{
    return ( (flag) 0 );
}
flag ds_compare_array_desc (desc1, desc2, recursive)
array_desc *desc1;
array_desc *desc2;
flag recursive;
{
    return ( (flag) 0 );
}
flag ds_compare_dim_desc (desc1, desc2)
dim_desc *desc1;
dim_desc *desc2;
{
    return ( (flag) 0 );
}
unsigned int ds_f_dim_in_array (arr_desc, name)
array_desc *arr_desc;
char *name;
{
    return ( (unsigned int) 0 );
}
unsigned int ds_get_array_offset (arr_desc, coordinates)
array_desc *arr_desc;
unsigned int *coordinates;
{
    return ( (unsigned int) 0 );
}
unsigned int ds_get_coord_num (dimension, coordinate, bias)
dim_desc *dimension;
double coordinate;
unsigned int bias;
{
    return ( (unsigned int) 0 );
}
flag ds_get_element (datum, datum_type, value, complex)
char *datum;
unsigned int datum_type;
double *value;
flag *complex;
{
    return ( (flag) 0 );
}
flag ds_get_elements (data, data_type, data_stride, values, complex,num_values)
char *data;
unsigned int data_type;
unsigned int data_stride;
double *values;
flag *complex;
unsigned int num_values;
{
    return ( (flag) 0 );
}
double *ds_get_coordinate_array (dimension)
dim_desc *dimension;
{
    return ( (double *) 0 );
}
flag ds_element_is_complex (element_type)
unsigned int element_type;
{
    return ( (flag) 0 );
}

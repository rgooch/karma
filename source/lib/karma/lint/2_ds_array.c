#include <karma_ds.h>
# 1 "" 
flag ds_find_single_extremes (data, elem_type, conv_type, dimension, stride,
			      scan_start, scan_end, min, max)
char *data;
unsigned int elem_type;
unsigned int conv_type;
dim_desc *dimension;
unsigned int stride;
double scan_start;
double scan_end;
double *min;
double *max;
{
    return ( (flag) 0 );
}
flag ds_find_plane_extremes (data, elem_type, conv_type,
			     abs_dim_desc, abs_dim_stride,
			     ord_dim_desc, ord_dim_stride,
			     abs_scan_start, abs_scan_end,
			     ord_scan_start, ord_scan_end,
			     min, max)
char *data;
unsigned int elem_type;
unsigned int conv_type;
dim_desc *abs_dim_desc;
unsigned int abs_dim_stride;
dim_desc *ord_dim_desc;
unsigned int ord_dim_stride;
double abs_scan_start;
double abs_scan_end;
double ord_scan_start;
double ord_scan_end;
double *min;
double *max;
{
    return ( (flag) 0 );
}

/*  karma_iarray.h

    Header for  iarray_  package.

    Copyright (C) 1992,1993,1994,1995  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*

    This include file contains all the definitions and function declarations
  needed to interface to the iarray_ routines in the Karma library.


    Written by      Richard Gooch   17-NOV-1992

    Last updated by Richard Gooch   18-JAN-1995

*/

#ifndef KARMA_IARRAY_H
#define KARMA_IARRAY_H


#ifndef KARMA_H
#  include <karma.h>
#endif

#ifndef KARMA_DS_DEF_H
#  include <karma_ds_def.h>
#endif

#ifndef KARMA_CH_DEF_H
#  include <karma_ch_def.h>
#endif

#ifndef KARMA_PSW_DEF_H
#  include <karma_psw_def.h>
#endif

#ifndef KARMA_C_DEF_H
#  include <karma_c_def.h>
#endif


/*  Structure declarations  */
typedef struct
{
    char *data;             /*  Pointer to start of array  */
    uaddr **offsets;        /*  Array of offset pointers for each dimension  */
    unsigned long *lengths; /*  Array of dimension lengths                   */
    flag *contiguous;
    packet_desc *top_pack_desc;
    char **top_packet;
    multi_array *multi_desc;
    array_desc *arr_desc;
    unsigned int array_num;
    int boundary_width;
    unsigned int elem_index;
    unsigned int num_dim;
    unsigned int *orig_dim_indices;
    unsigned int *restrictions;
    unsigned int magic_number;
    KCallbackList destroy_callbacks;
} *iarray;


/*  Macros  */

#ifdef DEBUG

#define F1(a, x) ( *(float *) iarray_get_element_1D ( (a), K_FLOAT, (x) ) )
#define I1(a, x) ( *(int *) iarray_get_element_1D ( (a), K_INT, (x) ) )
#define UI1(a, x) ( *(unsigned int *) iarray_get_element_1D ( (a), K_INT, (x) ) )
#define UB1(a, x) ( *(unsigned char *) iarray_get_element_1D ( (a), K_UBYTE,(x) ) )
#define B1(a, x) ( *(char *) iarray_get_element_1D ( (a), K_BYTE, (x) ) )

#define F2(a, y, x) ( *(float *) iarray_get_element_2D ( (a), K_FLOAT, (y), (x) ) )
#define UI2(a, y, x) ( *(unsigned int *) iarray_get_element_2D ( (a), K_INT, (y), (x) ) )
#define I2(a, y, x) ( *(int *) iarray_get_element_2D ( (a), K_INT, (y), (x) ) )
#define UB2(a, y, x) ( *(unsigned char *) iarray_get_element_2D ( (a), K_UBYTE, (y), (x) ) )
#define B2(a, y, x) ( *(char *) iarray_get_element_2D ( (a), K_BYTE, (y), (x) ) )

#define F3(a, z, y, x) ( *(float *) iarray_get_element_3D ( (a), K_FLOAT, (z), (y), (x) ) )
#define I3(a, z, y, x) ( *(int *) iarray_get_element_3D ( (a), K_INT, (z), (y), (x) ) )
#define UI3(a, z, y, x) ( *(unsigned int *) iarray_get_element_3D ( (a), K_INT, (z), (y), (x) ) )
#define UB3(a, z, y, x) ( *(unsigned char *) iarray_get_element_3D ( (a), K_UBYTE, (z), (y), (x) ) )
#define B3(a, z, y, x) ( *(char *) iarray_get_element_3D ( (a), K_BYTE, (z), (y), (x) ) )

#else  /*  !DEBUG  */

#define F1(a, x) ( *(float *) ( (*(a)).data + (*(a)).offsets[0][(x)] ) )
#define I1(a, x) ( *(int *) ( (*(a)).data + (*(a)).offsets[0][(x)] ) )
#define UI1(a, x) ( *(unsigned int *) ( (*(a)).data + (*(a)).offsets[0][(x)] ) )
#define UB1(a, x) ( *(unsigned char *) ( (*(a)).data + (*(a)).offsets[0][(x)] ) )
#define B1(a, x) ( *(char *) ( (*(a)).data + (*(a)).offsets[0][(x)] ) )

#define F2(a, y, x) ( *(float *) ( (*(a)).data + (*(a)).offsets[0][(y)] + (*(a)).offsets[1][(x)] ) )
#define I2(a, y, x) ( *(int *) ( (*(a)).data + (*(a)).offsets[0][(y)] + (*(a)).offsets[1][(x)] ) )
#define UI2(a, y, x) ( *(unsigned int *) ( (*(a)).data + (*(a)).offsets[0][(y)] + (*(a)).offsets[1][(x)] ) )
#define UB2(a, y, x) ( *(unsigned char *) ( (*(a)).data + (*(a)).offsets[0][(y)] + (*(a)).offsets[1][(x)] ) )
#define B2(a, y, x) ( *(char *) ( (*(a)).data + (*(a)).offsets[0][(y)] + (*(a)).offsets[1][(x)] ) )

#define F3(a, z, y, x) ( *(float *) ( (*(a)).data + (*(a)).offsets[0][(z)] + (*(a)).offsets[1][(y)] + (*(a)).offsets[2][(x)] ) )
#define I3(a, z, y, x) ( *(int *) ( (*(a)).data + (*(a)).offsets[0][(z)] + (*(a)).offsets[1][(y)] + (*(a)).offsets[2][(x)] ) )
#define UI3(a, z, y, x) ( *(unsigned int *) ( (*(a)).data + (*(a)).offsets[0][(z)] + (*(a)).offsets[1][(y)] + (*(a)).offsets[2][(x)] ) )
#define UB3(a, z, y, x) ( *(unsigned char *) ( (*(a)).data + (*(a)).offsets[0][(z)] + (*(a)).offsets[1][(y)] + (*(a)).offsets[2][(x)] ) )
#define B3(a, z, y, x) ( *(char *) ( (*(a)).data + (*(a)).offsets[0][(z)] + (*(a)).offsets[1][(y)] + (*(a)).offsets[2][(x)] ) )

#endif

#define iarray_num_dim(a) (*(a)).num_dim
#define iarray_type(a) (* (* (*(a)).arr_desc ).packet ).element_types[(*(a)).elem_index]
#define iarray_value_name(a) (* (* (*(a)).arr_desc ).packet ).element_desc[(*(a)).elem_index]


/*  File:  main.c   */
EXTERN_FUNCTION (iarray iarray_read_nD, (CONST char *arrayfile, flag cache,
					 CONST char *arrayname,
					 unsigned int num_dim,
					 char **dim_names, char *elem_name,
					 unsigned int mmap_option) );
EXTERN_FUNCTION (flag iarray_write, (iarray array, CONST char *arrayfile) );
EXTERN_FUNCTION (iarray iarray_create,
		 (unsigned int type, unsigned int num_dim, char **dim_names,
		  unsigned long *dim_lengths,
		  char *elem_name, iarray old_array) );
EXTERN_FUNCTION (iarray iarray_get_from_multi_array,
		 (multi_array *multi_desc, CONST char *arrayname,
		  unsigned int num_dim, char **dim_names, char *elem_name) );
EXTERN_FUNCTION (void iarray_dealloc, (iarray array) );
EXTERN_FUNCTION (flag iarray_put_named_value,
		 (iarray array, char *name, unsigned int type,
		  double *value) );
EXTERN_FUNCTION (flag iarray_put_named_string,
		 (iarray array, char *name, char *string) );
EXTERN_FUNCTION (flag iarray_get_named_value,
		 (iarray array, char *name, unsigned int *type,
		  double *value) );
EXTERN_FUNCTION (char *iarray_get_named_string, (iarray array, char *name) );
EXTERN_FUNCTION (flag iarray_copy_data,
		 (iarray output, iarray input, flag magnitude) );
EXTERN_FUNCTION (char *iarray_get_element_1D, (iarray array,
					       unsigned int type,
					       int x) );
EXTERN_FUNCTION (char *iarray_get_element_2D, (iarray array,
					       unsigned int type,
					       int y,
					       int x) );
EXTERN_FUNCTION (char *iarray_get_element_3D, (iarray array,
					       unsigned int type,
					       int z,
					       int y,
					       int x) );
EXTERN_FUNCTION (iarray iarray_get_sub_array_2D,
		 (iarray array, int starty, int startx,
		  unsigned int ylen, unsigned int xlen) );
EXTERN_FUNCTION (iarray iarray_get_2D_slice_from_3D,
		 (iarray cube, unsigned int ydim, unsigned int xdim,
		  unsigned int slice_pos) );
EXTERN_FUNCTION (unsigned long iarray_dim_length, (iarray array,
						   unsigned int index) );
EXTERN_FUNCTION (unsigned int iarray_get_restrictions,
		 (iarray array, char ***restr_names, double **restr_values) );
EXTERN_FUNCTION (flag iarray_fill, (iarray array, double *value) );
EXTERN_FUNCTION (flag iarray_min_max, (iarray array, unsigned int conv_type,
				       double *min, double *max) );
EXTERN_FUNCTION (flag iarray_scale_and_offset, (iarray out, iarray inp,
						double *scale,
						double *offset,
						flag magnitude) );
EXTERN_FUNCTION (flag iarray_add_and_scale, (iarray out,
					     iarray inp1, iarray inp2,
					     double *scale, flag magnitude) );
EXTERN_FUNCTION (flag iarray_sub_and_scale, (iarray out,
					     iarray inp1, iarray inp2,
					     double *scale, flag magnitude) );
EXTERN_FUNCTION (flag iarray_mul_and_offset, (iarray out,
					      iarray inp1, iarray inp2,
					      double *inp1_offset,
					      flag magnitude) );
EXTERN_FUNCTION (flag iarray_div_and_offset, (iarray out,
					      iarray inp1, iarray inp2,
					      double *inp1_offset,
					      flag magnitude) );
EXTERN_FUNCTION (void iarray_remap_torus, (iarray array,
					   unsigned int boundary_width) );
EXTERN_FUNCTION (char *iarray_dim_name, (iarray array, unsigned int index) );
EXTERN_FUNCTION (void iarray_set_world_coords, (iarray array,
						unsigned int index,
						double minimum,
						double maximum) );
EXTERN_FUNCTION (void iarray_get_world_coords, (iarray array,
						unsigned int index,
						double *minimum,
						double *maximum) );
EXTERN_FUNCTION (dim_desc *iarray_get_dim_desc, (iarray array,
						 unsigned int index) );
EXTERN_FUNCTION (flag iarray_compute_histogram,
		 (iarray array, unsigned int conv_type,
		  double min, double max, unsigned long num_bins,
		  unsigned long *histogram_array,
		  unsigned long *histogram_peak,
		  unsigned long *histogram_mode) );


/*  File:  wrappers.c   */
EXTERN_FUNCTION (iarray iarray_create_1D, (unsigned long xlen,
					   unsigned int type) );
EXTERN_FUNCTION (iarray iarray_create_2D, (unsigned long ylen,
					   unsigned long xlen,
					   unsigned int type) );
EXTERN_FUNCTION (iarray iarray_create_3D, (unsigned long zlen,
					   unsigned long ylen,
					   unsigned long xlen,
					   unsigned int type) );
EXTERN_FUNCTION (flag iarray_put_float, (iarray array, char *name,
					 float value) );
EXTERN_FUNCTION (flag iarray_put_int, (iarray array, char *name, int value) );
EXTERN_FUNCTION (float iarray_get_float, (iarray array, char *name) );
EXTERN_FUNCTION (int iarray_get_int, (iarray array, char *name) );
EXTERN_FUNCTION (flag iarray_fill_float, (iarray array, float value) );
EXTERN_FUNCTION (flag iarray_fill_int, (iarray array, int value) );
EXTERN_FUNCTION (flag iarray_min_max_float, (iarray array,
					     float *min, float *max) );
EXTERN_FUNCTION (flag iarray_min_max_int, (iarray array,
					   int *min, int *max) );
EXTERN_FUNCTION (flag iarray_scale_and_offset_float, (iarray out, iarray inp,
						      float scale,
						      float offset) );
EXTERN_FUNCTION (flag iarray_scale_and_offset_int, (iarray out, iarray inp,
						    int scale, int offset) );


/*  File:  ps.c  */
EXTERN_FUNCTION (flag iarray_write_mono_ps,
		 (iarray image, PostScriptPage pspage,
		  double xstart, double ystart, double xend, double yend,
		  flag iscale) );
EXTERN_FUNCTION (flag iarray_write_pseudocolour_ps,
		 (iarray image, PostScriptPage pspage,
		  double xstart, double ystart, double xend, double yend,
		  unsigned short *cmap, unsigned int cmap_size) );
EXTERN_FUNCTION (flag iarray_write_rgb_ps,
		 (iarray image_red, iarray image_green, iarray image_blue,
		  PostScriptPage pspage,
		  double xstart, double ystart, double xend, double yend) );


/*  File:  get.c  */
EXTERN_FUNCTION (flag iarray_get_image_from_multi,
		 (multi_array *multi_desc, iarray *pseudo,
		  iarray *red, iarray *green, iarray *blue,
		  unsigned int *cmap_index) );
EXTERN_FUNCTION (flag iarray_get_movie_from_multi,
		 (multi_array *multi_desc, iarray *pseudo,
		  iarray *red, iarray *green, iarray *blue,
		  unsigned int *cmap_index) );


#endif /*  KARMA_IARRAY_H  */

/*  karma_iarray.h

    Header for  iarray_  package.

    Copyright (C) 1992-1996  Richard Gooch

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

    Last updated by Richard Gooch   19-JUN-1996

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#if !defined(KARMA_DS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ds_def.h>
#endif

#if !defined(KARMA_CH_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ch_def.h>
#endif

#if !defined(KARMA_PSW_DEF_H) || defined(MAKEDEPEND)
#  include <karma_psw_def.h>
#endif

#if !defined(KARMA_IARRAY_DEF_H) || defined(MAKEDEPEND)
#  include <karma_iarray_def.h>
#endif

#if !defined(KARMA_C_H) || defined(MAKEDEPEND)
#  include <karma_c.h>
#endif

#ifndef KARMA_IARRAY_H
#define KARMA_IARRAY_H


/*  Macros  */

#ifdef DEBUG

/*  1-dimensional  */
#define F1(a, x) ( *(float *) iarray_get_element_1D ( (a), K_FLOAT, (x) ) )
#define D1(a, x) ( *(double *) iarray_get_element_1D ( (a), K_DOUBLE, (x) ) )
#define I1(a, x) ( *(signed int *) iarray_get_element_1D ( (a), K_INT, (x) ) )
#define UI1(a, x) ( *(unsigned int *) iarray_get_element_1D ( (a), K_UINT, (x) ) )
#define UB1(a, x) ( *(unsigned char *) iarray_get_element_1D ( (a), K_UBYTE,(x) ) )
#define B1(a, x) ( *(signed char *) iarray_get_element_1D ( (a), K_BYTE, (x) ) )

/*  2-dimensional  */
#define F2(a, y, x) ( *(float *) iarray_get_element_2D ( (a), K_FLOAT, (y), (x) ) )
#define D2(a, y, x) ( *(double *) iarray_get_element_2D ( (a), K_DOUBLE, (y), (x) ) )
#define UI2(a, y, x) ( *(unsigned int *) iarray_get_element_2D ( (a), K_UINT, (y), (x) ) )
#define I2(a, y, x) ( *(signed int *) iarray_get_element_2D ( (a), K_INT, (y), (x) ) )
#define US2(a, y, x) ( *(unsigned short *) iarray_get_element_2D ( (a), K_SHORT, (y), (x) ) )
#define S2(a, y, x) ( *(signed short *) iarray_get_element_2D ( (a), K_SHORT, (y), (x) ) )
#define UB2(a, y, x) ( *(unsigned char *) iarray_get_element_2D ( (a), K_UBYTE, (y), (x) ) )
#define B2(a, y, x) ( *(signed char *) iarray_get_element_2D ( (a), K_BYTE, (y), (x) ) )

/*  3-dimensional  */
#define F3(a, z, y, x) ( *(float *) iarray_get_element_3D ( (a), K_FLOAT, (z), (y), (x) ) )
#define D3(a, z, y, x) ( *(double *) iarray_get_element_3D ( (a), K_DOUBLE, (z), (y), (x) ) )
#define I3(a, z, y, x) ( *(signed int *) iarray_get_element_3D ( (a), K_INT, (z), (y), (x) ) )
#define UI3(a, z, y, x) ( *(unsigned int *) iarray_get_element_3D ( (a), K_UINT, (z), (y), (x) ) )
#define UB3(a, z, y, x) ( *(unsigned char *) iarray_get_element_3D ( (a), K_UBYTE, (z), (y), (x) ) )
#define B3(a, z, y, x) ( *(signed char *) iarray_get_element_3D ( (a), K_BYTE, (z), (y), (x) ) )

/*  4-dimensional  */
#define F4(a, z, y, x) ( *(float *) iarray_get_element_4D ( (a), K_FLOAT, (z), (y), (x), (w) ) )
#define D4(a, z, y, x) ( *(double *) iarray_get_element_4D ( (a), K_DOUBLE, (z), (y), (x), (w) ) )
#define I4(a, z, y, x) ( *(signed int *) iarray_get_element_4D ( (a), K_INT, (z), (y), (x), (w) ) )
#define UI4(a, z, y, x) ( *(unsigned int *) iarray_get_element_4D ( (a), K_INT, (z), (y), (x), (w) ) )
#define UB4(a, z, y, x) ( *(unsigned char *) iarray_get_element_4D ( (a), K_UBYTE, (z), (y), (x), (w) ) )
#define B4(a, z, y, x) ( *(signed char *) iarray_get_element_4D ( (a), K_BYTE, (z), (y), (x), (w) ) )

#else  /*  !DEBUG  */

/*  1-dimensional  */
#define F1(a, x) ( *(float *) ( (a)->data + (a)->offsets[0][(x)] ) )
#define D1(a, x) ( *(double *) ( (a)->data + (a)->offsets[0][(x)] ) )
#define I1(a, x) ( *(signed int *) ( (a)->data + (a)->offsets[0][(x)] ) )
#define UI1(a, x) ( *(unsigned int *) ( (a)->data + (a)->offsets[0][(x)] ) )
#define UB1(a, x) ( *(unsigned char *) ( (a)->data + (a)->offsets[0][(x)] ) )
#define B1(a, x) ( *(signed char *) ( (a)->data + (a)->offsets[0][(x)] ) )

/*  2-dimensional  */
#define F2(a, y, x) ( *(float *) ( (a)->data + (a)->offsets[0][(y)] + (a)->offsets[1][(x)] ) )
#define D2(a, y, x) ( *(double *) ( (a)->data + (a)->offsets[0][(y)] + (a)->offsets[1][(x)] ) )
#define I2(a, y, x) ( *(signed int *) ( (a)->data + (a)->offsets[0][(y)] + (a)->offsets[1][(x)] ) )
#define UI2(a, y, x) ( *(unsigned int *) ( (a)->data + (a)->offsets[0][(y)] + (a)->offsets[1][(x)] ) )
#define S2(a, y, x) ( *(signed short *) ( (a)->data + (a)->offsets[0][(y)] + (a)->offsets[1][(x)] ) )
#define US2(a, y, x) ( *(unsigned short *) ( (a)->data + (a)->offsets[0][(y)] + (a)->offsets[1][(x)] ) )
#define UB2(a, y, x) ( *(unsigned char *) ( (a)->data + (a)->offsets[0][(y)] + (a)->offsets[1][(x)] ) )
#define B2(a, y, x) ( *(signed char *) ( (a)->data + (a)->offsets[0][(y)] + (a)->offsets[1][(x)] ) )

/*  3-dimensional  */
#define F3(a, z, y, x) ( *(float *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] ) )
#define D3(a, z, y, x) ( *(double *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] ) )
#define I3(a, z, y, x) ( *(signed int *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] ) )
#define UI3(a, z, y, x) ( *(unsigned int *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] ) )
#define UB3(a, z, y, x) ( *(unsigned char *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] ) )
#define B3(a, z, y, x) ( *(signed char *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] ) )

/*  4-dimensional  */
#define F4(a, z, y, x, w) ( *(float *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] + (a)->offsets[3][(w)] ) )
#define D4(a, z, y, x, w) ( *(double *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] + (a)->offsets[3][(w)] ) )
#define I4(a, z, y, x, w) ( *(signed int *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] + (a)->offsets[3][(w)] ) )
#define UI4(a, z, y, x, w) ( *(unsigned int *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] + (a)->offsets[3][(w)] ) )
#define UB4(a, z, y, x, w) ( *(unsigned char *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] + (a)->offsets[3][(w)] ) )
#define B4(a, z, y, x, w) ( *(signed char *) ( (a)->data + (a)->offsets[0][(z)] + (a)->offsets[1][(y)] + (a)->offsets[2][(x)] + (a)->offsets[3][(w)] ) )

#endif

#define iarray_num_dim(a) (a)->num_dim
#define iarray_type(a) (a)->arr_desc->packet->element_types[(a)->elem_index]
#define iarray_value_name(a) (a)->arr_desc->packet->element_desc[(a)->elem_index]
#define iarray_register_destroy_func(a,func,o) c_register_callback (&(a)->destroy_callbacks, (func), (a), (o), FALSE, NULL, FALSE, FALSE)

/*  File:  main.c  */
EXTERN_FUNCTION (iarray iarray_read_nD,
		 (CONST char *arrayfile, flag cache, CONST char *arrayname,
		  unsigned int num_dim, CONST char **dim_names,
		  CONST char *elem_name, unsigned int mmap_option) );
EXTERN_FUNCTION (flag iarray_write, (iarray array, CONST char *arrayfile) );
EXTERN_FUNCTION (iarray iarray_create,
		 (unsigned int type, unsigned int num_dim,
		  CONST char **dim_names, CONST unsigned long *dim_lengths,
		  CONST char *elem_name, iarray old_array) );
EXTERN_FUNCTION (iarray iarray_get_from_multi_array,
		 (multi_array *multi_desc, CONST char *arrayname,
		  unsigned int num_dim, CONST char **dim_names,
		  CONST char *elem_name) );
EXTERN_FUNCTION (void iarray_dealloc, (iarray array) );
EXTERN_FUNCTION (flag iarray_put_named_value,
		 (iarray array, CONST char *name, unsigned int type,
		  double value[2]) );
EXTERN_FUNCTION (flag iarray_put_named_string,
		 (iarray array, CONST char *name, char *string) );
EXTERN_FUNCTION (flag iarray_get_named_value,
		 (iarray array, CONST char *name, unsigned int *type,
		  double value[2]) );
EXTERN_FUNCTION (char *iarray_get_named_string,
		 (iarray array, CONST char *name) );
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
EXTERN_FUNCTION (char *iarray_get_element_4D,
		 (iarray array, unsigned int type, int z, int y,int x,int w) );
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
EXTERN_FUNCTION (flag iarray_fill, (iarray array, double value[2]) );
EXTERN_FUNCTION (flag iarray_min_max, (iarray array, unsigned int conv_type,
				       double *min, double *max) );
EXTERN_FUNCTION (flag iarray_scale_and_offset, (iarray out, iarray inp,
						double *scale,
						double *offset,
						flag magnitude) );
EXTERN_FUNCTION (iarray_clip_scale_and_offset,
		 (iarray out, iarray inp, double scale, double offset,
		  double lower_clip, double upper_clip, flag blank) );
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
EXTERN_FUNCTION (CONST char *iarray_dim_name,
		 (iarray array, unsigned int index) );
EXTERN_FUNCTION (void iarray_set_world_coords,
		 (iarray array, unsigned int index,
		  double first, double last) );
EXTERN_FUNCTION (void iarray_get_world_coords,
		 (iarray array, unsigned int index,
		  double *first, double *last) );
EXTERN_FUNCTION (dim_desc *iarray_get_dim_desc, (iarray array,
						 unsigned int index) );
EXTERN_FUNCTION (flag iarray_compute_histogram,
		 (iarray array, unsigned int conv_type,
		  double min, double max, unsigned long num_bins,
		  unsigned long *histogram_array,
		  unsigned long *histogram_peak,
		  unsigned long *histogram_mode) );


/*  File:  wrappers.c  */
EXTERN_FUNCTION (iarray iarray_create_1D,
		 (unsigned long xlen, unsigned int type) );
EXTERN_FUNCTION (iarray iarray_create_2D,
		 (unsigned long ylen, unsigned long xlen, unsigned int type) );
EXTERN_FUNCTION (iarray iarray_create_3D,
		 (unsigned long zlen, unsigned long ylen, unsigned long xlen,
		  unsigned int type) );
EXTERN_FUNCTION (iarray iarray_create_4D,
		 (unsigned long zlen, unsigned long ylen,
		  unsigned long xlen, unsigned int wlen, unsigned int type) );
EXTERN_FUNCTION (flag iarray_put_float,
		 (iarray array, CONST char *name, float value) );
EXTERN_FUNCTION (flag iarray_put_int,
		 (iarray array, CONST char *name, int value) );
EXTERN_FUNCTION (float iarray_get_float, (iarray array, CONST char *name) );
EXTERN_FUNCTION (int iarray_get_int, (iarray array, CONST char *name) );
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


/*  File: misc.c  */
EXTERN_FUNCTION (unsigned int iarray_dim_index,
		 (iarray array, CONST char *name) );
EXTERN_FUNCTION (flag iarray_get_data_scaling,
		 (iarray array, double *scale, double *offset) );
EXTERN_FUNCTION (flag iarray_set_data_scaling,
		 (iarray array, double scale, double offset) );
EXTERN_FUNCTION (void iarray_format_value,
		 (iarray array, char string[STRING_LENGTH], double value,
		  double scale, double offset) );


#endif /*  KARMA_IARRAY_H  */

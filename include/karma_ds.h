/*  karma_ds.h

    Header for  ds_  package.

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
  needed to interface to the ds_ routines in the Karma library.


    Written by      Richard Gooch   13-SEP-1992

    Last updated by Richard Gooch   3-NOV-1996

*/

#if !defined(KARMA_DS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ds_def.h>
#endif

#ifndef KARMA_DS_H
#define KARMA_DS_H


/*  File:  alloc.c  */
EXTERN_FUNCTION (multi_array *ds_alloc_multi,
		 (unsigned int num_arrays) );
EXTERN_FUNCTION (packet_desc *ds_alloc_packet_desc,
		 (unsigned int num_elem) );
EXTERN_FUNCTION (char *ds_alloc_data, (packet_desc *pack_desc, flag clear,
				       flag array_alloc) );
EXTERN_FUNCTION (flag ds_alloc_packet_subdata,
		 (CONST packet_desc *pack_desc, char *packet, flag clear,
		  flag array_alloc) );
EXTERN_FUNCTION (char *ds_alloc_packet,
		 (packet_desc *pack_descriptor) );
EXTERN_FUNCTION (array_desc *ds_alloc_array_desc,
		 (unsigned int num_dimensions, unsigned int num_levels) );
EXTERN_FUNCTION (flag ds_alloc_tiling_info, (array_desc *arr_desc,
					     unsigned int num_levels) );
EXTERN_FUNCTION (dim_desc *ds_alloc_dim_desc,
		 (CONST char *dim_name, unsigned long length,
		  double first, double last, flag regular) );
EXTERN_FUNCTION (list_header *ds_alloc_list_head, () );
EXTERN_FUNCTION (list_entry *ds_alloc_list_entry,
		 (packet_desc *list_desc, flag array_alloc) );
EXTERN_FUNCTION (flag ds_alloc_array,
		 (CONST array_desc *arr_desc, char *element,
		  flag clear, flag array_alloc) );
EXTERN_FUNCTION (char *ds_easy_alloc_array,
		 (multi_array **multi_desc, unsigned int num_dim,
		  CONST unsigned long *lengths, CONST double *first_arr,
		  CONST double *last_arr,
		  CONST char **names, unsigned int data_type,
		  CONST char *data_name) );
EXTERN_FUNCTION (char *ds_easy_alloc_n_element_array,
		 (multi_array **multi_desc, unsigned int num_dim,
		  CONST unsigned long *lengths, CONST double *first_arr,
		  CONST double *last_arr,
		  CONST char **names, unsigned int num_elements,
		  CONST unsigned int *data_types, CONST char **data_names) );
EXTERN_FUNCTION (multi_array *ds_wrap_preallocated_n_element_array,
		 (char *array, unsigned int num_dim, CONST uaddr *lengths,
		  CONST double *first_arr, CONST double *last_arr,
		  CONST double **coordinates,
		  CONST char **names, unsigned int num_elements,
		  CONST unsigned int *data_types, CONST char **data_names) );
EXTERN_FUNCTION (array_desc *ds_easy_alloc_array_desc,
		 (unsigned int num_dim, CONST uaddr *lengths,
		  CONST double *first_arr, CONST double *last_arr,
		  CONST double **coordinates, CONST char **names,
		  unsigned int num_elements,
		  CONST unsigned int *data_types, CONST char **data_names) );
EXTERN_FUNCTION (flag ds_alloc_contiguous_list,
		 (CONST packet_desc *list_desc, list_header *list_head,
		  unsigned int length, flag clear, flag array_alloc) );

/*  File:  array.c  */
EXTERN_FUNCTION (flag ds_find_1D_extremes,
		 (CONST char *data, unsigned int num_values, uaddr *offsets,
		  unsigned int elem_type, unsigned int conv_type,
		  double *min, double *max) );
EXTERN_FUNCTION (flag ds_find_2D_extremes,
		 (CONST char *data, unsigned int length1, uaddr *offsets1,
		  unsigned int length2, uaddr *offsets2,
		  unsigned int elem_type, unsigned int conv_type,
		  double *min, double *max) );
EXTERN_FUNCTION (flag ds_find_contiguous_extremes,
		 (CONST char *data, unsigned int num_values,
		  uaddr stride, unsigned int elem_type, unsigned int conv_type,
		  double *min, double *max) );
EXTERN_FUNCTION (flag ds_find_single_histogram,
		 (CONST char *data, unsigned int elem_type,
		  unsigned int conv_type, unsigned int num_values,
		  CONST uaddr *offsets, unsigned int stride,
		  double min, double max, unsigned long num_bins,
		  unsigned long *histogram_array,
		  unsigned long *histogram_peak,
		  unsigned long *histogram_mode) );
EXTERN_FUNCTION (void ds_complex_to_real_1D,
		 (double *out, unsigned int out_stride,
		  double *inp, unsigned int num_values,
		  unsigned int conv_type) );
EXTERN_FUNCTION (flag ds_find_1D_sum,
		 (CONST char *data, unsigned int elem_type,
		  unsigned int num_values, CONST uaddr *offsets,
		  unsigned int stride, double sum[2]) );
EXTERN_FUNCTION (flag ds_find_1D_stats,
		 (CONST char *data, unsigned int num_values, uaddr *offsets,
		  unsigned int elem_type, unsigned int conv_type,
		  double *min, double *max, double *mean, double *stddev,
		  double *sum, double *sumsq, unsigned long *npoints) );
EXTERN_FUNCTION (flag ds_find_2D_stats,
		 (CONST char *data, unsigned int length1, uaddr *offsets1,
		  unsigned int length2, uaddr *offsets2,
		  unsigned int elem_type, unsigned int conv_type,
		  double *min, double *max, double *mean, double *stddev,
		  double *sum, double *sumsq, unsigned long *npoints) );
EXTERN_FUNCTION (flag ds_find_single_extremes, (char *data,
						unsigned int elem_type,
						unsigned int conv_type,
						dim_desc *dimension,
						unsigned int stride,
						double scan_start,
						double scan_end, double *min,
						double *max) );
EXTERN_FUNCTION (flag ds_find_plane_extremes, (char *data,
					       unsigned int elem_type,
					       unsigned int conv_type,
					       dim_desc *abs_dim_desc,
					       unsigned int abs_dim_stride,
					       dim_desc *ord_dim_desc,
					       unsigned int ord_dim_stride,
					       double abs_scan_start,
					       double abs_scan_end,
					       double ord_scan_start,
					       double ord_scan_end,
					       double *min, double *max) );

/*  File:  attach.c  */
EXTERN_FUNCTION (flag ds_put_unique_named_value,
		 (packet_desc *pack_desc, char **packet, CONST char *name,
		  unsigned int type, double value[2], flag update) );
EXTERN_FUNCTION (flag ds_put_unique_named_string, (packet_desc *pack_desc,
						   char **packet,
						   CONST char *name,
						   CONST char *string,
						   flag update) );
EXTERN_FUNCTION (flag ds_get_unique_named_value,
		 (CONST packet_desc *pack_desc, CONST char *packet,
		  CONST char *name, unsigned int *type, double value[2]) );
EXTERN_FUNCTION (char *ds_get_unique_named_string,
		 (CONST packet_desc *pack_desc, CONST char *packet,
		  CONST char *name) );
EXTERN_FUNCTION (flag ds_copy_unique_named_element,
		 (packet_desc *out_desc, char **out_packet,
		  CONST packet_desc *in_desc, CONST char *in_packet,
		  CONST char *name, flag fail_if_not_found,
		  flag fail_on_duplicate, flag replace) );
EXTERN_FUNCTION (unsigned int ds_get_fits_axis,
		 (CONST packet_desc *top_pack_desc,
		  CONST char *top_packet, CONST char *dim_name) );
EXTERN_FUNCTION (flag ds_get_data_scaling,
		 (CONST char *elem_name, CONST packet_desc *pack_desc,
		  CONST char *packet, double *scale, double *offset) );

/*  File:  cmap.c  */
EXTERN_FUNCTION (unsigned short *ds_cmap_alloc_colourmap,
		 (unsigned int size, multi_array **multi_desc,
		  packet_desc **pack_desc, char **packet) );
EXTERN_FUNCTION (unsigned short *ds_cmap_find_colourmap,
		 (packet_desc *top_pack_desc, char *top_packet,
		  unsigned int *size, flag *reordering_done,
		  CONST char *restr_names[], double *restr_values,
		  unsigned int num_restr) );
EXTERN_FUNCTION (unsigned int *ds_cmap_get_all_colourmaps,
		 (multi_array *multi_desc, unsigned int *num_found,
		  flag *reordering_done, CONST char *restr_names[],
		  double *restr_values, unsigned int num_restr) );

/*  File:  compare.c  */
EXTERN_FUNCTION (flag ds_compare_packet_desc,
		 (CONST packet_desc *desc1, CONST packet_desc *desc2,
		  flag recursive) );
EXTERN_FUNCTION (flag ds_compare_array_desc,
		 (CONST array_desc *desc1, CONST array_desc *desc2,
		  flag recursive) );
EXTERN_FUNCTION (flag ds_compare_dim_desc,
		 (CONST dim_desc *desc1, CONST dim_desc *desc2) );

/*  File: contour.c  */
EXTERN_FUNCTION (unsigned int ds_contour,
		 (CONST char *image, unsigned int elem_type,
		  CONST dim_desc *hdim, CONST uaddr *hoffsets,
		  CONST dim_desc *vdim, CONST uaddr *voffsets,
		  unsigned int num_contours, CONST double *contour_levels,
		  uaddr *buf_size,
		  double **x0_arr, double **y0_arr,
		  double **x1_arr, double **y1_arr) );

/*  File:  copy.c  */
EXTERN_FUNCTION (flag ds_copy_packet,
		 (CONST packet_desc *pack_desc, char *dest_packet,
		  CONST char *source_packet) );
EXTERN_FUNCTION (packet_desc *ds_copy_desc_until,
		 (CONST packet_desc *inp_desc, CONST char *name) );
EXTERN_FUNCTION (array_desc *ds_copy_array_desc_until,
		 (CONST array_desc *inp_desc, CONST char *name) );
EXTERN_FUNCTION (dim_desc *ds_copy_dim_desc,
		 (CONST dim_desc *inp_desc) );
EXTERN_FUNCTION (flag ds_copy_data,
		 (CONST packet_desc *inp_desc, CONST char *inp_data,
		  packet_desc *out_desc, char *out_data) );
EXTERN_FUNCTION (flag ds_copy_array,
		 (CONST array_desc *inp_desc, CONST char *inp_data,
		  array_desc *out_desc, char *out_data) );
EXTERN_FUNCTION (flag ds_copy_list,
		 (CONST packet_desc *inp_desc, CONST list_header *inp_head,
		  packet_desc *out_desc, list_header *out_head) );
EXTERN_FUNCTION (multi_array *ds_select_arrays, 
		 (char **array_list, unsigned int num_in_list,
		  multi_array *multi_desc, flag save_unproc,
		  unsigned int **index_list) );

/*  File:  dealloc.c  */
EXTERN_FUNCTION (void ds_dealloc_multi, (multi_array *multi_desc) );
EXTERN_FUNCTION (void ds_dealloc_packet, (packet_desc *pack_desc,
					  char *data) );
EXTERN_FUNCTION (void ds_dealloc_data, (packet_desc *pack_desc,
					char *packet) );
EXTERN_FUNCTION (void ds_dealloc_packet_subdata, (CONST packet_desc *pack_desc,
						  char *packet) );
EXTERN_FUNCTION (void ds_dealloc_array_desc, (array_desc *arr_desc) );
EXTERN_FUNCTION (void ds_dealloc_list, (packet_desc *list_desc,
					list_header *list_head) );
EXTERN_FUNCTION (void ds_dealloc_list_entries,
		 (CONST packet_desc *list_desc, list_header *list_head) );
EXTERN_FUNCTION (void ds_dealloc2_list, (list_header *list_head) );
EXTERN_FUNCTION (void ds_dealloc_array, (array_desc *arr_desc,
					 char *arr_element) );

/*  File:  draw.c  */
EXTERN_FUNCTION (flag ds_draw_ellipse, (char *array, unsigned int elem_type,
					dim_desc *abs_dim_desc,
					unsigned int abs_stride,
					dim_desc *ord_dim_desc,
					unsigned int ord_stride,
					double centre_abs, double centre_ord,
					double radius_abs, double radius_ord,
					double *value) );
EXTERN_FUNCTION (flag ds_draw_polygon, (char *array, unsigned int elem_type,
					dim_desc *abs_dim_desc,
					unsigned int abs_stride,
					dim_desc *ord_dim_desc,
					unsigned int ord_stride,
					edit_coord *coords,
					unsigned int num_points,
					double *value) );

/*  File: find.c  */
EXTERN_FUNCTION (unsigned int ds_identify_name,
		 (CONST multi_array *multi_desc, CONST char *name,
		  char **encls_desc, unsigned int *index) );
EXTERN_FUNCTION (unsigned int ds_f_array_name,
		 (CONST multi_array *multi_desc, CONST char *name,
		  char **encls_desc, unsigned int *index) );
EXTERN_FUNCTION (unsigned int ds_f_name_in_packet,
		 (CONST packet_desc *pack_desc, CONST char *name,
		  char **encls_desc, unsigned int *index) );
EXTERN_FUNCTION (unsigned int ds_f_name_in_array,
		 (CONST array_desc *arr_desc, CONST char *name,
		  char **encls_desc, unsigned int *index) );
EXTERN_FUNCTION (unsigned int ds_f_elem_in_packet,
		 (CONST packet_desc *pack_desc, CONST char *name) );
EXTERN_FUNCTION (unsigned int ds_find_hole,
		 (CONST packet_desc *inp_desc, packet_desc **out_desc,
		  unsigned int *elem_num) );
EXTERN_FUNCTION (unsigned int ds_f_dim_in_array,
		 (CONST array_desc *arr_desc, CONST char *name) );

/*  File:  get.c  */
EXTERN_FUNCTION (double ds_convert_atomic,
		 (CONST char *datum, unsigned int datum_type,
		  double *real_out, double *imag_out) );
EXTERN_FUNCTION (double ds_get_coordinate,
		 (CONST dim_desc *dimension, double coord_num) );
EXTERN_FUNCTION (unsigned int ds_get_element_offset,
		 (CONST packet_desc *pack_desc, unsigned int elem_num) );
EXTERN_FUNCTION (unsigned int ds_get_packet_size,
		 (CONST packet_desc *pack_desc) );
EXTERN_FUNCTION (unsigned long ds_get_array_size,
		 (CONST array_desc *arr_desc) );
EXTERN_FUNCTION (flag ds_packet_all_data, (CONST packet_desc *pack_desc) );
EXTERN_FUNCTION (flag ds_element_is_atomic, (unsigned int element_type) );
EXTERN_FUNCTION (flag ds_element_is_named, (unsigned int element_type) );
EXTERN_FUNCTION (flag ds_element_is_legal, (unsigned int element_type) );
EXTERN_FUNCTION (unsigned long ds_get_array_offset,
		 (CONST array_desc *arr_desc,
		  CONST unsigned long *coordinates) );
EXTERN_FUNCTION (unsigned long ds_get_coord_num,
		 (CONST dim_desc *dimension, double coordinate,
		  unsigned int bias) );
EXTERN_FUNCTION (flag ds_get_element,
		 (CONST char *datum, unsigned int datum_type,
		  double *value, flag *complex) );
EXTERN_FUNCTION (flag ds_get_elements, (CONST char *data,
					unsigned int data_type,
					unsigned int data_stride,
					double *values, flag *complex,
					unsigned int num_values) );
EXTERN_FUNCTION (double *ds_get_coordinate_array,
		 (CONST dim_desc *dimension) );
EXTERN_FUNCTION (flag ds_element_is_complex, (unsigned int element_type) );
EXTERN_FUNCTION (flag ds_get_scattered_elements,
		 (CONST char *data, unsigned int data_type,
		  CONST uaddr *offsets, double *values, flag *complex,
		  unsigned int num_values) );
EXTERN_FUNCTION (flag ds_can_transfer_element_as_block, (unsigned int type) );
EXTERN_FUNCTION (flag ds_can_transfer_packet_as_block,
		 (CONST packet_desc *pack_desc) );
EXTERN_FUNCTION (flag ds_can_swaptransfer_element, (unsigned int type) );

/*  File:  handle.c  */
EXTERN_FUNCTION (unsigned int ds_get_handle_in_packet,
		 (packet_desc *pack_desc, char *packet, CONST char *item_name,
		  CONST char *restr_names[], double *restr_values,
		  unsigned int num_restr, char **parent_desc, char **parent,
		  unsigned int *parent_type, unsigned int *index) );
EXTERN_FUNCTION (unsigned int ds_get_handle_in_array,
		 (array_desc *arr_desc, char *array, CONST char *item_name,
		  CONST char *restr_names[], double *restr_values,
		  unsigned int num_restr, char **parent_desc, char **parent,
		  unsigned int *parent_type, unsigned int *index) );
EXTERN_FUNCTION (unsigned int ds_get_handle_in_list,
		 (packet_desc *list_desc, list_header *list_head,
		  CONST char *item_name, CONST char *restr_names[],
		  double *restr_values,
		  unsigned int num_restr, char **parent_desc, char **parent,
		  unsigned int *parent_type, unsigned int *index) );

/*  File: history.c  */
EXTERN_FUNCTION (flag ds_history_append_string,
		 (multi_array *multi_desc, CONST char *string,
		  flag new_alloc) );
EXTERN_FUNCTION (flag ds_history_copy,
		 (multi_array *out, CONST multi_array *in) );

/*  File:  list.c  */
EXTERN_FUNCTION (void ds_list_insert, (list_header *list_head,
				       list_entry *new_entry,
				       list_entry *entry) );
EXTERN_FUNCTION (void ds_list_append, (list_header *list_head,
				       list_entry *entry) );
EXTERN_FUNCTION (void ds_list_delete, (packet_desc *list_desc,
				       list_header *list_head,
				       list_entry *entry) );
EXTERN_FUNCTION (flag ds_list_unfragment, (packet_desc *list_desc,
					   list_header *list_head) );
EXTERN_FUNCTION (flag ds_list_fragment, (packet_desc *list_desc,
					 list_header *list_head) );

/*  File:  misc.c  */
EXTERN_FUNCTION (void ds_format_unit, (char unit[STRING_LENGTH], double *scale,
				       CONST char *value_name) );
EXTERN_FUNCTION (void ds_format_value,
		 (char string[STRING_LENGTH], double value,
		  CONST char *value_name, double scale, double offset,
		  CONST packet_desc *pack_desc, CONST char *packet) );

/*  File:  mod_desc.c  */
EXTERN_FUNCTION (flag ds_remove_dim_desc, (array_desc *arr_desc,
					   CONST char *dim_name) );
EXTERN_FUNCTION (flag ds_append_dim_desc, (array_desc *arr_desc,
					   dim_desc *dimension) );
EXTERN_FUNCTION (flag ds_prepend_dim_desc, (array_desc *arr_desc,
					    dim_desc *dimension) );
EXTERN_FUNCTION (flag ds_compute_array_offsets, (array_desc *arr_desc) );
EXTERN_FUNCTION (void ds_remove_tiling_info, (array_desc *arr_desc) );
EXTERN_FUNCTION (flag ds_autotile_array,
		 (array_desc *arr_desc, flag allow_truncate) );

/*  File:  put.c  */
EXTERN_FUNCTION (char *ds_put_element, (char *output, unsigned int type,
					double input[2]) );
EXTERN_FUNCTION (flag ds_put_elements, (char *data, unsigned int data_type,
					unsigned int data_stride,
					double *values,
					unsigned int num_values) );
EXTERN_FUNCTION (flag ds_put_element_many_times,
		 (char *data, unsigned int data_type, unsigned int data_stride,
		  double value[2], unsigned int num_elem) );
EXTERN_FUNCTION (flag ds_put_named_element,
		 (CONST packet_desc *pack_desc, char *packet, CONST char *name,
		  double value[2]) );

/*  File:  traverse.c  */
EXTERN_FUNCTION (flag ds_reorder_array, (array_desc *arr_desc,
					 unsigned int order_list[],
					 char *array, flag mod_desc) );
EXTERN_FUNCTION (flag ds_foreach_occurrence, (packet_desc *pack_desc,
					      char *packet, CONST char *item,
					      flag as_whole,
					      flag (*function) ()) );
EXTERN_FUNCTION (flag ds_foreach_in_array, (array_desc *arr_desc,
					    char *array, CONST char *item,
					    flag as_whole,
					    flag (*function) ()) );
EXTERN_FUNCTION (flag ds_foreach_in_list, (packet_desc *list_desc,
					   list_header *list_head,
					   CONST char *item, flag as_whole,
					   flag (*function) ()) );
EXTERN_FUNCTION (flag ds_traverse_and_process, (packet_desc *inp_desc,
						char *inp_data,
						packet_desc *out_desc,
						char *out_data, flag as_whole,
						flag (*function) ()) );
EXTERN_FUNCTION (flag ds_traverse_array, (array_desc *inp_desc,
					  char *inp_data,
					  array_desc *out_desc,
					  char *out_data, flag as_whole,
					  flag (*function) ()) );
EXTERN_FUNCTION (flag ds_traverse_list, (packet_desc *inp_desc,
					 list_header *inp_head,
					 packet_desc *out_desc,
					 list_header *out_head,
					 flag as_whole, flag (*function) ()) );


#endif /*  KARMA_DS_H  */

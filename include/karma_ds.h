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

    Last updated by Richard Gooch   21-JAN-1996

*/

#if !defined(KARMA_DS_DEF_H) || defined(MAKEDEPEND)
#  include <karma_ds_def.h>
#endif

#ifndef KARMA_DS_H
#define KARMA_DS_H


/*  File:   ds_alloc.c   */
EXTERN_FUNCTION (multi_array *ds_alloc_multi,
		 (unsigned int num_arrays) );
EXTERN_FUNCTION (packet_desc *ds_alloc_packet_desc,
		 (unsigned int num_elem) );
EXTERN_FUNCTION (char *ds_alloc_data, (packet_desc *pack_desc, flag clear,
				       flag array_alloc) );
EXTERN_FUNCTION (flag ds_alloc_packet_subdata, (packet_desc *pack_desc,
						char *packet, flag clear,
						flag array_alloc) );
EXTERN_FUNCTION (char *ds_alloc_packet,
		 (packet_desc *pack_descriptor) );
EXTERN_FUNCTION (array_desc *ds_alloc_array_desc,
		 (unsigned int num_dimensions, unsigned int num_levels) );
EXTERN_FUNCTION (flag ds_alloc_tiling_info, (array_desc *arr_desc,
					     unsigned int num_levels) );
EXTERN_FUNCTION (dim_desc *ds_alloc_dim_desc, (CONST char *dim_name,
					       unsigned long length,
					       double min, double max,
					       flag regular) );
EXTERN_FUNCTION (list_header *ds_alloc_list_head, () );
EXTERN_FUNCTION (list_entry *ds_alloc_list_entry,
		 (packet_desc *list_desc, flag array_alloc) );
EXTERN_FUNCTION (flag ds_alloc_array, (array_desc *arr_desc, char *element,
				       flag clear, flag array_alloc) );
EXTERN_FUNCTION (char *ds_easy_alloc_array, (multi_array **multi_desc,
					     unsigned int num_dim,
					     unsigned long *lengths,
					     double *minima, double *maxima,
					     char **names,
					     unsigned int data_type,
					     CONST char *data_name) );
EXTERN_FUNCTION (char *ds_easy_alloc_n_element_array,
		 (multi_array **multi_desc, unsigned int num_dim,
		  unsigned long *lengths, double *minima, double *maxima,
		  char **names, unsigned int num_elements,
		  unsigned int *data_types, char **data_names) );
EXTERN_FUNCTION (multi_array *ds_wrap_preallocated_n_element_array,
		 (char *array, unsigned int num_dim, uaddr *lengths,
		  double *minima, double *maxima, double **coordinates,
		  char **names, unsigned int num_elements,
		  unsigned int *data_types, char **data_names) );
EXTERN_FUNCTION (array_desc *ds_easy_alloc_array_desc,
		 (unsigned int num_dim, uaddr *lengths,
		  double *minima, double *maxima,
		  double **coordinates, char **names,
		  unsigned int num_elements,
		  unsigned int *data_types, char **data_names) );
EXTERN_FUNCTION (flag ds_alloc_contiguous_list, (packet_desc *list_desc,
						 list_header *list_head,
						 unsigned int length,
						 flag clear,
						 flag array_alloc) );


/*  File:  ds_dealloc.c  */
EXTERN_FUNCTION (void ds_dealloc_multi, (multi_array *multi_desc) );
EXTERN_FUNCTION (void ds_dealloc_packet, (packet_desc *pack_desc,
					  char *data) );
EXTERN_FUNCTION (void ds_dealloc_data, (packet_desc *pack_desc,
					char *packet) );
EXTERN_FUNCTION (void ds_dealloc_packet_subdata, (packet_desc *pack_desc,
						  char *packet) );
EXTERN_FUNCTION (void ds_dealloc_array_desc, (array_desc *arr_desc) );
EXTERN_FUNCTION (void ds_dealloc_list, (packet_desc *list_desc,
					list_header *list_head) );
EXTERN_FUNCTION (void ds_dealloc_list_entries, (packet_desc *list_desc,
						list_header *list_head) );
EXTERN_FUNCTION (void ds_dealloc2_list, (list_header *list_head) );
EXTERN_FUNCTION (void ds_dealloc_array, (array_desc *arr_desc,
					 char *arr_element) );


/*  File:   ds_get.c   */
EXTERN_FUNCTION (double ds_convert_atomic,
		 (CONST char *datum, unsigned int datum_type,
		  double *real_out, double *imag_out) );
EXTERN_FUNCTION (double ds_get_coordinate, (dim_desc *dimension,
					    unsigned long coord_num) );
EXTERN_FUNCTION (unsigned int ds_get_element_offset,
		 (packet_desc *pack_desc, unsigned int elem_num) );
EXTERN_FUNCTION (unsigned int ds_get_packet_size,
		 (packet_desc *pack_desc) );
EXTERN_FUNCTION (unsigned long ds_get_array_size, (array_desc *arr_desc) );
EXTERN_FUNCTION (flag ds_packet_all_data, (packet_desc *pack_desc) );
EXTERN_FUNCTION (flag ds_element_is_atomic, (unsigned int element_type) );
EXTERN_FUNCTION (flag ds_element_is_named, (unsigned int element_type) );
EXTERN_FUNCTION (flag ds_element_is_legal, (unsigned int element_type) );
EXTERN_FUNCTION (unsigned int ds_identify_name,
		 (multi_array *multi_desc, CONST char *name,
		  char **encls_desc, unsigned int *index) );
EXTERN_FUNCTION (unsigned int ds_f_array_name, (multi_array *multi_desc,
						CONST char *name,
						char **encls_desc,
						unsigned int *index) );
EXTERN_FUNCTION (unsigned int ds_f_name_in_packet,
		 (packet_desc *pack_desc, CONST char *name,
		  char **encls_desc, unsigned int *index) );
EXTERN_FUNCTION (unsigned int ds_f_name_in_array,
		 (array_desc *arr_desc, CONST char *name, char **encls_desc,
		  unsigned int *index) );
EXTERN_FUNCTION (unsigned int ds_f_elem_in_packet,
		 (packet_desc *pack_desc, CONST char *name) );
EXTERN_FUNCTION (unsigned int ds_find_hole, (packet_desc *inp_desc,
					     packet_desc **out_desc,
					     unsigned int *elem_num) );
EXTERN_FUNCTION (flag ds_compare_packet_desc, (packet_desc *desc1,
					       packet_desc *desc2,
					       flag recursive) );
EXTERN_FUNCTION (flag ds_compare_array_desc, (array_desc *desc1,
					      array_desc *desc2,
					      flag recursive) );
EXTERN_FUNCTION (flag ds_compare_dim_desc, (dim_desc *desc1,
					    dim_desc *desc2) );
EXTERN_FUNCTION (unsigned int ds_f_dim_in_array,
		 (array_desc *arr_desc, CONST char *name) );
EXTERN_FUNCTION (unsigned long ds_get_array_offset,
		 (array_desc *arr_desc, unsigned long *coordinates) );
EXTERN_FUNCTION (unsigned long ds_get_coord_num,
		 (dim_desc *dimension, double coordinate, unsigned int bias) );
EXTERN_FUNCTION (flag ds_get_element,
		 (CONST char *datum, unsigned int datum_type,
		  double *value, flag *complex) );
EXTERN_FUNCTION (flag ds_get_elements, (CONST char *data,
					unsigned int data_type,
					unsigned int data_stride,
					double *values, flag *complex,
					unsigned int num_values) );
EXTERN_FUNCTION (double *ds_get_coordinate_array,
		 (dim_desc *dimension) );
EXTERN_FUNCTION (flag ds_element_is_complex, (unsigned int element_type) );
EXTERN_FUNCTION (flag ds_get_scattered_elements,
		 (CONST char *data, unsigned int data_type,
		  CONST uaddr *offsets, double *values, flag *complex,
		  unsigned int num_values) );
EXTERN_FUNCTION (flag ds_can_transfer_element_as_block, (unsigned int type) );
EXTERN_FUNCTION (flag ds_can_transfer_packet_as_block,
		 (packet_desc *pack_desc) );


/*  File:   ds_put.c  */
EXTERN_FUNCTION (char *ds_put_element, (char *output, unsigned int type,
					double *input) );
EXTERN_FUNCTION (flag ds_put_elements, (char *data, unsigned int data_type,
					unsigned int data_stride,
					double *values,
					unsigned int num_values) );
EXTERN_FUNCTION (flag ds_put_element_many_times, (char *data,
						  unsigned int data_type,
						  unsigned int data_stride,
						  double *value,
						  unsigned int num_elem) );
EXTERN_FUNCTION (flag ds_put_named_element, (packet_desc *pack_desc,
					     char *packet, CONST char *name,
					     double *value) );


/*  File:   ds_copy.c  */
EXTERN_FUNCTION (flag ds_copy_packet, (packet_desc *pack_desc,
				       char *dest_packet,
				       char *source_packet) );
EXTERN_FUNCTION (packet_desc *ds_copy_desc_until,
		 (packet_desc *inp_desc, CONST char *name) );
EXTERN_FUNCTION (array_desc *ds_copy_array_desc_until,
		 (array_desc *inp_desc, CONST char *name) );
EXTERN_FUNCTION (dim_desc *ds_copy_dim_desc,
		 (dim_desc *inp_desc) );
EXTERN_FUNCTION (flag ds_copy_data, (packet_desc *inp_desc,
				     char *inp_data,
				     packet_desc *out_desc,
				     char *out_data) );
EXTERN_FUNCTION (flag ds_copy_array, (array_desc *inp_desc,
				      char *inp_data,
				      array_desc *out_desc,
				      char *out_data) );
EXTERN_FUNCTION (flag ds_copy_list, (packet_desc *inp_desc,
				     list_header *inp_head,
				     packet_desc *out_desc,
				     list_header *out_head) );
EXTERN_FUNCTION (multi_array *ds_select_arrays, 
		 (char **array_list, unsigned int num_in_list,
		  multi_array *multi_desc, flag save_unproc,
		  unsigned int **index_list) );


/*  File:   ds_handle.c  */
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


/*  File:   ds_array.c  */
EXTERN_FUNCTION (flag ds_find_1D_extremes,
		 (CONST char *data, unsigned int num_values, uaddr *offsets,
		  unsigned int elem_type, unsigned int conv_type,
		  double *min, double *max) );
EXTERN_FUNCTION (flag ds_find_2D_extremes,
		 (CONST char *data, unsigned int length1, uaddr *offsets1,
		  unsigned int length2, uaddr *offsets2,
		  unsigned int elem_type, unsigned int conv_type,
		  double *min, double *max) );
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
EXTERN_FUNCTION (flag ds_find_single_histogram,
		 (char *data, unsigned int elem_type, unsigned int conv_type,
		  unsigned int num_values, uaddr *offsets, unsigned int stride,
		  double min, double max, unsigned long num_bins,
		  unsigned long *histogram_array,
		  unsigned long *histogram_peak,
		  unsigned long *histogram_mode) );
EXTERN_FUNCTION (void ds_complex_to_real_1D,
		 (double *out, unsigned int out_stride,
		  double *inp, unsigned int num_values,
		  unsigned int conv_type) );


/*  File:  ds_attach.c  */
EXTERN_FUNCTION (flag ds_put_unique_named_value,
		 (packet_desc *pack_desc, char **packet, CONST char *name,
		  unsigned int type, double *value, flag update) );
EXTERN_FUNCTION (flag ds_put_unique_named_string, (packet_desc *pack_desc,
						   char **packet,
						   CONST char *name,
						   CONST char *string,
						   flag update) );
EXTERN_FUNCTION (flag ds_get_unique_named_value,
		 (packet_desc *pack_desc, char *packet, CONST char *name,
		  unsigned int *type, double value[2]) );
EXTERN_FUNCTION (char *ds_get_unique_named_string, (packet_desc *pack_desc,
						    char *packet,
						    CONST char *name) );


/*  File:   ds_traverse.c  */
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


/*  File:   ds_draw.c  */
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


/*  File:   ds_mod_desc.c  */
EXTERN_FUNCTION (flag ds_remove_dim_desc, (array_desc *arr_desc,
					   CONST char *dim_name) );
EXTERN_FUNCTION (flag ds_append_dim_desc, (array_desc *arr_desc,
					   dim_desc *dimension) );
EXTERN_FUNCTION (flag ds_prepend_dim_desc, (array_desc *arr_desc,
					    dim_desc *dimension) );
EXTERN_FUNCTION (flag ds_compute_array_offsets, (array_desc *arr_desc) );
EXTERN_FUNCTION (void ds_remove_tiling_info, (array_desc *arr_desc) );


/*  File:  ds_list.c  */
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


/*  File:  ds_cmap.c  */
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


#endif /*  KARMA_DS_H  */

/*  karma_kcmap.h

    Header for  kcmap_  package.

    Copyright (C) 1993,1994  Richard Gooch

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
  needed to interface to the kcmap_ routines in the Karma library.


    Written by      Richard Gooch   24-FEB-1993

    Last updated by Richard Gooch   29-NOV-1994

*/

#ifndef KARMA_KCMAP_H
#define KARMA_KCMAP_H


#ifndef KARMA_DS_DEF_H
#  include <karma_ds_def.h>
#endif

#ifndef KARMA_C_DEF_H
#  include <karma_c_def.h>
#endif

#define KCMAP_ATT_END     0  /*  End of varargs list                 */
#define KCMAP_ATT_REVERSE 1  /*  G:(flag *)            S:(flag)      */
#define KCMAP_ATT_INVERT  2  /*  G:(flag *)            S:(flag)      */


typedef struct colourmap_type * Kcolourmap;


#ifndef KDISPLAY_DEFINED
#  define KDISPLAY_DEFINED
typedef struct kdisplay_handle_type * Kdisplay;
#endif


/*  File:   kcmap.c   */
EXTERN_FUNCTION (void kcmap_init, (unsigned int (*alloc_func) (),
				   void (*free_func) (),
				   void (*store_func) (),
				   void (*location_func) () ) );
EXTERN_FUNCTION (void kcmap_add_RGB_func,
		 (char *name,
		  void (*func) (unsigned int num_cells, unsigned short *reds,
				unsigned short *greens, unsigned short *blues,
				unsigned int stride,
				double x, double y, void *var_param),
		  unsigned int min_cells, unsigned int max_cells) );
EXTERN_FUNCTION (Kcolourmap kcmap_create, (char *name,
					   unsigned int num_cells,
					   flag tolerant,
					   Kdisplay dpy_handle) );
EXTERN_FUNCTION (KCallbackFunc kcmap_register_resize_func,
		 (Kcolourmap cmap, void (*resize_func) (), void *info) );
EXTERN_FUNCTION (flag kcmap_change, (Kcolourmap cmap, char *new_name,
				     unsigned int num_cells,
				     flag tolerant) );
EXTERN_FUNCTION (void kcmap_modify, (Kcolourmap cmap, double x, double y,
				     void *var_param) );
EXTERN_FUNCTION (char **kcmap_list_funcs, () );
EXTERN_FUNCTION (char *kcmap_get_active_func, (Kcolourmap cmap) );
EXTERN_FUNCTION (unsigned int kcmap_get_pixels,
		 (Kcolourmap cmap, unsigned long **pixel_values) );
EXTERN_FUNCTION (unsigned long kcmap_get_pixel,
		 (Kcolourmap cmap, unsigned int index) );
EXTERN_FUNCTION (void kcmap_prepare_for_slavery, (Kcolourmap cmap) );
EXTERN_FUNCTION (flag kcmap_copy_to_struct, (Kcolourmap cmap,
					     packet_desc **top_pack_desc,
					     char **top_packet) );
EXTERN_FUNCTION (flag kcmap_copy_from_struct, (Kcolourmap cmap,
					       packet_desc *top_pack_desc,
					       char *top_packet) );
EXTERN_FUNCTION (unsigned short *kcmap_get_rgb_values, (Kcolourmap cmap,
							unsigned int *size) );
#ifndef KCMAP_INTERNAL
EXTERN_FUNCTION (void kcmap_get_attributes, (Kcolourmap cmap, ...) );
EXTERN_FUNCTION (void kcmap_set_attributes, (Kcolourmap cmap, ...) );
#endif


#endif /*  KARMA_KCMAP_H  */

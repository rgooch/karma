/*  karma_viewimg.h

    Header for  viewimg_  package.

    Copyright (C) 1993  Richard Gooch

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
  needed to interface to the viewimg_ routines in the Karma library.


    Written by      Richard Gooch   18-APR-1993

    Last updated by Richard Gooch   18-NOV-1993

*/

#ifndef KARMA_VIEWIMG_H
#define KARMA_VIEWIMG_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#include <karma_iarray.h>
#include <karma_canvas.h>

#ifndef VIEWABLEIMAGE_DEFINED
#define VIEWABLEIMAGE_DEFINED
typedef void * ViewableImage;
#endif


/*  File:   viewimg.c   */
EXTERN_FUNCTION (ViewableImage viewimg_create, (KWorldCanvas canvas,
						multi_array *multi_desc,
						array_desc *arr_desc,
						char *slice,
						unsigned int hdim,
						unsigned int vdim,
						unsigned int elem_index) );
EXTERN_FUNCTION (ViewableImage viewimg_create_from_iarray,
		 (KWorldCanvas canvas, iarray array, flag swap) );
EXTERN_FUNCTION (ViewableImage *viewimg_create_sequence,
		 (KWorldCanvas canvas, multi_array *multi_desc,
		  array_desc *arr_desc, char *cube,
		  unsigned int hdim, unsigned int vdim, unsigned int fdim,
		  unsigned int elem_index) );
EXTERN_FUNCTION (flag viewimg_make_active, (ViewableImage vimage) );
EXTERN_FUNCTION (void viewimg_control_autoscaling,
		 (KWorldCanvas canvas,
		  flag auto_x, flag auto_y, flag auto_v,
		  flag int_x, flag int_y, flag maintain_aspect_ratio) );
EXTERN_FUNCTION (flag viewimg_register_data_change, (ViewableImage vimage) );
EXTERN_FUNCTION (void viewimg_destroy, (ViewableImage vimage) );
EXTERN_FUNCTION (ViewableImage viewimg_get_active, (KWorldCanvas canvas) );
EXTERN_FUNCTION (flag viewimg_fill_ellipse, (ViewableImage vimage,
					     double centre_x, double centre_y,
					     double radius_x, double radius_y,
					     double value[2]) );
EXTERN_FUNCTION (flag viewimg_fill_polygon, (ViewableImage vimage,
					     edit_coord *coords,
					     unsigned int num_vertices,
					     double value[2]) );
EXTERN_FUNCTION (flag viewimg_draw_edit_list, (ViewableImage vimage,
					       KImageEditList ilist) );
EXTERN_FUNCTION (flag viewimg_draw_edit_object, (ViewableImage vimage,
						 char *object) );


#endif /*  KARMA_VIEWIMG_H  */

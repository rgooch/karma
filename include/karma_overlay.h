/*  karma_overlay.h

    Header for  overlay_  package.

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
  needed to interface to the overlay_ routines in the Karma library.


    Written by      Richard Gooch   2-DEC-1993

    Last updated by Richard Gooch   12-NOV-1994

*/

#ifndef KARMA_OVERLAY_H
#define KARMA_OVERLAY_H


#include <karma_canvas.h>
#include <karma_iarray.h>

typedef struct overlay_list_type * KOverlayList;

#define OVERLAY_COORD_PIXEL (unsigned int) 0
#define OVERLAY_COORD_RELATIVE (unsigned int) 1
#define OVERLAY_COORD_WORLD (unsigned int) 2
#define OVERLAY_COORD_LAST (unsigned int) 3


/*  File:  overlay.c  */
EXTERN_FUNCTION (KOverlayList overlay_create_list, (void *info) );
EXTERN_FUNCTION (void overlay_specify_iarray_2d, (KOverlayList olist,
						  iarray array) );
EXTERN_FUNCTION (void overlay_specify_canvas, (KOverlayList olist,
					       KWorldCanvas canvas) );
EXTERN_FUNCTION (flag overlay_associate_display_canvas,
		 (KOverlayList olist, KWorldCanvas canvas) );
EXTERN_FUNCTION (flag overlay_unassociate_display_canvas,
		 (KOverlayList olist, KWorldCanvas canvas) );
EXTERN_FUNCTION (flag overlay_redraw_on_canvas,
		 (KOverlayList olist, KWorldCanvas canvas) );


/*  Drawing routines  */
EXTERN_FUNCTION (unsigned int overlay_line,
		 (KOverlayList olist, unsigned int type0, double x0, double y0,
		  unsigned int type1, double x1, double y1,
		  char *colourname) );
EXTERN_FUNCTION (unsigned int overlay_lines,
		 (KOverlayList olist, unsigned int num_coords,
		  unsigned int *types, double *x_arr, double *y_arr,
		  char *colourname) );
EXTERN_FUNCTION (unsigned int overlay_text,
		 (KOverlayList olist, char *string,
		  unsigned int type, double x, double y,
		  char *colourname, char *fontname, flag clear_under) );
EXTERN_FUNCTION (unsigned int overlay_ellipse,
		 (KOverlayList olist, unsigned int ctype, double cx, double cy,
		  unsigned int rtype, double rx, double ry, char *colourname,
		  flag filled) );
EXTERN_FUNCTION (unsigned int overlay_filled_polygon,
		 (KOverlayList olist, unsigned int num_coords,
		  unsigned int *types, double *x_arr, double *y_arr,
		  char *colourname) );
EXTERN_FUNCTION (unsigned int overlay_vector,
		 (KOverlayList olist, unsigned int stype, double sx, double sy,
		  unsigned int dtype, double dx, double dy,char *colourname) );
EXTERN_FUNCTION (unsigned int overlay_ellipses,
		 (KOverlayList olist, unsigned int num_ellipses,
		  unsigned int *ctypes, double *cx, double *cy,
		  unsigned int *rtypes, double *rx, double *ry,
		  char *colourname, flag filled) );
EXTERN_FUNCTION (unsigned int overlay_segments,
		 (KOverlayList olist, unsigned int num_segments,
		  unsigned int *types0, double *x0, double *y0,
		  unsigned int *types1, double *x1, double *y1,
		  char *colourname) );
EXTERN_FUNCTION (unsigned int overlay_vectors,
		 (KOverlayList olist, unsigned int num_vectors,
		  unsigned int *stypes, double *sx, double *sy,
		  unsigned int *dtypes, double *dx, double *dy,
		  char *colourname) );
EXTERN_FUNCTION (flag overlay_remove_objects, (KOverlayList olist,
					       unsigned int num_objects) );
EXTERN_FUNCTION (flag overlay_remove_object,
		 (KOverlayList olist, unsigned int id_in_list,
		  unsigned int list_id) );
EXTERN_FUNCTION (flag overlay_move_object,
		 (KOverlayList olist, unsigned int id_in_list,
		  unsigned int list_id, double dx, double dy) );


#endif /*  KARMA_OVERLAY_H  */

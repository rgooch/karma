/*LINTLIBRARY*/
/*PREFIX:"viewimg_"*/
/*  viewimg.c

    This code provides ViewableImage objects.

    Copyright (C) 1992,1993  Richard Gooch

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

/*  This file contains all routines needed for manipulating viewable images.


    Written by      Richard Gooch   15-APR-1992

    Updated by      Richard Gooch   26-APR-1993

    Updated by      Richard Gooch   30-APR-1993: Fixed bug in
  viewimg_fill_ellipse  and  viewimg_fill_polygon  routines.

    Updated by      Richard Gooch   3-MAY-1993: Added  viewimg_get_active  .

    Updated by      Richard Gooch   2-JUL-1993: Improved error trapping.

    Updated by      Richard Gooch   18-JUL-1993: Added  maintain_aspect_ratio
  parameter to  viewimg_control_autoscaling  and fixed some scale change code.

    Updated by      Richard Gooch   8-AUG-1993: Documented effect on the
  attachment count of  multi_array  data structures by some routines.

    Updated by      Richard Gooch   16-AUG-1993: Fixed bug in  aspect_zoom
  which could hang trying to compute an impossible zoom factor.

    Last updated by Richard Gooch   31-AUG-1993: Fixed declaration of
  first_canvas_holder  to be static.


*/
#include <stdio.h>
#include <math.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>

#define KWIN_GENERIC_ONLY

typedef struct viewableimage_type * ViewableImage;

#define VIEWABLEIMAGE_DEFINED
#include <karma_viewimg.h>

#define VIMAGE_MAGIC_NUMBER (unsigned int) 229573367
#define HOLDER_MAGIC_NUMBER (unsigned int) 1654545154

#define VERIFY_VIMAGE(vimage) if (vimage == NULL) \
{(void) fprintf (stderr, "NULL viewable image passed\n"); \
 a_prog_bug (function_name); } \
if ( (*vimage).magic_number != VIMAGE_MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid viewable image object\n"); \
 a_prog_bug (function_name); }

typedef struct canvas_holder_type * CanvasHolder;

struct canvas_holder_type
{
    unsigned int magic_number;
    KWorldCanvas canvas;
    ViewableImage first_image;
    ViewableImage active_image;
    CanvasHolder next;
    flag (*position_func) ();
    void *info;
    flag auto_x;
    flag auto_y;
    flag auto_v;
    flag int_x;
    flag int_y;
    flag maintain_aspect_ratio;
};

struct viewableimage_type
{
    double value_min;
    double value_max;
    unsigned int magic_number;
    CanvasHolder canvas_holder;
    multi_array *multi_desc;
    array_desc *arr_desc;
    char *slice;
    unsigned int hdim;
    unsigned int vdim;
    unsigned int elem_index;
    unsigned int hstride;
    unsigned int vstride;
    flag changed;
    int pixcanvas_width;
    int pixcanvas_height;
    KPixCanvasImageCache cache;
    ViewableImage next;
    ViewableImage prev;
    struct win_scale_type win_scale;
};


/*  Private data  */
static CanvasHolder first_canvas_holder = NULL;


/*  Local functions  */
static CanvasHolder get_canvas_holder (/* canvas, alloc */);
static void worldcanvas_refresh_func (/* canvas, width, height, win_scale,
					 info */);
static void recompute_image (/* holder, width, height, win_scale, cmap,
				vimage */);
static void worldcanvas_size_control_func (/* canvas, width, height, win_scale,
					      info */);
static flag worldcanvas_position_func (/* canvas, x, y, event_code, e_info,
					  f_info */);
static void aspect_zoom (/* hlength, width, hpixels,
			    vlength, height, vpixels */);


/* Public functions follow */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_create (canvas, multi_desc, arr_desc, slice,
			      hdim, vdim, elem_index)
/*  This routine will create a viewable image object from a 2-dimensional slice
    of a Karma data structure. At a later time, this viewable image may be made
    visible. This routine will not cause the canvas to be refreshed.
    The world canvas onto which the viewable image may be drawn must be given
    by  canvas  .Many viewable images may be associated with a single canvas.
    The  multi_array  descriptor which contains the Karma data structure must
    be pointed to by  multi_desc  .The routine increments the attachment count
    on the descriptor on successful completion.
    The array descriptor must be pointed to by  arr_desc  .
    The start of the slice data must be pointed to by  slice  .
    The dimension index of the horizontal dimension must be given by  hdim  .
    The dimension index of the vertical dimension must be given by  vdim  .
    The element index of the data packets must be given by  elem_index  .
    The routine may produce cache data which will vastly increase the speed of
    subsequent operations on this data. Prior to process exit, a call MUST be
    made to  viewimg_destroy  ,otherwise shared memory segments could remain
    after the process exits.
    The routine returns a viewable image on success, else it returns NULL.
*/
KWorldCanvas canvas;
multi_array *multi_desc;
array_desc *arr_desc;
char *slice;
unsigned int hdim;
unsigned int vdim;
unsigned int elem_index;
{
    CanvasHolder holder;
    ViewableImage vimage;
    unsigned int dim_count;
    static char function_name[] = "viewimg_create";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL world canvas passed\n");
	a_prog_bug (function_name);
    }
    if (multi_desc == NULL)
    {
	(void) fprintf (stderr,
			"NULL multi_array descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if (arr_desc == NULL)
    {
	(void) fprintf (stderr, "NULL array descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if (slice == NULL)
    {
	(void) fprintf (stderr, "NULL slice pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Sanity checks  */
    if (hdim >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"hdim: %u greater than number of dimensions: %u\n",
			hdim, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    if (vdim >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"vdim: %u greater than number of dimensions: %u\n",
			vdim, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    if (elem_index >= (* (*arr_desc).packet ).num_elements)
    {
	(void) fprintf (stderr,
			"elem_index: %u greater than number of elements: %u\n",
			elem_index, (* (*arr_desc).packet ).num_elements);
	a_prog_bug (function_name);
    }
    if ( (*arr_desc).num_levels > 0 )
    {
	(void) fprintf (stderr, "Tiling not supported yet\n");
	return (NULL);
    }
    if ( (*arr_desc).offsets == NULL )
    {
	if (ds_compute_array_offsets (arr_desc) != TRUE)
	{
	    (void) fprintf (stderr, "Error computing address offsets\n");
	    a_prog_bug (function_name);
	}
    }
    if ( ( holder = get_canvas_holder (canvas, TRUE) ) == NULL ) return (NULL);
    /*  Create new viewable image  */
    if ( ( vimage = (ViewableImage) m_alloc (sizeof *vimage) ) == NULL )
    {
	m_error_notify (function_name, "viewable image");
	return (NULL);
    }
    (*vimage).value_min = TOOBIG;
    (*vimage).value_max = TOOBIG;
    (*vimage).magic_number = VIMAGE_MAGIC_NUMBER;
    (*vimage).canvas_holder = holder;
    (*vimage).multi_desc = multi_desc;
    (*vimage).arr_desc = arr_desc;
    (*vimage).slice = slice;
    (*vimage).hdim = hdim;
    (*vimage).vdim = vdim;
    (*vimage).elem_index = elem_index;
    (*vimage).changed = TRUE;
    (*vimage).pixcanvas_width = -1;
    (*vimage).pixcanvas_height = -1;
    (*vimage).cache = NULL;
    /*  Compute strides  */
    (*vimage).hstride = ds_get_packet_size ( (*arr_desc).packet );
    for (dim_count = (*arr_desc).num_dimensions - 1; dim_count > hdim;
	 --dim_count)
    {
	(*vimage).hstride *= (* (*arr_desc).dimensions[dim_count] ).length;
    }
    (*vimage).vstride = ds_get_packet_size ( (*arr_desc).packet );
    for (dim_count = (*arr_desc).num_dimensions - 1; dim_count > vdim;
	 --dim_count)
    {
	(*vimage).vstride *= (* (*arr_desc).dimensions[dim_count] ).length;
    }
    (*vimage).prev = NULL;
    /*  Do not make the first viewable image for this canvas viewable, else
	the  viewimg_make_active  routine will not be able to detect a change
	in the active viewable image and will do nothing if called with this
	newly created viewable image.
	*/
    if ( (*holder).first_image != NULL )
    {
	/*  Insert at beginning of list  */
	(* (*holder).first_image ).prev = vimage;
    }
    (*vimage).next = (*holder).first_image;
    (*holder).first_image = vimage;
    /*  Attach  */
    ++(*multi_desc).attachments;
    return (vimage);
}   /*  End Function viewimg_create  */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_create_from_iarray (canvas, array, swap)
/*  This function will create a viewable image object from a 2-dimensional
    Intelligant Array. At a later time, this viewable image may be made
    visible. This routine will not cause the canvas to be refreshed.
    The world canvas onto which the viewable image may be drawn must be given
    by  canvas  .Many viewable images may be associated with a single canvas.
    The Intelligent Array must be given by  array  .The underlying  multi_array
    data strucuture will have its attachment count incremented upon successful
    completion.
    If the y axis should be display horizontally, the value of  swap  must be
    TRUE.
    The routine may produce cache data which will vastly increase the speed of
    subsequent operations on this data. Prior to process exit, a call MUST be
    made to  viewimg_destroy  ,otherwise shared memory segments could remain
    after the process exits.
    The routine returns a viewable image on success, else it returns NULL.
*/
KWorldCanvas canvas;
iarray array;
flag swap;
{
    static char function_name[] = "viewimg_create_from_iarray";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL world canvas passed\n");
	a_prog_bug (function_name);
    }
    if (array == NULL)
    {
	(void) fprintf (stderr, "NULL Intelligent Array passed\n");
	a_prog_bug (function_name);
    }
    FLAG_VERIFY (swap);
    if (iarray_num_dim (array) != 2)
    {
	(void) fprintf (stderr,
			"Intelligent Array must have exactly 2 dimensions\n");
	return (NULL);
    }
    if (!(*array).contiguous[0] || !(*array).contiguous[1])
    {
	(void) fprintf (stderr, "Intelligent Array must be contiguous\n");
	return (NULL);
    }
    if ( ( (*array).offsets[0] != (* (*array).arr_desc ).offsets[0] ) ||
	( (*array).offsets[1] != (* (*array).arr_desc ).offsets[1] ) )
    {
	(void) fprintf (stderr, "Intelligent Array must not be a sub-array\n");
	return (NULL);
    }
    if (swap)
    {
	return ( viewimg_create (canvas, (*array).multi_desc,
				(*array).arr_desc, (*array).data,
				 0, 1, 0) );
    }
    else
    {
	return ( viewimg_create (canvas, (*array).multi_desc,
				(*array).arr_desc, (*array).data,
				 1, 0, 0) );
    }
}   /*  End Function viewimg_create_from_iarray  */

/*PUBLIC_FUNCTION*/
flag viewimg_make_active (vimage)
/*  This routine will make a viewable image the active image for its associated
    world canvas. The canvas is then refreshed (possibly resized), provided
    that the new viewable image was not already active.
    The viewable image must be given by  vimage  .
    The routine returns TRUE on success, else it returns FALSE.
*/
ViewableImage vimage;
{
    CanvasHolder holder;
    static char function_name[] = "viewimg_make_active";

    VERIFY_VIMAGE (vimage);
    holder = (*vimage).canvas_holder;
    if (vimage == (*holder).active_image) return (TRUE);
    (*holder).active_image = vimage;
    return ( canvas_resize ( (*holder).canvas, (struct win_scale_type *) NULL,
			    FALSE ) );
}   /*  End Function viewimg_make_active  */

/*PUBLIC_FUNCTION*/
void viewimg_control_autoscaling (canvas, auto_x, auto_y, auto_v, int_x, int_y,
				  maintain_aspect_ratio)
/*  This routine will control the autoscaling options used when viewable images
    are displayed on their associated world canvas.
    The world canvas must be given by  canvas  .
    If  auto_x  is TRUE, then the horizontal window scaling information for the
    canvas is set to the horizontal range of the active viewable image. The
    default is TRUE.
    If  auto_y  is TRUE, then the vertical window scaling information for the
    canvas is set to the vertical range of the active viewable image. The
    default is TRUE.
    If  auto_v  is TRUE, then the value/ colourmap window scaling information
    for the canvas is set to the data range of the active viewable image. The
    default is TRUE.
    If  int_x  is TRUE, then when the number of horizontal data values in a
    viewable image is greater than the number of horizontal pixels (zoom out),
    as many pixels are used so that there are an integral number of data values
    per pixel; if the number of data values is less than the number of pixels
    (zoom in), as many pixels are used so that there are an integral number of
    pixels per data value. The default is TRUE.
    The  int_y  parameter is similar to the  int_x  parameter, except that it
    applies to vertical pixels. The default is TRUE.
    If  maintain_aspect_ratio  is TRUE, then any zoom in or out factors will be
    the same (ie. the image aspect ratio is preserved). The default is FALSE.
    The canvas is not refreshed by this operation.
    The routine returns nothing.
*/
KWorldCanvas canvas;
flag auto_x;
flag auto_y;
flag auto_v;
flag int_x;
flag int_y;
flag maintain_aspect_ratio;
{
    CanvasHolder holder;
    int width, height;
    struct win_scale_type win_scale;
    static char function_name[] = "viewimg_control_autoscaling";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL canvas passed\n");
	a_prog_bug (function_name);
    }
    FLAG_VERIFY (auto_x);
    FLAG_VERIFY (auto_y);
    FLAG_VERIFY (auto_v);
    FLAG_VERIFY (int_x);
    FLAG_VERIFY (int_y);
    FLAG_VERIFY (maintain_aspect_ratio);
    /*  Dummy operation to ensure canvas is OK  */
    canvas_get_size (canvas, &width, &height, &win_scale);
    if ( ( holder = get_canvas_holder (canvas, TRUE) ) == NULL )
    {
	m_abort (function_name, "canvas holder");
    }
    (*holder).auto_x = auto_x;
    (*holder).auto_y = auto_y;
    (*holder).auto_v = auto_v;
    (*holder).int_x = int_x;
    (*holder).int_y = int_y;
    (*holder).maintain_aspect_ratio = maintain_aspect_ratio;
}   /*  End Function viewimg_control_autoscaling  */

/*PUBLIC_FUNCTION*/
flag viewimg_register_data_change (vimage)
/*  This routine will register a change in the Karma data structure associated
    with a viewable image. If the viewable image is active, it will be
    immediately redrawn on its canvas.
    The viewable image must be given by  vimage  .
    The routine returns TRUE on success, else it returns FALSE.
*/
ViewableImage vimage;
{
    CanvasHolder holder;
    static char function_name[] = "viewimg_register_data_change";

    VERIFY_VIMAGE (vimage);
    holder = (*vimage).canvas_holder;
    (*vimage).changed = TRUE;
    (*vimage).value_min = TOOBIG;
    (*vimage).value_max = TOOBIG;
    if (vimage == (*holder).active_image)
    {
	/*  Active image: refresh  */
	return ( canvas_resize ( (*holder).canvas,
				(struct win_scale_type *) NULL, FALSE ) );
    }
    return (TRUE);
}   /*  End Function viewimg_register_data_change  */

/*PUBLIC_FUNCTION*/
void viewimg_destroy (vimage)
/*  This routine will destroy a viewable image. If this is not called prior to
    process exit, shared memory segments could remain after the process exits.
    The viewable image must be given by  vimage  .
    Note that the associated  multi_array  descriptor is also deallocated (or
    at least, the attachment count is decreased).
    The routine returns nothing.
*/
ViewableImage vimage;
{
    CanvasHolder holder;
    static char function_name[] = "viewimg_destroy";

    VERIFY_VIMAGE (vimage);
    holder = (*vimage).canvas_holder;
    kwin_free_cache_data ( (*vimage).cache );
    ds_dealloc_multi ( (*vimage).multi_desc );
    /*  Remove entry  */
    if ( (*vimage).next != NULL )
    {
	/*  Forward entry  */
	(* (*vimage).next ).prev = (*vimage).prev;
    }
    if ( (*vimage).prev != NULL )
    {
	(* (*vimage).prev ).next = (*vimage).next;
    }
    if (vimage == (*holder).first_image)
    {
	/*  Remove from start of list  */
	(*holder).first_image = (*vimage).next;
    }
    if (vimage == (*holder).active_image)
    {
	(*holder).active_image = NULL;
    }
    (*vimage).magic_number = 0;
    m_free ( (char *) vimage );
}   /*  End Function viewimg_destroy  */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_get_active (canvas)
/*  This routine will get the active ViewableImage associated with a
    KWorldCanvas object.
    The canvas must be given by  canvas  .
    The routine returns the active viewable image on success, else it returns
    NULL (indicating no viewable image is active for the canvas).
*/
KWorldCanvas canvas;
{
    CanvasHolder holder;
    int width, height;
    struct win_scale_type win_scale;
    static char function_name[] = "viewimg_get_active";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL canvas passed\n");
	a_prog_bug (function_name);
    }
    /*  Dummy operation to ensure canvas is OK  */
    canvas_get_size (canvas, &width, &height, &win_scale);
    if ( ( holder = get_canvas_holder (canvas, TRUE) ) == NULL )
    {
	m_abort (function_name, "canvas holder");
    }
    return ( (*holder).active_image );
}   /*  End Function viewimg_get_active  */

/*PUBLIC_FUNCTION*/
void viewimg_register_position_event_func (canvas, position_func, f_info)
/*  This routine will register a position event function for a world canvas
    which has a number of ViewableImage objects associated with it.
    The position event function will be called whenever a position event on the
    canvas has not been consumed. Only one position event function may be
    registered per canvas using this function.
    The canvas must be given by  canvas  .
    The function that is called when a position event occurs must be pointed to
    by  position_func  .
    The interface to this routine is as follows:

    flag position_func (vimage, x, y, value, event_code, e_info, f_info)
    *   This routine is a position event consumer for a world canvas which has
        a number of ViewableImage objects associated with it.
        The active viewable image is given by  vimage  .
	The horizontal world co-ordinate of the event will be given by  x  .
	The vertical world co-ordinate of the event will be given by  y  .
	The data value in the viewable image corresponding to the event
	co-ordinates will be written to the storage pointed to by  value  .
	This must be of type  K_DOUBLE  .
	The arbitrary event code is given by  event_code  .
	The arbitrary event information is pointed to by  e_info  .
	The arbitrary function information pointer is pointed to by  f_info  .
	The routine returns TRUE if the event was consumed, else it return
	FALSE indicating that the event is still to be processed.
    *
    ViewableImage vimage;
    double x;
    double y;
    double value[2];
    unsigned int event_code;
    void *e_info;
    void **f_info;

    The initial arbitrary function information pointer must be given by  f_info
    The routine returns nothing.
*/
KWorldCanvas canvas;
flag (*position_func) ();
void *f_info;
{
    CanvasHolder holder;
    static char function_name[] = "viewimg_register_position_func";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL canvas passed\n");
	a_prog_bug (function_name);
    }
    if ( ( holder = get_canvas_holder (canvas, TRUE) ) == NULL )
    {
	m_abort (function_name, "canvas holder");
    }
    if ( (*holder).position_func != NULL )
    {
	(void) fprintf (stderr,
			"Position event function already registered\n");
	a_prog_bug (function_name);
    }
    if (position_func == NULL) return;
    (*holder).position_func = position_func;
    (*holder).info = f_info;
}   /*  End Function viewimg_register_position_func  */

/*PUBLIC_FUNCTION*/
flag viewimg_fill_ellipse (vimage, centre_x, centre_y, radius_x,radius_y,value)
/*  This routine will draw a filled ellipse into a 2 dimensional slice of
    data associated with a viewable image.
    The viewable image must be given by  vimage  .
    The world co-ordinates of the centre of the ellipse must be given by
    centre_x  and centre_y  .
    The radii must be given by  radius_x  and  radius_y  .
    The complex value to fill the ellipse with must be pointed to be  value  .
    This must be of type K_DCOMPLEX.
    The routine returns TRUE on success, else it returns FALSE.
*/
ViewableImage vimage;
double centre_x;
double centre_y;
double radius_x;
double radius_y;
double value[2];
{
    packet_desc *pack_desc;
    array_desc *arr_desc;
    static char function_name[] = "viewimg_fill_ellipse";

    VERIFY_VIMAGE (vimage);
#ifdef ARCH_linux
    if ( (int) value % sizeof (float) != 0 )
#else
    if ( (int) value % sizeof (double) != 0 )
#endif
    {
	(void) fprintf (stderr,
			"value  address: %p not on a double boundary\n",
			value);
	a_prog_bug (function_name);
    }
    pack_desc = (* (*vimage).arr_desc ).packet;
    arr_desc = (*vimage).arr_desc;
    return ( ds_draw_ellipse
	    ( (*vimage).slice,
	     (*pack_desc).element_types[(*vimage).elem_index],
	     (*arr_desc).dimensions[(*vimage).hdim], (*vimage).hstride,
	     (*arr_desc).dimensions[(*vimage).vdim], (*vimage).vstride,
	     centre_x, centre_y, radius_x, radius_y, value ) );
}   /*  End Function viewimg_fill_ellipse  */

/*PUBLIC_FUNCTION*/
flag viewimg_fill_polygon (vimage, coords, num_vertices, value)
/*  This routine will draw a filled polygon into a 2 dimensional slice of data
    associated with a viewable image.
    The viewable image must be given by  vimage  .
    The array of world co-ordinates of vertices of the polygon must be pointed
    to by  coords  .
    The number of vertices in the polygon must be given by  num_vertices  .
    The complex value to fill the polygon with must be pointed to be  value  .
    This must be of type K_DCOMPLEX.
    The routine returns TRUE on success, else it returns FALSE.
*/
ViewableImage vimage;
edit_coord *coords;
unsigned int num_vertices;
double value[2];
{
    packet_desc *pack_desc;
    array_desc *arr_desc;
    static char function_name[] = "viewimg_fill_polygon";

    VERIFY_VIMAGE (vimage);
#ifdef ARCH_linux
    if ( (int) value % sizeof (float) != 0 )
#else
    if ( (int) value % sizeof (double) != 0 )
#endif
    {
	(void) fprintf (stderr,
			"value  address: %p not on a double boundary\n",
			value);
	a_prog_bug (function_name);
    }
    pack_desc = (* (*vimage).arr_desc ).packet;
    arr_desc = (*vimage).arr_desc;
    return ( ds_draw_polygon
	    ( (*vimage).slice,
	     (*pack_desc).element_types[(*vimage).elem_index],
	     (*arr_desc).dimensions[(*vimage).hdim], (*vimage).hstride,
	     (*arr_desc).dimensions[(*vimage).vdim], (*vimage).vstride,
	     coords, num_vertices, value ) );
}   /*  End Function viewimg_fill_polygon  */


/*  Private functions follow  */

static CanvasHolder get_canvas_holder (canvas, alloc)
/*  This routine will get the canvas holder for a world canvas.
    The canvas must be given by  canvas  .
    If the value of  alloc  is TRUE, then if a canvas holder is not found for
    the specified canvas, a new one is allocated.
    The routine returns the canvas holder on success, else it returns NULL.
*/
KWorldCanvas canvas;
flag alloc;
{
    CanvasHolder canvas_holder;
    extern CanvasHolder first_canvas_holder;
    static char function_name[] = "get_canvas_holder";

    /*  First search for existing canvas holder  */
    for (canvas_holder = first_canvas_holder; canvas_holder != NULL;
	 canvas_holder = (*canvas_holder).next)
    {
	if (canvas == (*canvas_holder).canvas) return (canvas_holder);
    }
    if (!alloc) return (NULL);
    /*  Must allocate holder  */
    if ( ( canvas_holder = (CanvasHolder) m_alloc (sizeof *canvas_holder) )
	== NULL )
    {
	m_error_notify (function_name, "canvas holder");
	return (NULL);
    }
    (*canvas_holder).magic_number = HOLDER_MAGIC_NUMBER;
    (*canvas_holder).canvas = canvas;
    (*canvas_holder).first_image = NULL;
    (*canvas_holder).active_image = NULL;
    (*canvas_holder).position_func = NULL;
    (*canvas_holder).info = NULL;
    (*canvas_holder).auto_x = TRUE;
    (*canvas_holder).auto_y = TRUE;
    (*canvas_holder).auto_v = TRUE;
    (*canvas_holder).int_x = TRUE;
    (*canvas_holder).int_y = TRUE;
    (*canvas_holder).maintain_aspect_ratio = FALSE;
    /*  Insert at beginning of list  */
    (*canvas_holder).next = first_canvas_holder;
    first_canvas_holder = canvas_holder;
    canvas_register_refresh_func (canvas, worldcanvas_refresh_func,
				  (void *) canvas_holder);
    
    canvas_register_size_control_func (canvas, worldcanvas_size_control_func,
				       (void *) canvas_holder);
    canvas_register_position_event_func (canvas, worldcanvas_position_func,
					 (void *) canvas_holder);
    return (canvas_holder);
}   /*  End Function get_canvas_holder  */

static void worldcanvas_refresh_func (canvas, width, height, win_scale,
				      cmap, cmap_resize, info)
/*  This routine registers a refresh event for a world canvas.
    The canvas is given by  canvas  .
    The width of the canvas in pixels is given by  width  .
    The height of the canvas in pixels is given by  height  .
    The window scaling information is pointed to by  win_scale  .
    The colourmap associated with the canvas is given by  cmap  .
    If the refresh function was called as a result of a colourmap resize the
    value of  cmap_resize  will be TRUE.
    The arbitrary canvas information pointer is pointed to by  info  .
    The routine returns nothing.
*/
KWorldCanvas canvas;
int width;
int height;
struct win_scale_type *win_scale;
Kcolourmap cmap;
flag cmap_resize;
void **info;
{
    CanvasHolder holder;
    ViewableImage vimage;
    static char function_name[] = "worldcanvas_refresh_func";

    if ( (holder = (CanvasHolder) *info) == NULL )
    {
	(void) fprintf (stderr, "NULL canvas holder\n");
	a_prog_bug (function_name);
    }
    if ( (*holder).magic_number != HOLDER_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid canvas holder object\n");
	a_prog_bug (function_name);
    }
    if (canvas != (*holder).canvas)
    {
	(void) fprintf (stderr, "Different canvas in canvas holder object\n");
	a_prog_bug (function_name);
    }
    if ( (*holder).active_image == NULL )
    {
	return;
    }
    vimage = (*holder).active_image;
    /*  Do tests on image  */
    if ( (*vimage).cache == NULL )
    {
	/*  No image cache  */
	recompute_image (holder, width, height, win_scale, cmap, vimage);
	return;
    }
    if ( (*vimage).changed )
    {
	/*  Data has changed  */
	recompute_image (holder, width, height, win_scale, cmap, vimage);
	return;
    }
    if (cmap_resize)
    {
	/*  Colourmap has changed  */
	recompute_image (holder, width, height, win_scale, cmap, vimage);
	return;
    }
/*
    (void) fprintf (stderr, "%s win_scale: %e %e  %e %e\n",
		    function_name,
		    (*win_scale).x_min, (*win_scale).x_max,
		    (*win_scale).y_min, (*win_scale).y_max);
    (void) fprintf (stderr, "%s vimage.win_scale: %e %e  %e %e\n",
		    function_name,
		    (*vimage).win_scale.x_min, (*vimage).win_scale.x_max,
		    (*vimage).win_scale.y_min, (*vimage).win_scale.y_max);
*/
    /*  More tests  */
    (*vimage).win_scale.x_offset = (*win_scale).x_offset;
    (*vimage).win_scale.y_offset = (*win_scale).y_offset;
    if (bcmp ( (char *) win_scale, (char *) &(*vimage).win_scale,
	      sizeof *win_scale ) != 0)
    {
	/*  Window scaling information has changed  */
/*
	(void) fprintf (stderr, "%s: SPURIOUS SCALE CHANGE\n", function_name);
*/
	recompute_image (holder, width, height, win_scale, cmap, vimage);
	return;
    }
    /*  No significant changes: use image cache  */
    if ( kwin_draw_cached_image ( (*vimage).cache,
				 (*win_scale).x_offset,
				 (*win_scale).y_offset ) )
    {
	/*  Cache OK  */
	return;
    }
    /*  Something wrong with cache  */
    recompute_image (holder, width, height, win_scale, cmap, vimage);
}   /*  End Function worldcanvas_refresh_func  */

static void recompute_image (holder, width, height, win_scale, cmap, vimage)
/*  This routine will recompute a viewable image and then redraw it onto a
    world canvas.
    The canvas holder must be given by  holder  .
    The width of the canvas in pixels must be given by  width  .
    The height of the canvas in pixels must be given by  height  .
    The window scaling information is pointed to by  win_scale  .
    The colourmap associated with the canvas must be given by  cmap  .
    The viewable image must be given by  vimage  .
*/
CanvasHolder holder;
int width;
int height;
struct win_scale_type *win_scale;
Kcolourmap cmap;
ViewableImage vimage;
{
    static char function_name[] = "recompute_image";

    m_copy ( (char *) &(*vimage).win_scale, (char *) win_scale,
	    sizeof *win_scale );
    (*vimage).changed = TRUE;
    if (canvas_draw_image ( (*holder).canvas,
			   (*vimage).arr_desc, (*vimage).slice,
			   (*vimage).hdim, (*vimage).vdim,
			   (*vimage).elem_index,
			   &(*vimage).cache ) != TRUE)
    {
	(void) fprintf (stderr, "Error drawing image onto world canvas\n");
	return;
    }
    (*vimage).changed = FALSE;
}   /*  End Function recompute_image  */

static void worldcanvas_size_control_func (canvas, width, height, win_scale,
					   info)
/*  This routine will modify the window scaling information for a world canvas.
    The canvas is given by  canvas  .
    The width of the canvas in pixels is given by  width  .
    The height of the canvas in pixels is given by  height  .
    The window scaling information is pointed to by  win_scale  .The data
    herein may be modified.
    The arbitrary canvas information pointer is pointed to by  info  .
    The routine returns nothing.
*/
KWorldCanvas canvas;
int width;
int height;
struct win_scale_type *win_scale;
void **info;
{
    CanvasHolder holder;
    ViewableImage vimage;
    int hlength, vlength;
    int factor;
    int coord_num0, coord_num1;
    flag scale_changed = FALSE;
    array_desc *arr_desc;
    packet_desc *pack_desc;
    dim_desc *hdim;
    dim_desc *vdim;
    static char function_name[] = "worldcanvas_size_control_func";

    if ( (holder = (CanvasHolder) *info) == NULL )
    {
	(void) fprintf (stderr, "NULL canvas holder\n");
	a_prog_bug (function_name);
    }
    if ( (*holder).magic_number != HOLDER_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid canvas holder object\n");
	a_prog_bug (function_name);
    }
    if (canvas != (*holder).canvas)
    {
	(void) fprintf (stderr, "Different canvas in canvas holder object\n");
	a_prog_bug (function_name);
    }
    if ( (*holder).active_image == NULL )
    {
	return;
    }
    vimage = (*holder).active_image;
    /*  Work on changing window scaling if need be  */
    arr_desc = (*vimage).arr_desc;
    pack_desc = (*arr_desc).packet;
    hdim = (*arr_desc).dimensions[(*vimage).hdim];
    vdim = (*arr_desc).dimensions[(*vimage).vdim];
/*
    (void) fprintf (stderr, "%s: BEFORE: %e %e  %e %e\n",
		    function_name,
		    (*win_scale).x_min, (*win_scale).x_max,
		    (*win_scale).y_min, (*win_scale).y_max);
*/
    /*  Determine canvas horizontal world co-ordinates  */
    if ( (*holder).auto_x )
    {
	(*win_scale).x_min = (*hdim).minimum;
	(*win_scale).x_max = (*hdim).maximum;
	hlength = (*hdim).length;
    }
    else
    {
	coord_num0 = ds_get_coord_num (hdim, (*win_scale).x_min,
				       SEARCH_BIAS_CLOSEST);
	coord_num1 = ds_get_coord_num (hdim, (*win_scale).x_max,
				       SEARCH_BIAS_CLOSEST);
	if (coord_num1 == coord_num0)
	{
	    /*  Same dimension co-ordinate: separate  */
	    --coord_num0;
	    ++coord_num1;
	}
	(*win_scale).x_min = ds_get_coordinate (hdim, coord_num0);
	(*win_scale).x_max = ds_get_coordinate (hdim, coord_num1);
	hlength = coord_num1 - coord_num0 + 1;
    }
    /*  Determine canvas vertical world co-ordinates  */
    if ( (*holder).auto_y )
    {
	(*win_scale).y_min = (*vdim).minimum;
	(*win_scale).y_max = (*vdim).maximum;
	vlength = (*vdim).length;
    }
    else
    {
	coord_num0 = ds_get_coord_num (vdim, (*win_scale).y_min,
				       SEARCH_BIAS_CLOSEST);
	coord_num1 = ds_get_coord_num (vdim, (*win_scale).y_max,
				       SEARCH_BIAS_CLOSEST);
	if (coord_num1 == coord_num0)
	{
	    /*  Same dimension co-ordinate: separate  */
	    --coord_num0;
	    ++coord_num1;
	}
	(*win_scale).y_min = ds_get_coordinate (vdim, coord_num0);
	(*win_scale).y_max = ds_get_coordinate (vdim, coord_num1);
	vlength = coord_num1 - coord_num0 + 1;
    }
    /*  Determine number of pixels required  */
    if ( (*holder).maintain_aspect_ratio )
    {
	/*  Image data aspect ratio must be preserved: think harder  */
	aspect_zoom (hlength, width, &(*win_scale).x_pixels, (*holder).int_x,
		     vlength, height, &(*win_scale).y_pixels, (*holder).int_y);
	(*win_scale).x_offset = (width - (*win_scale).x_pixels) / 2;
	(*win_scale).y_offset = (height - (*win_scale).y_pixels) / 2;
/*
	(void) fprintf (stderr, "hlength: %d  width: %d  hpixels: %d\n",
			hlength, width, (*win_scale).x_pixels);
	(void) fprintf (stderr, "vlength: %d  height: %d  vpixels: %d\n",
			vlength, height, (*win_scale).y_pixels);
*/
    }
    else
    {
	/* Can compute number of horizontal and vertical pixels independently*/
	/*  Determine number of horizontal pixels required  */
	if ( (*holder).int_x )
	{
	    if (width >= hlength)
	    {
		/*  Zoom in  */
		(*win_scale).x_pixels = (width / hlength) * hlength;
	    }
	    else
	    {
		/*  Zoom out  */
		factor = hlength / width + 1;
		while (hlength % factor != 0) ++factor;
		(*win_scale).x_pixels = hlength / factor;
	    }
	    (*win_scale).x_offset = (width - (*win_scale).x_pixels) / 2;
	}
	else
	{
	    (*win_scale).x_offset = 0;
	    (*win_scale).x_pixels = width;
	}
	/*  Determine number of vertical pixels required  */
	if ( (*holder).int_y )
	{
	    if (height >= vlength)
	    {
		/*  Zoom in  */
		(*win_scale).y_pixels = (height / vlength) * vlength;
	    }
	    else
	    {
		/*  Zoom out  */
		factor = vlength / height + 1;
		while (vlength % factor != 0) ++factor;
		(*win_scale).y_pixels = vlength / factor;
	    }
	    (*win_scale).y_offset = (height - (*win_scale).y_pixels) / 2;
	}
	else
	{
	    (*win_scale).y_offset = 0;
	    (*win_scale).y_pixels = height;
	}
    }
    /*  Now know the number of horizontal and vertical pixels required  */
/*
    (void) fprintf (stderr,
		    "%s: width: %d hlength: %d  height: %d vlength: %d\n",
		    function_name, width, hlength, height, vlength);
*/
    /*  Check if window scaling changed  */
    /*  First check for world co-ordinate change  */
    if ( (*win_scale).x_min != (*vimage).win_scale.x_min )
    {
/*
	(*vimage).win_scale.x_min = (*win_scale).x_min;
*/
	scale_changed = TRUE;
    }
    if ( (*win_scale).x_max != (*vimage).win_scale.x_max )
    {
/*
	(*vimage).win_scale.x_max = (*win_scale).x_max;
*/
	scale_changed = TRUE;
    }
    if ( (*win_scale).y_min != (*vimage).win_scale.y_min )
    {
/*
	(*vimage).win_scale.y_min = (*win_scale).y_min;
*/
	scale_changed = TRUE;
    }
    if ( (*win_scale).y_max != (*vimage).win_scale.y_max )
    {
/*
	(*vimage).win_scale.y_max = (*win_scale).y_max;
*/
	scale_changed = TRUE;
    }
    if (scale_changed)
    {
	(void) fprintf (stderr, "%s: scale changed\n", function_name);
	/*  Precomputed intensity range no longer valid  */
	(*vimage).value_min = TOOBIG;
	(*vimage).value_max = TOOBIG;
	(*vimage).changed = TRUE;
    }
/*
    (void) fprintf (stderr, "%s: AFTER: %e %e  %e %e\n",
		    function_name,
		    (*win_scale).x_min, (*win_scale).x_max,
		    (*win_scale).y_min, (*win_scale).y_max);
*/
    if ( (*holder).auto_v )
    {
	if ( ( (*vimage).value_min >= TOOBIG ) ||
	    ( (*vimage).value_max >= TOOBIG ) )
	{
	    /*  Must compute minimum and maximum values  */
	    (*vimage).value_min = TOOBIG;
	    (*vimage).value_max = -TOOBIG;
	    if (ds_find_plane_extremes
		( (*vimage).slice,
		 (*pack_desc).element_types[(*vimage).elem_index],
		 (*win_scale).conv_type,
		 hdim, (*vimage).hstride, vdim, (*vimage).vstride,
		 (*win_scale).x_min, (*win_scale).x_max,
		 (*win_scale).y_min, (*win_scale).y_max,
		 &(*vimage).value_min, &(*vimage).value_max ) != TRUE)
	    {
		(void) fprintf (stderr, "Error getting data range\n");
		a_prog_bug (function_name);
	    }
	    /*Image intensity range has changed: cached image no longer valid*/
	    (*vimage).changed = TRUE;
	}
	/*  World canvas intensity scale has changed  */
	(*win_scale).z_min = (*vimage).value_min;
	(*win_scale).z_max = (*vimage).value_max;
    }
}   /*  End Function worldcanvas_size_control_func  */

static flag worldcanvas_position_func (canvas, x, y, event_code, e_info,f_info)
/*  This routine is a position event consumer for a world canvas.
    The canvas is given by  canvas  .
    The horizontal world co-ordinate of the event will be given by  x  .
    The vertical world co-ordinate of the event will be given by  y  .
    The arbitrary event code is given by  event_code  .
    The arbitrary event information is pointed to by  e_info  .
    The arbitrary function information pointer is pointed to by  f_info  .
    The routine returns TRUE if the event was consumed, else it return
    FALSE indicating that the event is still to be processed.
*/
KWorldCanvas canvas;
double x;
double y;
unsigned int event_code;
void *e_info;
void **f_info;
{
    CanvasHolder holder;
    ViewableImage vimage;
    unsigned int x_coord, y_coord;
    double dx, dy;
    double value[2];
    array_desc *arr_desc;
    packet_desc *pack_desc;
    dim_desc *hdim;
    dim_desc *vdim;
    static char function_name[] = "wordcanvas_position_func";

    if ( (holder = (CanvasHolder) *f_info) == NULL )
    {
	(void) fprintf (stderr, "NULL canvas holder\n");
	a_prog_bug (function_name);
    }
    if ( (*holder).magic_number != HOLDER_MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid canvas holder object\n");
	a_prog_bug (function_name);
    }
    if (canvas != (*holder).canvas)
    {
	(void) fprintf (stderr, "Different canvas in canvas holder object\n");
	a_prog_bug (function_name);
    }
    if ( (*holder).position_func == NULL ) return (FALSE);
    if ( (*holder).active_image == NULL )
    {
	(void) fprintf (stderr, "No active ViewableImage object for canvas\n");
	(void) fprintf (stderr, "Event code: %u\n", event_code);
	return (FALSE);
    }
    vimage = (*holder).active_image;
    arr_desc = (*vimage).arr_desc;
    pack_desc = (*arr_desc).packet;
    hdim = (*arr_desc).dimensions[(*vimage).hdim];
    vdim = (*arr_desc).dimensions[(*vimage).vdim];
    x_coord = ds_get_coord_num (hdim, x, SEARCH_BIAS_LOWER);
    y_coord = ds_get_coord_num (vdim, y, SEARCH_BIAS_LOWER);
    dx = ds_get_coordinate (hdim, x_coord);
    dy = ds_get_coordinate (vdim, y_coord);
    if (ds_get_element
	( (*vimage).slice + (*arr_desc).offsets[(*vimage).vdim][y_coord] +
	 (*arr_desc).offsets[(*vimage).hdim][x_coord],
	 (*pack_desc).element_types[(*vimage).elem_index],
	 value, (flag *) NULL ) != TRUE)
    {
	(void) fprintf (stderr, "Error converting data\n");
	return (FALSE);
    }
    return ( (* (*holder).position_func ) (vimage, dx, dy, value, event_code,
					   e_info, &(*holder).info) );
}   /*  End Function wordcanvas_position_func  */

static void aspect_zoom (hlength, width, hpixels, int_x,
			 vlength, height, vpixels, int_y)
/*  This routine will compute the number of horizontal and vertical pixels to
    use whilst maintaining the image data aspect ratio.
    The number of horizontal data values must be given by  hlength  .
    The number of horizontal pixels available must be given by  width  .
    The number of horizontal pixels to use will be written to the storage
    pointed to by  hpixels  .
    If the horizontal axis must have integral zooming, the value of  int_x
    must be TRUE.
    The number of vertical data values must be given by  vlength  .
    The number of vertical pixels available must be given by  height  .
    The number of vertical pixels to use will be written to the storage
    pointed to by  vpixels  .
    If the vertical axis must have integral zooming, the value of  int_y
    must be TRUE.
    The routine returns nothing.
*/
int hlength;
int width;
int *hpixels;
flag int_x;
int vlength;
int height;
int *vpixels;
flag int_y;
{
    int hfactor, vfactor;
    int factor;
    static char function_name[] = "aspect_zoom";

    /*  NOTE:  a 1:1 zoom ratio is termed a zoom out of factor 1  */
    /*  Zoom in: replicate image data.    Zoom out: shrink image data  */
    if (int_x && int_y)
    {
	/*  Both axes require integral zoom factors  */
	if ( (width > hlength) && (height > hlength) )
	{
	    /*  Zoom in both axes  */
	    hfactor = width / hlength;
	    vfactor = height / vlength;
	    /*  Use smallest zoom in factor  */
	    factor = (hfactor > vfactor) ? vfactor : hfactor;
	    *hpixels = factor * hlength;
	    *vpixels = factor * vlength;
	    return;
	}
	/*  At least one axis must be zoomed out  */
	if (width < hlength)
	{
	    /*  Horizontal axis must be zoomed out  */
	    hfactor = hlength / width + 1;
	}
	else
	{
	    /*  Zoom out 1:1  */
	    hfactor = 1;
	}
	if (height < vlength)
	{
	    /*  Vertical axis must be zoomed out  */
	    vfactor = vlength / height + 1;
	}
	else
	{
	    /*  Zoom out 1:1  */
	    vfactor = 1;
	}
	/*  Start with largest zoom out factor  */
	factor = (hfactor > vfactor) ? hfactor : vfactor;
	while ( (hlength % factor != 0) || (vlength % factor != 0) )
	{
	    if (++factor > hlength)
	    {
		/*  Too much: fail  */
		(void) fprintf (stderr,
				"Cannot maintain aspect ratio without a bigger window\n");
		*hpixels = width;
		*vpixels = height;
		return;
	    }
	}
	*hpixels = hlength / factor;
	*vpixels = vlength / factor;
	return;
    }
    (void) fprintf (stderr, "Non-integral zooming not supported yet\n");
    a_prog_bug (function_name);
}   /*  End Function aspect_zoom  */

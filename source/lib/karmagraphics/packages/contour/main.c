/*LINTLIBRARY*/
/*  main.c

    This code provides KContourImage objects.

    Copyright (C) 1996  Richard Gooch

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

/*  This file contains all routines needed for manipulating contour images.


    Written by      Richard Gooch   20-JUL-1996

    Updated by      Richard Gooch   22-JUL-1996: Created
  <contour_create_sequence_from_iarray>.

    Updated by      Richard Gooch   4-SEP-1996: More than one contour image may
  be active per canvas.

    Updated by      Richard Gooch   5-SEP-1996: Optimisations when number of
  segments less than NUM_SECTIONS. Fixed bug where number of pixel segments
  less than NUM_SECTIONS was not trapped.

    Updated by      Richard Gooch   13-OCT-1996: Trapped NULL <<pixel_values>>
  in <contour_set_levels>.

    Last updated by Richard Gooch   15-OCT-1996: Trapped NULL
  <<contour_levels>> in several places.


*/
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdarg.h>
#include <karma.h>
#define KWIN_GENERIC_ONLY
#define NEW_WIN_SCALE
#include <karma_contour.h>
#include <karma_iarray.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>


#define CIMAGE_MAGIC_NUMBER (unsigned int) 1342908534
#define HOLDER_MAGIC_NUMBER (unsigned int) 1653453498
#define DEFAULT_COLOUR "green"
#define NUM_SECTIONS 64

#define VERIFY_CIMAGE(cimage) if (cimage == NULL) \
{fprintf (stderr, "NULL contourable image passed\n"); \
 a_prog_bug (function_name); } \
if (cimage->magic_number != CIMAGE_MAGIC_NUMBER) \
{fprintf (stderr, "Invalid contourable image object\n"); \
 a_prog_bug (function_name); }

typedef struct canvas_holder_type * CanvasHolder;
typedef struct sequence_holder_type * SequenceHolder;

struct canvas_holder_type
{
    unsigned int magic_number;
    KWorldCanvas canvas;
    KContourImage first_image;
    CanvasHolder next;
    KCallbackList position_list;
    char *colourname;
    void *info;
};

struct sequence_holder_type
{
    unsigned int attachments;
    unsigned int fdim;
};
struct section_limits_type
{
    double xmin[NUM_SECTIONS];
    double xmax[NUM_SECTIONS];
    double ymin[NUM_SECTIONS];
    double ymax[NUM_SECTIONS];
};

struct contourableimage_type
{
    unsigned int magic_number;
    CanvasHolder canvas_holder;
    flag active;
    /*  Image data  */
    multi_array *multi_desc;
    array_desc *arr_desc;
    char *slice;
    unsigned int hdim;
    unsigned int vdim;
    unsigned int elem_index;
    KwcsAstro astro_projection;
    /*  Contour levels  */
    unsigned int num_levels;
    double *contour_levels;
    unsigned long *contour_pixels;
    /*  Other information  */
    flag world_segs_valid; /*Whether the world co-ordinate segments are valid*/
    SequenceHolder sequence;
    unsigned int num_restrictions;
    char **restriction_names;
    double *restriction_values;
    /*  Cached segments  */
    uaddr world_coord_buf_size;
    unsigned int num_segments;
    double *world_x0;
    double *world_y0;
    double *world_x1;
    double *world_y1;
    int old_x_offset;
    int old_y_offset;
    int old_x_pixels;
    int old_y_pixels;
    double old_left_x;
    double old_right_x;
    double old_bottom_y;
    double old_top_y;
    double test_x[5];
    double test_y[5];
    uaddr pix_coord_buf_size;
    double *linear_world_x0;
    double *linear_world_y0;
    double *linear_world_x1;
    double *linear_world_y1;
    struct section_limits_type linear_limits;
    unsigned int num_pixel_segments;
    double *pix_x0;
    double *pix_y0;
    double *pix_x1;
    double *pix_y1;
    struct section_limits_type pix_limits;
    double *pix_area_x0;
    double *pix_area_y0;
    double *pix_area_x1;
    double *pix_area_y1;
    KContourImage next;
    KContourImage prev;
};


/*  Private data  */
static CanvasHolder first_canvas_holder = NULL;


/*  Private functions  */
STATIC_FUNCTION (CanvasHolder get_canvas_holder,
		 (KWorldCanvas canvas, flag alloc, char *func_name) );
STATIC_FUNCTION (CanvasHolder alloc_canvas_holder, (KWorldCanvas canvas) );
STATIC_FUNCTION (void process_canvas_attributes,
		 (CanvasHolder holder, va_list argp) );
STATIC_FUNCTION (void contour__worldcanvas_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize,
		  void **info, PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );
STATIC_FUNCTION (void refresh_image,
		 (KContourImage cimage, struct win_scale_type *win_scale,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas) );
STATIC_FUNCTION (void compute_pixel_coords,
		 (KContourImage cimage, struct win_scale_type *win_scale,
		  unsigned int num_pix_segs) );
STATIC_FUNCTION (void dealloc_cache, (KContourImage cimage) );


/* Public functions follow */

/*PUBLIC_FUNCTION*/
void contour_init (KWorldCanvas canvas, ...)
/*  [SUMMARY] Initialise the package for a particular canvas.
    [PURPOSE] This routine will initialise the [<contour>] package for a
    particular world canvas. Calling this routine causes a number of callback
    routines internal to the package to be registered with the canvas (such
    as refresh and position event callbacks). This routine must be called
    before creating contour images.
    <canvas> The world canvas object.
    [VARARGS] The list of parameter attribute-key attribute-value-ptr pairs
    must follow. This list must be terminated with the CONTOUR_CANVAS_ATT_END.
    See [<CONTOUR_CANVAS_ATTRIBUTES>] for the list of attributes.
    [RETURNS] Nothing.
*/
{
    va_list argp;
    CanvasHolder holder;
    extern CanvasHolder first_canvas_holder;
    static char function_name[] = "contour_init";

    va_start (argp, canvas);
    if (canvas == NULL)
    {
	fprintf (stderr, "NULL world canvas passed\n");
	a_prog_bug (function_name);
    }
    if (get_canvas_holder (canvas, FALSE, function_name) != NULL)
    {
	fprintf (stderr, "Canvas already initialised\n");
	a_prog_bug (function_name);
    }
    if ( ( holder = alloc_canvas_holder (canvas) ) == NULL )
    {
	m_abort (function_name, "canvas holder");
    }
    process_canvas_attributes (holder, argp);
    va_end (argp);
}   /*  End Function contour_init  */

/*PUBLIC_FUNCTION*/
KContourImage contour_create_restr (KWorldCanvas canvas,
				    multi_array *multi_desc,
				    array_desc *arr_desc, char *slice,
				    unsigned int hdim, unsigned int vdim,
				    unsigned int elem_index,
				    unsigned int num_levels,
				    CONST double *contour_levels,
				    unsigned num_restr,
				    char **restr_names, double *restr_values)
/*  [SUMMARY] Create contour image from 2D slice with restrictions.
    [PURPOSE] This routine will create a contour image object from a
    2-dimensional slice of a Karma data structure. At a later time, this
    contour image may be made visible. This routine will not cause the canvas
    to be refreshed.
    <canvas> The world canvas onto which the contour image may be drawn. Many
    contour images may be associated with a single canvas.
    <multi_desc> The  multi_array  descriptor which contains the Karma data
    structure. The routine increments the attachment count on the descriptor
    on successful completion. This may be NULL.
    <arr_desc> The array descriptor.
    <slice> The start of the slice data.
    <hdim> The dimension index of the horizontal dimension.
    <vdim> The dimension index of the vertical dimension.
    <elem_index> The element index in the data packets.
    <num_levels> The number of contour levels. This may be 0.
    <contour_levels> The array of contour levels.
    <num_restr> The number of matched restrictions.
    <restr_names> The restriction names.
    <restr_values> The restriction values.
    [NOTE] Restriction information is automatically deallocated when
    [<contour_destroy>] is called.
    [RETURNS] A KContourImage object on success, NULL.
*/
{
    CanvasHolder holder;
    KContourImage cimage;
    static char function_name[] = "contour_create_restr";

    if (canvas == NULL)
    {
	fprintf (stderr, "NULL world canvas passed\n");
	a_prog_bug (function_name);
    }
    if (arr_desc == NULL)
    {
	fprintf (stderr, "NULL array descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if (slice == NULL)
    {
	fprintf (stderr, "NULL slice pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Sanity checks  */
    if (hdim >= arr_desc->num_dimensions)
    {
	fprintf (stderr, "hdim: %u greater than number of dimensions: %u\n",
		 hdim, arr_desc->num_dimensions);
	a_prog_bug (function_name);
    }
    if (arr_desc->dimensions[hdim]->coordinates != NULL)
    {
	fprintf (stderr, "hdim: %u not regularly spaced\n", hdim);
	a_prog_bug (function_name);
    }
    if (vdim >= arr_desc->num_dimensions)
    {
	fprintf (stderr, "vdim: %u greater than number of dimensions: %u\n",
		 vdim, arr_desc->num_dimensions);
	a_prog_bug (function_name);
    }
    if (arr_desc->dimensions[vdim]->coordinates != NULL)
    {
	fprintf (stderr, "vdim: %u not regularly spaced\n", vdim);
	a_prog_bug (function_name);
    }
    if (elem_index >= arr_desc->packet->num_elements)
    {
	fprintf (stderr,"elem_index: %u greater than number of elements: %u\n",
		 elem_index, arr_desc->packet->num_elements);
	a_prog_bug (function_name);
    }
    if (arr_desc->offsets == NULL)
    {
	if ( !ds_compute_array_offsets (arr_desc) )
	{
	    fprintf (stderr, "Error computing address offsets\n");
	    a_prog_bug (function_name);
	}
    }
    if ( ( holder = get_canvas_holder (canvas, TRUE, function_name) ) == NULL )
    {
	return (NULL);
    }
    /*  Create new contour image  */
    if ( ( cimage = (KContourImage) m_alloc (sizeof *cimage) ) == NULL )
    {
	m_error_notify (function_name, "contour image");
	return (NULL);
    }
    if (contour_levels == NULL) cimage->contour_levels = NULL;
    else
    {
	if ( ( cimage->contour_levels = (double *)
	       m_dup ( (CONST char *) contour_levels,
		       sizeof *contour_levels * num_levels ) ) == NULL )
	{
	    m_abort (function_name, "contour levels");
	}
    }
    cimage->contour_pixels = NULL;
    cimage->num_levels = num_levels;
    cimage->magic_number = CIMAGE_MAGIC_NUMBER;
    cimage->active = FALSE;
    cimage->canvas_holder = holder;
    cimage->multi_desc = multi_desc;
    cimage->arr_desc = arr_desc;
    cimage->slice = slice + ds_get_element_offset (arr_desc->packet,
						   elem_index);
    cimage->hdim = hdim;
    cimage->vdim = vdim;
    cimage->elem_index = elem_index;
    cimage->astro_projection = wcs_astro_setup (multi_desc->headers[0],
						multi_desc->data[0]);
    cimage->world_segs_valid = FALSE;
    cimage->sequence = NULL;
    cimage->num_restrictions = num_restr;
    cimage->restriction_names = restr_names;
    cimage->restriction_values = restr_values;
    cimage->world_coord_buf_size = 0;
    cimage->world_x0 = NULL;
    cimage->world_y0 = NULL;
    cimage->world_x1 = NULL;
    cimage->world_y1 = NULL;
    cimage->old_x_offset = -1;
    cimage->old_y_offset = -1;
    cimage->old_x_pixels = -1;
    cimage->old_y_pixels = -1;
    cimage->old_left_x = TOOBIG;
    cimage->old_right_x = TOOBIG;
    cimage->old_bottom_y = TOOBIG;
    cimage->old_top_y = TOOBIG;
    cimage->test_x[0] = TOOBIG;
    cimage->pix_coord_buf_size = 0;
    cimage->linear_world_x0 = NULL;
    cimage->linear_world_y0 = NULL;
    cimage->linear_world_x1 = NULL;
    cimage->linear_world_y1 = NULL;
    cimage->pix_x0 = NULL;
    cimage->pix_y0 = NULL;
    cimage->pix_x1 = NULL;
    cimage->pix_y1 = NULL;
    cimage->pix_limits.xmin[0] = TOOBIG;
    cimage->pix_area_x0 = NULL;
    cimage->pix_area_y0 = NULL;
    cimage->pix_area_x1 = NULL;
    cimage->pix_area_y1 = NULL;
    cimage->prev = NULL;
    /*  Do not make the first contour image for this canvas contour, else
	the  contour_make_active  routine will not be able to detect a change
	in the active contour image and will do nothing if called with this
	newly created contour image.
	*/
    if (holder->first_image != NULL)
    {
	/*  Insert at beginning of list  */
	holder->first_image->prev = cimage;
    }
    cimage->next = holder->first_image;
    holder->first_image = cimage;
    /*  Attach  */
    if (multi_desc != NULL) ++multi_desc->attachments;
    return (cimage);
}   /*  End Function contour_create_restr  */

/*PUBLIC_FUNCTION*/
KContourImage contour_create_from_iarray (KWorldCanvas canvas, iarray array,
					  flag swap, unsigned int num_levels,
					  CONST double *contour_levels)
/*  [SUMMARY] Create a contour image from an Intelligent Array.
    [PURPOSE] This routine will create a contour image object from a
    2-dimensional Intelligant Array. At a later time, this contour image may
    be made visible. This routine will not cause the canvas to be refreshed.
    Many contour images may be associated with a single canvas.
    <canvas> The world canvas object.
    <array> The Intelligent Array. The underlying <<multi_array>> data
    structure will have its attachment count incremented upon successful
    completion.
    <swap> If TRUE the y axis will be displayed horizontally.
    <num_levels> The number of contour levels. This may be 0.
    <contour_levels> The array of contour levels.
    [RETURNS] A KContourImage object on success, else NULL.
*/
{
    unsigned int num_restr;
    char **restr_names;
    double *restr_values;
    static char function_name[] = "contour_create_from_iarray";

    if (canvas == NULL)
    {
	fprintf (stderr, "NULL world canvas passed\n");
	a_prog_bug (function_name);
    }
    if (array == NULL)
    {
	fprintf (stderr, "NULL Intelligent Array passed\n");
	a_prog_bug (function_name);
    }
    FLAG_VERIFY (swap);
    if (iarray_num_dim (array) != 2)
    {
	fprintf (stderr, "Intelligent Array must have exactly 2 dimensions\n");
	return (NULL);
    }
    if ( (array->offsets[0] != array->arr_desc->offsets[0]) ||
	(array->offsets[1] != array->arr_desc->offsets[1]) )
    {
	fprintf (stderr, "Intelligent Array must not be a sub-array\n");
	return (NULL);
    }
    num_restr = iarray_get_restrictions (array, &restr_names, &restr_values);
    if (swap)
    {
	return ( contour_create_restr (canvas, array->multi_desc,
				       array->arr_desc, array->data,
				       array->orig_dim_indices[0],
				       array->orig_dim_indices[1],
				       array->elem_index,
				       num_levels, contour_levels,
				       num_restr, restr_names, restr_values) );
    }
    else
    {
	return ( contour_create_restr (canvas, array->multi_desc,
				       array->arr_desc, array->data,
				       array->orig_dim_indices[1],
				       array->orig_dim_indices[0],
				       array->elem_index,
				       num_levels, contour_levels,
				       num_restr, restr_names, restr_values) );
    }
}   /*  End Function contour_create_from_iarray  */

/*PUBLIC_FUNCTION*/
KContourImage *contour_create_sequence (KWorldCanvas canvas,
					multi_array *multi_desc,
					array_desc *arr_desc, char *cube,
					unsigned int hdim, unsigned int vdim,
					unsigned int fdim,
					unsigned int elem_index,
					unsigned int num_levels,
					CONST double *contour_levels)
/*  [SUMMARY] Create a sequence of contour images from a 3D slice.
    [PURPOSE] This routine will create a sequence of contour image objects
    from a 3-dimensional cube of a Karma data structure. At a later time, this
    sequence of contour images may be made visible in any order.
    This routine will not cause the canvas to be refreshed.
    <canvas> The world canvas object.
    <multi_desc> The  multi_array  descriptor which contains the Karma data
    structure. The routine increments the attachment count on the descriptor
    on successful completion. This may be NULL.
    <arr_desc> The array descriptor.
    <cube> The start of the cube data.
    <hdim> The dimension index of the horizontal dimension.
    <vdim> The dimension index of the vertical dimension.
    <fdim> The dimension index of the frame dimension (dimension containing the
    sequence). The number of frames is the same as the length of this
    dimension.
    <elem_index> The element index in the data packets.
    <num_levels> The number of contour levels. This may be 0.
    <contour_levels> The array of contour levels.
    [RETURNS] A pointer to a dynamically allocated array of contour image
    objects on success, else NULL.
*/
{
    unsigned int num_frames;
    unsigned int frame_count;
    unsigned int num_restr = 1;
    KContourImage *cimages;
    uaddr *foffsets;
    char **restr_names;
    double *restr_values;
    static char function_name[] = "contour_create_sequence";

    if (arr_desc == NULL)
    {
	fprintf (stderr, "NULL array descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if (cube == NULL)
    {
	fprintf (stderr, "NULL slice pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Sanity checks  */
    if (fdim >= arr_desc->num_dimensions)
    {
	fprintf (stderr, "fdim: %u greater than number of dimensions: %u\n",
		 fdim, arr_desc->num_dimensions);
	a_prog_bug (function_name);
    }
    if (arr_desc->offsets == NULL)
    {
	if ( !ds_compute_array_offsets (arr_desc) )
	{
	    fprintf (stderr, "Error computing address offsets\n");
	    a_prog_bug (function_name);
	}
    }
    foffsets = arr_desc->offsets[fdim];
    num_frames = arr_desc->dimensions[fdim]->length;
    if ( ( cimages = (KContourImage *) m_alloc (sizeof *cimages * num_frames) )
	== NULL )
    {
	m_error_notify (function_name, "array of contourable images");
	return (NULL);
    }
    for (frame_count = 0; frame_count < num_frames; ++frame_count)
    {
	/*  Allocate restriction  */
	if ( ( restr_names = (char **)
	      m_alloc (sizeof *restr_names * num_restr) ) == NULL )
	{
	    m_abort (function_name, "array of restriction name pointers");
	}
	if ( ( restr_values = (double *)
	      m_alloc (sizeof *restr_values * num_restr) ) == NULL )
	{
	    m_abort (function_name, "array of restriction values");
	}
	restr_values[0] = ds_get_coordinate (arr_desc->dimensions[fdim],
					     frame_count);
	if ( ( restr_names[0] =
	      st_dup (arr_desc->dimensions[fdim]->name) ) == NULL )
	{
	    m_abort (function_name, "restriction name");
	}
	if ( ( cimages[frame_count] =
	      contour_create_restr (canvas, multi_desc, arr_desc,
				    cube + foffsets[frame_count],
				    hdim, vdim, elem_index,
				    num_levels, contour_levels,
				    num_restr, restr_names, restr_values) )
	    == NULL )
	{
	    for (; frame_count > 0; --frame_count)
	    {
		contour_destroy (cimages[frame_count - 1]);
	    }
	    m_free ( (char *) cimages );
	    return (NULL);
	}
    }
    return (cimages);
}   /*  End Function contour_create_sequence  */

/*PUBLIC_FUNCTION*/
KContourImage *contour_create_sequence_from_iarray
    (KWorldCanvas canvas, iarray array,
     unsigned int hdim, unsigned int vdim, unsigned int fdim,
     unsigned int num_levels, CONST double *contour_levels)
/*  [SUMMARY] Create a sequence of contour images from an Intelligent Array.
    [PURPOSE] This routine will create a sequence of contour image objects
    from a 3-dimensional Intelligent Array. At a later time, this sequence of
    contour images may be made visible in any order.
    This routine will not cause the canvas to be refreshed.
    <canvas> The world canvas object.
    <array> The Intelligent Array. The underlying <<multi_array>> data
    structure will have its attachment count incremented upon successful
    completion.
    <hdim> The dimension index of the horizontal dimension.
    <vdim> The dimension index of the vertical dimension.
    <fdim> The dimension index of the frame dimension (dimension containing the
    sequence). The number of frames is the same as the length of this
    dimension.
    <num_levels> The number of contour levels. This may be 0.
    <contour_levels> The array of contour levels.
    [RETURNS] A pointer to a dynamically allocated array of contour image
    objects on success, else NULL.
*/
{
    static char function_name[] = "contour_create_sequence_from_iarray";

    if (canvas == NULL)
    {
	fprintf (stderr, "NULL world canvas passed\n");
	a_prog_bug (function_name);
    }
    if (array == NULL)
    {
	fprintf (stderr, "NULL Intelligent Array passed\n");
	a_prog_bug (function_name);
    }
    if (iarray_num_dim (array) != 3)
    {
	fprintf (stderr, "Intelligent Array must have exactly 3 dimensions\n");
	return (NULL);
    }
    if ( hdim >= iarray_num_dim (array) )
    {
	fprintf (stderr, "hdim: %u too large\n", hdim);
	a_prog_bug (function_name);
    }
    if ( vdim >= iarray_num_dim (array) )
    {
	fprintf (stderr, "vdim: %u too large\n", vdim);
	a_prog_bug (function_name);
    }
    if ( fdim >= iarray_num_dim (array) )
    {
	fprintf (stderr, "fdim: %u too large\n", fdim);
	a_prog_bug (function_name);
    }
    if ( (array->offsets[fdim] != array->arr_desc->offsets[fdim]) ||
	(array->offsets[hdim] != array->arr_desc->offsets[hdim]) ||
	(array->offsets[hdim] != array->arr_desc->offsets[hdim]) )
    {
	fprintf (stderr, "Intelligent Array must not be a sub-array\n");
	return (NULL);
    }
    return ( contour_create_sequence (canvas, array->multi_desc,
				      array->arr_desc, array->data,
				      array->orig_dim_indices[hdim],
				      array->orig_dim_indices[vdim],
				      array->orig_dim_indices[fdim],
				      array->elem_index,
				      num_levels, contour_levels) );
}   /*  End Function contour_create_sequence_from_iarray  */

/*PUBLIC_FUNCTION*/
flag contour_set_active (KContourImage cimage, flag active, flag force_refresh,
			 flag refresh_if_changed, flag exclusive)
/*  [SUMMARY] Set active state for a contour image controlled refresh.
    [PURPOSE] This routine will make a contour image active or inactive.
    <cimage> The contour image.
    <active> If TRUE, the contourable image is made active, else it is made
    inactive.
    <force_refresh> If TRUE, the canvas is always refreshed.
    <refresh_if_changed> If TRUE, the canvas is refreshed if the contourable
    image active state changed.
    <exclusive> If TRUE and <<active>> is TRUE, make this contour image the
    only active one for this canvas.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    CanvasHolder holder;
    KContourImage curr;
    static char function_name[] = "contour_set_active";

    VERIFY_CIMAGE (cimage);
    FLAG_VERIFY (active);
    FLAG_VERIFY (force_refresh);
    FLAG_VERIFY (refresh_if_changed);
    holder = cimage->canvas_holder;
    if (active != cimage->active)
    {
	cimage->active = active;
	if (refresh_if_changed) force_refresh = TRUE;
    }
    if (exclusive && active)
    {
	/*  Make all others inactive  */
	for (curr = holder->first_image; curr != NULL; curr = curr->next)
	{
	    if (curr != cimage) curr->active = FALSE;
	}
    }
    if (force_refresh)
    {
	return ( canvas_resize (holder->canvas,
				(struct win_scale_type *) NULL, FALSE) );
    }
    return (TRUE);
}   /*  End Function contour_set_active  */

/*PUBLIC_FUNCTION*/
flag contour_register_data_change (KContourImage cimage)
/*  [SUMMARY] Notify data for contour image has changed.
    [PURPOSE] This routine will register a change in the Karma data structure
    associated with a contour image. If the contour image is active, it will
    be immediately redrawn on its canvas.
    <cimage> The contour image.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    CanvasHolder holder;
    static char function_name[] = "contour_register_data_change";

    VERIFY_CIMAGE (cimage);
    holder = cimage->canvas_holder;
    cimage->world_segs_valid = FALSE;
    if (cimage->active)
    {
	/*  Active image: refresh  */
	return ( canvas_resize (holder->canvas,
				(struct win_scale_type *) NULL, FALSE) );
    }
    return (TRUE);
}   /*  End Function contour_register_data_change  */

/*PUBLIC_FUNCTION*/
void contour_destroy (KContourImage cimage)
/*  [SUMMARY] Destroy contour image.
    [PURPOSE] This routine will destroy a contour image.
    <cimage> The contour image.
    [NOTE] The associated <<multi_array>> descriptor is also deallocated (or
    at least, the attachment count is decreased).
    [RETURNS] Nothing.
*/
{
    CanvasHolder holder;
    SequenceHolder sequence;
    unsigned int count;
    static char function_name[] = "contour_destroy";

    VERIFY_CIMAGE (cimage);
    holder = cimage->canvas_holder;
    if (cimage->contour_levels != NULL)
	m_free ( (char *) cimage->contour_levels );
    if (cimage->contour_pixels != NULL)
	m_free ( (char *) cimage->contour_pixels );
    ds_dealloc_multi (cimage->multi_desc);
    dealloc_cache (cimage);
    if (cimage->world_x0 != NULL) m_free ( (char *) cimage->world_x0 );
    if (cimage->world_y0 != NULL) m_free ( (char *) cimage->world_y0 );
    if (cimage->world_x1 != NULL) m_free ( (char *) cimage->world_x1 );
    if (cimage->world_y1 != NULL) m_free ( (char *) cimage->world_y1 );
    if (cimage->astro_projection != NULL)
	wcs_astro_destroy (cimage->astro_projection);
    if ( (sequence = cimage->sequence) != NULL )
    {
	if (sequence->attachments > 1)
	{
	    --sequence->attachments;
	}
	else
	{
	    m_free ( (char *) sequence );
	}
    }
    /*  Remove entry  */
    if (cimage->next != NULL)
    {
	/*  Forward entry  */
	cimage->next->prev = cimage->prev;
    }
    if (cimage->prev != NULL)
    {
	cimage->prev->next = cimage->next;
    }
    if (cimage == holder->first_image)
    {
	/*  Remove from start of list  */
	holder->first_image = cimage->next;
    }
    if (cimage->restriction_names != NULL)
    {
	for (count = 0; count < cimage->num_restrictions; ++count)
	{
	    if (cimage->restriction_names[count] != NULL)
	    {
		m_free (cimage->restriction_names[count]);
	    }
	}
	m_free ( (char *) cimage->restriction_names );
    }
    if (cimage->restriction_values != NULL)
    {
	m_free ( (char *) cimage->restriction_values );
    }
    cimage->magic_number = 0;
    m_free ( (char *) cimage );
}   /*  End Function contour_destroy  */

/*PUBLIC_FUNCTION*/
void contour_set_canvas_attributes (KWorldCanvas canvas, ...)
/*  [SUMMARY] Set the contour image attributes for a world canvas.
    <canvas> The world canvas.
    [VARARGS] The list of parameter attribute-key attribute-value-ptr pairs
    must follow. This list must be terminated with the CONTOUR_CANVAS_ATT_END.
    See [<CONTOUR_CANVAS_ATTRIBUTES>] for the list of attributes.
    [NOTE] The canvas is not refreshed by this operation.
    [RETURNS] Nothing.
*/
{
    CanvasHolder holder;
    va_list argp;
    static char function_name[] = "contour_set_canvas_attributes";

    if (canvas == NULL)
    {
	fprintf (stderr, "NULL canvas passed\n");
	a_prog_bug (function_name);
    }
    /*  Dummy operation to ensure canvas is OK  */
    canvas_get_attributes (canvas, CANVAS_ATT_END);
    if ( ( holder = get_canvas_holder (canvas, TRUE, function_name) ) == NULL )
    {
	m_abort (function_name, "canvas holder");
    }
    va_start (argp, canvas);
    process_canvas_attributes (holder, argp);
    va_end (argp);
}   /*  End Function contour_set_canvas_attributes  */

/*PUBLIC_FUNCTION*/
void contour_set_levels (KContourImage cimage, unsigned int num_levels,
			 CONST double *contour_levels,
			 CONST unsigned long *contour_pixels)
/*  [SUMMARY] Set/update the contour levels for a KContourImage object.
    [PURPOSE] This routine will set/update the contour levels for a
    KContourImage object. The canvas is not refreshed by this operation.
    <cimage> The KContourImage object.
    <num_levels> The number of contour levels.
    <contour_levels> The array of contour levels.
    <contour_pixels> The array of contour pixels. This may be NULL.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "contour_set_levels";

    VERIFY_CIMAGE (cimage);
    if (cimage->contour_levels != NULL)
    {
	m_free ( (char *) cimage->contour_levels );
    }
    if (cimage->contour_pixels != NULL)
    {
	m_free ( (char *) cimage->contour_pixels );
    }
    cimage->contour_pixels = NULL;
    if ( ( cimage->contour_levels = (double *)
	   m_dup ( (CONST char *) contour_levels,
		   sizeof *contour_levels * num_levels ) ) == NULL )
    {
	m_abort (function_name, "contour levels");
    }
    if (contour_pixels != NULL)
    {
	if ( ( cimage->contour_pixels = (unsigned long *)
	       m_dup ( (CONST char *) contour_pixels,
		       sizeof *contour_pixels * num_levels ) ) == NULL )
	{
	    m_abort (function_name, "contour pixels");
	}
    }
    cimage->num_levels = num_levels;
    cimage->world_segs_valid = FALSE;
}   /*  End Function contour_set_levels  */

/*PUBLIC_FUNCTION*/
KWorldCanvas contour_get_worldcanvas (KContourImage cimage)
/*  [SUMMARY] Get the world canvas for a contour image.
    <cimage> The KContourImage object.
    [RETURNS] The KWorldCanvas object.
*/
{
    static char function_name[] = "contour_get_worldcanvas";

    VERIFY_CIMAGE (cimage);
    return (cimage->canvas_holder->canvas);
}   /*  End Function contour_get_worldcanvas  */


/*  Private functions follow  */

static CanvasHolder get_canvas_holder (KWorldCanvas canvas, flag alloc,
				       char *func_name)
/*  This routine will get the canvas holder for a world canvas.
    The canvas must be given by  canvas  .
    If the value of  alloc  is TRUE, then if a canvas holder is not found for
    the specified canvas, a new one is allocated.
    The name of the calling function must be pointed to by  func_name  .
    The routine returns the canvas holder on success, else it returns NULL.
*/
{
    CanvasHolder canvas_holder;
    extern CanvasHolder first_canvas_holder;
    /*static char function_name[] = "get_canvas_holder";*/

    /*  First search for existing canvas holder  */
    for (canvas_holder = first_canvas_holder; canvas_holder != NULL;
	 canvas_holder = canvas_holder->next)
    {
	if (canvas == canvas_holder->canvas) return (canvas_holder);
    }
    if (!alloc) return (NULL);
    fprintf (stderr, "%s  called before:  contour_init\n", func_name);
    a_prog_bug (func_name);
    return ( alloc_canvas_holder (canvas) );
}   /*  End Function get_canvas_holder  */

static CanvasHolder alloc_canvas_holder (KWorldCanvas canvas)
/*  This routine will allocate a canvas holder for a world canvas.
    The canvas must be given by  canvas  .
    The routine returns the canvas holder on success, else it returns NULL.
*/
{
    CanvasHolder canvas_holder;
    extern CanvasHolder first_canvas_holder;
    static char function_name[] = "alloc_canvas_holder";

    /*  Must allocate holder  */
    if ( ( canvas_holder = (CanvasHolder) m_alloc (sizeof *canvas_holder) )
	== NULL )
    {
	m_error_notify (function_name, "canvas holder");
	return (NULL);
    }
    canvas_holder->magic_number = HOLDER_MAGIC_NUMBER;
    canvas_holder->canvas = canvas;
    canvas_holder->first_image = NULL;
    canvas_holder->colourname = NULL;
    canvas_holder->position_list = NULL;
    canvas_holder->info = NULL;
    /*  Insert at beginning of list  */
    canvas_holder->next = first_canvas_holder;
    first_canvas_holder = canvas_holder;
    canvas_register_refresh_func (canvas, contour__worldcanvas_refresh_func,
				  (void *) canvas_holder);
    return (canvas_holder);
}   /*  End Function alloc_canvas_holder  */

static void process_canvas_attributes (CanvasHolder holder, va_list argp)
/*  [PURPOSE] This routine will set the attributes for a canvas.
    <holder> The canvas holder.
    <argp> The optional list of parameter attribute-key attribute-value
    pairs.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int att_key;
    char *colour;
    static char function_name[] = "__contour_process_canvas_attributes";

    while ( ( att_key = va_arg (argp, unsigned int) ) !=
	    CONTOUR_CANVAS_ATT_END )
    {
	switch (att_key)
	{
	  case CONTOUR_CANVAS_ATT_COLOURNAME:
	    colour = va_arg (argp, char *);
	    if (colour == NULL) colour = st_dup (DEFAULT_COLOUR);
	    else colour = st_dup (colour);
	    if (colour == NULL) m_abort (function_name, "colour name");
	    if (holder->colourname != NULL) m_free (holder->colourname);
	    holder->colourname = colour;
	    break;
	  default:
	    fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
}   /*  End Function process_canvas_attributes  */

static void contour__worldcanvas_refresh_func(KWorldCanvas canvas,
					      int width, int height,
					      struct win_scale_type *win_scale,
					      Kcolourmap cmap,
					      flag cmap_resize,
					      void **info,
					      PostScriptPage pspage,
					      unsigned int num_areas,
					      KPixCanvasRefreshArea *areas,
					      flag *honoured_areas)
/*  [PURPOSE] This routine processes a refresh event for a world canvas.
    <canvas> The world canvas.
    <width> The width of the canvas in pixels.
    <height> The height of the canvas in pixels.
    <win_scale> A pointer to the window scaling information.
    <cmap> The canvas colourmap.
    <cmap_resize> If TRUE, the refresh is due to a colourmap resize.
    <info> A pointer to the arbitrary information pointer.
    <pspage> The PostScriptPage object. If this is NULL, the refresh is *not*
    destined for a PostScript page.
    <num_areas> The number of areas that need to be refreshed. If this is
    0 then the entire pixel canvas needs to be refreshed.
    <areas> The list of areas that need to be refreshed. Note that these
    areas are given in pixel co-ordinates.
    <honoured_areas> If the value TRUE is written here it is assumed the
    routine honoured the list of refresh areas and did not write outside
    these areas and hence the list of areas will be passed to subsequent
    registered refresh routines. If FALSE is written here (or nothing is
    written here), implying the routine refreshed the entire canvas,
    subsequent refresh routines will be told to refresh the entire canvas.
    [RETURNS] Nothing.
*/
{
    CanvasHolder holder;
    KContourImage cimage;
    static char function_name[] = "__contour_worldcanvas_refresh_func";

    if ( (holder = (CanvasHolder) *info) == NULL )
    {
	fprintf (stderr, "NULL canvas holder\n");
	a_prog_bug (function_name);
    }
    if (holder->magic_number != HOLDER_MAGIC_NUMBER)
    {
	fprintf (stderr, "Invalid canvas holder object\n");
	a_prog_bug (function_name);
    }
    if (canvas != holder->canvas)
    {
	fprintf (stderr, "Different canvas in canvas holder object\n");
	a_prog_bug (function_name);
    }
    /*  Show each active image  */
    for (cimage = holder->first_image; cimage != NULL; cimage = cimage->next)
    {
	if (cimage->active) refresh_image (cimage, win_scale, num_areas,areas);
    }
    *honoured_areas = TRUE;
}   /*  End Function contour__worldcanvas_refresh_func  */

static void refresh_image (KContourImage cimage,
			   struct win_scale_type *win_scale,
			   unsigned int num_areas,KPixCanvasRefreshArea *areas)
/*  [PURPOSE] This routine processes a refresh event for a world canvas.
    <cimage> The KContourImage object.
    <win_scale> A pointer to the window scaling information.
    <num_areas> The number of areas that need to be refreshed. If this is
    0 then the entire pixel canvas needs to be refreshed.
    <areas> The list of areas that need to be refreshed. Note that these
    areas are given in pixel co-ordinates.
    [RETURNS] Nothing.
*/
{
    KWorldCanvas canvas;
    CanvasHolder holder;
    flag section_visible, segment_visible;
    unsigned int count, num_pix_segs, area_count;
    unsigned int section_count, subsection_count, lin_section_size;
    unsigned int pix_section_size;
    unsigned long pixel_value;
    double win_x_min, win_y_min, win_x_max, win_y_max;
    double sec_xmin, sec_ymin, sec_xmax, sec_ymax;
    double area_startx, area_endx, area_starty, area_endy;
    double x0, y0, x1, y1;
    double offset = 1.0;
    double test_x[5], test_y[5];
    CONST packet_desc *pack_desc;
    CONST uaddr *hoffsets, *voffsets;
    CONST dim_desc *hdim, *vdim;
    static char function_name[] = "__contour_worldcanvas_refresh_func";

    if (cimage->num_levels < 1) return;
    holder = cimage->canvas_holder;
    canvas = holder->canvas;
    pack_desc = cimage->arr_desc->packet;
    hdim = cimage->arr_desc->dimensions[cimage->hdim];
    vdim = cimage->arr_desc->dimensions[cimage->vdim];
    hoffsets = cimage->arr_desc->offsets[cimage->hdim];
    voffsets = cimage->arr_desc->offsets[cimage->vdim];
    if (win_scale->left_x < win_scale->right_x)
    {
	win_x_min = win_scale->left_x;
	win_x_max = win_scale->right_x;
    }
    else
    {
	win_x_min = win_scale->right_x;
	win_x_max = win_scale->left_x;
    }
    if (win_scale->bottom_y < win_scale->top_y)
    {
	win_y_min = win_scale->bottom_y;
	win_y_max = win_scale->top_y;
    }
    else
    {
	win_y_min = win_scale->top_y;
	win_y_max = win_scale->bottom_y;
    }
    if (!cimage->world_segs_valid)
    {
	cimage->world_segs_valid = TRUE;
	cimage->old_x_offset = -1;
	cimage->test_x[0] = TOOBIG;
	/*  Compute segments  */
	/*fprintf (stderr, "computing contours...\n");*/
	cimage->num_segments =
	    ds_contour (cimage->slice,
			pack_desc->element_types[cimage->elem_index],
			hdim, hoffsets, vdim, voffsets,
			cimage->num_levels, cimage->contour_levels,
			&cimage->world_coord_buf_size,
			&cimage->world_x0, &cimage->world_y0,
			&cimage->world_x1, &cimage->world_y1);
	if (cimage->num_segments < 1) return;
	if (cimage->astro_projection != NULL)
	{
	    wcs_astro_transform (cimage->astro_projection,
				 cimage->num_segments,
				 cimage->world_x0, FALSE,
				 cimage->world_y0, FALSE, NULL, FALSE,
				 0, NULL, NULL);
	    wcs_astro_transform (cimage->astro_projection,
				 cimage->num_segments,
				 cimage->world_x1, FALSE,
				 cimage->world_y1, FALSE, NULL, FALSE,
				 0, NULL, NULL);
	}
    }
    if (cimage->contour_pixels == NULL)
    {
	if ( !canvas_get_colour (canvas, holder->colourname, &pixel_value,
				 NULL, NULL, NULL) )
	{
	    fprintf (stderr, "%s: error getting colour\n", function_name);
	    return;
	}
    }
    else
    {
	/*  TODO: should use different pixel value for each contour level  */
	pixel_value = cimage->contour_pixels[0];
    }
    if (cimage->num_segments > cimage->pix_coord_buf_size)
    {
	/*  Need to reallocate world co-ordinate buffers  */
	cimage->old_x_offset = -1;
	cimage->test_x[0] = TOOBIG;
	dealloc_cache (cimage);
	if ( ( cimage->linear_world_x0 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "linear x0 array");
	}
	if ( ( cimage->linear_world_y0 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "linear y0 array");
	}
	if ( ( cimage->linear_world_x1 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "linear x1 array");
	}
	if ( ( cimage->linear_world_y1 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "linear y1 array");
	}
	if ( ( cimage->pix_x0 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "pixel x0 array");
	}
	if ( ( cimage->pix_y0 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "pixel y0 array");
	}
	if ( ( cimage->pix_x1 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "pixel x1 array");
	}
	if ( ( cimage->pix_y1 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "pixel y1 array");
	}
	if ( ( cimage->pix_area_x0 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "pixel area x0 array");
	}
	if ( ( cimage->pix_area_y0 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "pixel area y0 array");
	}
	if ( ( cimage->pix_area_x1 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "pixel area x1 array");
	}
	if ( ( cimage->pix_area_y1 = (double *)
	       m_alloc (sizeof (double) * cimage->num_segments) ) == NULL )
	{
	    m_abort (function_name, "pixel area y1 array");
	}
	cimage->pix_coord_buf_size = cimage->num_segments;
    }
#ifdef debug
    canvas_draw_segments_p (canvas, cimage->world_x0, cimage->world_y0,
			    cimage->world_x1, cimage->world_y1,
			    cimage->num_segments, pixel_value);
    return;
#endif
    /*  At this point the contour segments have been computed in world
	co-ordinates. To speed the redisplay, check if the non-linear to linear
	world co-ordinate transformation has changed  */
    test_x[0] = hdim->first_coord;
    test_y[0] = vdim->first_coord;
    test_x[1] = hdim->last_coord;
    test_y[1] = vdim->first_coord;
    test_x[2] = (hdim->first_coord + hdim->last_coord) / 2.0;
    test_y[2] = (vdim->first_coord + vdim->last_coord) / 2.0;
    test_x[3] = hdim->first_coord;
    test_y[3] = vdim->last_coord;
    test_x[4] = hdim->last_coord;
    test_y[4] = vdim->last_coord;
    canvas_coords_transform (canvas, 5, test_x, FALSE, test_y, FALSE);
    if ( m_cmp ( (CONST char *) cimage->test_x, (CONST char *) test_x,
		 sizeof *test_x * 5 ) &&
	 m_cmp ( (CONST char *) cimage->test_y, (CONST char *) test_y,
		 sizeof *test_y * 5 ) )
    {
	/*  Cached linear world co-ordinates are still valid  */
    }
    else
    {
	/*  Have to compute linear world co-ordinates  */
	/*fprintf (stderr, "world co-ords transform changed\n");*/
	m_copy ( (char *) cimage->test_x, (CONST char *) test_x,
		 sizeof *test_x * 5 );
	m_copy ( (char *) cimage->test_y, (CONST char *) test_y,
		 sizeof *test_y * 5 );
	for (count = 0; count < cimage->num_segments; ++count)
	    cimage->linear_world_x0[count] = cimage->world_x0[count];
	for (count = 0; count < cimage->num_segments; ++count)
	    cimage->linear_world_y0[count] = cimage->world_y0[count];
	for (count = 0; count < cimage->num_segments; ++count)
	    cimage->linear_world_x1[count] = cimage->world_x1[count];
	for (count = 0; count < cimage->num_segments; ++count)
	    cimage->linear_world_y1[count] = cimage->world_y1[count];
	canvas_coords_transform (canvas, cimage->num_segments,
				 cimage->linear_world_x0, TRUE,
				 cimage->linear_world_y0, TRUE);
	canvas_coords_transform (canvas, cimage->num_segments,
				 cimage->linear_world_x1, TRUE,
				 cimage->linear_world_y1, TRUE);
	if (cimage->num_segments < NUM_SECTIONS)
	{
	    /*  Not enough segments: the sectioning algorithm will hang  */
	    cimage->num_pixel_segments = cimage->num_segments;
	    /*  Convert linear world segments to pixel segments  */
	    canvas_convert_from_canvas_coords
		(canvas, FALSE, TRUE, cimage->num_pixel_segments,
		 cimage->linear_world_x0, cimage->linear_world_y0,
		 cimage->pix_x0, cimage->pix_y0);
	    canvas_convert_from_canvas_coords
		(canvas, FALSE, TRUE, cimage->num_pixel_segments,
		 cimage->linear_world_x1, cimage->linear_world_y1,
		 cimage->pix_x1, cimage->pix_y1);
	    /*  Draw pixel segments  */
	    kwin_draw_segments_TRANSITION (canvas_get_pixcanvas (canvas),
					   cimage->pix_x0, cimage->pix_y0,
					   cimage->pix_x1, cimage->pix_y1,
					   cimage->num_pixel_segments,
					   pixel_value);
	    /*  Save current world->pixel setup  */
	    cimage->old_x_offset = win_scale->x_offset;
	    cimage->old_y_offset = win_scale->y_offset;
	    cimage->old_x_pixels = win_scale->x_pixels;
	    cimage->old_y_pixels = win_scale->y_pixels;
	    cimage->old_left_x = win_scale->left_x;
	    cimage->old_right_x = win_scale->right_x;
	    cimage->old_bottom_y = win_scale->bottom_y;
	    cimage->old_top_y = win_scale->top_y;
	    return;
	}
	/*  Divide the segment arrays into a number of sections and compute the
	    limits of each section. This can speed later operations. Since all
	    the segments have to be processed anyway, may as well perform the
	    world->pixel selection and conversion as well, even though it would
	    have been done later anyway. It should be faster to combine the
	    operations in one loop, rather than processing all the segments
	    twice  */
	lin_section_size = cimage->num_segments / NUM_SECTIONS;
	section_count = 0;
	sec_xmin = TOOBIG;
	sec_xmax = -TOOBIG;
	sec_ymin = TOOBIG;
	sec_ymax = -TOOBIG;
	for (count = 0, num_pix_segs = 0, subsection_count = 0;
	     count < cimage->num_segments; ++count, ++subsection_count)
	{
	    if (subsection_count >= lin_section_size)
	    {
		/*  End of section: save current section and go to next  */
		cimage->linear_limits.xmin[section_count] = sec_xmin;
		cimage->linear_limits.xmax[section_count] = sec_xmax;
		cimage->linear_limits.ymin[section_count] = sec_ymin;
		cimage->linear_limits.ymax[section_count] = sec_ymax;
		if (++section_count < NUM_SECTIONS)
		{
		    sec_xmin = TOOBIG;
		    sec_xmax = -TOOBIG;
		    sec_ymin = TOOBIG;
		    sec_ymax = -TOOBIG;
		    subsection_count = 0;
		}
		else
		{
		    subsection_count = 1; /* Carry one with the last section */
		    section_count = NUM_SECTIONS - 1;
		}
	    }
	    x0 = cimage->linear_world_x0[count];
	    y0 = cimage->linear_world_y0[count];
	    x1 = cimage->linear_world_x1[count];
	    y1 = cimage->linear_world_y1[count];
	    /*  Update section limits  */
	    if (x0 < sec_xmin) sec_xmin = x0;
	    if (x0 > sec_xmax) sec_xmax = x0;
	    if (y0 < sec_ymin) sec_ymin = y0;
	    if (y0 > sec_ymax) sec_ymax = y0;
	    if (x1 < sec_xmin) sec_xmin = x1;
	    if (x1 > sec_xmax) sec_xmax = x1;
	    if (y1 < sec_ymin) sec_ymin = y1;
	    if (y1 > sec_ymax) sec_ymax = y1;
	    /*  Add segment to pixel list if inside world canvas  */
	    if ( (x0 < win_x_min) && (x1 < win_x_min) ) continue;
	    if ( (x0 > win_x_max) && (x1 > win_x_max) ) continue;
	    if ( (y0 < win_y_min) && (y1 < win_y_min) ) continue;
	    if ( (y0 > win_y_max) && (y1 > win_y_max) ) continue;
	    cimage->pix_x0[num_pix_segs] = x0;
	    cimage->pix_y0[num_pix_segs] = y0;
	    cimage->pix_x1[num_pix_segs] = x1;
	    cimage->pix_y1[num_pix_segs] = y1;
	    ++num_pix_segs;
	}
	/*  Save last section limits  */
	cimage->linear_limits.xmin[section_count] = sec_xmin;
	cimage->linear_limits.xmax[section_count] = sec_xmax;
	cimage->linear_limits.ymin[section_count] = sec_ymin;
	cimage->linear_limits.ymax[section_count] = sec_ymax;
	compute_pixel_coords (cimage, win_scale, num_pix_segs);
    }
    /*  Now the linear world co-ordinates of the segments are valid: check if
	the world->pixel transformation is the same  */
    if ( (cimage->old_x_offset == win_scale->x_offset) &&
	 (cimage->old_y_offset == win_scale->y_offset) &&
	 (cimage->old_x_pixels == win_scale->x_pixels) &&
	 (cimage->old_y_pixels == win_scale->y_pixels) &&
	 (cimage->old_left_x == win_scale->left_x) &&
	 (cimage->old_right_x == win_scale->right_x) &&
	 (cimage->old_bottom_y == win_scale->bottom_y) &&
	 (cimage->old_top_y == win_scale->top_y) )
    {
	/*  It would appear all is the same  */
    }
    else
    {
	if (cimage->num_segments < NUM_SECTIONS)
	{
	    /*  Not enough segments: the sectioning algorithm will hang  */
	    cimage->num_pixel_segments = cimage->num_segments;
	    /*  Convert linear world segments to pixel segments  */
	    canvas_convert_from_canvas_coords
		(canvas, FALSE, TRUE, cimage->num_pixel_segments,
		 cimage->linear_world_x0, cimage->linear_world_y0,
		 cimage->pix_x0, cimage->pix_y0);
	    canvas_convert_from_canvas_coords
		(canvas, FALSE, TRUE, cimage->num_pixel_segments,
		 cimage->linear_world_x1, cimage->linear_world_y1,
		 cimage->pix_x1, cimage->pix_y1);
	    /*  Draw pixel segments  */
	    kwin_draw_segments_TRANSITION (canvas_get_pixcanvas (canvas),
					   cimage->pix_x0, cimage->pix_y0,
					   cimage->pix_x1, cimage->pix_y1,
					   cimage->num_pixel_segments,
					   pixel_value);
	    /*  Save current world->pixel setup  */
	    cimage->old_x_offset = win_scale->x_offset;
	    cimage->old_y_offset = win_scale->y_offset;
	    cimage->old_x_pixels = win_scale->x_pixels;
	    cimage->old_y_pixels = win_scale->y_pixels;
	    cimage->old_left_x = win_scale->left_x;
	    cimage->old_right_x = win_scale->right_x;
	    cimage->old_bottom_y = win_scale->bottom_y;
	    cimage->old_top_y = win_scale->top_y;
	    return;
	}
	/*  Process only those segments which lie inside the canvas  */
	lin_section_size = cimage->num_segments / NUM_SECTIONS;
	for (count = 0, num_pix_segs = 0, section_count = 0,
		 subsection_count = 0;
	     count < cimage->num_segments; ++count, ++subsection_count)
	{
	    if (subsection_count >= lin_section_size)
	    {
		if (++section_count < NUM_SECTIONS) subsection_count = 0;
		else
		{
		    subsection_count = 0;/*Pretend last section not processed*/
		    section_count = NUM_SECTIONS - 1;
		}
	    }
	    if (subsection_count == 0)
	    {
		/*  First time in this section: use the limits to quickly check
		    if any segments should be drawn  */
		if ( (cimage->linear_limits.xmin[section_count] < win_x_min) &&
		     (cimage->linear_limits.xmax[section_count] < win_x_min) )
		{
		    count += lin_section_size - 1;
		    subsection_count += lin_section_size - 1;
		    continue;
		}
		if ( (cimage->linear_limits.xmin[section_count] > win_x_max) &&
		     (cimage->linear_limits.xmax[section_count] > win_x_max) )
		{
		    count += lin_section_size - 1;
		    subsection_count += lin_section_size - 1;
		    continue;
		}
		if ( (cimage->linear_limits.ymin[section_count] < win_y_min) &&
		     (cimage->linear_limits.ymax[section_count] < win_y_min) )
		{
		    count += lin_section_size - 1;
		    subsection_count += lin_section_size - 1;
		    continue;
		}
		if ( (cimage->linear_limits.ymin[section_count] > win_y_max) &&
		     (cimage->linear_limits.ymax[section_count] > win_y_max) )
		{
		    count += lin_section_size - 1;
		    subsection_count += lin_section_size - 1;
		    continue;
		}
	    }
	    x0 = cimage->linear_world_x0[count];
	    y0 = cimage->linear_world_y0[count];
	    x1 = cimage->linear_world_x1[count];
	    y1 = cimage->linear_world_y1[count];
	    if ( (x0 < win_x_min) && (x1 < win_x_min) ) continue;
	    if ( (x0 > win_x_max) && (x1 > win_x_max) ) continue;
	    if ( (y0 < win_y_min) && (y1 < win_y_min) ) continue;
	    if ( (y0 > win_y_max) && (y1 > win_y_max) ) continue;
	    cimage->pix_x0[num_pix_segs] = x0;
	    cimage->pix_y0[num_pix_segs] = y0;
	    cimage->pix_x1[num_pix_segs] = x1;
	    cimage->pix_y1[num_pix_segs] = y1;
	    ++num_pix_segs;
	}
	compute_pixel_coords (cimage, win_scale, num_pix_segs);
    }
    if ( (num_areas < 1) || (cimage->num_pixel_segments < NUM_SECTIONS) )
    {
	/*  Either whole canvas is to be refreshed (hence all cached pixel
	    segments should be drawn) or there are not enough segments to be
	    sectioned, so they are all drawn too  */
	kwin_draw_segments_TRANSITION (canvas_get_pixcanvas (canvas),
				       cimage->pix_x0, cimage->pix_y0,
				       cimage->pix_x1, cimage->pix_y1,
				       cimage->num_pixel_segments,
				       pixel_value);
	return;
    }
    /*  Several refresh areas requested: draw only those areas  */
    pix_section_size = cimage->num_pixel_segments / NUM_SECTIONS;
    for (count = 0, section_count = 0, subsection_count = 0, num_pix_segs = 0;
	 count < cimage->num_pixel_segments; ++count, ++subsection_count)
    {
	if (subsection_count >= pix_section_size)
	{
	    if (++section_count < NUM_SECTIONS) subsection_count = 0;
	    else
	    {
		subsection_count = 0;  /* Pretend last section not processed */
		section_count = NUM_SECTIONS - 1;
	    }
	}
	if ( (subsection_count == 0) &&
	     (cimage->num_pixel_segments >= NUM_SECTIONS) )
	{
	    /*  First time in this section: use the limits to quickly check
		if any segments should be drawn  */
	    section_visible = FALSE;
	    for (area_count = 0; area_count < num_areas; ++area_count)
	    {
		area_startx = (double) areas[area_count].startx - offset;
		area_endx = (double) areas[area_count].endx + offset;
		area_starty = (double) areas[area_count].starty - offset;
		area_endy = (double) areas[area_count].endy + offset;
		if ( (cimage->pix_limits.xmin[section_count] < area_startx) &&
		     (cimage->pix_limits.xmax[section_count] < area_startx) )
		{
		    continue;
		}
		if ( (cimage->pix_limits.xmin[section_count] > area_endx) &&
		     (cimage->pix_limits.xmax[section_count] > area_endx) )
		{
		    continue;
		}
		if ( (cimage->pix_limits.ymin[section_count] < area_starty) &&
		     (cimage->pix_limits.ymax[section_count] < area_starty) )
		{
		    continue;
		}
		if ( (cimage->pix_limits.ymin[section_count] > area_endy) &&
		     (cimage->pix_limits.ymax[section_count] > area_endy) )
		{
		    continue;
		}
		/*  Section is visible in this area  */
		section_visible = TRUE;
		area_count = num_areas;
	    }
	    if (!section_visible)
	    {
		count += pix_section_size - 1;
		subsection_count += pix_section_size - 1;
		continue;
	    }
	    else
	    {
#ifdef dummy
		fprintf (stderr, "num pix segs: %u section: %u\n",
			 cimage->num_pixel_segments, section_count);
		fprintf (stderr, "sec: %e %e to %e %e\n",
			 cimage->pix_limits.xmin[section_count],
			 cimage->pix_limits.ymin[section_count],
			 cimage->pix_limits.xmax[section_count],
			 cimage->pix_limits.ymax[section_count]);
#endif
	    }
	}
	/*  Segment is in a visible section: test if segment itself is
	    visible  */
	x0 = cimage->pix_x0[count];
	y0 = cimage->pix_y0[count];
	x1 = cimage->pix_x1[count];
	y1 = cimage->pix_y1[count];
	segment_visible = FALSE;
	for (area_count = 0; area_count < num_areas; ++area_count)
	{
	    area_startx = (double) areas[area_count].startx - offset;
	    area_endx = (double) areas[area_count].endx + offset;
	    area_starty = (double) areas[area_count].starty - offset;
	    area_endy = (double) areas[area_count].endy + offset;
	    if ( (x0 < area_startx) && (x1 < area_startx) ) continue;
	    if ( (x0 > area_endx) && (x1 > area_endx) ) continue;
	    if ( (y0 < area_starty) && (y1 < area_starty) ) continue;
	    if ( (y0 > area_endy) && (y1 > area_endy) ) continue;
	    /*  Segment is visible in this area  */
	    segment_visible = TRUE;
	    area_count = num_areas;
	}
	if (segment_visible)
	{
	    cimage->pix_area_x0[num_pix_segs] = x0;
	    cimage->pix_area_y0[num_pix_segs] = y0;
	    cimage->pix_area_x1[num_pix_segs] = x1;
	    cimage->pix_area_y1[num_pix_segs] = y1;
	    ++num_pix_segs;
	}
    }
    if (num_pix_segs < 1) return;
    kwin_draw_segments_TRANSITION (canvas_get_pixcanvas (canvas),
				   cimage->pix_area_x0, cimage->pix_area_y0,
				   cimage->pix_area_x1, cimage->pix_area_y1,
				   num_pix_segs, pixel_value);
}   /*  End Function image_refresh  */

static void compute_pixel_coords (KContourImage cimage,
				  struct win_scale_type *win_scale,
				  unsigned int num_pix_segs)
/*  [SUMMARY] Compute pixel co-ordinates.
    <cimage> The KContourImage object.
    <win_scale> The window scaling information.
    <num_pix_segs> The number of pixel segments.
    [RETURNS] Nothing.
*/
{
    unsigned int count, section_count, subsection_count, pix_section_size;
    double sec_xmin, sec_ymin, sec_xmax, sec_ymax;
    double x0, y0, x1, y1;
    KWorldCanvas canvas = cimage->canvas_holder->canvas;
    /*static char function_name[] = "compute_pixel_coords";*/

    /*fprintf (stderr, "world->pixel co-ords conversion changed: %u segs\n",
	     num_pix_segs);*/
    cimage->num_pixel_segments = num_pix_segs;
    canvas_convert_from_canvas_coords (canvas, FALSE, TRUE,
				       cimage->num_pixel_segments,
				       cimage->pix_x0, cimage->pix_y0,
				       cimage->pix_x0, cimage->pix_y0);
    canvas_convert_from_canvas_coords (canvas, FALSE, TRUE,
				       cimage->num_pixel_segments,
				       cimage->pix_x1, cimage->pix_y1,
				       cimage->pix_x1, cimage->pix_y1);
    pix_section_size = cimage->num_pixel_segments / NUM_SECTIONS;
    if (num_pix_segs == cimage->num_segments)
    {
	/*  Since all segments are visible, assume that the order of the
	    segments in sections will be the same for linear world and
	    pixel co-ordinates  */
	canvas_convert_from_canvas_coords (canvas, FALSE, TRUE,
					   NUM_SECTIONS,
					   cimage->linear_limits.xmin,
					   cimage->linear_limits.ymin,
					   cimage->pix_limits.xmin,
					   cimage->pix_limits.ymin);
	canvas_convert_from_canvas_coords (canvas, FALSE, TRUE,
					   NUM_SECTIONS,
					   cimage->linear_limits.xmax,
					   cimage->linear_limits.ymax,
					   cimage->pix_limits.xmax,
					   cimage->pix_limits.ymax);
    }
    else if (cimage->num_pixel_segments >= NUM_SECTIONS)
    {
	/*  Have to compute limits on pixel segments  */
	sec_xmin = TOOBIG;
	sec_xmax = -TOOBIG;
	sec_ymin = TOOBIG;
	sec_ymax = -TOOBIG;
	for (count = 0, section_count = 0, subsection_count = 0;
	     count < num_pix_segs; ++count, ++subsection_count)
	{
	    if (subsection_count >= pix_section_size)
	    {
		/*  End of section: save current section and go to next  */
		cimage->pix_limits.xmin[section_count] = sec_xmin;
		cimage->pix_limits.xmax[section_count] = sec_xmax;
		cimage->pix_limits.ymin[section_count] = sec_ymin;
		cimage->pix_limits.ymax[section_count] = sec_ymax;
		if (++section_count < NUM_SECTIONS)
		{
		    sec_xmin = TOOBIG;
		    sec_xmax = -TOOBIG;
		    sec_ymin = TOOBIG;
		    sec_ymax = -TOOBIG;
		    subsection_count = 0;
		}
		else
		{
		    subsection_count = 1; /* Carry one with the last section */
		    section_count = NUM_SECTIONS - 1;
		}
	    }
	    x0 = cimage->pix_x0[count];
	    y0 = cimage->pix_y0[count];
	    x1 = cimage->pix_x1[count];
	    y1 = cimage->pix_y1[count];
	    /*  Update section limits  */
	    if (x0 < sec_xmin) sec_xmin = x0;
	    if (x0 > sec_xmax) sec_xmax = x0;
	    if (y0 < sec_ymin) sec_ymin = y0;
	    if (y0 > sec_ymax) sec_ymax = y0;
	    if (x1 < sec_xmin) sec_xmin = x1;
	    if (x1 > sec_xmax) sec_xmax = x1;
	    if (y1 < sec_ymin) sec_ymin = y1;
	    if (y1 > sec_ymax) sec_ymax = y1;
	}
	/*  Save last section limits  */
	cimage->pix_limits.xmin[section_count] = sec_xmin;
	cimage->pix_limits.xmax[section_count] = sec_xmax;
	cimage->pix_limits.ymin[section_count] = sec_ymin;
	cimage->pix_limits.ymax[section_count] = sec_ymax;
    }
    cimage->old_x_offset = win_scale->x_offset;
    cimage->old_y_offset = win_scale->y_offset;
    cimage->old_x_pixels = win_scale->x_pixels;
    cimage->old_y_pixels = win_scale->y_pixels;
    cimage->old_left_x = win_scale->left_x;
    cimage->old_right_x = win_scale->right_x;
    cimage->old_bottom_y = win_scale->bottom_y;
    cimage->old_top_y = win_scale->top_y;
}   /*  End Function compute_pixel_coords  */

static void dealloc_cache (KContourImage cimage)
/*  [SUMMARY] Deallocate cached data.
    <cimage> The KContourImage object.
    [RETURNS] Nothing.
*/
{
    if (cimage->linear_world_x0 != NULL)
    {
	m_free ( (char *) cimage->linear_world_x0 );
	m_free ( (char *) cimage->linear_world_y0 );
	m_free ( (char *) cimage->linear_world_x1 );
	m_free ( (char *) cimage->linear_world_y1 );
	m_free ( (char *) cimage->pix_x0 );
	m_free ( (char *) cimage->pix_y0 );
	m_free ( (char *) cimage->pix_x1 );
	m_free ( (char *) cimage->pix_y1 );
	m_free ( (char *) cimage->pix_area_x0 );
	m_free ( (char *) cimage->pix_area_y0 );
	m_free ( (char *) cimage->pix_area_x1 );
	m_free ( (char *) cimage->pix_area_y1 );
    }
    cimage->linear_world_x0 = NULL;
}   /*  End Function dealloc_cache  */

/*LINTLIBRARY*/
/*  main.c

    This code provides ViewableImage objects.

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

/*  This file contains all routines needed for manipulating viewable images.


    Written by      Richard Gooch   15-APR-1993

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

    Updated by      Richard Gooch   31-AUG-1993: Fixed declaration of
  first_canvas_holder  to be static.

    Updated by      Richard Gooch   18-NOV-1993: Added  viewimg_create_sequence

    Updated by      Richard Gooch   23-NOV-1993: Made use of routine
  canvas_register_convert_func  .

    Updated by      Richard Gooch   2-DEC-1993: Added  viewimg_init  routine.
  The use of this will become mandatory in version 2.0 of Karma.

    Updated by      Richard Gooch   4-DEC-1993: Added  viewimg_create_restr  .

    Updated by      Richard Gooch   6-DEC-1993: Made use of  canvas_specify
  routine.

    Updated by      Richard Gooch   11-JAN-1994: Fixed bug in  aspect_zoom  .

    Updated by      Richard Gooch   22-APR-1994: Added  viewimg_test_active  .

    Updated by      Richard Gooch   22-JUL-1994: Dropped message in
  worldcanvas_position_func  which complained about no viewable image active.

    Updated by      Richard Gooch   31-JUL-1994: Fixed bug in
  worldcanvas_size_control_func  where intensity scaling was not correctly set
  for compressed TrueColour images.

    Updated by      Richard Gooch   8-AUG-1994: Added support for 24 bit
  displays.

    Updated by      Richard Gooch   15-SEP-1994: Trapped uniform valued images.

    Updated by      Richard Gooch   24-SEP-1994: Filled in trap for failure to
  compress RGB images in  worldcanvas_size_control_func  .

    Updated by      Richard Gooch   2-OCT-1994: Made use of
  canvas_coord_transform  routine.

    Updated by      Richard Gooch   4-OCT-1994: Changed to  m_cmp  routine in
    order to avoid having to link with buggy UCB compatibility library in
    Slowaris 2.3

    Updated by      Richard Gooch   6-OCT-1994: Fixed
  worldcanvas_size_control_func  to fit world canvas to pixel canvas when no
  ViewableImage is active for world canvas.

    Updated by      Richard Gooch   24-OCT-1994: Created
  viewimg_get_canvas_attributes  and  viewimg_set_canvas_attributes  routines.

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   18-NOV-1994: Added attribute documentation.

    Updated by      Richard Gooch   22-NOV-1994: Created
  viewimg_unregister_position_event_func  .

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/viewimg/main.c

    Updated by      Richard Gooch   29-NOV-1994: Made use of  c_  package.

    Updated by      Richard Gooch   14-DEC-1994: Created  viewimg_set_active
  function.

    Last updated by Richard Gooch   27-DEC-1994: Allow NULL multi_array
  pointers to be passed.


*/
#include <stdio.h>
#include <math.h>
#include <varargs.h>
#include <karma.h>
#define KWIN_GENERIC_ONLY
#define VIEWIMG_INTERNAL
#include <karma_viewimg.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_c.h>


#define VIMAGE_MAGIC_NUMBER (unsigned int) 229573367
#define HOLDER_MAGIC_NUMBER (unsigned int) 1654545154

#define DEFAULT_NUMBER_OF_COLOURS 200

#define VERIFY_VIMAGE(vimage) if (vimage == NULL) \
{(void) fprintf (stderr, "NULL viewable image passed\n"); \
 a_prog_bug (function_name); } \
if ( (*vimage).magic_number != VIMAGE_MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid viewable image object\n"); \
 a_prog_bug (function_name); }

typedef struct canvas_holder_type * CanvasHolder;
typedef struct sequence_holder_type * SequenceHolder;

struct canvas_holder_type
{
    unsigned int magic_number;
    KWorldCanvas canvas;
    ViewableImage first_image;
    ViewableImage active_image;
    CanvasHolder next;
    KCallbackList position_list;
    void *info;
    flag auto_x;
    flag auto_y;
    flag auto_v;
    flag int_x;
    flag int_y;
    flag maintain_aspect_ratio;
    packet_desc *old_cmap;
};

struct sequence_holder_type
{
    unsigned int attachments;
    unsigned int fdim;
};

struct viewableimage_type
{
    double value_min;
    double value_max;
    unsigned int magic_number;
    CanvasHolder canvas_holder;
    /*  PseudoColour (single channel) image data  */
    multi_array *pc_multi_desc;
    array_desc *pc_arr_desc;
    char *pc_slice;
    unsigned int pc_hdim;
    unsigned int pc_vdim;
    unsigned int pc_elem_index;
    unsigned int pc_hstride;
    unsigned int pc_vstride;
    multi_array *cmap_multi_desc;
    unsigned int cmap_array_num;
    /*  TrueColour image data  */
    multi_array *tc_multi_desc;
    array_desc *tc_arr_desc;  /*  If this is not NULL, the original  */
    char *tc_slice;           /*  image data is TrueColour           */
    unsigned int tc_hdim;
    unsigned int tc_vdim;
    unsigned int tc_red_index;
    unsigned int tc_green_index;
    unsigned int tc_blue_index;
    unsigned int tc_hstride;
    unsigned int tc_vstride;
    /*  Other information  */
    flag recompute;
    flag changed;
    int pixcanvas_width;
    int pixcanvas_height;
    SequenceHolder sequence;
    KPixCanvasImageCache cache;
    unsigned int num_restrictions;
    char **restriction_names;
    double *restriction_values;
    ViewableImage next;
    ViewableImage prev;
    struct win_scale_type win_scale;
};

struct position_struct
{
    double x;
    double y;
    double *value;
    int event_code;
    void *e_info;
    double x_lin;
    double y_lin;
};


/*  Private data  */
static CanvasHolder first_canvas_holder = NULL;


/*  Local functions  */
void viewimg_get_canvas_attributes ();
void viewimg_set_canvas_attributes ();


/*  Private functions  */
STATIC_FUNCTION (CanvasHolder get_canvas_holder,
		 (KWorldCanvas canvas, flag alloc, char *func_name) );
STATIC_FUNCTION (CanvasHolder alloc_canvas_holder, (KWorldCanvas canvas) );
STATIC_FUNCTION (void worldcanvas_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize, void **info) );
STATIC_FUNCTION (void recompute_image,
		 (CanvasHolder holder, int width, int height,
		  struct win_scale_type *win_scale, Kcolourmap cmap,
		  ViewableImage vimage) );
STATIC_FUNCTION (void worldcanvas_size_control_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale, void **info,
		  flag *boundary_clear) );
STATIC_FUNCTION (flag worldcanvas_position_func,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info, void **f_info,
		  double x_lin, double y_lin) );
STATIC_FUNCTION (flag position_event_func,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );
STATIC_FUNCTION (void aspect_zoom,
		 (int hlength, int width, int *hpixels, flag int_x,
		  int vlength, int height, int *vpixels, flag int_y) );
STATIC_FUNCTION (flag coord_convert_func,
		 (KWorldCanvas canvas, struct win_scale_type *win_scale,
		  int *px, int *py, double *wx, double *wy,
		  flag to_world, void **info) );


/* Public functions follow */

/*PUBLIC_FUNCTION*/
void viewimg_init (KWorldCanvas canvas)
/*  This routine will initialise the  viewimg_  package for a particular
    world canvas. Calling this routine causes a number of callback routines
    internal to the  viewimg_  package to be registered with the canvas (such
    as refresh and position event callbacks). The use of this routine is
    optional at the moment: the routines which create viewable images perform
    this function automatically. In version 2.0 of Karma, this use of this
    routine before creating viewable images will become mandatory.
    The world canvas must be given by  canvas  .
    The routine returns nothing.
*/
{
    CanvasHolder holder;
    extern CanvasHolder first_canvas_holder;
    static char function_name[] = "viewimg_init";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL world canvas passed\n");
	a_prog_bug (function_name);
    }
    if (get_canvas_holder (canvas, FALSE, function_name) != NULL)
    {
	(void) fprintf (stderr, "Canvas already initialised\n");
	a_prog_bug (function_name);
    }
    if (alloc_canvas_holder (canvas) == NULL)
    {
	m_abort (function_name, "canvas holder");
    }
}   /*  End Function viewimg_init  */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_create_restr (KWorldCanvas canvas,
				    multi_array *multi_desc,
				    array_desc *arr_desc, char *slice,
				    unsigned int hdim, unsigned int vdim,
				    unsigned int elem_index,
				    unsigned num_restr,
				    char **restr_names, double *restr_values)
/*  [PURPOSE] This routine will create a PseudoColour viewable image object
    from a 2-dimensional slice of a Karma data structure. At a later time, this
    viewable image may be made visible. This routine will not cause the canvas
    to be refreshed.
    <canvas> The world canvas onto which the viewable image may be drawn. Many
    viewable images may be associated with a single canvas.
    <multi_desc> The  multi_array  descriptor which contains the Karma data
    structure. The routine increments the attachment count on the descriptor
    on successful completion. This may be NULL.
    <arr_desc> The array descriptor.
    <slice> The start of the slice data.
    <hdim> The dimension index of the horizontal dimension.
    <vdim> The dimension index of the vertical dimension.
    <elem_index> The element index in the data packets.
    <num_restr> The number of matched restrictions. If this is 0, no
    restrictions are recorded (this is the same as calling  viewimg_create  ).
    <restr_names> The restriction names.
    <restr_values> The restriction values.
    [NOTES] Restriction information is automatically deallocated when
    viewimg_destroy  is called.
    [NOTES] The routine may produce cache data which will vastly increase the
    speed of subsequent operations on this data. Prior to process exit, a call
    MUST be made to  viewimg_destroy  ,otherwise shared memory segments could
    remain after the process exits.
    [RETURNS] A viewable image on success, NULL.
*/
{
    CanvasHolder holder;
    ViewableImage vimage;
    unsigned int dim_count;
    static char function_name[] = "viewimg_create_restr";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL world canvas passed\n");
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
    if ( (* (*arr_desc).dimensions[hdim] ).coordinates != NULL )
    {
	(void) fprintf (stderr, "hdim: %u not regularly spaced\n", hdim);
	a_prog_bug (function_name);
    }
    if (vdim >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"vdim: %u greater than number of dimensions: %u\n",
			vdim, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    if ( (* (*arr_desc).dimensions[vdim] ).coordinates != NULL )
    {
	(void) fprintf (stderr, "vdim: %u not regularly spaced\n", vdim);
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
	(void) fprintf (stderr, "Tiling not supported.\n");
	return (NULL);
    }
    if ( (*arr_desc).offsets == NULL )
    {
	if ( !ds_compute_array_offsets (arr_desc) )
	{
	    (void) fprintf (stderr, "Error computing address offsets\n");
	    a_prog_bug (function_name);
	}
    }
    if ( ( holder = get_canvas_holder (canvas, TRUE, function_name) ) == NULL )
    {
	return (NULL);
    }
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
    (*vimage).pc_multi_desc = multi_desc;
    (*vimage).pc_arr_desc = arr_desc;
    (*vimage).pc_slice = slice;
    (*vimage).pc_hdim = hdim;
    (*vimage).pc_vdim = vdim;
    (*vimage).pc_elem_index = elem_index;
    (*vimage).tc_multi_desc = NULL;
    (*vimage).tc_arr_desc = NULL;
    (*vimage).tc_slice = NULL;
    (*vimage).recompute = TRUE;
    (*vimage).changed = TRUE;
    (*vimage).pixcanvas_width = -1;
    (*vimage).pixcanvas_height = -1;
    (*vimage).cache = NULL;
    /*  Compute strides  */
    (*vimage).pc_hstride = ds_get_packet_size ( (*arr_desc).packet );
    for (dim_count = (*arr_desc).num_dimensions - 1; dim_count > hdim;
	 --dim_count)
    {
	(*vimage).pc_hstride *= (* (*arr_desc).dimensions[dim_count] ).length;
    }
    (*vimage).pc_vstride = ds_get_packet_size ( (*arr_desc).packet );
    for (dim_count = (*arr_desc).num_dimensions - 1; dim_count > vdim;
	 --dim_count)
    {
	(*vimage).pc_vstride *= (* (*arr_desc).dimensions[dim_count] ).length;
    }
    (*vimage).sequence = NULL;
    (*vimage).num_restrictions = num_restr;
    (*vimage).restriction_names = restr_names;
    (*vimage).restriction_values = restr_values;
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
    if (multi_desc != NULL) ++(*multi_desc).attachments;
    return (vimage);
}   /*  End Function viewimg_create_restr  */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_create (KWorldCanvas canvas, multi_array *multi_desc,
			      array_desc *arr_desc, char *slice,
			      unsigned int hdim, unsigned int vdim,
			      unsigned int elem_index)
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
    The element index in the data packets must be given by  elem_index  .
    The routine may produce cache data which will vastly increase the speed of
    subsequent operations on this data. Prior to process exit, a call MUST be
    made to  viewimg_destroy  ,otherwise shared memory segments could remain
    after the process exits.
    The routine returns a viewable image on success, else it returns NULL.
*/
{
    CanvasHolder holder;
    ViewableImage vimage;
    unsigned int dim_count;
    static char function_name[] = "viewimg_create";

    return ( viewimg_create_restr (canvas, multi_desc, arr_desc, slice,
				   hdim, vdim, elem_index,
				   0, (char **) NULL, (double *) NULL) );
}   /*  End Function viewimg_create  */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_create_from_iarray (KWorldCanvas canvas, iarray array,
					  flag swap)
/*  This function will create a viewable image object from a 2-dimensional
    Intelligant Array. At a later time, this viewable image may be made
    visible. This routine will not cause the canvas to be refreshed.
    The world canvas onto which the viewable image may be drawn must be given
    by  canvas  .Many viewable images may be associated with a single canvas.
    The Intelligent Array must be given by  array  .The underlying  multi_array
    data strucuture will have its attachment count incremented upon successful
    completion.
    If the y axis should be displayed horizontally, the value of  swap  must be
    TRUE.
    The routine may produce cache data which will vastly increase the speed of
    subsequent operations on this data. Prior to process exit, a call MUST be
    made to  viewimg_destroy  ,otherwise shared memory segments could remain
    after the process exits.
    The routine returns a viewable image on success, else it returns NULL.
*/
{
    ViewableImage vimage;
    unsigned int num_restr;
    char **restr_names;
    double *restr_values;
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
    num_restr = iarray_get_restrictions (array, &restr_names, &restr_values);
    if (swap)
    {
	return ( viewimg_create_restr (canvas, (*array).multi_desc,
				       (*array).arr_desc, (*array).data,
				       (*array).orig_dim_indices[0],
				       (*array).orig_dim_indices[1],
				       (*array).elem_index,
				       num_restr, restr_names, restr_values) );
    }
    else
    {
	return ( viewimg_create_restr (canvas, (*array).multi_desc,
				       (*array).arr_desc, (*array).data,
				       (*array).orig_dim_indices[1],
				       (*array).orig_dim_indices[0],
				       (*array).elem_index,
				       num_restr, restr_names, restr_values) );
    }
}   /*  End Function viewimg_create_from_iarray  */

/*PUBLIC_FUNCTION*/
ViewableImage *viewimg_create_sequence (KWorldCanvas canvas,
					multi_array *multi_desc,
					array_desc *arr_desc, char *cube,
					unsigned int hdim, unsigned int vdim,
					unsigned int fdim,
					unsigned int elem_index)
/*  This routine will create a sequence of viewable image objects from a
    3-dimensional cube of a Karma data structure. At a later time, this
    sequence of viewable images may be made visible in any order.
    This routine will not cause the canvas to be refreshed.
    The world canvas onto which the viewable image may be drawn must be given
    by  canvas  .
    The  multi_array  descriptor which contains the Karma data structure must
    be pointed to by  multi_desc  .The routine increments the attachment count
    on the descriptor on successful completion.
    The array descriptor must be pointed to by  arr_desc  .
    The start of the cube data must be pointed to by  cube  .
    The dimension index of the horizontal dimension must be given by  hdim  .
    The dimension index of the vertical dimension must be given by  vdim  .
    The dimension index of the frame dimension (dimension containing the
    sequence) must be given by  fdim  .The number of frames is the same as the
    length of this dimension.
    The element index in the data packets must be given by  elem_index  .
    The routine may produce cache data which will vastly increase the speed of
    subsequent operations on this data. Prior to process exit, a call MUST be
    made to  viewimg_destroy  ,otherwise shared memory segments could remain
    after the process exits.
    An arbitrary number of dimension restrictions
    The routine returns a pointer to a dynamically allocated array of viewable
    image objects on success, else it returns NULL.
*/
{
    unsigned int dim_count;
    unsigned int num_frames;
    unsigned int frame_count;
    unsigned int num_restr = 1;
    ViewableImage *vimages;
    uaddr *foffsets;
    char **restr_names;
    double *restr_values;
    static char function_name[] = "viewimg_create_sequence";

    if (arr_desc == NULL)
    {
	(void) fprintf (stderr, "NULL array descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if (cube == NULL)
    {
	(void) fprintf (stderr, "NULL slice pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Sanity checks  */
    if (fdim >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"fdim: %u greater than number of dimensions: %u\n",
			fdim, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    if ( (*arr_desc).num_levels > 0 )
    {
	(void) fprintf (stderr, "Tiling not supported.\n");
	return (NULL);
    }
    if ( (*arr_desc).offsets == NULL )
    {
	if ( !ds_compute_array_offsets (arr_desc) )
	{
	    (void) fprintf (stderr, "Error computing address offsets\n");
	    a_prog_bug (function_name);
	}
    }
    foffsets = (*arr_desc).offsets[fdim];
    num_frames = (* (*arr_desc).dimensions[fdim] ).length;
    if ( ( vimages = (ViewableImage *) m_alloc (sizeof *vimages * num_frames) )
	== NULL )
    {
	m_error_notify (function_name, "array of viewable images");
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
	restr_values[0] = ds_get_coordinate ( (*arr_desc).dimensions[fdim],
					     frame_count );
	if ( ( restr_names[0] =
	      st_dup ( (* (*arr_desc).dimensions[fdim] ).name ) ) == NULL )
	{
	    m_abort (function_name, "restriction name");
	}
	if ( ( vimages[frame_count] =
	      viewimg_create_restr (canvas, multi_desc, arr_desc,
				    cube + foffsets[frame_count],
				    hdim, vdim, elem_index,
				    num_restr, restr_names, restr_values) )
	    == NULL )
	{
	    for (; frame_count > 0; --frame_count)
	    {
		viewimg_destroy (vimages[frame_count - 1]);
	    }
	    m_free ( (char *) vimages );
	    return (NULL);
	}
    }
    return (vimages);
}   /*  End Function viewimg_create_sequence  */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_create_rgb (KWorldCanvas canvas, multi_array *multi_desc,
				  array_desc *arr_desc, char *slice,
				  unsigned int hdim, unsigned int vdim,
				  unsigned int red_index,
				  unsigned int green_index,
				  unsigned int blue_index, unsigned num_restr,
				  char **restr_names, double *restr_values)
/*  [PURPOSE] This routine will create a TrueColour viewable image object from
    a 2-dimensional slice of a Karma data structure. At a later time, this
    viewable image may be made visible. This routine will not cause the canvas
    to be refreshed.
    <canvas> The world canvas onto which the viewable image may be drawn. Many
    viewable images may be associated with a single canvas.
    <multi_desc> The  multi_array  descriptor which contains the Karma data
    structure. The routine increments the attachment count on the descriptor on
    successful completion. This may be NULL.
    <arr_desc> The array descriptor.
    <slice> The start of the slice data.
    <hdim> The dimension index of the horizontal dimension.
    <vdim> The dimension index of the vertical dimension.
    <red_index> The element index of the red component in the data packets.
    <green_index> The element index of the green component in the data packets.
    <blue_index> The element index of the blue component in the data packets.
    [NOTES] The 3 colour components must be of type  K_UBYTE  .
    <num_restr> The number of matched restrictions. If this is 0, no
    restrictions are recorded.
    <restr_names> The restriction names.
    <restr_values> The restriction values.
    [NOTES] Restriction information is automatically deallocated when
    viewimg_destroy  is called.
    [NOTES] The routine may produce cache data which will vastly increase the
    speed of subsequent operations on this data. Prior to process exit, a call
    MUST be made to  viewimg_destroy  ,otherwise shared memory segments could
    remain after the process exits.
    [RETURNS] A viewable image on success, else NULL.
*/
{
    CanvasHolder holder;
    ViewableImage vimage;
    unsigned int dim_count;
    unsigned int depth;
    static char function_name[] = "viewimg_create_rgb";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL world canvas passed\n");
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
    if ( (* (*arr_desc).dimensions[hdim] ).coordinates != NULL )
    {
	(void) fprintf (stderr, "hdim: %u not regularly spaced\n", hdim);
	a_prog_bug (function_name);
    }
    if (vdim >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"vdim: %u greater than number of dimensions: %u\n",
			vdim, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    if ( (* (*arr_desc).dimensions[vdim] ).coordinates != NULL )
    {
	(void) fprintf (stderr, "vdim: %u not regularly spaced\n", vdim);
	a_prog_bug (function_name);
    }
    if (red_index >= (* (*arr_desc).packet ).num_elements)
    {
	(void) fprintf (stderr,
			"red_index: %u greater than number of elements: %u\n",
			red_index, (* (*arr_desc).packet ).num_elements);
	a_prog_bug (function_name);
    }
    if ( (* (*arr_desc).packet ).element_types[red_index] != K_UBYTE )
    {
	(void) fprintf (stderr,
			"Red component type: %u is not K_UBYTE\n",
			(* (*arr_desc).packet ).element_types[red_index]);
	return (NULL);
    }
    if (green_index >= (* (*arr_desc).packet ).num_elements)
    {
	(void) fprintf(stderr,
		       "green_index: %u greater than number of elements: %u\n",
		       green_index, (* (*arr_desc).packet ).num_elements);
	a_prog_bug (function_name);
    }
    if ( (* (*arr_desc).packet ).element_types[green_index] != K_UBYTE )
    {
	(void) fprintf (stderr,
			"Green component type: %u is not K_UBYTE\n",
			(* (*arr_desc).packet ).element_types[green_index]);
	return (NULL);
    }
    if (blue_index >= (* (*arr_desc).packet ).num_elements)
    {
	(void) fprintf (stderr,
			"blue_index: %u greater than number of elements: %u\n",
			blue_index, (* (*arr_desc).packet ).num_elements);
	a_prog_bug (function_name);
    }
    if ( (* (*arr_desc).packet ).element_types[blue_index] != K_UBYTE )
    {
	(void) fprintf (stderr,
			"Blue component type: %u is not K_UBYTE\n",
			(* (*arr_desc).packet ).element_types[blue_index]);
	return (NULL);
    }
    kwin_get_attributes (canvas_get_pixcanvas (canvas),
			 KWIN_ATT_DEPTH, &depth,
			 KWIN_ATT_END);
    if ( ( (*arr_desc).num_levels > 0 ) && (depth != 24) )
    {
	(void) fprintf (stderr,
			"Tiling not supported for non 24 bit canvases.\n");
	return (NULL);
    }
    if ( (*arr_desc).offsets == NULL )
    {
	if ( !ds_compute_array_offsets (arr_desc) )
	{
	    (void) fprintf (stderr, "Error computing address offsets\n");
	    a_prog_bug (function_name);
	}
    }
    if ( ( holder = get_canvas_holder (canvas, TRUE, function_name) ) == NULL )
    {
	return (NULL);
    }
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
    (*vimage).pc_multi_desc = NULL;
    (*vimage).pc_arr_desc = NULL;
    (*vimage).pc_slice = NULL;
    (*vimage).tc_multi_desc = multi_desc;
    (*vimage).tc_arr_desc = arr_desc;
    (*vimage).tc_slice = slice;
    (*vimage).tc_hdim = hdim;
    (*vimage).tc_vdim = vdim;
    (*vimage).tc_red_index = red_index;
    (*vimage).tc_green_index = green_index;
    (*vimage).tc_blue_index = blue_index;
    (*vimage).recompute = TRUE;
    (*vimage).changed = TRUE;
    (*vimage).pixcanvas_width = -1;
    (*vimage).pixcanvas_height = -1;
    (*vimage).cache = NULL;
    /*  Compute strides  */
    (*vimage).tc_hstride = ds_get_packet_size ( (*arr_desc).packet );
    for (dim_count = (*arr_desc).num_dimensions - 1; dim_count > hdim;
	 --dim_count)
    {
	(*vimage).tc_hstride *= (* (*arr_desc).dimensions[dim_count] ).length;
    }
    (*vimage).tc_vstride = ds_get_packet_size ( (*arr_desc).packet );
    for (dim_count = (*arr_desc).num_dimensions - 1; dim_count > vdim;
	 --dim_count)
    {
	(*vimage).tc_vstride *= (* (*arr_desc).dimensions[dim_count] ).length;
    }
    (*vimage).sequence = NULL;
    (*vimage).num_restrictions = num_restr;
    (*vimage).restriction_names = restr_names;
    (*vimage).restriction_values = restr_values;
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
    if (multi_desc != NULL) ++(*multi_desc).attachments;
    return (vimage);
}   /*  End Function viewimg_create_rgb  */

/*PUBLIC_FUNCTION*/
ViewableImage *viewimg_create_rgb_sequence (KWorldCanvas canvas,
					    multi_array *multi_desc,
					    array_desc *arr_desc, char *cube,
					    unsigned int hdim,
					    unsigned int vdim,
					    unsigned int fdim,
					    unsigned int red_index,
					    unsigned int green_index,
					    unsigned int blue_index,
					    unsigned num_restr,
					    char **restr_names,
					    double *restr_values)
/*  [PURPOSE] This routine will create a sequence of TrueColour viewable image
    objects from a 3-dimensional cube of a Karma data structure. At a later
    time, this sequence of viewable images may be made visible in any order.
    This routine will not cause the canvas to be refreshed.
    <canvas> The world canvas onto which the viewable image may be drawn. Many
    viewable images may be associated with a single canvas.
    <multi_desc> The  multi_array  descriptor which contains the Karma data
    structure must. The routine increments the attachment count on the
    descriptor on successful completion. This may be NULL.
    <arr_desc> The array descriptor.
    <cube> The start of the cube data.
    <hdim> The dimension index of the horizontal dimension.
    <vdim> The dimension index of the vertical dimension.
    <fdim> The dimension index of the frame dimension (dimension containing the
    sequence). The number of frames is the same as the length of this dimension
    <red_index> The element index of the red component in the data packets.
    <green_index> The element index of the green component in the data packets.
    <blue_index> The element index of the blue component in the data packets.
    [NOTES] The 3 colour components must be of type  K_UBYTE  .
    <num_restr> The number of matched restrictions. If this is 0, no
    restrictions are recorded.
    <restr_names> The restriction names.
    <restr_values> The restriction values.
    [NOTES] Restriction information is copied into internally allocated
    storage.
    [NOTES] The routine may produce cache data which will vastly increase the
    speed of subsequent operations on this data. Prior to process exit, a call
    MUST be made to  viewimg_destroy  ,otherwise shared memory segments could
    remain after the process exits.
    [RETURNS] A viewable image on success, else NULL.
*/
{
    ViewableImage vimage;
    SequenceHolder sequence;
    unsigned int dim_count, frame_count;
    unsigned int num_frames;
    unsigned int depth;
    unsigned int r_count;
    ViewableImage *vimages;
    uaddr *foffsets;
    char **r_names;
    double *r_values;
    static char function_name[] = "viewimg_create_rgb_sequence";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL world canvas passed\n");
	a_prog_bug (function_name);
    }
    if (arr_desc == NULL)
    {
	(void) fprintf (stderr, "NULL array descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if (cube == NULL)
    {
	(void) fprintf (stderr, "NULL slice pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Sanity checks  */
    if (fdim >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"fdim: %u greater than number of dimensions: %u\n",
			fdim, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    if (hdim >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"hdim: %u greater than number of dimensions: %u\n",
			hdim, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    if ( (* (*arr_desc).dimensions[hdim] ).coordinates != NULL )
    {
	(void) fprintf (stderr, "hdim: %u not regularly spaced\n", hdim);
	a_prog_bug (function_name);
    }
    if (vdim >= (*arr_desc).num_dimensions)
    {
	(void) fprintf (stderr,
			"vdim: %u greater than number of dimensions: %u\n",
			vdim, (*arr_desc).num_dimensions);
	a_prog_bug (function_name);
    }
    if ( (* (*arr_desc).dimensions[vdim] ).coordinates != NULL )
    {
	(void) fprintf (stderr, "vdim: %u not regularly spaced\n", vdim);
	a_prog_bug (function_name);
    }
    if (red_index >= (* (*arr_desc).packet ).num_elements)
    {
	(void) fprintf (stderr,
			"red_index: %u greater than number of elements: %u\n",
			red_index, (* (*arr_desc).packet ).num_elements);
	a_prog_bug (function_name);
    }
    if ( (* (*arr_desc).packet ).element_types[red_index] != K_UBYTE )
    {
	(void) fprintf (stderr,
			"Red component type: %u is not K_UBYTE\n",
			(* (*arr_desc).packet ).element_types[red_index]);
	return (NULL);
    }
    if (green_index >= (* (*arr_desc).packet ).num_elements)
    {
	(void) fprintf(stderr,
		       "green_index: %u greater than number of elements: %u\n",
		       green_index, (* (*arr_desc).packet ).num_elements);
	a_prog_bug (function_name);
    }
    if ( (* (*arr_desc).packet ).element_types[green_index] != K_UBYTE )
    {
	(void) fprintf (stderr,
			"Green component type: %u is not K_UBYTE\n",
			(* (*arr_desc).packet ).element_types[green_index]);
	return (NULL);
    }
    if (blue_index >= (* (*arr_desc).packet ).num_elements)
    {
	(void) fprintf (stderr,
			"blue_index: %u greater than number of elements: %u\n",
			blue_index, (* (*arr_desc).packet ).num_elements);
	a_prog_bug (function_name);
    }
    if ( (* (*arr_desc).packet ).element_types[blue_index] != K_UBYTE )
    {
	(void) fprintf (stderr,
			"Blue component type: %u is not K_UBYTE\n",
			(* (*arr_desc).packet ).element_types[blue_index]);
	return (NULL);
    }
    kwin_get_attributes (canvas_get_pixcanvas (canvas),
			 KWIN_ATT_DEPTH, &depth,
			 KWIN_ATT_END);
    if ( ( (*arr_desc).num_levels > 0 ) && (depth != 24) )
    {
	(void) fprintf (stderr,
			"Tiling not supported for non 24 bit canvases.\n");
	return (NULL);
    }
    if ( (*arr_desc).offsets == NULL )
    {
	if ( !ds_compute_array_offsets (arr_desc) )
	{
	    (void) fprintf (stderr, "Error computing address offsets\n");
	    a_prog_bug (function_name);
	}
    }
    foffsets = (*arr_desc).offsets[fdim];
    num_frames = (* (*arr_desc).dimensions[fdim] ).length;
    if ( ( vimages = (ViewableImage *) m_alloc (sizeof *vimages * num_frames) )
	== NULL )
    {
	m_error_notify (function_name, "array of viewable images");
	return (NULL);
    }
    if ( ( sequence = (SequenceHolder) m_alloc (sizeof *sequence) ) == NULL )
    {
	m_error_notify (function_name, "sequence holder");
	m_free ( (char *) vimages );
	return (NULL);
    }
    (*sequence).attachments = 0;
    (*sequence).fdim = fdim;
    for (frame_count = 0; frame_count < num_frames; ++frame_count)
    {
	/*  Allocate restrictions  */
	if ( ( r_names = (char **)
	      m_alloc ( sizeof *r_names * (num_restr + 1) ) ) == NULL )
	{
	    m_abort (function_name, "array of restriction name pointers");
	}
	if ( ( r_values = (double *)
	      m_alloc ( sizeof *restr_values * (num_restr + 1) ) ) == NULL )
	{
	    m_abort (function_name, "array of restriction values");
	}
	for (r_count = 0; r_count < num_restr; ++r_count)
	{
	    if ( ( r_names[r_count] = st_dup (restr_names[r_count]) )
		== NULL )
	    {
		m_abort (function_name, "restriction name");
	    }
	    r_values[r_count] = restr_values[r_count];
	}
	r_values[num_restr] = ds_get_coordinate ( (*arr_desc).dimensions[fdim],
						 frame_count );
	if ( ( r_names[num_restr] =
	      st_dup ( (* (*arr_desc).dimensions[fdim] ).name ) ) == NULL )
	{
	    m_abort (function_name, "restriction name");
	}
	if ( ( vimages[frame_count] =
	      viewimg_create_rgb (canvas, multi_desc, arr_desc,
				  cube + foffsets[frame_count],
				  hdim, vdim,
				  red_index, green_index, blue_index,
				  num_restr + 1, r_names, r_values) )
	    == NULL )
	{
	    for (; frame_count > 0; --frame_count)
	    {
		viewimg_destroy (vimages[frame_count - 1]);
	    }
	    m_free ( (char *) vimages );
	    m_free ( (char *) sequence );
	    return (NULL);
	}
	(*vimages[frame_count]).sequence = sequence;
	++(*sequence).attachments;
    }
    return (vimages);
}   /*  End Function viewimg_create_rgb_sequence  */

/*PUBLIC_FUNCTION*/
flag viewimg_make_active (ViewableImage vimage)
/*  [PURPOSE] This routine will make a viewable image the active image for its
    associated world canvas. The canvas is then refreshed (possibly resized),
    provided that the new viewable image was not already active.
    <vimage> The viewable image.
    [RETURNS] TRUE on success, else FALSE.
    [SEE ALSO] viewimg_set_active
*/
{
    CanvasHolder holder;
    unsigned int hdim, vdim;
    dim_desc **dimensions;
    static char function_name[] = "viewimg_make_active";

    VERIFY_VIMAGE (vimage);
    holder = (*vimage).canvas_holder;
    if (vimage == (*holder).active_image) return (TRUE);
    return ( viewimg_set_active (vimage, TRUE) );
}   /*  End Function viewimg_make_active  */

/*PUBLIC_FUNCTION*/
flag viewimg_set_active (ViewableImage vimage, flag refresh)
/*  [PURPOSE] This routine will make a viewable image the active image for its
    associated world canvas.
    <vimage> The viewable image.
    <refresh> If TRUE, the canvas is always refreshed, if FALSE, the canvas is
    not refreshed.
    [RETURNS] TRUE on success, else FALSE.
    [SEE ALSO] viewimg_make_active
*/
{
    CanvasHolder holder;
    unsigned int hdim, vdim;
    dim_desc **dimensions;
    static char function_name[] = "viewimg_set_active";

    VERIFY_VIMAGE (vimage);
    FLAG_VERIFY (refresh);
    holder = (*vimage).canvas_holder;
    if (vimage == (*holder).active_image)
    {
	if (refresh)
	{
	    return ( canvas_resize ( (*holder).canvas,
				    (struct win_scale_type *) NULL, FALSE ) );
	}
	return (TRUE);
    }
    (*holder).active_image = vimage;
    if ( (*vimage).tc_arr_desc == NULL )
    {
	dimensions = (* (*vimage).pc_arr_desc ).dimensions;
	hdim = (*vimage).pc_hdim;
	vdim = (*vimage).pc_vdim;
    }
    else
    {
	dimensions = (* (*vimage).tc_arr_desc ).dimensions;
	hdim = (*vimage).tc_hdim;
	vdim = (*vimage).tc_vdim;
    }
    if ( !canvas_specify ( (*holder).canvas,
			  (*dimensions[hdim]).name,
			  (*dimensions[vdim]).name,
			  (*vimage).num_restrictions,
			  (*vimage).restriction_names,
			  (*vimage).restriction_values ) )
    {
	return (FALSE);
    }
    if (refresh)
    {
	return ( canvas_resize ( (*holder).canvas,
				(struct win_scale_type *) NULL, FALSE ) );
    }
    return (TRUE);
}   /*  End Function viewimg_set_active  */

/*OBSOLETE_FUNCTION*/
void viewimg_control_autoscaling (KWorldCanvas canvas,
				  flag auto_x, flag auto_y, flag auto_v,
				  flag int_x, flag int_y,
				  flag maintain_aspect_ratio)
/*  This routine will control the autoscaling options used when viewable images
    are displayed on their associated world canvas.
    THIS FUNCTION IS OBSOLETE. USE  VIEWIMG_SET_CANVAS_ATTRIBUTES  INSTEAD.
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
{
    CanvasHolder holder;
    int width, height;
    struct win_scale_type win_scale;
    static char function_name[] = "viewimg_control_autoscaling";

    (void) fprintf (stderr,
		    "Function: %s will be removed in Karma version 2.0\n",
		    function_name);
    (void) fprintf (stderr, "Use:  viewimg_set_canvas_attributes  instead.\n");
/*  This will be activated in Karma version 2.0
    a_prog_bug (func_name);
*/
    viewimg_set_canvas_attributes (canvas,
				   VIEWIMG_ATT_AUTO_X, auto_x,
				   VIEWIMG_ATT_AUTO_Y, auto_y,
				   VIEWIMG_ATT_AUTO_V, auto_v,
				   VIEWIMG_ATT_INT_X, int_x,
				   VIEWIMG_ATT_INT_Y, int_y,
				   VIEWIMG_ATT_MAINTAIN_ASPECT,
				   maintain_aspect_ratio,
				   VIEWIMG_ATT_END);
}   /*  End Function viewimg_control_autoscaling  */

/*PUBLIC_FUNCTION*/
flag viewimg_register_data_change (ViewableImage vimage)
/*  This routine will register a change in the Karma data structure associated
    with a viewable image. If the viewable image is active, it will be
    immediately redrawn on its canvas.
    The viewable image must be given by  vimage  .
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    CanvasHolder holder;
    static char function_name[] = "viewimg_register_data_change";

    VERIFY_VIMAGE (vimage);
    holder = (*vimage).canvas_holder;
    (*vimage).recompute = TRUE;
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
void viewimg_destroy (ViewableImage vimage)
/*  This routine will destroy a viewable image. If this is not called prior to
    process exit, shared memory segments could remain after the process exits.
    The viewable image must be given by  vimage  .
    Note that the associated  multi_array  descriptor is also deallocated (or
    at least, the attachment count is decreased).
    The routine returns nothing.
*/
{
    CanvasHolder holder;
    SequenceHolder sequence;
    unsigned int count;
    static char function_name[] = "viewimg_destroy";

    VERIFY_VIMAGE (vimage);
    holder = (*vimage).canvas_holder;
    kwin_free_cache_data ( (*vimage).cache );
    ds_dealloc_multi ( (*vimage).pc_multi_desc );
    ds_dealloc_multi ( (*vimage).tc_multi_desc );
    if ( (sequence = (*vimage).sequence) != NULL )
    {
	if ( (*sequence).attachments > 1 )
	{
	    --(*sequence).attachments;
	}
	else
	{
	    m_free ( (char *) sequence );
	}
    }
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
    if ( (*vimage).restriction_names != NULL )
    {
	for (count = 0; count < (*vimage).num_restrictions; ++count)
	{
	    if ( (*vimage).restriction_names[count] != NULL )
	    {
		m_free ( (*vimage).restriction_names[count] );
	    }
	}
	m_free ( (char *) (*vimage).restriction_names );
    }
    if ( (*vimage).restriction_values != NULL )
    {
	m_free ( (char *) (*vimage).restriction_values );
    }
    (*vimage).magic_number = 0;
    m_free ( (char *) vimage );
}   /*  End Function viewimg_destroy  */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_get_active (KWorldCanvas canvas)
/*  This routine will get the active ViewableImage associated with a
    KWorldCanvas object.
    The canvas must be given by  canvas  .
    The routine returns the active viewable image on success, else it returns
    NULL (indicating no viewable image is active for the canvas).
*/
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
    if ( ( holder = get_canvas_holder (canvas, TRUE, function_name) ) == NULL )
    {
	m_abort (function_name, "canvas holder");
    }
    return ( (*holder).active_image );
}   /*  End Function viewimg_get_active  */

/*PUBLIC_FUNCTION*/
flag viewimg_test_active (ViewableImage vimage)
/*  This routine will test if a viewable image is the active image for its
    associated world canvas.
    The viewable image must be given by  vimage  .
    The routine returns TRUE on if the viewable image is actice,
    else it returns FALSE.
*/
{
    CanvasHolder holder;
    dim_desc **dimensions;
    static char function_name[] = "viewimg_test_active";

    VERIFY_VIMAGE (vimage);
    holder = (*vimage).canvas_holder;
    if (vimage == (*holder).active_image) return (TRUE);
    return (FALSE);
}   /*  End Function viewimg_test_active  */

/*PUBLIC_FUNCTION*/
KCallbackFunc viewimg_register_position_event_func (KWorldCanvas canvas,
						    flag (*func) (),
						    void *f_info)
/*  This routine will register a position event function for a world canvas
    which has a number of ViewableImage objects associated with it.
    The position event function will be called whenever a position event on the
    canvas has not been consumed. Many position event functions may be
    registered per canvas. The first function registered is the first function
    called upon a position event.
    The canvas must be given by  canvas  .
    The function that is called when a position event occurs must be pointed to
    by  func  .
    The interface to this routine is as follows:

    flag func (vimage, x, y, value, event_code, e_info, f_info, x_lin, y_lin)
    *   This routine is a position event consumer for a world canvas which has
        a number of ViewableImage objects associated with it.
        The active viewable image is given by  vimage  .
	The horizontal world co-ordinate of the event will be given by  x  .
	The vertical world co-ordinate of the event will be given by  y  .
	These values will have been transformed by the registered transform
	function (see  canvas_register_transform_func  ) for the associated
	world canvas.
	The data value in the viewable image corresponding to the event
	co-ordinates will be written to the storage pointed to by  value  .
	This must be of type  K_DOUBLE  .
	The arbitrary event code is given by  event_code  .
	The arbitrary event information is pointed to by  e_info  .
	The arbitrary function information pointer is pointed to by  f_info  .
	The linear world co-ordinates (the co-ordinates prior to the transform
	function being called) will be given by  x_lin  and  y_lin  .
	The routine returns TRUE if the event was consumed, else it returns
	FALSE indicating that the event is still to be processed.
    *
    ViewableImage vimage;
    double x;
    double y;
    double value[2];
    unsigned int event_code;
    void *e_info;
    void **f_info;
    double x_lin;
    double y_lin;

    The initial arbitrary function information pointer must be given by  f_info
    The routine returns a handle to a KWorldCanvasPositionFunc object.
*/
{
    CanvasHolder holder;
    static char function_name[] = "viewimg_register_position_func";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL canvas passed\n");
	a_prog_bug (function_name);
    }
    if ( ( holder = get_canvas_holder (canvas, TRUE, function_name) ) == NULL )
    {
	m_abort (function_name, "canvas holder");
    }
    return ( c_register_callback (&(*holder).position_list,
				  position_event_func,
				  holder, f_info, TRUE, (void *) func, FALSE,
				  TRUE) );
}   /*  End Function viewimg_register_position_func  */

/*PUBLIC_FUNCTION*/
flag viewimg_fill_ellipse (ViewableImage vimage,
			   double centre_x, double centre_y,
			   double radius_x, double radius_y, double value[2])
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
{
    packet_desc *pack_desc;
    array_desc *arr_desc;
    static char function_name[] = "viewimg_fill_ellipse";

    VERIFY_VIMAGE (vimage);
#ifdef MACHINE_i386
    if ( (int) value % sizeof (float) != 0 )
#else
    if ( (long) value % sizeof (double) != 0 )
#endif
    {
	(void) fprintf (stderr,
			"value  address: %p not on a double boundary\n",
			value);
	a_prog_bug (function_name);
    }
    if ( (*vimage).tc_arr_desc != NULL )
    {
	(void) fprintf (stderr, "%s: TrueColour images not supported yet\n",
			function_name);
	return (FALSE);
    }
    arr_desc = (*vimage).pc_arr_desc;
    pack_desc = (*arr_desc).packet;
    return ( ds_draw_ellipse
	    ( (*vimage).pc_slice,
	     (*pack_desc).element_types[(*vimage).pc_elem_index],
	     (*arr_desc).dimensions[(*vimage).pc_hdim], (*vimage).pc_hstride,
	     (*arr_desc).dimensions[(*vimage).pc_vdim], (*vimage).pc_vstride,
	     centre_x, centre_y, radius_x, radius_y, value ) );
}   /*  End Function viewimg_fill_ellipse  */

/*PUBLIC_FUNCTION*/
flag viewimg_fill_polygon (ViewableImage vimage, edit_coord *coords,
			   unsigned int num_vertices, double value[2])
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
{
    packet_desc *pack_desc;
    array_desc *arr_desc;
    static char function_name[] = "viewimg_fill_polygon";

    VERIFY_VIMAGE (vimage);
#ifdef MACHINE_i386
    if ( (int) value % sizeof (float) != 0 )
#else
    if ( (long) value % sizeof (double) != 0 )
#endif
    {
	(void) fprintf (stderr,
			"value  address: %p not on a double boundary\n",
			value);
	a_prog_bug (function_name);
    }
    if ( (*vimage).tc_arr_desc != NULL )
    {
	(void) fprintf (stderr, "%s: TrueColour images not supported yet\n",
			function_name);
	return (FALSE);
    }
    arr_desc = (*vimage).pc_arr_desc;
    pack_desc = (*arr_desc).packet;
    return ( ds_draw_polygon
	    ( (*vimage).pc_slice,
	     (*pack_desc).element_types[(*vimage).pc_elem_index],
	     (*arr_desc).dimensions[(*vimage).pc_hdim], (*vimage).pc_hstride,
	     (*arr_desc).dimensions[(*vimage).pc_vdim], (*vimage).pc_vstride,
	     coords, num_vertices, value ) );
}   /*  End Function viewimg_fill_polygon  */

/*PUBLIC_FUNCTION*/
void viewimg_get_canvas_attributes (canvas, va_alist)
/*  This routine will get the attributes for a world canvas.
    The world canvas must be given by  canvas  .
    The list of parameter attribute-key attribute-value pairs must follow.
    This list must be terminated with the value  VIEWIMG_ATT_END  .
    See the documentation for the  viewimg_set_canvas_attributes  routine for
    legal attributes.
    The routine returns nothing.
*/
KWorldCanvas canvas;
va_dcl
{
    CanvasHolder holder;
    va_list arg_pointer;
    unsigned int att_key;
    int width, height;
    struct win_scale_type win_scale;
    static char function_name[] = "viewimg_get_canvas_attributes";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL canvas passed\n");
	a_prog_bug (function_name);
    }
    /*  Dummy operation to ensure canvas is OK  */
    canvas_get_size (canvas, &width, &height, &win_scale);
    if ( ( holder = get_canvas_holder (canvas, TRUE, function_name) ) == NULL )
    {
	m_abort (function_name, "canvas holder");
    }
    va_start (arg_pointer);
    while ( ( att_key = va_arg (arg_pointer, unsigned int) ) !=
	   VIEWIMG_ATT_END )
    {
	switch (att_key)
	{
	  case VIEWIMG_ATT_AUTO_X:
	    *( va_arg (arg_pointer, unsigned int *) ) = (*holder).auto_x;
	    break;
	  case VIEWIMG_ATT_AUTO_Y:
	    *( va_arg (arg_pointer, unsigned int *) ) = (*holder).auto_y;
	    break;
	  case VIEWIMG_ATT_AUTO_V:
	    *( va_arg (arg_pointer, unsigned int *) ) = (*holder).auto_v;
	    break;
	  case VIEWIMG_ATT_INT_X:
	    *( va_arg (arg_pointer, unsigned int *) ) = (*holder).int_x;
	    break;
	  case VIEWIMG_ATT_INT_Y:
	    *( va_arg (arg_pointer, unsigned int *) ) = (*holder).int_y;
	    break;
	  case VIEWIMG_ATT_MAINTAIN_ASPECT:
	    *( va_arg (arg_pointer, unsigned int *) ) =
	    (*holder).maintain_aspect_ratio;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (arg_pointer);
}   /*  End Function viewimg_get_canvas_attributes  */

/*PUBLIC_FUNCTION*/
void viewimg_set_canvas_attributes (canvas, va_alist)
/*  This routine will control the autoscaling options used when viewable images
    are displayed on their associated world canvas.
    The world canvas must be given by  canvas  .
    The list of parameter attribute-key attribute-value pairs must follow.
    This list must be terminated with the value  VIEWIMG_ATT_END  .
    Legal attributes are:
  VIEWIMG_ATT_AUTO_X           Enable automatic horizontal scaling
  VIEWIMG_ATT_AUTO_Y           Enable automatic vertical scaling
  VIEWIMG_ATT_AUTO_V           Enable automatic intensity scaling
  VIEWIMG_ATT_INT_X            Force integer horizontal zoom-in/zoom-out factor
  VIEWIMG_ATT_INT_Y            Force integer vertical zoom-in/zoom-out factor
  VIEWIMG_ATT_MAINTAIN_ASPECT  Maintain data image aspect ratio
    See the include file  karma_viewimg.h  for matching attribute types.
    The canvas is not refreshed by this operation.
    The routine returns nothing.
*/
KWorldCanvas canvas;
va_dcl
{
    CanvasHolder holder;
    va_list arg_pointer;
    flag bool;
    unsigned int att_key;
    int width, height;
    struct win_scale_type win_scale;
    static char function_name[] = "viewimg_set_canvas_attributes";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL canvas passed\n");
	a_prog_bug (function_name);
    }
    /*  Dummy operation to ensure canvas is OK  */
    canvas_get_size (canvas, &width, &height, &win_scale);
    if ( ( holder = get_canvas_holder (canvas, TRUE, function_name) ) == NULL )
    {
	m_abort (function_name, "canvas holder");
    }
    va_start (arg_pointer);
    while ( ( att_key = va_arg (arg_pointer, unsigned int) ) !=
	   VIEWIMG_ATT_END )
    {
	switch (att_key)
	{
	  case VIEWIMG_ATT_AUTO_X:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    (*holder).auto_x = bool;
	    break;
	  case VIEWIMG_ATT_AUTO_Y:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    (*holder).auto_y = bool;
	    break;
	  case VIEWIMG_ATT_AUTO_V:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    (*holder).auto_v = bool;
	    break;
	  case VIEWIMG_ATT_INT_X:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    (*holder).int_x = bool;
	    break;
	  case VIEWIMG_ATT_INT_Y:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    (*holder).int_y = bool;
	    break;
	  case VIEWIMG_ATT_MAINTAIN_ASPECT:
	    bool = va_arg (arg_pointer, flag);
	    FLAG_VERIFY (bool);
	    (*holder).maintain_aspect_ratio = bool;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (arg_pointer);
}   /*  End Function viewimg_set_canvas_attributes  */


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
    static char function_name[] = "get_canvas_holder";

    /*  First search for existing canvas holder  */
    for (canvas_holder = first_canvas_holder; canvas_holder != NULL;
	 canvas_holder = (*canvas_holder).next)
    {
	if (canvas == (*canvas_holder).canvas) return (canvas_holder);
    }
    if (!alloc) return (NULL);
    (void) fprintf (stderr, "**WARNING**:  %s  called before:  viewimg_init\n",
		    func_name);
    (void) fprintf (stderr,
		    "for this canvas. This will break in Karma version 2.0\n");
/*  This will be activated in Karma version 2.0
    a_prog_bug (func_name);
*/
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
    (*canvas_holder).magic_number = HOLDER_MAGIC_NUMBER;
    (*canvas_holder).canvas = canvas;
    (*canvas_holder).first_image = NULL;
    (*canvas_holder).active_image = NULL;
    (*canvas_holder).position_list = NULL;
    (*canvas_holder).info = NULL;
    (*canvas_holder).auto_x = TRUE;
    (*canvas_holder).auto_y = TRUE;
    (*canvas_holder).auto_v = TRUE;
    (*canvas_holder).int_x = TRUE;
    (*canvas_holder).int_y = TRUE;
    (*canvas_holder).maintain_aspect_ratio = FALSE;
    (*canvas_holder).old_cmap = NULL;
    /*  Insert at beginning of list  */
    (*canvas_holder).next = first_canvas_holder;
    first_canvas_holder = canvas_holder;
    canvas_register_refresh_func (canvas, worldcanvas_refresh_func,
				  (void *) canvas_holder);
    canvas_register_size_control_func (canvas, worldcanvas_size_control_func,
				       (void *) canvas_holder);
    (void) canvas_register_position_event_func (canvas,
						worldcanvas_position_func,
						(void *) canvas_holder);
    canvas_register_convert_func (canvas, coord_convert_func, canvas_holder);
    return (canvas_holder);
}   /*  End Function alloc_canvas_holder  */

static void worldcanvas_refresh_func (KWorldCanvas canvas,
				      int width, int height,
				      struct win_scale_type *win_scale,
				      Kcolourmap cmap, flag cmap_resize,
				      void **info)
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
    if ( (*vimage).recompute )
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
#ifdef DEBUG
    (void) fprintf (stderr, "%s win_scale: %e %e  %e %e\n",
		    function_name,
		    (*win_scale).x_min, (*win_scale).x_max,
		    (*win_scale).y_min, (*win_scale).y_max);
    (void) fprintf (stderr, "%s vimage.win_scale: %e %e  %e %e\n",
		    function_name,
		    (*vimage).win_scale.x_min, (*vimage).win_scale.x_max,
		    (*vimage).win_scale.y_min, (*vimage).win_scale.y_max);
#endif
    /*  More tests  */
    (*vimage).win_scale.x_offset = (*win_scale).x_offset;
    (*vimage).win_scale.y_offset = (*win_scale).y_offset;
    if (!m_cmp ( (char *) win_scale, (char *) &(*vimage).win_scale,
		sizeof *win_scale ) )
    {
	/*  Window scaling information has changed  */
#ifdef DEBUG
	(void) fprintf (stderr, "%s: SPURIOUS SCALE CHANGE\n", function_name);
#endif
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

static void recompute_image (CanvasHolder holder, int width, int height,
			     struct win_scale_type *win_scale, Kcolourmap cmap,
			     ViewableImage vimage)
/*  This routine will recompute a viewable image and then redraw it onto a
    world canvas.
    The canvas holder must be given by  holder  .
    The width of the canvas in pixels must be given by  width  .
    The height of the canvas in pixels must be given by  height  .
    The window scaling information is pointed to by  win_scale  .
    The colourmap associated with the canvas must be given by  cmap  .
    The viewable image must be given by  vimage  .
*/
{
    static char function_name[] = "recompute_image";

    m_copy ( (char *) &(*vimage).win_scale, (char *) win_scale,
	    sizeof *win_scale );
    (*vimage).recompute = TRUE;
    if ( (*vimage).pc_arr_desc == NULL )
    {
	if ( !canvas_draw_rgb_image ( (*holder).canvas,
				     (*vimage).tc_arr_desc, (*vimage).tc_slice,
				     (*vimage).tc_hdim, (*vimage).tc_vdim,
				     (*vimage).tc_red_index,
				     (*vimage).tc_green_index,
				     (*vimage).tc_blue_index,
				     &(*vimage).cache ) )
	{
	    (void) fprintf (stderr, "Error drawing image onto world canvas\n");
	    return;
	}
    }
    else
    {
	if ( !canvas_draw_image ( (*holder).canvas,
				 (*vimage).pc_arr_desc,
				 (*vimage).pc_slice,
				 (*vimage).pc_hdim,
				 (*vimage).pc_vdim,
				 (*vimage).pc_elem_index,
				 &(*vimage).cache ) )
	{
	    (void) fprintf (stderr, "Error drawing image onto world canvas\n");
	    return;
	}
    }
    (*vimage).recompute = FALSE;
}   /*  End Function recompute_image  */

static void worldcanvas_size_control_func (KWorldCanvas canvas,
					   int width, int height,
					   struct win_scale_type *win_scale,
					   void **info, flag *boundary_clear)
/*  This routine will modify the window scaling information for a world canvas.
    The canvas is given by  canvas  .
    The width of the canvas in pixels is given by  width  .
    The height of the canvas in pixels is given by  height  .
    The window scaling information is pointed to by  win_scale  .The data
    herein may be modified.
    The arbitrary canvas information pointer is pointed to by  info  .
    If the value TRUE is written to the storage pointed to by
    boundary_clear  then the  canvas_resize  routine will attempt to clear
    only the boundary between the pixel canvas and the world canvas. If
    the value FALSE is written here or nothing is written here, the
    canvas_resize  routine will clear the entire pixel canvas as
    appropriate.
    The routine returns nothing.
*/
{
    CanvasHolder holder;
    ViewableImage vimage;
    Kcolourmap cmap;
    iarray pseudo_arr;
    int hlength, vlength;
    int factor;
    int coord_num0, coord_num1;
    unsigned int canvas_visual, canvas_depth;
    flag scale_changed = FALSE;
    char *cmap_packet;
    array_desc *arr_desc;
    packet_desc *pack_desc;
    dim_desc *hdim;
    dim_desc *vdim;
    packet_desc *cmap_pack_desc;
    static char function_name[] = "__viewimg_worldcanvas_size_control_func";

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
	(*win_scale).x_offset = 0;
	(*win_scale).x_pixels = width;
	(*win_scale).y_offset = 0;
	(*win_scale).y_pixels = height;
	return;
    }
    *boundary_clear = TRUE;
    vimage = (*holder).active_image;
    /*  Work on changing window scaling if need be  */
    if ( (*vimage).tc_arr_desc == NULL )
    {
	arr_desc = (*vimage).pc_arr_desc;
	hdim = (*arr_desc).dimensions[(*vimage).pc_hdim];
	vdim = (*arr_desc).dimensions[(*vimage).pc_vdim];
    }
    else
    {
	arr_desc = (*vimage).tc_arr_desc;
	hdim = (*arr_desc).dimensions[(*vimage).tc_hdim];
	vdim = (*arr_desc).dimensions[(*vimage).tc_vdim];
    }
    pack_desc = (*arr_desc).packet;
#ifdef DEBUG
    (void) fprintf (stderr, "%s: BEFORE: %e %e  %e %e\n",
		    function_name,
		    (*win_scale).x_min, (*win_scale).x_max,
		    (*win_scale).y_min, (*win_scale).y_max);
#endif
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
#ifdef DEBUG
	(void) fprintf (stderr, "hlength: %d  width: %d  hpixels: %d\n",
			hlength, width, (*win_scale).x_pixels);
	(void) fprintf (stderr, "vlength: %d  height: %d  vpixels: %d\n",
			vlength, height, (*win_scale).y_pixels);
#endif
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
#ifdef DEBUG
    (void) fprintf (stderr,
		    "%s: width: %d hlength: %d  height: %d vlength: %d\n",
		    function_name, width, hlength, height, vlength);
#endif
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
	if (getuid () == 465)
	{
	    (void) fprintf (stderr, "%s: scale changed\n", function_name);
	}
	/*  Precomputed intensity range no longer valid  */
	(*vimage).value_min = TOOBIG;
	(*vimage).value_max = TOOBIG;
	(*vimage).recompute = TRUE;
    }
#ifdef DEBUG
    (void) fprintf (stderr, "%s: AFTER: %e %e  %e %e\n",
		    function_name,
		    (*win_scale).x_min, (*win_scale).x_max,
		    (*win_scale).y_min, (*win_scale).y_max);
#endif
    if ( (*holder).auto_v && ( (*vimage).tc_arr_desc == NULL ) )
    {
	if ( ( (*vimage).value_min >= TOOBIG ) ||
	    ( (*vimage).value_max >= TOOBIG ) )
	{
	    /*  Must compute minimum and maximum values  */
	    (*vimage).value_min = TOOBIG;
	    (*vimage).value_max = -TOOBIG;
	    if ( !ds_find_plane_extremes
		( (*vimage).pc_slice,
		 (*pack_desc).element_types[(*vimage).pc_elem_index],
		 (*win_scale).conv_type,
		 hdim, (*vimage).pc_hstride, vdim, (*vimage).pc_vstride,
		 (*win_scale).x_min, (*win_scale).x_max,
		 (*win_scale).y_min, (*win_scale).y_max,
		 &(*vimage).value_min, &(*vimage).value_max ) )
	    {
		(void) fprintf (stderr, "Error getting data range\n");
		a_prog_bug (function_name);
	    }
	    /*Image intensity range has changed: cached image no longer valid*/
	    (*vimage).recompute = TRUE;
	    /*  Ensure ranges are sensible  */
	    if ( (*vimage).value_min >= (*vimage).value_max )
	    {
		if ( (*vimage).value_min < 0.0 )
		{
		    (*vimage).value_min *= 2.0;
		    (*vimage).value_max = 0.0;
		}
		else if ( (*vimage).value_min > 0.0 )
		{
		    (*vimage).value_min = 0.0;
		    (*vimage).value_max *= 2.0;
		}
		else
		{
		    (*vimage).value_min = -1.0;
		    (*vimage).value_max = 1.0;
		}
	    }
	}
	/*  World canvas intensity scale has changed  */
	(*win_scale).z_min = (*vimage).value_min;
	(*win_scale).z_max = (*vimage).value_max;
    }
    if ( (*vimage).changed && ( (*vimage).tc_arr_desc != NULL ) )
    {
	kwin_get_attributes (canvas_get_pixcanvas (canvas),
			     KWIN_ATT_VISUAL, &canvas_visual,
			     KWIN_ATT_DEPTH, &canvas_depth,
			     KWIN_ATT_END);
	if ( (canvas_visual == KWIN_VISUAL_PSEUDOCOLOUR) &&
	    ( (*vimage).sequence != NULL ) )
	{
	    (void) fprintf (stderr,
			    "RGB movies on PseudoColour canvas not implemented yet\n");
	    return;
	}
	if (canvas_visual != KWIN_VISUAL_PSEUDOCOLOUR)
	{
	    (*vimage).changed = FALSE;
	    (*vimage).recompute = TRUE;
	    return;
	}
	/*  If TrueColour image and PseudoColour canvas, recompress image  */
	(*vimage).recompute = TRUE;
	/*  Have RGB component arrays: create PseudoColour array  */
	ds_dealloc_multi ( (*vimage).pc_multi_desc );
	if ( ( pseudo_arr = iarray_create_2D ( (*vdim).length, (*hdim).length,
					      K_UBYTE ) ) == NULL )
	{
	    m_abort (function_name, "PseudoColour image");
	}
	iarray_set_world_coords (pseudo_arr, 1,
				 (*hdim).minimum, (*hdim).maximum);
	iarray_set_world_coords (pseudo_arr, 0,
				 (*vdim).minimum, (*vdim).maximum);
	(*vimage).pc_multi_desc = (*pseudo_arr).multi_desc;
	(*vimage).pc_arr_desc = (*pseudo_arr).arr_desc;
	(*vimage).pc_slice = (*pseudo_arr).data;
	(*vimage).pc_hdim = 1;
	(*vimage).pc_vdim = 0;
	(*vimage).pc_elem_index = 0;
	(*vimage).pc_hstride = 1;
	(*vimage).pc_vstride = (*hdim).length;
	++(* (*pseudo_arr).multi_desc ).attachments;
	iarray_dealloc (pseudo_arr);
	if ( !imc_24to8 ( (*vdim).length * (*hdim).length,
			 (*vimage).tc_slice +
			 ds_get_element_offset (pack_desc,
						(*vimage).tc_red_index),
			 (*vimage).tc_slice +
			 ds_get_element_offset (pack_desc,
						(*vimage).tc_green_index),
			 (*vimage).tc_slice +
			 ds_get_element_offset (pack_desc,
						(*vimage).tc_blue_index),
			 ds_get_packet_size (pack_desc),
			 (*vimage).pc_slice, 1, DEFAULT_NUMBER_OF_COLOURS,
			 0, &cmap_pack_desc, &cmap_packet) )
	{
	    (void) fprintf (stderr, "Error compressing image\n");
	    a_prog_bug (function_name);
	}
	cmap = canvas_get_cmap (canvas);
	if ( !kcmap_copy_from_struct (cmap, cmap_pack_desc, cmap_packet) )
	{
	    m_abort (function_name, "Compressed image");
	}
	(*win_scale).z_min = 0.0;
	(*win_scale).z_max = (kcmap_get_pixels (cmap, (unsigned long **) NULL)
			      - 1);
	ds_dealloc_packet (cmap_pack_desc, cmap_packet);
	(*vimage).changed = FALSE;
    }
}   /*  End Function worldcanvas_size_control_func  */


static flag worldcanvas_position_func (KWorldCanvas canvas, double x, double y,
				       unsigned int event_code,
				       void *e_info, void **f_info,
				       double x_lin, double y_lin)
/*  This routine is a position event consumer for a world canvas.
    The canvas is given by  canvas  .
    The horizontal world co-ordinate of the event will be given by  x  .
    The vertical world co-ordinate of the event will be given by  y  .
    These values will have been transformed by the registered transform
    function (see  canvas_register_transform_func  ).
    The arbitrary event code is given by  event_code  .
    The arbitrary event information is pointed to by  e_info  .
    The arbitrary function information pointer is pointed to by  f_info  .
    The linear world co-ordinates (the co-ordinates prior to the transform
    function being called) will be given by  x_lin  and  y_lin  .
    The routine returns TRUE if the event was consumed, else it returns
    FALSE indicating that the event is still to be processed.
*/
{
    CanvasHolder holder;
    ViewableImage vimage;
    unsigned long x_coord, y_coord;
    double dx, dy;
    struct position_struct data;
    double value[2];
    array_desc *arr_desc;
    packet_desc *pack_desc;
    dim_desc *hdim;
    dim_desc *vdim;
    static char function_name[] = "worldcanvas_position_func";

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
    if ( (*holder).position_list == NULL ) return (FALSE);
    if ( (*holder).active_image == NULL ) return (FALSE);
    vimage = (*holder).active_image;
    if ( (*vimage).tc_arr_desc != NULL )
    {
	(void) fprintf (stderr, "%s: TrueColour images not supported yet\n",
			function_name);
	return (FALSE);
    }
    arr_desc = (*vimage).pc_arr_desc;
    pack_desc = (*arr_desc).packet;
    hdim = (*arr_desc).dimensions[(*vimage).pc_hdim];
    vdim = (*arr_desc).dimensions[(*vimage).pc_vdim];
    x_coord = ds_get_coord_num (hdim, x_lin, SEARCH_BIAS_LOWER);
    y_coord = ds_get_coord_num (vdim, y_lin, SEARCH_BIAS_LOWER);
    dx = ds_get_coordinate (hdim, x_coord);
    dy = ds_get_coordinate (vdim, y_coord);
    if ( !ds_get_element
	( (*vimage).pc_slice +
	 (*arr_desc).offsets[(*vimage).pc_vdim][y_coord] +
	 (*arr_desc).offsets[(*vimage).pc_hdim][x_coord],
	 (*pack_desc).element_types[(*vimage).pc_elem_index],
	 value, (flag *) NULL ) )
    {
	(void) fprintf (stderr, "Error converting data\n");
	return (FALSE);
    }
    x = dx;
    y = dy;
    (void) canvas_coord_transform (canvas, &x, &y, FALSE);
    /*  Call event consumer functions  */
    data.x = x;
    data.y = y;
    data.value = value;
    data.event_code = event_code;
    data.e_info = e_info;
    data.x_lin = dx;
    data.y_lin = dy;
    return ( c_call_callbacks ( (*holder).position_list, &data ) );
}   /*  End Function worldcanvas_position_func  */

static flag position_event_func (void *object, void *client1_data,
				 void *call_data, void *client2_data)
/*  This routine is called when object callbacks are called.
    The object information pointer will be given by  object  .
    The first client information pointer will be given by  client1_data  .
    The call information pointer will be given by  call_data  .
    The second client information pointer will be given by  client2_data  .
    The routine returns TRUE if further callbacks should not be called.
*/
{
    CanvasHolder holder;
    ViewableImage vimage;
    flag (*func) ();
    struct position_struct *data;
    static char function_name[] = "position_event_func";

    holder = (CanvasHolder) object;
    vimage = (*holder).active_image;
    VERIFY_VIMAGE (vimage);
    data = (struct position_struct *) call_data;
    func = ( flag (*) () ) client2_data;
    return ( (*func) (vimage, (*data).x, (*data).y, (*data).value,
		      (*data).event_code, (*data).e_info, client1_data,
		      (*data).x_lin, (*data).y_lin) );
}   /*  End Function position_event_func  */

static void aspect_zoom (int hlength, int width, int *hpixels, flag int_x,
			 int vlength, int height, int *vpixels, flag int_y)
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
{
    int hfactor, vfactor;
    int factor;
    static char function_name[] = "aspect_zoom";

    /*  NOTE:  a 1:1 zoom ratio is termed a zoom out of factor 1  */
    /*  Zoom in: replicate image data.    Zoom out: shrink image data  */
    if (int_x && int_y)
    {
	/*  Both axes require integral zoom factors  */
	if ( (width > hlength) && (height > vlength) )
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

static flag coord_convert_func (KWorldCanvas canvas,
				struct win_scale_type *win_scale,
				int *px, int *py, double *wx, double *wy,
				flag to_world, void **info)
/*  This routine will modify the window scaling information for a world
    canvas.
    The canvas is given by  canvas  .
    The window scaling information is pointed to by  win_scale  .The data
    herein may be modified.
    The horizontal pixel co-ordinate storage must be pointed to by  px  .
    The vertical pixel co-ordinate storage must be pointed to by  py  .
    The horizontal world co-ordinate storage must be pointed to by  wx  .
    The vertical world co-ordinate storage must be pointed to by  wy  .
    If the value of  to_world  is TRUE, then a pixel to world co-ordinate
    transform is required, else a world to pixel co-ordinate transform is
    required.
    The arbitrary canvas information pointer is pointed to by  info  .
    The routine should return TRUE if the conversion was completed,
    else it returns FALSE indicating that the default conversions should be
    used.
*/
{
    CanvasHolder holder;
    ViewableImage vimage;
    int x_pix, y_pix;
    int coord_num0, coord_num1;
    double coord0, coord1;
    double world_x, world_y;
    array_desc *arr_desc;
    packet_desc *pack_desc;
    dim_desc *hdim;
    dim_desc *vdim;
    static char function_name[] = "coord_convert_func";

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
    if ( (vimage = (*holder).active_image) == NULL ) return (FALSE);
    if ( (*vimage).tc_arr_desc == NULL )
    {
	arr_desc = (*vimage).pc_arr_desc;
	hdim = (*arr_desc).dimensions[(*vimage).pc_hdim];
	vdim = (*arr_desc).dimensions[(*vimage).pc_vdim];
    }
    else
    {
	arr_desc = (*vimage).tc_arr_desc;
	hdim = (*arr_desc).dimensions[(*vimage).tc_hdim];
	vdim = (*arr_desc).dimensions[(*vimage).tc_vdim];
    }
    pack_desc = (*arr_desc).packet;
    if (to_world)
    {
	/*  Convert from pixel to world co-ordinates  */
	x_pix = *px - (*win_scale).x_offset;
	y_pix = *py - (*win_scale).y_offset;
	y_pix = (*win_scale).y_pixels - y_pix - 1;
	/*  Removed offsets and flipped vertical  */
	/*  Compute x co-ordinate  */
	coord_num0 = ds_get_coord_num (hdim, (*win_scale).x_min,
				       SEARCH_BIAS_CLOSEST);
	coord0 = ds_get_coordinate (hdim, coord_num0);
	coord_num1 = ds_get_coord_num (hdim, (*win_scale).x_max,
				       SEARCH_BIAS_CLOSEST);
	if (coord_num1 + 1 < (*hdim).length)
	{
	    coord1 = ds_get_coordinate (hdim, coord_num1 + 1);
	}
	else
	{
	    coord1 = ds_get_coordinate (hdim, coord_num1);
	    coord1 += (coord1 - coord0) / (coord_num1 - coord_num0);
	}
	*wx = (coord1 - coord0) * (double) x_pix;
	*wx /= (double) ( (*win_scale).x_pixels );
	*wx += coord0;
	*wx += (coord1 - coord0) / (double) (coord_num1 - coord_num0) / 1000.0;
	/*  Compute y co-ordinate  */
	coord_num0 = ds_get_coord_num (vdim, (*win_scale).y_min,
				       SEARCH_BIAS_CLOSEST);
	coord0 = ds_get_coordinate (vdim, coord_num0);
	coord_num1 = ds_get_coord_num (vdim, (*win_scale).y_max,
				       SEARCH_BIAS_CLOSEST);
	if (coord_num1 + 1 < (*vdim).length)
	{
	    coord1 = ds_get_coordinate (vdim, coord_num1 + 1);
	}
	else
	{
	    coord1 = ds_get_coordinate (vdim, coord_num1);
	    coord1 += (coord1 - coord0) / (coord_num1 - coord_num0);
	}
	*wy = (coord1 - coord0) * (double) y_pix;
	*wy /= (double) ( (*win_scale).y_pixels );
	*wy += (coord1 - coord0) / (double) (coord_num1 - coord_num0) / 1000.0;
	*wy += coord0;
	return (TRUE);
    }
    /*  Convert from world to pixel co-ordinates  */
    /*  Compute x co-ordinate  */
    coord_num0 = ds_get_coord_num (hdim, (*win_scale).x_min,
				   SEARCH_BIAS_CLOSEST);
    coord0 = ds_get_coordinate (hdim, coord_num0);
    coord_num1 = ds_get_coord_num (hdim, (*win_scale).x_max,
				   SEARCH_BIAS_CLOSEST);
    if (coord_num1 + 1 < (*hdim).length)
    {
	coord1 = ds_get_coordinate (hdim, coord_num1 + 1);
    }
    else
    {
	coord1 = ds_get_coordinate (hdim, coord_num1);
	coord1 += (coord1 - coord0) / (coord_num1 - coord_num0);
    }
    world_x = (*wx - coord0) * (double) ( (*win_scale).x_pixels );
    world_x /= coord1 - coord0;
    world_x += 0.01;
    *px = (int) world_x + (*win_scale).x_offset;
    /*  Compute y co-ordinate  */
    coord_num0 = ds_get_coord_num (vdim, (*win_scale).y_min,
				   SEARCH_BIAS_CLOSEST);
    coord0 = ds_get_coordinate (vdim, coord_num0);
    coord_num1 = ds_get_coord_num (vdim, (*win_scale).y_max,
				   SEARCH_BIAS_CLOSEST);
    if (coord_num1 + 1 < (*vdim).length)
    {
	coord1 = ds_get_coordinate (vdim, coord_num1 + 1);
    }
    else
    {
	coord1 = ds_get_coordinate (vdim, coord_num1);
	coord1 += (coord1 - coord0) / (coord_num1 - coord_num0);
    }
    world_y = (*wy - coord0) * (double) ( (*win_scale).y_pixels );
    world_y /= coord1 - coord0;
    world_y += 0.01;
    y_pix = (int) (world_y + 0.01);
    /*  Flip vertical  */
    *py = (*win_scale).y_offset + (*win_scale).y_pixels - 1 - y_pix;
    return (TRUE);
}   /*  End Function coord_convert_func  */

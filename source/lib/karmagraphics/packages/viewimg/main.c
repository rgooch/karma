/*LINTLIBRARY*/
/*  main.c

    This code provides ViewableImage objects.

    Copyright (C) 1993-1996  Richard Gooch

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

    Updated by      Richard Gooch   27-DEC-1994: Allow NULL multi_array
  pointers to be passed.

    Updated by      Richard Gooch   16-MAR-1995: Generate PostScript directly
  from TrueColour images if available, rather than compressed PseudoColour.

    Updated by      Richard Gooch   31-MAR-1995: Allow RGB movies on
  PseudoColour canvas, but print a warning.

    Updated by      Richard Gooch   15-JUN-1995: Made use of IS_ALIGNED macro.

    Updated by      Richard Gooch   13-JUL-1995: Added
  VIEWIMG_ATT_ALLOW_TRUNCATION attribute.

    Updated by      Richard Gooch   26-JUL-1995: Added support for position
  events on TrueColour ViewableImages.

    Updated by      Richard Gooch   6-SEP-1995: Eased restrictions on tiled
  arrays and created <viewimg_get_attributes>.

    Updated by      Richard Gooch   19-SEP-1995: Ensure canvas size is at least
  3*3 in <worldcanvas_refresh_func> and <worldcanvas_size_control_func>

    Updated by      Richard Gooch   5-JAN-1996: Switched to
  <canvas_register_d_convert_func> routine.

    Updated by      Richard Gooch   21-JAN-1996: Removed warnings about tiled
  arrays for PseudoColour and added for TrueColour.

    Updated by      Richard Gooch   19-FEB-1996: Created
  <viewimg_partial_refresh> routine.

    Updated by      Richard Gooch   28-FEB-1996: Prevent computing image twice
  in some circumstances where not needed and initialise win_scale field of
  viewable image properly. Uninitialised data caused problems in
  <worldcanvas_size_control_func> on alpha_OSF1.

    Updated by      Richard Gooch   27-MAR-1996: Check for genuine data change
  in <worldcanvas_size_control_func> and clear changed flag and set recompute
  flag to prevent message about spurious data change under some conditions.

    Updated by      Richard Gooch   15-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   12-MAY-1996: Added full support for
  non-integral zooming.

    Updated by      Richard Gooch   19-MAY-1996: Changed from using window
  scale structure to using <canvas_get_attributes>.

    Updated by      Richard Gooch   23-MAY-1996: Worked around problem with
  floating point comparisons in <worldcanvas_size_control_func> when running
  under i386_Linux with gcc 2.7.2

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   28-MAY-1996: Fixed bug in
  <worldcanvas_refresh_func> which caused image to be drawn twice if cache is
  invalidated within the <kwin> package (i.e. PostScriptPage refresh).

    Updated by      Richard Gooch   31-MAY-1996: Fixed bug in <determine_size>
  when zooming out without aspect ratio fixed: image could be smaller than
  necessary.

    Updated by      Richard Gooch   4-JUN-1996: Switched to left-right
  bottom-top co-ordinate specification instead of min-max x and y.

    Updated by      Richard Gooch   10-JUN-1996: Added panning support.

    Updated by      Richard Gooch   12-JUN-1996: Optimised panning support.

    Updated by      Richard Gooch   16-JUN-1996: Fixed handling of negative
  co-ordinate systems in <worldcanvas_position_func>.

    Updated by      Richard Gooch   21-JUN-1996: Created
  <viewimg_get_worldcanvas>.

    Updated by      Richard Gooch   18-JUL-1996: Added offset of half a data
  value when converting from world to pixel co-ordinates. This means that a
  point drawn at the co-ordinate for a data value will be drawn in the centre
  of the box of pixels representing that data value, rather than the
  bottom-left corner.

    Updated by      Richard Gooch   20-JUL-1996: Switched to
  <canvas_register_coords_convert_func>.

    Updated by      Richard Gooch   22-JUL-1996: Created
  <viewimg_create_sequence_from_iarray>.

    Updated by      Richard Gooch   6-AUG-1996: Added VIEWIMG_VATT_MULTI_ARRAY
  attribute.

    Updated by      Richard Gooch   31-AUG-1996: Made <viewimg_partial_refresh>
  more efficient when clearing small areas outside the image.

    Updated by      Richard Gooch   5-OCT-1996: Detect changes to intensity
  scaling function.

    Updated by      Richard Gooch   11-OCT-1996: Fixed bug in previous change
  when image minimum and maximum not available (i.e. no auto intensity scaling)

    Updated by      Richard Gooch   2-NOV-1996: Added VIEWIMG_VATT_DATA_SCALE
  and VIEWIMG_VATT_DATA_OFFSET attributes. Created <viewimg_set_attributes> and
  <viewimg_set_sequence_scale> routines.

    Updated by      Richard Gooch   10-NOV-1996: Created
  <viewimg_set_array_attributes> and removed <viewimg_set_sequence_scale>
  routines.

    Updated by      Richard Gooch   27-NOV-1996: Changed <viewimg_set_active>
  to not refresh canvas if it's not visible.

    Last updated by Richard Gooch   8-DEC-1996: Created <copy_restrictions> to
  fix restriction copying: was just copying pointers incorrectly.


*/
#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <stdarg.h>
#include <karma.h>
#define KWIN_GENERIC_ONLY
#define NEW_WIN_SCALE
#include <karma_viewimg.h>
#include <karma_iarray.h>
#include <karma_imc.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_c.h>
#include <os.h>


#define VIMAGE_MAGIC_NUMBER (unsigned int) 229573367
#define HOLDER_MAGIC_NUMBER (unsigned int) 1654545154

#define DEFAULT_NUMBER_OF_COLOURS 200

#define VERIFY_VIMAGE(vimage) {if (vimage == NULL) \
{fprintf (stderr, "NULL viewable image passed\n"); \
 a_prog_bug (function_name); } \
if (vimage->magic_number != VIMAGE_MAGIC_NUMBER) \
{fprintf (stderr, "Invalid viewable image object\n"); \
 a_prog_bug (function_name); } }

#define NUM_ISCALE_VALUES 10
static double iscale_factors[NUM_ISCALE_VALUES] =
{0.01, 0.1, 0.2, 0.5, 0.51, 0.55, 0.8, 0.9, 0.99, 1.0};


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
    flag allow_truncation;
    flag enable_panning;
    long pan_centre_x;
    long pan_centre_y;
    int pan_magnification;
    packet_desc *old_cmap;
};

struct sequence_holder_type
{
    unsigned int attachments;
    unsigned int fdim;
};

struct canvas_override_type
{
    double value_min;
    double value_max;
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
    double pc_data_scale;
    double pc_data_offset;
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
    flag recompute;        /* Whether the image cache needs to be recomputed */
    flag changed;          /* Whether the array data has changed             */
    int pixcanvas_width;
    int pixcanvas_height;
    SequenceHolder sequence;
    KPixCanvasImageCache cache;
    unsigned int num_restrictions;
    char **restriction_names;
    double *restriction_values;
    double iscale_test[NUM_ISCALE_VALUES];
    struct canvas_override_type override;
    ViewableImage next;
    ViewableImage prev;
    struct win_scale_type win_scale;
};

struct position_struct
{
    double x;
    double y;
    void *value;
    int event_code;
    void *e_info;
    double x_lin;
    double y_lin;
    unsigned int type;
};

typedef flag (*IscaleFunc) (double *out, unsigned int out_stride,
			    CONST double *inp, unsigned int inp_stride,
			    unsigned int num_values,
			    double i_min, double i_max, void *info);


/*  Private data  */
static CanvasHolder first_canvas_holder = NULL;


/*  Private functions  */
STATIC_FUNCTION (CanvasHolder get_canvas_holder,
		 (KWorldCanvas canvas, flag alloc, char *func_name) );
STATIC_FUNCTION (CanvasHolder alloc_canvas_holder, (KWorldCanvas canvas) );
STATIC_FUNCTION (void worldcanvas_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale,
		  Kcolourmap cmap, flag cmap_resize,
		  void **info, PostScriptPage pspage,
		  unsigned int num_areas, KPixCanvasRefreshArea *areas,
		  flag *honoured_areas) );
STATIC_FUNCTION (void recompute_image,
		 (CanvasHolder holder, int width, int height,
		  struct win_scale_type *win_scale, Kcolourmap cmap,
		  ViewableImage vimage) );
STATIC_FUNCTION (void worldcanvas_size_control_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale, void **info,
		  flag *boundary_clear) );
STATIC_FUNCTION (flag determine_size,
		 (CanvasHolder holder, int width, int height,
		  struct win_scale_type *win_scale,
		  long *hstart, long *hend, long *vstart, long *vend) );
STATIC_FUNCTION (void aspect_zoom,
		 (long hlength, int *hpixels, flag int_x,
		  long vlength, int *vpixels, flag int_y) );
STATIC_FUNCTION (void trunc_zoom,
		 (flag maintain_aspect_ratio,
		  long *hstart, long *hend, int *hpixels, flag int_x,
		  long *vstart, long *vend, int *vpixels, flag int_y) );
STATIC_FUNCTION (void draw_subcache,
		 (CanvasHolder holder, ViewableImage vimage,
		  int width, int height) );
STATIC_FUNCTION (flag worldcanvas_position_func,
		 (KWorldCanvas canvas, double x, double y,
		  unsigned int event_code, void *e_info, void **f_info,
		  double x_lin, double y_lin) );
STATIC_FUNCTION (flag position_event_func,
		 (void *object, void *client1_data,
		  void *call_data, void *client2_data) );
STATIC_FUNCTION (flag coord_convert_func,
		 (KWorldCanvas canvas, unsigned int num_coords,
		  CONST double *xin, CONST double *yin,
		  double *xout, double *yout, flag to_world, void **info) );
STATIC_FUNCTION (void initialise_vimage, (ViewableImage vimage) );
STATIC_FUNCTION (flag copy_restrictions,
		 (ViewableImage vimage, unsigned int num_restr,
		  CONST char **restr_names, CONST double *restr_values) );


/* Public functions follow */

/*PUBLIC_FUNCTION*/
void viewimg_init (KWorldCanvas canvas)
/*  [SUMMARY] Initialise the package for a particular canvas.
    [PURPOSE] This routine will initialise the [<viewimg>] package for a
    particular world canvas. Calling this routine causes a number of callback
    routines internal to the package to be registered with the canvas (such
    as refresh and position event callbacks). The use of this routine is
    optional at the moment: the routines which create viewable images perform
    this function automatically. In version 2.0 of Karma, this use of this
    routine before creating viewable images will become mandatory.
    <canvas> The world canvas object.
    [RETURNS] Nothing.
*/
{
    extern CanvasHolder first_canvas_holder;
    static char function_name[] = "viewimg_init";

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
				    CONST char **restr_names,
				    CONST double *restr_values)
/*  [SUMMARY] Create viewable image from 2D slice with restrictions.
    [PURPOSE] This routine will create a PseudoColour viewable image object
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
    restrictions are recorded (this is the same as calling [<viewimg_create>]).
    <restr_names> The restriction names.
    <restr_values> The restriction values.
    [NOTE] Restriction information is automatically deallocated when
    [<viewimg_destroy>] is called.
    [NOTE] The routine may produce cache data which will vastly increase the
    speed of subsequent operations on this data. Prior to process exit, a call
    MUST be made to [<viewimg_destroy>], otherwise shared memory segments could
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
    /*  Create new viewable image  */
    if ( ( vimage = (ViewableImage) m_alloc (sizeof *vimage) ) == NULL )
    {
	m_error_notify (function_name, "viewable image");
	return (NULL);
    }
    initialise_vimage (vimage);
    vimage->canvas_holder = holder;
    vimage->pc_multi_desc = multi_desc;
    vimage->pc_arr_desc = arr_desc;
    vimage->pc_slice = slice;
    vimage->pc_hdim = hdim;
    vimage->pc_vdim = vdim;
    vimage->pc_elem_index = elem_index;
    /*  Compute strides  */
    vimage->pc_hstride = ds_get_packet_size (arr_desc->packet);
    for (dim_count = arr_desc->num_dimensions - 1; dim_count > hdim;
	 --dim_count)
    {
	vimage->pc_hstride *= arr_desc->dimensions[dim_count]->length;
    }
    vimage->pc_vstride = ds_get_packet_size (arr_desc->packet);
    for (dim_count = arr_desc->num_dimensions - 1; dim_count > vdim;
	 --dim_count)
    {
	vimage->pc_vstride *= arr_desc->dimensions[dim_count]->length;
    }
    vimage->pc_data_scale = 1.0;
    vimage->pc_data_offset = 0.0;
    if ( !copy_restrictions (vimage, num_restr, restr_names, restr_values) )
    {
	m_clear ( (char *) vimage, sizeof *vimage );
	m_free ( (char *) vimage );
	return (NULL);
    }
    /*  Do not make the first viewable image for this canvas viewable, else
	the  viewimg_make_active  routine will not be able to detect a change
	in the active viewable image and will do nothing if called with this
	newly created viewable image.
	*/
    if (holder->first_image != NULL)
    {
	/*  Insert at beginning of list  */
	holder->first_image->prev = vimage;
    }
    vimage->next = holder->first_image;
    holder->first_image = vimage;
    /*  Attach  */
    if (multi_desc != NULL) ++multi_desc->attachments;
    return (vimage);
}   /*  End Function viewimg_create_restr  */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_create (KWorldCanvas canvas, multi_array *multi_desc,
			      array_desc *arr_desc, char *slice,
			      unsigned int hdim, unsigned int vdim,
			      unsigned int elem_index)
/*  [SUMMARY] Create viewable image from 2D slice.
    [PURPOSE] This routine will create a PseudoColour viewable image object
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
    [NOTE] The routine may produce cache data which will vastly increase the
    speed of subsequent operations on this data. Prior to process exit, a call
    MUST be made to [<viewimg_destroy>], otherwise shared memory segments could
    remain after the process exits.
    [RETURNS] A viewable image on success, NULL.
*/
{
    /*static char function_name[] = "viewimg_create";*/

    return ( viewimg_create_restr (canvas, multi_desc, arr_desc, slice,
				   hdim, vdim, elem_index,
				   0, NULL, (double *) NULL) );
}   /*  End Function viewimg_create  */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_create_from_iarray (KWorldCanvas canvas, iarray array,
					  flag swap)
/*  [SUMMARY] Create a viewable image from an Intelligent Array.
    [PURPOSE] This routine will create a viewable image object from a
    2-dimensional Intelligent Array. At a later time, this viewable image may
    be made visible. This routine will not cause the canvas to be refreshed.
    Many viewable images may be associated with a single canvas.
    <canvas> The world canvas object.
    <array> The Intelligent Array. The underlying <<multi_array>> data
    structure will have its attachment count incremented upon successful
    completion.
    <swap> If TRUE the y axis will be displayed horizontally.
    [NOTE] The routine may produce cache data which will vastly increase the
    speed of subsequent operations on this data. Prior to process exit, a call
    MUST be made to [<viewimg_destroy>], otherwise shared memory segments could
    remain after the process exits.
    [RETURNS] A viewable image on success, else NULL.
*/
{
    unsigned int num_restr;
    char **restr_names;
    double *restr_values;
    static char function_name[] = "viewimg_create_from_iarray";

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
	fprintf (stderr,
			"Intelligent Array must have exactly 2 dimensions\n");
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
	return ( viewimg_create_restr (canvas, array->multi_desc,
				       array->arr_desc, array->data,
				       array->orig_dim_indices[0],
				       array->orig_dim_indices[1],
				       array->elem_index,
				       num_restr, (CONST char **) restr_names,
				       restr_values) );
    }
    else
    {
	return ( viewimg_create_restr (canvas, array->multi_desc,
				       array->arr_desc, array->data,
				       array->orig_dim_indices[1],
				       array->orig_dim_indices[0],
				       array->elem_index,
				       num_restr, (CONST char **) restr_names,
				       restr_values) );
    }
}   /*  End Function viewimg_create_from_iarray  */

/*PUBLIC_FUNCTION*/
ViewableImage *viewimg_create_sequence (KWorldCanvas canvas,
					multi_array *multi_desc,
					array_desc *arr_desc, char *cube,
					unsigned int hdim, unsigned int vdim,
					unsigned int fdim,
					unsigned int elem_index)
/*  [SUMMARY] Create a sequence of viewable images from a 3D slice.
    [PURPOSE] This routine will create a sequence of viewable image objects
    from a 3-dimensional cube of a Karma data structure. At a later time, this
    sequence of viewable images may be made visible in any order.
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
    [NOTE] The routine may produce cache data which will vastly increase the
    speed of subsequent operations on this data. Prior to process exit, a call
    MUST be made to [<viewimg_destroy>], otherwise shared memory segments could
    remain after the process exits.
    [RETURNS] A pointer to a dynamically allocated array of viewable image
    objects on success, else NULL.
*/
{
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
	fprintf (stderr,
			"fdim: %u greater than number of dimensions: %u\n",
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
	restr_values[0] = ds_get_coordinate (arr_desc->dimensions[fdim],
					     frame_count);
	if ( ( restr_names[0] =
	       st_dup (arr_desc->dimensions[fdim]->name) ) == NULL )
	{
	    m_abort (function_name, "restriction name");
	}
	if ( ( vimages[frame_count] =
	       viewimg_create_restr (canvas, multi_desc, arr_desc,
				     cube + foffsets[frame_count],
				     hdim, vdim, elem_index,
				     num_restr, (CONST char **) restr_names,
				     restr_values) )
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
ViewableImage *viewimg_create_sequence_from_iarray (KWorldCanvas canvas,
						    iarray array,
						    unsigned int hdim,
						    unsigned int vdim,
						    unsigned int fdim)
/*  [SUMMARY] Create a sequence of viewable images from an Intelligent Array.
    [PURPOSE] This routine will create a sequence of viewable image objects
    from a 3-dimensional Intelligent Array. At a later time, this sequence of
    viewable images may be made visible in any order.
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
    [NOTE] The routine may produce cache data which will vastly increase the
    speed of subsequent operations on this data. Prior to process exit, a call
    MUST be made to [<viewimg_destroy>], otherwise shared memory segments could
    remain after the process exits.
    [RETURNS] A pointer to a dynamically allocated array of viewable image
    objects on success, else NULL.
*/
{
    static char function_name[] = "viewimg_create_sequence_from_iarray";

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
	fprintf (stderr,
			"Intelligent Array must have exactly 3 dimensions\n");
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
    return ( viewimg_create_sequence (canvas, array->multi_desc,
				      array->arr_desc, array->data,
				      array->orig_dim_indices[hdim],
				      array->orig_dim_indices[vdim],
				      array->orig_dim_indices[fdim],
				      array->elem_index) );
}   /*  End Function viewimg_create_sequence_from_iarray  */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_create_rgb (KWorldCanvas canvas, multi_array *multi_desc,
				  array_desc *arr_desc, char *slice,
				  unsigned int hdim, unsigned int vdim,
				  unsigned int red_index,
				  unsigned int green_index,
				  unsigned int blue_index, unsigned num_restr,
				  CONST char **restr_names,
				  CONST double *restr_values)
/*  [SUMMARY] Create a TrueColour viewable image.
    [PURPOSE] This routine will create a TrueColour viewable image object from
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
    [NOTE] The 3 colour components must be of type  K_UBYTE  .
    <num_restr> The number of matched restrictions. If this is 0, no
    restrictions are recorded.
    <restr_names> The restriction names.
    <restr_values> The restriction values.
    [NOTE] Restriction information is automatically deallocated when
    [<viewimg_destroy>] is called.
    [NOTE] The routine may produce cache data which will vastly increase the
    speed of subsequent operations on this data. Prior to process exit, a call
    MUST be made to [<viewimg_destroy>], otherwise shared memory segments could
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
    if (arr_desc->num_levels > 0)
    {
	fprintf (stderr, "%s: Tiled array. May cause problems.\n",
		 function_name);
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
    if (red_index >= arr_desc->packet->num_elements)
    {
	fprintf (stderr, "red_index: %u greater than number of elements: %u\n",
		 red_index, arr_desc->packet->num_elements);
	a_prog_bug (function_name);
    }
    if (arr_desc->packet->element_types[red_index] != K_UBYTE)
    {
	fprintf (stderr, "Red component type: %u is not K_UBYTE\n",
		 arr_desc->packet->element_types[red_index]);
	return (NULL);
    }
    if (green_index >= arr_desc->packet->num_elements)
    {
	fprintf (stderr,
		 "green_index: %u greater than number of elements: %u\n",
		 green_index, arr_desc->packet->num_elements);
	a_prog_bug (function_name);
    }
    if (arr_desc->packet->element_types[green_index] != K_UBYTE)
    {
	fprintf (stderr, "Green component type: %u is not K_UBYTE\n",
		 arr_desc->packet->element_types[green_index]);
	return (NULL);
    }
    if (blue_index >= arr_desc->packet->num_elements)
    {
	fprintf (stderr,
		 "blue_index: %u greater than number of elements: %u\n",
		 blue_index, arr_desc->packet->num_elements);
	a_prog_bug (function_name);
    }
    if (arr_desc->packet->element_types[blue_index] != K_UBYTE)
    {
	fprintf (stderr, "Blue component type: %u is not K_UBYTE\n",
		 arr_desc->packet->element_types[blue_index]);
	return (NULL);
    }
    kwin_get_attributes (canvas_get_pixcanvas (canvas),
			 KWIN_ATT_DEPTH, &depth,
			 KWIN_ATT_END);
    if ( (arr_desc->num_levels > 0) && (depth != 24) )
    {
	fprintf (stderr, "%s: Tiling not supported for non 24 bit canvases.\n",
		 function_name);
	return (NULL);
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
    /*  Create new viewable image  */
    if ( ( vimage = (ViewableImage) m_alloc (sizeof *vimage) ) == NULL )
    {
	m_error_notify (function_name, "viewable image");
	return (NULL);
    }
    initialise_vimage (vimage);
    vimage->canvas_holder = holder;
    vimage->tc_multi_desc = multi_desc;
    vimage->tc_arr_desc = arr_desc;
    vimage->tc_slice = slice;
    vimage->tc_hdim = hdim;
    vimage->tc_vdim = vdim;
    vimage->tc_red_index = red_index;
    vimage->tc_green_index = green_index;
    vimage->tc_blue_index = blue_index;
    /*  Compute strides  */
    vimage->tc_hstride = ds_get_packet_size (arr_desc->packet);
    for (dim_count = arr_desc->num_dimensions - 1; dim_count > hdim;
	 --dim_count)
    {
	vimage->tc_hstride *= arr_desc->dimensions[dim_count]->length;
    }
    vimage->tc_vstride = ds_get_packet_size (arr_desc->packet);
    for (dim_count = arr_desc->num_dimensions - 1; dim_count > vdim;
	 --dim_count)
    {
	vimage->tc_vstride *= arr_desc->dimensions[dim_count]->length;
    }
    if ( !copy_restrictions (vimage, num_restr, restr_names, restr_values) )
    {
	m_clear ( (char *) vimage, sizeof *vimage );
	m_free ( (char *) vimage );
	return (NULL);
    }
    /*  Do not make the first viewable image for this canvas viewable, else
	the  viewimg_make_active  routine will not be able to detect a change
	in the active viewable image and will do nothing if called with this
	newly created viewable image.
	*/
    if (holder->first_image != NULL)
    {
	/*  Insert at beginning of list  */
	holder->first_image->prev = vimage;
    }
    vimage->next = holder->first_image;
    holder->first_image = vimage;
    /*  Attach  */
    if (multi_desc != NULL) ++multi_desc->attachments;
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
/*  [SUMMARY] Create sequence of TrueColour viewable images from 3D slice.
    [PURPOSE] This routine will create a sequence of TrueColour viewable image
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
    [NOTE] The 3 colour components must be of type K_UBYTE.
    <num_restr> The number of matched restrictions. If this is 0, no
    restrictions are recorded.
    <restr_names> The restriction names.
    <restr_values> The restriction values.
    [NOTE] Restriction information is copied into internally allocated
    storage.
    [NOTE] The routine may produce cache data which will vastly increase the
    speed of subsequent operations on this data. Prior to process exit, a call
    MUST be made to [<viewimg_destroy>], otherwise shared memory segments could
    remain after the process exits.
    [RETURNS] A viewable image on success, else NULL.
*/
{
    SequenceHolder sequence;
    unsigned int frame_count;
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
	fprintf (stderr, "NULL world canvas passed\n");
	a_prog_bug (function_name);
    }
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
    if (arr_desc->num_levels > 0)
    {
	fprintf (stderr, "%s: Tiled array. May cause problems.\n",
		 function_name);
    }
    /*  Sanity checks  */
    if (fdim >= arr_desc->num_dimensions)
    {
	fprintf (stderr, "fdim: %u greater than number of dimensions: %u\n",
		 fdim, arr_desc->num_dimensions);
	a_prog_bug (function_name);
    }
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
    if (red_index >= arr_desc->packet->num_elements)
    {
	fprintf (stderr, "red_index: %u greater than number of elements: %u\n",
		 red_index, arr_desc->packet->num_elements);
	a_prog_bug (function_name);
    }
    if (arr_desc->packet->element_types[red_index] != K_UBYTE)
    {
	fprintf (stderr, "Red component type: %u is not K_UBYTE\n",
		 arr_desc->packet->element_types[red_index]);
	return (NULL);
    }
    if (green_index >= arr_desc->packet->num_elements)
    {
	fprintf (stderr,
		 "green_index: %u greater than number of elements: %u\n",
		 green_index, arr_desc->packet->num_elements);
	a_prog_bug (function_name);
    }
    if (arr_desc->packet->element_types[green_index] != K_UBYTE)
    {
	fprintf (stderr, "Green component type: %u is not K_UBYTE\n",
		 arr_desc->packet->element_types[green_index]);
	return (NULL);
    }
    if (blue_index >= arr_desc->packet->num_elements)
    {
	fprintf (stderr,
		 "blue_index: %u greater than number of elements: %u\n",
		 blue_index, arr_desc->packet->num_elements);
	a_prog_bug (function_name);
    }
    if (arr_desc->packet->element_types[blue_index] != K_UBYTE)
    {
	fprintf (stderr, "Blue component type: %u is not K_UBYTE\n",
		 arr_desc->packet->element_types[blue_index]);
	return (NULL);
    }
    kwin_get_attributes (canvas_get_pixcanvas (canvas),
			 KWIN_ATT_DEPTH, &depth,
			 KWIN_ATT_END);
    if ( (arr_desc->num_levels > 0) && (depth != 24) )
    {
	fprintf (stderr, "%s: Tiling not supported for non 24 bit canvases.\n",
		 function_name);
	return (NULL);
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
    sequence->attachments = 0;
    sequence->fdim = fdim;
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
	r_values[num_restr] = ds_get_coordinate (arr_desc->dimensions[fdim],
						 frame_count);
	if ( ( r_names[num_restr] =
	       st_dup (arr_desc->dimensions[fdim]->name) ) == NULL )
	{
	    m_abort (function_name, "restriction name");
	}
	if ( ( vimages[frame_count] =
	       viewimg_create_rgb (canvas, multi_desc, arr_desc,
				   cube + foffsets[frame_count],
				   hdim, vdim,
				   red_index, green_index, blue_index,
				   num_restr + 1, (CONST char **) r_names,
				   r_values) )
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
	vimages[frame_count]->sequence = sequence;
	++sequence->attachments;
    }
    return (vimages);
}   /*  End Function viewimg_create_rgb_sequence  */

/*PUBLIC_FUNCTION*/
flag viewimg_make_active (ViewableImage vimage)
/*  [SUMMARY] Make viewable image active and possibly refresh.
    [PURPOSE] This routine will make a viewable image the active image for its
    associated world canvas. The canvas is then refreshed (possibly resized),
    provided that the new viewable image was not already active.
    <vimage> The viewable image.
    [RETURNS] TRUE on success, else FALSE.
    [SEE ALSO] [<viewimg_set_active>].
*/
{
    CanvasHolder holder;
    static char function_name[] = "viewimg_make_active";

    VERIFY_VIMAGE (vimage);
    holder = vimage->canvas_holder;
    if (vimage == holder->active_image) return (TRUE);
    return ( viewimg_set_active (vimage, TRUE) );
}   /*  End Function viewimg_make_active  */

/*PUBLIC_FUNCTION*/
flag viewimg_set_active (ViewableImage vimage, flag refresh)
/*  [SUMMARY] Make viewable image active with controlled refresh.
    [PURPOSE] This routine will make a viewable image the active image for its
    associated world canvas.
    <vimage> The viewable image.
    <refresh> If TRUE, the canvas is always refreshed, if FALSE, the canvas is
    not refreshed.
    [RETURNS] TRUE on success, else FALSE.
    [SEE ALSO] [<viewimg_make_active>].
*/
{
    CanvasHolder holder;
    flag visible;
    unsigned int hdim, vdim;
    dim_desc **dimensions;
    static char function_name[] = "viewimg_set_active";

    VERIFY_VIMAGE (vimage);
    FLAG_VERIFY (refresh);
    holder = vimage->canvas_holder;
    kwin_get_attributes (canvas_get_pixcanvas (holder->canvas),
			 KWIN_ATT_VISIBLE, &visible,
			 KWIN_ATT_END);
    if (!visible) refresh = FALSE;
    if (vimage == holder->active_image)
    {
	if (refresh)
	{
	    return ( canvas_resize (holder->canvas,
				    (struct win_scale_type *) NULL, FALSE) );
	}
	return (TRUE);
    }
    holder->active_image = vimage;
    if (vimage->tc_arr_desc == NULL)
    {
	dimensions = vimage->pc_arr_desc->dimensions;
	hdim = vimage->pc_hdim;
	vdim = vimage->pc_vdim;
    }
    else
    {
	dimensions = vimage->tc_arr_desc->dimensions;
	hdim = vimage->tc_hdim;
	vdim = vimage->tc_vdim;
    }
    if ( !canvas_specify (holder->canvas,
			  dimensions[hdim]->name,
			  dimensions[vdim]->name,
			  vimage->num_restrictions,
			  vimage->restriction_names,
			  vimage->restriction_values) )
    {
	return (FALSE);
    }
    if (refresh)
    {
	return ( canvas_resize (holder->canvas,
				(struct win_scale_type *) NULL, FALSE) );
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
    <canvas> The world canvas object.
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
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "viewimg_control_autoscaling";

    fprintf (stderr,
		    "Function: %s will be removed in Karma version 2.0\n",
		    function_name);
    fprintf (stderr, "Use:  viewimg_set_canvas_attributes  instead.\n");
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
/*  [SUMMARY] Notify data for viewable image has changed.
    [PURPOSE] This routine will register a change in the Karma data structure
    associated with a viewable image. If the viewable image is active, it will
    be immediately redrawn on its canvas.
    <vimage> The viewable image.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    CanvasHolder holder;
    static char function_name[] = "viewimg_register_data_change";

    VERIFY_VIMAGE (vimage);
    holder = vimage->canvas_holder;
    vimage->changed = TRUE;
    vimage->value_min = TOOBIG;
    vimage->value_max = TOOBIG;
    if (vimage == holder->active_image)
    {
	/*  Active image: refresh  */
	return ( canvas_resize (holder->canvas,
				(struct win_scale_type *) NULL, FALSE) );
    }
    return (TRUE);
}   /*  End Function viewimg_register_data_change  */

/*PUBLIC_FUNCTION*/
void viewimg_destroy (ViewableImage vimage)
/*  [SUMMARY] Destroy viewable image.
    [PURPOSE] This routine will destroy a viewable image. If this is not called
    prior to process exit, shared memory segments could remain after the
    process exits.
    <vimage> The viewable image.
    [NOTE] The associated <<multi_array>> descriptor is also deallocated (or
    at least, the attachment count is decreased).
    [RETURNS] Nothing.
*/
{
    CanvasHolder holder;
    SequenceHolder sequence;
    unsigned int count;
    static char function_name[] = "viewimg_destroy";

    VERIFY_VIMAGE (vimage);
    holder = vimage->canvas_holder;
    kwin_free_cache_data (vimage->cache);
    ds_dealloc_multi (vimage->pc_multi_desc);
    ds_dealloc_multi (vimage->tc_multi_desc);
    if ( (sequence = vimage->sequence) != NULL )
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
    if (vimage->next != NULL)
    {
	/*  Forward entry  */
	vimage->next->prev = vimage->prev;
    }
    if (vimage->prev != NULL)
    {
	vimage->prev->next = vimage->next;
    }
    if (vimage == holder->first_image)
    {
	/*  Remove from start of list  */
	holder->first_image = vimage->next;
    }
    if (vimage == holder->active_image)
    {
	holder->active_image = NULL;
    }
    if (vimage->restriction_names != NULL)
    {
	for (count = 0; count < vimage->num_restrictions; ++count)
	{
	    if (vimage->restriction_names[count] != NULL)
	    {
		m_free (vimage->restriction_names[count]);
	    }
	}
	m_free ( (char *) vimage->restriction_names );
    }
    if (vimage->restriction_values != NULL)
    {
	m_free ( (char *) vimage->restriction_values );
    }
    vimage->magic_number = 0;
    m_free ( (char *) vimage );
}   /*  End Function viewimg_destroy  */

/*PUBLIC_FUNCTION*/
ViewableImage viewimg_get_active (KWorldCanvas canvas)
/*  [SUMMARY] Get the active ViewableImage associated with a world canvas.
    <canvas> The world canvas object.
    [RETURNS] The active viewable image on success, else NULL (indicating no
    viewable image is active for the canvas).
*/
{
    CanvasHolder holder;
    static char function_name[] = "viewimg_get_active";

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
    return (holder->active_image);
}   /*  End Function viewimg_get_active  */

/*PUBLIC_FUNCTION*/
flag viewimg_test_active (ViewableImage vimage)
/*  [SUMMARY] Test if viewable image is active.
    [PURPOSE] This routine will test if a viewable image is the active image
    for its associated world canvas.
    <vimage> The viewable image.
    [RETURNS] TRUE if the viewable image is actice, else FALSE.
*/
{
    CanvasHolder holder;
    static char function_name[] = "viewimg_test_active";

    VERIFY_VIMAGE (vimage);
    holder = vimage->canvas_holder;
    if (vimage == holder->active_image) return (TRUE);
    return (FALSE);
}   /*  End Function viewimg_test_active  */

/*PUBLIC_FUNCTION*/
KCallbackFunc viewimg_register_position_event_func (KWorldCanvas canvas,
						    flag (*func) (),
						    void *f_info)
/*  [SUMMARY] Register position event callback.
    [PURPOSE] This routine will register a position event function for a world
    canvas which has a number of ViewableImage objects associated with it.
    The position event function will be called whenever a position event on the
    canvas has not been consumed. Many position event functions may be
    registered per canvas. The first function registered is the first function
    called upon a position event.
    <canvas> The world canvas object.
    <func> The function that is called when a position event occurs. The
    prototype function is [<VIEWIMG_PROTO_position_func>].
    <f_info> The initial arbitrary function information pointer.
    [RETURNS] A handle to a KWorldCanvasPositionFunc object.
*/
{
    CanvasHolder holder;
    static char function_name[] = "viewimg_register_position_func";

    if (canvas == NULL)
    {
	fprintf (stderr, "NULL canvas passed\n");
	a_prog_bug (function_name);
    }
    if ( ( holder = get_canvas_holder (canvas, TRUE, function_name) ) == NULL )
    {
	m_abort (function_name, "canvas holder");
    }
    return ( c_register_callback (&holder->position_list,
				  position_event_func,
				  holder, f_info, TRUE, (void *) func, FALSE,
				  TRUE) );
}   /*  End Function viewimg_register_position_func  */

/*PUBLIC_FUNCTION*/
flag viewimg_fill_ellipse (ViewableImage vimage,
			   double centre_x, double centre_y,
			   double radius_x, double radius_y, double value[2])
/*  [SUMMARY] Draw an ellipse into an array associated with a viewable image.
    [PURPOSE] This routine will draw a filled ellipse into a 2 dimensional
    slice of data associated with a viewable image.
    <vimage> The viewable image.
    <centre_x> The horizontal world co-ordinate of the centre of the ellipse.
    <centre_y> The vertical world co-ordinate of the centre of the ellipse.
    <radius_x> The horizontal radius.
    <radius_y> The vertical radius.
    <value> The complex value to fill the ellipse. This must be of type
    K_DCOMPLEX.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    packet_desc *pack_desc;
    array_desc *arr_desc;
    static char function_name[] = "viewimg_fill_ellipse";

    VERIFY_VIMAGE (vimage);
#ifdef MACHINE_i386
    if ( !IS_ALIGNED ( value, sizeof (float) ) )
#else
    if ( !IS_ALIGNED ( value, sizeof (double) ) )
#endif
    {
	fprintf (stderr,
			"value  address: %p not on a double boundary\n",
			value);
	a_prog_bug (function_name);
    }
    if (vimage->tc_arr_desc != NULL)
    {
	fprintf (stderr, "%s: TrueColour images not supported yet\n",
			function_name);
	return (FALSE);
    }
    arr_desc = vimage->pc_arr_desc;
    pack_desc = arr_desc->packet;
    return ( ds_draw_ellipse
	    (vimage->pc_slice,
	     pack_desc->element_types[vimage->pc_elem_index],
	     arr_desc->dimensions[vimage->pc_hdim], vimage->pc_hstride,
	     arr_desc->dimensions[vimage->pc_vdim], vimage->pc_vstride,
	     centre_x, centre_y, radius_x, radius_y, value) );
}   /*  End Function viewimg_fill_ellipse  */

/*PUBLIC_FUNCTION*/
flag viewimg_fill_polygon (ViewableImage vimage, edit_coord *coords,
			   unsigned int num_vertices, double value[2])
/*  [SUMMARY] Draw a polygon into an array associated with a viewable image.
    [PURPOSE] This routine will draw a filled polygon into a 2 dimensional
    slice of data associated with a viewable image.
    <vimage> The viewable image.
    <coords> The array of world co-ordinates of vertices of the polygon.
    <num_vertices> The number of vertices in the polygon.
    <value> The complex value to fill the polygon with. This must be of type
    K_DCOMPLEX.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    packet_desc *pack_desc;
    array_desc *arr_desc;
    static char function_name[] = "viewimg_fill_polygon";

    VERIFY_VIMAGE (vimage);
#ifdef MACHINE_i386
    if ( !IS_ALIGNED ( value, sizeof (float) ) )
#else
    if ( !IS_ALIGNED ( value, sizeof (double) ) )
#endif
    {
	fprintf (stderr,
			"value  address: %p not on a double boundary\n",
			value);
	a_prog_bug (function_name);
    }
    if (vimage->tc_arr_desc != NULL)
    {
	fprintf (stderr, "%s: TrueColour images not supported yet\n",
			function_name);
	return (FALSE);
    }
    arr_desc = vimage->pc_arr_desc;
    pack_desc = arr_desc->packet;
    return ( ds_draw_polygon
	    (vimage->pc_slice,
	     pack_desc->element_types[vimage->pc_elem_index],
	     arr_desc->dimensions[vimage->pc_hdim], vimage->pc_hstride,
	     arr_desc->dimensions[vimage->pc_vdim], vimage->pc_vstride,
	     coords, num_vertices, value) );
}   /*  End Function viewimg_fill_polygon  */

/*PUBLIC_FUNCTION*/
void viewimg_get_canvas_attributes (KWorldCanvas canvas, ...)
/*  [SUMMARY] Get the viewable image attributes for a world canvas.
    <canvas> The world canvas object.
    [VARARGS] The list of parameter attribute-key attribute-value-ptr pairs
    must follow. This list must be terminated with the VIEWIMG_ATT_END.
    See [<VIEWIMG_CANVAS_ATTRIBUTES>] for the list of attributes.
    [RETURNS] Nothing.
*/
{
    CanvasHolder holder;
    va_list argp;
    unsigned int att_key;
    static char function_name[] = "viewimg_get_canvas_attributes";

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
    while ( ( att_key = va_arg (argp, unsigned int) ) != VIEWIMG_ATT_END )
    {
	switch (att_key)
	{
	  case VIEWIMG_ATT_AUTO_X:
	    *( va_arg (argp, unsigned int *) ) = holder->auto_x;
	    break;
	  case VIEWIMG_ATT_AUTO_Y:
	    *( va_arg (argp, unsigned int *) ) = holder->auto_y;
	    break;
	  case VIEWIMG_ATT_AUTO_V:
	    *( va_arg (argp, unsigned int *) ) = holder->auto_v;
	    break;
	  case VIEWIMG_ATT_INT_X:
	    *( va_arg (argp, unsigned int *) ) = holder->int_x;
	    break;
	  case VIEWIMG_ATT_INT_Y:
	    *( va_arg (argp, unsigned int *) ) = holder->int_y;
	    break;
	  case VIEWIMG_ATT_MAINTAIN_ASPECT:
	    *( va_arg (argp, unsigned int *) ) =
	    holder->maintain_aspect_ratio;
	    break;
	  case VIEWIMG_ATT_ALLOW_TRUNCATION:
	    *( va_arg (argp, unsigned int *) ) =
	    holder->allow_truncation;
	    break;
	  default:
	    fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (argp);
}   /*  End Function viewimg_get_canvas_attributes  */

/*PUBLIC_FUNCTION*/
void viewimg_set_canvas_attributes (KWorldCanvas canvas, ...)
/*  [SUMMARY] Set the viewable image attributes for a world canvas.
    [PURPOSE] This routine will control the autoscaling options used when
    viewable images are displayed on their associated world canvas.
    <canvas> The world canvas.
    [VARARGS] The list of parameter attribute-key attribute-value pairs
    must follow. This list must be terminated with the VIEWIMG_ATT_END.
    See [<VIEWIMG_CANVAS_ATTRIBUTES>] for the list of attributes.
    [NOTE] The canvas is not refreshed by this operation.
    [RETURNS] Nothing.
*/
{
    CanvasHolder holder;
    va_list argp;
    flag bool;
    unsigned int att_key;
    static char function_name[] = "viewimg_set_canvas_attributes";

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
    while ( ( att_key = va_arg (argp, unsigned int) ) != VIEWIMG_ATT_END )
    {
	switch (att_key)
	{
	  case VIEWIMG_ATT_AUTO_X:
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    holder->auto_x = bool;
	    break;
	  case VIEWIMG_ATT_AUTO_Y:
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    holder->auto_y = bool;
	    break;
	  case VIEWIMG_ATT_AUTO_V:
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    holder->auto_v = bool;
	    break;
	  case VIEWIMG_ATT_INT_X:
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    holder->int_x = bool;
	    break;
	  case VIEWIMG_ATT_INT_Y:
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    holder->int_y = bool;
	    break;
	  case VIEWIMG_ATT_MAINTAIN_ASPECT:
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    holder->maintain_aspect_ratio = bool;
	    break;
	  case VIEWIMG_ATT_ALLOW_TRUNCATION:
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    holder->allow_truncation = bool;
	    break;
	  case VIEWIMG_ATT_ENABLE_PANNING:
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    holder->enable_panning = bool;
	    break;
	  case VIEWIMG_ATT_PAN_CENTRE_X:
	    holder->pan_centre_x = va_arg (argp, unsigned long);
	    break;
	  case VIEWIMG_ATT_PAN_CENTRE_Y:
	    holder->pan_centre_y = va_arg (argp, unsigned long);
	    break;
	  case VIEWIMG_ATT_PAN_MAGNIFICATION:
	    holder->pan_magnification = va_arg (argp, unsigned int);
	    break;
	  default:
	    fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (argp);
}   /*  End Function viewimg_set_canvas_attributes  */

/*PUBLIC_FUNCTION*/
void viewimg_get_attributes (ViewableImage vimage, ...)
/*  [SUMMARY] Get the attributes for a viewable image.
    <vimage> The ViewableImage.
    [VARARGS] The list of parameter attribute-key attribute-value-ptr pairs
    must follow. This list must be terminated with the VIEWIMG_VATT_END. See
    [<VIEWIMG_VIMAGE_ATTRIBUTES>] for the list of attributes.
    [RETURNS] Nothing.
*/
{
    va_list argp;
    flag truecolour;
    unsigned int att_key;
    unsigned int hdim, vdim;
    char *slice;
    array_desc *arr_desc;
    multi_array *multi_desc;
    static char function_name[] = "viewimg_get_attributes";

    VERIFY_VIMAGE (vimage);
    va_start (argp, vimage);
    truecolour = (vimage->tc_arr_desc == NULL) ? FALSE : TRUE;
    arr_desc = truecolour ? vimage->tc_arr_desc : vimage->pc_arr_desc;
    slice = truecolour ? vimage->tc_slice : vimage->pc_slice;
    hdim = truecolour ? vimage->tc_hdim : vimage->pc_hdim;
    vdim = truecolour ? vimage->tc_vdim : vimage->pc_vdim;
    multi_desc = truecolour ? vimage->tc_multi_desc : vimage->pc_multi_desc;
    while ( ( att_key = va_arg (argp, unsigned int) ) != VIEWIMG_VATT_END )
    {
	switch (att_key)
	{
	  case VIEWIMG_VATT_TRUECOLOUR:
	    *( va_arg (argp, flag *) ) = truecolour;
	    break;
	  case VIEWIMG_VATT_ARRAY_DESC:
	    *( va_arg (argp, array_desc **) ) = arr_desc;
	    break;
	  case VIEWIMG_VATT_SLICE:
	    *( va_arg (argp, char **) ) = slice;
	    break;
	  case VIEWIMG_VATT_HDIM:
	    *( va_arg (argp, unsigned int *) ) = hdim;
	    break;
	  case VIEWIMG_VATT_VDIM:
	    *( va_arg (argp, unsigned int *) ) = vdim;
	    break;
	  case VIEWIMG_VATT_PSEUDO_INDEX:
	    if (truecolour)
	    {
		fprintf (stderr, "TrueColour image!\n");
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, unsigned int *) ) = vimage->pc_elem_index;
	    break;
	  case VIEWIMG_VATT_RED_INDEX:
	    if (!truecolour)
	    {
		fprintf (stderr, "PseudoColour image!\n");
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, unsigned int *) ) = vimage->tc_red_index;
	    break;
	  case VIEWIMG_VATT_GREEN_INDEX:
	    if (!truecolour)
	    {
		fprintf (stderr, "PseudoColour image!\n");
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, unsigned int *) ) = vimage->tc_green_index;
	    break;
	  case VIEWIMG_VATT_BLUE_INDEX:
	    if (!truecolour)
	    {
		fprintf (stderr, "PseudoColour image!\n");
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, unsigned int *) ) = vimage->tc_blue_index;
	    break;
	  case VIEWIMG_VATT_MULTI_ARRAY:
	    *( va_arg (argp, multi_array **) ) = multi_desc;
	    break;
	  case VIEWIMG_VATT_DATA_SCALE:
	    if (truecolour)
	    {
		fprintf (stderr,
			 "Data scale not defined for TrueColour image\n");
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, double *) ) = vimage->pc_data_scale;
	    break;
	  case VIEWIMG_VATT_DATA_OFFSET:
	    if (truecolour)
	    {
		fprintf (stderr,
			 "Data offset not defined for TrueColour image\n");
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, double *) ) = vimage->pc_data_offset;
	    break;
	  case VIEWIMG_VATT_VALUE_MIN:
	    if (truecolour)
	    {
		fprintf (stderr,
			 "Override min value not defined for TrueColour image\n");
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, double *) ) = vimage->override.value_min;
	    break;
	  case VIEWIMG_VATT_VALUE_MAX:
	    if (truecolour)
	    {
		fprintf (stderr,
			 "Override max value not defined for TrueColour image\n");
		a_prog_bug (function_name);
	    }
	    *( va_arg (argp, double *) ) = vimage->override.value_max;
	    break;
	  default:
	    fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (argp);
}   /*  End Function viewimg_get_attributes  */

/*PUBLIC_FUNCTION*/
void viewimg_set_attributes (ViewableImage vimage, ...)
/*  [SUMMARY] Set the attributes for a viewable image.
    <vimage> The ViewableImage.
    [VARARGS] The list of parameter attribute-key attribute-value pairs
    must follow. This list must be terminated with the VIEWIMG_VATT_END. See
    [<VIEWIMG_VIMAGE_ATTRIBUTES>] for the list of attributes.
    [RETURNS] Nothing.
*/
{
    va_list argp;
    flag truecolour;
    unsigned int att_key;
    unsigned int hdim, vdim;
    char *slice;
    array_desc *arr_desc;
    multi_array *multi_desc;
    static char function_name[] = "viewimg_set_attributes";

    VERIFY_VIMAGE (vimage);
    va_start (argp, vimage);
    truecolour = (vimage->tc_arr_desc == NULL) ? FALSE : TRUE;
    arr_desc = truecolour ? vimage->tc_arr_desc : vimage->pc_arr_desc;
    slice = truecolour ? vimage->tc_slice : vimage->pc_slice;
    hdim = truecolour ? vimage->tc_hdim : vimage->pc_hdim;
    vdim = truecolour ? vimage->tc_vdim : vimage->pc_vdim;
    multi_desc = truecolour ? vimage->tc_multi_desc : vimage->pc_multi_desc;
    while ( ( att_key = va_arg (argp, unsigned int) ) != VIEWIMG_VATT_END )
    {
	switch (att_key)
	{
	  case VIEWIMG_VATT_DATA_SCALE:
	    if (truecolour)
	    {
		fprintf (stderr,
			 "Data scale not defined for TrueColour image\n");
		a_prog_bug (function_name);
	    }
	    vimage->pc_data_scale = va_arg (argp, double);
	    break;
	  case VIEWIMG_VATT_DATA_OFFSET:
	    if (truecolour)
	    {
		fprintf (stderr,
			 "Data offset not defined for TrueColour image\n");
		a_prog_bug (function_name);
	    }
	    vimage->pc_data_offset = va_arg (argp, double);
	    break;
	  case VIEWIMG_VATT_VALUE_MIN:
	    if (truecolour)
	    {
		fprintf (stderr,
			 "Override min value not defined for TrueColour image\n");
		a_prog_bug (function_name);
	    }
	    vimage->override.value_min = va_arg (argp, double);
	    break;
	  case VIEWIMG_VATT_VALUE_MAX:
	    if (truecolour)
	    {
		fprintf (stderr,
			 "Override max value not defined for TrueColour image\n");
		a_prog_bug (function_name);
	    }
	    vimage->override.value_max = va_arg (argp, double);
	    break;
	  default:
	    fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (argp);
}   /*  End Function viewimg_set_attributes  */

/*PUBLIC_FUNCTION*/
void viewimg_set_array_attributes (ViewableImage *vimages, unsigned int len,
				   ...)
/*  [SUMMARY] Set the attributes for an array of viewable images.
    <vimages> The array of ViewableImage objects.
    <len> The length of the array.
    [VARARGS] The list of parameter attribute-key attribute-value pairs
    must follow. This list must be terminated with the VIEWIMG_VATT_END. See
    [<VIEWIMG_VIMAGE_ATTRIBUTES>] for the list of attributes.
    [RETURNS] Nothing.
*/
{
    va_list argp;
    unsigned int att_key;
    unsigned int count;
    double dval;
    static char function_name[] = "viewimg_set_attributes";

    if (len < 1) return;
    if (vimages == NULL)
    {
	fprintf (stderr, "NULL vimages pointer passed\n");
	a_prog_bug (function_name);
    }
    for (count = 0; count < len; ++count) VERIFY_VIMAGE (vimages[count]);
    va_start (argp, len);
    while ( ( att_key = va_arg (argp, unsigned int) ) != VIEWIMG_VATT_END )
    {
	switch (att_key)
	{
	  case VIEWIMG_VATT_DATA_SCALE:
	    dval = va_arg (argp, double);
	    for (count = 0; count < len; ++count)
	    {
		if (vimages[count]->tc_arr_desc != NULL)
		{
		    fprintf (stderr,
			     "Data scale not defined for TrueColour image\n");
		    a_prog_bug (function_name);
		}
		vimages[count]->pc_data_scale = dval;
	    }
	    break;
	  case VIEWIMG_VATT_DATA_OFFSET:
	    dval = va_arg (argp, double);
	    for (count = 0; count < len; ++count)
	    {
		if (vimages[count]->tc_arr_desc != NULL)
		{
		    fprintf (stderr,
			     "Data offset not defined for TrueColour image\n");
		    a_prog_bug (function_name);
		}
		vimages[count]->pc_data_offset = dval;
	    }
	    break;
	  case VIEWIMG_VATT_VALUE_MIN:
	    dval = va_arg (argp, double);
	    for (count = 0; count < len; ++count)
	    {
		if (vimages[count]->tc_arr_desc != NULL)
		{
		    fprintf (stderr,
			     "Override min value not defined for TrueColour image\n");
		    a_prog_bug (function_name);
		}
		vimages[count]->override.value_min = dval;
	    }
	    break;
	  case VIEWIMG_VATT_VALUE_MAX:
	    dval = va_arg (argp, double);
	    for (count = 0; count < len; ++count)
	    {
		if (vimages[count]->tc_arr_desc != NULL)
		{
		    fprintf (stderr,
			     "Override max value not defined for TrueColour image\n");
		    a_prog_bug (function_name);
		}
		vimages[count]->override.value_max = dval;
	    }
	    break;
	  default:
	    fprintf (stderr, "Illegal attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	}
    }
    va_end (argp);
}   /*  End Function viewimg_set_array_attributes  */

/*EXPERIMENTAL_FUNCTION*/
flag viewimg_partial_refresh (KWorldCanvas canvas, unsigned int num_areas,
			      KPixCanvasRefreshArea *areas)
/*  [SUMMARY] Perform a partial refresh of a canvas.
    [PURPOSE] This routine will perform a partial refresh of a canvas. This
    call is similar to <<kwin_partial_refresh>> except that areas are not
    cleared prior to drawing if they lie within the image boundary.
    <canvas> The world canvas.
    <num_areas> The number of areas in the pixel canvas to refresh.
    <areas> The list of areas to refresh. The values here are updated to ensure
    all points lie within the boundaries of the underlaying pixel canvas.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    CanvasHolder holder;
    KPixCanvas pixcanvas;
    int x_offset, y_offset, x_pixels, y_pixels;
    int min_x_clear, min_y_clear, max_x_clear, max_y_clear;
    long clear_area, boundary_area;
    unsigned int count;
    static char function_name[] = "viewimg_partial_refresh";

    if (canvas == NULL)
    {
	fprintf (stderr, "NULL canvas passed\n");
	a_prog_bug (function_name);
    }
    canvas_get_attributes (canvas,
			   CANVAS_ATT_X_OFFSET, &x_offset,
			   CANVAS_ATT_Y_OFFSET, &y_offset,
			   CANVAS_ATT_X_PIXELS, &x_pixels,
			   CANVAS_ATT_Y_PIXELS, &y_pixels,
			   CANVAS_ATT_END);
    if ( ( holder = get_canvas_holder (canvas, TRUE, function_name) ) == NULL )
    {
	m_abort (function_name, "canvas holder");
    }
    pixcanvas = canvas_get_pixcanvas (canvas);
    if (holder->active_image == NULL)
    {
	return ( kwin_partial_refresh (pixcanvas, num_areas, areas, FALSE) );
    }
    clear_area = 0;
    min_x_clear = x_offset + x_pixels;
    max_x_clear = -1;
    min_y_clear = y_offset + y_pixels;
    max_y_clear = -1;
    for (count = 0; count < num_areas; ++count)
    {
	if (!areas[count].clear) continue;
	/*  Area should be cleared  */
	if ( (areas[count].startx >= x_offset) &&
	     (areas[count].endx < x_offset + x_pixels) &&
	     (areas[count].starty >= y_offset) &&
	     (areas[count].endy < y_offset + y_pixels) )
	{
	    /*  This area is entirely within image: no need to clear  */
	    areas[count].clear = FALSE;
	    continue;
	}
	/*  This area is at least partly outside the image and must be cleared:
	    accumulate clear area and update clear boundary  */
	clear_area += (areas[count].endx - areas[count].startx + 1) *
	    (areas[count].endy - areas[count].starty + 1);
	if (areas[count].startx < min_x_clear) min_x_clear=areas[count].startx;
	if (areas[count].endx > max_x_clear) max_x_clear = areas[count].endx;
	if (areas[count].starty < min_y_clear) min_y_clear=areas[count].starty;
	if (areas[count].endy > max_y_clear) max_y_clear = areas[count].endy;
    }
    if (clear_area < 1)
    {
	/*  No areas require clearing: simple  */
	return ( kwin_partial_refresh (pixcanvas, num_areas, areas, FALSE) );
    }
    /*  Some areas require clearing: check if it would be faster to simply
	clear the requisite area around the image  */
    boundary_area = 0;
    if (min_x_clear < x_offset)
    {
	/*  Left of image requires a clear  */
	boundary_area += (x_offset - min_x_clear) * (max_y_clear - min_y_clear
						     + 1);
    }
    if (max_x_clear >= x_offset + x_pixels)
    {
	/*  Right of image requires a clear  */
	boundary_area += (max_x_clear - x_offset - x_pixels + 1) *
	    (max_y_clear - min_y_clear + 1);
    }
    if (min_y_clear < y_offset)
    {
	/*  Top of image requires a clear  */
	boundary_area += (y_offset - min_y_clear) * x_pixels;
    }
    if (max_y_clear >= y_offset + y_pixels)
    {
	/*  Bottom of image requires a clear  */
	boundary_area += (max_y_clear - y_offset - y_pixels + 1) * x_pixels;
    }
    if (boundary_area > clear_area)
    {
	/*  Faster to clear areas  */
	return ( kwin_partial_refresh (pixcanvas, num_areas, areas, FALSE) );
    }
    /*  Faster to clear boundary: do not clear areas  */
    for (count = 0; count < num_areas; ++count) areas[count].clear = FALSE;
    /*  Clear boundary  */
    if (min_x_clear < x_offset)
    {
	/*  Left of image requires a clear  */
	kwin_clear (pixcanvas, min_x_clear, min_y_clear,
		    x_offset - min_x_clear, max_y_clear - min_y_clear + 1);
    }
    if (max_x_clear >= x_offset + x_pixels)
    {
	/*  Right of image requires a clear  */
	kwin_clear (pixcanvas, x_offset + x_pixels, min_y_clear,
		    max_x_clear - x_offset - x_pixels + 1,
		    max_y_clear - min_y_clear + 1);
    }
    if (min_y_clear < y_offset)
    {
	/*  Top of image requires a clear  */
	kwin_clear (pixcanvas, x_offset, min_y_clear,
		    x_pixels, y_offset - min_y_clear);
    }
    if (max_y_clear >= y_offset + y_pixels)
    {
	/*  Bottom of image requires a clear  */
	kwin_clear (pixcanvas, x_offset, y_offset + y_pixels,
		    x_pixels, max_y_clear - y_offset - y_pixels + 1);
    }
    return ( kwin_partial_refresh (pixcanvas, num_areas, areas, FALSE) );
}   /*  End Function viewimg_partial_refresh  */

/*PUBLIC_FUNCTION*/
KWorldCanvas viewimg_get_worldcanvas (ViewableImage vimage)
/*  [SUMMARY] Get the world canvas for a viewable image.
    <vimage> The ViewableImage object.
    [RETURNS] The KWorldCanvas object.
*/
{
    static char function_name[] = "viewimg_get_worldcanvas";

    VERIFY_VIMAGE (vimage);
    return (vimage->canvas_holder->canvas);
}   /*  End Function viewimg_get_worldcanvas  */


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
    fprintf (stderr, "**WARNING**:  %s  called before:  viewimg_init\n",
		    func_name);
    fprintf (stderr,
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
    canvas_holder->magic_number = HOLDER_MAGIC_NUMBER;
    canvas_holder->canvas = canvas;
    canvas_holder->first_image = NULL;
    canvas_holder->active_image = NULL;
    canvas_holder->position_list = NULL;
    canvas_holder->info = NULL;
    canvas_holder->auto_x = TRUE;
    canvas_holder->auto_y = TRUE;
    canvas_holder->auto_v = TRUE;
    canvas_holder->int_x = TRUE;
    canvas_holder->int_y = TRUE;
    canvas_holder->maintain_aspect_ratio = FALSE;
    canvas_holder->allow_truncation = FALSE;
    canvas_holder->enable_panning = FALSE;
    canvas_holder->pan_centre_x = 0;
    canvas_holder->pan_centre_y = 0;
    canvas_holder->pan_magnification = 4;
    canvas_holder->old_cmap = NULL;
    /*  Insert at beginning of list  */
    canvas_holder->next = first_canvas_holder;
    first_canvas_holder = canvas_holder;
    canvas_register_refresh_func (canvas, worldcanvas_refresh_func,
				  (void *) canvas_holder);
    canvas_register_size_control_func (canvas, worldcanvas_size_control_func,
				       (void *) canvas_holder);
    canvas_register_position_event_func (canvas,
						worldcanvas_position_func,
						(void *) canvas_holder);
    canvas_register_coords_convert_func (canvas, coord_convert_func,
					 canvas_holder);
    return (canvas_holder);
}   /*  End Function alloc_canvas_holder  */

static void worldcanvas_refresh_func (KWorldCanvas canvas,
				      int width, int height,
				      struct win_scale_type *win_scale,
				      Kcolourmap cmap, flag cmap_resize,
				      void **info, PostScriptPage pspage,
				      unsigned int num_areas,
				      KPixCanvasRefreshArea *areas,
				      flag *honoured_areas)
/*  [PURPOSE] This routine registers a refresh event for a world canvas.
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
    ViewableImage vimage;
    unsigned int visual, count;
    double iscale_values[NUM_ISCALE_VALUES];
    IscaleFunc iscale_func;
    static char function_name[] = "__viewimg_worldcanvas_refresh_func";

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
    if (holder->active_image == NULL) return;
    if ( (width < 3) || (height < 3) ) return;
    vimage = holder->active_image;
    if ( (pspage != NULL) && (vimage->tc_arr_desc != NULL) )
    {
	kwin_get_attributes (canvas_get_pixcanvas (holder->canvas),
			     KWIN_ATT_VISUAL, &visual,
			     KWIN_ATT_END);
	if (visual == KWIN_VISUAL_PSEUDOCOLOUR)
	{
	    fprintf (stderr,
		     "%s: drawing TrueColour image directly to PostScriptPage\n",
		     function_name);
	}
	/*  Drawing TrueColour image to PostScript page  */
	if ( !canvas_draw_rgb_image (holder->canvas,
				     vimage->tc_arr_desc, vimage->tc_slice,
				     vimage->tc_hdim, vimage->tc_vdim,
				     vimage->tc_red_index,
				     vimage->tc_green_index,
				     vimage->tc_blue_index,
				     (KPixCanvasImageCache *) NULL) )
	{
	    fprintf (stderr, "Error drawing image onto world canvas\n");
	}
	return;
    }
    /*  Do tests on image  */
    if (vimage->cache == NULL)
    {
	/*  No image cache  */
	recompute_image (holder, width, height, win_scale, cmap, vimage);
	return;
    }
    if (vimage->changed)
    {
	/*  Data has changed  */
	fprintf (stderr, "%s: spurious data change\n", function_name);
	vimage->changed = FALSE;
	vimage->recompute = TRUE;
	recompute_image (holder, width, height, win_scale, cmap, vimage);
	return;
    }
    if ( (vimage->tc_arr_desc == NULL) && (win_scale->iscale_func != NULL) &&
	 !vimage->recompute )
    {
	/*  PseudoColour image and intensity scaling function used: check that
	    the mapping function has not changed. Do this by comparing a set
	    of scaled values with a set previously computed. If there is a
	    difference, the intensity scaling has changed. This is only an
	    approximate algorithm  */
	iscale_func = (IscaleFunc) win_scale->iscale_func;
	/*  Compute test values  */
	for (count = 0; count < NUM_ISCALE_VALUES; ++count)
	{
	    iscale_values[count] = win_scale->z_min +
		(win_scale->z_max - win_scale->z_min) * iscale_factors[count];
	}
	if ( !(*iscale_func) (iscale_values, 1, iscale_values, 1,
			      NUM_ISCALE_VALUES,
			      win_scale->z_min, win_scale->z_max,
			      win_scale->iscale_info) )
	{
	    fprintf (stderr, "%s: error scaling test values\n", function_name);
	    return;
	}
	/*  Now compare  */
	if ( (vimage->iscale_test[0] >= TOOBIG) ||
	     !m_cmp ( (char *) vimage->iscale_test, (char *) iscale_values,
		      sizeof *iscale_values * NUM_ISCALE_VALUES ) )
	{
	    /*  Old values not consistent: copy and force recompute  */
	    m_copy ( (char *) vimage->iscale_test, (char *) iscale_values,
		     sizeof *iscale_values * NUM_ISCALE_VALUES );
	    vimage->recompute = TRUE;
	}
    }
    if (vimage->recompute)
    {
	/*  Image cache is no longer valid for some reason  */
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
    fprintf (stderr, "%s win_scale: %e %e  %e %e\n",
	     function_name,
	     win_scale->left_x, win_scale->right_x,
	     win_scale->bottom_y, win_scale->top_y);
    fprintf (stderr, "%s vimage.win_scale: %e %e  %e %e\n",
	     function_name,
	     vimage->win_scale.left_x, vimage->win_scale.right_x,
	     vimage->win_scale.bottom_y, vimage->win_scale.top_y);
#endif
    /*  More tests  */
    vimage->win_scale.x_offset = win_scale->x_offset;
    vimage->win_scale.y_offset = win_scale->y_offset;
    if (!m_cmp ( (char *) win_scale, (char *) &vimage->win_scale,
		 sizeof *win_scale ) )
    {
	/*  Window scaling information has changed  */
#ifdef DEBUG
	fprintf (stderr, "%s: SPURIOUS SCALE CHANGE\n", function_name);
#endif
	recompute_image (holder, width, height, win_scale, cmap, vimage);
	return;
    }
    /*  No significant changes: use image cache  */
    if (holder->enable_panning && !holder->auto_v)
    {
	draw_subcache (holder, vimage, width, height);
	return;
    }
    if (num_areas < 1)
    {
	if ( kwin_draw_cached_image (vimage->cache,
				     win_scale->x_offset,
				     win_scale->y_offset) )
	{
	    /*  Cache OK  */
	    return;
	}
	/*  Something wrong with cache  */
	recompute_image (holder, width, height, win_scale, cmap, vimage);
	return;
    }
    /*  Only draw parts of the image  */
    if ( kwin_draw_cached_subimages (vimage->cache,
				     win_scale->x_offset, win_scale->y_offset,
				     num_areas, areas) )
    {
	*honoured_areas = TRUE;
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
    <vimage> The viewable image.
*/
{
    Kcolourmap kcmap;
    unsigned int num_pixels, elem_index;
    array_desc *arr_desc;
    packet_desc *pack_desc;
    unsigned long *pixel_values;
    dim_desc *hdim, *vdim;
    /*static char function_name[] = "__viewimg_recompute_image";*/

#ifdef DEBUG
    fprintf (stderr, "%s win_scale: %e %e  %e %e\n",
	     function_name,
	     win_scale->left_x, win_scale->right_x,
	     win_scale->bottom_y, win_scale->top_y);
    fprintf (stderr, "%s old vimage.win_scale: %e %e  %e %e\n",
	     function_name,
	     vimage->win_scale.left_x, vimage->win_scale.right_x,
	     vimage->win_scale.bottom_y, vimage->win_scale.top_y);
#endif
    m_copy ( (char *) &vimage->win_scale, (char *) win_scale,
	     sizeof *win_scale );
    vimage->recompute = TRUE;
    /*  If a PseudoColour array exists, either the image is PseudoColour or the
	canvas is PseudoColour  */
    if (holder->enable_panning && !holder->auto_v)
    {
	/*  Compute a different kind of image cache which is the entire image
	    magnified, irrespective of the canvas size, then draw the image
	    cache
	    */
	if (vimage->pc_arr_desc != NULL)
	{
	    kcmap = canvas_get_cmap (holder->canvas);
	    num_pixels = kcmap_get_pixels (kcmap, &pixel_values);
	    arr_desc = vimage->pc_arr_desc;
	    hdim = arr_desc->dimensions[vimage->pc_hdim];
	    vdim = arr_desc->dimensions[vimage->pc_vdim];
	    pack_desc = arr_desc->packet;
	    elem_index = vimage->pc_elem_index;
	    if ( !kwin_draw_pc_image (canvas_get_pixcanvas (holder->canvas),
				      0, 0,
				      hdim->length * holder->pan_magnification,
				      vdim->length * holder->pan_magnification,
				      vimage->pc_slice +
				      ds_get_element_offset (pack_desc,
							     elem_index),
				      arr_desc->offsets[vimage->pc_hdim],
				      arr_desc->offsets[vimage->pc_vdim],
				      hdim->length, vdim->length,
				      pack_desc->element_types[elem_index],
				      win_scale->conv_type,
				      num_pixels, pixel_values,
				      win_scale->blank_pixel,
				      win_scale->min_sat_pixel,
				      win_scale->max_sat_pixel,
				      win_scale->z_min, win_scale->z_max,
				      win_scale->iscale_func,
				      win_scale->iscale_info,
				      &vimage->cache) )
	    {
		fprintf (stderr, "Error drawing image onto world canvas\n");
		return;
	    }
	}
	else
	{
	    arr_desc = vimage->tc_arr_desc;
	    hdim = arr_desc->dimensions[vimage->tc_hdim];
	    vdim = arr_desc->dimensions[vimage->tc_vdim];
	    pack_desc = arr_desc->packet;
	    if ( !kwin_draw_rgb_image(canvas_get_pixcanvas (holder->canvas),
				      0, 0,
				      hdim->length * holder->pan_magnification,
				      vdim->length * holder->pan_magnification,
				      (CONST unsigned char *)vimage->tc_slice +
				      ds_get_element_offset (pack_desc,
							     vimage->tc_red_index),
				      (CONST unsigned char *)vimage->tc_slice +
				      ds_get_element_offset (pack_desc,
							     vimage->tc_green_index),
				      (CONST unsigned char *)vimage->tc_slice +
				      ds_get_element_offset (pack_desc,
							     vimage->tc_blue_index),
				      arr_desc->offsets[vimage->tc_hdim],
				      arr_desc->offsets[vimage->tc_vdim],
				      hdim->length, vdim->length,
				      &vimage->cache) )
	    {
		fprintf (stderr, "Error drawing image onto world canvas\n");
		return;
	    }
	}
	/*  Now draw the desired part of this huge image cache  */
	draw_subcache (holder, vimage, width, height);
    }
    else
    {
	/*  Compute image over region defined by world canvas  */
	if (vimage->pc_arr_desc != NULL)
	{
	    if ( !canvas_draw_image (holder->canvas,
				     vimage->pc_arr_desc,
				     vimage->pc_slice,
				     vimage->pc_hdim,
				     vimage->pc_vdim,
				     vimage->pc_elem_index,
				     &vimage->cache) )
	    {
		fprintf (stderr, "Error drawing image onto world canvas\n");
		return;
	    }
	}
	else
	{
	    if ( !canvas_draw_rgb_image (holder->canvas,
					 vimage->tc_arr_desc, vimage->tc_slice,
					 vimage->tc_hdim, vimage->tc_vdim,
					 vimage->tc_red_index,
					 vimage->tc_green_index,
					 vimage->tc_blue_index,
					 &vimage->cache) )
	    {
		fprintf (stderr, "Error drawing image onto world canvas\n");
		return;
	    }
	}
    }
    vimage->recompute = FALSE;
    /*  Also need to clear the changed flag because the size control function
	will only clear when auto intensity scaling is on, and it will not be
	cleared on the first canvas refresh (i.e. when no cache is valid).  */
    vimage->changed = FALSE;
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
    [RETURNS] Nothing.
*/
{
    CanvasHolder holder;
    ViewableImage vimage;
    Kcolourmap cmap;
    iarray pseudo_arr;
    long hstart, hend, vstart, vend;  /*  Inclusive co-ordinates  */
    unsigned int canvas_visual, canvas_depth;
    flag scale_changed = FALSE;
    uaddr *hoffsets, *voffsets;
    char *cmap_packet;
    array_desc *arr_desc;
    packet_desc *pack_desc;
    dim_desc *hdim;
    dim_desc *vdim;
    packet_desc *cmap_pack_desc;
    static char function_name[] = "__viewimg_worldcanvas_size_control_func";

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
    win_scale->x_pixels = width;
    win_scale->y_pixels = height;
    if (holder->active_image == NULL)
    {
	win_scale->x_offset = 0;
	win_scale->y_offset = 0;
	return;
    }
    if ( (width < 3) || (height < 3) ) return;
    *boundary_clear = TRUE;
    vimage = holder->active_image;
    /*  Work on changing window scaling if need be  */
    if (vimage->tc_arr_desc == NULL)
    {
	arr_desc = vimage->pc_arr_desc;
	hdim = arr_desc->dimensions[vimage->pc_hdim];
	vdim = arr_desc->dimensions[vimage->pc_vdim];
	hoffsets = arr_desc->offsets[vimage->pc_hdim];
	voffsets = arr_desc->offsets[vimage->pc_vdim];
    }
    else
    {
	arr_desc = vimage->tc_arr_desc;
	hdim = arr_desc->dimensions[vimage->tc_hdim];
	vdim = arr_desc->dimensions[vimage->tc_vdim];
	hoffsets = arr_desc->offsets[vimage->tc_hdim];
	voffsets = arr_desc->offsets[vimage->tc_vdim];
    }
    pack_desc = arr_desc->packet;
#ifdef DEBUG
    fprintf (stderr, "%s: BEFORE: %e %e  %e %e\n",
	     function_name,
	     win_scale->left_x, win_scale->right_x,
	     win_scale->bottom_y, win_scale->top_y);
#endif
    /*  Determine canvas horizontal world co-ordinates  */
    if (holder->enable_panning)
    {
	hstart = holder->pan_centre_x - width / holder->pan_magnification / 2;
	if (hstart < 0) hstart = 0;
	hend = hstart + width / holder->pan_magnification - 1;
	if (hend >= hdim->length)
	{
	    hend = hdim->length - 1;
	    hstart = hend - width / holder->pan_magnification + 1;
	}
	if (hstart < 0) hstart = 0;
	win_scale->x_pixels = (hend - hstart + 1) * holder->pan_magnification;
    }
    else if (holder->auto_x)
    {
	hstart = 0;
	hend = hdim->length - 1;
    }
    else
    {
	hstart = ds_get_coord_num (hdim, win_scale->left_x,
				   SEARCH_BIAS_CLOSEST);
	hend = ds_get_coord_num (hdim, win_scale->right_x,SEARCH_BIAS_CLOSEST);
	if (hstart > hend)
	{
	    fprintf (stderr, "hstart: %ld is larger than hend: %ld\n",
			    hstart, hend);
	    a_prog_bug (function_name);
	}
	if (hend == hstart)
	{
	    /*  Same dimension co-ordinate: separate  */
	    --hstart;
	    ++hend;
	}
    }
    /*  Determine canvas vertical world co-ordinates  */
    if (holder->enable_panning)
    {
	vstart = holder->pan_centre_y - height / holder->pan_magnification / 2;
	if (vstart < 0) vstart = 0;
	vend = vstart + height / holder->pan_magnification - 1;
	if (vend >= vdim->length)
	{
	    vend = vdim->length - 1;
	    vstart = vend - height / holder->pan_magnification + 1;
	}
	if (vstart < 0) vstart = 0;
	win_scale->y_pixels = (vend - vstart + 1) * holder->pan_magnification;
    }
    else if (holder->auto_y)
    {
	vstart = 0;
	vend = vdim->length - 1;
    }
    else
    {
	vstart = ds_get_coord_num (vdim, win_scale->bottom_y,
				   SEARCH_BIAS_CLOSEST);
	vend = ds_get_coord_num (vdim, win_scale->top_y, SEARCH_BIAS_CLOSEST);
	if (vstart > vend)
	{
	    fprintf (stderr, "vstart: %ld is larger than vend: %ld\n",
		     vstart, vend);
	    a_prog_bug (function_name);
	}
	if (vend == vstart)
	{
	    /*  Same dimension co-ordinate: separate  */
	    --vstart;
	    ++vend;
	}
    }
    /*  Determine number of pixels required  */
    if (!holder->enable_panning)
	determine_size (holder, width, height, win_scale, &hstart, &hend,
			&vstart, &vend);
    /*  Centre image  */
    win_scale->x_offset = (width - win_scale->x_pixels) / 2;
    win_scale->y_offset = (height - win_scale->y_pixels) / 2;
    win_scale->left_x = ds_get_coordinate (hdim, hstart);
    win_scale->right_x = ds_get_coordinate (hdim, hend);
    win_scale->bottom_y = ds_get_coordinate (vdim, vstart);
    win_scale->top_y = ds_get_coordinate (vdim, vend);
#ifdef MACHINE_i386
    /*  Something wrong the following 4 floating point tests under i386_Linux,
	gcc 2.7.2. Problem exists for both i486 and i586, so it looks like a
	compiler bug. The following dummy call fixes things!
	*/
    ds_element_is_atomic (K_FLOAT);
#endif
    /*  Now know the number of horizontal and vertical pixels required  */
    /*  Check if window scaling changed. Since the intensity scale may change
	with the horizontal and vertical scale, this needs to be done prior to
	any refresh functions being called  */
    /*  First check for world co-ordinate change  */
    if (win_scale->left_x != vimage->win_scale.left_x) scale_changed = TRUE;
    if (win_scale->right_x != vimage->win_scale.right_x) scale_changed = TRUE;
    if (win_scale->bottom_y != vimage->win_scale.bottom_y) scale_changed =TRUE;
    if (win_scale->top_y != vimage->win_scale.top_y) scale_changed = TRUE;
    if (scale_changed && holder->enable_panning && !holder->auto_v)
    {
	/*  Fake it so that the image is not recomputed when panning and auto
	    intensity scale is not enabled. The entire image should already be
	    computed (i.e. moving around in x,y does not require recomputation
	    unless auto intensity scaling is enabled)  */
	scale_changed = FALSE;
	vimage->win_scale.left_x = win_scale->left_x;
	vimage->win_scale.right_x = win_scale->right_x;
	vimage->win_scale.bottom_y = win_scale->bottom_y;
	vimage->win_scale.top_y = win_scale->top_y;
	vimage->win_scale.x_pixels = win_scale->x_pixels;
	vimage->win_scale.y_pixels = win_scale->y_pixels;
    }
    if (scale_changed)
    {
	/*  Precomputed intensity range no longer valid  */
	vimage->value_min = TOOBIG;
	vimage->value_max = TOOBIG;
	vimage->recompute = TRUE;
    }
    if ( holder->auto_v && (vimage->tc_arr_desc == NULL) )
    {
	/*  Automatic intensity scaling with PseudoColour image  */
	if ( (vimage->value_min >= TOOBIG) ||
	     (vimage->value_max >= TOOBIG) )
	{
	    /*  Must compute minimum and maximum values  */
	    vimage->value_min = TOOBIG;
	    vimage->value_max = -TOOBIG;
	    if ( !ds_find_2D_extremes (vimage->pc_slice,
				       vend - vstart + 1, voffsets + vstart,
				       hend - hstart + 1, hoffsets + hstart,
				       pack_desc->element_types[vimage->pc_elem_index],
				       win_scale->conv_type,
				       &vimage->value_min,&vimage->value_max) )
	    {
		fprintf (stderr, "Error getting data range\n");
		a_prog_bug (function_name);
	    }
	    vimage->changed = FALSE;
	    /*Image intensity range has changed: cached image no longer valid*/
	    vimage->recompute = TRUE;
	    /*  Ensure ranges are sensible  */
	    if (vimage->value_min >= vimage->value_max)
	    {
		if (vimage->value_min < 0.0)
		{
		    vimage->value_min *= 2.0;
		    vimage->value_max = 0.0;
		}
		else if (vimage->value_min > 0.0)
		{
		    vimage->value_min = 0.0;
		    vimage->value_max *= 2.0;
		}
		else
		{
		    vimage->value_min = -1.0;
		    vimage->value_max = 1.0;
		}
	    }
	}
	/*  World canvas intensity scale has changed  */
	win_scale->z_min = vimage->value_min;
	win_scale->z_max = vimage->value_max;
    }
    if ( !holder->auto_v && vimage->changed && (vimage->tc_arr_desc == NULL) )
    {
	/*  No automatic intensity scaling, data changed and PseudoColour
	    image  */
	vimage->changed = FALSE;
	vimage->recompute = TRUE;
    }
    if ( !holder->auto_v && (vimage->override.value_min < TOOBIG) &&
	 (vimage->tc_arr_desc == NULL) )
    {
	/*  No automatic intensity scaling, override min value provided and
	    PseudoColour image  */
	win_scale->z_min = vimage->override.value_min;
    }
    if ( !holder->auto_v && (vimage->override.value_max < TOOBIG) &&
	 (vimage->tc_arr_desc == NULL) )
    {
	/*  No automatic intensity scaling, override max value provided and
	    PseudoColour image  */
	win_scale->z_max = vimage->override.value_max;
    }
    if ( vimage->changed && (vimage->tc_arr_desc != NULL) )
    {
	/*  Data changed and TrueColour image  */
	if (vimage->tc_arr_desc->num_levels > 0)
	{
	    fprintf (stderr, "%s: Tiling not supported.\n", function_name);
	    return;
	}
	kwin_get_attributes (canvas_get_pixcanvas (canvas),
			     KWIN_ATT_VISUAL, &canvas_visual,
			     KWIN_ATT_DEPTH, &canvas_depth,
			     KWIN_ATT_END);
	if ( (canvas_visual == KWIN_VISUAL_PSEUDOCOLOUR) &&
	     (vimage->sequence != NULL) )
	{
	    fprintf (stderr, "WARNING: RGB movies on PseudoColour canvas");
	    fprintf (stderr, " might fail and is slow!\n");
	}
	if (canvas_visual != KWIN_VISUAL_PSEUDOCOLOUR)
	{
	    vimage->changed = FALSE;
	    vimage->recompute = TRUE;
	    return;
	}
	/*  If TrueColour image and PseudoColour canvas, recompress image  */
	vimage->recompute = TRUE;
	/*  Have RGB component arrays: create PseudoColour array  */
	ds_dealloc_multi (vimage->pc_multi_desc);
	if ( ( pseudo_arr = iarray_create_2D (vdim->length, hdim->length,
					      K_UBYTE) ) == NULL )
	{
	    m_abort (function_name, "PseudoColour image");
	}
	iarray_set_world_coords (pseudo_arr, 1,
				 hdim->first_coord, hdim->last_coord);
	iarray_set_world_coords (pseudo_arr, 0,
				 vdim->first_coord, vdim->last_coord);
	vimage->pc_multi_desc = pseudo_arr->multi_desc;
	vimage->pc_arr_desc = pseudo_arr->arr_desc;
	vimage->pc_slice = pseudo_arr->data;
	vimage->pc_hdim = 1;
	vimage->pc_vdim = 0;
	vimage->pc_elem_index = 0;
	vimage->pc_hstride = 1;
	vimage->pc_vstride = hdim->length;
	++pseudo_arr->multi_desc->attachments;
	iarray_dealloc (pseudo_arr);
	if ( !imc_24to8 (vdim->length * hdim->length,
			 (unsigned char *) vimage->tc_slice +
			 ds_get_element_offset (pack_desc,
						vimage->tc_red_index),
			 (unsigned char *) vimage->tc_slice +
			 ds_get_element_offset (pack_desc,
						vimage->tc_green_index),
			 (unsigned char *) vimage->tc_slice +
			 ds_get_element_offset (pack_desc,
						vimage->tc_blue_index),
			 ds_get_packet_size (pack_desc),
			 (unsigned char *) vimage->pc_slice, 1,
			 DEFAULT_NUMBER_OF_COLOURS,
			 0, &cmap_pack_desc, &cmap_packet) )
	{
	    fprintf (stderr, "Error compressing image\n");
	    return;
	}
	cmap = canvas_get_cmap (canvas);
	if ( !kcmap_copy_from_struct (cmap, cmap_pack_desc, cmap_packet) )
	{
	    m_abort (function_name, "Compressed image");
	}
	win_scale->z_min = 0.0;
	win_scale->z_max = (kcmap_get_pixels (cmap, (unsigned long **) NULL)
			    -1);
	ds_dealloc_packet (cmap_pack_desc, cmap_packet);
	vimage->changed = FALSE;
	/*  Need to add a mechanism to update the colourmap when running a
	    movie where each frame is individually compressed. Right now the
	    colourmap for the first image in the sequence made active is used,
	    which is clearly wrong  */
    }
}   /*  End Function worldcanvas_size_control_func  */

static flag determine_size (CanvasHolder holder, int width, int height,
			    struct win_scale_type *win_scale,
			    long *hstart, long *hend, long *vstart, long *vend)
/*  [SUMMARY] Determine size of image to be drawn.
    <holder> The canvas holder.
    <width> The width of the canvas in pixels.
    <height> The height of the canvas in pixels.
    <win_scale> The window scaling information. This is modified.
    <hstart> The horizontal start index. This may be modified.
    <hend> The horizontal end index. This may be modified.
    <vstart> The vertical start index. This may be modified.
    <vend> The vertical end index. This may be modified.
    [RETURNS] TRUE if a size could be determined, else FALSE indicating the
    sizing policy is not flexible enough.
*/
{
    long hlength, vlength;
    int factor;

    hlength = *hend - *hstart + 1;
    vlength = *vend - *vstart + 1;
    /*  First test if 1:1 zooming is possible  */
    if ( (width == hlength) && (height == vlength) )
    {
	win_scale->x_pixels = width;
	win_scale->y_pixels = height;
	return (TRUE);
    }
    /*  Determine number of pixels required  */
    if (holder->allow_truncation)
    {
	/*  Permit a small number of data values at the end of each dimension
	    to be ignored in order to allow images whose dimensions are
	    mutually prime to be shrunk. The aspect ratio may need to be
	    preserved
	    */
	trunc_zoom (holder->maintain_aspect_ratio,
		    hstart, hend, &win_scale->x_pixels, holder->int_x,
		    vstart, vend, &win_scale->y_pixels, holder->int_y);
	return (TRUE);
    }
    if (holder->maintain_aspect_ratio)
    {
	/*  Must display all specified image values and also must maintain
	    aspect ratio: think harder
	    */
	aspect_zoom (hlength, &win_scale->x_pixels, holder->int_x,
		     vlength, &win_scale->y_pixels, holder->int_y);
	return (TRUE);
    }
    /*  It is not necessary to maintain the aspect ratio: it is permissible
	to shrink each dimension independently
	*/
    /*  Determine number of horizontal pixels required  */
    if (holder->int_x)
    {
	if (hlength <= width)
	{
	    /*  Zoom in or 1:1  */
	    win_scale->x_pixels = (width / hlength) * hlength;
	}
	else
	{
	    /*  Zoom out  */
	    factor = hlength / width;
	    while (hlength % factor != 0) ++factor;
	    win_scale->x_pixels = hlength / factor;
	}
    }
    else
    {
	win_scale->x_pixels = width;
    }
    /*  Determine number of vertical pixels required  */
    if (holder->int_y)
    {
	if (vlength <= height)
	{
	    /*  Zoom in  */
	    win_scale->y_pixels = (height / vlength) * vlength;
	}
	else
	{
	    /*  Zoom out or 1:1  */
	    factor = vlength / height;
	    while (vlength % factor != 0) ++factor;
	    win_scale->y_pixels = vlength / factor;
	}
	win_scale->y_offset = (height - win_scale->y_pixels) / 2;
    }
    else
    {
	win_scale->y_pixels = height;
    }
    return (TRUE);
}   /*  End Function determine_size  */

static void aspect_zoom (long hlength, int *hpixels, flag int_x,
			 long vlength, int *vpixels, flag int_y)
/*  [PURPOSE] This routine will compute the number of horizontal and vertical
    pixels to use whilst maintaining the image data aspect ratio.
    <hlength> The number of horizontal data values.
    <hpixels> The number of horizontal pixels available. This should be
    modified to reflect the number of pixels required.
    <int_x> If TRUE, the horizontal axis will have integral zooming.
    <vlength> The number of vertical data values.
    <vpixels> The number of vertical pixels available. This should be modified
    to reflect the number of pixels required.
    <int_y> If TRUE, the vertical axis will have integral zooming.
    [RETURNS] Nothing.
*/
{
    flag int_factor;
    float hfactor, vfactor, factor;
    float tiny_offset = 1e-3;
    /*static char function_name[] = "aspect_zoom";*/

#ifdef DUMMY
    if (getuid () == 465)
	fprintf (stderr,
		 "h_values: %ld  h_pixels: %d  v_values: %ld v_pixels: %d\n",
		 hlength, *hpixels, vlength, *vpixels);
#endif
    /*  NOTE:  a 1:1 zoom ratio is termed a zoom in of factor 1  */
    /*  Zoom in: replicate image data.    Zoom out: shrink image data  */
    if ( (*hpixels >= hlength) && (*vpixels >= vlength) )
    {
	/*  Zoom in (expand) both axes  */
	hfactor = (float) *hpixels / (float) hlength;
	if (int_x) hfactor = floor (hfactor);
	vfactor = (float) *vpixels / (float) vlength;
	if (int_y) vfactor = floor (vfactor);
	/*  Use smallest zoom in factor  */
	factor = (hfactor > vfactor) ? vfactor : hfactor;
	*hpixels = factor * hlength;
	*vpixels = factor * vlength;
	return;
    }
    /*  At least one axis must be zoomed out (shrunk)  */
    int_factor = FALSE;
    if (*hpixels < hlength)
    {
	/*  Horizontal axis must be zoomed out  */
	hfactor = (float) hlength / (float) *hpixels;
	if (int_x) int_factor = TRUE;
    }
    else
    {
	/*  Zoom out 1:1  */
	hfactor = 1.0;
    }
    if (*vpixels < vlength)
    {
	/*  Vertical axis must be zoomed out  */
	vfactor = (float) vlength / (float) *vpixels;
	if (int_y) int_factor = TRUE;
    }
    else
    {
	/*  Zoom out 1:1  */
	vfactor = 1.0;
    }
    /*  Start with largest zoom out factor  */
    factor = (hfactor > vfactor) ? hfactor : vfactor;
    if (!int_factor)
    {
	/*  Do not need integral factor: easy  */
	*hpixels = (float) hlength / factor + tiny_offset;
	*vpixels = (float) vlength / factor + tiny_offset;
	return;
    }
    /*  Must have an integral factor: search for one  */
    while ( (hlength % (int) factor != 0) || (vlength % (int) factor != 0) )
    {
	if (++factor > hlength)
	{
	    /*  Too much: fail  */
	    fprintf (stderr,
			    "Cannot maintain aspect ratio without a bigger window\n");
	    return;
	}
    }
    *hpixels = (float) hlength / factor + tiny_offset;
    *vpixels = (float) vlength / factor + tiny_offset;
}   /*  End Function aspect_zoom  */

static void trunc_zoom (flag maintain_aspect_ratio,
			long *hstart, long *hend, int *hpixels, flag int_x,
			long *vstart, long *vend, int *vpixels, flag int_y)
/*  [PURPOSE] This routine will compute the number of horizontal and vertical
    pixels to use, permitting the input image to be truncated.
    <maintain_aspect_ratio> If TRUE, the aspect ratio must be preserved.
    <hstart> The starting horizontal image value co-ordinate. This may be
    modified inwards.
    <hend> The end horizontal image value co-ordinate. This may be modified
    inwards.
    <hpixels> The number of horizontal pixels available. This may be modified
    inwards.
    <int_x> If TRUE, the horizontal axis must have integral zooming.
    <vstart> The starting vertical image value co-ordinate. This may be
    modified inwards.
    <vend> The end vertical image value co-ordinate. This may be modified
    inwards.
    <vpixels> The number of vertical pixels available. This may be modified
    inwards.
    <int_y> If TRUE, the vertical axis must have integral zooming.
    [RETURNS] Nothing.
*/
{
    flag int_factor;
    long hlength, vlength;
    float hfactor, vfactor, factor;
    float tiny_offset = 1e-3;
    /*static char function_name[] = "trunc_zoom";*/

    hlength = *hend - *hstart + 1;
    vlength = *vend - *vstart + 1;
    if (maintain_aspect_ratio)
    {
	if ( (*hpixels >= hlength) && (*vpixels >= vlength) )
	{
	    /*  Zoom in (expand) both axes. Don't truncate in this case since
		data would be lost. Since the canvas is big enough to see the
		whole image, it would be inappropriate to loose data
	     */
	    hfactor = (float) *hpixels / (float) hlength;
	    if (int_x) hfactor = floor (hfactor);
	    vfactor = (float) *vpixels / (float) vlength;
	    if (int_y) vfactor = floor (vfactor);
	    /*  Use smallest zoom in factor  */
	    factor = (hfactor > vfactor) ? vfactor : hfactor;
	    *hpixels = (factor * (float) hlength + tiny_offset);
	    *vpixels = (factor * (float) vlength + tiny_offset);
	    return;
	}
	/*  At least one axis must be zoomed out (shrunk)  */
	int_factor = FALSE;
	if (*hpixels < hlength)
	{
	    /*  Horizontal axis must be zoomed out  */
	    hfactor = (float) hlength / (float) *hpixels;
	    if (int_x) int_factor = TRUE;
	}
	else
	{
	    /*  Zoom out 1:1  */
	    hfactor = 1.0;
	}
	if (*vpixels < vlength)
	{
	    /*  Vertical axis must be zoomed out  */
	    vfactor = (float) vlength / (float) *vpixels;
	    if (int_y) int_factor = TRUE;
	}
	else
	{
	    /*  Zoom out 1:1  */
	    vfactor = 1.0;
	}
	/*  Start with largest zoom out factor  */
	factor = (hfactor > vfactor) ? hfactor : vfactor;
	if (!int_factor)
	{
	    /*  Do not need integral zoom factor: easy, no truncation needed */
	    *hpixels = (float) hlength / factor + tiny_offset;
	    *vpixels = (float) vlength / factor + tiny_offset;
	    return;
	}
	/*  It seems we do need to truncate, since integral zoom is required */
	factor = floor (factor);
	while ( (hlength / (long) factor > *hpixels) ||
		(vlength / (long) factor > *vpixels) ) ++factor;
	/*  Modify input image co-ordinates  */
	*hend = *hstart + (hlength / (long) factor) * (long) factor - 1;
	*vend = *vstart + (vlength / (long) factor) * (long) factor - 1;
	/*  Modify output pixels  */
	*hpixels = ( (float) hlength / factor + tiny_offset );
	*vpixels = ( (float) vlength / factor + tiny_offset );
	return;
    }
    /*  Scale for each axis is independant  */
    if (int_x)
    {
	/*  Must use integral zoom factor for horizontal dimension  */
	if (hlength > *hpixels)
	{
	    /*  Must shrink horizontally  */
	    factor = floor ( (float) hlength / (float) *hpixels );
	    while (hlength / (int) factor > *hpixels) ++factor;
	    /*  Modify input image co-ordinates  */
	    *hend = *hstart + (hlength / (int) factor) * (int) factor - 1;
	    /*  Modify output pixels  */
	    *hpixels = ( (float) hlength / factor + tiny_offset );
	}
	else
	{
	    /*  Possible horizontal expansion  */
	    *hpixels = (*hpixels / hlength) * hlength;
	}
    }
    if (int_y)
    {
	if (vlength > *vpixels)
	{
	    /*  Must shrink vertically  */
	    factor = floor ( (float) vlength / (float) *vpixels );
	    while (vlength / (int) factor > *vpixels) ++factor;
	    /*  Modify input image co-ordinates  */
	    *vend = *vstart + (vlength / (int) factor) * (int) factor - 1;
	    /*  Modify output pixels  */
	    *vpixels = ( (float) vlength / factor + tiny_offset );
	}
	else
	{
	    /*  Possible horizontal expansion  */
	    *vpixels = (*vpixels / vlength) * vlength;
	}
    }
}   /*  End Function trunc_zoom  */

static void draw_subcache (CanvasHolder holder, ViewableImage vimage,
			   int width, int height)
/*  [SUMMARY] Draw an image subcache.
    <holder> The canvas holder object.
    <vimage> The ViewableImage object.
    [RETURNS] Nothing.
*/
{
    KPixCanvasRefreshArea area;
    int xstart, ystart;
    dim_desc *hdim, *vdim;

    /*  Compute part of image cache to draw  */
    xstart = holder->pan_centre_x * holder->pan_magnification - width / 2;
    if (vimage->pc_arr_desc != NULL)
    {
	hdim = vimage->pc_arr_desc->dimensions[vimage->pc_hdim];
	vdim = vimage->pc_arr_desc->dimensions[vimage->pc_vdim];
    }
    else
    {
	hdim = vimage->tc_arr_desc->dimensions[vimage->tc_hdim];
	vdim = vimage->tc_arr_desc->dimensions[vimage->tc_vdim];
    }
    ystart = (vdim->length - holder->pan_centre_y - 1) *
	holder->pan_magnification;
    ystart = ystart - height / 2;
    area.startx = 0;
    area.endx = width - 1;
    area.starty = 0;
    area.endy = height - 1;
    if ( (xstart < 0) ||
	 (holder->pan_centre_y - height / holder->pan_magnification / 2 < 0) ||
	 (holder->pan_centre_x + width / holder->pan_magnification / 2 >
	  hdim->length) ||
	 (holder->pan_centre_y + height / holder->pan_magnification / 2 >=
	  vdim->length) )
    {
	kwin_clear (canvas_get_pixcanvas (holder->canvas), 0, 0, -1, -1);
    }
    kwin_draw_cached_subimages (vimage->cache, -xstart, -ystart, 1, &area);
}   /*  End Function draw_subcache  */

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
    unsigned int hdim_index, vdim_index;
    unsigned long x_coord, y_coord;
    double dx, dy;
    struct position_struct data;
    unsigned char rgb_value[3];
    double value[2];
    array_desc *arr_desc;
    packet_desc *pack_desc;
    dim_desc *hdim;
    dim_desc *vdim;
    static char function_name[] = "worldcanvas_position_func";

    if ( (holder = (CanvasHolder) *f_info) == NULL )
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
    if (holder->position_list == NULL) return (FALSE);
    if (holder->active_image == NULL) return (FALSE);
    vimage = holder->active_image;
    if (vimage->tc_arr_desc == NULL)
    {
	arr_desc = vimage->pc_arr_desc;
	hdim_index = vimage->pc_hdim;
	vdim_index = vimage->pc_vdim;
	data.value = (void *) value;
	data.type = K_DCOMPLEX;
    }
    else
    {
	arr_desc = vimage->tc_arr_desc;
	hdim_index = vimage->tc_hdim;
	vdim_index = vimage->tc_vdim;
	data.value = (void *) rgb_value;
	data.type = K_UB_RGB;
    }
    pack_desc = arr_desc->packet;
    hdim = arr_desc->dimensions[hdim_index];
    vdim = arr_desc->dimensions[vdim_index];
    if (hdim->first_coord < hdim->last_coord)
    {
	x_coord = ds_get_coord_num (hdim, x_lin, SEARCH_BIAS_LOWER);
    }
    else x_coord = ds_get_coord_num (hdim, x_lin, SEARCH_BIAS_UPPER);
    if (vdim->first_coord < vdim->last_coord)
    {
	y_coord = ds_get_coord_num (vdim, y_lin, SEARCH_BIAS_LOWER);
    }
    else y_coord = ds_get_coord_num (vdim, y_lin, SEARCH_BIAS_UPPER);
    dx = ds_get_coordinate (hdim, x_coord);
    dy = ds_get_coordinate (vdim, y_coord);
    if (vimage->tc_arr_desc == NULL)
    {
	if ( !ds_get_element (vimage->pc_slice +
			      ds_get_element_offset (pack_desc,
						     vimage->pc_elem_index) +
			      arr_desc->offsets[vdim_index][y_coord] +
			      arr_desc->offsets[hdim_index][x_coord],
			      pack_desc->element_types[vimage->pc_elem_index],
			      value, (flag *) NULL) )
	{
	    fprintf (stderr, "Error converting data\n");
	    return (FALSE);
	}
    }
    else
    {
	if ( !ds_get_element (vimage->tc_slice +
			      ds_get_element_offset (pack_desc,
						     vimage->tc_red_index) +
			      arr_desc->offsets[vdim_index][y_coord] +
			      arr_desc->offsets[hdim_index][x_coord],
			      pack_desc->element_types[vimage->tc_red_index],
			      value, (flag *) NULL) )
	{
	    fprintf (stderr, "Error converting data\n");
	    return (FALSE);
	}
	rgb_value[0] = 0xff & (int) value[0];
	if ( !ds_get_element (vimage->tc_slice +
			      ds_get_element_offset (pack_desc,
						     vimage->tc_green_index) +
			      arr_desc->offsets[vdim_index][y_coord] +
			      arr_desc->offsets[hdim_index][x_coord],
			      pack_desc->element_types[vimage->tc_green_index],
			      value, (flag *) NULL) )
	{
	    fprintf (stderr, "Error converting data\n");
	    return (FALSE);
	}
	rgb_value[1] = 0xff & (int) value[0];
	if ( !ds_get_element (vimage->tc_slice +
			      ds_get_element_offset (pack_desc,
						     vimage->tc_blue_index) +
			      arr_desc->offsets[vdim_index][y_coord] +
			      arr_desc->offsets[hdim_index][x_coord],
			      pack_desc->element_types[vimage->tc_blue_index],
			      value, (flag *) NULL) )
	{
	    fprintf (stderr, "Error converting data\n");
	    return (FALSE);
	}
	rgb_value[2] = 0xff & (int) value[0];
    }
    x = dx;
    y = dy;
    canvas_coords_transform (canvas, 1, &x, FALSE, &y, FALSE);
    /*  Call event consumer functions  */
    data.x = x;
    data.y = y;
    data.event_code = event_code;
    data.e_info = e_info;
    data.x_lin = dx;
    data.y_lin = dy;
    return ( c_call_callbacks (holder->position_list, &data) );
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
    vimage = holder->active_image;
    VERIFY_VIMAGE (vimage);
    data = (struct position_struct *) call_data;
    func = ( flag (*) () ) client2_data;
    return ( (*func) (vimage, data->x, data->y, data->value,
		      data->event_code, data->e_info, client1_data,
		      data->x_lin, data->y_lin, data->type) );
}   /*  End Function position_event_func  */

static flag coord_convert_func (KWorldCanvas canvas,
				unsigned int num_coords,
				CONST double *xin, CONST double *yin,
				double *xout, double *yout,
				flag to_world, void **info)
/*  [SUMMARY] Co-ordinate conversion callback.
    [PURPOSE] This routine will convert between world co-ordinates and pixel
    co-ordinates for a world canvas.
    <canvas> The canvas.
    <num_coords> The number of co-ordinates to transform.
    <xin> The array of input horizontal co-ordinates.
    <yin> The array of input vertical co-ordinates.
    <xout> The array of output horizontal co-ordinates are written here.
    <yout> The array of output vertical co-ordinates are written here.
    <to_world> If TRUE, then a pixel to world co-ordinate transform is
    required, else a world to pixel co-ordinate transform is required.
    <info> A pointer to the arbitrary canvas information pointer.
    [RETURNS] TRUE if the conversion was completed, else FALSE indicating
    that the default conversions should be used.
*/
{
    CanvasHolder holder;
    ViewableImage vimage;
    int count;
    int coord_num0, coord_num1;
    double coord0, coord1;
    double x_offset, y_offset, x_pixels, y_pixels;
    double x_pix, y_pix, xdelt, ydelt;
    struct win_scale_type win_scale;
    array_desc *arr_desc;
    packet_desc *pack_desc;
    dim_desc *hdim;
    dim_desc *vdim;
    static char function_name[] = "__viewimg_coord_convert_func";

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
    if ( (vimage = holder->active_image) == NULL) return (FALSE);
    if (vimage->tc_arr_desc == NULL)
    {
	arr_desc = vimage->pc_arr_desc;
	hdim = arr_desc->dimensions[vimage->pc_hdim];
	vdim = arr_desc->dimensions[vimage->pc_vdim];
    }
    else
    {
	arr_desc = vimage->tc_arr_desc;
	hdim = arr_desc->dimensions[vimage->tc_hdim];
	vdim = arr_desc->dimensions[vimage->tc_vdim];
    }
    pack_desc = arr_desc->packet;
    /*  These are the correspondences we have to insure:
	wx = left_x             <-->    px = x_offset
	wx = right_x + x_delta  <-->    px = x_offset + x_pixels
	wy = bottom_y           <-->    py = y_offset + y_pixels - 1
	wy = top_y + y_delta    <-->    py = y_offset - 1

	where x_delta and y_delta are the world co-ordinate increments.
	*/
    canvas_get_attributes (canvas,
			   CANVAS_ATT_X_OFFSET, &win_scale.x_offset,
			   CANVAS_ATT_X_PIXELS, &win_scale.x_pixels,
			   CANVAS_ATT_Y_OFFSET, &win_scale.y_offset,
			   CANVAS_ATT_Y_PIXELS, &win_scale.y_pixels,
			   CANVAS_ATT_LEFT_X, &win_scale.left_x,
			   CANVAS_ATT_RIGHT_X, &win_scale.right_x,
			   CANVAS_ATT_BOTTOM_Y, &win_scale.bottom_y,
			   CANVAS_ATT_TOP_Y, &win_scale.top_y,
			   CANVAS_ATT_END);
    x_offset = win_scale.x_offset;
    x_pixels = win_scale.x_pixels;
    y_offset = win_scale.y_offset;
    y_pixels = win_scale.y_pixels;
    if (to_world)
    {
	/*  Convert from pixel to world co-ordinates  */
	/*  Compute x co-ordinates  */
	coord_num0 = ds_get_coord_num (hdim, win_scale.left_x,
				       SEARCH_BIAS_CLOSEST);
	coord0 = ds_get_coordinate (hdim, coord_num0);
	coord_num1 = ds_get_coord_num (hdim, win_scale.right_x,
				       SEARCH_BIAS_CLOSEST);
	if (coord_num1 + 1 < hdim->length)
	{
	    coord1 = ds_get_coordinate (hdim, coord_num1 + 1);
	}
	else
	{
	    coord1 = ds_get_coordinate (hdim, coord_num1);
	    coord1 += (coord1 - coord0) / (coord_num1 - coord_num0);
	}
	for (count = 0; count < num_coords; ++count)
	{
	    x_pix = xin[count] - x_offset;
	    x_pix = (coord1 - coord0) * x_pix / x_pixels;
	    x_pix += (coord1 - coord0) / (double) (coord_num1 -
						   coord_num0) / 1000.0;
	    xout[count] = x_pix + coord0;
	}
	/*  Compute y co-ordinates  */
	coord_num0 = ds_get_coord_num (vdim, win_scale.bottom_y,
				       SEARCH_BIAS_CLOSEST);
	coord0 = ds_get_coordinate (vdim, coord_num0);
	coord_num1 = ds_get_coord_num (vdim, win_scale.top_y,
				       SEARCH_BIAS_CLOSEST);
	if (coord_num1 + 1 < vdim->length)
	{
	    coord1 = ds_get_coordinate (vdim, coord_num1 + 1);
	}
	else
	{
	    coord1 = ds_get_coordinate (vdim, coord_num1);
	    coord1 += (coord1 - coord0) / (coord_num1 - coord_num0);
	}
	for (count = 0; count < num_coords; ++count)
	{
	    y_pix = yin[count] - y_offset;
	    y_pix = (y_pixels - 1.0) - y_pix;
	    /*  Removed offsets and flipped vertical  */
	    y_pix = (coord1 - coord0) * y_pix / y_pixels;
	    y_pix += (coord1 - coord0) / (double) (coord_num1 -
						   coord_num0) / 1000.0;
	    yout[count] = y_pix + coord0;
	}
	return (TRUE);
    }
    /*  Convert from world to pixel co-ordinates  */
    /*  Compute x co-ordinates  */
    coord_num0 = ds_get_coord_num (hdim, win_scale.left_x,
				   SEARCH_BIAS_CLOSEST);
    coord0 = ds_get_coordinate (hdim, coord_num0);
    /*  Find co-ordinate one data value beyond canvas maximum  */
    coord_num1 = ds_get_coord_num (hdim, win_scale.right_x,
				   SEARCH_BIAS_CLOSEST);
    if (coord_num1 + 1 < hdim->length)
    {
	coord1 = ds_get_coordinate (hdim, coord_num1 + 1);
    }
    else
    {
	coord1 = ds_get_coordinate (hdim, coord_num1);
	coord1 += (coord1 - coord0) / (coord_num1 - coord_num0);
    }
    /*  Compute increment between dimension indices. This increment is used to
	place a point that lies exactly on a data value's co-ordinate at the
	centre of the box of pixels for the data value  */
    xdelt = ds_get_coordinate (hdim, 1) - ds_get_coordinate (hdim, 0);
    for (count = 0; count < num_coords; ++count)
    {
	x_pix = (xin[count] - coord0 + xdelt * 0.5) / (coord1 - coord0);
	x_pix *= x_pixels;
	xout[count] = x_pix + x_offset;
    }
    /*  Compute y co-ordinates  */
    coord_num0 = ds_get_coord_num (vdim, win_scale.bottom_y,
				   SEARCH_BIAS_CLOSEST);
    coord0 = ds_get_coordinate (vdim, coord_num0);
    /*  Find co-ordinate one data value beyond canvas maximum  */
    coord_num1 = ds_get_coord_num (vdim, win_scale.top_y,
				   SEARCH_BIAS_CLOSEST);
    coord1 = ds_get_coordinate (vdim, coord_num1);
    if (coord_num1 + 1 < vdim->length)
    {
	coord1 = ds_get_coordinate (vdim, coord_num1 + 1);
    }
    else
    {
	coord1 = ds_get_coordinate (vdim, coord_num1);
	coord1 += (coord1 - coord0) / (coord_num1 - coord_num0);
    }
    /*  Compute increment between dimension indices. This increment is used to
	place a point that lies exactly on a data value's co-ordinate at the
	centre of the box of pixels for the data value  */
    ydelt = ds_get_coordinate (vdim, 1) - ds_get_coordinate (vdim, 0);
    for (count = 0; count < num_coords; ++count)
    {
	y_pix = (yin[count] - coord0 + ydelt * 0.5) / (coord1 - coord0);
	y_pix *= y_pixels;
	/*  Flip vertical  */
	yout[count] = (y_offset + y_pixels - 1.0) - y_pix;
    }
    return (TRUE);
}   /*  End Function coord_convert_func  */

static void initialise_vimage (ViewableImage vimage)
/*  [SUMMARY] Initialise a ViewableImage structure.
    <vimage> The ViewableImage to initialise.
    [RETURNS] Nothing.
*/
{
    vimage->value_min = TOOBIG;
    vimage->value_max = TOOBIG;
    vimage->magic_number = VIMAGE_MAGIC_NUMBER;
    vimage->canvas_holder = NULL;
    vimage->pc_multi_desc = NULL;
    vimage->pc_arr_desc = NULL;
    vimage->pc_slice = NULL;
    vimage->pc_hdim = 0;
    vimage->pc_vdim = 0;
    vimage->pc_elem_index = 0;
    vimage->tc_multi_desc = NULL;
    vimage->tc_arr_desc = NULL;
    vimage->tc_slice = NULL;
    vimage->recompute = TRUE;
    vimage->changed = TRUE;
    vimage->pixcanvas_width = -1;
    vimage->pixcanvas_height = -1;
    vimage->cache = NULL;
    /*  Compute strides  */
    vimage->pc_hstride = 0;
    vimage->pc_vstride = 0;
    vimage->pc_data_scale = 1.0;
    vimage->pc_data_offset = 0.0;
    vimage->sequence = NULL;
    vimage->num_restrictions = 0;
    vimage->restriction_names = NULL;
    vimage->restriction_values = NULL;
    vimage->iscale_test[0] = TOOBIG;
    canvas_init_win_scale (&vimage->win_scale, K_WIN_SCALE_MAGIC_NUMBER);
    vimage->override.value_min = TOOBIG;
    vimage->override.value_max = TOOBIG;
    vimage->prev = NULL;
    vimage->next = NULL;
}   /*  End Function initialise_vimage  */

static flag copy_restrictions (ViewableImage vimage, unsigned int num_restr,
			       CONST char **restr_names,
			       CONST double *restr_values)
/*  [SUMMARY] Copy restriction information.
    <vimage> The ViewableImage to copy to.
    <num_restr> The number of matched restrictions. If this is 0, no
    restrictions are recorded (this is the same as calling [<viewimg_create>]).
    <restr_names> The restriction names.
    <restr_values> The restriction values.
    [RETURNS] TRUE on sucess, else FALSE.
*/
{
    unsigned int count;
    static char function_name[] = "__viewimg_copy_restrictions";

    if ( ( vimage->restriction_names = (char **)
	   m_alloc (sizeof *restr_names * num_restr) ) == NULL )
    {
	m_error_notify (function_name, "array of restriction names");
	return (FALSE);
    }
    if ( ( vimage->restriction_values = (double *)
	   m_alloc (sizeof *restr_values * num_restr) ) == NULL )
    {
	m_error_notify (function_name, "array of restriction values");
	return (FALSE);
    }
    for (count = 0; count < num_restr; ++count)
    {
	if ( ( vimage->restriction_names[count] = st_dup (restr_names[count]) )
	     == NULL )
	{
	    m_error_notify (function_name, "restriction name");
	    while (count > 0)
	    {
		m_free (vimage->restriction_names[count]);
		--count;
	    }
	    m_free ( (char *) vimage->restriction_names );
	    m_free ( (char *) vimage->restriction_values );
	    return (FALSE);
	}
	vimage->restriction_values[count] = restr_values[count];
    }
    vimage->num_restrictions = num_restr;
    return (TRUE);
}   /*  End Function copy_restrictions  */

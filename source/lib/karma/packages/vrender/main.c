/*LINTLIBRARY*/
/*  main.c

    This code provides basic volume rendering support.

    Copyright (C) 1995-1996  Richard Gooch

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

    This file provides basic volume rendering support.


    Written by      Richard Gooch   15-OCT-1995

    Updated by      Richard Gooch   3-JAN-1996: Implemented all useful existing
  features in the old volume rendering tool that belong here.

    Updated by      Richard Gooch   18-JAN-1996: Fixed bug in
  <vrender_project_3d> which falsely decided some corner points were invisible.

    Updated by      Richard Gooch   13-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   3-MAY-1996: Changed to shared thread pool.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   4-JUN-1996: Took account of new fields in
  dimension descriptor for first and last co-ordinate.

    Updated by      Richard Gooch   21-AUG-1996: Created <vrender_get_eye_info>

    Updated by      Richard Gooch   2-SEP-1996: Created
  <vrender_view_notify_func>.

    Updated by      Richard Gooch   12-SEP-1996: Do not allocate reorder buffer
  for left and right eyes if stereo has not yet been displayed, rather,
  allocate on demand in <reorder_worker>.

    Last updated by Richard Gooch   29-OCT-1996: Tidied up macros to keep
  Solaris 2 compiler happy.


*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdarg.h>
#include <errno.h>
#include <karma.h>
#include <karma_vrender.h>
#include <karma_iarray.h>
#include <karma_dsrw.h>
#include <karma_conn.h>
#include <karma_pio.h>
#include <karma_ch.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_aa.h>
#include <karma_mt.h>
#include <karma_wf.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_c.h>

#define VERTICAL_DIMENSION_NAME "y"
#define HORIZONTAL_DIMENSION_NAME "x"
#define DEFAULT_LENGTH 256
#define NUM_OUTPUT_DIM 2
#define STEP_X 0
#define STEP_Y 1
#define STEP_Z 2

#define CONTEXT_MAGIC_NUMBER (unsigned int) 1453908345
#define PROTOCOL_VERSION (unsigned int) 0

#define VERIFY_CONTEXT(ctx) if (ctx == NULL) \
{fprintf (stderr, "NULL vrend context passed\n"); \
 a_prog_bug (function_name); } \
if (ctx->magic_number != CONTEXT_MAGIC_NUMBER) \
{fprintf (stderr, "Invalid vrend context object\n"); \
 a_prog_bug (function_name); }


/*  Protocol definition  */

/*  Master to Slave commands  */
#define MtoS_CUBE                0
#define MtoS_VIEW                1
#define MtoS_SHADER              2
#define MtoS_SUBCUBE             3
#define MtoS_PROJECTION          4
#define MtoS_EYE_SEPARATION      5
#define MtoS_SMOOTH_CACHE        6
#define MtoS_SHADER_BLANK_PACKET 7
#define MtoS_RENDER              8
#define MtoS_COMPUTE_CACHES      9

/*  Structure declarations follow  */

typedef struct
{
    float h;
    float v;
    float d;
} RotatedKcoord_3d;

typedef struct shader_type
{
    CONST char *name;
    void (*slow_func) (signed char **_sh_planes,
		       uaddr *_sh_dim_offsets_v, uaddr *_sh_dim_offsets_h,
		       float _sh_ray_start_d,
		       float _sh_ray_start_v,
		       float _sh_ray_start_h,
		       float _sh_ray_direction_d,
		       float _sh_ray_direction_v,
		       float _sh_ray_direction_h,
		       float _sh_one_on_ray_direction_d,
		       float _sh_min_d, float _sh_max_d,
		       double *minimum_image_value,
		       double *maximum_image_value,
		       char *pixel_ptr,
		       RotatedKcoord_3d normal, RotatedKcoord_3d vpc,
		       float t_enter);
    void (*fast_func) (signed char *ray, int length,
		       double *minimum_image_value,
		       double *maximum_image_value,
		       void *pixel_ptr);
    packet_desc *pack_desc;
    char *blank_packet;
    unsigned int packet_size;
    void *info;
} *Shader;

typedef struct
{
    double min;
    double max;
    unsigned int start_y;
    unsigned int stop_y;
    char *left_line;
    char *right_line;
    KVolumeRenderContext context;
} job_info;

struct line_type
{
    unsigned int start;
    unsigned int stop;
};

typedef struct
{
    int start_plane;
    int length;
    float t_enter;
    float t_leave;
    signed char *ray;
} ray_data;

typedef struct
{
    KVolumeRenderContext context;
    Kcoord_3d orig_position, orig_direction, orig_horizontal;
    Kcoord_3d orig_ras_plane_centre;
    /*  Information valid after view applied  */
    RotatedKcoord_3d rot_subcube_start, rot_subcube_end;
    RotatedKcoord_3d rot_position, rot_focus;
    RotatedKcoord_3d rot_vertical, rot_direction, rot_horizontal;
    uaddr *h_offsets, *v_offsets, *d_offsets;
    unsigned int num_planes_allocated;
    signed char **planes;
    RotatedKcoord_3d rot_ras_plane_centre;
    unsigned int num_lines_allocated;
    struct line_type *lines;
    unsigned int num_lines_computed;
    /*  Re-ordered cube  */
    signed char *reorder_buffer;
    uaddr reorder_buf_size;
    ray_data *reorder_rays;
    uaddr reorder_plane_size;
    unsigned int num_reordered_lines;
    unsigned int num_setup_lines;
    signed char *next_block;
} eye_info;

struct vrendercontext_type
{
    unsigned int magic_number;
    void *info;
    /*  Varargs specified  */
    iarray cube;
    view_specification view;
    Shader shader;
    unsigned long subcube_x_start;
    unsigned long subcube_x_end;
    unsigned long subcube_y_start;
    unsigned long subcube_y_end;
    unsigned long subcube_z_start;
    unsigned long subcube_z_end;
    unsigned int projection;
    float eye_separation;
    flag smooth_cache;
    /*  Varargs obtainable  */
    array_desc *arr_desc;
    flag valid_image_desc;
    /*  Private data  */
    KCallbackFunc cube_destroy_cbk;
    flag cube_from_master;
    dim_desc h_dim;
    dim_desc v_dim;
    flag valid_view_info_cache;
    job_info *job_data;
    eye_info cyclops, left, right;
    signed char *query_ray;
    uaddr query_ray_length;
    KWorkFunc worker;
    flag never_did_stereo;
    KCallbackList image_desc_notify_list;
    KCallbackList cache_notify_list;
    KCallbackList view_notify_list;
    Connection master;
    unsigned int num_slaves;
};

typedef struct
{
    int max_voxel;
    float red;
    float green;
    float blue;
    float opacity;
} substance;

typedef struct
{
    KVolumeRenderContext context;
} *ClientInfo;


/*  Private data  */
static KAssociativeArray shaders = NULL;
static KVolumeRenderContext context_for_connections = NULL;


/*  Private functions  */
STATIC_FUNCTION (flag process_context_attributes,
		 (KVolumeRenderContext context, va_list argp) );
STATIC_FUNCTION (void initialise_shader_aa, () );
STATIC_FUNCTION (void *key_copy_func,
		 (CONST char *key, uaddr length, flag *ok) );
STATIC_FUNCTION (void shader_destroy_func, (Shader shader) );
STATIC_FUNCTION (void compute_output_image_desc,
		 (KVolumeRenderContext context) );
STATIC_FUNCTION (void compute_view_info_cache, (KVolumeRenderContext context));
STATIC_FUNCTION (void compute_eye_info_cache, (eye_info *eye, flag no_alloc) );
STATIC_FUNCTION (void rotate_3d,
		 (RotatedKcoord_3d *rot, Kcoord_3d orig, unsigned int step) );
STATIC_FUNCTION (void generate_line,
		 (void *pool_info, void *call_info1, void *call_info2,
		  void *call_info3, void *call_info4, void *thread_info) );
STATIC_FUNCTION (flag get_ray_intersections_with_cube,
		 (RotatedKcoord_3d *position, RotatedKcoord_3d *direction,
		  RotatedKcoord_3d *one_on_direction,
		  RotatedKcoord_3d *subcube_start,
		  RotatedKcoord_3d *subcube_end,
		  float *min_d, float *max_d, float *enter, float *leave) );
STATIC_FUNCTION (void collect_ray_rough,
		 (iarray cube, eye_info *eye,
		  RotatedKcoord_3d ray_start, RotatedKcoord_3d direction,
		  float t_enter, float t_leave,
		  signed char *buffer, unsigned int ray_length) );
STATIC_FUNCTION (void collect_ray_smooth,
		 (iarray cube, eye_info *eye,
		  RotatedKcoord_3d ray_start, RotatedKcoord_3d direction,
		  float t_enter, float t_leave,
		  signed char *buffer, unsigned int ray_length) );
STATIC_FUNCTION (flag test_pixel_sees_cube,
		 (KVolumeRenderContext context, eye_info *eye,
		  int x_coord, int y_coord, int *ray_length) );
STATIC_FUNCTION (void reorder_job,
		 (void *pool_info, void *call_info1, void *call_info2,
		  void *call_info3, void *call_info4, void *thread_info) );
STATIC_FUNCTION (flag reorder_worker, (eye_info *eye) );
STATIC_FUNCTION (flag worker_function, (void **info) );
STATIC_FUNCTION (flag eye_worker, (eye_info *eye) );
STATIC_FUNCTION (flag compute_line_for_cache, (eye_info *eye) );
STATIC_FUNCTION (void geom_vector_multiply,
		 (Kcoord_3d *out, Kcoord_3d vec1, Kcoord_3d vec2) );
STATIC_FUNCTION (float geom_vector_dot_product,
		 (Kcoord_3d vec1, Kcoord_3d vec2) );
STATIC_FUNCTION (float geom_intersect_plane_with_ray,
		 (Kcoord_3d point, Kcoord_3d normal,
		  Kcoord_3d start, Kcoord_3d direction,
		  Kcoord_3d *intersection_point) );
STATIC_FUNCTION (void cube_destroy_func, (iarray array, void *info) );
STATIC_FUNCTION (void initialise_communications, () );
STATIC_FUNCTION (flag server_open_func, (Connection connection, void **info) );
STATIC_FUNCTION (flag server_read_func, (Connection connection, void **info) );
STATIC_FUNCTION (void server_close_func, (Connection connection, void *info) );
STATIC_FUNCTION (flag client_validate_func, (void **info) );
STATIC_FUNCTION (flag client_open_func, (Connection connection, void **info) );
STATIC_FUNCTION (flag client_read_func, (Connection connection, void **info) );
STATIC_FUNCTION (void client_close_func, (Connection connection, void *info) );
STATIC_FUNCTION (flag send_cube_func,
		 (KVolumeRenderContext context, Channel channel) );
STATIC_FUNCTION (flag send_view_func,
		 (KVolumeRenderContext context, Channel channel) );
STATIC_FUNCTION (flag send_shader_func,
		 (KVolumeRenderContext context, Channel channel) );
STATIC_FUNCTION (flag send_subcube_func,
		 (KVolumeRenderContext context, Channel channel) );
STATIC_FUNCTION (flag send_projection_func,
		 (KVolumeRenderContext context, Channel channel) );
STATIC_FUNCTION (flag send_eye_separation_func,
		 (KVolumeRenderContext context, Channel channel) );
STATIC_FUNCTION (flag send_smooth_cache_func,
		 (KVolumeRenderContext context, Channel channel) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KVolumeRenderContext vrender_create_context (void *info, ...)
/*  [SUMMARY] Create a context for volume rendering.
    <info> An arbitrary information pointer associated with the context.
    [VARARGS] The optional list of parameter attribute-key attribute-value
    pairs must follow. This list must be terminated with the value
    VRENDER_CONTEXT_ATT_END. See [<VRENDER_ATTRIBUTES>] for a list of defined
    attributes.
    [RETURNS] A KVolumeRenderContext on success, else NULL.
*/
{
    va_list argp;
    KVolumeRenderContext context;
    extern KVolumeRenderContext context_for_connections;
    static char function_name[] = "vrender_create_context";

    va_start (argp, info);
    initialise_communications ();
    if ( ( context = (KVolumeRenderContext) m_alloc (sizeof *context) )
	 == NULL )
    {
	m_error_notify (function_name, "context");
	return (NULL);
    }
    m_clear ( (char *) context, sizeof *context );
    context->info = info;
    context->magic_number = CONTEXT_MAGIC_NUMBER;
    context->projection = VRENDER_PROJECTION_PARALLEL;
    context->eye_separation = 50.0;
    context->smooth_cache = FALSE;
    context->valid_image_desc = FALSE;
    context->cube_destroy_cbk = NULL;
    context->cube_from_master = FALSE;
    context->valid_view_info_cache = FALSE;
    context->cyclops.context = context;
    context->cyclops.num_planes_allocated = 0;
    context->cyclops.planes = NULL;
    context->cyclops.num_lines_allocated = 0;
    context->cyclops.lines = NULL;
    context->cyclops.reorder_buffer = NULL;
    context->cyclops.reorder_buf_size = 0;
    context->cyclops.reorder_rays = NULL;
    context->cyclops.reorder_plane_size = 0;
    context->cyclops.num_reordered_lines = 0;
    context->cyclops.num_setup_lines = 0;
    context->left.context = context;
    context->left.num_planes_allocated = 0;
    context->left.planes = NULL;
    context->left.num_lines_allocated = 0;
    context->left.lines = NULL;
    context->left.reorder_buffer = NULL;
    context->left.reorder_buf_size = 0;
    context->left.reorder_rays = NULL;
    context->left.reorder_plane_size = 0;
    context->left.num_reordered_lines = 0;
    context->left.num_setup_lines = 0;
    context->right.context = context;
    context->right.num_planes_allocated = 0;
    context->right.planes = NULL;
    context->right.num_lines_allocated = 0;
    context->right.lines = NULL;
    context->right.reorder_buffer = NULL;
    context->right.reorder_buf_size = 0;
    context->right.reorder_rays = NULL;
    context->right.reorder_plane_size = 0;
    context->right.num_reordered_lines = 0;
    context->right.num_setup_lines = 0;
    context->job_data = NULL;
    context->query_ray = NULL;
    context->query_ray_length = 0;
    context->worker = NULL;
    context->never_did_stereo = TRUE;
    context->image_desc_notify_list = NULL;
    context->cache_notify_list = NULL;
    context->view_notify_list = NULL;
    context->master = NULL;
    context->num_slaves = 0;
    if ( !process_context_attributes (context, argp) )
    {
	context->magic_number = 0;
	m_free ( (char *) context );
	return (NULL);
    }
    va_end (argp);
    context_for_connections = context;
    return (context);
}   /*  End Function vrender_create_context  */

/*PUBLIC_FUNCTION*/
flag vrender_set_context_attributes (KVolumeRenderContext context, ...)
/*  [SUMMARY] Set the attributes for a volume rendering context.
    <context> The volume rendering context.
    [VARARGS] The optional list of parameter attribute-key attribute-value
    pairs must follow. This list must be terminated with the value
    VRENDER_CONTEXT_ATT_END. See [<VRENDER_ATTRIBUTES>] for a list of defined
    attributes.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    static char function_name[] = "vrender_set_context_attributes";

    va_start (argp, context);
    VERIFY_CONTEXT (context);
    return ( process_context_attributes (context, argp) );
    va_end (argp);
}   /*  End Function vrender_set_context_attributes  */

/*PUBLIC_FUNCTION*/
flag vrender_get_context_attributes (KVolumeRenderContext context, ...)
/*  [SUMMARY] Get the attributes for a volume rendering context.
    <context> The volume rendering context.
    [VARARGS] The optional list of parameter attribute-key attribute-value-ptr
    pairs must follow. This list must be terminated with the value
    VRENDER_CONTEXT_ATT_END. See [<VRENDER_ATTRIBUTES>] for a list of defined
    attributes.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    unsigned int att_key;
    flag *flag_ptr;
    unsigned long *ul_ptr;
    array_desc **arr_desc;
    view_specification *view;
    static char function_name[] = "vrender_get_context_attributes";

    va_start (argp, context);
    VERIFY_CONTEXT (context);
    while ( ( att_key = va_arg (argp, unsigned int) ) !=
	   VRENDER_CONTEXT_ATT_END )
    {
	switch (att_key)
	{
	  case VRENDER_CONTEXT_ATT_VIEW:
	    if ( ( view = va_arg (argp, view_specification *) ) == NULL )
	    {
		fprintf (stderr, "NULL view pointer passed\n");
		a_prog_bug (function_name);
	    }
	    m_copy ( (char *) view, (char *) &context->view, sizeof *view );
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_X_START:
	    if ( ( ul_ptr = va_arg (argp, unsigned long *) ) == NULL )
	    {
		fprintf (stderr, "NULL x_start pointer passed\n");
		a_prog_bug (function_name);
	    }
	    *ul_ptr = context->subcube_x_start;
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_X_END:
	    if ( ( ul_ptr = va_arg (argp, unsigned long *) ) == NULL )
	    {
		fprintf (stderr, "NULL x_end pointer passed\n");
		a_prog_bug (function_name);
	    }
	    *ul_ptr = context->subcube_x_end;
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_Y_START:
	    if ( ( ul_ptr = va_arg (argp, unsigned long *) ) == NULL )
	    {
		fprintf (stderr, "NULL y_start pointer passed\n");
		a_prog_bug (function_name);
	    }
	    *ul_ptr = context->subcube_y_start;
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_Y_END:
	    if ( ( ul_ptr = va_arg (argp, unsigned long *) ) == NULL )
	    {
		fprintf (stderr, "NULL y_end pointer passed\n");
		a_prog_bug (function_name);
	    }
	    *ul_ptr = context->subcube_y_end;
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_Z_START:
	    if ( ( ul_ptr = va_arg (argp, unsigned long *) ) == NULL )
	    {
		fprintf (stderr, "NULL z_start pointer passed\n");
		a_prog_bug (function_name);
	    }
	    *ul_ptr = context->subcube_z_start;
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_Z_END:
	    if ( ( ul_ptr = va_arg (argp, unsigned long *) ) == NULL )
	    {
		fprintf (stderr, "NULL z_end pointer passed\n");
		a_prog_bug (function_name);
	    }
	    *ul_ptr = context->subcube_z_end;
	    break;
	  case VRENDER_CONTEXT_ATT_IMAGE_DESC:
	    if ( ( arr_desc = va_arg (argp, array_desc **) ) == NULL )
	    {
		fprintf (stderr,
				"NULL image descriptor pointer passed\n");
		a_prog_bug (function_name);
	    }
	    compute_output_image_desc (context);
	    *arr_desc = context->arr_desc;
	    break;
	  case VRENDER_CONTEXT_ATT_VALID_IMAGE_DESC:
	    if ( ( flag_ptr = va_arg (argp, flag *) ) == NULL )
	    {
		fprintf (stderr,
				"NULL valid flag pointer passed\n");
		a_prog_bug (function_name);
	    }
	    *flag_ptr = context->valid_image_desc;
	    break;
	  default:
	    fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    va_end (argp);
    return (TRUE);
}   /*  End Function vrender_get_context_attributes  */

/*PUBLIC_FUNCTION*/
void vrender_register_shader (void (*slow_func) (), void (*fast_func) (),
			      CONST char *name,
			      packet_desc *pack_desc, CONST char *blank_packet,
			      void *info, flag front)
/*  [SUMMARY] Register a shader function.
    <slow_func> The slow shader function.
    <fast_func> The fast shader function. If this is NULL the slow function
    will always be used.
    <name> The name of the shader function.
    <pack_desc> The packet descriptor describing the output image pixels. A
    copy is made, hence the original may be deallocated.
    <blank_packet> A pointer to a sample packet containing blank data.
    <info> An arbitrary information pointer associated with the shader.
    <front> If TRUE, the new shader is placed at the front of the list, else
    it is placed at the back of the list.
    [RETURNS] Nothing. The process aborts on error.
*/
{
    struct shader_type tmp;
    void **ptr;
    extern KAssociativeArray shaders;
    static char function_name[] = "vrender_register_shader";

    initialise_shader_aa ();
    if ( ( tmp.name = st_dup (name) ) == NULL )
    {
	m_abort (function_name, "shader name");
    }
    ptr = (void **) &tmp.slow_func;
    *ptr = (void *) slow_func;
    ptr = (void **) &tmp.fast_func;
    *ptr = (void *) fast_func;
    if ( !ds_packet_all_data (pack_desc) )
    {
	fprintf (stderr, "Shader packet descriptor not atomic\n");
	a_prog_bug (function_name);
    }
    if ( ( tmp.pack_desc = ds_copy_desc_until (pack_desc, NULL) ) == NULL )
    {
	m_abort (function_name, "shader output descriptor");
    }
    if ( ( tmp.blank_packet = ds_alloc_packet (pack_desc) ) == NULL )
    {
	m_abort (function_name, "shader blank packet");
    }
    tmp.packet_size = ds_get_packet_size (pack_desc);
    m_copy (tmp.blank_packet, blank_packet, tmp.packet_size);
    tmp.info = info;
    if (aa_put_pair (shaders, (void *) name, 0, &tmp, sizeof tmp,
		     KAA_REPLACEMENT_POLICY_NEW, front) == NULL)
    {
	m_abort (function_name, "new shader");
    }
}   /*  End Function vrender_register_shader  */

/*PUBLIC_FUNCTION*/
void vrender_change_shader_blank_packet (CONST char *name,
					 CONST char *blank_packet)
/*  [SUMMARY] Change blank value for a particular shader.
    [PURPOSE] This routine will change the data used to write blank values for
    a particular shader.
    <name> The name of the shader.
    <blank_packet> A pointer to a sample packet containing blank data.
    [RETURNS] Nothing.
*/
{
    Shader shader;
    extern KAssociativeArray shaders;
    static char function_name[] = "vrender_change_shader_blank_packet";

    if (aa_get_pair (shaders, (void *) name, (void **) &shader) == NULL)
    {
	fprintf (stderr, "Shader: \"%s\" not found\n", name);
	a_prog_bug (function_name);
    }
    m_copy (shader->blank_packet, blank_packet, shader->packet_size);
}   /*  End Function vrender_change_shader_blank_packet  */

/*PUBLIC_FUNCTION*/
CONST char **vrender_get_shaders (unsigned int *num_shaders)
/*  [SUMMARY] Get the names of all registered shaders.
    <num_shaders> The number of registered shaders is written here.
    [RETURNS] An array of shader names on success, else NULL. This array must
    be deallocated when no longer needed. The individual names must NOT be
    deallocated.
*/
{
    unsigned int count;
    uaddr key_length, value_length;
    KAssociativeArrayPair *pairs;
    char **names;
    void *key, *value;
    extern KAssociativeArray shaders;
    static char function_name[] = "vrender_get_shaders";

    if ( ( pairs = aa_get_all_pairs (shaders, num_shaders) ) == NULL )
    {
	m_error_notify (function_name, "shaders");
	return (NULL);
    }
    if ( ( names = (char **) m_alloc (sizeof *names * *num_shaders) ) == NULL )
    {
	m_error_notify (function_name, "array of shader names");
	m_free ( (char *) pairs );
	return (NULL);
    }
    for (count = 0; count < *num_shaders; ++count)
    {
	aa_get_pair_info (pairs[count], &key, &key_length,
			  &value, &value_length);
	names[count] = (char *) key;
    }
    m_free ( (char *) pairs );
    return ( (CONST char **) names );
}   /*  End Function vrender_get_shaders  */

/*PUBLIC_FUNCTION*/
flag vrender_to_buffer (KVolumeRenderContext context, char *left_buffer,
			char *right_buffer, double *min, double *max,
			void (*notify_func) (void *info), void *info)
/*  [SUMMARY] Render a scene in a volume rendering context to a buffer.
    <context> The volume rendering context.
    <left_buffer> The left eye buffer to render into. This must be correctly
    allocated.
    <right_buffer> The right eye buffer to render into. If this is NULL no
    right eye view is rendered and a monoscopic image is rendered into the left
    eye buffer.
    <min> The minimum value in the rendered images(s) is written here.
    <max> The maximum value in the rendered images(s) is written here.
    <notify_func> The function that is called when the rendering is completed.
    If this is NULL this routine blocks. Note that even if this function is
    specified, the routine may still block until the render completes if the
    operating system does not have thread support. The prototype function is
    [<VRENDER_PROTO_render_notify_func>].
    <info> The arbitrary information pointer.
    [MT-LEVEL] Unsafe per context.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    KThreadPool pool;
    unsigned int job_count, y_coord, y_step;
    unsigned int v_stride;
    double minimum_image_value;
    double maximum_image_value;
    job_info *job_data;
    static char function_name[] = "vrender_to_buffer";

    VERIFY_CONTEXT (context);
    if (context->cube == NULL)
    {
	fprintf (stderr, "No cube specified!\n");
	a_prog_bug (function_name);
    }
    if (context->shader == NULL)
    {
	fprintf (stderr, "No shader specified!\n");
	a_prog_bug (function_name);
    }
    if (left_buffer == NULL)
    {
	fprintf (stderr, "No left image buffer specified!\n");
	a_prog_bug (function_name);
    }
    compute_output_image_desc (context);
    compute_view_info_cache (context);
    minimum_image_value = TOOBIG;
    maximum_image_value = -TOOBIG;
    /*  Setup job structures  */
    pool = mt_get_shared_pool ();
    if (mt_num_threads (pool) < 2) y_step = context->v_dim.length;
    else y_step = context->v_dim.length / mt_num_threads (pool) / 4;
    v_stride = context->shader->packet_size * context->h_dim.length;
    job_data = context->job_data;
    for (y_coord = 0, job_count = 0; y_coord < context->v_dim.length;
	 y_coord += y_step, ++job_count)
    {
	job_data[job_count].start_y = y_coord;
	job_data[job_count].stop_y = y_coord + y_step;
	if (job_data[job_count].stop_y > context->v_dim.length)
	{
	    job_data[job_count].stop_y = context->v_dim.length;
	}
	job_data[job_count].left_line = left_buffer + y_coord * v_stride;
	if (right_buffer == NULL) job_data[job_count].right_line = NULL;
	else job_data[job_count].right_line = right_buffer + y_coord *v_stride;
	job_data[job_count].context = context;
	mt_launch_job (pool, generate_line,
		       job_data + job_count, NULL, NULL, NULL);
    }
    mt_wait_for_all_jobs (pool);
    /*  Aggregate minima and maxima  */
    for (y_coord = 0, job_count = 0; y_coord < context->v_dim.length;
	 y_coord += y_step, ++job_count)
    {
	if (job_data[job_count].min < minimum_image_value)
	{
	    minimum_image_value = job_data[job_count].min;
	}
	if (job_data[job_count].max > maximum_image_value)
	{
	    maximum_image_value = job_data[job_count].max;
	}
    }
    *min = minimum_image_value;
    *max = maximum_image_value;
    if ( (right_buffer != NULL) && context->never_did_stereo )
    {
	/*  Doing stereo for the first time: make sure cache is computed for
	    left and right eyes.  */
	context->never_did_stereo = FALSE;
	if ( worker_function ( (void **) &context ) )
	{
	    /*  More work needs to be done: do it in the background.  */
	    if (context->worker == NULL)
	    {
		context->worker = wf_register_func (worker_function, context,
						    KWF_PRIORITY_HIGHEST);
	    }
	}
    }
    if (notify_func != NULL) (*notify_func) (info);
    return (TRUE);
}   /*  End Function vrender_to_buffer  */

/*PUBLIC_FUNCTION*/
CONST signed char *vrender_collect_ray (KVolumeRenderContext context,
					unsigned int eye_view,
					Kcoord_2d pos_2d, Kcoord_3d *ray_start,
					Kcoord_3d *direction,
					float *t_enter, float *t_leave,
					unsigned int *ray_length)
/*  [SUMMARY] Collect a ray projected from an image plane into a volume.
    <context> The volume rendering context. This specifies the volume and view
    information.
    <eye_view> The eye which sees the ray. See [<VRENDER_EYES>] for a list of
    legal values.
    <pos_2d> The 2-dimensional position in the image plane to project from.
    <ray_start> The 3-dimensional position of the starting point of the ray is
    written here. This point lies on the raster plane.
    <direction> The 3-dimensional direction vector of the ray is written here.
    This is not normalised.
    <t_enter> The distance down the ray corresponding to the first voxel is
    written here.
    <t_leave> The distance down the ray corresponding to the last voxel is
    written here.
    <ray_length> The length of the ray through the volume is written here. If
    the ray does not intersect the volume, 0 is written here.
    [MT-LEVEL] Unsafe per context.
    [RETURNS] A pointer to the collected ray. This is a statically allocated
    array per context which is updated on each call. On error NULL is returned.
*/
{
    RotatedKcoord_3d ray_direction, one_on_ray_dir, rot_ray_start;
    eye_info *eye = NULL;  /*  Initialised to keep compiler happy  */
    ray_data *ray;
    int x_coord, y_coord;
    unsigned int length;
    float min_d, max_d;
    static char function_name[] = "vrender_collect_ray";

    VERIFY_CONTEXT (context);
    if (context->cube == NULL)
    {
	fprintf (stderr, "No cube specified!\n");
	a_prog_bug (function_name);
    }
    compute_view_info_cache (context);
    switch (eye_view)
    {
      case VRENDER_EYE_CHOICE_CYCLOPS:
	eye = &context->cyclops;
	break;
      case VRENDER_EYE_CHOICE_LEFT:
	eye = &context->left;
	break;
      case VRENDER_EYE_CHOICE_RIGHT:
	eye = &context->right;
	break;
      default:
	fprintf (stderr, "Illegal value of eye_view: %u\n", eye_view);
	a_prog_bug (function_name);
	break;
    }
    if ( (pos_2d.x < context->h_dim.first_coord) ||
	 (pos_2d.x > context->h_dim.last_coord) ||
	 (pos_2d.y < context->v_dim.first_coord) ||
	 (pos_2d.y > context->v_dim.last_coord) )
    {
	fprintf (stderr, "%s: WARNING: 2D point: %e %e is not in view plane\n",
		 function_name, pos_2d.x, pos_2d.y);
	return (NULL);
    }
    x_coord = ds_get_coord_num (&context->h_dim, pos_2d.x,
				SEARCH_BIAS_CLOSEST);
    y_coord = ds_get_coord_num (&context->v_dim, pos_2d.y,
				SEARCH_BIAS_CLOSEST);
    /*  Do a quick check if point has a ray which interesects the volume  */
    if (y_coord < eye->num_lines_computed)
    {
	if ( (x_coord < eye->lines[y_coord].start) ||
	     (x_coord >= eye->lines[y_coord].stop) ) return (NULL);
    }
    /*  Determine point in raster plane which corresponds to this pixel  */
    /*  For the benefit of the application  */
    ray_start->x = (eye->orig_ras_plane_centre.x +
		    pos_2d.x * eye->orig_horizontal.x +
		    pos_2d.y * context->view.vertical.x);
    ray_start->y = (eye->orig_ras_plane_centre.y +
		    pos_2d.x * eye->orig_horizontal.y +
		    pos_2d.y * context->view.vertical.y);
    ray_start->z = (eye->orig_ras_plane_centre.z +
		    pos_2d.x * eye->orig_horizontal.z +
		    pos_2d.y * context->view.vertical.z);
    /*  For my purposes  */
    rot_ray_start.h = (eye->rot_ras_plane_centre.h +
		       pos_2d.x * eye->rot_horizontal.h +
		       pos_2d.y * eye->rot_vertical.h);
    rot_ray_start.v = (eye->rot_ras_plane_centre.v +
		       pos_2d.x * eye->rot_horizontal.v +
		       pos_2d.y * eye->rot_vertical.v);
    rot_ray_start.d = (eye->rot_ras_plane_centre.d +
		       pos_2d.x * eye->rot_horizontal.d +
		       pos_2d.y * eye->rot_vertical.d);
    if (context->projection == VRENDER_PROJECTION_PARALLEL)
    {
	ray_direction = eye->rot_direction;
	direction->x = eye->orig_direction.x;
	direction->y = eye->orig_direction.y;
	direction->z = eye->orig_direction.z;
    }
    if (context->projection == VRENDER_PROJECTION_PERSPECTIVE)
    {
	/*  For perspective projection need to compute ray direction
	    for each point.
	    */
	ray_direction.h = rot_ray_start.h - eye->rot_position.h;
	ray_direction.v = rot_ray_start.v - eye->rot_position.v;
	ray_direction.d = rot_ray_start.d - eye->rot_position.d;
	direction->x = ray_start->x - eye->orig_position.x;
	direction->y = ray_start->y - eye->orig_position.y;
	direction->z = ray_start->z - eye->orig_position.z;
    }
    if (y_coord < eye->num_reordered_lines)
    {
	ray = eye->reorder_rays + x_coord + y_coord * context->h_dim.length;
	if (ray->ray == NULL)
	{
	    fprintf (stderr, "NULL ray  %f  %f\n", pos_2d.x, pos_2d.y);
	    return (NULL);
	}
	*t_enter = ray->t_enter;
	*t_leave = ray->t_leave;
	*ray_length = ray->length;
	return ( (CONST signed char *) ray->ray );
    }
    one_on_ray_dir.h = (ray_direction.h ==
			0.0) ? TOOBIG : 1.0 / ray_direction.h;
    one_on_ray_dir.v = (ray_direction.v ==
			0.0) ? TOOBIG : 1.0 / ray_direction.v;
    one_on_ray_dir.d = (ray_direction.d ==
			0.0) ? TOOBIG : 1.0 / ray_direction.d;
    if ( !get_ray_intersections_with_cube (&rot_ray_start, &ray_direction,
					   &one_on_ray_dir,
					   &eye->rot_subcube_start,
					   &eye->rot_subcube_end,
					   &min_d, &max_d, t_enter, t_leave) )
    {
	*ray_length = 0;
	return (NULL);
    }
    length = (int) floor (max_d) - (int) ceil (min_d) + 1;
    if (length > context->query_ray_length)
    {
	if (context->query_ray != NULL) m_free ( (char *) context->query_ray );
	context->query_ray = NULL;
	context->query_ray_length = 0;
    }
    if (context->query_ray == NULL)
    {
	if ( ( context->query_ray = (signed char *)
	      m_alloc (sizeof *context->query_ray * length) ) == NULL )
	{
	    m_error_notify (function_name, "ray");
	    return (NULL);
	}
	context->query_ray_length = length;
    }
    collect_ray_rough (context->cube, eye, rot_ray_start, ray_direction,
		       *t_enter, *t_leave, context->query_ray, length);
    *ray_length = length;
    return ( (CONST signed char *) context->query_ray );
}   /*  End Function vrender_collect_ray  */

/*PUBLIC_FUNCTION*/
flag vrender_project_3d (KVolumeRenderContext context, unsigned int eye_view,
			 Kcoord_3d point_3d, Kcoord_2d *point_2d,
			 flag test_visible)
/*  [SUMMARY] Project a point in 3-dimensional space onto a view plane.
    <context> The volume render context which determines the view information.
    <eye_view> The eye which will see the point. See [<VRENDER_EYES>] for a
    list of legal values.
    <point_3d> The 3 dimensional point to be projected.
    <point_2d> The projected 2 dimensional point will be written here.
    <test_visible> If TRUE, the point is tested for visibility.
    [RETURNS] TRUE if the point is visible, else FALSE if it would be obscured
    by the volume or <<test_visible>> is FALSE.
*/
{
    Kcoord_3d ray_direction, subcube_start, subcube_end, ray_start, tmp3d;
    Kcoord_3d one_on_ray_dir;
    float mag, distance_down_ray;
    float t, t_enter, t_leave;
    float offset = 1e-3;
    eye_info *eye = NULL;  /*  Initialised to keep compiler happy  */
    static char function_name[] = "vrender_project_3d";

    VERIFY_CONTEXT (context);
    if (context->cube == NULL)
    {
	fprintf (stderr, "No cube specified!\n");
	a_prog_bug (function_name);
    }
    compute_view_info_cache (context);
    switch (eye_view)
    {
      case VRENDER_EYE_CHOICE_CYCLOPS:
	eye = &context->cyclops;
	break;
      case VRENDER_EYE_CHOICE_LEFT:
	eye = &context->left;
	break;
      case VRENDER_EYE_CHOICE_RIGHT:
	eye = &context->right;
	break;
      default:
	fprintf (stderr, "Illegal value of eye_view: %u\n", eye_view);
	a_prog_bug (function_name);
	break;
    }
    ray_direction.x = point_3d.x - eye->orig_position.x;
    ray_direction.y = point_3d.y - eye->orig_position.y;
    ray_direction.z = point_3d.z - eye->orig_position.z;
    if (context->projection == VRENDER_PROJECTION_PARALLEL)
    {
	point_2d->x = geom_vector_dot_product (eye->orig_horizontal,
					       ray_direction);
	point_2d->y = geom_vector_dot_product (context->view.vertical,
					       ray_direction);
	if (!test_visible) return (FALSE);
	ray_start.x = eye->orig_position.x;
	ray_start.x += point_2d->x * eye->orig_horizontal.x;
	ray_start.x += point_2d->y * context->view.vertical.x;
	ray_start.y = eye->orig_position.y;
	ray_start.y += point_2d->x * eye->orig_horizontal.y;
	ray_start.y += point_2d->y * context->view.vertical.y;
	ray_start.z = eye->orig_position.z;
	ray_start.z += point_2d->x * eye->orig_horizontal.z;
	ray_start.z += point_2d->y * context->view.vertical.z;
	ray_direction.x = point_3d.x - ray_start.x;
	ray_direction.y = point_3d.y - ray_start.y;
	ray_direction.z = point_3d.z - ray_start.z;
    }
    else
    {
	/*  Perspective projection  */
	/*  Find distance down ray where raster plane is intersected  */
	t = geom_intersect_plane_with_ray (eye->orig_ras_plane_centre,
					   eye->orig_direction,
					   eye->orig_position, ray_direction,
					   NULL);
	/*  Convert distance to a ray of required length  */
	tmp3d.x = t * ray_direction.x;
	tmp3d.y = t * ray_direction.y;
	tmp3d.z = t * ray_direction.z;
	point_2d->x = geom_vector_dot_product (eye->orig_horizontal, tmp3d);
	point_2d->y = geom_vector_dot_product (context->view.vertical, tmp3d);
	if (!test_visible) return (FALSE);
	ray_start = eye->orig_position;
    }
    subcube_start.x = (float) context->subcube_x_start;
    subcube_end.x = (float) context->subcube_x_end;
    subcube_start.y = (float) context->subcube_y_start;
    subcube_end.y = (float) context->subcube_y_end;
    subcube_start.z = (float) context->subcube_z_start;
    subcube_end.z = (float) context->subcube_z_end;
    /*  Quick check if point is inside subcube  */
    if ( (point_3d.x > subcube_start.x) && (point_3d.x < subcube_end.x) &&
	(point_3d.y > subcube_start.y) && (point_3d.y < subcube_end.y) &&
	(point_3d.z > subcube_start.z) && (point_3d.z < subcube_end.z) )
    {
	return (FALSE);
    }
    /*  Normalise ray direction vector  */
    mag = sqrt (ray_direction.x * ray_direction.x +
		ray_direction.y * ray_direction.y +
		ray_direction.z * ray_direction.z);
    ray_direction.x /= mag;
    ray_direction.y /= mag;
    ray_direction.z /= mag;
    distance_down_ray = mag;
    subcube_start.x -= offset;
    subcube_end.x += offset;
    subcube_start.y -= offset;
    subcube_end.y += offset;
    subcube_start.z -= offset;
    subcube_end.z += offset;
    if (ray_direction.x == 0.0) one_on_ray_dir.x = TOOBIG;
    else one_on_ray_dir.x = 1.0 / ray_direction.x;
    if (ray_direction.y == 0.0) one_on_ray_dir.y = TOOBIG;
    else one_on_ray_dir.y = 1.0 / ray_direction.y;
    if (ray_direction.z == 0.0) one_on_ray_dir.z = TOOBIG;
    else one_on_ray_dir.z = 1.0 / ray_direction.z;
    if ( !get_ray_intersections_with_cube ( (RotatedKcoord_3d *) &ray_start,
					   (RotatedKcoord_3d *) &ray_direction,
					   (RotatedKcoord_3d *)&one_on_ray_dir,
					   (RotatedKcoord_3d *) &subcube_start,
					   (RotatedKcoord_3d *) &subcube_end,
					   NULL, NULL, &t_enter, &t_leave ) )
    {
/*
	if ( (point_3d.x == 0.0) && (point_3d.y == 0.0) && (point_3d.z ==0.0) )
	{
	    fprintf (stderr, "distance: %e\n",
			    distance_down_ray);
	}
*/
	return (TRUE);
    }
/*
    fprintf (stderr,
		    "distance_down_ray: %e  t_enter: %e  t_leave: %e\n",
		    distance_down_ray, t_enter, t_leave);
*/
    if (t_leave - t_enter < 0.5) return (TRUE);  /*  Ray barely hits cube  */
    /*  Ray passes though a significant depth of the cube  */
    if (distance_down_ray > t_leave + 0.5) return (FALSE);
    if (distance_down_ray < t_enter + 0.5) return (TRUE);
    /*  Assume not visible  */
    return (FALSE);
}   /*  End Function vrender_project_3d  */

/*PUBLIC_FUNCTION*/
void vrender_compute_caches (KVolumeRenderContext context, unsigned int eyes,
			     flag notify)
/*  [SUMMARY] Compute cache for volume rendering context.
    [PURPOSE] This routine will compute the caches for the specified eyes. This
    speeds up subsequent rendering several times. Nothing is done if the
    cache(s) are already computed.
    <context> The volume render context.
    <eyes> A bitmask specifying which eye views to compute. The bitmask may be
    constructed by ORing some values. See [<VRENDER_EYE_MASKS>] for a list of
    masks.
    <notify> If TRUE any functions registered with
    [<vrender_cache_notify_func>] will be called.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "vrender_compute_caches";

    VERIFY_CONTEXT (context);
    if (context->cube == NULL)
    {
	fprintf (stderr, "No cube specified!\n");
	a_prog_bug (function_name);
    }
    compute_output_image_desc (context);
    compute_view_info_cache (context);
    if (eyes & VRENDER_EYE_MASK_CYCLOPS) while (eye_worker(&context->cyclops));
    if (eyes & VRENDER_EYE_MASK_LEFT) while ( eye_worker (&context->left) );
    if (eyes & VRENDER_EYE_MASK_RIGHT) while ( eye_worker (&context->right) );
    if (notify) c_call_callbacks (context->cache_notify_list,
					 (void *) (uaddr) eyes);
}   /*  End Function vrender_compute_caches  */

/*PUBLIC_FUNCTION*/
KCallbackFunc vrender_image_desc_notify_func
    (KVolumeRenderContext context,
     void (*func) (KVolumeRenderContext context, void **info),
     void *info)
/*  [SUMMARY] Register image descriptor change callback.
    [PURPOSE] This routine will register a function which should be called
    whenever the output image descriptor is changed.
    <context> The volume render context.
    <func> The routine to be called. The prototype function is
    [<VRENDER_PROTO_image_desc_notify_func>]
    <info> The initial arbitrary information pointer.
    [RETURNS] A KCallbackFunc object on success. On failure, the process aborts
*/
{
    static char function_name[] = "vrender_image_desc_notify_func";

    VERIFY_CONTEXT (context);
    return ( c_register_callback (&context->image_desc_notify_list,
				  ( flag (*) () ) func, context, info, TRUE,
				  NULL, FALSE, FALSE) );
}   /*  End Function vrender_image_desc_notify_func  */

/*PUBLIC_FUNCTION*/
KCallbackFunc vrender_cache_notify_func
    (KVolumeRenderContext context,
     void (*func) (KVolumeRenderContext context, void **info, uaddr eyes),
     void *info)
/*  [SUMMARY] Register cache computed callback.
    [PURPOSE] This routine will register a function which should be called
    whenever the cache for one or more eye views is computed.
    <context> The volume render context.
    <func> The routine to be called. The prototype function is
    [<VRENDER_PROTO_cache_notify_func>]
    <info> The initial arbitrary information pointer.
    [RETURNS] A KCallbackFunc object on success. On failure, the process aborts
*/
{
    static char function_name[] = "vrender_cache_notify_func";

    VERIFY_CONTEXT (context);
    return ( c_register_callback (&context->cache_notify_list,
				  ( flag (*) () ) func, context, info, TRUE,
				  NULL, FALSE, FALSE) );
}   /*  End Function vrender_cache_notify_func  */

/*PUBLIC_FUNCTION*/
KCallbackFunc vrender_view_notify_func
    (KVolumeRenderContext context,
     void (*func) (KVolumeRenderContext context, void **info),
     void *info)
/*  [SUMMARY] Register view changed computed callback.
    [PURPOSE] This routine will register a function which should be called
    whenever the view for a context is changed.
    <context> The volume render context.
    <func> The routine to be called. The prototype function is
    [<VRENDER_PROTO_view_notify_func>]. This is called after view information
    has been computed, but before any cache information is computed.
    <info> The initial arbitrary information pointer.
    [RETURNS] A KCallbackFunc object on success. On failure, the process aborts
*/
{
    static char function_name[] = "vrender_view_notify_func";

    VERIFY_CONTEXT (context);
    return ( c_register_callback (&context->view_notify_list,
				  ( flag (*) () ) func, context, info, TRUE,
				  NULL, FALSE, FALSE) );
}   /*  End Function vrender_view_notify_func  */

/*PUBLIC_FUNCTION*/
void vrender_get_eye_info (KVolumeRenderContext context, unsigned int eye_view,
			   Kcoord_3d *ras_centre, Kcoord_3d *direction,
			   Kcoord_3d *horizontal)
/*  [SUMMARY] Get eye information.
    <context> The volume render context which determines the view information.
    <eye_view> The eye which will see the point. See [<VRENDER_EYES>] for a
    list of legal values.
    <ras_centre> The centre of the raster plane is written here.
    <direction> The direction towards infinity is written here.
    <horizontal> The horizontal direction vector is written here.
    [RETURNS] Nothing.
*/
{
    eye_info *eye = NULL;  /*  Initialised to keep compiler happy  */
    static char function_name[] = "vrender_get_eye_info";

    VERIFY_CONTEXT (context);
    if (context->cube == NULL)
    {
	fprintf (stderr, "No cube specified!\n");
	a_prog_bug (function_name);
    }
    compute_view_info_cache (context);
    switch (eye_view)
    {
      case VRENDER_EYE_CHOICE_CYCLOPS:
	eye = &context->cyclops;
	break;
      case VRENDER_EYE_CHOICE_LEFT:
	eye = &context->left;
	break;
      case VRENDER_EYE_CHOICE_RIGHT:
	eye = &context->right;
	break;
      default:
	fprintf (stderr, "Illegal value of eye_view: %u\n", eye_view);
	a_prog_bug (function_name);
	break;
    }
    *ras_centre = eye->orig_ras_plane_centre;
    *direction = eye->orig_direction;
    *horizontal = eye->orig_horizontal;
}   /*  End Function vrender_project_3d  */


/*  Private functions follow  */

static flag process_context_attributes (KVolumeRenderContext context,
					va_list argp)
/*  [PURPOSE] This routine will set the attributes for a volume rendering
    context.
    <context> The volume rendering context.
    <argp> The optional list of parameter attribute-key attribute-value
    pairs.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    iarray cube;
    Connection connection;
    Channel channel;
    Shader shader;
    flag bool;
    flag send_cube = FALSE;
    flag send_view = FALSE;
    flag send_shader = FALSE;
    flag send_subcube = FALSE;
    flag send_projection = FALSE;
    flag send_eye_separation = FALSE;
    flag send_smooth_cache = FALSE;
    unsigned int att_key, ui_val, count;
    unsigned long ulong;
    double d_val;
    view_specification *view;
    CONST char *shader_name;
    extern KAssociativeArray shaders;
    extern char *sys_errlist[];
    static char function_name[] = "__vrender_process_context_attributes";

    initialise_shader_aa ();
    while ( ( att_key = va_arg (argp, unsigned int) ) !=
	   VRENDER_CONTEXT_ATT_END )
    {
	switch (att_key)
	{
	  case VRENDER_CONTEXT_ATT_CUBE:
	    cube = va_arg (argp, iarray);
	    if (cube != NULL)
	    {
		if (iarray_num_dim (cube) != 3)
		{
		    fprintf (stderr, "Cube not 3-dimensional\n");
		    a_prog_bug (function_name);
		}
		if (iarray_type (cube) != K_BYTE)
		{
		    fprintf (stderr, "Cube not type K_BYTE\n");
		    a_prog_bug (function_name);
		}
	    }
	    if (context->cube_destroy_cbk != NULL)
	    {
		c_unregister_callback (context->cube_destroy_cbk);
	    }
	    if ( context->cube_from_master && (context->cube != NULL) )
	    {
		iarray_dealloc (context->cube);
	    }
	    context->cube_from_master = FALSE;
	    context->cube = cube;
	    context->cube_destroy_cbk =
		iarray_register_destroy_func (cube,
					      ( flag (*)() ) cube_destroy_func,
					      context);
	    context->subcube_x_start = 0;
	    context->subcube_x_end = iarray_dim_length (cube, 2) - 1;
	    context->subcube_y_end = 0;
	    context->subcube_y_end = iarray_dim_length (cube, 1) - 1;
	    context->subcube_z_start = 0;
	    context->subcube_z_end = iarray_dim_length (cube, 0) - 1;
	    context->valid_image_desc = FALSE;
	    context->valid_view_info_cache = FALSE;
	    send_cube = TRUE;
	    break;
	  case VRENDER_CONTEXT_ATT_VIEW:
	    if ( ( view = va_arg (argp, view_specification *) ) == NULL )
	    {
		fprintf (stderr, "NULL view specification pointer\n");
		a_prog_bug (function_name);
	    }
	    m_copy ( (char *) &context->view, (char *) view, sizeof *view );
	    context->valid_view_info_cache = FALSE;
	    send_view = TRUE;
	    break;
	  case VRENDER_CONTEXT_ATT_SHADER:
	    if ( ( shader_name = va_arg (argp, CONST char *) ) == NULL )
	    {
		fprintf (stderr, "NULL shader name pointer\n");
		a_prog_bug (function_name);
	    }
	    if (aa_get_pair (shaders, (void *) shader_name, (void **) &shader)
		== NULL)
	    {
		fprintf (stderr, "Shader: \"%s\" not found\n",
				shader_name);
		a_prog_bug (function_name);
	    }
	    context->shader = shader;
	    context->valid_image_desc = FALSE;
	    send_shader = TRUE;
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_X_START:
	    ulong = va_arg (argp, unsigned long);
	    if (iarray_dim_length (context->cube, 2) <= ulong)
	    {
		fprintf ( stderr,
			  "Subcube x start: %lu not less than length: %lu\n",
			  ulong, iarray_dim_length (context->cube, 2) );
		a_prog_bug (function_name);
	    }
	    if (ulong >= context->subcube_x_end)
	    {
		fprintf (stderr,
			 "Subcube x start: %lu not less than end: %lu\n",
			 ulong, context->subcube_x_end);
		a_prog_bug (function_name);
	    }
	    context->subcube_x_start = ulong;
	    context->valid_image_desc = FALSE;
	    context->valid_view_info_cache = FALSE;
	    send_subcube = TRUE;
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_X_END:
	    ulong = va_arg (argp, unsigned long);
	    if (iarray_dim_length (context->cube, 2) <= ulong)
	    {
		fprintf ( stderr,
			  "Subcube x end: %lu not less than length: %lu\n",
			  ulong, iarray_dim_length (context->cube, 2) );
		a_prog_bug (function_name);
	    }
	    if (ulong <= context->subcube_x_start)
	    {
		fprintf (stderr,
			 "Subcube x end: %lu not greater than start: %lu\n",
			 ulong, context->subcube_x_start);
		a_prog_bug (function_name);
	    }
	    context->subcube_x_end = ulong;
	    context->valid_image_desc = FALSE;
	    context->valid_view_info_cache = FALSE;
	    send_subcube = TRUE;
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_Y_START:
	    ulong = va_arg (argp, unsigned long);
	    if (iarray_dim_length (context->cube, 1) <= ulong)
	    {
		fprintf ( stderr,
			  "Subcube y start: %lu not less than length: %lu\n",
			  ulong, iarray_dim_length (context->cube, 1) );
		a_prog_bug (function_name);
	    }
	    if (ulong >= context->subcube_y_end)
	    {
		fprintf (stderr,
			 "Subcube y start: %lu not less than end: %lu\n",
			 ulong, context->subcube_y_end);
		a_prog_bug (function_name);
	    }
	    context->subcube_y_start = ulong;
	    context->valid_image_desc = FALSE;
	    context->valid_view_info_cache = FALSE;
	    send_subcube = TRUE;
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_Y_END:
	    ulong = va_arg (argp, unsigned long);
	    if (iarray_dim_length (context->cube, 1) <= ulong)
	    {
		fprintf ( stderr,
			  "Subcube y end: %lu not less than length: %lu\n",
			  ulong, iarray_dim_length (context->cube, 1) );
		a_prog_bug (function_name);
	    }
	    if (ulong <= context->subcube_y_start)
	    {
		fprintf (stderr,
			 "Subcube y end: %lu not greater than start: %lu\n",
			 ulong, context->subcube_y_start);
		a_prog_bug (function_name);
	    }
	    context->subcube_y_end = ulong;
	    context->valid_image_desc = FALSE;
	    context->valid_view_info_cache = FALSE;
	    send_subcube = TRUE;
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_Z_START:
	    ulong = va_arg (argp, unsigned long);
	    if (iarray_dim_length (context->cube, 0) <= ulong)
	    {
		fprintf ( stderr,
			  "Subcube z start: %lu not less than length: %lu\n",
			  ulong, iarray_dim_length (context->cube, 0) );
		a_prog_bug (function_name);
	    }
	    if (ulong >= context->subcube_z_end)
	    {
		fprintf (stderr,
			 "Subcube z start: %lu not less than end: %lu\n",
			 ulong, context->subcube_z_end);
		a_prog_bug (function_name);
	    }
	    context->subcube_z_start = ulong;
	    context->valid_image_desc = FALSE;
	    context->valid_view_info_cache = FALSE;
	    send_subcube = TRUE;
	    break;
	  case VRENDER_CONTEXT_ATT_SUBCUBE_Z_END:
	    ulong = va_arg (argp, unsigned long);
	    if (iarray_dim_length (context->cube, 0) <= ulong)
	    {
		fprintf ( stderr,
			  "Subcube z end: %lu not less than length: %lu\n",
			  ulong, iarray_dim_length (context->cube, 0) );
		a_prog_bug (function_name);
	    }
	    if (ulong <= context->subcube_z_start)
	    {
		fprintf (stderr,
			 "Subcube z end: %lu not greater than start: %lu\n",
			 ulong, context->subcube_z_start);
		a_prog_bug (function_name);
	    }
	    context->subcube_z_end = ulong;
	    context->valid_image_desc = FALSE;
	    context->valid_view_info_cache = FALSE;
	    send_subcube = TRUE;
	    break;
	  case VRENDER_CONTEXT_ATT_PROJECTION:
	    ui_val = va_arg (argp, unsigned int);
	    if ( (ui_val != VRENDER_PROJECTION_PARALLEL) &&
		(ui_val != VRENDER_PROJECTION_PERSPECTIVE) )
	    {
		fprintf (stderr, "Illegal projection type: %u\n",
				ui_val);
		a_prog_bug (function_name);
	    }
	    context->projection = ui_val;
	    context->valid_view_info_cache = FALSE;
	    send_projection = TRUE;
	    break;
	  case VRENDER_CONTEXT_ATT_EYE_SEPARATION:
	    if ( ( d_val = va_arg (argp, double) ) <= 0.0 )
	    {
		fprintf (stderr,
				"Separation: %e not greater than zero\n",
				d_val);
		a_prog_bug (function_name);
	    }
	    context->eye_separation = d_val;
	    context->valid_view_info_cache = FALSE;
	    send_eye_separation = TRUE;
	    break;
	  case VRENDER_CONTEXT_ATT_SMOOTH_CACHE:
	    bool = va_arg (argp, flag);
	    FLAG_VERIFY (bool);
	    if (context->smooth_cache != bool)
	    {
		context->smooth_cache = bool;
		context->valid_view_info_cache = FALSE;
	    }
	    send_smooth_cache = TRUE;
	    break;
	  default:
	    fprintf (stderr, "Unknown attribute key: %u\n", att_key);
	    a_prog_bug (function_name);
	    break;
	}
    }
    /*  Try to make the output image descriptor valid  */
    compute_output_image_desc (context);
    compute_view_info_cache (context);
    if (context->num_slaves < 1) return (TRUE);
    /*  Have to pass on the changes to all slaves  */
    for (count = 0; count < context->num_slaves; ++count)
    {
	if ( ( connection = conn_get_serv_connection ("VRENDER_private",
						      count) ) == NULL )
	{
	    fprintf (stderr, "Error getting connection: %u\n", count);
	    a_prog_bug (function_name);
	}
	channel = conn_get_channel (connection);
	if (send_cube)
	{
	    if ( !send_cube_func (context, channel) ) continue;
	}
	if (send_view)
	{
	    if ( !send_view_func (context, channel) )
	    {
		conn_close (connection);
		continue;
	    }
	}
	if (send_shader)
	{
	    if ( !send_shader_func (context, channel) )
	    {
		conn_close (connection);
		continue;
	    }
	}
	if (send_subcube)
	{
	    if ( !send_subcube_func (context, channel) )
	    {
		conn_close (connection);
		continue;
	    }
	}
	if (send_projection)
	{
	    if ( !send_projection_func (context, channel) )
	    {
		conn_close (connection);
		continue;
	    }
	}
	if (send_eye_separation)
	{
	    if ( !send_eye_separation_func (context, channel) )
	    {
		conn_close (connection);
		continue;
	    }
	}
	if (send_smooth_cache)
	{
	    if ( !send_smooth_cache_func (context, channel) )
	    {
		conn_close (connection);
		continue;
	    }
	}
	if ( !ch_flush (channel) )
	{
	    fprintf (stderr, "Error flushing channel\t%s\n",
		     sys_errlist[errno]);
	    conn_close (connection);
	    continue;
	}
    }
    return (TRUE);
}   /*  End Function process_context_attributes  */

static void initialise_shader_aa ()
/*  [PURPOSE] This routine will initialise the associative array for the shader
    list.
    [RETURNS] Nothing.
*/
{
    extern KAssociativeArray shaders;
    static char function_name[] = "__vrender_initialise_shader_aa";

    if (shaders != NULL) return;
    shaders = aa_create (NULL, strcmp, key_copy_func, m_free,
			 ( void *(*) () ) NULL,
			 shader_destroy_func);
    if (shaders == NULL) m_abort (function_name, "shader list");
}   /*  End Function initialise_shader_aa   */

static void *key_copy_func (CONST char *key, uaddr length, flag *ok)
/*  [PURPOSE] This routine will copy a shader name.
    <key> The name.
    <length> Ignored.
    <ok> The value TRUE will be written here on success, else FALSE is written
    here.
    [RETURNS] The new shader on success, else NULL.
*/
{
    char *new;
    static char function_name[] = "__vrender_key_copy_func";

    if ( ( new = st_dup (key) ) == NULL )
    {
	m_error_notify (function_name, "shader name");
	*ok = FALSE;
    }
    else *ok = TRUE;
    return (new);
}   /*  End Function key_copy_func  */

static void shader_destroy_func (Shader shader)
/*  [PURPOSE] This routine will destroy a shader.
    <shader> The shader.
    [RETURNS] Nothing.
*/
{
    ds_dealloc_packet (shader->pack_desc, NULL);
    m_free ( (char *) shader );
}   /*  End Function shader_destroy_func  */

static void compute_output_image_desc (KVolumeRenderContext context)
/*  [PURPOSE] This routine will compute the output image array descriptor for a
    particular cube.
    <context> The volume rendering context. This is updated.
    [RETURNS] Nothing.
*/
{
    flag compute_offsets = FALSE;
    unsigned long tmp, ul_extent, length;
    double min, max, extent;
    static char function_name[] = "__vrender_compute_output_image_desc";

    VERIFY_CONTEXT (context);
    if (context->cube == NULL) return;
    if (context->shader == NULL) return;
    if (context->valid_image_desc) return;
    /*  Start computing extent (size) of cube  */
    tmp = context->subcube_x_end - context->subcube_x_start + 1;
    ul_extent = tmp * tmp;
    tmp = context->subcube_y_end - context->subcube_y_start + 1;
    ul_extent += tmp * tmp;
    tmp = context->subcube_z_end - context->subcube_z_start + 1;
    ul_extent += tmp * tmp;
    extent = sqrt ( (double) ul_extent );
    min = floor (-extent / 2.0);
    max = ceil (extent / 2.0);
    extent = max - min;
    /*  Compute horizontal and vertical lengths  */
    length = (unsigned long) extent + 1;
    if (context->arr_desc == NULL) compute_offsets = TRUE;
    else if (context->arr_desc->packet == NULL)
    {
	context->arr_desc->packet = context->shader->pack_desc;
	compute_offsets = TRUE;
    }
    else if (context->arr_desc->offsets == NULL) compute_offsets = TRUE;
    else
    {
	if (ds_get_packet_size (context->arr_desc->packet) !=
	    context->shader->packet_size) compute_offsets = TRUE;
	if (length != context->h_dim.length)
	{
	    /*  Deallocate the offset arrays since the lengths have changed  */
	    m_free ( (char *) context->arr_desc->offsets[0] );
	    m_free ( (char *) context->arr_desc->offsets[1] );
	    m_free ( (char *) context->arr_desc->offsets );
	    context->arr_desc->offsets = NULL;
	}
    }
    if ( (context->h_dim.length != length) ||
	(context->v_dim.length != length) )
    {
	context->cyclops.num_lines_computed = 0;
	context->cyclops.num_reordered_lines = 0;
	context->cyclops.num_setup_lines = 0;
	context->left.num_lines_computed = 0;
	context->left.num_reordered_lines = 0;
	context->left.num_setup_lines = 0;
	context->right.num_lines_computed = 0;
	context->right.num_reordered_lines = 0;
	context->right.num_setup_lines = 0;
	/*  Register worker if needed  */
	if ( context->valid_view_info_cache && (context->worker == NULL) )
	{
/*
	    context->worker = wf_register_func (worker_function, context,
						KWF_PRIORITY_HIGHEST);
*/
	}
    }	
    context->h_dim.length = length;
    context->v_dim.length = length;
    /*  Compute minima and maxima  */
    context->h_dim.first_coord = min;
    context->h_dim.last_coord = max;
    context->h_dim.minimum = context->h_dim.first_coord;
    context->h_dim.maximum = context->h_dim.last_coord;
    context->v_dim.first_coord = min;
    context->v_dim.last_coord = max;
    context->v_dim.minimum = context->v_dim.first_coord;
    context->v_dim.maximum = context->v_dim.last_coord;
    context->h_dim.name = HORIZONTAL_DIMENSION_NAME;
    context->v_dim.name = VERTICAL_DIMENSION_NAME;
    if (context->arr_desc == NULL)
    {
	if ( ( context->arr_desc = ds_alloc_array_desc (2, 0) ) == NULL )
	{
	    m_abort (function_name, "image array descriptor");
	}
    }
    context->arr_desc->dimensions[0] = &context->v_dim;
    context->arr_desc->dimensions[1] = &context->h_dim;
    context->arr_desc->lengths[0] = context->v_dim.length;
    context->arr_desc->lengths[1] = context->h_dim.length;
    context->arr_desc->packet = context->shader->pack_desc;
    if (compute_offsets)
    {
	if ( !ds_compute_array_offsets (context->arr_desc) )
	{
	    m_abort (function_name, "offsets arrays");
	}
    }
    if (context->job_data != NULL)
    {
	m_free ( (char *) context->job_data );
    }
    if ( ( context->job_data = (job_info *)
	  m_alloc (sizeof *context->job_data * context->v_dim.length) )
	== NULL )
    {
	m_abort (function_name, "job structures");
    }
    context->valid_image_desc = TRUE;
    c_call_callbacks (context->image_desc_notify_list, NULL);
}   /*  End Function compute_output_image_desc  */

static void compute_view_info_cache (KVolumeRenderContext context)
/*  [PURPOSE] This routine will compute the view information cache for a volume
    rendering context.
    <context> The volume rendering context. This is updated.
    [RETURNS] Nothing.
*/
{
    float mag, factor;
    Kcoord_3d direction, horizontal;
    eye_info *eye;
    static char function_name[] = "__vrender_compute_view_info_cache";

    VERIFY_CONTEXT (context);
    if (context->cube == NULL) return;
    if (context->valid_view_info_cache) return;
    compute_output_image_desc (context);
    if (!context->valid_image_desc)
    {
	fprintf (stderr, "No valid image descriptor\n");
	a_prog_bug (function_name);
    }
    /*  Normalise vertical vector  */
    mag = sqrt (context->view.vertical.x * context->view.vertical.x +
		context->view.vertical.y * context->view.vertical.y +
		context->view.vertical.z * context->view.vertical.z);
    if (mag != 1.0)
    {
	context->view.vertical.x /= mag;
	context->view.vertical.y /= mag;
	context->view.vertical.z /= mag;
    }
    /*  Compute vision direction vector  */
    direction.x = context->view.focus.x - context->view.position.x;
    direction.y = context->view.focus.y - context->view.position.y;
    direction.z = context->view.focus.z - context->view.position.z;
    /*  Normalise direction vector  */
    mag = sqrt (direction.x * direction.x + direction.y * direction.y +
		direction.z * direction.z);
    if (mag != 1.0)
    {
	direction.x /= mag;
	direction.y /= mag;
	direction.z /= mag;
    }
    /*  Compute horizontal vector  */
    geom_vector_multiply (&horizontal, direction, context->view.vertical);
    /*  Normalise horizontal vector  */
    mag = sqrt (horizontal.x * horizontal.x + horizontal.y * horizontal.y +
		horizontal.z * horizontal.z);
    if (mag != 1.0)
    {
	horizontal.x /= mag;
	horizontal.y /= mag;
	horizontal.z /= mag;
    }
    /*  Mono eye (cyclops)  */
    eye = &context->cyclops;
    eye->orig_position = context->view.position;
    compute_eye_info_cache (&context->cyclops, FALSE);
    factor = context->eye_separation / 2.0;
    /*  Left eye  */
    eye = &context->left;
    eye->orig_position.x = context->view.position.x - horizontal.x * factor;
    eye->orig_position.y = context->view.position.y - horizontal.y * factor;
    eye->orig_position.z = context->view.position.z - horizontal.z * factor;
    compute_eye_info_cache (&context->left, context->never_did_stereo);
    /*  Right eye  */
    eye = &context->right;
    eye->orig_position.x = context->view.position.x + horizontal.x * factor;
    eye->orig_position.y = context->view.position.y + horizontal.y * factor;
    eye->orig_position.z = context->view.position.z + horizontal.z * factor;
    compute_eye_info_cache (&context->right, context->never_did_stereo);
    context->valid_view_info_cache = TRUE;
    c_call_callbacks (context->view_notify_list, NULL);
    /*  No point computing lines or re-order buffer if work procedures not
	supported.  */
    if ( !wf_test_supported () ) return;
    /*  Use existing worker function if possible  */
    if (context->worker == NULL)
    {
	context->worker = wf_register_func (worker_function, context,
					    KWF_PRIORITY_HIGHEST);
    }
}   /*  End Function compute_view_info_cache  */

static void compute_eye_info_cache (eye_info *eye, flag no_alloc)
/*  [PURPOSE] This routine will compute the view information cache for a volume
    rendering context.
    <eye> The eye information. This is updated. The position must already be
    valid.
    <no_alloc> If TRUE, the routine will not allocate the reorder buffer.
    [RETURNS] Nothing.
*/
{
    KVolumeRenderContext context = eye->context;
    uaddr cube_size, plane_size;
    int depth_dim_index = 0;  /*  Initialised to keep compiler happy  */
    unsigned int i_tmp;
    unsigned int y_coord;
    unsigned int step = 0;    /*  Initialised to keep compiler happy  */
    float mag, f_tmp, t, half_cube_size;
    RotatedKcoord_3d cube_centre;
    static char function_name[] = "__vrender_compute_eye_info_cache";

    VERIFY_CONTEXT (context);
    if (context->cube == NULL) return;
    if (context->valid_view_info_cache) return;
    eye->orig_direction.x = context->view.focus.x - eye->orig_position.x;
    eye->orig_direction.y = context->view.focus.y - eye->orig_position.y;
    eye->orig_direction.z = context->view.focus.z - eye->orig_position.z;
    /*  Normalise direction vector  */
    mag = sqrt (eye->orig_direction.x * eye->orig_direction.x +
		eye->orig_direction.y * eye->orig_direction.y +
		eye->orig_direction.z * eye->orig_direction.z);
    if (mag != 1.0)
    {
	eye->orig_direction.x /= mag;
	eye->orig_direction.y /= mag;
	eye->orig_direction.z /= mag;
    }
    /*  Compute horizontal vector  */
    geom_vector_multiply (&eye->orig_horizontal, eye->orig_direction,
			  context->view.vertical);
    /*  Normalise horizontal vector  */
    mag = sqrt (eye->orig_horizontal.x * eye->orig_horizontal.x +
		eye->orig_horizontal.y * eye->orig_horizontal.y +
		eye->orig_horizontal.z * eye->orig_horizontal.z);
    if (mag != 1.0)
    {
	eye->orig_horizontal.x /= mag;
	eye->orig_horizontal.y /= mag;
	eye->orig_horizontal.z /= mag;
    }
    /*  Determine which dimension is most nearly aligned with the direction the
	observer is looking.
	*/
    if ( ( fabs (eye->orig_direction.z) >= fabs (eye->orig_direction.y) ) &&
	( fabs (eye->orig_direction.z) >= fabs (eye->orig_direction.x) ) )
    {
	/*  Dimension 0 (Z) is the one to step through  */
	depth_dim_index = 0;
	step = STEP_Z;
	eye->h_offsets = context->cube->offsets[2];
	eye->v_offsets = context->cube->offsets[1];
	eye->d_offsets = context->cube->offsets[0];
	eye->rot_subcube_start.h = context->subcube_x_start;
	eye->rot_subcube_end.h = context->subcube_x_end;
	eye->rot_subcube_start.v = context->subcube_y_start;
	eye->rot_subcube_end.v = context->subcube_y_end;
	eye->rot_subcube_start.d = context->subcube_z_start;
	eye->rot_subcube_end.d = context->subcube_z_end;
    }
    if ( ( fabs (eye->orig_direction.y) >= fabs (eye->orig_direction.x) ) &&
	( fabs (eye->orig_direction.y) >= fabs (eye->orig_direction.z) ) )
    {
	/*  Dimension 1 (Y) is the one to step through  */
	depth_dim_index = 1;
	step = STEP_Y;
	eye->h_offsets = context->cube->offsets[0];
	eye->v_offsets = context->cube->offsets[2];
	eye->d_offsets = context->cube->offsets[1];
	eye->rot_subcube_start.h = context->subcube_z_start;
	eye->rot_subcube_end.h = context->subcube_z_end;
	eye->rot_subcube_start.v = context->subcube_x_start;
	eye->rot_subcube_end.v = context->subcube_x_end;
	eye->rot_subcube_start.d = context->subcube_y_start;
	eye->rot_subcube_end.d = context->subcube_y_end;
    }
    if ( ( fabs (eye->orig_direction.x) >= fabs (eye->orig_direction.y) ) &&
	( fabs (eye->orig_direction.x) >= fabs (eye->orig_direction.z) ) )
    {
	/*  Dimension 2 (X: least significant) is the one to step through  */
	depth_dim_index = 2;
	step = STEP_X;
	eye->h_offsets = context->cube->offsets[0];
	eye->v_offsets = context->cube->offsets[1];
	eye->d_offsets = context->cube->offsets[2];
	eye->rot_subcube_start.h = context->subcube_z_start;
	eye->rot_subcube_end.h = context->subcube_z_end;
	eye->rot_subcube_start.v = context->subcube_y_start;
	eye->rot_subcube_end.v = context->subcube_y_end;
	eye->rot_subcube_start.d = context->subcube_x_start;
	eye->rot_subcube_end.d = context->subcube_x_end;
    }
    rotate_3d (&eye->rot_position, eye->orig_position, step);
    rotate_3d (&eye->rot_focus, context->view.focus, step);
    rotate_3d (&eye->rot_vertical, context->view.vertical, step);
    rotate_3d (&eye->rot_direction, eye->orig_direction, step);
    rotate_3d (&eye->rot_horizontal, eye->orig_horizontal, step);
    cube_centre.h = (eye->rot_subcube_start.h + eye->rot_subcube_end.h) / 2.0;
    cube_centre.v = (eye->rot_subcube_start.v + eye->rot_subcube_end.v) / 2.0;
    cube_centre.d = (eye->rot_subcube_start.d + eye->rot_subcube_end.d) / 2.0;
    /*  Compute array of plane pointers  */
    if (context->cube->lengths[depth_dim_index] > eye->num_planes_allocated)
    {
	if (eye->planes != NULL) m_free ( (char *) eye->planes );
	eye->num_planes_allocated = context->cube->lengths[depth_dim_index];
	if ( ( eye->planes = (signed char **)
	      m_alloc (sizeof *eye->planes *
		       eye->num_planes_allocated) ) ==
	    NULL )
	{
	    m_abort (function_name, "array of plane pointers");
	}
    }
    /*  Compute base addresses for each plane  */
    for (i_tmp = 0; i_tmp < context->cube->lengths[depth_dim_index]; ++i_tmp)
    {
	eye->planes[i_tmp] = (signed char *) context->cube->data + eye->d_offsets[i_tmp];
    }
    /*  Next, find half size of cube  */
    f_tmp = eye->rot_subcube_start.h - eye->rot_subcube_end.h;
    half_cube_size = f_tmp * f_tmp;
    f_tmp = eye->rot_subcube_start.v - eye->rot_subcube_end.v;
    half_cube_size += f_tmp * f_tmp;
    f_tmp = eye->rot_subcube_start.d - eye->rot_subcube_end.d;
    half_cube_size += f_tmp * f_tmp;
    half_cube_size = sqrt (half_cube_size) / 2.0;
    /*  Compute centre of raster plane  */
    /*  NOTE: co-ordinate 0.0, 0.0 (x, y) is center of image plane  */
    /*  LEFT  */
    /*  Find distance from eye to centre of cube  */
    f_tmp = cube_centre.h - eye->rot_position.h;
    mag = f_tmp * f_tmp;
    f_tmp = cube_centre.v - eye->rot_position.v;
    mag += f_tmp * f_tmp;
    f_tmp = cube_centre.d - eye->rot_position.d;
    mag += f_tmp * f_tmp;
    mag = sqrt (mag);
    t = 0.99 * (mag - half_cube_size);  /*  Distance from eye to start of cube * 0.99  */
    eye->orig_ras_plane_centre.x = eye->orig_position.x + t * eye->orig_direction.x;
    eye->orig_ras_plane_centre.y = eye->orig_position.y + t * eye->orig_direction.y;
    eye->orig_ras_plane_centre.z = eye->orig_position.z + t * eye->orig_direction.z;
    eye->rot_ras_plane_centre.h = eye->rot_position.h + t * eye->rot_direction.h;
    eye->rot_ras_plane_centre.v = eye->rot_position.v + t * eye->rot_direction.v;
    eye->rot_ras_plane_centre.d = eye->rot_position.d + t * eye->rot_direction.d;
    /*  Compute line start and stop co-ordinates. This avoids having to project
	rays from the view plane just to determine if they intersect the volume
	*/
    if (eye->num_lines_allocated < context->v_dim.length)
    {
	if (eye->lines != NULL) m_free ( (char *) eye->lines );
	if ( ( eye->lines = (struct line_type *)
	      m_alloc (sizeof *eye->lines * context->v_dim.length) ) ==
	    NULL )
	{
	    m_abort (function_name, "array of line structures");
	}
	eye->num_lines_allocated = context->v_dim.length;
/*
	fprintf (stderr, "Allocated: %lu bytes for line cache\n",
			sizeof *eye->lines * context->v_dim.length);
*/
    }
    /*  Invalidate line cache  */
    eye->num_lines_computed = 0;
    for (y_coord = 0; y_coord < context->v_dim.length; ++y_coord)
    {
	/*  Default is entire line sees cube.  */
	eye->lines[y_coord].start = 0;
	eye->lines[y_coord].stop = context->h_dim.length;
    }
    /*  Ensure enough space for plane information  */
    plane_size = context->h_dim.length * context->v_dim.length;
    if (eye->reorder_plane_size < plane_size)
    {
	if (eye->reorder_rays != NULL) m_free ( (char *) eye->reorder_rays );
	if ( ( eye->reorder_rays = (ray_data *)
	      m_alloc (sizeof *eye->reorder_rays * plane_size) ) == NULL )
	{
	    m_abort (function_name, "array of ray information");
	}
	eye->reorder_plane_size = plane_size;
/*
	fprintf (stderr, "Allocated: %lu bytes for plane of rays\n",
			sizeof *eye->reorder_rays * plane_size);
*/
	m_clear ( (char *) eye->reorder_rays,
		 sizeof *eye->reorder_rays * plane_size );
    }
    /*  Invalidate reorder cache  */
    eye->num_reordered_lines = 0;
    eye->num_setup_lines = 0;
    /*  Ensure enough space is allocated for re-ordered cube  */
    cube_size = iarray_dim_length (context->cube, 0);
    cube_size *= iarray_dim_length (context->cube, 1);
    cube_size *= iarray_dim_length (context->cube, 2);
    if (eye->reorder_buf_size < cube_size)
    {
	if (eye->reorder_buffer != NULL) m_free ( (char *)eye->reorder_buffer);
	if (no_alloc)
	{
	    /*  Do not allocate buffer at this time: set to NULL to indicate
		to workers that it should be computed later  */
	    eye->reorder_buffer = NULL;
	    eye->next_block = NULL;
	    return;
	}
	/*  Now is the time to allocate the reorder buffer  */
	if ( ( eye->reorder_buffer = (signed char *) m_alloc (cube_size) )
	    == NULL )
	{
	    m_abort (function_name, "reorder buffer");
	}
	eye->reorder_buf_size = cube_size;
	fprintf (stderr, "Allocated: %lu bytes for reorder buffer\n",
		 cube_size);
    }
    eye->next_block = eye->reorder_buffer;
}   /*  End Function compute_eye_info_cache  */

static void rotate_3d (RotatedKcoord_3d *rot, Kcoord_3d orig,unsigned int step)
/*  [PURPOSE] This routine will rotate a 3-dimensional co-ordinate from X,Y,Z
    space to H,V,D space.
    <rot> The rotated co-ordinate is written here.
    <orig> The original co-ordinate.
    <step> The step dimension.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "rotate_3d";

    switch (step)
    {
      case STEP_X:
	rot->h = orig.z;
	rot->v = orig.y;
	rot->d = orig.x;
	break;
      case STEP_Y:
	rot->h = orig.z;
	rot->v = orig.x;
	rot->d = orig.y;
	break;
      case STEP_Z:
	rot->h = orig.x;
	rot->v = orig.y;
	rot->d = orig.z;
	break;
      default:
	fprintf (stderr, "Illegal step code: %u\n", step);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function rotate_3d  */

static void generate_line (void *pool_info,
			   void *call_info1, void *call_info2,
			   void *call_info3, void *call_info4,
			   void *thread_info)
/*  [PURPOSE] The function generates one line.
    <info> The job info.
    [RETURNS] Nothing.
*/
{
    job_info *info = (job_info *) call_info1;
    KVolumeRenderContext context;
    RotatedKcoord_3d vo, ray_direction, one_on_ray_dir, ray_start;
    flag stereo;
    unsigned int x_coord, y_coord;
    float x, y;
    float min_d, max_d;
    float x_min, x_scale, y_min, y_scale;
    float t_enter;
    Shader shader;
    eye_info *eye = NULL;
    ray_data *curr_ray;
    char *left_image = info->left_line;
    char *right_image = info->right_line;
    /*static char function_name[] = "__vrender_generate_line";*/

    context = info->context;
    info->min = TOOBIG;
    info->max = -TOOBIG;
    x_min = context->h_dim.first_coord;
    x_scale = context->h_dim.last_coord - x_min;
    x_scale /= (float) (context->h_dim.length - 1);
    y_min = context->v_dim.first_coord;
    y_scale = context->v_dim.last_coord - y_min;
    y_scale /= (float) (context->v_dim.length - 1);
    stereo = (info->right_line == NULL) ? FALSE : TRUE;
    shader = context->shader;
    for (y_coord = info->start_y; y_coord < info->stop_y; ++y_coord)
    {
	y = y_min + (float) y_coord * y_scale;
	/*  LEFT  */
	eye = stereo ? &context->left : &context->cyclops;
	/*  First write blanks up to the start pixel  */
	for (x_coord = 0; x_coord < eye->lines[y_coord].start;
	     ++x_coord, left_image += context->shader->packet_size)
	{
	    m_copy (left_image, context->shader->blank_packet,
		    context->shader->packet_size);
	}
	if ( (y_coord < eye->num_reordered_lines) &&
	     (shader->fast_func != NULL) )
	{
	    /*  YES! We can use the re-ordered cube  */
	    curr_ray = eye->reorder_rays + y_coord * context->h_dim.length + x_coord;
	    for (; x_coord < eye->lines[y_coord].stop;
		 ++x_coord, left_image += context->shader->packet_size,
		 ++curr_ray)
	    {
		(*shader->fast_func) (curr_ray->ray, curr_ray->length,
				      &info->min, &info->max,
				      (void *) left_image);
	    }
	}
	else
	{
	    /*  Oh, well, we have to do things the slow way  */
	    vo.h = y * eye->rot_vertical.h;
	    vo.v = y * eye->rot_vertical.v;
	    vo.d = y * eye->rot_vertical.d;
	    if (context->projection == VRENDER_PROJECTION_PARALLEL)
	    {
		/*  For parallel projection need to compute ray direction
		    only once.  */
		ray_direction.h = eye->rot_direction.h;
		ray_direction.v = eye->rot_direction.v;
		ray_direction.d = eye->rot_direction.d;
		one_on_ray_dir.h = (ray_direction.h ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.h;
		one_on_ray_dir.v = (ray_direction.v ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.v;
		one_on_ray_dir.d = (ray_direction.d ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.d;
	    }
	}
	/*  Process all (unprocessed) pixels in this row. Note how x_coord is
	    remembered from last loop.  */
	for (; x_coord < eye->lines[y_coord].stop;
	     ++x_coord, left_image += context->shader->packet_size)
	{
	    /*  Raycast this image pixel  */
	    x = x_min + (float) x_coord * x_scale;
	    /*  Determine point in 3D space which corresponds to this pixel  */
	    ray_start.h = eye->rot_ras_plane_centre.h + x*eye->rot_horizontal.h + vo.h;
	    ray_start.v = eye->rot_ras_plane_centre.v + x*eye->rot_horizontal.v + vo.v;
	    ray_start.d = eye->rot_ras_plane_centre.d + x*eye->rot_horizontal.d + vo.d;
	    if (context->projection == VRENDER_PROJECTION_PERSPECTIVE)
	    {
		/*  For perspective projection need to compute ray direction
		    for each point.
		    */
		ray_direction.h = ray_start.h - eye->rot_position.h;
		ray_direction.v = ray_start.v - eye->rot_position.v;
		ray_direction.d = ray_start.d - eye->rot_position.d;
		one_on_ray_dir.h = (ray_direction.h ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.h;
		one_on_ray_dir.v = (ray_direction.v ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.v;
		one_on_ray_dir.d = (ray_direction.d ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.d;
	    }
	    /*  WARNING: at this point,  ray_direction  is not normalised  */
	    if ( !get_ray_intersections_with_cube (&ray_start, &ray_direction,
						   &one_on_ray_dir,
						   &eye->rot_subcube_start,
						   &eye->rot_subcube_end,
						   &min_d, &max_d,
						   &t_enter, NULL) )
	    {
		m_copy (left_image, context->shader->blank_packet,
			context->shader->packet_size);
/*
		fprintf (stderr, "y: %u  start: %u  stop: %u\n",
				y_coord,
				eye->lines[y_coord].start,
				eye->lines[y_coord].stop);
		fprintf (stderr, "pixel: %u,%u cannot see volume\n",
				x_coord, y_coord);
*/
		continue;
	    }
	    min_d = ceil (min_d);
	    max_d = floor (max_d);
	    if (min_d >= max_d)
	    {
		m_copy (left_image, context->shader->blank_packet,
			context->shader->packet_size);
		continue;
	    }
	    (*shader->slow_func) (eye->planes, eye->v_offsets, eye->h_offsets,
				  ray_start.d, ray_start.v, ray_start.h,
				  ray_direction.d, ray_direction.v,
				  ray_direction.h,
				  one_on_ray_dir.d,
				  min_d, max_d,
				  &info->min, &info->max, (void *) left_image,
				  eye->rot_direction,
				  eye->rot_ras_plane_centre, t_enter);
	}
	/*  Write blanks past stop pixel  */
	for (; x_coord < context->h_dim.length;
	     ++x_coord, left_image += context->shader->packet_size)
	{
	    m_copy (left_image, context->shader->blank_packet,
		    context->shader->packet_size);
	}
	/*  RIGHT  */
	if (!stereo) continue;
	eye = &context->right;
	/*  First write blanks up to the start pixel  */
	for (x_coord = 0; x_coord < eye->lines[y_coord].start;
	     ++x_coord, right_image += context->shader->packet_size)
	{
	    m_copy (right_image, context->shader->blank_packet,
		    context->shader->packet_size);
	}
	if ( (y_coord < eye->num_reordered_lines) &&
	     (shader->fast_func != NULL) )
	{
	    /*  YES! We can use the re-ordered cube  */
	    curr_ray = eye->reorder_rays + y_coord * context->h_dim.length + x_coord;
	    for (; x_coord < eye->lines[y_coord].stop;
		 ++x_coord, right_image += context->shader->packet_size,
		 ++curr_ray)
	    {
		(*shader->fast_func) (curr_ray->ray, curr_ray->length,
				      &info->min, &info->max,
				      (void *) right_image);
	    }
	}
	else
	{
	    /*  Oh, well, we have to do things the slow way  */
	    vo.h = y * eye->rot_vertical.h;
	    vo.v = y * eye->rot_vertical.v;
	    vo.d = y * eye->rot_vertical.d;
	    if (context->projection == VRENDER_PROJECTION_PARALLEL)
	    {
		/*  For parallel projection need to compute ray direction
		    only once.  */
		ray_direction.h = eye->rot_direction.h;
		ray_direction.v = eye->rot_direction.v;
		ray_direction.d = eye->rot_direction.d;
		one_on_ray_dir.h = (ray_direction.h ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.h;
		one_on_ray_dir.v = (ray_direction.v ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.v;
		one_on_ray_dir.d = (ray_direction.d ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.d;
	    }
	}
	/*  Process all (unprocessed) pixels in this row. Note how x_coord is
	    remembered from last loop.  */
	for (; x_coord < eye->lines[y_coord].stop;
	     ++x_coord, right_image += context->shader->packet_size)
	{
	    /*  Raycast this image pixel  */
	    x = x_min + (float) x_coord * x_scale;
	    /*  Determine point in 3D space which corresponds to this pixel  */
	    ray_start.h = eye->rot_ras_plane_centre.h + x*eye->rot_horizontal.h + vo.h;
	    ray_start.v = eye->rot_ras_plane_centre.v + x*eye->rot_horizontal.v + vo.v;
	    ray_start.d = eye->rot_ras_plane_centre.d + x*eye->rot_horizontal.d + vo.d;
	    if (context->projection == VRENDER_PROJECTION_PERSPECTIVE)
	    {
		/*  For perspective projection need to compute ray direction
		    for each point.
		    */
		ray_direction.h = ray_start.h - eye->rot_position.h;
		ray_direction.v = ray_start.v - eye->rot_position.v;
		ray_direction.d = ray_start.d - eye->rot_position.d;
		one_on_ray_dir.h = (ray_direction.h ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.h;
		one_on_ray_dir.v = (ray_direction.v ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.v;
		one_on_ray_dir.d = (ray_direction.d ==
				    0.0) ? TOOBIG : 1.0 / ray_direction.d;
	    }
	    /*  WARNING: at this point,  ray_direction  is not normalised  */
	    if ( !get_ray_intersections_with_cube (&ray_start, &ray_direction,
						   &one_on_ray_dir,
						   &eye->rot_subcube_start,
						   &eye->rot_subcube_end,
						   &min_d, &max_d,
						   &t_enter, NULL) )
	    {
		m_copy (right_image, context->shader->blank_packet,
			context->shader->packet_size);
		continue;
	    }
	    min_d = ceil (min_d);
	    max_d = floor (max_d);
	    if (min_d >= max_d)
	    {
		m_copy (right_image, context->shader->blank_packet,
			context->shader->packet_size);
		continue;
	    }
	    (*shader->slow_func) (eye->planes, eye->v_offsets, eye->h_offsets,
				  ray_start.d, ray_start.v, ray_start.h,
				  ray_direction.d, ray_direction.v,
				  ray_direction.h,
				  one_on_ray_dir.d,
				  min_d, max_d,
				  &info->min, &info->max,
				  (void *) right_image,
				  eye->rot_direction,
				  eye->rot_ras_plane_centre, t_enter);
	}
	/*  Write blanks past stop pixel  */
	for (; x_coord < context->h_dim.length;
	     ++x_coord, right_image += context->shader->packet_size)
	{
	    m_copy (right_image, context->shader->blank_packet,
		    context->shader->packet_size);
	}
    }
}   /*  End Function generate_line  */

static flag get_ray_intersections_with_cube(RotatedKcoord_3d *position,
					    RotatedKcoord_3d *direction,
					    RotatedKcoord_3d *one_on_direction,
					    RotatedKcoord_3d *subcube_start,
					    RotatedKcoord_3d *subcube_end,
					    float *min_d, float *max_d,
					    float *enter, float *leave)
/*  [PURPOSE] This routine will compute the intersections a ray makes with a
    cube.
    <position> The starting position of the ray.
    <direction> The direction of the ray.
    <one_on_direction> The inverse of the direction ray.
    <subcube_start> The start of the subcube.
    <subcube_end> The end of the subcube.
    <min_d> The minimum plane intersection is written here.
    <max_d> The maximum plane intersection is written here.
    <enter> The position along the ray where it enters the cube will be
    written here.
    <leave> The position along the ray where it leaves the cube will be
    written here.
    [RETURNS] TRUE if the ray intersects the cube, else FALSE.
*/
{
    float h, v, d, t;
    float min = TOOBIG;
    float max = -TOOBIG;
    float t_enter = TOOBIG;
    float t_leave = -TOOBIG;
    float zero = 0.0;
    float half = 0.5;

    /*  Check each face  */
    if (direction->d != zero)
    {
	/*  Face d = min  */
	d = subcube_start->d;
	t = (d - position->d) * one_on_direction->d;
	if (t >= zero)
	{
	    h = position->h + t * direction->h;
	    v = position->v + t * direction->v;
	    if ( (h >= subcube_start->h) && (h <= subcube_end->h) &&
		 (v >= subcube_start->v) && (v <= subcube_end->v) )
	    {
		if (d < min) min = d;
		if (d > max) max = d;
		if (t < t_enter) t_enter = t;
		if (t > t_leave) t_leave = t;
	    }
#define dummy  /*  Undefine only for timing tests.  */
#ifdef dummy
	    else
	    {
		/*  No intersection at d = min, but perhaps at d = min + 0.5 */
		d = subcube_start->d + half;
		t = (d - position->d) * one_on_direction->d;
		if (t >= zero)
		{
		    h = position->h + t * direction->h;
		    v = position->v + t * direction->v;
		    if ( (h >= subcube_start->h) && (h <= subcube_end->h) &&
			(v >= subcube_start->v) && (v <= subcube_end->v) )
		    {
			if (d < min) min = d;
			if (d > max) max = d;
			if (t < t_enter) t_enter = t;
			if (t > t_leave) t_leave = t;
		    }
		}
	    }
#endif
	}
	/*  Face d = max  */
	d = subcube_end->d;
	t = (d - position->d) * one_on_direction->d;
	if (t >= zero)
	{
	    h = position->h + t * direction->h;
	    v = position->v + t * direction->v;
	    if ( (h >= subcube_start->h) && (h <= subcube_end->h) &&
		(v >= subcube_start->v) && (v <= subcube_end->v) )
	    {
		if (d < min) min = d;
		if (d > max) max = d;
		if (t < t_enter) t_enter = t;
		if (t > t_leave) t_leave = t;
	    }
#ifdef dummy
	    else
	    {
		/*  No intersection at d = max, but perhaps at d = max - 0.5 */
		d = subcube_end->d - half;
		t = (d - position->d) * one_on_direction->d;
		if (t >= zero)
		{
		    h = position->h + t * direction->h;
		    v = position->v + t * direction->v;
		    if ( (h >= subcube_start->h) && (h <= subcube_end->h) &&
			(v >= subcube_start->v) && (v <= subcube_end->v) )
		    {
			if (d < min) min = d;
			if (d > max) max = d;
			if (t < t_enter) t_enter = t;
			if (t > t_leave) t_leave = t;
		    }
		}
	    }
#endif
	}
    }
    if (direction->v != zero)
    {
	/*  Face v = min  */
	v = subcube_start->v;
	t = (v - position->v) * one_on_direction->v;
	if (t >= zero)
	{
	    h = position->h + t * direction->h;
	    d = position->d + t * direction->d;
	    if ( (h >= subcube_start->h) && (h <= subcube_end->h) &&
		(d >= subcube_start->d) && (d <= subcube_end->d) )
	    {
		if (d < min) min = d;
		if (d > max) max = d;
		if (t < t_enter) t_enter = t;
		if (t > t_leave) t_leave = t;
	    }
#ifdef dummy
	    else
	    {
		/*  No intersection at v = min, but perhaps at v = min + 0.5 */
		v = subcube_start->v + half;
		t = (v - position->v) * one_on_direction->v;
		if (t >= zero)
		{
		    h = position->h + t * direction->h;
		    d = position->d + t * direction->d;
		    if ( (h >= subcube_start->h) && (h <= subcube_end->h) &&
			(d >= subcube_start->d) && (d <= subcube_end->d) )
		    {
			if (d < min) min = d;
			if (d > max) max = d;
			if (t < t_enter) t_enter = t;
			if (t > t_leave) t_leave = t;
		    }
		}
	    }
#endif
	}
	/*  Face v = max  */
	v = subcube_end->v;
	t = (v - position->v) * one_on_direction->v;
	if (t >= zero)
	{
	    h = position->h + t * direction->h;
	    d = position->d + t * direction->d;
	    if ( (h >= subcube_start->h) && (h <= subcube_end->h) &&
		(d >= subcube_start->d) && (d <= subcube_end->d) )
	    {
		if (d < min) min = d;
		if (d > max) max = d;
		if (t < t_enter) t_enter = t;
		if (t > t_leave) t_leave = t;
	    }
#ifdef dummy
	    else
	    {
		/*  No intersection at v = max, but perhaps at v = max - 0.5 */
		v = subcube_end->v - half;
		t = (v - position->v) * one_on_direction->v;
		if (t >= zero)
		{
		    h = position->h + t * direction->h;
		    d = position->d + t * direction->d;
		    if ( (h >= subcube_start->h) && (h <= subcube_end->h) &&
			(d >= subcube_start->d) && (d <= subcube_end->d) )
		    {
			if (d < min) min = d;
			if (d > max) max = d;
			if (t < t_enter) t_enter = t;
			if (t > t_leave) t_leave = t;
		    }
		}
	    }
#endif
	}
    }
    if (direction->h != zero)
    {
	/*  Face h = min  */
	h = subcube_start->h;
	t = (h - position->h) * one_on_direction->h;
	if (t >= zero)
	{
	    d = position->d + t * direction->d;
	    v = position->v + t * direction->v;
	    if ( (d >= subcube_start->d) && (d <= subcube_end->d) &&
		(v >= subcube_start->v) && (v <= subcube_end->v) )
	    {
		if (d < min) min = d;
		if (d > max) max = d;
		if (t < t_enter) t_enter = t;
		if (t > t_leave) t_leave = t;
	    }
#ifdef dummy
	    else
	    {
		/*  No intersection at h = min, but perhaps at h = min + 0.5 */
		h = subcube_start->h + half;
		t = (h - position->h) * one_on_direction->h;
		if (t >= zero)
		{
		    d = position->d + t * direction->d;
		    v = position->v + t * direction->v;
		    if ( (d >= subcube_start->d) && (d <= subcube_end->d) &&
			(v >= subcube_start->v) && (v <= subcube_end->v) )
		    {
			if (d < min) min = d;
			if (d > max) max = d;
			if (t < t_enter) t_enter = t;
			if (t > t_leave) t_leave = t;
		    }
		}
	    }
#endif
	}
	/*  Face h = max  */
	h = subcube_end->h;
	t = (h - position->h) * one_on_direction->h;
	if (t >= zero)
	{
	    d = position->d + t * direction->d;
	    v = position->v + t * direction->v;
	    if ( (d >= subcube_start->d) && (d <= subcube_end->d) &&
		(v >= subcube_start->v) && (v <= subcube_end->v) )
	    {
		if (d < min) min = d;
		if (d > max) max = d;
		if (t < t_enter) t_enter = t;
		if (t > t_leave) t_leave = t;
	    }
#ifdef dummy
	    else
	    {
		/*  No intersection at h = max, but perhaps at h = max - 0.5 */
		h = subcube_end->h - half;
		t = (h - position->h) * one_on_direction->h;
		if (t >= zero)
		{
		    d = position->d + t * direction->d;
		    v = position->v + t * direction->v;
		    if ( (d >= subcube_start->d) && (d <= subcube_end->d) &&
			(v >= subcube_start->v) && (v <= subcube_end->v) )
		    {
			if (d < min) min = d;
			if (d > max) max = d;
			if (t < t_enter) t_enter = t;
			if (t > t_leave) t_leave = t;
		    }
		}
	    }
#endif
	}
    }
    if (min >= max) return (FALSE);
    if (min_d != NULL) *min_d = min;
    if (max_d != NULL) *max_d = max;
    if (enter != NULL) *enter = t_enter;
    if (leave != NULL) *leave = t_leave;
    return (TRUE);
}   /*  End Function get_ray_intersections_with_cube  */

static void collect_ray_rough (iarray cube, eye_info *eye,
			       RotatedKcoord_3d ray_start,
			       RotatedKcoord_3d direction,
			       float t_enter, float t_leave,
			       signed char *buffer, unsigned int ray_length)
/*  [PURPOSE] This routine will collect a ray through a cube.
    <cube> The cube data.
    <eye> The eye information.
    <ray_start> The starting point of the ray (this is aribitrary).
    <direction> The ray direction.
    <t_enter> The position along the ray where it enters the cube.
    <t_leave> The position along the ray where it leaves the cube.
    <buffer> The buffer to write the ray data into.
    <ray_length> The length of the ray.
    [RETURNS] Nothing.
*/
{
    RotatedKcoord_3d enter, leave;
    uaddr off;
    int enter_pos, leave_pos, plane_pos, plane_count, plane_inc;
    float half = 0.5;
    float offset = 0.01;
    float h, v, t, one_on_direction_d;
    /*static char function_name[] = "__vrender_collect_ray_rough";*/

    /*  Find enter co-ordinate  */
    enter.h = ray_start.h + t_enter * direction.h;
    enter.v = ray_start.v + t_enter * direction.v;
    enter.d = ray_start.d + t_enter * direction.d;
    /*  Find leave co-ordinate  */
    leave.h = ray_start.h + t_leave * direction.h;
    leave.v = ray_start.v + t_leave * direction.v;
    leave.d = ray_start.d + t_leave * direction.d;
    enter_pos = (enter.d + half);
    leave_pos = (leave.d + half);
    plane_inc = (enter_pos < leave_pos) ? 1 : -1;
    one_on_direction_d = 1.0 / direction.d;
    for (plane_count = 0, plane_pos = enter_pos; plane_count < ray_length;
	 ++plane_count, plane_pos += plane_inc)
    {
	t = ( (float) plane_pos - ray_start.d ) * one_on_direction_d;
	h = ray_start.h + t * direction.h + offset;
	v = ray_start.v + t * direction.v + offset;
	off = eye->h_offsets[(int) h] + eye->v_offsets[(int) v];
	buffer[plane_count] = eye->planes[plane_pos][off];
    }
}   /*  End Function collect_ray_rough  */

static void collect_ray_smooth (iarray cube, eye_info *eye,
				RotatedKcoord_3d ray_start,
				RotatedKcoord_3d direction,
				float t_enter, float t_leave,
				signed char *buffer, unsigned int ray_length)
/*  [PURPOSE] This routine will collect a ray through a cube.
    <cube> The cube data.
    <eye> The eye information.
    <ray_start> The starting point of the ray (this is aribitrary).
    <direction> The ray direction.
    <t_enter> The position along the ray where it enters the cube.
    <t_leave> The position along the ray where it leaves the cube.
    <buffer> The buffer to write the ray data into.
    <ray_length> The length of the ray.
    [RETURNS] Nothing.
*/
{
    RotatedKcoord_3d enter, leave;
    uaddr off_00, off_10, off_01, off_11;
    int enter_pos, leave_pos, plane_pos, plane_count, plane_inc;
    int i_val_00;
    float half = 0.5;
    float one = 1.0;
    float offset = 0.01;
    float h, v, t, one_on_direction_d;
    float h1, v1;
    float val, val_10, val_01, val_11;
    float dh0, dv0, dh1, dv1;
    /*static char function_name[] = "__vrender_collect_ray_smooth";*/

    /*  Find enter co-ordinate  */
    enter.h = ray_start.h + t_enter * direction.h;
    enter.v = ray_start.v + t_enter * direction.v;
    enter.d = ray_start.d + t_enter * direction.d;
    /*  Find leave co-ordinate  */
    leave.h = ray_start.h + t_leave * direction.h;
    leave.v = ray_start.v + t_leave * direction.v;
    leave.d = ray_start.d + t_leave * direction.d;
    enter_pos = (enter.d + half);
    leave_pos = (leave.d + half);
    plane_inc = (enter_pos < leave_pos) ? 1 : -1;
    one_on_direction_d = 1.0 / direction.d;
    for (plane_count = 0, plane_pos = enter_pos; plane_count < ray_length;
	 ++plane_count, plane_pos += plane_inc)
    {
	t = ( (float) plane_pos - ray_start.d ) * one_on_direction_d;
	h = ray_start.h + t * direction.h + offset;
	v = ray_start.v + t * direction.v + offset;
	dh0 = h - floor (h);
	dv0 = v - floor (v);
	off_00 = eye->h_offsets[(int) h] + eye->v_offsets[(int) v];
	if ( (dh0 < offset) && (dv0 < offset) )
	{
	    buffer[plane_count] = eye->planes[plane_pos][off_00];
	    continue;
	}
	i_val_00 = eye->planes[plane_pos][off_00];
	/*  Intersect in this plane is between grid points  */
	h1 = ceil (h);
	v1 = ceil (v);
	if ( (h1 > eye->rot_subcube_end.h) || (v1 > eye->rot_subcube_end.v) )
	{
	    /*  Near the edge of the cube: cannot smooth  */
	    buffer[plane_count] = i_val_00;
	    continue;
	}
	/*  Must take contribution from 4 voxels  */
	dh1 = one - dh0;
	dv1 = one - dv0;
	off_10 = eye->h_offsets[(int) h1] + eye->v_offsets[(int) v];
	off_01 = eye->h_offsets[(int) h] + eye->v_offsets[(int) v1];
	off_11 = eye->h_offsets[(int) h1] + eye->v_offsets[(int) v1];
	val_10 = (float) eye->planes[plane_pos][off_10];
	val_01 = (float) eye->planes[plane_pos][off_01];
	val_11 = (float) eye->planes[plane_pos][off_11];
	val = ( (float) i_val_00 * dh1 * dv1 + val_10 * dh0 * dv1 +
	       val_01 * dh1 * dv0 + val_11 * dh0 * dv0 );
	buffer[plane_count] = (int) val;
    }
}   /*  End Function collect_ray_smooth  */

static flag test_pixel_sees_cube (KVolumeRenderContext context, eye_info *eye,
				  int x_coord, int y_coord, int *ray_length)
/*  [PURPOSE] This routine will determine if a pixel (point) in the view plane
    is able to "see" the volume data. In other words, it projects a ray from
    the pixel into 3-D and checks if the ray intersects the volume.
    <context> The volume rendering context.
    <eye> A pointer to the eye information.
    <x_coord> The horizontal co-ordinate of the pixel.
    <y_coord> The vertical co-ordinate of the pixel.
    <ray_length> The length of the ray will be written here. If NULL, nothing
    is written here.
    [RETURNS] TRUE if the pixel sees the volume, else FALSE.
*/
{
    RotatedKcoord_3d ray_start, ray_direction, one_on_ray_dir, vo;
    float x, y;
    float x_scale, y_scale;
    float min_d, max_d;

    x_scale = context->h_dim.last_coord - context->h_dim.first_coord;
    x_scale /= (float) (context->h_dim.length - 1);
    y_scale = context->v_dim.last_coord - context->v_dim.first_coord;
    y_scale /= (float) (context->v_dim.length - 1);
    x = context->h_dim.first_coord + (float) x_coord * x_scale;
    y = context->v_dim.first_coord + (float) y_coord * y_scale;
    vo.h = y * eye->rot_vertical.h;
    vo.v = y * eye->rot_vertical.v;
    vo.d = y * eye->rot_vertical.d;
    /*  Determine point in raster plane which corresponds to this pixel  */
    ray_start.h = eye->rot_ras_plane_centre.h + x * eye->rot_horizontal.h + vo.h;
    ray_start.v = eye->rot_ras_plane_centre.v + x * eye->rot_horizontal.v + vo.v;
    ray_start.d = eye->rot_ras_plane_centre.d + x * eye->rot_horizontal.d + vo.d;
    if (context->projection == VRENDER_PROJECTION_PARALLEL)
    {
	ray_direction = eye->rot_direction;
    }
    else
    {
	/*  Perspective projection  */
	ray_direction.h = ray_start.h - eye->rot_position.h;
	ray_direction.v = ray_start.v - eye->rot_position.v;
	ray_direction.d = ray_start.d - eye->rot_position.d;
    }
    one_on_ray_dir.h = (ray_direction.h ==
			0.0) ? TOOBIG : 1.0 / ray_direction.h;
    one_on_ray_dir.v = (ray_direction.v ==
			0.0) ? TOOBIG : 1.0 / ray_direction.v;
    one_on_ray_dir.d = (ray_direction.d ==
			0.0) ? TOOBIG : 1.0 / ray_direction.d;
    if ( !get_ray_intersections_with_cube (&ray_start, &ray_direction,
					   &one_on_ray_dir,
					   &eye->rot_subcube_start,
					   &eye->rot_subcube_end,
					   &min_d, &max_d,
					   NULL, NULL) ) return (FALSE);
    min_d = ceil (min_d);
    max_d = floor (max_d);
    if (min_d >= max_d) return (FALSE);
    if (ray_length != NULL) *ray_length = (int) max_d - (int) min_d + 1;
    return (TRUE);
}   /*  End Function test_pixel_sees_cube  */

static void reorder_job (void *pool_info, void *call_info1, void *call_info2,
			 void *call_info3, void *call_info4, void *thread_info)
/*  [PURPOSE] This routine will compute the re-ordered cube for a single line.
    This routine is meant to be called through the multi threading [<mt>]
    package.
    <pool_info> The arbitrary pool pointer.
    <call_info1> An arbitrary job pointer.
    <call_info2> An arbitrary job pointer.
    <call_info3> An arbitrary job pointer.
    <call_info4> An arbitrary job pointer.
    [RETURNS] Nothing.
*/
{
    KVolumeRenderContext context;
    RotatedKcoord_3d vo, ray_direction, one_on_ray_dir, ray_start;
    uaddr y_coord = (uaddr) call_info2;
    unsigned int x_coord;
    float x, y;
    float min_d, max_d;
    float x_min, x_scale, y_min, y_scale;
    eye_info *eye = (eye_info *) call_info1;
    ray_data *ray;
    static char function_name[] = "__vrender_compute_reordered_cube_for_line";

    context = eye->context;
    x_min = context->h_dim.first_coord;
    x_scale = context->h_dim.last_coord - x_min;
    x_scale /= (float) (context->h_dim.length - 1);
    y_min = context->v_dim.first_coord;
    y_scale = context->v_dim.last_coord - y_min;
    y_scale /= (float) (context->v_dim.length - 1);
    if (context->projection == VRENDER_PROJECTION_PARALLEL)
    {
	/*  For parallel projection need to compute ray direction
	    only once.  */
	ray_direction.h = eye->rot_direction.h;
	ray_direction.v = eye->rot_direction.v;
	ray_direction.d = eye->rot_direction.d;
	one_on_ray_dir.h = (ray_direction.h ==
			    0.0) ? TOOBIG : 1.0 / ray_direction.h;
	one_on_ray_dir.v = (ray_direction.v ==
			    0.0) ? TOOBIG : 1.0 / ray_direction.v;
	one_on_ray_dir.d = (ray_direction.d ==
			    0.0) ? TOOBIG : 1.0 / ray_direction.d;
    }
    ray = eye->reorder_rays + eye->lines[y_coord].start;
    ray += y_coord * context->h_dim.length;
    y = y_min + (float) y_coord * y_scale;
    vo.h = y * eye->rot_vertical.h;
    vo.v = y * eye->rot_vertical.v;
    vo.d = y * eye->rot_vertical.d;
    /*  Process all pixels in this row  */
    for (x_coord = eye->lines[y_coord].start;
	 x_coord < eye->lines[y_coord].stop;
	 ++x_coord, ++ray)
    {
	/*  Raycast this image pixel  */
	x = x_min + (float) x_coord * x_scale;
	/*  Determine point in raster plane which corresponds to this pixel  */
	ray_start.h = eye->rot_ras_plane_centre.h + x*eye->rot_horizontal.h + vo.h;
	ray_start.v = eye->rot_ras_plane_centre.v + x*eye->rot_horizontal.v + vo.v;
	ray_start.d = eye->rot_ras_plane_centre.d + x*eye->rot_horizontal.d + vo.d;
	if (context->projection == VRENDER_PROJECTION_PERSPECTIVE)
	{
	    /*  For perspective projection need to compute ray direction
		for each point.
		*/
	    ray_direction.h = ray_start.h - eye->rot_position.h;
	    ray_direction.v = ray_start.v - eye->rot_position.v;
	    ray_direction.d = ray_start.d - eye->rot_position.d;
	    one_on_ray_dir.h = (ray_direction.h ==
				0.0) ? TOOBIG : 1.0 / ray_direction.h;
	    one_on_ray_dir.v = (ray_direction.v ==
				0.0) ? TOOBIG : 1.0 / ray_direction.v;
	    one_on_ray_dir.d = (ray_direction.d ==
				0.0) ? TOOBIG : 1.0 / ray_direction.d;
	}
	/*  WARNING: at this point,  ray_direction  is not normalised  */
	if ( !get_ray_intersections_with_cube (&ray_start, &ray_direction,
					       &one_on_ray_dir,
					       &eye->rot_subcube_start,
					       &eye->rot_subcube_end,
					       &min_d, &max_d,
					       &ray->t_enter, &ray->t_leave) )
	{
	    ray->length = 0;
	    ray->ray = NULL;
	    continue;
	}
	min_d = ceil (min_d);
	max_d = floor (max_d);
	if (min_d >= max_d)
	{
	    ray->length = 0;
	    ray->ray = NULL;
	    continue;
	}
	if (ray->length != (int) max_d - (int) min_d + 1)
	{
	    fprintf(stderr,
			  "Computed ray length: %d is not stored length: %d\n",
			  (int) max_d - (int) min_d + 1, ray->length);
	    a_prog_bug (function_name);
	}
	if (context->smooth_cache)
	{
	    collect_ray_smooth (context->cube, eye, ray_start, ray_direction,
				ray->t_enter, ray->t_leave, ray->ray,
				ray->length);
	}
	else collect_ray_rough (context->cube, eye, ray_start, ray_direction,
				ray->t_enter, ray->t_leave, ray->ray,
				ray->length);
    }
}   /*  End Function reorder_job  */

static flag reorder_worker (eye_info *eye)
/*  [PURPOSE] This routine will reorder part of the cube.
    <eye> The eye information.
    [RETURNS] TRUE if more work needs to be done, else FALSE.
*/
{
    KVolumeRenderContext context = eye->context;
    KThreadPool pool;
    uaddr y_coord, cube_size;
    int x_coord;
    unsigned int count, num_to_compute, num_threads;
    ray_data *ray;
    signed char *end_buffer;
    static char function_name[] = "__vrender_reorder_worker";

    if (eye->reorder_buffer == NULL)
    {
	/*  There is no reordered buffer, so do it now  */
	cube_size = iarray_dim_length (context->cube, 0);
	cube_size *= iarray_dim_length (context->cube, 1);
	cube_size *= iarray_dim_length (context->cube, 2);
	if ( ( eye->reorder_buffer = (signed char *) m_alloc (cube_size) )
	    == NULL )
	{
	    m_abort (function_name, "reorder buffer");
	}
	eye->reorder_buf_size = cube_size;
	fprintf (stderr, "Allocated: %lu bytes for reorder buffer\n",
		 cube_size);
	eye->next_block = eye->reorder_buffer;
    }
    y_coord = eye->num_setup_lines;
    if (y_coord < context->v_dim.length)
    {
	/*  First need to setup each ray. After all lines done, then we can
	    collect rays. We have to do this because we want to multi-thread
	    this operation. Because synchronising the next block in the reorder
	    buffer is too hard with multiple threads, we split the setup
	    (allocation) task from the collection task.  */
	ray = eye->reorder_rays + y_coord * context->h_dim.length;
	end_buffer = eye->reorder_buffer + eye->reorder_buf_size;
	/*  First write blanks up to the start pixel  */
	for (x_coord = 0; x_coord < eye->lines[y_coord].start;
	     ++x_coord, ++ray)
	{
	    ray->length = 0;
	    ray->ray = NULL;
	}
	for (x_coord = eye->lines[y_coord].start;
	     x_coord < eye->lines[y_coord].stop;
	     ++x_coord, ++ray)
	{
	    if ( test_pixel_sees_cube (context, eye, x_coord, y_coord,
				       &ray->length) )
	    {
		/*  Check if reorder buffer too small  */
		if (eye->next_block + ray->length > end_buffer)
		{
		    fprintf (stderr, "Reorder buffer too small\n");
		    a_prog_bug (function_name);
		}
		ray->ray = eye->next_block;
		eye->next_block += ray->length;
	    }
	    else
	    {
		ray->length = 0;
		ray->ray = NULL;
	    }
	}
	/*  Write blanks past stop pixel  */
	for (; x_coord < context->h_dim.length; ++x_coord, ++ray)
	{
	    ray->length = 0;
	    ray->ray = NULL;
	}
	++eye->num_setup_lines;
/*
	if (eye->num_setup_lines >= context->v_dim.length)
	{
	    fprintf (stderr, "Reorder buffer for eye setup\n");
	}
*/
	return (TRUE);
    }
    /*  Now proceed and collect rays  */
    if (eye->num_reordered_lines >= context->v_dim.length) return (FALSE);
    num_to_compute = context->v_dim.length - eye->num_reordered_lines;
    pool = mt_get_shared_pool ();
    num_threads = mt_num_threads (pool);
    if (num_to_compute > num_threads) num_to_compute = num_threads;
    for (count = 0, y_coord = eye->num_reordered_lines; count < num_to_compute;
	 ++count, ++y_coord)
    {
	mt_launch_job (pool, reorder_job,
		       (void *) eye, (void *) y_coord, NULL, NULL);
    }
    mt_wait_for_all_jobs (pool);
    eye->num_reordered_lines += num_to_compute;
    return (TRUE);
}   /*  End Function reorder_worker  */

static flag worker_function (void **info)
/*  [PURPOSE] This routine is called to perform some work.
    <info> A pointer to the arbitrary work function information pointer.
    [RETURNS] TRUE if the work function should be called again, else FALSE
    indicating that the work function is to be unregistered.
*/
{
    KVolumeRenderContext context = (KVolumeRenderContext) *info;
    uaddr eyes = VRENDER_EYE_MASK_CYCLOPS;
    static char function_name[] = "worker_function";

    VERIFY_CONTEXT (context);
    if ( eye_worker (&context->cyclops) ) return (TRUE);
    if (context->never_did_stereo)
    {
	context->worker = NULL;
	c_call_callbacks (context->cache_notify_list, (void *) eyes);
	return (FALSE);
    }
    eyes |= VRENDER_EYE_MASK_LEFT | VRENDER_EYE_MASK_RIGHT;
    if ( eye_worker (&context->left) ) return (TRUE);
    if ( eye_worker (&context->right) ) return (TRUE);
    context->worker = NULL;
    c_call_callbacks (context->cache_notify_list, (void *) eyes);
    return (FALSE);
}   /*  End Function worker_function  */

static flag eye_worker (eye_info *eye)
/*  [PURPOSE] This routine is called to perform some work for an eye.
    <eye> The eye information.
    [RETURNS] TRUE if more work needs to be done, else FALSE.
*/
{
    /*static char function_name[] = "eye_worker";*/

    /*  First ensure line cache for eye is done.  */
    if ( compute_line_for_cache (eye) ) return (TRUE);
    return ( reorder_worker (eye) );
}   /*  End Function eye_worker  */

static flag compute_line_for_cache (eye_info *eye)
/*  [PURPOSE] This routine will compute a single line cache entry. The start
    and stop positions are calculated.
    <eye> The eye information.
    [RETURNS] TRUE if more work needs to be done, else FALSE.
*/
{
    KVolumeRenderContext context = eye->context;
    int x_coord, y_coord, curr_x_coord, low_x_coord, high_x_coord;

    y_coord = eye->num_lines_computed;
    if (y_coord >= context->v_dim.length) return (FALSE);
    /*  Default is entire line does not see cube.  */
    eye->lines[y_coord].start = context->h_dim.length;
    eye->lines[y_coord].stop = context->h_dim.length;
    /*  Search for start and stop y co-ordinates. First check if the entire
	line sees the cube.  */
    if ( test_pixel_sees_cube (context, eye, 0, y_coord, NULL) )
    {
	/*  All pixels on this line see the volume  */
	eye->lines[y_coord].start = 0;
	eye->lines[y_coord].stop = context->h_dim.length;
	eye->num_lines_computed++;
	if (eye->num_lines_computed >= context->v_dim.length)
	{
/*
	    fprintf (stderr, "Line cache for eye computed\n");
*/
	    return (FALSE);
	}
	return (TRUE);
    }
    /*  Now check if centre y co-ordinate sees cube. If so, we can perform
	binary searches for the horizontal start and stop y co-ordinates.
	If not, we have to search the whole line for the start
	y co-ordinate and then can perform a binary search for the stop
	y co-ordinate.  */
    if ( test_pixel_sees_cube (context, eye,
			       context->h_dim.length / 2, y_coord, NULL) )
    {
	/*  Perform a binary search for horizontal start position  */
	low_x_coord = 0;
	high_x_coord = context->h_dim.length / 2;
	while (high_x_coord - low_x_coord > 1)
	{
	    curr_x_coord = (low_x_coord + high_x_coord) / 2;
	    if ( test_pixel_sees_cube (context, eye, curr_x_coord, y_coord,
				       NULL) )
	    {
		/*  This one intersects the volume: go down  */
		high_x_coord = curr_x_coord;
	    }
	    else
	    {
		/*  This one doesn't  */
		low_x_coord = curr_x_coord;
	    }
	}
	eye->lines[y_coord].start = high_x_coord;
    }
    else
    {
	/*  Central pixel on this line does not see the volume: bugger  */
	for (x_coord = 0;
	     (x_coord < context->h_dim.length) &&
	     (eye->lines[y_coord].start >= context->h_dim.length);
	     ++x_coord)
	{
	    if ( test_pixel_sees_cube (context, eye, x_coord, y_coord, NULL) )
	    {
		/*  Found one!  */
		eye->lines[y_coord].start = x_coord;
	    }
	}
	if (eye->lines[y_coord].start >= context->h_dim.length)
	{
	    /*  The entire line does not see the cube  */
	    eye->num_lines_computed++;
	    if (eye->num_lines_computed >= context->v_dim.length)
	    {
/*
		fprintf (stderr, "Line cache for eye computed\n");
*/
		return (FALSE);
	    }
	    return (TRUE);
	}
	/*  Ah, ha! Part of the line does see the cube.  */
    }
    /*  Perform a binary search for horizontal stop position  */
    low_x_coord = eye->lines[y_coord].start;
    high_x_coord = context->h_dim.length - 1;
    while (high_x_coord - low_x_coord > 1)
    {
	curr_x_coord = (low_x_coord + high_x_coord) / 2;
	if ( test_pixel_sees_cube (context, eye, curr_x_coord, y_coord, NULL) )
	{
	    /*  This one intersects the volume: go up  */
	    low_x_coord = curr_x_coord;
	}
	else
	{
	    /*  This one doesn't  */
	    high_x_coord = curr_x_coord;
	}
    }
    eye->lines[y_coord].stop = high_x_coord;
    eye->num_lines_computed++;
    if (eye->num_lines_computed >= context->v_dim.length)
    {
/*
	fprintf (stderr, "Line cache for eye computed\n");
*/
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function compute_line_for_cache  */


/*  Geometry routines follow  */

static void geom_vector_multiply (Kcoord_3d *out, Kcoord_3d vec1,
				  Kcoord_3d vec2)
/*  [PURPOSE] This routine will multiply two vectors together, using the left
    hand rule, to produce another mutually perpendicular vector.
    <out> The new vector will be written here.
    <vec1> One of the input vectors.
    <vec2> The other input vector.
    [RETURNS] Nothing.
*/
{
    out->x = vec1.z * vec2.y - vec1.y * vec2.z;
    out->y = vec1.x * vec2.z - vec1.z * vec2.x;
    out->z = vec1.y * vec2.x - vec1.x * vec2.y;
}   /*  End Function geom_vector_multiply  */

static float geom_vector_dot_product (Kcoord_3d vec1, Kcoord_3d vec2)
/*  [PURPOSE] This routine will compute the dot product of two vectors.
    <vec1> One of the input vectors.
    <vec2> The other input vector.
    [RETURNS] The dot product.
*/
{
    return (vec1.x * vec2.x + vec1.y * vec2.y + vec1.z * vec2.z);
}   /*  End Function geom_vector_dot_product  */

static float geom_intersect_plane_with_ray (Kcoord_3d point, Kcoord_3d normal,
					    Kcoord_3d start,
					    Kcoord_3d direction,
					    Kcoord_3d *intersection_point)
/*  [PURPOSE] This routine will compute the intersection of a ray with a plane.
    The ray is assumed to intersect the plane, else a floating point error may
    occur.
    <point> Any point on the plane.
    <normal> The normal vector to the plane.
    <start> The starting point of the ray. The ray is considered bi-directional
    <direction> The direction vector of the ray. This need not be normalised.
    <intersection_point> The intersection point will be written here. If this
    is NULL nothing is written here.
    [RETURNS] The distance from the ray start of the intersection, given in
    units of the ray direction.
*/
{
    float t;

    t = ( normal.x * (point.x - start.x) + normal.y * (point.y - start.y) +
	  normal.z * (point.z - start.z) ) /
	(normal.x * direction.x + normal.y * direction.y +
	 normal.z * direction.z);
    if (intersection_point != NULL)
    {
	intersection_point->x = start.x + t * direction.x;
	intersection_point->y = start.y + t * direction.y;
	intersection_point->z = start.z + t * direction.z;
    }
    return (t);
}   /*  End Function geom_intersect_plane_with_ray  */

static void cube_destroy_func (iarray array, void *info)
/*  [SUMMARY] Register destruction of an Intelligent Array.
    <array> The Intelligent Array.
    <info> A pointer to the arbitrary information.
    [RETURNS] Nothing.
*/
{
    KVolumeRenderContext context = info;
    static char function_name[] = "cube_destroy_func";

    VERIFY_CONTEXT (context);
    context->cube = NULL;
    context->cube_destroy_cbk = NULL;
}   /*  End Function cube_destroy_func  */

static void initialise_communications ()
/*  [SUMMARY] Initialise the communications support.
    [RETURNS] Nothing.
*/
{
    static flag initialised = FALSE;

    if (initialised) return;
    initialised = TRUE;
    conn_register_server_protocol ("VRENDER_private", PROTOCOL_VERSION, 0,
				   server_open_func, server_read_func,
				   server_close_func);
    conn_register_client_protocol ("VRENDER_private", PROTOCOL_VERSION, 0,
				   client_validate_func, client_open_func,
				   client_read_func, client_close_func);
}   /*  End Function initialise_communiations  */


/*  Server connection callbacks follow  */

static flag server_open_func (Connection connection, void **info)
/*  [SUMMARY] Connection open event callback.
    [PURPOSE] This routine is called when a connection from a client opens.
    <connection> The connection object.
    <info> A pointer to the arbitrary information pointer. This may be modified
    [RETURNS] TRUE on successful registration, else FALSE (indicating the
    connection should be closed).
    [NOTE] The <<close_func>> will not be called if this routine returns
    FALSE.
*/
{
    KVolumeRenderContext context;
    ClientInfo new;
    extern KVolumeRenderContext context_for_connections;
    static char function_name[] = "__vrender_server_open_func";

    if (context_for_connections == NULL) return (FALSE);
    context = context_for_connections;
    if ( ( new = (ClientInfo) m_alloc (sizeof *new) ) == NULL )
    {
	m_error_notify (function_name, "ClientInfo");
	return (FALSE);
    }
    new->context = context;
    ++context->num_slaves;
    *info = new;
    return (TRUE);
}   /*  End Function server_open_func  */

static flag server_read_func (Connection connection, void **info)
/*  [SUMMARY] Connection read event callback.
    [PURPOSE] This routine is called when data is ready to be read from a
    connection.
    <connection> The connection object.
    <info> A pointer to the arbitrary information pointer. This may be modified
    [RETURNS] TRUE on successful reading, else FALSE (indicating the connection
    should be closed).
    [NOTE] The <<close_func>> will be called if this routine returns FALSE.
*/
{
    ClientInfo client = (ClientInfo) *info;
    static char function_name[] = "__vrender_server_read_func";

    VERIFY_CONTEXT (client->context);
    return (TRUE);
}   /*  End Function server_read_func  */

static void server_close_func (Connection connection, void *info)
/*  [SUMMARY] Connection close event callback.
    [PURPOSE] This routine is called when a connection closed.
    When this routine is called, this is the last chance to read any
    buffered data from the channel associated with the connection object.
    <connection> The connection object.
    <info> The arbitrary connection information pointer.
    [RETURNS] Nothing.
*/
{
    ClientInfo client = (ClientInfo) info;
    static char function_name[] = "__vrender_server_close_func";

    if (client->context->num_slaves < 1)
    {
	fprintf (stderr, "Client closure with zero client count!\n");
	a_prog_bug (function_name);
    }
    --client->context->num_slaves;
    m_free ( (char *) client );
}   /*  End Function server_close_func  */


/*  Client connection callbacks follow  */

static flag client_validate_func (void **info)
/*  [SUMMARY] Client connection validate event callback.
    [PURPOSE] This routine is called to validate whether it is appropriate to
    open a connection.
    <info> A pointer to the arbitrary information pointer. This may be modified
    [RETURNS] TRUE if the connection should be attempted, else FALSE
    indicating the connection should be aborted.
    [NOTE] Even if this routine is called and returns TRUE, there is no
    guarantee that the connection will be subsequently opened.
*/
{
    KVolumeRenderContext context;
    extern KVolumeRenderContext context_for_connections;
    /*static char function_name[] = "__vrender_client_validate_func";*/

    if (context_for_connections == NULL) return (FALSE);
    context = context_for_connections;
    if (context->master != NULL) return (FALSE);
    return (TRUE);
}   /*  End Function client_validate_func  */

static flag client_open_func (Connection connection, void **info)
/*  [SUMMARY] Connection open event callback.
    [PURPOSE] This routine is called when a connection to a server opens.
    <connection> The connection object.
    <info> A pointer to the arbitrary information pointer. This may be modified
    [RETURNS] TRUE on successful registration, else FALSE (indicating the
    connection should be closed).
    [NOTE] The <<close_func>> will not be called if this routine returns
    FALSE.
*/
{
    KVolumeRenderContext context;
    extern KVolumeRenderContext context_for_connections;
    /*static char function_name[] = "__vrender_client_open_func";*/

    if (context_for_connections == NULL) return (FALSE);
    context = context_for_connections;
    if (context->master != NULL) return (FALSE);
    context->master = connection;
    *info = context;
    return (TRUE);
}   /*  End Function client_open_func  */

static flag client_read_func (Connection connection, void **info)
/*  [SUMMARY] Connection read event callback.
    [PURPOSE] This routine is called when data is ready to be read from a
    connection.
    <connection> The connection object.
    <info> A pointer to the arbitrary information pointer. This may be modified
    [RETURNS] TRUE on successful reading, else FALSE (indicating the connection
    should be closed).
    [NOTE] The <<close_func>> will be called if this routine returns FALSE.
*/
{
    Channel channel;
    char command;
    iarray array;
    KVolumeRenderContext context = *info;
    multi_array *multi_desc;
    extern char *sys_errlist[];
    static char function_name[] = "__vrender_client_read_func";

    VERIFY_CONTEXT (context);
    channel = conn_get_channel (connection);
    if (ch_read (channel, &command, 1) < 1)
    {
	fprintf (stderr, "%s: error reading command\t%s\n",
		 function_name, sys_errlist[errno]);
	return (FALSE);
    }
    switch (command)
    {
      case MtoS_CUBE:
	if ( ( multi_desc = dsrw_read_multi (channel) ) == NULL ) return FALSE;
	array = iarray_get_from_multi_array (multi_desc, NULL, 3, NULL, NULL);
	ds_dealloc_multi (multi_desc);
	if (array == NULL) return (FALSE);
	if (iarray_type (array) != K_BYTE)
	{
	    fprintf (stderr, "Received non-byte cube!\n");
	    iarray_dealloc (array);
	    return (FALSE);
	}
	vrender_set_context_attributes (context,
					VRENDER_CONTEXT_ATT_CUBE, array,
					VRENDER_CONTEXT_ATT_END);
	context->cube_from_master = TRUE;
	break;
      case MtoS_VIEW:
	break;
      case MtoS_SHADER:
	break;
      case MtoS_SUBCUBE:
	break;
      case MtoS_PROJECTION:
	break;
      case MtoS_EYE_SEPARATION:
	break;
      case MtoS_SMOOTH_CACHE:
	break;
      case MtoS_SHADER_BLANK_PACKET:
	break;
      case MtoS_RENDER:
	break;
      case MtoS_COMPUTE_CACHES:
	break;
      default:
	fprintf (stderr, "%s: illegal command code: %d\n",
		 function_name, command);
	return (FALSE);
	/*break;*/
    }
    return (TRUE);
}   /*  End Function client_read_func  */

static void client_close_func (Connection connection, void *info)
/*  [SUMMARY] Connection close event callback.
    [PURPOSE] This routine is called when a connection closed.
    When this routine is called, this is the last chance to read any
    buffered data from the channel associated with the connection object.
    <connection> The connection object.
    <info> The arbitrary connection information pointer.
    [RETURNS] Nothing.
*/
{
    KVolumeRenderContext context = info;
    static char function_name[] = "__vrender_client_close_func";

    VERIFY_CONTEXT (context);
    context->master = NULL;
}   /*  End Function client_close_func  */


/*  Communications support functions  */

static flag send_cube_func (KVolumeRenderContext context, Channel channel)
/*  [SUMMARY] Send a cube to a slave.
    <context> The KVolumeRenderContext object.
    <channel> The channel object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char command = MtoS_CUBE;
    extern char *sys_errlist[];

    if (ch_write (channel, &command, 1) < 1)
    {
	fprintf (stderr, "Error writing command to channel\t%s\n",
		 sys_errlist[errno]);
	return (FALSE);
    }
    return dsrw_write_multi (channel, context->cube->multi_desc);
}   /*  End Function send_cube_func  */

static flag send_view_func (KVolumeRenderContext context, Channel channel)
/*  [SUMMARY] Send a view to a slave.
    <context> The KVolumeRenderContext object.
    <channel> The channel object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char command = MtoS_VIEW;
    extern char *sys_errlist[];

    if (ch_write (channel, &command, 1) < 1)
    {
	fprintf (stderr, "Error writing command to channel\t%s\n",
		 sys_errlist[errno]);
	return (FALSE);
    }
    if ( !pio_write_float (channel, context->view.position.x) ) return (FALSE);
    if ( !pio_write_float (channel, context->view.position.y) ) return (FALSE);
    if ( !pio_write_float (channel, context->view.position.z) ) return (FALSE);
    if ( !pio_write_float (channel, context->view.focus.x) ) return (FALSE);
    if ( !pio_write_float (channel, context->view.focus.y) ) return (FALSE);
    if ( !pio_write_float (channel, context->view.focus.z) ) return (FALSE);
    if ( !pio_write_float (channel, context->view.vertical.x) ) return (FALSE);
    if ( !pio_write_float (channel, context->view.vertical.y) ) return (FALSE);
    if ( !pio_write_float (channel, context->view.vertical.z) ) return (FALSE);
    return (TRUE);
}   /*  End Function send_view_func  */

static flag send_shader_func (KVolumeRenderContext context, Channel channel)
/*  [SUMMARY] Send a cube to a slave.
    <context> The KVolumeRenderContext object.
    <channel> The channel object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char command = MtoS_SHADER;
    extern char *sys_errlist[];

    if (ch_write (channel, &command, 1) < 1)
    {
	fprintf (stderr, "Error writing command to channel\t%s\n",
		 sys_errlist[errno]);
	return (FALSE);
    }
    return pio_write_string (channel, context->shader->name);
}   /*  End Function send_shader_func  */

static flag send_subcube_func (KVolumeRenderContext context, Channel channel)
/*  [SUMMARY] Send a cube to a slave.
    <context> The KVolumeRenderContext object.
    <channel> The channel object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char command = MtoS_SUBCUBE;
    extern char *sys_errlist[];

    if (ch_write (channel, &command, 1) < 1)
    {
	fprintf (stderr, "Error writing command to channel\t%s\n",
		 sys_errlist[errno]);
	return (FALSE);
    }
    if ( !pio_write32 (channel, context->subcube_x_start) ) return (FALSE);
    if ( !pio_write32 (channel, context->subcube_x_end) ) return (FALSE);
    if ( !pio_write32 (channel, context->subcube_y_start) ) return (FALSE);
    if ( !pio_write32 (channel, context->subcube_y_end) ) return (FALSE);
    if ( !pio_write32 (channel, context->subcube_z_start) ) return (FALSE);
    if ( !pio_write32 (channel, context->subcube_z_end) ) return (FALSE);
    return (TRUE);
}   /*  End Function send_subcube_func  */

static flag send_projection_func (KVolumeRenderContext context,
				  Channel channel)
/*  [SUMMARY] Send a cube to a slave.
    <context> The KVolumeRenderContext object.
    <channel> The channel object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char command = MtoS_PROJECTION;
    extern char *sys_errlist[];

    if (ch_write (channel, &command, 1) < 1)
    {
	fprintf (stderr, "Error writing command to channel\t%s\n",
		 sys_errlist[errno]);
	return (FALSE);
    }
    return pio_write32 (channel, context->projection);
}   /*  End Function send_projection_func  */

static flag send_eye_separation_func (KVolumeRenderContext context,
				      Channel channel)
/*  [SUMMARY] Send a cube to a slave.
    <context> The KVolumeRenderContext object.
    <channel> The channel object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char command = MtoS_EYE_SEPARATION;
    extern char *sys_errlist[];

    if (ch_write (channel, &command, 1) < 1)
    {
	fprintf (stderr, "Error writing command to channel\t%s\n",
		 sys_errlist[errno]);
	return (FALSE);
    }
    return pio_write_float (channel, context->eye_separation);
}   /*  End Function send_eye_separation_func  */

static flag send_smooth_cache_func (KVolumeRenderContext context,
				    Channel channel)
/*  [SUMMARY] Send a cube to a slave.
    <context> The KVolumeRenderContext object.
    <channel> The channel object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char command = MtoS_SMOOTH_CACHE;
    extern char *sys_errlist[];

    if (ch_write (channel, &command, 1) < 1)
    {
	fprintf (stderr, "Error writing command to channel\t%s\n",
		 sys_errlist[errno]);
	return (FALSE);
    }
    return dsrw_write_flag (channel, context->smooth_cache);
}   /*  End Function send_smooth_cache_func  */

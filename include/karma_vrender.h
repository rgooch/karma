/*  karma_vrender.h

    Header for  vrender_  package.

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

    This include file contains all the definitions and function declarations
  needed to interface to the vrender_ routines in the Karma library.


    Written by      Richard Gooch   15-OCT-1995

    Last updated by Richard Gooch   25-MAR-1996

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#if !defined(KARMA_IARRAY_DEF_H) || defined(MAKEDEPEND)
#  include <karma_iarray_def.h>
#endif

#if !defined(KARMA_VRENDER_DEF_H) || defined(MAKEDEPEND)
#  include <karma_vrender_def.h>
#endif

#if !defined(KARMA_C_DEF_H) || defined(MAKEDEPEND)
#  include <karma_c_def.h>
#endif

#ifndef KARMA_VRENDER_H
#define KARMA_VRENDER_H


#define VRENDER_CONTEXT_ATT_END               0  /*  End of varargs list     */
#define VRENDER_CONTEXT_ATT_CUBE              1  /* S:(iarray)  G:(iarray *) */
#define VRENDER_CONTEXT_ATT_VIEW              2  /* S:(view_specification *) */
#define VRENDER_CONTEXT_ATT_SHADER            3  /* S:(CONST char *)         */
#define VRENDER_CONTEXT_ATT_SUBCUBE_X_START   4  /* S:(unsigned long)        */
#define VRENDER_CONTEXT_ATT_SUBCUBE_X_END     5  /* S:(unsigned long)        */
#define VRENDER_CONTEXT_ATT_SUBCUBE_Y_START   6  /* S:(unsigned long)        */
#define VRENDER_CONTEXT_ATT_SUBCUBE_Y_END     7  /* S:(unsigned long)        */
#define VRENDER_CONTEXT_ATT_SUBCUBE_Z_START   8  /* S:(unsigned long)        */
#define VRENDER_CONTEXT_ATT_SUBCUBE_Z_END     9  /* S:(unsigned long)        */
#define VRENDER_CONTEXT_ATT_IMAGE_DESC        10 /* G:(array_desc *)         */
#define VRENDER_CONTEXT_ATT_VALID_IMAGE_DESC  11 /* G:(flag *)               */
#define VRENDER_CONTEXT_ATT_PROJECTION        12 /* S:(unsigned int)         */
#define VRENDER_CONTEXT_ATT_EYE_SEPARATION    13 /* S:(double)               */
#define VRENDER_CONTEXT_ATT_SMOOTH_CACHE      14 /* S:(flag)                 */

#define VRENDER_PROJECTION_PARALLEL    0
#define VRENDER_PROJECTION_PERSPECTIVE 1

#define VRENDER_EYE_CHOICE_CYCLOPS 0
#define VRENDER_EYE_CHOICE_LEFT    1
#define VRENDER_EYE_CHOICE_RIGHT   2

#define VRENDER_EYE_MASK_CYCLOPS 0x01
#define VRENDER_EYE_MASK_LEFT    0x02
#define VRENDER_EYE_MASK_RIGHT   0x04


/*  File:   main.c   */
EXTERN_FUNCTION (KVolumeRenderContext,
		 vrender_create_context (void *info, ...) );
EXTERN_FUNCTION (flag vrender_set_context_attributes,
		 (KVolumeRenderContext context, ...) );
EXTERN_FUNCTION (flag vrender_get_context_attributes,
		 (KVolumeRenderContext context, ...) );
EXTERN_FUNCTION (void vrender_register_shader,
		 (void (*slow_func) (), void (*fast_func) (), CONST char *name,
		  packet_desc *pack_desc,
		  CONST char *blank_packet, void *info, flag front) );
EXTERN_FUNCTION (void vrender_change_shader_blank_packet,
		 (CONST char *name, CONST char *blank_packet) );
EXTERN_FUNCTION (CONST char **vrender_get_shaders,
		 (unsigned int *num_shaders) );
EXTERN_FUNCTION (flag vrender_to_buffer,
		 (KVolumeRenderContext context, char *left_buffer,
		  char *right_buffer, double *min, double *max,
		  void (*notify_func) (void *info), void *info) );
EXTERN_FUNCTION (CONST signed char *vrender_collect_ray,
		 (KVolumeRenderContext context, unsigned int eye_view,
		  Kcoord_2d pos_2d,
		  Kcoord_3d *ray_start, Kcoord_3d *direction, float *t_enter,
		  unsigned int *ray_length) );
EXTERN_FUNCTION (flag vrender_project_3d,
		 (KVolumeRenderContext context, unsigned int eye_view,
		  Kcoord_3d point_3d, Kcoord_2d *point_2d,
		  flag test_visible) );
EXTERN_FUNCTION (void vrender_compute_caches,
		 (KVolumeRenderContext context, unsigned int eyes,
		  flag notify) );
EXTERN_FUNCTION (KCallbackFunc vrender_image_desc_notify_func,
		 (KVolumeRenderContext context,
		  void (*func) (KVolumeRenderContext context, void **info),
		  void *info) );
EXTERN_FUNCTION (KCallbackFunc vrender_cache_notify_func,
		 (KVolumeRenderContext context,
		  void (*func) (KVolumeRenderContext context, void **info,
				uaddr eyes),
		  void *info) );


#endif /*  KARMA_VRENDER_H  */

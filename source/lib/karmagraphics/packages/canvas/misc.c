/*LINTLIBRARY*/
/*  misc.c

    This code provides miscellaneous routines for world canvases.

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

/*  This file contains miscellaneous routines for world canvases.


    Written by      Richard Gooch   29-NOV-1993

    Updated by      Richard Gooch   30-NOV-1993

    Updated by      Richard Gooch   3-OCT-1994: Stripped setting of log flags.

    Updated by      Richard Gooch   30-OCT-1994: Fixed bug in
  log_transform_func  when converting from linear co-ordinates.

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/canvas/misc.c

    Updated by      Richard Gooch   10-APR-1995: Added initialisation of
  conv_type  field in <canvas_init_win_scale>.

    Updated by      Richard Gooch   26-APR-1995: Added initialisation of
  iscale_func  and  iscale_info  fields in <canvas_init_win_scale>.

    Updated by      Richard Gooch   14-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   4-JUN-1996: Switched to left-right
  bottom-top co-ordinate specification instead of min-max x and y.

    Last updated by Richard Gooch   15-JUN-1996: Switched new co-ordinate
  transformation interface.


*/
#include <stdio.h>
#include <math.h>
#define NEW_WIN_SCALE
#include <karma_canvas.h>
#include <karma_wcs.h>
#include <karma_m.h>
#include <karma_a.h>


/*  Private functions  */
STATIC_FUNCTION (void log_transform_func,
		 (KWorldCanvas canvas, unsigned int num_coords,
		  double *x, flag x_to_linear,
		  double *y, flag y_to_linear,
		  double left_x, double right_x,
		  double bottom_y, double top_y,
		  void **info) );
STATIC_FUNCTION (void astro_transform_func,
		 (KWorldCanvas canvas, unsigned int num_coords,
		  double *x, flag x_to_linear,
		  double *y, flag y_to_linear,
		  double left_x, double right_x,
		  double bottom_y, double top_y,
		  void **info) );


/*  Private structures  */
struct log_transform_info
{
    flag x;
    flag y;
};


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void canvas_init_win_scale (struct win_scale_type *win_scale,
			    unsigned int magic_number)
/*  [SUMMARY] Initialise win_scale structure.
    [PURPOSE] This routine will initialise a window scaling structure with
    sensible values. This routine may be used prior to calling <canvas_create>.
    <win_scale> A pointer to the window scaling structure.
    <magic_number> The value of this must be K_WIN_SCALE_MAGIC_NUMBER.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_init_win_scale";

    if (magic_number != K_WIN_SCALE_MAGIC_NUMBER)
    {
	(void) fprintf (stderr,
			"Bad magic number: %u\nRecompile application\n",
			magic_number);
	a_prog_bug (function_name);
    }
    m_clear ( (char *) win_scale, sizeof *win_scale );
    win_scale->magic_number = K_WIN_SCALE_MAGIC_NUMBER;
    win_scale->left_x = 0.0;
    win_scale->right_x = 1.0;
    win_scale->bottom_y = 0.0;
    win_scale->top_y = 1.0;
    win_scale->z_scale = K_INTENSITY_SCALE_LINEAR; /*  Remove in Karma v2.0  */
    win_scale->iscale_func = ( flag (*) () ) NULL;
    win_scale->iscale_free_info_func = ( void (*) (void *info) ) NULL;
    win_scale->iscale_info = NULL;
    win_scale->conv_type = CONV_CtoR_REAL;
}   /*  End Function canvas_init_win_scale  */

/*PUBLIC_FUNCTION*/
void canvas_use_log_scale (KWorldCanvas canvas, flag x_log, flag y_log)
/*  [SUMMARY] Enable logarithmic scaling for a world canvas.
    [PURPOSE] This routine will enable logarithmic scaling of the co-ordinates
    for a world canvas object.
    <canvas> The world canvas object.
    <x_log> If TRUE, the horizontal co-ordinates will be scaled
    logarithmically, else they will be scaled linearly.
    <y_log> If TRUE the vertical co-ordinates should be scaled logarithmically,
    else they will be scaled linearly.
    [RETURNS] Nothing.
*/
{
    struct log_transform_info *info;
    static char function_name[] = "canvas_use_log_scale";

    if (!x_log && !y_log) return;
    if ( ( info = (struct log_transform_info *) m_alloc (sizeof *info) )
	== NULL )
    {
	m_abort (function_name, "logarithmic info");
    }
    info->x = x_log;
    info->y = y_log;
    canvas_register_transforms_func (canvas, log_transform_func,
				     (void *) info);
}   /*  End Function canvas_use_log_scale  */

/*EXPERIMENTAL_FUNCTION*/
void canvas_use_astro_transform (KWorldCanvas canvas, KwcsAstro *ap)
/*  [SUMMARY] Use astronomical sky projections for a canvas.
    <canvas> The world canvas object.
    <ap> A pointer to the storage for a KwcsAstro object. The storage may
    contain a NULL object.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "canvas_use_astro_transform";

    if (ap == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    canvas_register_transforms_func (canvas, astro_transform_func, ap);
}   /*  End Function canvas_use_astro_transform  */


/*  Private functions follow  */

static void log_transform_func (KWorldCanvas canvas, unsigned int num_coords,
				double *x, flag x_to_linear,
				double *y, flag y_to_linear,
				double left_x, double right_x,
				double bottom_y, double top_y,
				void **info)
/*  [SUMMARY] Co-ordinate transformation callback.
    [PURPOSE] This routine will transform between linear and logarithmic world
    co-ordinates. This routine is intended to be registered with the
    [<canvas_register_transforms_func>]
    <canvas> The canvas on which the event occurred.
    <num_coords> The number of co-ordinates to transform.
    <x> The array of horizontal world co-ordinates. These are modified.
    <x_to_linear> If TRUE, then the horizontal co-ordinates are converted from
    non-linear to linear, else they are converted from linear to non-linear.
    <y> The array of vertical world co-ordinates. These are modified.
    <y_to_linear> If TRUE, then the vertical co-ordinates are converted from
    non-linear to linear, else they are converted from linear to non-linear.
    <info> A pointer to the arbitrary transform information pointer.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    double tmp;
    double zero = 0.0;
    struct log_transform_info *log_info;
    static char function_name[] = "__canvas_log_transform_func";

    FLAG_VERIFY (x_to_linear);
    FLAG_VERIFY (y_to_linear);
    log_info = (struct log_transform_info *) *info;
    if (log_info->x)
    {
	for (count = 0; count < num_coords; ++count)
	{
	    if ( (x[count] < left_x) || (left_x <= zero) || (right_x <= zero) )
	    {
		x[count] = left_x;
	    }
	    else
	    {
		if (x_to_linear)
		{
		    x[count] = log (x[count] / left_x) /
			log (right_x / left_x) * (right_x - left_x) + left_x;
		}
		else
		{
		    tmp = (x[count] - left_x) / (right_x - left_x);
		    x[count] = exp (log (right_x / left_x) * tmp) * left_x;
		}
	    }
	}
    }
    if (log_info->y)
    {
	for (count = 0; count < num_coords; ++count)
	{
	    if ( (y[count] < bottom_y) || (bottom_y <=zero) || (top_y <=zero) )
	    {
		y[count] = bottom_y;
	    }
	    else
	    {
		if (y_to_linear)
		{
		    y[count] = log (y[count] / bottom_y) /
			log (top_y / bottom_y) * (top_y - bottom_y) + bottom_y;
		}
		else
		{
		    tmp = (y[count] - bottom_y) / (top_y - bottom_y);
		    y[count] = exp (log (top_y / bottom_y) * tmp) * bottom_y;
		}
	    }
	}
    }
}   /*  End Function log_transform_func  */

static void astro_transform_func (KWorldCanvas canvas, unsigned int num_coords,
				  double *x, flag x_to_linear,
				  double *y, flag y_to_linear,
				  double left_x, double right_x,
				  double bottom_y, double top_y,
				  void **info)
/*  [SUMMARY] Co-ordinate transformation callback.
    [PURPOSE] This routine will transform between linear and non-linear world
    co-ordinates.
    <canvas> The canvas on which the event occurred.
    <num_coords> The number of co-ordinates to transform.
    <x> The array of horizontal world co-ordinates. These are modified.
    <x_to_linear> If TRUE, then the horizontal co-ordinates are converted from
    non-linear to linear, else they are converted from linear to non-linear.
    <y> The array of vertical world co-ordinates. These are modified.
    <y_to_linear> If TRUE, then the vertical co-ordinates are converted from
    non-linear to linear, else they are converted from linear to non-linear.
    <info> A pointer to the arbitrary transform information pointer.
    [RETURNS] Nothing.
*/
{
    KwcsAstro astro_projection = *(KwcsAstro *) *info;
    unsigned int num_restr;
    char *xlabel, *ylabel;
    char **restr_names;
    double *restr_values;

    if (astro_projection == NULL) return;
    if (x_to_linear != y_to_linear)
    {
	(void) fprintf (stderr, "x_lin: %d  y_lin: %d\n",
			x_to_linear, y_to_linear);
    }
    canvas_get_specification (canvas, &xlabel, &ylabel, &num_restr,
			      &restr_names, &restr_values);
    wcs_astro_transform3 (astro_projection, num_coords,
			  xlabel, x, x_to_linear,
			  ylabel, y, y_to_linear,
			  NULL, NULL, FALSE,
			  num_restr, (CONST char **)restr_names, restr_values);
}   /*  End Function astro_transform_func  */

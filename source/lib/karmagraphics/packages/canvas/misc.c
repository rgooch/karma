/*LINTLIBRARY*/
/*  misc.c

    This code provides miscellaneous routines for world canvases.

    Copyright (C) 1993,1994,1995  Richard Gooch

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

    Last updated by Richard Gooch   26-APR-1995: Added initialisation of
  iscale_func  and  iscale_info  fields in <canvas_init_win_scale>.


*/
#include <stdio.h>
#include <math.h>
#include <karma_canvas.h>
#include <karma_m.h>


/*  Private functions  */
STATIC_FUNCTION (void log_transform_func,
		 (double *x, double *y, flag to_linear,
		  double x_min, double x_max, double y_min, double y_max,
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
/*  [PURPOSE] This routine will initialise a window scaling structure with
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
    win_scale->x_min = 0.0;
    win_scale->x_max = 1.0;
    win_scale->y_min = 0.0;
    win_scale->y_max = 1.0;
    win_scale->z_scale = K_INTENSITY_SCALE_LINEAR; /*  Remove in Karma v2.0  */
    win_scale->iscale_func = ( flag (*) () ) NULL;
    win_scale->iscale_free_info_func = ( void (*) (void *info) ) NULL;
    win_scale->iscale_info = NULL;
    win_scale->conv_type = CONV_CtoR_REAL;
}   /*  End Function canvas_init_win_scale  */

/*PUBLIC_FUNCTION*/
void canvas_use_log_scale (KWorldCanvas canvas, flag x_log, flag y_log)
/*  This routine will enable logarithmic scaling of the co-ordinates for a
    world canvas object.
    The world canvas must be given by  canvas  .
    If the horizontal co-ordinates should be scaled logarithmically, the value
    of  x_log  must be TRUE, else they will be scaled linearly.
    If the vertical co-ordinates should be scaled logarithmically, the value
    of  y_log  must be TRUE, else they will be scaled linearly.
    The routine returns nothing.
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
    (*info).x = x_log;
    (*info).y = y_log;
    canvas_register_transform_func (canvas, log_transform_func, (void *) info);
}   /*  End Function canvas_use_log_scale  */


/*  Private functions follow  */
static void log_transform_func (double *x, double *y, flag to_linear,
				double x_min, double x_max,
				double y_min, double y_max, void **info)
/*  This routine will transform between linear and logarithmic world
    co-ordinates. This routine is intended to be registered with the
    canvas_register_transform_func  routine.
    The window scaling information is pointed to by  win_scale  .
    The horizontal world co-ordinate storage must be pointed to by  x  .
    The vertical world co-ordinate storage must be pointed to by  y  .
    If the value of  to_linear  is TRUE, then a non-linear to linear
    transform is required, else a linear to non-linear transform is
    required.
    The arbitrary transform information pointer is pointed to by  info  .
    The routine returns nothing.
*/
{
    double tmp;
    struct log_transform_info *log_info;
    static char function_name[] = "__canvas_log_transform_func";

    FLAG_VERIFY (to_linear);
    log_info = (struct log_transform_info *) *info;
    if ( (*log_info).x )
    {
	if ( (*x < x_min) || (x_min <= 0.0) || (x_max <= 0.0) )
	{
	    *x = x_min;
	}
	else
	{
	    if (to_linear)
	    {
		*x = log (*x / x_min) / log (x_max /
					     x_min) * (x_max - x_min) + x_min;
	    }
	    else
	    {
		tmp = (*x - x_min) / (x_max - x_min);
		*x = exp (log (x_max / x_min) * tmp) * x_min;
	    }
	}
    }
    if ( (*log_info).y )
    {
	if ( (*y < y_min) || (y_min <= 0.0) || (y_max <= 0.0) )
	{
	    *y = y_min;
	}
	else
	{
	    if (to_linear)
	    {
		*y = log (*y / y_min) / log (y_max /
					     y_min) * (y_max - y_min) + y_min;
	    }
	    else
	    {
		tmp = (*y - y_min) / (y_max - y_min);
		*y = exp (log (y_max / y_min) * tmp) * y_min;
	    }
	}
    }
}   /*  End Function log_transform_func  */

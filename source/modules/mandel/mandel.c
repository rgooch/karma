/*  mandel.c

    Source file for  mandel  (Mandelbrot generator module).

    Copyright (C) 1993-1996  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This Karma module will compute the Mandelbrot set.


    Written by      Richard Gooch   9-OCT-1993

    Updated by      Richard Gooch   16-OCT-1993

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   31-JAN-1995: Multi-threaded.

    Updated by      Richard Gooch   18-MAY-1996: Changed to <kcmap_va_create>.

    Updated by      Richard Gooch   1-JUN-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   28-JUL-1996: Made use of thread-private
  information to update flop count.


*/
#include <stdio.h>
#include <math.h>
#include <errno.h>
#include <sys/time.h>
#include <sys/resource.h>
#include <os.h>
#include <karma.h>
#include <karma_module.h>
#include <karma_iarray.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_kcmap.h>
#include <karma_mt.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_st.h>
#include <karma_ch.h>
#include <karma_cf.h>
#include <karma_im.h>
#include <karma_a.h>
#include <karma_m.h>
#ifdef OS_VXMVX
#  include <karma_vc.h>
#  include <vx/vx.h>
#endif

#define VERSION "1.3"


STATIC_FUNCTION (void compute, () );
STATIC_FUNCTION (void compute_xy,
		 (void *pool_info,
		  void *call_info1, void *call_info2,
		  void *call_info3, void *call_info4, void *thread_info) );

#ifdef OS_VXMVX
EXTERN_FUNCTION (int compute_x, (unsigned char *data, int num_iterations,
				 float cy, float x_min, int x_pixels,
				 float x_scale, float four, float one,
				 int stride, int num_colours) );
static void clear_buffer (/* value */);
#else
EXTERN_FUNCTION (int compute_x,
		 (unsigned char *data, int num_iterations, float cy,
		  float x_min, int x_pixels, float x_scale, int stride,
		  int num_colours) );
#endif


static char *output_file = NULL;
static unsigned int num_iterations = 1000;

/*  Image parameters  */
float x_min = -2.0;
float x_max = 2.0;
unsigned int x_pixels = 512;
float y_min = -2.0;
float y_max = 2.0;
unsigned int y_pixels = 512;
int cmap_size = 193;
int num_colours = 192;

/*  Convenient global data  */
float y_scale;
float x_scale;

#ifdef OS_VXMVX
static int screen_width = 0;
static int screen_height = 0;
static unsigned int *frame_buffer = (unsigned int *) 0x80000000;
#endif


int main (int argc, char **argv)
{
    KControlPanel panel;
#ifdef OS_VXMVX
    Kdisplay dpy_handle;
    Kcolourmap cmap;
    extern int screen_width;
    extern int screen_height;
#endif
    static char function_name[] = "main";

    /*  Set up user interface  */
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "compute", "action", PIT_FUNCTION, (void *) compute,
		    PIA_END);
    panel_add_item (panel, "y_pixels", "number", K_UINT, &y_pixels,
		    PIA_END);
    panel_add_item (panel, "y_max", "co-ordinate", K_FLOAT, &y_max,
		    PIA_END);
    panel_add_item (panel, "y_min", "co-ordinate", K_FLOAT, &y_min,
		    PIA_END);
    panel_add_item (panel, "x_pixels", "number", K_UINT, &x_pixels,
		    PIA_END);
    panel_add_item (panel, "x_max", "co-ordinate", K_FLOAT, &x_max,
		    PIA_END);
    panel_add_item (panel, "x_min", "co-ordinate", K_FLOAT, &x_min,
		    PIA_END);
    panel_add_item (panel, "output", "filename", K_VSTRING, &output_file,
		    PIA_END);
    panel_add_item (panel, "num_iterations", "number", K_UINT, &num_iterations,
		    PIA_END);
    panel_push_onto_stack (panel);
    /*  Set up colourmap  */
#ifdef OS_VXMVX
    vx_videoformat (NULL, &screen_width, &screen_height);
    clear_buffer (0xffffffff);
    if ( ( dpy_handle = vc_get_dpy_handle () ) == NULL )
    {
	(void) fprintf (stderr, "Error getting display handle\n");
	exit (RV_UNDEF_ERROR);
    }
    if ( ( cmap = kcmap_va_create ("mandelbrot", cmap_size, FALSE, dpy_handle,
				   vc_alloc_colours, vc_free_colours,
				   vc_store_colours, vc_get_location,
				   KCMAP_ATT_END) )
	== NULL )
    {
	(void) fprintf (stderr, "Error creating colourmap\n");
	exit (RV_UNDEF_ERROR);
    }
#endif
    im_register_lib_version (KARMA_VERSION);
    /*  Enter the event loop  */
    module_run (argc, argv, "mandel", VERSION, ( flag (*) () ) NULL, -1, 0,
		FALSE);
    return (RV_OK);
}   /*  End Function main   */

static void compute ()
{
    iarray image;
    int y_pos, y_step, num_jobs, job_count;
    unsigned int num_threads, thread_count, flop_count;
    long wall_clock_time_taken;
    struct timeval start_time;
    struct timeval stop_time;
#ifdef HAS_GETRUSAGE
    long cputime_taken;
    struct rusage start_usage;
    struct rusage stop_usage;
#endif  /*  HAS_GETRUSAGE  */
    char *data;
    unsigned int *flop_ptr;
    static struct timezone tz = {0, 0};
    extern unsigned int num_iterations;
    extern unsigned int x_pixels;
    extern unsigned int y_pixels;
    extern float x_min;
    extern float x_max;
    extern float y_min;
    extern float y_max;
    extern float x_scale;
    extern float y_scale;
    extern char *output_file;
    extern char *sys_errlist[];
    static KThreadPool pool = NULL;
    static char function_name[] = "compute";

    if (pool == NULL)
    {
	if ( ( pool = mt_create_pool (NULL) ) == NULL )
	{
	    m_abort (function_name, "thread pool");
	}
	mt_new_thread_info (pool, NULL, sizeof flop_count);
    }
    flop_count = 0;
    if ( ( image = iarray_create_2D (y_pixels, x_pixels, K_UBYTE) ) == NULL )
    {
	m_error_notify (function_name, "image");
	return;
    }
    iarray_set_world_coords (image, 0, y_min, y_max);
    iarray_set_world_coords (image, 1, x_min, x_max);
    x_scale = (x_max - x_min) / (float) (x_pixels - 1);
    y_scale = (y_max - y_min) / (float) (y_pixels - 1);
    (void) fprintf (stderr, "compute...\n");
    if (gettimeofday (&start_time, &tz) != 0)
    {
	(void) fprintf (stderr, "Error getting time of day\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
#ifdef HAS_GETRUSAGE
    if (getrusage (RUSAGE_SELF, &start_usage) != 0)
    {
	(void) fprintf (stderr, "Error getting resource usage\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
#endif  /*  HAS_GETRUSAGE  */
    num_threads = mt_num_threads (pool);
    /*  Clear thread info  */
    flop_ptr = (unsigned int *) mt_get_thread_info (pool);
    m_clear (mt_get_thread_info (pool), num_threads * sizeof flop_count);
    num_jobs = (num_threads > 1) ? num_threads * 4 : 1;
    if (num_jobs > y_pixels) num_jobs = y_pixels;
    y_step = y_pixels / num_jobs;
    data = image->data;
    for (y_pos = 0, job_count = 0; y_pos < y_pixels;
	 y_pos += y_step, ++job_count, data += y_step * x_pixels)
    {
	if (y_step + y_pos > y_pixels) y_step = y_pixels - y_pos;
	mt_launch_job (pool, compute_xy,
		       data, (void *) y_pos, (void *) (y_pos + y_step), NULL);
    }
    mt_wait_for_all_jobs (pool);
    /*  Accumulate flop_count  */
    for (thread_count = 0; thread_count < num_threads; ++thread_count)
    {
	flop_count += flop_ptr[thread_count];
    }
    if (gettimeofday (&stop_time, &tz) != 0)
    {
	(void) fprintf (stderr, "Error getting time of day\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
    wall_clock_time_taken = 1000 * (stop_time.tv_sec - start_time.tv_sec);
    wall_clock_time_taken += (stop_time.tv_usec - start_time.tv_usec) / 1000;
#ifdef HAS_GETRUSAGE
    if (getrusage (RUSAGE_SELF, &stop_usage) != 0)
    {
	(void) fprintf (stderr, "Error getting resource usage\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
    cputime_taken = (stop_usage.ru_utime.tv_sec - start_usage.ru_utime.tv_sec);
    cputime_taken *= 1000;
    cputime_taken += (stop_usage.ru_utime.tv_usec -
		      start_usage.ru_utime.tv_usec) / 1000;
    if (cputime_taken > 0)
    {
	fprintf (stderr,
		 "Realtime: %ld ms\tCPUtime MFLOPS: %ld\trealtime MFLOPS: %ld\n",
		 wall_clock_time_taken, (flop_count / cputime_taken) / 1000,
		 (flop_count / wall_clock_time_taken) / 1000);
    }
    else
    {
	(void) fprintf (stderr, "Realtime: %ld ms\trealtime MFLOPS: %ld\n",
			wall_clock_time_taken,
			(flop_count / wall_clock_time_taken) / 1000);
    }
#else
    (void) fprintf (stderr, "Realtime: %ld ms\trealtime MFLOPS: %ld\n",
		    wall_clock_time_taken,
		    (flop_count / wall_clock_time_taken) / 1000);
#endif  /*  HAS_GETRUSAGE  */
    if ( (output_file != NULL) && (*output_file != '\0') )
    {
	iarray_write (image, output_file);
    }
    iarray_dealloc (image);
}   /*  End Function compute  */

static void compute_xy (void *pool_info,
			void *call_info1, void *call_info2,
			void *call_info3, void *call_info4, void *thread_info)
{
    int y_index;
    int y_start = (iaddr) call_info2;
    int y_stop = (iaddr) call_info3;
    unsigned int tmp_flop_count = 0;
    unsigned char *data = (unsigned char *) call_info1;
    unsigned int *flop_ptr = (unsigned int *) thread_info;
    float cy;
#ifdef OS_VXMVX
    float four = 4.0, one = 1.0;
#endif
    extern int num_colours;
    extern unsigned int x_pixels;
    extern float x_min;
    extern float x_max;
    extern float y_min;
    extern float x_scale;
    extern float y_scale;
#ifdef OS_VXMVX
    unsigned char *ptr;
    extern int screen_width;
    extern int screen_height;
    extern unsigned int *frame_buffer;
#endif
    /*static char function_name[] = "compute_xy";*/

    for (y_index = y_start; y_index < y_stop; ++y_index, data += x_pixels)
    {
	cy = y_min + y_scale * (float) y_index;
	tmp_flop_count += 2;
#ifdef OS_VXMVX
	ptr = (unsigned char *) (frame_buffer + screen_width * y_index);
	tmp_flop_count += compute_x (ptr, num_iterations, cy,
				     x_min, x_pixels, x_scale, four, one, 4,
				     num_colours);
#else
	tmp_flop_count += compute_x (data, num_iterations, cy,
				     x_min, x_pixels, x_scale, 1, num_colours);
#endif
    }
    *flop_ptr += tmp_flop_count;
}   /*  End Function compute_xy  */

#ifdef OS_VXMVX
static void clear_buffer (value)
unsigned int value;
{
    double val;
    double *ptr;
    double *end;
    extern int screen_width;
    extern int screen_height;
    extern unsigned int *frame_buffer;

    ptr = (double *) frame_buffer;
    end = (double *) frame_buffer + screen_width * screen_height / 2;
    *(int *) &val = value;
    *( (int *) &val + 1 ) = value;
    while (ptr < end) *ptr++ = val;
}   /*  End Function clear_buffer  */
#endif

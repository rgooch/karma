/*LINTLIBRARY*/
/*  main.c

    This code provides routines to compute an XImage from a data structure.

    Copyright (C) 1992,1993,1994  Richard Gooch

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

    This file contains the various utility routines for drawing into an XImage
  structure.


    Written by      Richard Gooch   3-OCT-1992

    Updated by      Richard Gooch   30-MAR-1993

    Updated by      Richard Gooch   13-APR-1993: Used change to  win_scale
  structure.

    Updated by      Richard Gooch   25-NOV-1993: Added more support for blank
  values and tested for (long) z_max == (long) m_min with K_UBYTE.

    Updated by      Richard Gooch   30-NOV-1993: Yet more support (and
  bugfixes) for blank values.

    Updated by      Richard Gooch   3-OCT-1994: Stripped check of log flags and
  added  #include <sys/rusage.h>  for sun4sol2.

    Updated by      Richard Gooch   4-OCT-1994: Added  #ifdef HAS_GETRUSAGE
  around calls to  getrusage  in order to avoid having to link with buggy UCB
  compatibility library in Slowaris 2.3

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/drw/main.c

    Last updated by Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <sys/types.h>
#include <sys/time.h>
#include <signal.h>
#include <sys/resource.h>
#include <errno.h>
#include <karma_drw.h>
#include <karma_ds.h>
#include <os.h>
#include <karma_m.h>
#include <karma_a.h>

#define UBYTE_TABLE_LENGTH 256

/*  External functions  */
#ifdef NEEDS_MISALIGN_COMPILE
extern flag misalign__drw_single_plane (/* ximage, num_pixels, pixel_values,
					   data, elem_type, conv_type,
					   abs_dim_desc, abs_dim_stride,
					   ord_dim_desc, ord_dim_stride,
					   win_scale */);
#endif  /*  NEEDS_MISALIGN_COMPILE  */


/*  Private functions  */
static flag fast_draw_to_char ();
static flag fast_draw_zoom_in ();
static flag fast_draw_zoom_out();
static double *alloc_values_buffer (/* num_values */);
static void setup_ubyte_lookup_table ();


/*  Private data  */
static unsigned long ubyte_lookup_table[UBYTE_TABLE_LENGTH];


/*PUBLIC_FUNCTION*/
flag drw_single_plane (ximage, num_pixels, pixel_values,
		       data, elem_type, conv_type,
		       abs_dim_desc, abs_dim_stride,
		       ord_dim_desc, ord_dim_stride, win_scale)
/*  This routine will display a single plane of an element.
    The image structure to write to must be pointed to by  ximage  .
    The number of pixel values to use must be given by  num_pixels  .
    The pixel values to use must be in the array pointed to by  pixel_values  .
    The data must be pointed to by  data  .
    The type of the element must be in  elem_type  .
    The conversion to be used for complex numbers must be in  conv_type  .
    The abscissa dimension descriptor must be pointed to by  abs_dim_desc  .
    The abscissa dimension stride of the elements in memory must be in
    abs_dim_stride  .
    The ordinate dimension descriptor must be pointed to by  ord_dim_desc  .
    The ordinate dimension stride of the elements in memory must be in
    ord_dim_stride  .
    The window scaling information must be pointed to by  win_scale  .
    The routine returns TRUE on success, else it returns FALSE.
*/
XImage *ximage;
unsigned int num_pixels;
unsigned long *pixel_values;
char *data;
unsigned int elem_type;
unsigned int conv_type;
dim_desc *abs_dim_desc;
unsigned int abs_dim_stride;
dim_desc *ord_dim_desc;
unsigned int ord_dim_stride;
struct win_scale_type *win_scale;
{
    long cpu_usec;
    long cpu_sec;
    long real_msec;
    struct timezone tz;
    struct timeval start_time;
    struct timeval stop_time;
#ifdef HAS_GETRUSAGE
    struct rusage start_usage;
    struct rusage stop_usage;
#endif
    extern char *sys_errlist[];
    int x;
    int y;
    unsigned int abs_start_coord;
    unsigned int abs_end_coord;
    unsigned int ord_start_coord;
    unsigned int ord_end_coord;
    unsigned int num_abs_coords;
    unsigned int num_ord_coords;
    char *image_ptr;
    extern char host_type_sizes[NUMTYPES];
    static int my_uid = -1;
    static char function_name[] = "drw_single_plane";

    if ( (*win_scale).z_max <= (*win_scale).z_min )
    {
	(void) fprintf (stderr,
			"Minimum intensity: %e  must be less than maximum: %e\n",
			(*win_scale).z_min, (*win_scale).z_max);
	return (FALSE);
    }
    if (elem_type == K_UBYTE)
    {
	/*  Precompute pixel index  */
	if ( (long) (*win_scale).z_min == (long) (*win_scale).z_max )
	{
	    (void) fprintf (stderr,
			    "Minimum: %e  and maximum: %e  intensity have difference less than 1\n");
	    (void) fprintf (stderr, "This is an error with K_UBYTE data\n");
	    return (FALSE);
	}
	setup_ubyte_lookup_table ( (long) (*win_scale).z_min,
				  (long) (*win_scale).z_max,
				  (int) (*win_scale).z_scale,
				  num_pixels, pixel_values,
				  (*win_scale).blank_pixel,
				  (*win_scale).min_sat_pixel,
				  (*win_scale).max_sat_pixel );
    }
    /*  Awful hack preparation  */
    if (my_uid < 0) my_uid = getuid ();
    /*  Awful hack  */
    if (my_uid == 465)
    {
	(void) fprintf (stderr,
			"Minimum intensity: %e  maximum: %e  z_scale: %u\n",
			(*win_scale).z_min, (*win_scale).z_max,
			(*win_scale).z_scale);
    }
#ifdef NEEDS_MISALIGN_COMPILE
    /*  Test if data is not aligned  */
    if (host_type_sizes[elem_type] != 1)
    {
	/*  Data type is possible to have misaligned  */
	flag aligned = TRUE;

	if ( (int) data % host_type_sizes[elem_type] != 0 )
	{
	    aligned = FALSE;
	}
	if (abs_dim_stride % host_type_sizes[elem_type] != 0 )
	{
	    aligned = FALSE;
	}
	if (ord_dim_stride % host_type_sizes[elem_type] != 0 )
	{
	    aligned = FALSE;
	}
	if (aligned != TRUE)
	{
	    /*  Data is not aligned: called specially compiled routine  */
	    return ( misalign__drw_single_plane (ximage, num_pixels,
						 pixel_values,
						 data, elem_type, conv_type,
						 abs_dim_desc, abs_dim_stride,
						 ord_dim_desc, ord_dim_stride,
						 win_scale) );
	}
    }
#endif  /*  NEEDS_MISALIGN_COMPILE  */

    /*  Awful hack  */
    if (my_uid == 465)
    {
	(void) fprintf (stderr, "%s started\tnum_pixels: %u\n",
			function_name, num_pixels);
    }
    if (num_pixels < 1)
    {
	return (FALSE);
    }
#ifdef HAS_GETRUSAGE
    if (getrusage (RUSAGE_SELF, &start_usage) != 0)
    {
	(void) fprintf (stderr, "Error getting resource usage\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
    if (gettimeofday (&start_time, &tz) != 0)
    {
	(void) fprintf (stderr, "Error getting time of day\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
#endif
    if ( (data == NULL) || (abs_dim_desc == NULL) || (ord_dim_desc == NULL)
	|| (ximage == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    /*  Determine start and stop co-ordinates along each dimension  */
    abs_start_coord = ds_get_coord_num (abs_dim_desc, (*win_scale).x_min,
					SEARCH_BIAS_CLOSEST);
    abs_end_coord = ds_get_coord_num (abs_dim_desc, (*win_scale).x_max,
				      SEARCH_BIAS_CLOSEST);
    num_abs_coords = abs_end_coord - abs_start_coord + 1;
    ord_start_coord = ds_get_coord_num (ord_dim_desc, (*win_scale).y_min,
					SEARCH_BIAS_CLOSEST);
    ord_end_coord = ds_get_coord_num (ord_dim_desc, (*win_scale).y_max,
				      SEARCH_BIAS_CLOSEST);
    num_ord_coords = ord_end_coord - ord_start_coord + 1;
    data = data + ord_dim_stride * ord_start_coord;
    data += abs_dim_stride * abs_start_coord;
    /*  Awful hack  */
    if (my_uid == 465)
    {
	(void)fprintf(stderr,
		      "num_abs: %u  num_ord: %u  x_pixels: %d  y_pixels: %d\n",
		      num_abs_coords, num_ord_coords,
		      (*win_scale).x_pixels, (*win_scale).y_pixels);
    }
    _XInitImageFuncPtrs (ximage);

    if ( (num_abs_coords == (*win_scale).x_pixels) &&
	(num_ord_coords == (*win_scale).y_pixels) )
    {
	/*  The same number of coordinates in the data to be plotted as
	    the number of pixels to be plotted and the scales are linear:
	    can use fast algorithm  */
	/*  Generate all of image  */
	for (y = 0; y < (*win_scale).y_pixels; ++y, data += ord_dim_stride)
	{
	    /*  Generate all of line  */
	    /*  Switch on image depth  */
	    switch ( (*ximage).depth )
	    {
	      case sizeof (char) * 8:
		/*  Do a fast conversion  */
		image_ptr = (*ximage).data + ( (*win_scale).y_pixels - 1 - y )
		* (*ximage).bytes_per_line;
		if (fast_draw_to_char (image_ptr, data, elem_type,
				       abs_dim_stride, (*win_scale).x_pixels,
				       conv_type, num_pixels, pixel_values,
				       (*win_scale).blank_pixel,
				       (*win_scale).min_sat_pixel,
				       (*win_scale).max_sat_pixel,
				       (*win_scale).z_min, (*win_scale).z_max,
				       (*win_scale).z_scale) != TRUE)
		{
		    return (FALSE);
		}
		break;
	      default:
		/*  Use Xlib call  */
		(void) fprintf (stderr, "Not finished non 8-bit planes\n");
		return (FALSE);
	    }
	    /*  Line generated  */
	}
	/*  Image generated  */
#ifdef HAS_GETRUSAGE
	if (getrusage (RUSAGE_SELF, &stop_usage) != 0)
	{
	    (void) fprintf (stderr, "Error getting resource usage\t%s%c\n",
			    sys_errlist[errno], BEL);
	    (void) exit (RV_SYS_ERROR);
	}
	if (gettimeofday (&stop_time, &tz) != 0)
	{
	    (void) fprintf (stderr, "Error getting time of day\t%s%c\n",
			    sys_errlist[errno], BEL);
	    (void) exit (RV_SYS_ERROR);
	}
	cpu_usec = stop_usage.ru_utime.tv_usec - start_usage.ru_utime.tv_usec;
	cpu_sec = stop_usage.ru_utime.tv_sec - start_usage.ru_utime.tv_sec;
	if (cpu_usec < 0)
	{
	    cpu_sec -= 1;
	    cpu_usec += 1000000;
	}
	real_msec = (stop_time.tv_sec - start_time.tv_sec) * 1000;
	real_msec += (stop_time.tv_usec - start_time.tv_usec) / 1000;
	/*  Awful hack  */
	if (my_uid == 465)
	{
	    (void) fprintf (stderr,
			    "%s ended\tcpu time: %lu msecs\treal time: %lu msecs\n",
			    function_name, cpu_sec * 1000 + cpu_usec / 1000,
			    real_msec);
	}
#endif  /*  HAS_GETRUSAGE  */
	return (TRUE);
    }
    if ( ( (*win_scale).x_pixels % num_abs_coords == 0 ) &&
	( (*win_scale).y_pixels % num_ord_coords == 0 ) )
    {
	/*  Number of pixels to draw is an integer multiple of number of data
	    points to draw and the scales are linear: this is a zoom in  */
	/*  Use fast algorithm  */
	if (fast_draw_zoom_in (ximage, data, elem_type,
			       abs_dim_stride, num_abs_coords,
			       (*win_scale).x_pixels / num_abs_coords,
			       ord_dim_stride, num_ord_coords,
			       (*win_scale).y_pixels / num_ord_coords,
			       conv_type, num_pixels, pixel_values,
			       (*win_scale).blank_pixel,
			       (*win_scale).min_sat_pixel,
			       (*win_scale).max_sat_pixel,
			       (*win_scale).z_min, (*win_scale).z_max,
			       (*win_scale).z_scale) != TRUE)
	{
	    return (FALSE);
	}
	/*  Image generated  */
#ifdef HAS_GETRUSAGE
	if (getrusage (RUSAGE_SELF, &stop_usage) != 0)
	{
	    (void) fprintf (stderr, "Error getting resource usage\t%s%c\n",
			    sys_errlist[errno], BEL);
	    (void) exit (RV_SYS_ERROR);
	}
	if (gettimeofday (&stop_time, &tz) != 0)
	{
	    (void) fprintf (stderr, "Error getting time of day\t%s%c\n",
			    sys_errlist[errno], BEL);
	    (void) exit (RV_SYS_ERROR);
	}
	cpu_usec = stop_usage.ru_utime.tv_usec - start_usage.ru_utime.tv_usec;
	cpu_sec = stop_usage.ru_utime.tv_sec - start_usage.ru_utime.tv_sec;
	if (cpu_usec < 0)
	{
	    cpu_sec -= 1;
	    cpu_usec += 1000000;
	}
	real_msec = (stop_time.tv_sec - start_time.tv_sec) * 1000;
	real_msec += (stop_time.tv_usec - start_time.tv_usec) / 1000;
	/*  Awful hack  */
	if (my_uid == 465)
	{
	    (void) fprintf (stderr,
			    "%s ended\tcpu time: %lu msecs\treal time: %lu msecs\n",
			    function_name, cpu_sec * 1000 + cpu_usec / 1000,
			    real_msec);
	}
#endif  /*  HAS_GETRUSAGE  */
	return (TRUE);
    }
    if ( (num_abs_coords % (*win_scale).x_pixels == 0) &&
	(num_ord_coords % (*win_scale).y_pixels == 0) )
    {
	/*  Number of data points to draw is an integer multiple of number of
	    pixels to draw and the  scales are linear: this is a zoom out  */
	/*  Use fast algorithm  */
	if (fast_draw_zoom_out (ximage, data, elem_type,
				abs_dim_stride, num_abs_coords,
				num_abs_coords / (*win_scale).x_pixels,
				ord_dim_stride, num_ord_coords,
				num_ord_coords / (*win_scale).y_pixels,
				conv_type, num_pixels, pixel_values,
				(*win_scale).blank_pixel,
				(*win_scale).min_sat_pixel,
				(*win_scale).max_sat_pixel,
				(*win_scale).z_min, (*win_scale).z_max,
				(*win_scale).z_scale) != TRUE)
	{
	    return (FALSE);
	}
	/*  Image generated  */
#ifdef HAS_GETRUSAGE
	if (getrusage (RUSAGE_SELF, &stop_usage) != 0)
	{
	    (void) fprintf (stderr, "Error getting resource usage\t%s%c\n",
			    sys_errlist[errno], BEL);
	    (void) exit (RV_SYS_ERROR);
	}
	if (gettimeofday (&stop_time, &tz) != 0)
	{
	    (void) fprintf (stderr, "Error getting time of day\t%s%c\n",
			    sys_errlist[errno], BEL);
	    (void) exit (RV_SYS_ERROR);
	}
	cpu_usec = stop_usage.ru_utime.tv_usec - start_usage.ru_utime.tv_usec;
	cpu_sec = stop_usage.ru_utime.tv_sec - start_usage.ru_utime.tv_sec;
	if (cpu_usec < 0)
	{
	    cpu_sec -= 1;
	    cpu_usec += 1000000;
	}
	real_msec = (stop_time.tv_sec - start_time.tv_sec) * 1000;
	real_msec += (stop_time.tv_usec - start_time.tv_usec) / 1000;
	/*  Awful hack  */
	if (my_uid == 465)
	{
	    (void) fprintf (stderr,
			    "%s ended\tcpu time: %lu msecs\treal time: %lu\n",
			    function_name, cpu_sec * 1000 + cpu_usec / 1000,
			    real_msec);
	}
#endif  /*  HAS_GETRUSAGE  */
	return (TRUE);
    }
    /*  Relationship between data points and pixels is complicated: use the
	all-singing-all-dancing, slow algorithm  */
    /*  Image generated  */
#ifdef HAS_GETRUSAGE
    if (getrusage (RUSAGE_SELF, &stop_usage) != 0)
    {
	(void) fprintf (stderr, "Error getting resource usage\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
    if (gettimeofday (&stop_time, &tz) != 0)
    {
	(void) fprintf (stderr, "Error getting time of day\t%s%c\n",
			sys_errlist[errno], BEL);
	(void) exit (RV_SYS_ERROR);
    }
    cpu_usec = stop_usage.ru_utime.tv_usec - start_usage.ru_utime.tv_usec;
    cpu_sec = stop_usage.ru_utime.tv_sec - start_usage.ru_utime.tv_sec;
    if (cpu_usec < 0)
    {
	cpu_sec -= 1;
	cpu_usec += 1000000;
    }
    real_msec = (stop_time.tv_sec - start_time.tv_sec) * 1000;
    real_msec += (stop_time.tv_usec - start_time.tv_usec) / 1000;
    /*  Awful hack  */
    if (my_uid == 465)
    {
	(void) fprintf (stderr,
			"%s ended\tcpu time: %lu msecs\treal time: %lu msecs\n",
			function_name, cpu_sec * 1000 + cpu_usec / 1000,
			real_msec);
    }
    (void) fprintf (stderr, "Not finished slow draw yet%c\n", BEL);
#endif  /*  HAS_GETRUSAGE  */
    return (FALSE);
}   /*  End Function drw_single_plane  */

static flag fast_draw_to_char (image, data, elem_type, stride, num_pixels,
			       conv_type, num_pixel_values, pixel_values,
			       blank_pixel, min_sat_pixel, max_sat_pixel,
			       z_min, z_max, z_scale)
/*  This routine will do a fast draw of a line of an image from data.
    The image data to write the line to must be pointed to by  image  .
    The data must be pointed to by  data  and the element type must be given
    by  elem_type  .
    The stride of elements in memory (in bytes) must be given by  stride  .
    The number of pixels to write must be given by  num_pixels  .
    The conversion to use when the elements are complex must be given by
    conv_type  .
    The number of pixel values to use must be given by  num_pixel_values  and
    the corresponding pixel values to use must be pointed to by  pixel_values
    The pixel value to be used when the intensity value is below the minimum
    value must be given by  min_sat_pixel  .
    The pixel value to be used when the intensity value is an undefined value
    value must be given by  blank_pixel  .
    The pixel value to be used when the intensity value is above the maximum
    value must be given by  max_sat_pixel  .
    The minimum and maximum intensity values must be given by  z_min  and
    z_max  .
    The intensity scale type must be given by  z_scale  .
    The routine returns TRUE on success, else it returns FALSE.
*/
char *image;
char *data;
unsigned int elem_type;
unsigned int stride;
int num_pixels;
unsigned int conv_type;
unsigned int num_pixel_values;
unsigned long *pixel_values;
unsigned long blank_pixel;
unsigned long min_sat_pixel;
unsigned long max_sat_pixel;
double z_min;
double z_max;
unsigned int z_scale;
{
    flag complex;
    int pixel_count;
    long l_mul;
    long l_div;
    long l_data;
    long l_min = z_min;
    long l_max = z_max;
    long l_blank;
    float f_mul;
    float f_data;
    float f_min = z_min;
    float f_max = z_max;
    float f_toobig = TOOBIG;
    double d_toobig = TOOBIG;
    double d_mul;
    double d_data;
    double *values;
    extern unsigned long ubyte_lookup_table[UBYTE_TABLE_LENGTH];
    static char function_name[] = "fast_draw_to_char";

    if (elem_type == K_UBYTE)
    {
	/*  Do a fast draw  */
	for (pixel_count = 0; pixel_count < num_pixels;
	     ++pixel_count, data += stride)
	{
	    *image++ = ubyte_lookup_table[*(unsigned char *) data];
	}
	return (TRUE);
    }
    switch (elem_type)
    {
      case K_BYTE:
	l_blank = -128;
	break;
      case K_SHORT:
	l_blank = -32768;
	break;
      default:
	l_blank = -2147483648;
	break;
    }
    /*  Allocate values buffer  */
    if ( ( values = alloc_values_buffer ( (unsigned int) num_pixels ) )
	== NULL )
    {
	return (FALSE);
    }
    /*  Switch on intensity scale type  */
    switch (z_scale)
    {
      case K_INTENSITY_SCALE_LINEAR:
	/*  Switch on element type  */
	switch (elem_type)
	{
	  case K_FLOAT:
	    f_mul = (num_pixel_values - 1) / (f_max - f_min);
	    /*  No need to switch on conversion type  */
	    /*  Loop  */
	    for (pixel_count = 0; pixel_count < num_pixels;
		 ++pixel_count, data += stride)
	    {
		if ( (f_data = *(float *) data) < f_min )
		{
		    *image++ = min_sat_pixel;
		}
		else if (f_data >= f_toobig)
		{
		    *image++ = blank_pixel;
		}
		else if (f_data > f_max)
		{
		    *image++ = max_sat_pixel;
		}
		else
		{
		    *image++ = pixel_values[(unsigned int)
					    ( (f_data - f_min)
					     * f_mul + 0.5 )];
		}
	    }
	    /*  End  K_FLOAT  */
	    break;
	  case K_DOUBLE:
	    d_mul = (num_pixel_values - 1) / (z_max - z_min);
	    /*  No need to switch on conversion type  */
	    /*  Loop  */
	    for (pixel_count = 0; pixel_count < num_pixels;
		 ++pixel_count, data += stride)
	    {
		if ( (d_data = *(double *) data) < z_min )
		{
		    *image++ = min_sat_pixel;
		}
		else if (d_data >= d_toobig)
		{
		    *image++ = blank_pixel;
		}
		else if (d_data > z_max)
		{
		    *image++ = max_sat_pixel;
		}
		else
		{
		    *image++ = pixel_values[(unsigned int)
					    ( (d_data - z_min)
					     * d_mul + 0.5 )];
		}
	    }
	    /*  End  K_DOUBLE  */
	    break;
	  case K_BYTE:
	  case K_INT:
	  case K_SHORT:
	    /*  No need to switch on conversion type  */
	    l_mul = (num_pixel_values - 1);
	    l_div = (z_max - z_min);
	    /*  Loop  */
	    for (pixel_count = 0; pixel_count < num_pixels;
		 ++pixel_count, data += stride)
	    {
		switch (elem_type)
		{
		  case K_BYTE:
		    l_data = *(char *) data;
		    break;
		  case K_INT:
		    l_data = *(int *) data;
		    break;
		  case K_SHORT:
		    l_data = *(short *) data;
		    break;
		}
		if (l_data == l_blank)
		{
		    *image++ = blank_pixel;
		}
		else if (l_data < l_min)
		{
		    *image++ = min_sat_pixel;
		}
		else if (l_data > l_max)
		{
		    *image++ = max_sat_pixel;
		}
		else
		{
		    *image++ = pixel_values[(unsigned int)
					    ( (l_data - l_min)
					     * l_mul / l_div )];
		}
	    }
	    /*  End  K_BYTE, K_INT, K_SHORT  */
	    break;
	  case K_UINT:
	    f_mul = (num_pixel_values - 1) / (f_max - f_min);
	    /*  No need to switch on conversion type  */
	    /*  Loop  */
	    for (pixel_count = 0; pixel_count < num_pixels;
		 ++pixel_count, data += stride)
	    {
		if ( (f_data = *(unsigned int *) data) < f_min )
		{
		    *image++ = min_sat_pixel;
		}
		else if (f_data > f_max)
		{
		    *image++ = max_sat_pixel;
		}
		else
		{
		    *image++ = pixel_values[(unsigned int)
					    ( (f_data - f_min)
					     * f_mul + 0.5 )];
		}
	    }
	    /*  End  K_UINT  */
	    break;
	  default:
	    (void) fprintf (stderr, "Not finished various data types\n");
	    return (FALSE);
	    break;
	}
	/*  End  K_INTENSITY_SCALE_LINEAR  */
	break;
      case K_INTENSITY_SCALE_LOGARITHMIC:
	/*  Convert data to generic data type  */
	if (ds_get_elements (data, elem_type, stride, values, &complex,
			     (unsigned int) num_pixels) != TRUE)
	{
	    (void) fprintf (stderr, "Error converting data\n");
	    return (FALSE);
	}
	d_mul = (num_pixel_values - 1) / log10 (z_max / z_min);
	/*  Loop for each value  */
	for (pixel_count = 0; pixel_count < num_pixels;
	     ++pixel_count, values += 2)
	{
	    if (complex == TRUE)
	    {
		/*  Complex data: convert  */
		switch (conv_type)
		{
		  case KIMAGE_COMPLEX_CONV_REAL:
		    d_data = *values;
		    break;
		  case KIMAGE_COMPLEX_CONV_IMAG:
		    d_data = values[1];
		    break;
		  case KIMAGE_COMPLEX_CONV_ABS:
		    d_data = sqrt (values[0] * values[0] +
				   values[1] * values[1]);
		    break;
		  case KIMAGE_COMPLEX_CONV_SQUARE_ABS:
		    d_data = values[0] * values[0] + values[1] * values[1];
		    break;
		  case KIMAGE_COMPLEX_CONV_PHASE:
		    if ( (values[0] == 0.0) && (values[1] == 0.0) )
		    {
			d_data = 0.0;
		    }
		    else
		    {
			d_data = atan2 (values[1], values[0]);
		    }
		    break;
		  case KIMAGE_COMPLEX_CONV_CONT_PHASE:
		    (void) fprintf (stderr, "Not finished continuous phase\n");
		    return (FALSE);
		    break;
		  default:
		    (void) fprintf (stderr,
				    "Illegal value of conversion: %d\n",
				    conv_type);
		    a_prog_bug (function_name);
		    break;
		}
	    }
	    else
	    {
		/*  Real data  */
		d_data = values[0];
	    }
	    if (d_data < z_min)
	    {
		*image++ = min_sat_pixel;
	    }
	    else if (d_data > z_max)
	    {
		*image++ = max_sat_pixel;
	    }
	    else
	    {
		*image++ = pixel_values[(unsigned int)
					(log10 (d_data / z_min) * d_mul)];
	    }
	}
	/*  End  K_INTENSITY_SCALE_LOGARITHMIC  */
	break;
      default:
	(void) fprintf (stderr, "Illegal value of intensity scale: %u\n",
			z_scale);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function fast_draw_to_char  */

static flag fast_draw_zoom_in (ximage, data, elem_type,
			       abs_dim_stride, num_abs_coords, x_pixel_factor,
			       ord_dim_stride, num_ord_coords, y_pixel_factor,
			       conv_type, num_pixel_values, pixel_values,
			       blank_pixel, min_sat_pixel, max_sat_pixel,
			       z_min, z_max, z_scale)
/*  This routine will perform a fast zoom-in operation using integral zoom
    factors.
    The XImage structure to write the image into must be pointed to by  ximage
    The data to be drawn must be pointed to by  data  .
    The element type of the data must be given by  elem_type  .
    The stride of elements in memory (in bytes) along the abscissa dimension
    must be given by  abs_dim_stride  .
    The number of abscissa dimension co-ordinate points to draw must be given
    by  num_abs_coords  .
    The number of pixels to draw for each abscissa dimension co-ordinate point
    must be given by  x_pixel_factor  .
    The stride of elements in memory (in bytes) along the ordinate dimension
    must be given by  ord_dim_stride  .
    The number of ordinate dimension co-ordinate points to draw must be given
    by  num_ord_coords  .
    The number of pixels to draw for each ordinate dimension co-ordinate point
    must be given by  y_pixel_factor  .
    The conversion to use when the elements are complex must be given by
    conv_type  .
    The number of pixel values to use must be given by  num_pixel_values  and
    the corresponding pixel values to use must be pointed to by  pixel_values
    The pixel value to be used when the intensity value is an undefined value
    value must be given by  blank_pixel  .
    The pixel value to be used when the intensity value is below the minimum
    value must be given by  min_sat_pixel  .
    The pixel value to be used when the intensity value is above the maximum
    value must be given by  max_sat_pixel  .
    The minimum and maximum intensity values must be given by  z_min  and
    z_max  .
    The intensity scale type must be given by  z_scale  .
    The routine returns TRUE on success, else it returns FALSE.
*/
XImage *ximage;
char *data;
unsigned int elem_type;
unsigned int abs_dim_stride;
unsigned int num_abs_coords;
unsigned int x_pixel_factor;
unsigned int ord_dim_stride;
unsigned int num_ord_coords;
unsigned int y_pixel_factor;
unsigned int conv_type;
unsigned int num_pixel_values;
unsigned long *pixel_values;
unsigned long blank_pixel;
unsigned long min_sat_pixel;
unsigned long max_sat_pixel;
double z_min;
double z_max;
unsigned int z_scale;
{
    flag complex;
    unsigned int abs_coord_count;
    unsigned int ord_coord_count;
    unsigned int bytes_per_abs_coord;
    unsigned int bytes_per_ord_coord;
    int x;
    int y;
    unsigned long pixel;
    double d_data;
    double mul;
    double toobig = TOOBIG;
    char *image;
    char *image_ptr;
    char *line_ptr;
    unsigned char *c_pixel_ptr;
    unsigned short *s_pixel_ptr;
    unsigned int *i_pixel_ptr;
    double *values;
    double *values_ptr;
    static int my_uid = -1;
    static char function_name[] = "fast_draw_zoom_in";

    /*  Awful hack preparation  */
    if (my_uid < 0) my_uid = getuid ();
    /*  Awful hack  */
    if (my_uid == 465)
    {
	(void) fprintf (stderr, "%s started\n", function_name);
    }
    /*  Set up scale info  */
    switch (z_scale)
    {
      case K_INTENSITY_SCALE_LINEAR:
	mul = (double) (num_pixel_values - 1) / (z_max - z_min);
	break;
      case K_INTENSITY_SCALE_LOGARITHMIC:
	mul = (num_pixel_values - 1) / log10 (z_max / z_min);
	break;
      default:
	(void) fprintf (stderr, "Not finished various scale types\n");
	return (FALSE);
	break;
    }
    /*  Set up image pointer (in case it's needed for the fast draw code)  */
    image = (*ximage).data;
    image += (num_ord_coords * y_pixel_factor - 1) * (*ximage).bytes_per_line;
    bytes_per_abs_coord = (*ximage).depth / 8 * x_pixel_factor;
    bytes_per_ord_coord = (*ximage).bytes_per_line * y_pixel_factor;
    /*  Allocate values buffer  */
    if ( ( values = alloc_values_buffer (num_abs_coords) ) == NULL )
    {
	return (FALSE);
    }
    for (ord_coord_count = 0; ord_coord_count < num_ord_coords;
	 ++ord_coord_count, data += ord_dim_stride,
	 image -= bytes_per_ord_coord)
    {
	image_ptr = image;
	/*  Convert data points  */
	if (ds_get_elements (data, elem_type, abs_dim_stride, values,
			     &complex, num_abs_coords) != TRUE)
	{
	    (void) fprintf (stderr, "Error converting data\n");
	    return (FALSE);
	}
	for (abs_coord_count = 0, values_ptr = values;
	     abs_coord_count < num_abs_coords;
	     ++abs_coord_count, image_ptr += bytes_per_abs_coord,
	     values_ptr +=2)
	{
	    if (complex == TRUE)
	    {
		/*  Complex data: convert  */
		switch (conv_type)
		{
		  case KIMAGE_COMPLEX_CONV_REAL:
		    d_data = *values_ptr;
		    break;
		  case KIMAGE_COMPLEX_CONV_IMAG:
		    d_data = values_ptr[1];
		    break;
		  case KIMAGE_COMPLEX_CONV_ABS:
		    d_data = sqrt (values_ptr[0] * values_ptr[0] +
				   values_ptr[1] * values_ptr[1]);
		    break;
		  case KIMAGE_COMPLEX_CONV_SQUARE_ABS:
		    d_data = (values_ptr[0] * values_ptr[0] +
			      values_ptr[1] * values_ptr[1]);
		    break;
		  case KIMAGE_COMPLEX_CONV_PHASE:
		    if ( (values_ptr[0] == 0.0) && (values_ptr[1] == 0.0) )
		    {
			d_data = 0.0;
		    }
		    else
		    {
			d_data = atan2 (values_ptr[1], values_ptr[0]);
		    }
		    break;
		  case KIMAGE_COMPLEX_CONV_CONT_PHASE:
		    (void) fprintf (stderr, "Not finished continuous phase\n");
		    return (FALSE);
		    break;
		  default:
		    (void) fprintf (stderr,
				    "Illegal value of conversion: %d\n",
				    conv_type);
		    a_prog_bug (function_name);
		    break;
		}
	    }
	    else
	    {
		/*  Real data  */
		d_data = values_ptr[0];
	    }
	    /*  Have data point  */
	    /*  Determine pixel value  */
	    switch (z_scale)
	    {
	      case K_INTENSITY_SCALE_LINEAR:
		if (d_data < z_min)
		{
		    pixel = min_sat_pixel;
		}
		else if (d_data >= toobig)
		{
		    pixel = blank_pixel;
		}
		else if (d_data > z_max)
		{
		    pixel = max_sat_pixel;
		}
		else
		{
		    pixel = pixel_values[(unsigned long)
					 ( (d_data - z_min) * mul + 0.5 )];
		}
		break;
	      case K_INTENSITY_SCALE_LOGARITHMIC:
		if (d_data < z_min)
		{
		    pixel = min_sat_pixel;
		}
		else if (d_data >= toobig)
		{
		    pixel = blank_pixel;
		}
		else if (d_data > z_max)
		{
		    pixel = max_sat_pixel;
		}
		else
		{
		    pixel = pixel_values[(unsigned int)
					 (log10 (d_data / z_min) * mul + 0.5)];
		}
		break;
	      default:
		(void) fprintf (stderr, "Not finished various scale types\n");
		return (FALSE);
		break;
	    }
	    /*  Write out pixel value to many pixels  */
	    /*  Switch on image depth  */
	    if ( (*ximage).depth == sizeof (char) * 8 )
	    {
		/*  Byte sized pixels  */
		line_ptr = image_ptr;
		for (y = 0; y < y_pixel_factor;
		     ++y, line_ptr -= (*ximage).bytes_per_line)
		{
		    c_pixel_ptr = (unsigned char *) line_ptr;
		    for (x = 0; x < x_pixel_factor; ++x)
		    {
			*c_pixel_ptr++ = pixel;
		    }
		}
	    }
	    else if ( (*ximage).depth == sizeof (short) * 8 )
	    {
		/*  Short sized pixels  */
		line_ptr = image_ptr;
		for (y = 0; y < y_pixel_factor;
		     ++y, line_ptr -= (*ximage).bytes_per_line)
		{
		    s_pixel_ptr = (unsigned short *) line_ptr;
		    for (x = 0; x < x_pixel_factor; ++x)
		    {
			*s_pixel_ptr++ = pixel;
		    }
		}
	    }
	    else if ( (*ximage).depth == sizeof (int) * 8 )
	    {
		/*  Int sized pixels  */
		line_ptr = image_ptr;
		for (y = 0; y < y_pixel_factor;
		     ++y, line_ptr -= (*ximage).bytes_per_line)
		{
		    i_pixel_ptr = (unsigned int *) line_ptr;
		    for (x = 0; x < x_pixel_factor; ++x)
		    {
			*i_pixel_ptr++ = pixel;
		    }
		}
	    }
	    else
	    {
		/*  Some other sized pixels: use Xlib call to XPutPixel  */
		for (y = 0; y < y_pixel_factor; ++y)
		{
		    for (x = 0; x < x_pixel_factor; ++x)
		    {
			XPutPixel (ximage,
				   x + abs_coord_count * x_pixel_factor,
				   (num_ord_coords - ord_coord_count) *
				   y_pixel_factor - 1 - y,
				   pixel);
		    }
		}
	    }
	}
    }
    return (TRUE);
}   /*  End Function fast_draw_zoom_in  */

static flag fast_draw_zoom_out (ximage, data, elem_type,
				abs_dim_stride, num_abs_coords, x_pixel_factor,
				ord_dim_stride, num_ord_coords, y_pixel_factor,
				conv_type, num_pixel_values, pixel_values,
				blank_pixel, min_sat_pixel, max_sat_pixel,
				z_min, z_max, z_scale)
/*  This routine will perform a fast zoom-out operation using integral zoom
    factors.
    The XImage structure to write the image into must be pointed to by  ximage
    The data to be drawn must be pointed to by  data  .
    The element type of the data must be given by  elem_type  .
    The stride of elements in memory (in bytes) along the abscissa dimension
    must be given by  abs_dim_stride  .
    The number of abscissa dimension co-ordinate points to draw must be given
    by  num_abs_coords  .
    The number of abscissa dimension co-ordinate points for each pixel to draw
    must be given by  x_pixel_factor  .
    The stride of elements in memory (in bytes) along the ordinate dimension
    must be given by  ord_dim_stride  .
    The number of ordinate dimension co-ordinate points to draw must be given
    by  num_ord_coords  .
    The number of ordinate dimension co-ordinate points for each pixel to draw
    must be given by  y_pixel_factor  .
    The conversion to use when the elements are complex must be given by
    conv_type  .
    The number of pixel values to use must be given by  num_pixel_values  and
    the corresponding pixel values to use must be pointed to by  pixel_values
    The pixel value to be used when the intensity value is an undefined value
    value must be given by  blank_pixel  .
    The pixel value to be used when the intensity value is below the minimum
    value must be given by  min_sat_pixel  .
    The pixel value to be used when the intensity value is above the maximum
    value must be given by  max_sat_pixel  .
    The minimum and maximum intensity values must be given by  z_min  and
    z_max  .
    The intensity scale type must be given by  z_scale  .
    The routine returns TRUE on success, else it returns FALSE.
*/
XImage *ximage;
char *data;
unsigned int elem_type;
unsigned int abs_dim_stride;
unsigned int num_abs_coords;
unsigned int x_pixel_factor;
unsigned int ord_dim_stride;
unsigned int num_ord_coords;
unsigned int y_pixel_factor;
unsigned int conv_type;
unsigned int num_pixel_values;
unsigned long *pixel_values;
unsigned long blank_pixel;
unsigned long min_sat_pixel;
unsigned long max_sat_pixel;
double z_min;
double z_max;
unsigned int z_scale;
{
    flag complex;
    unsigned int abs_coord_count;
    unsigned int ord_coord_count;
    unsigned int image_width;
    unsigned int image_height;
    unsigned int bytes_per_x;
    unsigned int bytes_per_y;
    int x;
    int y;
    unsigned long pixel;
    double d_data;
    double mul;
    double av_factor;
    double toobig = TOOBIG;
    char *image;
    char *pixel_ptr;
    char *abs_ptr;
    char *block_ptr;
    double *values;
    double *values_ptr;
    static int my_uid = -1;
    static char function_name[] = "fast_draw_zoom_out";

    /*  Awful hack preparation  */
    if (my_uid < 0) my_uid = getuid ();
    /*  Awful hack  */
    if (my_uid == 465)
    {
	(void) fprintf (stderr, "%s started\n", function_name);
    }
    av_factor = 1.0 / (double) (x_pixel_factor * y_pixel_factor);
    image_width = num_abs_coords / x_pixel_factor;
    image_height = num_ord_coords / y_pixel_factor;
    bytes_per_x = x_pixel_factor * abs_dim_stride;
    bytes_per_y = y_pixel_factor * ord_dim_stride;
    /*  Allocate values buffer  */
    if ( ( values = alloc_values_buffer (x_pixel_factor) ) == NULL )
    {
	return (FALSE);
    }
    /*  Set up scale info  */
    switch (z_scale)
    {
      case K_INTENSITY_SCALE_LINEAR:
	mul = (double) (num_pixel_values - 1) / (z_max - z_min);
	break;
      case K_INTENSITY_SCALE_LOGARITHMIC:
	mul = (num_pixel_values - 1) / log10 (z_max / z_min);
	break;
      default:
	(void) fprintf (stderr, "Not finished various scale types\n");
	return (FALSE);
	break;
    }
    /*  Set up image pointer (in case it's needed for the fast draw code)  */
    image = (*ximage).data;
    image += (image_height - 1) * (*ximage).bytes_per_line;
    /*  Loop through all pixels in image  */
    for (y = 0; y < image_height;
	 y++, image -= (*ximage).bytes_per_line, data += bytes_per_y)
    {
	/*  Loop through all pixels in line  */
	pixel_ptr = image;
	block_ptr = data;
	for (x = 0; x < image_width; x++, block_ptr += bytes_per_x)
	{
	    /*  Compute block of data points  */
	    d_data = 0.0;
	    abs_ptr = block_ptr;
	    for (ord_coord_count = 0; ord_coord_count < y_pixel_factor;
		 ++ord_coord_count, abs_ptr += ord_dim_stride)
	    {
		/*  Convert values  */
		if (ds_get_elements (abs_ptr, elem_type, abs_dim_stride,
				     values, &complex, x_pixel_factor) != TRUE)
		{
		    (void) fprintf (stderr, "Error converting values\n");
		    return (FALSE);
		}
		for (abs_coord_count = 0, values_ptr = values;
		     abs_coord_count < x_pixel_factor;
		     ++abs_coord_count, values_ptr += 2)
		{
		    /*  Accumulate data point  */
		    if (complex == TRUE)
		    {
			/*  Complex data: convert  */
			switch (conv_type)
			{
			  case KIMAGE_COMPLEX_CONV_REAL:
			    d_data += *values_ptr;
			    break;
			  case KIMAGE_COMPLEX_CONV_IMAG:
			    d_data += values_ptr[1];
			    break;
			  case KIMAGE_COMPLEX_CONV_ABS:
			    d_data += sqrt (values_ptr[0] * values_ptr[0] +
					    values_ptr[1] * values_ptr[1]);
			    break;
			  case KIMAGE_COMPLEX_CONV_SQUARE_ABS:
			    d_data += (values_ptr[0] * values_ptr[0] +
				       values_ptr[1] * values_ptr[1]);
			    break;
			  case KIMAGE_COMPLEX_CONV_PHASE:
			    if ( (values_ptr[0] == 0.0) && (values_ptr[1] == 0.0) )
			    {
				d_data += 0.0;
			    }
			    else
			    {
				d_data += atan2 (values_ptr[1], values_ptr[0]);
			    }
			    break;
			  case KIMAGE_COMPLEX_CONV_CONT_PHASE:
			    (void) fprintf (stderr,
					    "Not finished continuous phase\n");
			    return (FALSE);
			    break;
			  default:
			    (void) fprintf (stderr,
					    "Illegal value of conversion: %d\n",
					    conv_type);
			    a_prog_bug (function_name);
			    break;
			}
		    }
		    else
		    {
			/*  Real data  */
			d_data += values_ptr[0];
		    }
		}
	    }
	    d_data *= av_factor;
	    /*  Determine pixel value  */
	    switch (z_scale)
	    {
	      case K_INTENSITY_SCALE_LINEAR:
		if (d_data < z_min)
		{
		    pixel = min_sat_pixel;
		}
		else if (d_data >= toobig)
		{
		    pixel = blank_pixel;
		}
		else if (d_data > z_max)
		{
		    pixel = max_sat_pixel;
		}
		else
		{
		    pixel = pixel_values[(unsigned int)
					 ( (d_data - z_min) * mul + 0.5 )];
		}
		break;
	      case K_INTENSITY_SCALE_LOGARITHMIC:
		if (d_data < z_min)
		{
		    pixel = min_sat_pixel;
		}
		else if (d_data >= toobig)
		{
		    pixel = blank_pixel;
		}
		else if (d_data > z_max)
		{
		    pixel = max_sat_pixel;
		}
		else
		{
		    pixel = pixel_values[(unsigned int)
					 (log10 (d_data / z_min) * mul + 0.5)];
		}
		break;
	      default:
		(void) fprintf (stderr, "Not finished various scale types\n");
		return (FALSE);
		break;
	    }
	    /*  Write out pixel value  */
	    /*  Switch on image depth  */
	    if ( (*ximage).depth == sizeof (char) * 8 )
	    {
		/*  Byte sized pixels  */
		*(char *) pixel_ptr = pixel;
		pixel_ptr += sizeof (char);
	    }
	    else if ( (*ximage).depth == sizeof (short) * 8 )
	    {
		/*  Short sized pixels  */
		*(short *) pixel_ptr = pixel;
		pixel_ptr += sizeof (short);
	    }
	    else if ( (*ximage).depth == sizeof (int) * 8 )
	    {
		/*  Int sized pixels  */
		*(int *) pixel_ptr = pixel;
		pixel_ptr += sizeof (int);
	    }
	    else
	    {
		/*  Some other sized pixels: use Xlib call to XPutPixel  */
		XPutPixel (ximage, x, image_height - y - 1, pixel);
	    }
	}
    }
    return (TRUE);
}   /*  End Function fast_draw_zoom_out  */

static double *alloc_values_buffer (num_values)
/*  This routine will allocate a buffer space for generic data values.
    The number of (DCOMPLEX) values to allocate must be given by  num_values  .
    The routine will return a pointer to the buffer space on success,
    else it returns NULL. The buffer is global and must NOT be deallocated.
*/
unsigned int num_values;
{
    static unsigned int value_buf_len = 0;
    static double *values = NULL;
    static char function_name[] = "alloc_values_buffer";

    /*  Make sure value buffer is big enough  */
    if (value_buf_len < num_values)
    {
	/*  Buffer too small  */
	if (values != NULL)
	{
	    m_free ( (char *) values );
	}
	value_buf_len = 0;
	if ( ( values = (double *) m_alloc (sizeof *values * 2 * num_values)
	      ) == NULL )
	{
	    m_error_notify (function_name, "values buffer");
	    return (NULL);
	}
	value_buf_len = num_values;
    }
    return (values);
}   /*  End Function alloc_values_buffer  */

static void setup_ubyte_lookup_table (min, max, scale,
				      num_pixels, pixel_values,
				      blank_pixel,min_sat_pixel, max_sat_pixel)
/*  This routine will setup a lookup table for unsigned byte data which will
    convert from data values to pixel values.
    The minimum (lower intensity clipping point) value must be given by  min  .
    The maximum (upper intensity clipping point) value must be given by  max  .
    The intensity scale to compute must be given by  scale  .
    The number of pixel values to use must be given by  num_pixels  .
    The pixel values to use must be in the array pointed to by  pixel_values  .
    The pixel value to be used when the intensity value is an undefined value
    value must be given by  blank_pixel  .
    The pixel value to be used when the intensity value is below the minimum
    value must be given by  min_sat_pixel  .
    The pixel value to be used when the intensity value is above the maximum
    value must be given by  max_sat_pixel  .
    The routine will write the lookup table into the global array:
    ubyte_lookup_table  .
    The routine returns nothing.
*/
long min;
long max;
int scale;
unsigned int num_pixels;
unsigned long *pixel_values;
unsigned long blank_pixel;
unsigned long min_sat_pixel;
unsigned long max_sat_pixel;
{
    long l_mul;
    long l_div;
    long ubyte_val;
    double d_mul;
    extern unsigned long ubyte_lookup_table[UBYTE_TABLE_LENGTH];
    static long old_min = -1;
    static long old_max = -1;
    static int old_scale = -1;
    static unsigned long old_first_pixel = 0;
    static unsigned int old_num_pixels = 0;
    static unsigned long *old_pixel_values = NULL;
    static char function_name[] = "setup_ubyte_lookup_table";

    if ( (old_min == min) && (old_max == max) && (old_scale == scale) &&
	(old_num_pixels == num_pixels) && (old_pixel_values == pixel_values) &&
	(old_first_pixel == pixel_values[0]) )
    {
	/*  No change  */
	return;
    }
    old_min = min;
    old_max = max;
    old_scale = scale;
    old_num_pixels = num_pixels;
    old_pixel_values = pixel_values;
    /*  Precompute pixel index  */
    switch (scale)
    {
      case K_INTENSITY_SCALE_LINEAR:
	l_mul = (num_pixels - 1);
	l_div = max - min;
	/*  Loop through all possible ubyte values  */
	for (ubyte_val = 0; ubyte_val < UBYTE_TABLE_LENGTH; ++ubyte_val)
	{
	    if (ubyte_val < min)
	    {
		ubyte_lookup_table[ubyte_val] = min_sat_pixel;
	    }
	    else if (ubyte_val > max)
	    {
		ubyte_lookup_table[ubyte_val] = max_sat_pixel;
	    }
	    else
	    {
		ubyte_lookup_table[ubyte_val] = pixel_values[(unsigned int)
							     ( (ubyte_val -
								min)
							      * l_mul /
							      l_div )];
	    }
	}
	break;
      case K_INTENSITY_SCALE_LOGARITHMIC:
	d_mul = (num_pixels - 1) / log10 ( (double) max / (double) min );
	/*  Loop through all possible ubyte values  */
	for (ubyte_val = 0; ubyte_val < UBYTE_TABLE_LENGTH; ++ubyte_val)
	{
	    if (ubyte_val < min)
	    {
		ubyte_lookup_table[ubyte_val] = min_sat_pixel;
	    }
	    else if (ubyte_val > max)
	    {
		ubyte_lookup_table[ubyte_val] = max_sat_pixel;
	    }
	    else
	    {
		ubyte_lookup_table[ubyte_val] =
		pixel_values[(unsigned int) (log10 ( (double) ubyte_val /
						    (double) min ) *
					     d_mul)];
	    }
	}
	break;
      default:
	(void) fprintf (stderr, "Illegal intensity scale value: %d\n", scale);
	a_prog_bug (function_name);
	break;
    }
}   /*  End Function setup_ubyte_lookup_table  */

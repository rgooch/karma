/*  send-vectors.c

    Source file for  send-vectors  (Read vector data and send overlays).

    Copyright (C) 1996  Richard Gooch

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

/*  This Karma module read two 2-dimensional arrays. One file must contain
  angle information and the other must contain length information. This
  information is combined to generate a list of vector overlays.


    Written by      Richard Gooch   11-JUL-1996

    Last updated by Richard Gooch   16-JUL-1996: Added controls for length
  limits, colourname and constant angle offset.


*/
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <karma.h>
#include <karma_overlay.h>
#include <karma_foreign.h>
#include <karma_module.h>
#include <karma_iarray.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_im.h>
#include <karma_m.h>
#include <karma_n.h>
#include <karma_a.h>
#include <karma_s.h>


#define VERSION "1.1"



/*  Private data  */
static unsigned int sample_interval = 10;
static char *angle_file = NULL;
static char *length_file = NULL;
static double length_scale = 1.0;
static KOverlayList olist = NULL;
static unsigned int last_id = 0;
static double offset_angle = 0.0;
static char *colourname = "green";
static double min_length = 0.0;
static double max_length = TOOBIG;


/*  Private functions  */
STATIC_FUNCTION (void compute, (CONST char *p) );
STATIC_FUNCTION (void empty_list, (CONST char *p) );


/*  Public functions follow  */

int main (int argc, char **argv)
{
    KControlPanel panel;
    extern KOverlayList olist;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "scale_length",
		    "scale factor to apply to length data",
		    K_DOUBLE, &length_scale,
		    PIA_END);
    panel_add_item (panel, "sample_interval", "sample density of vector data",
		    K_UINT, &sample_interval,
		    PIA_END);
    panel_add_item (panel, "offset_angle",
		    "constant offset to be added to angles",
		    K_DOUBLE, (char *) &offset_angle,
		    PIA_END);
    panel_add_item (panel, "min_length",
		    "length values below this are ignored",
		    K_DOUBLE, (char *) &min_length,
		    PIA_END);
    panel_add_item (panel, "max_length",
		    "length values above this are ignored",
		    K_DOUBLE, (char *) &max_length,
		    PIA_END);
    panel_add_item (panel, "length_file","name of file containing length data",
		    K_VSTRING, (char *) &length_file,
		    PIA_END);
    panel_add_item (panel, "go", "computes", PIT_FUNCTION, (void *) compute,
		    PIA_END);
    panel_add_item (panel, "empty_list", "empties overlay list", PIT_FUNCTION,
		    (void *) empty_list,
		    PIA_END);
    panel_add_item (panel, "colour_name", "name of colour to draw with",
		    K_VSTRING, (char *) &colourname,
		    PIA_END);
    panel_add_item (panel, "angle_file", "name of file containing angle data",
		    K_VSTRING, (char *) &angle_file,
		    PIA_END);
    panel_push_onto_stack (panel);
    if ( ( olist = overlay_create_list (NULL) ) == NULL )
    {
	exit (1);
    }
    im_register_lib_version (KARMA_VERSION);
    module_run (argc, argv, "send_vectors", VERSION, NULL, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static void compute (CONST char *p)
/*  [SUMMARY] Compute a set of vectors from input files.
    <p> Unused.
    [RETURNS] Nothing.
*/
{
    iarray angle_arr, length_arr;
    iarray ra_arr, dec_arr;
    KwcsAstro angle_ap, length_ap;
    unsigned int ftype, xlen, ylen, x, y, vector_count;
    float length_val;
    double x0, y0, x1, y1, angle;
    multi_array *multi_desc;
    dim_desc *xdim, *ydim;
    double *x0_arr, *y0_arr, *x1_arr, *y1_arr;
    extern char *angle_file;
    extern char *length_file;
    static char function_name[] = "compute";

    if ( ( multi_desc = foreign_guess_and_read (angle_file, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading file: \"%s\"\n", angle_file);
	return;
    }
    angle_ap = wcs_astro_setup (multi_desc->headers[0],
				multi_desc->data[0]);
    if (angle_ap == NULL)
    {
	(void) fprintf (stderr, "No header for angle file\n");
	ds_dealloc_multi (multi_desc);
	return;
    }
    /*  Try to get 2-D image  */
    if ( ( angle_arr = iarray_get_from_multi_array (multi_desc, NULL, 2,
						    NULL, NULL) )
	 == NULL )
    {
	(void) fprintf (stderr, "Could not find image\n");
	ds_dealloc_multi (multi_desc);
	wcs_astro_destroy (angle_ap);
	return;
    }
    ds_dealloc_multi (multi_desc);
    if (iarray_type (angle_arr) != K_FLOAT)
    {
	(void) fprintf (stderr, "Angle file is not floating point\n");
	iarray_dealloc (angle_arr);
	wcs_astro_destroy (angle_ap);
	return;
    }
    if ( ( multi_desc = foreign_guess_and_read (length_file, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading file: \"%s\"\n", length_file);
	iarray_dealloc (angle_arr);
	wcs_astro_destroy (angle_ap);
	return;
    }
    length_ap = wcs_astro_setup (multi_desc->headers[0],
				 multi_desc->data[0]);
    if (length_ap == NULL)
    {
	(void) fprintf (stderr, "No header for angle file\n");
	iarray_dealloc (angle_arr);
	wcs_astro_destroy (angle_ap);
	ds_dealloc_multi (multi_desc);
	return;
    }
    /*  Try to get 2-D image  */
    if ( ( length_arr = iarray_get_from_multi_array (multi_desc, NULL, 2,
						     NULL, NULL) )
	 == NULL )
    {
	(void) fprintf (stderr, "Could not find image\n");
	iarray_dealloc (angle_arr);
	wcs_astro_destroy (angle_ap);
	ds_dealloc_multi (multi_desc);
	return;
    }
    ds_dealloc_multi (multi_desc);
    if (iarray_type (length_arr) != K_FLOAT)
    {
	(void) fprintf (stderr, "Length file is not floating point\n");
	iarray_dealloc (angle_arr);
	wcs_astro_destroy (angle_ap);
	iarray_dealloc (length_arr);
	wcs_astro_destroy (length_ap);
	return;
    }
    xlen = iarray_dim_length (angle_arr, 1);
    ylen = iarray_dim_length (angle_arr, 0);
    if ( (iarray_dim_length (length_arr, 0) != ylen) ||
	 (iarray_dim_length (length_arr, 1) != xlen) )
    {
	(void) fprintf (stderr, "Images sizes differ\n");
	iarray_dealloc (angle_arr);
	wcs_astro_destroy (angle_ap);
	iarray_dealloc (length_arr);
	wcs_astro_destroy (length_ap);
	return;
    }
    (void) fprintf (stderr, "loaded...\n");
    if ( ( ra_arr = iarray_create_2D (ylen, xlen, K_DOUBLE) ) == NULL )
    {
	m_abort (function_name, "RA array");
    }
    if ( ( dec_arr = iarray_create_2D (ylen, xlen, K_DOUBLE) ) == NULL )
    {
	m_abort (function_name, "DEC array");
    }
    /*  Fill arrays with linear world co-ordinates  */
    xdim = iarray_get_dim_desc (angle_arr, 1);
    ydim = iarray_get_dim_desc (angle_arr, 0);
    (void) fprintf (stderr, "filling co-ordinate arrays...\n");
    for (y = 0; y < ylen; ++y) for (x = 0; x < xlen; ++x)
    {
	D2 (ra_arr, y, x) = ds_get_coordinate (xdim, x);
	D2 (dec_arr, y, x) = ds_get_coordinate (ydim, y);
    }
    (void) fprintf (stderr, "unprojecting...\n");
    wcs_astro_transform (angle_ap, xlen * ylen,
			 (double *) ra_arr->data, FALSE,
			 (double *) dec_arr->data, FALSE,
			 NULL, FALSE,
			 0, NULL, NULL);
    vector_count = (ylen / sample_interval) * (xlen / sample_interval);
    if ( ( x0_arr = (double *) m_alloc (sizeof *x0_arr * vector_count) )
	 == NULL )
    {
	m_abort (function_name, "x0 array");
    }
    if ( ( y0_arr = (double *) m_alloc (sizeof *y0_arr * vector_count) )
	 == NULL )
    {
	m_abort (function_name, "y0 array");
    }
    if ( ( x1_arr = (double *) m_alloc (sizeof *x1_arr * vector_count) )
	 == NULL )
    {
	m_abort (function_name, "x1 array");
    }
    if ( ( y1_arr = (double *) m_alloc (sizeof *y1_arr * vector_count) )
	 == NULL )
    {
	m_abort (function_name, "y1 array");
    }
    if (last_id != 0) overlay_remove_object (olist, last_id, 0);
    (void) fprintf (stderr, "computing vectors...\n");
    vector_count = 0;
    for (y = 0; y < ylen; y += sample_interval)
    {
	for (x = 0; x < xlen; x += sample_interval)
	{
	    x0 = D2 (ra_arr, y, x);
	    y0 = D2 (dec_arr, y, x);
	    /*  Compute angle in radians  */
	    angle = (F2 (angle_arr, y, x) + 90.0 + offset_angle) * PION180;
	    if ( ( length_val = F2 (length_arr, y, x) ) >= TOOBIG ) continue;
	    if (length_val < min_length) continue;
	    if (length_val > max_length) continue;
	    x1 = x0 - length_val * length_scale * cos (angle);
	    y1 = y0 + length_val * length_scale * sin (angle);
	    x0_arr[vector_count] = x0;
	    y0_arr[vector_count] = y0;
	    x1_arr[vector_count] = x1;
	    y1_arr[vector_count] = y1;
	    ++vector_count;
	}
    }
    fprintf (stderr, "sending vectors...\n");
    last_id = overlay_segments (olist, vector_count,
				NULL, x0_arr, y0_arr,
				NULL, x1_arr, y1_arr,
				colourname);
    iarray_dealloc (angle_arr);
    wcs_astro_destroy (angle_ap);
    iarray_dealloc (length_arr);
    wcs_astro_destroy (length_ap);
    iarray_dealloc (ra_arr);
    iarray_dealloc (dec_arr);
    m_free ( (char *) x0_arr );
    m_free ( (char *) y0_arr );
    m_free ( (char *) x1_arr );
    m_free ( (char *) y1_arr );
}   /*  End Function compute  */

static void empty_list (CONST char *p)
/*  [SUMMARY] Empty the overlay list.
    <p> Unused.
    [RETURNS] Nothing.
*/
{
    extern KOverlayList olist;
    extern unsigned int last_id;

    overlay_remove_objects (olist, 0);
    last_id = 0;
}   /*  End Function empty_list  */

/*  send-contours.c

    Source file for  send-contours  (Compute contours and send overlays).

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

/*  This Karma module reads a 2-dimensional array. Contours are extracted from
  this image and overlays are generated.


    Written by      Richard Gooch   17-JUL-1996

    Last updated by Richard Gooch   17-JUL-1996


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


#define VERSION "1.0"
#define MAX_CONTOURS 20


/*  Private data  */
static char *in_file = NULL;
static KOverlayList olist = NULL;
static unsigned int last_id = 0;
static char *colourname = "green";
static unsigned int num_contours = 0;
static double contour_levels[MAX_CONTOURS];


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
    panel_add_item (panel, "in_file", "name of file to extract contours from",
		    K_VSTRING, (char *) &in_file,
		    PIA_END);
    panel_add_item (panel, "go", "computes", PIT_FUNCTION, (void *) compute,
		    PIA_END);
    panel_add_item (panel, "empty_list", "empties overlay list", PIT_FUNCTION,
		    (void *) empty_list,
		    PIA_END);
    panel_add_item (panel, "contour_levels", "array of contour levels",
		    K_DOUBLE, contour_levels,
		    PIA_ARRAY_MAX_LENGTH, MAX_CONTOURS,
		    PIA_ARRAY_LENGTH, &num_contours,
		    PIA_END);
    panel_add_item (panel, "colour_name", "name of colour to draw with",
		    K_VSTRING, (char *) &colourname,
		    PIA_END);
    panel_push_onto_stack (panel);
    if ( ( olist = overlay_create_list (NULL) ) == NULL )
    {
	exit (1);
    }
    im_register_lib_version (KARMA_VERSION);
    module_run (argc, argv, "send_contours", VERSION, NULL, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static void compute (CONST char *p)
/*  [SUMMARY] Compute a set of contours from input file.
    <p> Unused.
    [RETURNS] Nothing.
*/
{
    iarray in_arr;
    KwcsAstro ap;
    unsigned int ftype, xlen, ylen, num_segs;
    multi_array *multi_desc;
    dim_desc *xdim, *ydim;
    extern unsigned int num_contours;
    extern char *in_file;
    extern double contour_levels[MAX_CONTOURS];
    static uaddr buf_size = 0;
    static double *x0_arr = NULL;
    static double *y0_arr = NULL;
    static double *x1_arr = NULL;
    static double *y1_arr = NULL;
    /*static char function_name[] = "compute";*/

    if ( ( multi_desc = foreign_guess_and_read (in_file, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading file: \"%s\"\n", in_file);
	return;
    }
    ap = wcs_astro_setup (multi_desc->headers[0],
			  multi_desc->data[0]);
    /*  Try to get 2-D image  */
    if ( ( in_arr = iarray_get_from_multi_array (multi_desc, NULL, 2,
						 NULL, NULL) ) == NULL )
    {
	(void) fprintf (stderr, "Could not find image\n");
	ds_dealloc_multi (multi_desc);
	if (ap != NULL) wcs_astro_destroy (ap);
	return;
    }
    ds_dealloc_multi (multi_desc);
    if (iarray_type (in_arr) != K_FLOAT)
    {
	(void) fprintf (stderr, "Input file is not floating point\n");
	iarray_dealloc (in_arr);
	if (ap != NULL) wcs_astro_destroy (ap);
	return;
    }
    xlen = iarray_dim_length (in_arr, 1);
    ylen = iarray_dim_length (in_arr, 0);
    xdim = iarray_get_dim_desc (in_arr, 1);
    ydim = iarray_get_dim_desc (in_arr, 0);
    if ( (xdim->coordinates != NULL) || (ydim->coordinates != NULL) )
    {
	(void) fprintf (stderr, "Co-ordinate array not supported\n");
	iarray_dealloc (in_arr);
	if (ap != NULL) wcs_astro_destroy (ap);
	return;
    }
    (void) fprintf (stderr, "loaded...\n");
    if (last_id != 0) overlay_remove_object (olist, last_id, 0);
    (void) fprintf (stderr, "computing contours...\n");
    num_segs = iarray_contour (in_arr, num_contours, contour_levels,
			       &buf_size,
			       &x0_arr, &y0_arr, &x1_arr, &y1_arr);
    /*  Fill arrays with linear world co-ordinates  */
    if (ap != NULL)
    {
	(void) fprintf (stderr, "unprojecting...\n");
	wcs_astro_transform (ap, num_segs,
			     x0_arr, FALSE, y0_arr, FALSE, NULL, FALSE,
			     0, NULL, NULL);
	wcs_astro_transform (ap, num_segs,
			     x1_arr, FALSE, y1_arr, FALSE, NULL, FALSE,
			     0, NULL, NULL);
    }
    (void) fprintf (stderr, "sending %u contour segments...\n", num_segs);
    last_id = overlay_segments (olist, num_segs,
				NULL, x0_arr, y0_arr,
				NULL, x1_arr, y1_arr,
				colourname);
#ifdef dummy
    /*  Draw line with one end at Ra 9h 45m 53.87 s  Dec -31d 13m 7.72s  */
    overlay_line (olist,
		  OVERLAY_COORD_WORLD, 9.7649639 * 15.0, -31.21881111,
		  OVERLAY_COORD_WORLD, 9.7649639 * 15.0 + 1e-4, -31.21881111 + 1e-4,
		  colourname);
#endif
    iarray_dealloc (in_arr);
    if (ap != NULL) wcs_astro_destroy (ap);
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

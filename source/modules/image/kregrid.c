/*  kregrid.c

    Source file for  kregrid  (Regrid file according to another files grid).

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

/*  This Karma module will read two 2-dimensional arrays. One file will be used
  to compute a new grid for the other file.


    Written by      Richard Gooch   13-JUL-1996

    Last updated by Richard Gooch   14-JUL-1996


*/
#include <stdio.h>
#include <math.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <karma.h>
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



/*  Private data  */
static char *grid_file = NULL;

#define SAMPLE_OPTION_DATA_COPY 0
#define SAMPLE_OPTION_LINEAR_INTERPOLATION 1
#define NUM_SAMPLE_OPTIONS 2
static char *sample_option_alternatives[NUM_SAMPLE_OPTIONS] =
{
    "data_copy",
    "linear_interpolation"
};
static unsigned int sample_option = SAMPLE_OPTION_DATA_COPY;


/*  Private functions  */
STATIC_FUNCTION (flag kregrid, (char *command, FILE *fp) );
STATIC_FUNCTION (void regrid_file,
		 (CONST char *infile, CONST char *grid_file) );
STATIC_FUNCTION (flag iarray_regrid_2D,
		 (iarray out_arr, iarray in_arr, unsigned int sample_option) );
STATIC_FUNCTION (flag iarray_regrid_3D,
		 (iarray out_arr, iarray in_arr, unsigned int sample_option) );


/*  Public functions follow  */

int main (int argc, char **argv)
{
    KControlPanel panel;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "sample_option", "how to sample data",
		    PIT_CHOICE_INDEX, &sample_option,
		    PIA_NUM_CHOICE_STRINGS, NUM_SAMPLE_OPTIONS,
		    PIA_CHOICE_STRINGS, sample_option_alternatives,
		    PIA_END);
    panel_add_item (panel, "grid_file", "", K_VSTRING, (char *) &grid_file,
		    PIA_END);
    panel_push_onto_stack (panel);
    im_register_lib_version (KARMA_VERSION);
    module_run (argc, argv, "kregrid", VERSION, kregrid, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag kregrid (p, fp)
char *p;
FILE *fp;
{
    char *infile;
    extern char *grid_file;

    if ( (grid_file == NULL) || (*grid_file == '\0') )
    {
	(void) fprintf (stderr, "No grid_file specified\n");
	return (TRUE);
    }
    for ( ; p != NULL; p = ex_word_skip (p) )
    {
	if ( ( infile = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp, "Error extracting arrayfile name\n");
	    return (TRUE);
	}
	regrid_file (infile, grid_file);
	m_free (infile);
    }
    return (TRUE);
}   /*  End Function kregrid  */

static void regrid_file (CONST char *infile, CONST char *grid_file)
/*  [SUMMARY] Regrid a file to the grid of another file.
    <infile> The file to regrid.
    <grid_file> The file containing the desired grid.
    [RETURNS] Nothing.
*/
{
    iarray in_arr, grid_arr, out_arr;
    flag ok;
    unsigned int ftype, in_dim, grid_dim;
    double first_coord, last_coord;
    unsigned long dim_lengths[3];
    multi_array *multi_desc;
    CONST char *dim_names[3];
    char txt[STRING_LENGTH];
    extern unsigned int sample_option;

    /*  Read input array  */
    if ( ( multi_desc = foreign_guess_and_read (infile, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading file: \"%s\"\n", infile);
	return;
    }
    /*  Try to get iarray  */
    if ( ( in_arr = iarray_get_from_multi_array (multi_desc, NULL, 0,
						 NULL, NULL) )
	 == NULL )
    {
	(void) fprintf (stderr, "Could not find iarray\n");
	ds_dealloc_multi (multi_desc);
	return;
    }
    ds_dealloc_multi (multi_desc);
    in_dim = iarray_num_dim (in_arr);
    if ( (in_dim < 2) || (in_dim > 3) )
    {
	(void) fprintf (stderr, "Unsupported number of dimensions: %u\n",
			in_dim);
	iarray_dealloc (in_arr);
	return;
    }
    /*  Read grid array  */
    if ( ( multi_desc = foreign_guess_and_read (grid_file, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading file: \"%s\"\n", grid_file);
	iarray_dealloc (in_arr);
	return;
    }
    /*  Try to get array  */
    if ( ( grid_arr = iarray_get_from_multi_array (multi_desc, NULL, 0,
						   NULL, NULL) )
	 == NULL )
    {
	(void) fprintf (stderr, "Could not find iarray\n");
	iarray_dealloc (in_arr);
	ds_dealloc_multi (multi_desc);
	return;
    }
    ds_dealloc_multi (multi_desc);
    grid_dim = iarray_num_dim (grid_arr);
    if (grid_dim < 2)
    {
	(void) fprintf (stderr, "Unsupported number of dimensions: %u\n",
			grid_dim);
	iarray_dealloc (in_arr);
	iarray_dealloc (grid_arr);
	return;
    }
    /*  Create output array with same grid as grid file  */
    dim_lengths[in_dim - 2] = iarray_dim_length (grid_arr, grid_dim - 2);
    dim_lengths[in_dim - 1] = iarray_dim_length (grid_arr, grid_dim - 1);
    dim_names[in_dim - 2] = iarray_dim_name (grid_arr, grid_dim - 2);
    dim_names[in_dim - 1] = iarray_dim_name (grid_arr, grid_dim - 1);
    if (in_dim == 3)
    {
	dim_lengths[0] = iarray_dim_length (in_arr, 0);
	dim_names[0] = iarray_dim_name (in_arr, 0);
    }
    if ( ( out_arr = iarray_create (K_FLOAT, in_dim, dim_names, dim_lengths,
				    iarray_value_name (grid_arr), grid_arr) )
	 == NULL )
    {
	(void) fprintf (stderr, "Error creating output array\n");
	iarray_dealloc (in_arr);
	iarray_dealloc (grid_arr);
	return;
    }
    iarray_get_world_coords (grid_arr, grid_dim - 2, &first_coord,&last_coord);
    iarray_set_world_coords (out_arr, in_dim - 2, first_coord, last_coord);
    iarray_get_world_coords (grid_arr, grid_dim - 1, &first_coord,&last_coord);
    iarray_set_world_coords (out_arr, in_dim - 1, first_coord, last_coord);
    iarray_dealloc (grid_arr);
    if (in_dim == 3)
    {
	iarray_get_world_coords (in_arr, 0, &first_coord, &last_coord);
	iarray_set_world_coords (out_arr, 0, first_coord, last_coord);
	ok = iarray_regrid_3D (out_arr, in_arr, sample_option);
    }
    else ok = iarray_regrid_2D (out_arr, in_arr, sample_option);
    if (ok)
    {
	(void) sprintf (txt, "kregrid_%s", infile);
	(void) iarray_write (out_arr, txt);
    }
    iarray_dealloc (in_arr);
    iarray_dealloc (out_arr);
}   /*  End Function regrid_file  */

static flag iarray_regrid_2D (iarray out_arr, iarray in_arr,
			      unsigned int sample_option)
/*  [SUMMARY] Regrid an array.
    <out_arr> The output array. The new grid must already be defined.
    <in_arr> The input array.
    <sample_option> The sample option to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    KwcsAstro in_ap, out_ap;
    int toobig_count;
    unsigned int xlen, ylen, x, y;
    float val_00, val_10, val_01, val_11;
    float xg, yg, x0, x1, y0, y1, dx, dy, dx_r, dy_r;
    float xmax, ymax;
    float zero = 0.0;
    float one = 1.0;
    float thres = 1e-3;
    float toobig = TOOBIG;
    double xscale, yscale;
    double half = 0.5;
    double *ra_arr, *dec_arr;
    dim_desc *xdim, *ydim;
    static char function_name[] = "iarray_regrid_2D";

    if ( (iarray_type (in_arr) != K_FLOAT) ||
	 (iarray_type (out_arr) != K_FLOAT) )
    {
	(void) fprintf (stderr, "Only floating-point arrays supported\n");
	return (FALSE);
    }
    if ( ( in_ap = wcs_astro_setup (in_arr->top_pack_desc,
				    *in_arr->top_packet) ) == NULL )
    {
	return (FALSE);
    }
    if ( ( out_ap = wcs_astro_setup (out_arr->top_pack_desc,
				     *out_arr->top_packet) ) == NULL )
    {
	wcs_astro_destroy (in_ap);
	return (FALSE);
    }
    /*  Setup co-ordinate arrays. These co-ordinate arrays are used to
	translate (integral) co-ordinate indices in the output array to
	co-ordinate indices in the input array. To do this, the co-ordinate
	arrays are filled with linear world co-ordinates of the output array,
	then converted to proper non-linear world co-ordinates using the
	projection system of the output array, then converted back to linear
	world co-ordinates using the projection system of the input array, and
	finally to co-ordinate indices in the input array  */
    xlen = iarray_dim_length (out_arr, 1);
    ylen = iarray_dim_length (out_arr, 0);
    if ( ( ra_arr = (double *) m_alloc (xlen * sizeof *ra_arr) ) == NULL )
    {
	m_abort (function_name, "RA array");
    }
    if ( ( dec_arr = (double *) m_alloc (xlen * sizeof *dec_arr) ) == NULL )
    {
	m_abort (function_name, "DEC array");
    }
    (void) fprintf (stderr, "regridding...");
    /*  Loop through the lines  */
    for (y = 0; y < ylen; ++y)
    {
	/*  Fill arrays with linear world co-ordinates for output array  */
	xdim = iarray_get_dim_desc (out_arr, 1);
	ydim = iarray_get_dim_desc (out_arr, 0);
	for (x = 0; x < xlen; ++x)
	{
	    ra_arr[x] = ds_get_coordinate (xdim, x);
	    dec_arr[x] = ds_get_coordinate (ydim, y);
	}
	/*  Convert to non-linear world co-ordinates using output projection
	    system  */
	wcs_astro_transform (out_ap, xlen,
			     ra_arr, FALSE,
			     dec_arr, FALSE,
			     NULL, FALSE,
			     0, NULL, NULL);
	/*Convert to linear world co-ordinates using input projection system */
	wcs_astro_transform (in_ap, xlen,
			     ra_arr, TRUE,
			     dec_arr, TRUE,
			     NULL, FALSE,
			     0, NULL, NULL);
	/*  Convert linear world co-ordinates to co-ordinate indices and regrid
	    data on-the-fly  */
	xdim = iarray_get_dim_desc (in_arr, 1);
	ydim = iarray_get_dim_desc (in_arr, 0);
	xscale = (double) (xdim->length - 1) /
	    (xdim->last_coord - xdim->first_coord);
	yscale = (double) (ydim->length - 1) /
	    (ydim->last_coord - ydim->first_coord);
	xmax = xdim->length - 1;
	ymax = ydim->length - 1;
	switch (sample_option)
	{
	  case SAMPLE_OPTION_DATA_COPY:
	    for (x = 0; x < xlen; ++x)
	    {
		xg = (ra_arr[x] - xdim->first_coord) * xscale + half;
		yg = (dec_arr[x] - ydim->first_coord) * yscale + half;
		if (xg < zero) xg = zero;
		else if (xg > xmax) xg = xmax;
		if (yg < zero) yg = zero;
		else if (yg > ymax) yg = ymax;
		F2 (out_arr, y, x) = F2 (in_arr, (int) yg, (int) xg);
	    }
	    break;
	  case SAMPLE_OPTION_LINEAR_INTERPOLATION:
	    for (x = 0; x < xlen; ++x)
	    {
		xg = (ra_arr[x] - xdim->first_coord) * xscale;
		if (xg < thres)
		{
		    x0 = zero;
		    x1 = one;
		    dx = zero;
		    dx_r = one;
		}
		else if (xg > xmax - thres)
		{
		    x0 = xdim->length - 2;
		    x1 = xdim->length - 1;
		    dx = one;
		    dx_r = zero;
		}
		else
		{
		    x0 = floor (xg);
		    x1 = ceil (xg);
		    dx = xg - x0;
		    dx_r = one - dx;
		}
		yg = (dec_arr[x] - ydim->first_coord) * yscale;
		if (yg < thres)
		{
		    y0 = zero;
		    y1 = one;
		    dy = zero;
		    dy_r = one;
		}
		else if (yg > ymax - thres)
		{
		    y0 = ydim->length - 2;
		    y1 = ydim->length - 1;
		    dy = one;
		    dy_r = zero;
		}
		else
		{
		    y0 = floor (yg);
		    y1 = ceil (yg);
		    dy = yg - y0;
		    dy_r = one - dy;
		}
		/*  Take contribution from 4 neighbouring points  */
		toobig_count = 0;
		if ( ( val_00 = F2 (in_arr, (int) y0, (int) x0) ) >= toobig )
		{
		    val_00 = zero;
		    ++toobig_count;
		}
		if ( ( val_10 = F2 (in_arr, (int) y0, (int) x1) ) >= toobig )
		{
		    val_10 = zero;
		    ++toobig_count;
		}
		if ( ( val_01 = F2 (in_arr, (int) y1, (int) x0) ) >= toobig )
		{
		    val_01 = zero;
		    ++toobig_count;
		}
		if ( ( val_11 = F2 (in_arr, (int) y1, (int) x1) ) >= toobig )
		{
		    val_11 = zero;
		    ++toobig_count;
		}
		if (toobig_count > 3) F2 (out_arr, y, x) = toobig;
		else F2 (out_arr, y, x) = (val_00 * dx_r * dy_r +
					   val_10 * dx * dy_r +
					   val_01 * dx_r * dy +
					   val_11 * dx * dy);
	    }
	    break;
	  default:
	    fprintf (stderr, "Illegal sample_option: %u\n", sample_option);
	    a_prog_bug (function_name);
	    break;
	}
    }
    wcs_astro_destroy (in_ap);
    wcs_astro_destroy (out_ap);
    m_free ( (char *) ra_arr );
    m_free ( (char *) dec_arr );
    (void) fprintf (stderr, "\tregridded\n");
    return (TRUE);
}   /*  End Function iarray_regrid_2D  */

static flag iarray_regrid_3D (iarray out_arr, iarray in_arr,
			      unsigned int sample_option)
/*  [SUMMARY] Regrid an array.
    <out_arr> The output array. The new grid must already be defined.
    <in_arr> The input array.
    <sample_option> The sample option to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    KwcsAstro in_ap, out_ap;
    iarray ra_arr, dec_arr;
    int toobig_count;
    unsigned int xlen, ylen, zlen, x, y, z;
    float val_00, val_10, val_01, val_11;
    float xg, yg, x0, x1, y0, y1, dx, dy, dx_r, dy_r;
    float xmax, ymax;
    float zero = 0.0;
    float one = 1.0;
    float thres = 1e-3;
    float toobig = TOOBIG;
    double xscale, yscale;
    double half = 0.5;
    dim_desc *xdim, *ydim;
    static char function_name[] = "iarray_regrid_3D";

    if ( (iarray_type (in_arr) != K_FLOAT) ||
	 (iarray_type (out_arr) != K_FLOAT) )
    {
	(void) fprintf (stderr, "Only floating-point arrays supported\n");
	return (FALSE);
    }
    if ( ( in_ap = wcs_astro_setup (in_arr->top_pack_desc,
				    *in_arr->top_packet) ) == NULL )
    {
	return (FALSE);
    }
    if ( ( out_ap = wcs_astro_setup (out_arr->top_pack_desc,
				     *out_arr->top_packet) ) == NULL )
    {
	wcs_astro_destroy (in_ap);
	return (FALSE);
    }
    /*  Setup co-ordinate arrays. These co-ordinate arrays are used to
	translate (integral) co-ordinate indices in the output array to
	co-ordinate indices in the input array. To do this, the co-ordinate
	arrays are filled with linear world co-ordinates of the output array,
	then converted to proper non-linear world co-ordinates using the
	projection system of the output array, then converted back to linear
	world co-ordinates using the projection system of the input array, and
	finally to co-ordinate indices in the input array  */
    xlen = iarray_dim_length (out_arr, 2);
    ylen = iarray_dim_length (out_arr, 1);
    zlen = iarray_dim_length (out_arr, 0);
    if ( ( ra_arr = iarray_create_2D (ylen, xlen, K_DOUBLE) ) == NULL )
    {
	m_abort (function_name, "RA array");
    }
    if ( ( dec_arr = iarray_create_2D (ylen, xlen, K_DOUBLE) ) == NULL )
    {
	m_abort (function_name, "DEC array");
    }
    (void) fprintf (stderr, "filling co-ordinate arrays...\n");
    xdim = iarray_get_dim_desc (out_arr, 2);
    ydim = iarray_get_dim_desc (out_arr, 1);
    for (y = 0; y < ylen; ++y) for (x = 0; x < xlen; ++x)
    {
	D2 (ra_arr, y, x) = ds_get_coordinate (xdim, x);
	D2 (dec_arr, y, x) = ds_get_coordinate (ydim, y);
    }
    (void) fprintf (stderr, "unprojecting...\n");
    wcs_astro_transform (out_ap, xlen * ylen,
			 (double *) ra_arr->data, FALSE,
			 (double *) dec_arr->data, FALSE,
			 NULL, FALSE,
			 0, NULL, NULL);
    (void) fprintf (stderr, "reprojecting...\n");
    wcs_astro_transform (in_ap, xlen * ylen,
			 (double *) ra_arr->data, TRUE,
			 (double *) dec_arr->data, TRUE,
			 NULL, FALSE,
			 0, NULL, NULL);
    (void) fprintf (stderr, "converting to co-ordinate indices...\n");
    xdim = iarray_get_dim_desc (in_arr, 2);
    ydim = iarray_get_dim_desc (in_arr, 1);
    xscale = (double) (xdim->length - 1) /
	(xdim->last_coord - xdim->first_coord);
    yscale = (double) (ydim->length - 1) /
	    (ydim->last_coord - ydim->first_coord);
    xmax = xdim->length - 1;
    ymax = ydim->length - 1;
    for (y = 0; y < ylen; ++y) for (x = 0; x < xlen; ++x)
    {
	D2 (ra_arr, y, x) = (D2 (ra_arr, y, x) - xdim->first_coord) * xscale +
	    half;
	D2 (dec_arr, y, x) = (D2 (dec_arr, y, x) - ydim->first_coord) * yscale
	    + half;
    }
    fprintf (stderr, "regridding");
    /*  Loop through the planes  */
    for (z = 0; z < zlen; ++z)
    {
	switch (sample_option)
	{
	  case SAMPLE_OPTION_DATA_COPY:
	    for (y = 0; y < ylen; ++y) for (x = 0; x < xlen; ++x)
	    {
		xg = D2 (ra_arr, y, x);
		yg = D2 (dec_arr, y, x);
		if (xg < zero) xg = zero;
		else if (xg > xmax) xg = xmax;
		if (yg < zero) yg = zero;
		else if (yg > ymax) yg = ymax;
		F3 (out_arr, z, y, x) = F3 (in_arr, z, (int) yg, (int) xg);
	    }
	    break;
	  case SAMPLE_OPTION_LINEAR_INTERPOLATION:
	    for (y = 0; y < ylen; ++y) for (x = 0; x < xlen; ++x)
	    {
		xg = D2 (ra_arr, y, x);
		if (xg < thres)
		{
		    x0 = zero;
		    x1 = one;
		    dx = zero;
		    dx_r = one;
		}
		else if (xg > xmax - thres)
		{
		    x0 = xdim->length - 2;
		    x1 = xdim->length - 1;
		    dx = one;
		    dx_r = zero;
		}
		else
		{
		    x0 = floor (xg);
		    x1 = ceil (xg);
		    dx = xg - x0;
		    dx_r = one - dx;
		}
		yg = D2 (dec_arr, y, x);
		if (yg < thres)
		{
		    y0 = zero;
		    y1 = one;
		    dy = zero;
		    dy_r = one;
		}
		else if (yg > ymax - thres)
		{
		    y0 = ydim->length - 2;
		    y1 = ydim->length - 1;
		    dy = one;
		    dy_r = zero;
		}
		else
		{
		    y0 = floor (yg);
		    y1 = ceil (yg);
		    dy = yg - y0;
		    dy_r = one - dy;
		}
		/*  Take contribution from 4 neighbouring points  */
		toobig_count = 0;
		if ( ( val_00 = F3 (in_arr, z, (int) y0, (int) x0) ) >=toobig )
		{
		    val_00 = zero;
		    ++toobig_count;
		}
		if ( ( val_10 = F3 (in_arr, z, (int) y0, (int) x1) ) >=toobig )
		{
		    val_10 = zero;
		    ++toobig_count;
		}
		if ( ( val_01 = F3 (in_arr, z, (int) y1, (int) x0) ) >=toobig )
		{
		    val_01 = zero;
		    ++toobig_count;
		}
		if ( ( val_11 = F3 (in_arr, z, (int) y1, (int) x1) ) >=toobig )
		{
		    val_11 = zero;
		    ++toobig_count;
		}
		if (toobig_count > 3) F3 (out_arr, z, y, x) = toobig;
		else F3 (out_arr, z, y, x) = (val_00 * dx_r * dy_r +
					   val_10 * dx * dy_r +
					   val_01 * dx_r * dy +
					   val_11 * dx * dy);
	    }
	    break;
	  default:
	    fprintf (stderr, "Illegal sample_option: %u\n", sample_option);
	    a_prog_bug (function_name);
	    break;
	}
    }
    wcs_astro_destroy (in_ap);
    wcs_astro_destroy (out_ap);
    iarray_dealloc (ra_arr);
    iarray_dealloc (dec_arr);
    (void) fprintf (stderr, "\tregridded\n");
    return (TRUE);
}   /*  End Function iarray_regrid_3D  */

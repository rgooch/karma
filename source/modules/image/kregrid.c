/*  kregrid.c

    Source file for  kregrid  (Regrid file according to another file's grid).

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

    Updated by      Richard Gooch   20-NOV-1996: Trap for pixels outside
  defined co-ordinate system.

    Updated by      Richard Gooch   22-NOV-1996: Write blanks when outside
  input image boundaries, rather than clipping to "nearest" input value.

    Updated by      Richard Gooch   23-NOV-1996: Added mosaicing support.

    Updated by      Richard Gooch   24-NOV-1996: Improved speed by only
  regridding area(s) corresponding to input image(s). Especially good for
  mosaicing.

    Last updated by Richard Gooch   26-NOV-1996: Added manual grid
  specification.


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_module.h>
#include <karma_iarray.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_wcs.h>
#include <karma_chs.h>
#include <karma_ds.h>
#include <karma_ch.h>
#include <karma_ex.h>
#include <karma_im.h>
#include <karma_m.h>
#include <karma_a.h>


#define VERSION "1.5"
#define MIN_DIMENSIONS 2
#define MAX_DIMENSIONS 3


/*  Private data  */
static char *grid_file = NULL;
static flag mosaic = FALSE;

#define SAMPLE_OPTION_DATA_COPY 0
#define SAMPLE_OPTION_LINEAR_INTERPOLATION 1
#define NUM_SAMPLE_OPTIONS 2
static char *sample_option_alternatives[NUM_SAMPLE_OPTIONS] =
{
    "data_copy",
    "linear_interpolation"
};
static char *sample_option_comments[NUM_SAMPLE_OPTIONS] =
{
    "copy nearest data value",
    "bi-linear interpolation",
};
static unsigned int sample_option = SAMPLE_OPTION_DATA_COPY;

static unsigned int num_dimensions = 2;
static char *names[MAX_DIMENSIONS] = {"RA---SIN", "DEC--SIN"};
static unsigned long lengths[MAX_DIMENSIONS] = {512, 512};
static double crval[MAX_DIMENSIONS] = {20.0, -34.0};
static double crpix[MAX_DIMENSIONS] = {256.0, 256.0};
static double cdelt[MAX_DIMENSIONS] = {-0.1, 0.1};
static double crota = 0.0;
static char *unit_name = "JY/BEAM";
static flag manual_grid = FALSE;


/*  Private functions  */
STATIC_FUNCTION (flag kregrid, (char *command, FILE *fp) );
STATIC_FUNCTION (void regrid_one_file,
		 (CONST char *infile, CONST char *grid_file) );
STATIC_FUNCTION (void regrid_many_files,
		 (CONST char *infile, CONST char *grid_file) );
STATIC_FUNCTION (iarray create_from_gridfile,
		 (CONST char *file, iarray in, KwcsAstro *out_ap) );
STATIC_FUNCTION (iarray create_manual_grid, (iarray in, KwcsAstro *out_ap) );
STATIC_FUNCTION (flag iarray_regrid_2D,
		 (iarray out_arr, KwcsAstro out_ap,
		  iarray in_arr, KwcsAstro in_ap,
		  unsigned int sample_option) );
STATIC_FUNCTION (flag iarray_regrid_3D,
		 (iarray out_arr, KwcsAstro out_ap,
		  iarray in_arr, KwcsAstro in_ap,
		  unsigned int sample_option) );
STATIC_FUNCTION (void compute_region,
		 (iarray out_arr, KwcsAstro out_ap,
		  iarray in_arr, KwcsAstro in_ap,
		  unsigned int *startx, unsigned int *stopx,
		  unsigned int *starty, unsigned int *stopy) );


/*  Public functions follow  */

int main (int argc, char **argv)
{
    KControlPanel panel, group;
    static char function_name[] = "main";

    if ( ( group = panel_create_group () ) == NULL )
    {
	m_abort (function_name, "group panel");
    }
    panel_add_item (group, "cdelt", "pixel increment", K_DOUBLE,
		    (char *) cdelt,
		    PIA_END);
    panel_add_item (group, "crpix", "reference pixel position", K_DOUBLE,
		    (char *) crpix,
		    PIA_END);
    panel_add_item (group, "crval", "reference value", K_DOUBLE,
		    (char *) crval,
		    PIA_END);
    panel_add_item (group, "length", "length of axis", K_ULONG,
		    (char *) lengths,
		    PIA_END);
    panel_add_item (group, "name", "name of axis", K_VSTRING, (char *) names,
		    PIA_END);
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "unit_name", "used for manual grid specification",
		    K_VSTRING, &unit_name,
		    PIA_END);
    panel_add_item (panel, "sample_option", "how to sample data",
		    PIT_CHOICE_INDEX, &sample_option,
		    PIA_NUM_CHOICE_STRINGS, NUM_SAMPLE_OPTIONS,
		    PIA_CHOICE_STRINGS, sample_option_alternatives,
		    PIA_CHOICE_COMMENTS, sample_option_comments,
		    PIA_END);
    panel_add_item (panel, "mosaic", "input file contains a list of files",
		    PIT_FLAG, &mosaic,
		    PIA_END);
    panel_add_item (panel, "manual_grid", "use the manual grid specification",
		    PIT_FLAG, &manual_grid,
		    PIA_END);
    panel_add_item (panel, "grid_file", "file containing desired grid",
		    K_VSTRING, (char *) &grid_file,
		    PIA_END);
    panel_add_item (panel, "crota", "rotation angle", K_DOUBLE,
		    (char *) &crota,
		    PIA_END);
    panel_add_item (panel, "dimensions", "manual grid specification",
		    PIT_GROUP, (char *) group,
		    PIA_ARRAY_MIN_LENGTH, MIN_DIMENSIONS,
		    PIA_ARRAY_MAX_LENGTH, MAX_DIMENSIONS,
		    PIA_ARRAY_LENGTH, (char *) &num_dimensions,
		    PIA_END);
    panel_push_onto_stack (panel);
    panel_set_extra_comment_string (panel, "\nGeneral usage: infile");
    im_register_lib_version (KARMA_VERSION);
    module_run (argc, argv, "kregrid", VERSION, kregrid, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag kregrid (char *p, FILE *fp)
{
    char *infile;
    extern char *grid_file;

    for ( ; p != NULL; p = ex_word_skip (p) )
    {
	if ( ( infile = ex_str (p, &p) ) == NULL )
	{
	    fprintf (fp, "Error extracting arrayfile name\n");
	    return (TRUE);
	}
	if (mosaic) regrid_many_files (infile, grid_file);
	else regrid_one_file (infile, grid_file);
	m_free (infile);
    }
    return (TRUE);
}   /*  End Function kregrid  */

static void regrid_one_file (CONST char *infile, CONST char *grid_file)
/*  [SUMMARY] Regrid a file to the grid of another file.
    <infile> The file to regrid.
    <grid_file> The file containing the desired grid.
    [RETURNS] Nothing.
*/
{
    KwcsAstro in_ap = NULL;
    KwcsAstro out_ap;
    iarray in_arr = NULL;
    iarray out_arr;
    flag ok;
    unsigned int ftype, in_dim;
    char txt[STRING_LENGTH];
    extern unsigned int sample_option;

    /*  Read input array  */
    if ( !foreign_read_and_setup (infile, K_CH_MAP_IF_AVAILABLE, FALSE,
				  &ftype, TRUE, 0, K_FLOAT, TRUE,
				  &in_arr, NULL, NULL,
				  TRUE, &in_ap) ) return;
    if (in_ap == NULL)
    {
	fprintf (stderr, "File: \"%s\" has no astronomical projection\n",
		 infile);
	iarray_dealloc (in_arr);
	return;
    }
    in_dim = iarray_num_dim (in_arr);
    if ( (in_dim < 2) || (in_dim > 3) )
    {
	fprintf (stderr, "Unsupported number of dimensions: %u\n", in_dim);
	iarray_dealloc (in_arr);
	wcs_astro_destroy (in_ap);
	return;
    }
    if (manual_grid) out_arr = create_manual_grid (in_arr, &out_ap);
    else out_arr = create_from_gridfile (grid_file, in_arr, &out_ap);
    if (out_arr == NULL)
    {
	iarray_dealloc (in_arr);
	wcs_astro_destroy (in_ap);
	return;
    }
    if (in_dim == 3) ok = iarray_regrid_3D (out_arr, out_ap, in_arr, in_ap,
					    sample_option);
    else ok = iarray_regrid_2D (out_arr, out_ap, in_arr, in_ap, sample_option);
    if (ok)
    {
	sprintf (txt, "kregrid_%s", infile);
	iarray_write (out_arr, txt);
    }
    iarray_dealloc (in_arr);
    wcs_astro_destroy (in_ap);
    iarray_dealloc (out_arr);
    wcs_astro_destroy (out_ap);
}   /*  End Function regrid_one_file  */

static void regrid_many_files (CONST char *infile, CONST char *grid_file)
/*  [SUMMARY] Regrid many files to the grid of another file.
    <infile> The file containing the list of file to regrid.
    <grid_file> The file containing the desired grid.
    [RETURNS] Nothing.
*/
{
    KwcsAstro in_ap = NULL;
    KwcsAstro out_ap;
    Channel ch;
    iarray in_arr = NULL;
    iarray out_arr;
    flag ok;
    unsigned int ftype, num_dim, len;
    char txt[STRING_LENGTH];
    extern unsigned int sample_option;

    if ( ( ch = ch_open_file (infile, "r") ) == NULL ) return;
    if (manual_grid) out_arr = create_manual_grid (in_arr, &out_ap);
    else out_arr = create_from_gridfile (grid_file, NULL, &out_ap);
    if (out_arr == NULL)
    {
	iarray_dealloc (in_arr);
	return;
    }
    num_dim = iarray_num_dim (out_arr);
    while ( ( len = chs_get_value (ch, txt, STRING_LENGTH) ) > 0 )
    {
	txt[len] = '\0';
	fprintf (stderr, "Processing file: \"%s\"\n", txt);
	if ( !foreign_read_and_setup (txt, K_CH_MAP_IF_AVAILABLE, FALSE,
				      &ftype, TRUE, num_dim, K_FLOAT, TRUE,
				      &in_arr, NULL, NULL,
				      TRUE, &in_ap) )
	{
	    ch_close (ch);
	    iarray_dealloc (out_arr);
	    wcs_astro_destroy (out_ap);
	    return;
	}
	if (in_ap == NULL)
	{
	    fprintf (stderr, "File: \"%s\" has no astronomical projection\n",
		     txt);
	    ch_close (ch);
	    iarray_dealloc (out_arr);
	    wcs_astro_destroy (out_ap);
	    iarray_dealloc (in_arr);
	    return;
	}
	if (iarray_num_dim (in_arr) != num_dim)
	{
	    fprintf (stderr, "File: \"%s\" has %u dimensions, should be: %u\n",
		     txt, iarray_num_dim (in_arr), num_dim);
	    ch_close (ch);
	    iarray_dealloc (out_arr);
	    iarray_dealloc (in_arr);
	    wcs_astro_destroy (out_ap);
	    wcs_astro_destroy (in_ap);
	    return;
	}
	if (num_dim == 3) ok = iarray_regrid_3D (out_arr, out_ap, in_arr,in_ap,
						 sample_option);
	else ok = iarray_regrid_2D (out_arr, out_ap, in_arr, in_ap,
				    sample_option);
	if (!ok)
	{
	    ch_close (ch);
	    iarray_dealloc (out_arr);
	    iarray_dealloc (in_arr);
	    wcs_astro_destroy (out_ap);
	    wcs_astro_destroy (in_ap);
	    return;
	}
	iarray_dealloc (in_arr);
	in_arr = NULL;
	wcs_astro_destroy (in_ap);
	in_ap = NULL;
    }
    wcs_astro_destroy (out_ap);
    sprintf (txt, "kregrid_%s", infile);
    iarray_write (out_arr, txt);
    iarray_dealloc (out_arr);
}   /*  End Function regrid_many_files  */

static iarray create_from_gridfile (CONST char *file, iarray in,
				    KwcsAstro *out_ap)
/*  [SUMMARY] Create an array using a gridfile as a template.
    <file> The template grid file.
    <in> The input file used to control the z dimension (if it exists). This
    may be NULL.
    <out_ap> The KwcsAstro object for the output array is written here.
    [RETURNS] The array on success, else NULL.
*/
{
    KwcsAstro ap = NULL;
    iarray new;
    iarray grid = NULL;
    unsigned int dummy, in_dim, grid_dim;
    double first_coord, last_coord;
    unsigned long dim_lengths[3];
    CONST char *dim_names[3];

    if ( (file == NULL) || (*file == '\0') )
    {
	fprintf (stderr, "No grid_file specified\n");
	return (NULL);
    }
    if ( !foreign_read_and_setup (file, K_CH_MAP_IF_AVAILABLE, FALSE, &dummy,
				  TRUE, 0, NONE, FALSE, &grid, NULL, NULL,
				  FALSE, &ap) ) return (NULL);
    if (ap == NULL)
    {
	fprintf (stderr, "Grid file has no astronomical projection\n");
	iarray_dealloc (grid);
	return (NULL);
    }
    wcs_astro_destroy (ap);
    grid_dim = iarray_num_dim (grid);
    if ( (grid_dim != 2) && (grid_dim != 3) )
    {
	fprintf (stderr, "Unsupported number of grid dimensions: %u\n",
		 grid_dim);
	iarray_dealloc (grid);
	return (NULL);
    }
    if ( (in == NULL) || (iarray_num_dim (in) == 2) )
    {
	/*  Take all from grid file  */
	fprintf (stderr, "creating...");
	new = iarray_create_from_template (grid, K_FLOAT, TRUE, TRUE, TRUE);
	iarray_dealloc (grid);
	fprintf (stderr, "\tfilling...");
	iarray_fill_float (new, TOOBIG);
	fprintf (stderr, "\tfilled\n");
	if ( ( *out_ap = wcs_astro_setup (new->top_pack_desc,
					  *new->top_packet) ) == NULL )
	{
	    iarray_dealloc (new);
	    return (NULL);
	}
	return (new);
    }
    /*  Take x & y dimensions from grid file and z dimension from input file */
    in_dim = iarray_num_dim (in);
    /*  Create output array with same grid as grid file  */
    dim_lengths[in_dim - 2] = iarray_dim_length (grid, grid_dim - 2);
    dim_lengths[in_dim - 1] = iarray_dim_length (grid, grid_dim - 1);
    dim_names[in_dim - 2] = iarray_dim_name (grid, grid_dim - 2);
    dim_names[in_dim - 1] = iarray_dim_name (grid, grid_dim - 1);
    if (in_dim == 3)
    {
	dim_lengths[0] = iarray_dim_length (in, 0);
	dim_names[0] = iarray_dim_name (in, 0);
    }
    fprintf (stderr, "creating...");
    if ( ( new = iarray_create (K_FLOAT, in_dim, dim_names, dim_lengths,
				iarray_value_name (grid), grid) ) == NULL )
    {
	fprintf (stderr, "Error creating output array\n");
	iarray_dealloc (grid);
	return (NULL);
    }
    iarray_dealloc (grid);
    fprintf (stderr, "\tfilling...");
    iarray_fill_float (new, TOOBIG);
    fprintf (stderr, "\tfilled\n");
    if (in_dim == 3)
    {
	iarray_get_world_coords (in, 0, &first_coord, &last_coord);
	iarray_set_world_coords (new, 0, first_coord, last_coord);
    }
    if ( ( *out_ap = wcs_astro_setup (new->top_pack_desc,
				      *new->top_packet) ) == NULL )
    {
	iarray_dealloc (new);
	return (NULL);
    }
    return (new);
}   /*  End Function create_from_gridfile  */

static iarray create_manual_grid (iarray in, KwcsAstro *out_ap)
/*  [SUMMARY] Create an array using a manual grid as a template.
    <in> The input file used to control the z dimension (if it exists). This
    may be NULL.
    <out_ap> The KwcsAstro object for the output array is written here.
    [RETURNS] The array on success, else NULL.
*/
{
    iarray new;
    unsigned int in_dim = 0;  /*  Initialised to keep compiler happy  */
    unsigned int count, new_dim;
    double first_coord, last_coord;
    unsigned long dim_lengths[3];
    CONST char *dim_names[3];
    char txt[STRING_LENGTH];
    double value[2];

    if (in == NULL) new_dim = num_dimensions;
    else
    {
	in_dim = iarray_num_dim (in);
	new_dim = in_dim;
    }
    /*  Setup the x,y (RA,DEC) dimensions  */
    dim_lengths[new_dim - 1] = lengths[0];
    dim_lengths[new_dim - 2] = lengths[1];
    dim_names[new_dim - 1] = names[0];
    dim_names[new_dim - 2] = names[1];
    if (new_dim == 3)
    {
	if (in == NULL)
	{
	    dim_lengths[0] = lengths[2];
	    dim_names[0] = names[2];
	}
	else
	{
	    dim_lengths[0] = iarray_dim_length (in, 0);
	    dim_names[0] = iarray_dim_name (in, 0);
	}
    }
    fprintf (stderr, "creating...");
    if ( ( new = iarray_create (K_FLOAT, new_dim, dim_names, dim_lengths,
				unit_name, NULL) ) == NULL )
    {
	fprintf (stderr, "Error creating output array\n");
	return (NULL);
    }
    fprintf (stderr, "\tfilling...");
    iarray_fill_float (new, TOOBIG);
    fprintf (stderr, "\tfilled\n");
    if ( (in != NULL) && (in_dim == 3) )
    {
	iarray_get_world_coords (in, 0, &first_coord, &last_coord);
	iarray_set_world_coords (new, 0, first_coord, last_coord);
    }
    /*  Add header information  */
    value[1] = 0.0;
    for (count = 0; (count < new_dim) && (count < num_dimensions); ++count)
    {
	sprintf (txt, "CTYPE%u", count + 1);
	if ( !iarray_put_named_string (new, txt, names[count]) )
	{
	    iarray_dealloc (new);
	    return (NULL);
	}
	sprintf (txt, "CRVAL%u", count + 1);
	value[0] = crval[count];
	if ( !iarray_put_named_value (new, txt, K_DOUBLE, value) )
	{
	    iarray_dealloc (new);
	    return (NULL);
	}
	sprintf (txt, "CRPIX%u", count + 1);
	value[0] = crpix[count];
	if ( !iarray_put_named_value (new, txt, K_DOUBLE, value) )
	{
	    iarray_dealloc (new);
	    return (NULL);
	}
	sprintf (txt, "CDELT%u", count + 1);
	value[0] = cdelt[count];
	if ( !iarray_put_named_value (new, txt, K_DOUBLE, value) )
	{
	    iarray_dealloc (new);
	    return (NULL);
	}
    }
    value[0] = crota;
    if ( !iarray_put_named_value (new, "CROTA2", K_DOUBLE, value) )
    {
	iarray_dealloc (new);
	return (NULL);
    }
    if ( ( *out_ap = wcs_astro_setup (new->top_pack_desc,
				      *new->top_packet) ) == NULL )
    {
	fprintf (stderr, "Error creating KwcsAstro object for output\n");
	iarray_dealloc (new);
	return (NULL);
    }
    return (new);
}   /*  End Function create_manual_grid  */

static flag iarray_regrid_2D (iarray out_arr, KwcsAstro out_ap,
			      iarray in_arr, KwcsAstro in_ap,
			      unsigned int sample_option)
/*  [SUMMARY] Regrid an array.
    <out_arr> The output array. The new grid must already be defined.
    <out_ap> The output KwcsAstro object.
    <in_arr> The input array.
    <n_ap> The input KwcsAstro object.
    <sample_option> The sample option to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    int toobig_count;
    unsigned int xlen, ylen, x, y;
    unsigned int startx, stopx, starty, stopy;
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
	fprintf (stderr, "Only floating-point arrays supported\n");
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
    compute_region (out_arr, out_ap, in_arr, in_ap, &startx, &stopx,
		    &starty, &stopy);
    xlen = stopx - startx;
    ylen = stopy - starty;
    if ( ( ra_arr = (double *) m_alloc (xlen * sizeof *ra_arr) ) == NULL )
    {
	m_abort (function_name, "RA array");
    }
    if ( ( dec_arr = (double *) m_alloc (xlen * sizeof *dec_arr) ) == NULL )
    {
	m_abort (function_name, "DEC array");
    }
    fprintf (stderr, "regridding...");
    /*  Loop through the lines  */
    for (y = starty; y < stopy; ++y)
    {
	/*  Fill arrays with linear world co-ordinates for output array  */
	xdim = iarray_get_dim_desc (out_arr, 1);
	ydim = iarray_get_dim_desc (out_arr, 0);
	for (x = startx; x < stopx; ++x)
	{
	    ra_arr[x - startx] = ds_get_coordinate (xdim, x);
	    dec_arr[x - startx] = ds_get_coordinate (ydim, y);
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
	    for (x = startx; x < stopx; ++x)
	    {
		xg = ra_arr[x - startx];
		yg = dec_arr[x - startx];
		if ( (xg >= toobig) || (yg >= toobig) ) continue;
		xg = (xg - xdim->first_coord) * xscale + half;
		yg = (yg - ydim->first_coord) * yscale + half;
		if ( (xg < zero) || (xg > xmax) || (yg < zero) || (yg > ymax) )
		    continue;
		F2 (out_arr, y, x) = F2 (in_arr, (int) yg, (int) xg);
	    }
	    break;
	  case SAMPLE_OPTION_LINEAR_INTERPOLATION:
	    for (x = startx; x < stopx; ++x)
	    {
		xg = ra_arr[x - startx];
		yg = dec_arr[x - startx];
		if ( (xg >= toobig) || (yg >= toobig) ) continue;
		xg = (xg - xdim->first_coord) * xscale;
		yg = (yg - ydim->first_coord) * yscale;
		if ( (xg < thres) || (xg > xmax - thres) ||
		     (yg < thres) || (yg > ymax - thres) ) continue;
		x0 = floor (xg);
		x1 = ceil (xg);
		dx = xg - x0;
		dx_r = one - dx;
		y0 = floor (yg);
		y1 = ceil (yg);
		dy = yg - y0;
		dy_r = one - dy;
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
		if (toobig_count > 3) continue;
		F2 (out_arr, y, x) = (val_00 * dx_r * dy_r +
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
    m_free ( (char *) ra_arr );
    m_free ( (char *) dec_arr );
    fprintf (stderr, "\tregridded\n");
    return (TRUE);
}   /*  End Function iarray_regrid_2D  */

static flag iarray_regrid_3D (iarray out_arr, KwcsAstro out_ap,
			      iarray in_arr, KwcsAstro in_ap,
			      unsigned int sample_option)
/*  [SUMMARY] Regrid an array.
    <out_arr> The output array. The new grid must already be defined.
    <out_ap> The output KwcsAstro object.
    <in_arr> The input array.
    <in_ap> The input KwcsAstro object.
    <sample_option> The sample option to use.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    iarray ra_arr, dec_arr;
    int toobig_count;
    unsigned int xlen, ylen, zlen, x, y, z;
    unsigned int startx, stopx, starty, stopy;
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
	fprintf (stderr, "Only floating-point arrays supported\n");
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
    zlen = iarray_dim_length (out_arr, 0);
    compute_region (out_arr, out_ap, in_arr, in_ap, &startx, &stopx,
		    &starty, &stopy);
    xlen = stopx - startx;
    ylen = stopy - starty;
    if ( ( ra_arr = iarray_create_2D (ylen, xlen, K_DOUBLE) ) == NULL )
    {
	m_abort (function_name, "RA array");
    }
    if ( ( dec_arr = iarray_create_2D (ylen, xlen, K_DOUBLE) ) == NULL )
    {
	m_abort (function_name, "DEC array");
    }
    fprintf (stderr, "filling co-ordinate arrays...\n");
    xdim = iarray_get_dim_desc (out_arr, 2);
    ydim = iarray_get_dim_desc (out_arr, 1);
    for (y = 0; y < ylen; ++y) for (x = 0; x < xlen; ++x)
    {
	D2 (ra_arr, y, x) = ds_get_coordinate (xdim, startx + x);
	D2 (dec_arr, y, x) = ds_get_coordinate (ydim, starty + y);
    }
    fprintf (stderr, "unprojecting...\n");
    wcs_astro_transform (out_ap, xlen * ylen,
			 (double *) ra_arr->data, FALSE,
			 (double *) dec_arr->data, FALSE,
			 NULL, FALSE,
			 0, NULL, NULL);
    fprintf (stderr, "reprojecting...\n");
    wcs_astro_transform (in_ap, xlen * ylen,
			 (double *) ra_arr->data, TRUE,
			 (double *) dec_arr->data, TRUE,
			 NULL, FALSE,
			 0, NULL, NULL);
    fprintf (stderr, "converting to co-ordinate indices...\n");
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
	    for (y = starty; y < stopy; ++y) for (x = startx; x < stopx; ++x)
	    {
		xg = D2 (ra_arr, y - starty, startx);
		yg = D2 (dec_arr, y - starty, x - startx);
		if ( (xg >= toobig) || (yg >= toobig) ) continue;
		if ( (xg < zero) || (xg > xmax) || (yg < zero) || (yg > ymax) )
		    continue;
		F3 (out_arr, z, y, x) = F3 (in_arr, z, (int) yg, (int) xg);
	    }
	    break;
	  case SAMPLE_OPTION_LINEAR_INTERPOLATION:
	    for (y = starty; y < stopy; ++y) for (x = startx; x < stopx; ++x)
	    {
		xg = D2 (ra_arr, y - starty, startx);
		yg = D2 (dec_arr, y - starty, x - startx);
		if ( (xg >= toobig) || (yg >= toobig) ) continue;
		xg = (xg - xdim->first_coord) * xscale;
		yg = (yg - ydim->first_coord) * yscale;
		if ( (xg < thres) || (xg > xmax - thres) ||
		     (yg < thres) || (yg > ymax - thres) ) continue;
		x0 = floor (xg);
		x1 = ceil (xg);
		dx = xg - x0;
		dx_r = one - dx;
		y0 = floor (yg);
		y1 = ceil (yg);
		dy = yg - y0;
		dy_r = one - dy;
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
		if (toobig_count > 3) continue;
		F3 (out_arr, z, y, x) = (val_00 * dx_r * dy_r +
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
    iarray_dealloc (ra_arr);
    iarray_dealloc (dec_arr);
    fprintf (stderr, "\tregridded\n");
    return (TRUE);
}   /*  End Function iarray_regrid_3D  */

static void compute_region (iarray out_arr, KwcsAstro out_ap,
			    iarray in_arr, KwcsAstro in_ap,
			    unsigned int *startx, unsigned int *stopx,
			    unsigned int *starty, unsigned int *stopy)
/*  [SUMMARY] Compute the region the input image covers.
    <out_arr> The output array.
    <out_ap> The output KwcsAstro object.
    <in_arr> The input array.
    <in_ap> The input KwcsAstro object.
    <startx> The starting X position in the output image.
    <stopx> The stop X position in the output image.
    <starty> The starting Y position in the output image.
    <stopy> The stop Y position in the output image.
    [RETURNS] Nothing.
*/
{
    unsigned int in_dim, out_dim, num_coords, count;
    unsigned long in_xlen, in_ylen;
    double out_xlen, out_ylen;
    double xmin = TOOBIG;
    double xmax = -TOOBIG;
    double ymin = TOOBIG;
    double ymax = -TOOBIG;
    double toobig = TOOBIG;
    double zero = 0.0;
    double ra, dec;
    double *ptr, *ra_arr, *dec_arr;
    static char function_name[] = "compute_region";

    in_dim = iarray_num_dim (in_arr);
    out_dim = iarray_num_dim (out_arr);
    in_xlen = iarray_dim_length (in_arr, in_dim - 1);
    in_ylen = iarray_dim_length (in_arr, in_dim - 2);
    out_xlen = iarray_dim_length (out_arr, out_dim - 1);
    out_ylen = iarray_dim_length (out_arr, out_dim - 2);
    num_coords = in_xlen * 2 + in_ylen * 2;
    if (out_xlen * out_ylen <= num_coords)
    {
	/*  Not likely to be profitable computing sub-region  */
	*startx = 0;
	*stopx = out_xlen;
	*starty = 0;
	*stopy = out_ylen;
	return;
    }
    ptr = (double *) m_alloc_scratch (num_coords * sizeof *ptr * 2,
				      function_name);
    ra_arr = ptr;
    dec_arr = ra_arr + num_coords;
    /*  Fill arrays with input pixel co-ordinates  */
    /*  First along Y = 0  */
    for (count = 0; count < in_xlen; ++count)
    {
	ra_arr[count] = iarray_get_coordinate (in_arr, in_dim - 1, count);
	dec_arr[count] = 0.0;
    }
    ra_arr += count;
    dec_arr += count;
    /*  Next along Y = max  */
    for (count = 0; count < in_xlen; ++count)
    {
	ra_arr[count] = iarray_get_coordinate (in_arr, in_dim - 1, count);
	dec_arr[count] = in_ylen - 1;
    }
    ra_arr += count;
    dec_arr += count;
    /*  Now along X = 0  */
    for (count = 0; count < in_ylen; ++count)
    {
	ra_arr[count] = 0.0;
	dec_arr[count] = iarray_get_coordinate (in_arr, in_dim - 2, count);
    }
    ra_arr += count;
    dec_arr += count;
    /*  Now along X = max  */
    for (count = 0; count < in_ylen; ++count)
    {
	ra_arr[count] = in_xlen - 1;
	dec_arr[count] = iarray_get_coordinate (in_arr, in_dim - 2, count);
    }
    /*  Transform into world co-ordinates  */
    ra_arr = ptr;
    dec_arr = ra_arr + num_coords;
    wcs_astro_transform (in_ap, num_coords,
			 ra_arr, FALSE, dec_arr, FALSE, NULL, FALSE,
			 0, NULL, NULL);
    /*  Transform into output pixel co-ordinates  */
    wcs_astro_transform (out_ap, num_coords,
			 ra_arr, TRUE, dec_arr, TRUE, NULL, FALSE,
			 0, NULL, NULL);
    /*  Now test the limits  */
    out_xlen -= 1.0;
    out_ylen -= 1.0;
    for (count = 0; count < num_coords; ++count)
    {
	ra = ra_arr[count];
	dec = dec_arr[count];
	if ( (ra >= toobig) || (dec >= toobig) ) continue;
	if ( (ra < zero) || (ra > out_xlen) ) continue;
	if ( (dec < zero) || (dec > out_ylen) ) continue;
	if (ra < xmin) xmin = ra;
	if (ra > xmax) xmax = ra;
	if (dec < ymin) ymin = dec;
	if (dec > ymax) ymax = dec;
    }
    *startx = floor (xmin);
    *stopx = (unsigned int) floor (xmax) + 1;
    *starty = floor (ymin);
    *stopy = (unsigned int) floor (ymax) + 1;
    m_free_scratch ();
}   /*  End Function compute_region  */

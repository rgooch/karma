/*  kmkshell.c

    Source file for  kmkshell  (generate 3-D shell in Karma format).

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

/*  This Karma module will generate the projection of a 3-dimensional expanding
    shell into RA-DEC-vel space and will write the data to a Karma data file.


    Written by      Richard Gooch   26-SEP-1996

    Last updated by Richard Gooch   6-OCT-1996: Added rotation.


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <karma_foreign.h>
#include <karma_iarray.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_im.h>
#include <karma_m.h>
#include <karma_s.h>


#define VERSION "1.1"

#define MAX_DIMENSIONS (unsigned int) 3


/*  Private functions  */
STATIC_FUNCTION (flag kmkshell, (char *p, FILE *fp) );
STATIC_FUNCTION (void generate_file,
		 (CONST char *outfile, unsigned long xlen, unsigned long ylen,
		  unsigned long zlen) );


/*  Private data  */
static unsigned long xlen = 128;
static unsigned int xcentre = 64;
static unsigned int xradius = 32;
static unsigned long ylen = 128;
static unsigned int ycentre = 64;
static unsigned int yradius = 32;
static unsigned long zlen = 128;
static unsigned int zcentre = 64;
static unsigned int zradius = 32;
static unsigned int thickness = 2;
static float centre_value = 1.0;
static float shell_value = 0.5;
static char *template_file = NULL;
static float velocity_expansion = 10.0;
static float velocity_rotation = 0.0;
static float rotation_axis_tilt = 0.0;


/*  Public functions follow  */

int main (int argc, char **argv)
{
    KControlPanel panel;
    static char function_name[] = "main";

    im_register_lib_version (KARMA_VERSION);
    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "template_file", "contains template grid",
		    K_VSTRING, (char *) &template_file,
		    PIA_END);
    panel_add_item (panel, "vel_rotation",
		    "tangential rotation velocity (km/s)",
		    K_FLOAT, (char *) &velocity_rotation,
		    PIA_END);
    panel_add_item (panel, "vel_expansion", "expansion velocity (km/s)",
		    K_FLOAT, (char *) &velocity_expansion,
		    PIA_END);
    panel_add_item (panel, "shell_value", "value in shell", K_FLOAT,
		    (char *) &shell_value,
		    PIA_END);
    panel_add_item (panel, "centre_value", "value at centre", K_FLOAT,
		    (char *) &centre_value,
		    PIA_END);
    panel_add_item (panel, "thickness", "thickness of shell", K_UINT,
		    (char *) &thickness,
		    PIA_END);
    panel_add_item (panel, "zradius", "radius of shell in Z dimension",
		    K_UINT, (char *) &zradius,
		    PIA_END);
    panel_add_item (panel, "zcentre", "centre in Z dimension", K_UINT,
		    (char *) &zcentre,
		    PIA_END);
    panel_add_item (panel, "zlen", "length of Z dimension", K_ULONG,
		    (char *) &zlen,
		    PIA_END);
    panel_add_item (panel, "yradius", "radius of shell in Y dimension",
		    K_UINT, (char *) &yradius,
		    PIA_END);
    panel_add_item (panel, "ycentre", "centre in Y dimension", K_UINT,
		    (char *) &ycentre,
		    PIA_END);
    panel_add_item (panel, "ylen", "length of Y dimension", K_ULONG,
		    (char *) &ylen,
		    PIA_END);
    panel_add_item (panel, "xradius", "radius of shell in X dimension",
		    K_UINT, (char *) &xradius,
		    PIA_END);
    panel_add_item (panel, "xcentre", "centre in X dimension", K_UINT,
		    (char *) &xcentre,
		    PIA_END);
    panel_add_item (panel, "xlen", "length of X dimension", K_ULONG,
		    (char *) &xlen,
		    PIA_END);
    panel_add_item (panel, "axis_tilt", "tilt of rotation axis (degrees)",
		    K_FLOAT, (char *) &rotation_axis_tilt,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "kmkshell", VERSION, kmkshell, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */


/*  Private functions follow  */

static flag kmkshell (char *p, FILE *fp)
{
    char *arrayfile;

    for ( ; p; p = ex_command_skip (p) )
    {
	if ( ( arrayfile = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp, "Error extracting arrayfile name\n");
	    return (TRUE);
	}
	generate_file (arrayfile, xlen, ylen, zlen);
	m_free (arrayfile);
    }
    return (TRUE);
}   /*  End Function kmkshell  */

static void generate_file (CONST char *outfile, unsigned long xlen,
			   unsigned long ylen, unsigned long zlen)
/*  [SUMMARY] Generate a cube.
    [PURPOSE] This routine will generate a Karma arrayfile with a
    3-dimensional array of a single atomic element.
    <outfile> The name of the Karma arrayfile that will be generated.
    [RETURNS] Nothing.
*/
{
    KwcsAstro ap;
    iarray cube, template_arr;
    int x, y, z;
    unsigned int ftype;
    float radius, sq_radius1, sq_radius2, distance, rx, ry, rz, dx, dy, dz;
    float min_velocity, max_velocity;
    float zrad, one_on_zradius;
    float omega;
    double velocity;
    unsigned long dim_lengths[3];
    multi_array *multi_desc;
    CONST char *dim_names[3], **dnames, *elem_name;
    extern char *template_file;
    static char function_name[] = "generate_file";

    max_velocity = fabs (velocity_expansion) + fabs (velocity_rotation);
    min_velocity = -max_velocity;
    if ( (template_file == NULL) || (*template_file == '\0') )
    {
	/*  Create plain output array  */
	dnames = NULL;
	elem_name = NULL;
	template_arr = NULL;
	ap = NULL;
    }
    else
    {
	/*  Try to load template file  */
	if ( ( multi_desc = foreign_guess_and_read (template_file,
						    K_CH_MAP_LOCAL, FALSE,
						    &ftype,
						    FA_GUESS_READ_END) )
	     == NULL )
	{
	    fprintf (stderr, "Error reading file: \"%s\"\n", template_file);
	    return;
	}
	/*  Try to get iarray  */
	if ( ( template_arr = iarray_get_from_multi_array (multi_desc, NULL, 3,
							   NULL, NULL) )
	     == NULL )
	{
	    fprintf (stderr, "Could not find iarray\n");
	    ds_dealloc_multi (multi_desc);
	    return;
	}
	ds_dealloc_multi (multi_desc);
	/*  Create output array with same grid as grid file  */
	xlen = iarray_dim_length (template_arr, 2);
	ylen = iarray_dim_length (template_arr, 1);
	zlen = iarray_dim_length (template_arr, 0);
	dim_names[2] = iarray_dim_name (template_arr, 2);
	dim_names[1] = iarray_dim_name (template_arr, 1);
	dim_names[0] = iarray_dim_name (template_arr, 0);
	dnames = dim_names;
	elem_name = iarray_value_name (template_arr);
	ap = wcs_astro_setup (template_arr->top_pack_desc,
			      *template_arr->top_packet);
    }
    if (ap != NULL)
    {
	if ( (strncmp (dim_names[0], "VELO", 4) == 0) ||
	     (strncmp (dim_names[0], "FELO", 4) == 0) )
	{
	    /*  Velocity information available  */
	    min_velocity = TOOBIG;
	    max_velocity = TOOBIG;
	}
    }
    dim_lengths[0] = zlen;
    dim_lengths[1] = ylen;
    dim_lengths[2] = xlen;
    fprintf (stderr, "Making cube %lu * %lu * %lu\n", xlen, ylen, zlen);
    if ( ( cube = iarray_create (K_FLOAT, 3, dnames, dim_lengths, elem_name,
				 template_arr) ) == NULL )
    {
	m_abort (function_name, "cube");
    }
    if (template_arr != NULL)
    {
	iarray_dealloc (template_arr);
	/*  Clear new array because <iarray_create> copies over the old data
	    when the array sizes are the same (as they are in this case)  */
	iarray_fill_float (cube, 0.0);
    }
    F3 (cube, zlen / 2, ylen / 2, xlen / 2) = centre_value;
    radius = (xradius > yradius) ? xradius : yradius;
    if (zradius > radius) radius = zradius;
    sq_radius1 = (radius - thickness) * (radius - thickness);
    sq_radius2 = (radius + thickness) * (radius + thickness);
    zrad = zradius;
    one_on_zradius = 1.0 / zrad;
    rx = radius / (float) xradius;
    ry = radius / (float) yradius;
    rz = radius * one_on_zradius;
    omega = velocity_rotation / radius;  /*  Angular velocity  */
    for (z = 0; z < zlen; ++z)
	for (y = 0; y < ylen; ++y) for (x = 0; x < xlen; ++x)
	{
	    dx = x - (int) xcentre;
	    dy = y - (int) ycentre;
	    dz = z - (int) zcentre;
	    distance = dx * dx * rx * rx;
	    distance += dy * dy * ry * ry;
	    distance += dz * dz * rz * rz;
	    if ( (distance < sq_radius1) || (distance > sq_radius2) ) continue;
	    /*  Point lies within shell: determine projected velocity  */
	    /*  First compute component from expansion velocity  */
	    velocity = dz * one_on_zradius * velocity_expansion;
	    /*  Now compute and add the component from rotation velocity  */
	    velocity += dx * omega;
	    if (max_velocity < TOOBIG)
	    {
		/*  Simple linear transformation  */
		dz = velocity / max_velocity * zrad + zcentre;
	    }
	    else
	    {
		/*  Use transformation  */
		wcs_astro_transform (ap, 1, NULL, FALSE, NULL, FALSE,
				     &velocity, TRUE, 0, NULL, NULL);
		dz = velocity;
	    }
	    if ( (dz < 0.0) || (dz >= zlen) )
	    {
		fprintf (stderr, "WARNING: z: %e\n", dz);
		continue;
	    }
	    F3 (cube, (int) dz, y, x) = shell_value;
	}
    if ( !panel_put_history_with_stack (cube->multi_desc, TRUE) )
    {
	iarray_dealloc (cube);
	return;
    }
    iarray_write (cube, outfile);
    iarray_dealloc (cube);
}   /*  End Function generate_file  */

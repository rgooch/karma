/*  kgetslice.c

    Source file for  kgetslice  (get 2-D slice from a cube).

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

/*  This Karma module will extract a 2-dimensional slice from a 3-dimensional
    data volume.


    Written by      Richard Gooch   28-FEB-1996

    Updated by      Richard Gooch   30-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   14-JUN-1996: Copied attachments and
  dimension information properly.


*/
#include <stdio.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <karma.h>
#include <karma_iarray.h>
#include <karma_module.h>
#include <karma_panel.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_ex.h>
#include <karma_m.h>
#include <karma_n.h>

#define VERSION "1.0"

STATIC_FUNCTION (flag kgetslice, (char *command, FILE *fp) );
STATIC_FUNCTION (void generate_file, (char *arrayfile) );
STATIC_FUNCTION (void view, (char *p) );


/*  Private data  */
static char *x_name = "x";
static char *y_name = "y";
static unsigned int slice_pos = 0;

int main (int argc, char **argv)
{
    KControlPanel panel;
    static char function_name[] = "main";

    if ( ( panel = panel_create (FALSE) ) == NULL )
    {
	m_abort (function_name, "control panel");
    }
    panel_add_item (panel, "view", "supply Karma filename",
		    PIT_FUNCTION, (void *) view,
		    PIA_END);
    panel_add_item (panel, "x_name", "dimension name", K_VSTRING, &x_name,
		    PIA_END);
    panel_add_item (panel, "y_name", "dimension name", K_VSTRING, &y_name,
		    PIA_END);
    panel_add_item (panel, "slice_pos", "absolute", K_UINT, &slice_pos,
		    PIA_END);
    panel_push_onto_stack (panel);
    module_run (argc, argv, "kgetslice", VERSION, kgetslice, -1, 0, FALSE);
    return (RV_OK);
}   /*  End Function main   */

static flag kgetslice (p, fp)
char *p;
FILE *fp;
{
    char *arrayfile;

    for ( ; p; p = ex_command_skip (p) )
    {
	if ( ( arrayfile = ex_str (p, &p) ) == NULL )
	{
	    (void) fprintf (fp, "Error extracting arrayfile name\n");
	    return (TRUE);
	}
	generate_file (arrayfile);
	m_free (arrayfile);
    }
    return (TRUE);
}   /*  End Function kgetslice  */

static void generate_file (char *arrayfile)
/*  [PURPOSE] This routine will generate a Karma arrayfile with a
    multi-dimensional array of a single atomic element.
    <arrayfile> The name of the Karma arrayfile.
    [RETURNS] Nothing.
*/
{
    iarray cube, slice, image;
    unsigned int xdim, ydim;
    double first_coord, last_coord;
    char outfile[STRING_LENGTH];
    CONST char *dim_names[2];
    unsigned long dim_lengths[2];
    extern unsigned int slice_pos;
    extern char *x_name;
    extern char *y_name;
    extern char *sys_errlist[];
    /*static char function_name[] = "generate_file";*/

    if ( ( cube = iarray_read_nD (arrayfile, FALSE, NULL, 3, NULL, NULL,
				  K_CH_MAP_IF_AVAILABLE) ) == NULL )
    {
	(void) fprintf (stderr, "Error loading cube\n");
	return;
    }
    if ( (xdim = iarray_dim_index (cube, x_name) ) >= iarray_num_dim (cube) )
    {
	(void) fprintf (stderr, "Dimension name: \"%s\" does not exist\n",
			x_name);
	iarray_dealloc (cube);
	return;
    }
    if ( (ydim = iarray_dim_index (cube, y_name) ) >= iarray_num_dim (cube) )
    {
	(void) fprintf (stderr, "Dimension name: \"%s\" does not exist\n",
			y_name);
	iarray_dealloc (cube);
	return;
    }
    fprintf (stderr, "x: %u  y: %u\n", xdim, ydim);
    if ( ( slice = iarray_get_2D_slice_from_3D (cube, ydim, xdim, slice_pos) )
	 == NULL )
    {
	(void) fprintf (stderr, "Error creating 2-D slice alias\n");
	iarray_dealloc (cube);
	return;
    }
    /*  Use <iarray_create> rather than <iarray_create_2D> since the former
	allows attached information to be copied from the cube. This is
	important so FITS style keywords can be preserved  */
    dim_lengths[0] = iarray_dim_length (cube, ydim);
    dim_lengths[1] = iarray_dim_length (cube, xdim);
    dim_names[0] = iarray_dim_name (cube, ydim);
    dim_names[1] = iarray_dim_name (cube, xdim);
    if ( ( image = iarray_create (iarray_type (cube),
				  2, dim_names, dim_lengths,
				  iarray_value_name (cube), cube) ) == NULL )
    {
	(void) fprintf (stderr, "Error creating image\n");
	iarray_dealloc (slice);
	iarray_dealloc (cube);
	return;
    }
    /*  Copy dimension co-ordinates  */
    iarray_get_world_coords (cube, ydim, &first_coord, &last_coord);
    iarray_set_world_coords (image, 0, first_coord, last_coord);
    iarray_get_world_coords (cube, xdim, &first_coord, &last_coord);
    iarray_set_world_coords (image, 1, first_coord, last_coord);
    if ( !iarray_copy_data (image, slice, FALSE) )
    {
	(void) fprintf (stderr, "Error copying data\n");
	iarray_dealloc (image);
	iarray_dealloc (slice);
	iarray_dealloc (cube);
	return;
    }
    iarray_dealloc (slice);
    iarray_dealloc (cube);
    sprintf (outfile, "kgetslice_%s", arrayfile);
    if ( !iarray_write (image, outfile) )
    {
	(void) fprintf (stderr, "Error writing output file: \"%s\"\n",
			outfile);
    }
    iarray_dealloc (image);
}   /*  End Function generate_file  */

static void view (char *p)
/*  [PURPOSE] This routine will view the header of an Intelligent Array.
    <p> A string containing the file to load.
    [RETURNS] Nothing.
*/
{
    iarray cube;
    /*static char function_name[] = "view";*/

    if (p == NULL)
    {
	(void) fprintf (stderr, "Must specify file to view\n");
	return;
    }
    if ( ( cube = iarray_read_nD (p, FALSE, NULL, 3, NULL, NULL,
				  K_CH_MAP_IF_AVAILABLE) ) == NULL )
    {
	(void) fprintf (stderr, "Error loading cube\n");
	return;
    }
    fprintf ( stderr, "Upper  dimension (0) name: \"%s\"\tlength: %lu\n",
	      iarray_dim_name (cube, 0), iarray_dim_length (cube, 0) );
    fprintf ( stderr, "Middle dimension (1) name: \"%s\"\tlength: %lu\n",
		     iarray_dim_name (cube, 1), iarray_dim_length (cube, 1) );
    fprintf ( stderr, "Lower  dimension (2) name: \"%s\"\tlength: %lu\n",
	      iarray_dim_name (cube, 2), iarray_dim_length (cube, 2) );
    iarray_dealloc (cube);
}   /*  End Function view  */

/*  kfill_sphere.c

    Source file for  kfill_sphere  (fill 3-D sphere in Karma format).

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

/*  This Karma module will read a 3D Karma file, writes a sphere into the
    volume and writes out a new Karma file.


    Written by      Richard Gooch   12-SEP-1996

    Last updated by Richard Gooch   21-NOV-1996


*/
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <karma.h>
#include <karma_iarray.h>

void main (int argc, char **argv)
{
    iarray inp, out;
    int xlen, ylen, zlen;
    int centre_x, centre_y, centre_z;
    int radius_x, radius_y, radius_z;
    int x, y, z, value;
    float distance, one_on_rx2, one_on_ry2, one_on_rz2, tmp;
    static char usage_string[] = "Usage:\tkfill_sphere infile outfile Cx Cy Cz Rx Ry Rz";

    if (argc != 9)
    {
	(void) fprintf (stderr, "%s\n", usage_string);
	exit (1);
    }
    if ( ( inp = iarray_read_nD (argv[1], FALSE, NULL, 3, (CONST char **) NULL,
				 NULL, K_CH_MAP_LOCAL) ) == NULL ) exit (2);
    if (iarray_type (inp) != K_BYTE)
    {
	(void) fprintf (stderr, "Must be a byte cube\n");
	exit (3);
    }
    xlen = iarray_dim_length (inp, 2);
    ylen = iarray_dim_length (inp, 1);
    zlen = iarray_dim_length (inp, 0);
    if ( ( out = iarray_create_3D (zlen, ylen, xlen, K_BYTE) ) == NULL )
    {
	exit (4);
    }
    centre_x = atoi (argv[3]);
    centre_y = atoi (argv[4]);
    centre_z = atoi (argv[5]);
    radius_x = atoi (argv[6]);
    radius_y = atoi (argv[7]);
    radius_z = atoi (argv[8]);
    one_on_rx2 = 1.0 / (float) (radius_x * radius_x);
    one_on_ry2 = 1.0 / (float) (radius_y * radius_y);
    one_on_rz2 = 1.0 / (float) (radius_z * radius_z);
    for (z = 0; z < zlen; ++z) for (y = 0; y < ylen; ++y)
    {
	for (x = 0; x < xlen; ++x)
	{
	    tmp = x - centre_x;
	    distance = tmp * tmp * one_on_rx2;
	    tmp = y - centre_y;
	    distance += tmp * tmp * one_on_ry2;
	    tmp = z - centre_z;
	    distance += tmp * tmp * one_on_rz2;
	    if (distance <= 1.0)
	    {
		/*  Inside the sphere  */
		B3 (out, z, y, x) = 122;
	    }
	    else
	    {
		/*  Outside the sphere  */
		value = B3 (inp, z, y, x);
		if (value >= 115) value = 114;
		B3 (out, z, y, x) = value;
	    }
	}
    }
    if ( !iarray_write (out, argv[2]) )
    {
	(void) fprintf (stderr, "Error writing\n");
    }
}   /*  End Function main  */

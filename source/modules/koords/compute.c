/*  compute.c

    Compute file for  koords  (X11 co-ordinate generator tool for Karma).

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

/*
    This Karma module will enable interactive generation of an astronomical
    co-ordinate system on a target image, using a reference image.
    This module runs on an X11 server.


    Written by      Richard Gooch   15-OCT-1996

    Last updated by Richard Gooch   17-OCT-1996: Created <apply_coordinates>.


*/

#include <stdio.h>
#include <math.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <karma.h>
#include <karma_iarray.h>
#include <karma_wcs.h>
#include <karma_dsra.h>
#include <karma_ds.h>
#include <karma_ch.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>
#include <karma_n.h>
#include "koords.h"


/*  Private functions  */
STATIC_FUNCTION (void setup_headers, () );


/*  Private data  */
static char *str_standard_desc[] =
{
    "PACKET",
    "  9",
    "END",
    "  ELEMENT",
    "    VSTRING",
    "    CTYPE1",
    "  END",
    "  ELEMENT",
    "    DOUBLE",
    "    CRVAL1",
    "  END",
    "  ELEMENT",
    "    DOUBLE",
    "    CRPIX1",
    "  END",
    "  ELEMENT",
    "    DOUBLE",
    "    CDELT1",
    "  END",
    "  ELEMENT",
    "    VSTRING",
    "    CTYPE2",
    "  END",
    "  ELEMENT",
    "    DOUBLE",
    "    CRVAL2",
    "  END",
    "  ELEMENT",
    "    DOUBLE",
    "    CRPIX2",
    "  END",
    "  ELEMENT",
    "    DOUBLE",
    "    CDELT2",
    "  END",
    "  ELEMENT",
    "    DOUBLE",
    "    CROTA2",
    "  END",
    NULL
};
static packet_desc *standard_desc = NULL;
static char *standard_packet = NULL;


/*  Public functions follow  */

void compute_and_store ()
/*  [SUMMARY] Compute co-ordinate system, print and store.
    [RETURNS] Nothing.
*/
{
    unsigned int count, xlen, ylen;
    char txt[STRING_LENGTH], tmp[STRING_LENGTH];
    double crval1[2], crpix1[2], cdelt1[2], crval2[2], crpix2[2], cdelt2[2];
    double crota[2];
    double pred_x[MAX_PAIRS], pred_y[MAX_PAIRS];
    extern KwcsAstro target_ap;
    extern iarray tar_array;
    extern unsigned int num_reference_points;
    extern unsigned int num_target_points;
    extern double reference_ra[MAX_PAIRS], reference_dec[MAX_PAIRS];
    extern double target_x[MAX_PAIRS], target_y[MAX_PAIRS];

    setup_headers ();
    if ( (num_reference_points != num_target_points) || (tar_array == NULL) )
    {
	ring_bell ();
	return;
    }
    /*  First compute the new co-ordinate system  */
    xlen = iarray_dim_length (tar_array, 1);
    ylen = iarray_dim_length (tar_array, 0);
    if (compute_coords (crval1, crpix1, cdelt1, crval2, crpix2, cdelt2, crota,
			NULL, num_reference_points,
			reference_ra, reference_dec, target_x, target_y,
			xlen, ylen) < 1)
    {
	fprintf (stderr, "Error computing co-ordinate system\n");
	return;
    }
    /*  Now put entries into temporary header and create projection object  */
    if ( !ds_put_unique_named_string (standard_desc, &standard_packet,
				      "CTYPE1", "RA---ARC", TRUE) ) return;
    if ( !ds_put_named_element (standard_desc, standard_packet, "CRVAL1",
				crval1) ) return;
    if ( !ds_put_named_element (standard_desc, standard_packet, "CRPIX1",
				crpix1) ) return;
    if ( !ds_put_named_element (standard_desc, standard_packet, "CDELT1",
				cdelt1) ) return;
    if ( !ds_put_unique_named_string (standard_desc, &standard_packet,
				      "CTYPE2", "DEC--ARC", TRUE) ) return;
    if ( !ds_put_named_element (standard_desc, standard_packet, "CRVAL2",
				crval2) ) return;
    if ( !ds_put_named_element (standard_desc, standard_packet, "CRPIX2",
				crpix2) ) return;
    if ( !ds_put_named_element (standard_desc, standard_packet, "CDELT2",
				cdelt2) ) return;
    if ( !ds_put_named_element (standard_desc, standard_packet, "CROTA2",
				crota) ) return;
    if (target_ap != NULL) wcs_astro_destroy (target_ap);
    if ( (target_ap = wcs_astro_setup (standard_desc, standard_packet) )
	 == NULL )
    {
	fprintf (stderr, "Could not create standard projection object\n");
	return;
    }
    /*  Copy RA,DEC values from reference and convert to x,y in target  */
    for (count = 0; count < num_reference_points; ++count)
    {
	pred_x[count] = reference_ra[count];
	pred_y[count] = reference_dec[count];
    }
    wcs_astro_transform (target_ap, num_reference_points,
			 pred_x, TRUE, pred_y, TRUE, NULL, FALSE,
			 0, NULL, NULL);
    fputs ("#  Ra              Dec              Tx     Ty      Px     Py      Dx     Dy\n",
	   stderr);
    for (count = 0; count < num_reference_points; ++count)
    {
	fprintf (stderr, "%-3u", count);
	wcs_astro_format_ra (txt, reference_ra[count]);
	fprintf (stderr, "%-16s", txt);
	wcs_astro_format_dec (txt, reference_dec[count]);
	fprintf (stderr, "%-15s  ", txt);
	fprintf (stderr, "%-7.1f%-8.1f%-7.1f%-8.1f%-7.1f%-.1f\n",
		 target_x[count], target_y[count],
		 pred_x[count], pred_y[count],
		 target_x[count] - pred_x[count],
		 target_y[count] - pred_y[count]);
    }
    /*  Print header in nice format  */
    fprintf (stderr, "\nHeader information:\nProjection: ARC (rectangular)\n");
    wcs_astro_format_ra (txt, crval1[0]);
    wcs_astro_format_dec (tmp, crval2[0]);
    fprintf (stderr, "Reference:  Ra %s  Dec %s   pixel: %.1f  %.1f\n",
	     txt, tmp, crpix1[0], crpix2[0]);
    fprintf (stderr,
	     "Co-ordinate increment:  Ra %.3f  Dec %.3f  (arcsec/pixel)\n",
	     cdelt1[0] * 3600.0, cdelt2[0] * 3600.0);
    fprintf (stderr, "Rotation: %.3f degrees\n", crota[0]);
}   /*  End Function compute_and_store  */

void apply_coordinates ()
/*  [SUMMARY] Apply co-ordinate system to target.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    dim_desc *dim;
    extern iarray tar_array;
    extern KwcsAstro target_ap;
    extern packet_desc *standard_desc;
    extern char *standard_packet;
    static char function_name[] = "apply_coordinates";

    if ( (target_ap == NULL) || (tar_array == NULL) )
    {
	ring_bell ();
	return;
    }
    /*  Copy over each element in the header to the output array  */
    for (count = 0; count < standard_desc->num_elements; ++count)
    {
	if ( !ds_copy_unique_named_element (tar_array->top_pack_desc,
					    tar_array->top_packet,
					    standard_desc, standard_packet,
					    standard_desc->element_desc[count],
					    FALSE, FALSE, TRUE) )
	{
	    fprintf (stderr, "Error adding co-ordinate system to header\n");
	    return;
	}
    }
    /*  Copy over the axis names if not already  */
    dim = iarray_get_dim_desc (tar_array, 1);
    if (strcmp (dim->name, "RA---ARC") != 0)
    {
	m_free ( (char *) dim->name );
	if ( ( dim->name = st_dup ("RA---ARC") ) == NULL )
	{
	    m_abort (function_name, "RA axis name");
	}
    }
    dim = iarray_get_dim_desc (tar_array, 0);
    if (strcmp (dim->name, "DEC--ARC") != 0)
    {
	m_free ( (char *) dim->name );
	if ( ( dim->name = st_dup ("DEC--ARC") ) == NULL )
	{
	    m_abort (function_name, "DEC axis name");
	}
    }
}   /*  End Function apply_coordinates  */


/*  Private functions follow  */

static void setup_headers ()
/*  [SUMMARY] Create the temporary headers.
    [RETURNS] Nothing.
*/
{
    Channel channel;
    extern packet_desc *standard_desc;
    extern char *standard_packet;
    extern char *str_standard_desc[];
    static char function_name[] = "setup_headers";

    if (standard_desc != NULL) return;
    /*  Create descriptor  */
    if ( ( channel = ch_open_and_fill_memory (str_standard_desc) ) == NULL )
    {
	m_abort (function_name, "memory channel");
    }
    if ( ( standard_desc = dsra_packet_desc (channel) ) == NULL )
    {
	ch_close (channel);
	m_abort (function_name, "standard header descriptor");
    }
    ch_close (channel);
    /*  Create data  */
    if ( ( standard_packet = ds_alloc_data (standard_desc, TRUE, TRUE) )
	 == NULL )
    {
	m_abort (function_name, "standard header data");
    }
}   /*  End Function setup_headers  */

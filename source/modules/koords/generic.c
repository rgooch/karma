/*  generic.c

    Generic file for  koords  (X11 co-ordinate generator tool for Karma).

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


    Written by      Richard Gooch   14-OCT-1996: Copied from kshell/generic.c

    Updated by      Richard Gooch   28-OCT-1996: Added hostname and port number
  to title.

    Last updated by Richard Gooch   3-NOV-1996: Changed <load_image> to not
  insist that target image have no projections.


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <errno.h>
#include <karma.h>
#include <karma_viewimg.h>
#include <karma_foreign.h>
#include <karma_iarray.h>
#include <karma_dsxfr.h>
#include <karma_conn.h>
#include <karma_wcs.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_r.h>
#include "koords.h"


/*  Private functions  */
STATIC_FUNCTION (void destroy_all_vimages,
		 (ViewableImage *image, ViewableImage *magnified_image) );


/*  Private data  */
static char *keywords[] =
{
    "OBJECT",
    "DATE-OBS",
    "EPOCH",
    "BUNIT",
    "TELESCOP",
    "INSTRUME",
    "EQUINOX",
    "BTYPE",
    "BMAJ",
    "BMIN",
    "BPA",
    "RESTFREQ",
    "VELREF",
    "PBFWHM",
    NULL
};


/*  Public functions follow  */

void setup_comms (Display *display)
/*  This routine will initialise the communications system.
    The display the module is connected to must be pointed to by  display  .
    NOTE:  conn_register_managers  MUST be called first.
    The routine returns nothing.
*/
{
    int def_port_number;
    unsigned int server_port_number;
    char hostname[STRING_LENGTH];
    extern char module_name[STRING_LENGTH + 1];
    extern char module_version_date[STRING_LENGTH + 1];
    extern char title_name[STRING_LENGTH];

    /*  Get default port number  */
    if ( ( def_port_number = r_get_def_port ( module_name,
					     DisplayString (display) ) ) < 0 )
    {
	fprintf (stderr, "Could not get default port number\n");
	return;
    }
    r_gethostname (hostname, STRING_LENGTH);
    server_port_number = def_port_number;
    if ( !conn_become_server (&server_port_number, CONN_MAX_INSTANCES) )
    {
	fprintf (stderr, "Module not operating as Karma server\n");
	sprintf (title_name, "%s v%s @%s", module_name, module_version_date,
		 hostname);
    }
    else
    {
	fprintf (stderr, "Port allocated: %d\n", server_port_number);
	sprintf (title_name, "%s v%s @%s:%u",
		 module_name, module_version_date, hostname,
		 server_port_number);
    }
}   /*  End Function setup_comms  */

flag load_image (CONST char *filename, iarray *array, KwcsAstro *ap, flag ref,
		 KWorldCanvas pseudo_canvas, KWorldCanvas mag_pseudo_canvas,
		 ViewableImage *image, ViewableImage *magnified_image,
		 double *min, double *max)
/*  [PURPOSE] This routine will load a file and display it.
    <filename> The name of the file to load.
    <array> The image array will be written here.
    <ap> The astronomical projection system will be written here.
    <ref> If TRUE, the reference image is being loaded.
    <pseudo_canvas> The PseudoColour canvas.
    <image> If an image is loaded the ViewableImage is written here. If no
    image is loaded, NULL is written here. The value written here must be
    preserved between calls.
    <magnified_image> If an image is loaded the magnified ViewableImage is
    written here. If no image is loaded, NULL is written here. The value
    written here must be preserved between calls.
    <min> The minimum image value is written here.
    <max> The maximum image value is written here.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int ftype;
    /*static char function_name[] = "load_image";*/

    if ( !foreign_read_and_setup (filename, K_CH_MAP_LOCAL, FALSE, &ftype,
				  TRUE, 2, K_FLOAT, FALSE, array, min, max,
				  TRUE, ap) ) return (FALSE);
    if ( ref && (*ap == NULL) )
    {
	fprintf (stderr, "No astronomical projection found for reference\n");
	iarray_dealloc (*array);
	*array = NULL;
	return (FALSE);
    }
    destroy_all_vimages (image, magnified_image);
    if ( ( *image = viewimg_create_from_iarray (pseudo_canvas, *array,
						FALSE) ) == NULL )
    {
	fprintf (stderr, "Error getting ViewableImage from Iarray\n");
	iarray_dealloc (*array);
	*array = NULL;
	if (ap != NULL)
	{
	    wcs_astro_destroy (*ap);
	    *ap = NULL;
	}
	return (FALSE);
    }
    if ( ( *magnified_image =
	   viewimg_create_from_iarray (mag_pseudo_canvas, *array,
				       FALSE) ) == NULL )
    {
	fprintf (stderr, "Error getting ViewableImage from Iarray\n");
	destroy_all_vimages (image, magnified_image);
	iarray_dealloc (*array);
	*array = NULL;
	if (ap != NULL)
	{
	    wcs_astro_destroy (*ap);
	    *ap = NULL;
	}
	return (FALSE);
    }
    canvas_set_attributes (pseudo_canvas,
			   CANVAS_ATT_VALUE_MIN, *min,
			   CANVAS_ATT_VALUE_MAX, *max,
			   CANVAS_ATT_END);
    canvas_set_attributes (mag_pseudo_canvas,
			   CANVAS_ATT_VALUE_MIN, *min,
			   CANVAS_ATT_VALUE_MAX, *max,
			   CANVAS_ATT_END);
    viewimg_make_active (*image);
    viewimg_make_active (*magnified_image);
    return (TRUE);
}   /*  End Function load_image  */

flag copy_header_info (iarray out, iarray in)
/*  [SUMMARY] Copy some header information from one array to another.
    <out> The output array.
    <in> The input array.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int count;
    static char function_name[] = "copy_header_info";

    for (count = 0; keywords[count] != NULL; ++count)
    {
	if ( !iarray_copy_named_element (out, in, keywords[count],
					 FALSE, FALSE, TRUE) )
	{
	    fprintf (stderr, "%s: Failed to copy header keyword %s\n",
		     function_name, keywords[count]);
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function copy_header_info  */

void convert_lin_world_coords (double *xout, double *yout,
			       KwcsAstro out_ap,
			       CONST double *xin, CONST double *yin,
			       KwcsAstro in_ap, unsigned int num_coords)
/*  [SUMMARY] Convert linear world co-ordinates in two reference frames.
    [PURPOSE] This routine will convert linear world co-ordinates in one
    reference frame to linear world co-ordinates in another frame. If
    necessary, linear to non-linear and non-linear to linear conversions are
    performed.
    <xout> The output horizontal co-ordinates are written here.
    <yout> The output vertical co-ordinates are written here.
    <out_ap> The output reference frame.
    <xin> The input horizontal co-ordinates are written here.
    <yin> The input vertical co-ordinates are written here.
    <in_ap> The input reference frame.
    [RETURNS] Nothing.
*/
{
    unsigned int count;

    /*  First copy to the output arrays  */
    for (count = 0; count < num_coords; ++count)
    {
	xout[count] = xin[count];
	yout[count] = yin[count];
    }
    if ( !wcs_astro_test_radec (in_ap) && !wcs_astro_test_radec (out_ap) )
    {
	/*  Both reference frames are linear: plain copy  */
	return;
    }
    if ( wcs_astro_test_radec (in_ap) && wcs_astro_test_radec (out_ap) )
    {
	/*  Both reference frames are non-linear: convert back and forth  */
	wcs_astro_transform (in_ap, num_coords,
			     xout, FALSE, yout, FALSE, NULL, FALSE,
			     0, NULL, NULL);
	wcs_astro_transform (out_ap, num_coords,
			     xout, TRUE, yout, TRUE, NULL, FALSE,
			     0, NULL, NULL);
	return;
    }
    if ( wcs_astro_test_radec (in_ap) )
    {
	/*  Input reference frame is non-linear but output is linear: convert
	    back to linear  */
	wcs_astro_transform (in_ap, num_coords,
			     xout, TRUE, yout, TRUE, NULL, FALSE,
			     0, NULL, NULL);
	return;
    }
    /*  Output reference frame is non-linear but input is linear: do nothing */
}   /*  End Function convert_lin_world_coords  */


/*  Private functions follow  */

static void destroy_all_vimages (ViewableImage *image,
				 ViewableImage *magnified_image)
/*  [PURPOSE] This routine will destroy all ViewableImages and clear the
    appropriate pointers.
    [RETURNS] Nothing.
*/
{
    if (*image != NULL) viewimg_destroy (*image);
    *image = NULL;
    if (*magnified_image != NULL) viewimg_destroy (*magnified_image);
    *magnified_image = NULL;
}   /*  End Function destroy_all_vimages  */

/*  generic.c

    Generic file for  kshell  (X11 ellipse integrator tool for Karma).

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
    This Karma module will enable interactive integration along the
    circumference of concentric ellipses.
    This module runs on an X11 server.


    Written by      Richard Gooch   24-SEP-1996: Copied from kpvslice module.

    Last updated by Richard Gooch   28-OCT-1996: Added hostname and port number
  to title.


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
#include "kshell.h"


/*  Private functions  */
STATIC_FUNCTION (void destroy_all_vimages,
		 (ViewableImage *image, ViewableImage *magnified_image) );
STATIC_FUNCTION (void new_data_on_connection, (flag first_time_data) );
/*
STATIC_FUNCTION (void connection_closed, (flag data_deallocated) );
*/


/*  Private data  */
static char *keywords[] =
{
    "OBJECT",
    "DATE-OBS",
    "OBSRA",
    "OBSDEC",
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
	/*  Register the protocols  */
	dsxfr_register_connection_limits (1, -1);
	dsxfr_register_read_func ( ( void (*) () ) new_data_on_connection );
	/*dsxfr_register_close_func (connection_closed);*/
	sprintf (title_name, "%s v%s @%s:%u",
		 module_name, module_version_date, hostname,
		 server_port_number);
    }
}   /*  End Function setup_comms  */

iarray load_image (CONST char *inp_filename,
		   KWorldCanvas pseudo_canvas, KWorldCanvas mag_pseudo_canvas,
		   ViewableImage *image, ViewableImage *magnified_image)
/*  [PURPOSE] This routine will load a file and display it.
    <inp_filename> The name of the file to load.
    <pseudo_canvas> The PseudoColour canvas.
    <image> If an image is loaded the ViewableImage is written here. If no
    image is loaded, NULL is written here. The value written here must be
    preserved between calls.
    <magnified_image> If an image is loaded the magnified ViewableImage is
    written here. If no image is loaded, NULL is written here. The value
    written here must be preserved between calls.
    [RETURNS] The image array on success, else NULL.
*/
{
    iarray image_pseudo;
    unsigned int ftype;
    double i_min, i_max;
    multi_array *multi_desc;
    extern KwcsAstro loaded_image_ap, main_ap;
    extern unsigned int image_mode;
    /*static char function_name[] = "load_image";*/

    if ( ( multi_desc = foreign_guess_and_read (inp_filename, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	fprintf (stderr, "Error reading file: \"%s\"\n", inp_filename);
	return (NULL);
    }
    /*  Try to get 2-D image  */
    if ( ( image_pseudo = iarray_get_from_multi_array (multi_desc, NULL, 2,
						       NULL, NULL) )
	 == NULL )
    {
	fprintf (stderr, "Could not find image\n");
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    ds_dealloc_multi (multi_desc);
    if (iarray_type (image_pseudo) != K_FLOAT)
    {
	fprintf (stderr, "Sorry, only float data supported\n");
	iarray_dealloc (image_pseudo);
	return (NULL);
    }
    destroy_all_vimages (image, magnified_image);
    if ( !iarray_min_max (image_pseudo, CONV1_REAL, &i_min, &i_max) )
    {
	fprintf (stderr, "Error computing min-max\n");
	iarray_dealloc (image_pseudo);
	return (NULL);
    }
    if (i_min == i_max)
    {
	fprintf (stderr, "min: %e is same as max: pointless!\n", i_min);
	iarray_dealloc (image_pseudo);
	return (NULL);
    }
    if ( ( *image = viewimg_create_from_iarray (pseudo_canvas, image_pseudo,
						FALSE) ) == NULL )
    {
	fprintf (stderr,
			"Error getting ViewableImage from Iarray\n");
	iarray_dealloc (image_pseudo);
	return (NULL);
    }
    if ( ( *magnified_image =
	   viewimg_create_from_iarray (mag_pseudo_canvas, image_pseudo,
				       FALSE) ) == NULL )
    {
	fprintf (stderr,
			"Error getting ViewableImage from Iarray\n");
	iarray_dealloc (image_pseudo);
	destroy_all_vimages (image, magnified_image);
	return (NULL);
    }
    canvas_set_attributes (pseudo_canvas,
			   CANVAS_ATT_VALUE_MIN, i_min,
			   CANVAS_ATT_VALUE_MAX, i_max,
			   CANVAS_ATT_END);
    canvas_set_attributes (mag_pseudo_canvas,
			   CANVAS_ATT_VALUE_MIN, i_min,
			   CANVAS_ATT_VALUE_MAX, i_max,
			   CANVAS_ATT_END);
    if (main_ap == loaded_image_ap) main_ap = NULL;
    if (loaded_image_ap != NULL) wcs_astro_destroy (loaded_image_ap);
    loaded_image_ap = wcs_astro_setup (image_pseudo->top_pack_desc,
				       *image_pseudo->top_packet);
    fprintf (stderr, "loaded_image_astro_projection: %p\n", loaded_image_ap);
    if (image_mode == IMAGE_MODE_LOADED)
    {
	if ( !viewimg_make_active (*image) )
	{
	    fprintf (stderr, "Error making ViewableImage active\n");
	    if (loaded_image_ap != NULL) wcs_astro_destroy (loaded_image_ap);
	    iarray_dealloc (image_pseudo);
	    destroy_all_vimages (image, magnified_image);
	    return (NULL);
	}
	if ( !viewimg_make_active (*magnified_image) )
	{
	    fprintf (stderr, "Error making ViewableImage active\n");
	    if (loaded_image_ap != NULL) wcs_astro_destroy (loaded_image_ap);
	    iarray_dealloc (image_pseudo);
	    destroy_all_vimages (image, magnified_image);
	    return (NULL);
	}
	main_ap = loaded_image_ap;
    }
    return (image_pseudo);
}   /*  End Function load_image  */

iarray load_cube (CONST char *inp_filename, double *min, double *max)
/*  [PURPOSE] This routine will load a cube.
    <inp_filename> The name of the file to load.
    <min> The minimum cube value is written here.
    <max> The maximum cube value is written here.
    [RETURNS] The cube array on success, else NULL.
*/
{
    iarray cube;
    unsigned int ftype, xlen, ylen, x, y;
    multi_array *multi_desc;
    extern KwcsAstro cube_ap;
    extern iarray cube_ra_coords, cube_dec_coords;
    static char function_name[] = "load_cube";

    if ( ( multi_desc = foreign_guess_and_read (inp_filename, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	fprintf (stderr, "Error reading file: \"%s\"\n", inp_filename);
	return (NULL);
    }
    /*  Try to get cube  */
    if ( ( cube = iarray_get_from_multi_array (multi_desc, NULL, 3,
					       NULL, NULL) )
	 == NULL )
    {
	fprintf (stderr, "Could not find cube\n");
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    ds_dealloc_multi (multi_desc);
    if (iarray_type (cube) != K_FLOAT)
    {
	fprintf (stderr, "Sorry, only float data supported\n");
	if (cube_ap != NULL) wcs_astro_destroy (cube_ap);
	iarray_dealloc (cube);
	return (NULL);
    }
    fprintf ( stderr, "Loaded cube of %lu * %lu * %lu\n",
	      iarray_dim_length (cube, 2), iarray_dim_length (cube, 1),
	      iarray_dim_length (cube, 0) );
    /*  Compute the minimum and maximum  */
    iarray_min_max (cube, CONV_CtoR_REAL, min, max);
    if (*min == *max)
    {
	fprintf (stderr, "min: %e is same as max: pointless!\n", *min);
	iarray_dealloc (cube);
	return (NULL);
    }
    fprintf (stderr, "Cube minimum: %e  maximum: %e\n", *min, *max);
    if (cube_ap != NULL) wcs_astro_destroy (cube_ap);
    cube_ap = wcs_astro_setup (cube->top_pack_desc, *cube->top_packet);
    fprintf (stderr, "cube_astro_projection: %p\n", cube_ap);
    /*  Setup cube co-ordinate arrays which will convert from x-y pixels to
	proper RA and DEC  */
    if (cube_ra_coords != NULL) iarray_dealloc (cube_ra_coords);
    if (cube_dec_coords != NULL) iarray_dealloc (cube_dec_coords);
    xlen = iarray_dim_length (cube, 2);
    ylen = iarray_dim_length (cube, 1);
    if ( ( cube_ra_coords = iarray_create_2D (ylen, xlen, K_DOUBLE) ) == NULL )
    {
	m_abort (function_name, "cube RA co-ordinate array");
    }
    if ( ( cube_dec_coords = iarray_create_2D (ylen, xlen,K_DOUBLE) ) == NULL )
    {
	m_abort (function_name, "cube DEC co-ordinate array");
    }
    /*  Fill in linear co-ordinates  */
    for (y = 0; y < ylen; ++y) for (x = 0; x < xlen; ++x)
    {
	D2 (cube_ra_coords, y, x) = iarray_get_coordinate (cube, 2, x);
	D2 (cube_dec_coords, y, x) = iarray_get_coordinate (cube, 1, y);
    }
    fprintf (stderr,
	     "Filled in linear co-ordinates in cube co-ordinate array\n");
    if (cube_ap != NULL)
    {
	/*  Convert to proper RA and DEC  */
	wcs_astro_transform (cube_ap, xlen * ylen,
			     (double *) cube_ra_coords->data, FALSE,
			     (double *) cube_dec_coords->data, FALSE,
			     NULL, FALSE,
			     0, NULL, NULL);
    }
    fprintf (stderr, "Created cube co-ordinate array\n");
    return (cube);
}   /*  End Function load_cube  */

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


/*  Private routines follow  */

static void new_data_on_connection (flag first_time_data)
/*  This routine is called when new data arrives.
    If data is arriving the first time, then  first_time_data  will be TRUE.
    The routine returns nothing.
*/
{
    load_and_setup ("connection");
}   /*  End Function new_data_on_connection  */

#ifdef UNIMPLEMENTED
static void connection_closed (flag data_deallocated)
/*  This routine is called when the "multi_array" connection closes.
    If there was data on the connection, the value of  data_deallocated  will
    be TRUE.
    The routine returns nothing.
*/
{
    unsigned int count;
    extern unsigned int num_frames;
    extern ViewableImage *frames;

    /*  Deallocate old ViewableImage objects  */
    for (count = 0; count < num_frames; ++count)
    {
	viewimg_destroy (frames[count]);
    }
    num_frames = 0;
    if (frames != NULL) m_free ( (char *) frames );
    frames = NULL;
}   /*  End Function connection_closed  */
#endif

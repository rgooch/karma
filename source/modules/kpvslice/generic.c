/*  generic.c

    Generic file for  kpvslice  (X11 Position-Velocity slice tool for Karma).

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
    This Karma module will enable interactive selection of a Position-Velocity
    slice through a data cube.
    This module runs on an X11 server.


    Written by      Richard Gooch   12-JUN-1996: Copied from kview module.

    Last updated by Richard Gooch   27-JUN-1996: Removed <coord_transform_func>


*/
#include <stdio.h>
#include <math.h>
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


/*  Local functions  */
EXTERN_FUNCTION (void setup_comms, (Display *display) );
EXTERN_FUNCTION (iarray load_image,
		 (CONST char *inp_filename,
		  KWorldCanvas pseudo_canvas, KWorldCanvas mag_pseudo_canvas,
		  ViewableImage *image, ViewableImage *magnified_image) );
EXTERN_FUNCTION (iarray load_cube, (CONST char *inp_filename) );


/*  External functions  */
EXTERN_FUNCTION (void load_and_setup, (CONST char *filename) );


/*  Private functions  */
STATIC_FUNCTION (void destroy_all_vimages,
		 (ViewableImage *image, ViewableImage *magnified_image) );
STATIC_FUNCTION (void new_data_on_connection, (flag first_time_data) );
/*
STATIC_FUNCTION (void connection_closed, (flag data_deallocated) );
*/


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
    extern char module_name[STRING_LENGTH + 1];

    /*  Get default port number  */
    if ( ( def_port_number = r_get_def_port ( module_name,
					     DisplayString (display) ) ) < 0 )
    {
	(void) fprintf (stderr, "Could not get default port number\n");
	return;
    }
    server_port_number = def_port_number;
    if ( !conn_become_server (&server_port_number, CONN_MAX_INSTANCES) )
    {
	(void) fprintf (stderr, "Module not operating as Karma server\n");
    }
    else
    {
	(void) fprintf (stderr, "Port allocated: %d\n", server_port_number);
	/*  Register the protocols  */
	dsxfr_register_connection_limits (1, -1);
	dsxfr_register_read_func ( ( void (*) () ) new_data_on_connection );
/*
	dsxfr_register_close_func (connection_closed);
*/
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
    extern KwcsAstro image_ap;
    /*static char function_name[] = "load_image";*/

    if ( ( multi_desc = foreign_guess_and_read (inp_filename, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading file: \"%s\"\n", inp_filename);
	return (FALSE);
    }
    if (image_ap != NULL) wcs_astro_destroy (image_ap);
    image_ap = wcs_astro_setup (multi_desc->headers[0], multi_desc->data[0]);
    (void) fprintf (stderr, "image_astro_projection: %p\n", image_ap);
    /*  Try to get 2-D image  */
    if ( ( image_pseudo = iarray_get_from_multi_array (multi_desc, NULL, 2,
						       NULL, NULL) )
	 == NULL )
    {
	(void) fprintf (stderr, "Could not find image\n");
	ds_dealloc_multi (multi_desc);
	return (NULL);
    }
    ds_dealloc_multi (multi_desc);
    destroy_all_vimages (image, magnified_image);
    if ( ( *image = viewimg_create_from_iarray (pseudo_canvas, image_pseudo,
						FALSE) ) == NULL )
    {
	(void) fprintf (stderr,
			"Error getting ViewableImage from Iarray\n");
	iarray_dealloc (image_pseudo);
	return (NULL);
    }
    if ( ( *magnified_image =
	   viewimg_create_from_iarray (mag_pseudo_canvas, image_pseudo,
				       FALSE) ) == NULL )
    {
	(void) fprintf (stderr,
			"Error getting ViewableImage from Iarray\n");
	iarray_dealloc (image_pseudo);
	destroy_all_vimages (image, magnified_image);
	return (NULL);
    }
    if ( !iarray_min_max (image_pseudo, CONV1_REAL, &i_min, &i_max) )
    {
	(void) fprintf (stderr, "Error computing min-max\n");
    }
    if (i_min == i_max)
    {
	(void) fprintf (stderr, "min: %e is same as max: modifying\n",
			i_min);
	--i_min;
	++i_max;
    }
    canvas_set_attributes (pseudo_canvas,
			   CANVAS_ATT_VALUE_MIN, i_min,
			   CANVAS_ATT_VALUE_MAX, i_max,
			   CANVAS_ATT_END);
    canvas_set_attributes (mag_pseudo_canvas,
			   CANVAS_ATT_VALUE_MIN, i_min,
			   CANVAS_ATT_VALUE_MAX, i_max,
			   CANVAS_ATT_END);
    if ( !viewimg_make_active (*image) )
    {
	(void) fprintf (stderr, "Error making ViewableImage active\n");
	iarray_dealloc (image_pseudo);
	destroy_all_vimages (image, magnified_image);
	return (NULL);
    }
    if ( !viewimg_make_active (*magnified_image) )
    {
	(void) fprintf (stderr, "Error making ViewableImage active\n");
	iarray_dealloc (image_pseudo);
	destroy_all_vimages (image, magnified_image);
	return (NULL);
    }
    return (image_pseudo);
}   /*  End Function load_image  */

iarray load_cube (CONST char *inp_filename)
/*  [PURPOSE] This routine will load a cube.
    <inp_filename> The name of the file to load.
    [RETURNS] The cube array on success, else NULL.
*/
{
    iarray cube;
    unsigned int ftype;
    multi_array *multi_desc;
    extern KwcsAstro cube_ap;
    /*static char function_name[] = "load_cube";*/

    if ( ( multi_desc = foreign_guess_and_read (inp_filename, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading file: \"%s\"\n", inp_filename);
	return (FALSE);
    }
    if (cube_ap != NULL) wcs_astro_destroy (cube_ap);
    cube_ap = wcs_astro_setup (multi_desc->headers[0], multi_desc->data[0]);
    (void) fprintf (stderr, "cube_astro_projection: %p\n", cube_ap);
    /*  Try to get cube  */
    if ( ( cube = iarray_get_from_multi_array (multi_desc, NULL, 3,
					       NULL, NULL) )
	 == NULL )
    {
	(void) fprintf (stderr, "Could not find cube\n");
	return (NULL);
    }
    return (cube);
}   /*  End Function load_cube  */


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

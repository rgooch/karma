/*  generic.c

    Generic file for  kview  (X11 image/movie display tool for Karma).

    Copyright (C) 1995-1996  Richard Gooch

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
    This Karma module will enable on-screen imaging of a single arrayfile of
    the general data structure format. The arrayfile must contain a 2 or 3
    dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   30-AUG-1995: Copied from  kmovie_main.c
  and  xraycast_3d/load_disp.c

    Updated by      Richard Gooch   11-JUN-1996: Added magnifier support.

    Updated by      Richard Gooch   12-JUN-1996: Fiddle minimum and maximum if
  equal.

    Updated by      Richard Gooch   15-JUN-1996: Created <coord_transform_func>

    Updated by      Richard Gooch   19-JUN-1996: Fixed bug in <display_file>
  when displaying TrueColour images with only PseudoColour canvas available for
  magnifier.

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
EXTERN_FUNCTION (flag display_file,
		 (CONST char *inp_filename,
		  KWorldCanvas pseudo_canvas, KWorldCanvas rgb_canvas,
		  KWorldCanvas mag_pseudo_canvas, KWorldCanvas mag_rgb_canvas,
		  iarray *pseudo_arr,
		  ViewableImage *image, ViewableImage **movie,
		  ViewableImage *magnified_image,
		  ViewableImage **magnified_movie,
		  unsigned int *num_frames) );


/*  External functions  */
EXTERN_FUNCTION (void load_and_setup, (CONST char *filename) );


/*  Private functions  */
STATIC_FUNCTION (void destroy_all_vimages,
		 (ViewableImage *image, ViewableImage **movie,
		  ViewableImage *magnified_image,
		  ViewableImage **magnified_movie,
		  unsigned int *num_frames) );
STATIC_FUNCTION (void new_data_on_connection, (flag first_time_data) );
/*
STATIC_FUNCTION (void connection_closed, (flag data_deallocated) );
*/


/*  Global data  */
KwcsAstro astro_projection = NULL;


/*  Private data  */


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
    if ( !conn_controlled_by_cm_tool () )
    {
	conn_register_client_protocol ("hog_request", 0, 0,
				       ( flag (*) () ) NULL,
				       ( flag (*) () ) NULL,
				       ( flag (*) () ) NULL,
				       ( void (*) () ) NULL);
    }
}   /*  End Function setup_comms  */

flag display_file (CONST char *inp_filename,
		   KWorldCanvas pseudo_canvas, KWorldCanvas rgb_canvas,
		   KWorldCanvas mag_pseudo_canvas, KWorldCanvas mag_rgb_canvas,
		   iarray *pseudo_arr,
		   ViewableImage *image, ViewableImage **movie,
		   ViewableImage *magnified_image,
		   ViewableImage **magnified_movie,
		   unsigned int *num_frames)
/*  [PURPOSE] This routine will load a file and display it.
    <inp_filename> The name of the file to load.
    <pseudo_canvas> The PseudoColour canvas.
    <rgb_canvas> The TrueColour or DirectColour canvas. May be NULL.
    <pseudo_arr> If a PseudoColour data set is loaded the Intelligent Array is
    written here, else NULL is written here. The value written here must be
    preserved between calls.
    <image> If an image is loaded the ViewableImage is written here. If no
    image is loaded, NULL is written here. The value written here must be
    preserved between calls.
    <movie> If a movie is loaded the array of ViewableImages is written here.
    If no movie is loaded, NULL is written here. The value written here must be
    preserved between calls.
    <magnified_image> If an image is loaded the magnified ViewableImage is
    written here. If no image is loaded, NULL is written here. The value
    written here must be preserved between calls.
    <magnified_movie> If a movie is loaded the array of magnified
    ViewableImages is written here. If no movie is loaded, NULL is written
    here. The value written here must be preserved between calls.
    <num_frames> If a movie is loaded the number of frames is written here. If
    no movie is loaded, 0 is written here. The value written here must be
    preserved between calls.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Kcolourmap cmap;
    KWorldCanvas main_canvas, mag_canvas;
    iarray image_pseudo, image_red, image_green, image_blue;
    iarray movie_pseudo, movie_red, movie_green, movie_blue;
    unsigned int cmap_index, ftype;
    double i_min, i_max;
    multi_array *multi_desc;
    extern KwcsAstro astro_projection;
    extern char *sys_errlist[];
    static flag first_time = TRUE;
    /*static char function_name[] = "display_file";*/

    if (first_time)
    {
	first_time = FALSE;
	(void) conn_attempt_connection ("unix",
					r_get_def_port ("khogd", ":0.0"),
					"hog_request");
    }
    if ( ( multi_desc = foreign_guess_and_read (inp_filename, K_CH_MAP_LOCAL,
						FALSE, &ftype,
						FA_GUESS_READ_END) ) == NULL )
    {
	(void) fprintf (stderr, "Error reading file: \"%s\"\n", inp_filename);
	return (FALSE);
    }
    if (astro_projection != NULL) wcs_astro_destroy (astro_projection);
    astro_projection = wcs_astro_setup (multi_desc->headers[0],
					multi_desc->data[0]);
    (void) fprintf (stderr, "astro_projection: %p\n", astro_projection);
    if (*pseudo_arr != NULL) iarray_dealloc (*pseudo_arr);
    *pseudo_arr = NULL;
    destroy_all_vimages (image, movie, magnified_image, magnified_movie,
			 num_frames);
    /*  Try to get 2-D image  */
    if ( iarray_get_image_from_multi (multi_desc, &image_pseudo,
				      &image_red, &image_green, &image_blue,
				      &cmap_index) )
    {
	if (image_pseudo == NULL)
	{
	    /*  Image is TrueColour: use RGB canvas if possible, PseudoColour
		canvas if not  */
	    main_canvas = (rgb_canvas == NULL) ? pseudo_canvas : rgb_canvas;
	    mag_canvas = (mag_rgb_canvas == NULL) ?
		mag_pseudo_canvas : mag_rgb_canvas;
	    if ( ( *image = viewimg_create_rgb (main_canvas, multi_desc,
						image_red->arr_desc,
						image_red->data, 1, 0,
						image_red->elem_index,
						image_green->elem_index,
						image_blue->elem_index,
						0, (char **) NULL,
						(double *) NULL) )
		== NULL )
	    {
		(void) fprintf (stderr,
				"Error getting ViewableImage from Iarray\n");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_red);
		iarray_dealloc (image_green);
		iarray_dealloc (image_blue);
		return (FALSE);
	    }
	    if ( ( *magnified_image =
		   viewimg_create_rgb (mag_canvas, multi_desc,
				       image_red->arr_desc,
				       image_red->data, 1, 0,
				       image_red->elem_index,
				       image_green->elem_index,
				       image_blue->elem_index,
				       0, (char **) NULL,
				       (double *) NULL) )
		 == NULL )
	    {
		(void) fprintf (stderr,
				"Error getting ViewableImage from Iarray\n");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_red);
		iarray_dealloc (image_green);
		iarray_dealloc (image_blue);
		destroy_all_vimages (image, movie,
				     magnified_image, magnified_movie,
				     num_frames);
		return (FALSE);
	    }
	    iarray_dealloc (image_red);
	    iarray_dealloc (image_green);
	    iarray_dealloc (image_blue);
	    if ( !viewimg_make_active (*image) )
	    {
		(void) fprintf (stderr, "Error making ViewableImage active\n");
		ds_dealloc_multi (multi_desc);
		destroy_all_vimages (image, movie,
				     magnified_image, magnified_movie,
				     num_frames);
		return (FALSE);
	    }
	    if ( !viewimg_make_active (*magnified_image) )
	    {
		(void) fprintf (stderr, "Error making ViewableImage active\n");
		ds_dealloc_multi (multi_desc);
		destroy_all_vimages (image, movie,
				     magnified_image, magnified_movie,
				     num_frames);
		return (FALSE);
	    }
	}
	else
	{
	    /*  Image is PseudoColour: must use PseudoColour canvas  */
	    if (pseudo_canvas == NULL)
	    {
		(void) fprintf (stderr, "No PseudoColour canvas available\n");
		iarray_dealloc (image_pseudo);
		ds_dealloc_multi (multi_desc);
		return (FALSE);
	    }
	    main_canvas = pseudo_canvas;
	    mag_canvas = mag_pseudo_canvas;
	    if ( ( *image = viewimg_create_from_iarray (main_canvas,
							image_pseudo,
							FALSE) ) == NULL )
	    {
		(void) fprintf (stderr,
				"Error getting ViewableImage from Iarray\n");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_pseudo);
		return (FALSE);
	    }
	    if ( ( *magnified_image =
		   viewimg_create_from_iarray (mag_canvas, image_pseudo,
					       FALSE) ) == NULL )
	    {
		(void) fprintf (stderr,
				"Error getting ViewableImage from Iarray\n");
		ds_dealloc_multi (multi_desc);
		iarray_dealloc (image_pseudo);
		destroy_all_vimages (image, movie,
				     magnified_image, magnified_movie,
				     num_frames);
		return (FALSE);
	    }
	    cmap = canvas_get_cmap (main_canvas);
	    if ( (cmap != NULL) && (cmap_index < multi_desc->num_arrays) )
	    {
		if ( !kcmap_copy_from_struct (cmap,
					      multi_desc->headers[cmap_index],
					      multi_desc->data[cmap_index]) )
		{
		    (void) fprintf (stderr, "Error changing colourmap\n");
		}
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
	    *pseudo_arr = image_pseudo;
	    canvas_set_attributes (main_canvas,
				   CANVAS_ATT_VALUE_MIN, i_min,
				   CANVAS_ATT_VALUE_MAX, i_max,
				   CANVAS_ATT_END);
	    canvas_set_attributes (mag_canvas,
				   CANVAS_ATT_VALUE_MIN, i_min,
				   CANVAS_ATT_VALUE_MAX, i_max,
				   CANVAS_ATT_END);
	    if ( !viewimg_make_active (*image) )
	    {
		(void) fprintf (stderr, "Error making ViewableImage active\n");
		ds_dealloc_multi (multi_desc);
		destroy_all_vimages (image, movie,
				     magnified_image, magnified_movie,
				     num_frames);
		return (FALSE);
	    }
	    if ( !viewimg_make_active (*magnified_image) )
	    {
		(void) fprintf (stderr, "Error making ViewableImage active\n");
		ds_dealloc_multi (multi_desc);
		destroy_all_vimages (image, movie,
				     magnified_image, magnified_movie,
				     num_frames);
		return (FALSE);
	    }
	}
	ds_dealloc_multi (multi_desc);
	return (TRUE);
    }
    /*  Try to get movie  */
    if ( !iarray_get_movie_from_multi (multi_desc, &movie_pseudo,
				       &movie_red, &movie_green, &movie_blue,
				       &cmap_index) )
    {
	(void) fprintf (stderr, "Error getting movie\n");
	ds_dealloc_multi (multi_desc);
	return (FALSE);
    }
    if (movie_pseudo == NULL)
    {
	/*  Movie is TrueColour: use RGB canvas if possible, PseudoColour
	    canvas if not  */
	main_canvas = (rgb_canvas == NULL) ? pseudo_canvas : rgb_canvas;
	mag_canvas = (mag_rgb_canvas == NULL) ?
	    mag_pseudo_canvas : mag_rgb_canvas;
	if ( ( *movie =
	      viewimg_create_rgb_sequence (main_canvas, multi_desc,
					   movie_red->arr_desc,
					   movie_red->data,
					   2, 1, 0,
					   movie_red->elem_index,
					   movie_green->elem_index,
					   movie_blue->elem_index,
					   0, (char **) NULL,
					   (double *) NULL) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error creating ViewableImage sequence\n");
	    ds_dealloc_multi (multi_desc);
	    iarray_dealloc (movie_red);
	    iarray_dealloc (movie_green);
	    iarray_dealloc (movie_blue);
	    return (FALSE);
	}
	if ( ( *magnified_movie =
	      viewimg_create_rgb_sequence (mag_canvas, multi_desc,
					   movie_red->arr_desc,
					   movie_red->data,
					   2, 1, 0,
					   movie_red->elem_index,
					   movie_green->elem_index,
					   movie_blue->elem_index,
					   0, (char **) NULL,
					   (double *) NULL) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error creating ViewableImage sequence\n");
	    ds_dealloc_multi (multi_desc);
	    iarray_dealloc (movie_red);
	    iarray_dealloc (movie_green);
	    iarray_dealloc (movie_blue);
	    destroy_all_vimages (image, movie,
				 magnified_image, magnified_movie,
				 num_frames);
	    return (FALSE);
	}
	*num_frames = iarray_dim_length (movie_red, 0);
	iarray_dealloc (movie_red);
	iarray_dealloc (movie_green);
	iarray_dealloc (movie_blue);
    }
    else
    {
	/*  Movie is PseudoColour: must use PseudoColour canvas  */
	if (pseudo_canvas == NULL)
	{
	    (void) fprintf (stderr, "No PseudoColour canvas available\n");
	    iarray_dealloc (movie_pseudo);
	    ds_dealloc_multi (multi_desc);
	    return (FALSE);
	}
	main_canvas = pseudo_canvas;
	mag_canvas = mag_pseudo_canvas;
	if ( ( *movie =
	      viewimg_create_sequence (main_canvas, multi_desc,
				       movie_pseudo->arr_desc,
				       movie_pseudo->data,
				       2, 1, 0, movie_pseudo->elem_index) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error creating ViewableImage sequence\n");
	    ds_dealloc_multi (multi_desc);
	    iarray_dealloc (movie_pseudo);
	    return (FALSE);
	}
	if ( ( *magnified_movie =
	      viewimg_create_sequence (mag_canvas, multi_desc,
				       movie_pseudo->arr_desc,
				       movie_pseudo->data,
				       2, 1, 0, movie_pseudo->elem_index) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error creating ViewableImage sequence\n");
	    ds_dealloc_multi (multi_desc);
	    iarray_dealloc (movie_pseudo);
	    destroy_all_vimages (image, movie,
				 magnified_image, magnified_movie,
				 num_frames);
	    return (FALSE);
	}
	*num_frames = iarray_dim_length (movie_pseudo, 0);
	cmap = canvas_get_cmap (main_canvas);
	if ( (cmap != NULL) && (cmap_index < multi_desc->num_arrays) )
	{
	    if ( !kcmap_copy_from_struct (cmap,
					  multi_desc->headers[cmap_index],
					  multi_desc->data[cmap_index]) )
	    {
		(void) fprintf (stderr, "Error changing colourmap\n");
	    }
	    i_min = 0;
	    i_max = kcmap_get_pixels (cmap, (unsigned long **) NULL) - 1;
	}
	else
	{
	    (void)fprintf(stderr,"Computing minimum and maximum of cube...\n");
	    if ( !iarray_min_max (movie_pseudo, CONV1_REAL, &i_min, &i_max) )
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
	    else fprintf (stderr, "Minimum: %e  maximum: %e\n",i_min, i_max);
	}
	*pseudo_arr = movie_pseudo;
	/*  Set intensity scale (since autoscaling is not on)  */
	canvas_set_attributes (main_canvas,
			       CANVAS_ATT_VALUE_MIN, i_min,
			       CANVAS_ATT_VALUE_MAX, i_max,
			       CANVAS_ATT_END);
	canvas_set_attributes (mag_canvas,
			       CANVAS_ATT_VALUE_MIN, i_min,
			       CANVAS_ATT_VALUE_MAX, i_max,
			       CANVAS_ATT_END);
    }
    ds_dealloc_multi (multi_desc);
    if ( !viewimg_make_active (movie[0][0]) )
    {
        (void) fprintf (stderr, "Error making ViewableImage active\n");
        return (FALSE);
    }
    if ( !viewimg_make_active (magnified_movie[0][0]) )
    {
        (void) fprintf (stderr, "Error making ViewableImage active\n");
        return (FALSE);
    }
    return (TRUE);
}   /*  End Function display_file  */


/*  Private functions follow  */

static void destroy_all_vimages (ViewableImage *image,
				 ViewableImage **movie,
				 ViewableImage *magnified_image,
				 ViewableImage **magnified_movie,
				 unsigned int *num_frames)
/*  [PURPOSE] This routine will destroy all ViewableImages and clear the
    appropriate pointers.
    [RETURNS] Nothing.
*/
{
    unsigned int count;

    for (count = 0; count < *num_frames; ++count)
    {
	if ( (*movie != NULL) && ( (*movie)[count] != NULL ) )
	{
	    viewimg_destroy ( (*movie)[count] );
	    (*movie)[count] = NULL;
	}
	if ( (*magnified_movie != NULL) &&
	     ( (*magnified_movie)[count] != NULL ) )
	{
	    viewimg_destroy ( (*magnified_movie)[count] );
	    (*magnified_movie)[count] = NULL;
	}
    }
    if (*movie != NULL) m_free ( (char *) *movie );
    *movie = NULL;
    if (*magnified_movie != NULL) m_free ( (char *) *magnified_movie );
    *magnified_movie = NULL;
    *num_frames = 0;
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

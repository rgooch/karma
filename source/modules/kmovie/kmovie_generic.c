/*  kmovie_generic.c

    Generic file for  kmovie  (X11 movie display tool for Karma).

    Copyright (C) 1994  Richard Gooch

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
    the general data structure format. The arrayfile must contain a 3
    dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   25-DEC-1994: Copied from  kmovie_generic.c

    Last updated by Richard Gooch   25-DEC-1994


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#include <karma.h>
#include <karma_viewimg.h>
#include <karma_iarray.h>
#include <karma_dsxfr.h>
#include <karma_kcmap.h>
#include <karma_conn.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_r.h>


/*  External functions  */
/*  File: kmovie_main.c  */
EXTERN_FUNCTION (void load_and_setup, (char *arrayfile) );


/*  Local functions  */
EXTERN_FUNCTION (void setup_comms, () );
EXTERN_FUNCTION (flag load,
		 (char *arrayfile,
		  KWorldCanvas pseudo_canvas, KWorldCanvas rgb_canvas) );


/*  Private functions  */
STATIC_FUNCTION (void new_data_on_connection, (flag first_time_data) );
STATIC_FUNCTION (void connection_closed, (flag data_deallocated) );


/*  Public data follows  */
iarray movie_pseudo = NULL;
ViewableImage *frames = NULL;
unsigned int num_frames = 0;


/*  Public routines follow  */

void setup_comms (display)
/*  This routine will initialise the communications system.
    The display the module is connected to must be pointed to by  display  .
    NOTE:  conn_register_managers  MUST be called first.
    The routine returns nothing.
*/
Display *display;
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
    if (conn_become_server (&server_port_number, CONN_MAX_INSTANCES) != TRUE)
    {
	(void) fprintf (stderr, "Module not operating as Karma server\n");
    }
    else
    {
	(void) fprintf (stderr, "Port allocated: %d\n", server_port_number);
	/*  Register the protocols  */
	dsxfr_register_connection_limits (1, -1);
	dsxfr_register_read_func (new_data_on_connection);
	dsxfr_register_close_func (connection_closed);
    }
}   /*  End Function setup_comms  */

flag load (char *arrayfile, KWorldCanvas pseudo_canvas,KWorldCanvas rgb_canvas)
/*  This routine will load an arrayfile.
*/
{
    Kcolourmap cmap;
    iarray movie_red, movie_green, movie_blue;
    int width, height;
    unsigned cmap_index, count;
    struct win_scale_type win_scale;
    multi_array *multi_desc;
    extern iarray movie_pseudo;
    extern unsigned int num_frames;
    extern ViewableImage *frames;
    static char function_name[] = "load";

    if ( ( multi_desc = dsxfr_get_multi (arrayfile, FALSE,
					 K_CH_MAP_IF_AVAILABLE, FALSE) )
	== NULL )
    {
	(void) fprintf (stderr, "Error getting arrayfile: \"%s\"\n",
			arrayfile);
	return (FALSE);
    }
    if (rgb_canvas == NULL) rgb_canvas = pseudo_canvas;
    if (movie_pseudo != NULL)
    {
	iarray_dealloc (movie_pseudo);
	movie_pseudo = NULL;
    }
    if ( !iarray_get_movie_from_multi (multi_desc, &movie_pseudo,
				       &movie_red, &movie_green, &movie_blue,
				       &cmap_index) )
    {
	(void) fprintf (stderr, "Error getting movie\n");
	ds_dealloc_multi (multi_desc);
	return (FALSE);
    }
    /*  Deallocate old ViewableImage objects  */
    for (count = 0; count < num_frames; ++count)
    {
	viewimg_destroy (frames[count]);
    }
    num_frames = 0;
    if (frames != NULL) m_free ( (char *) frames );
    if (movie_pseudo == NULL)
    {
	if ( ( frames =
	      viewimg_create_rgb_sequence (rgb_canvas, multi_desc,
					   (*movie_red).arr_desc,
					   (*movie_red).data,
					   2, 1, 0,
					   (*movie_red).elem_index,
					   (*movie_green).elem_index,
					   (*movie_blue).elem_index,
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
	num_frames = iarray_dim_length (movie_red, 0);
	iarray_dealloc (movie_red);
	iarray_dealloc (movie_green);
	iarray_dealloc (movie_blue);
    }
    else
    {
	if ( ( frames =
	      viewimg_create_sequence (pseudo_canvas, multi_desc,
				       (*movie_pseudo).arr_desc,
				       (*movie_pseudo).data,
				       2, 1, 0, (*movie_pseudo).elem_index) )
	    == NULL )
	{
	    (void) fprintf (stderr, "Error creating ViewableImage sequence\n");
	    ds_dealloc_multi (multi_desc);
	    iarray_dealloc (movie_pseudo);
	    return (FALSE);
	}
	num_frames = iarray_dim_length (movie_pseudo, 0);
	canvas_get_size (pseudo_canvas, &width, &height, &win_scale);
	cmap = canvas_get_cmap (pseudo_canvas);
	if ( (cmap != NULL) && (cmap_index < (*multi_desc).num_arrays) )
	{
	    if ( !kcmap_copy_from_struct (cmap,
					  (*multi_desc).headers[cmap_index],
					  (*multi_desc).data[cmap_index]) )
	    {
		(void) fprintf (stderr, "Error changing colourmap\n");
	    }
	    win_scale.z_min = 0;
	    win_scale.z_max = kcmap_get_pixels (cmap,
						(unsigned long **) NULL) - 1;
	}
	else
	{
	    (void)fprintf(stderr,"Computing minimum and maximum of cube...\n");
	    if (iarray_min_max (movie_pseudo, CONV1_REAL,
				&(win_scale.z_min), &(win_scale.z_max))
		!= TRUE)
	    {
		(void) fprintf (stderr, "Error computing min-max\n");
	    }
	    (void) fprintf (stderr, "Minimum: %e  maximum: %e\n",
			    win_scale.z_min, win_scale.z_max);
	}
	/*  Set intensity scale (since autoscaling is not on)  */
	if (canvas_resize (pseudo_canvas, &win_scale, FALSE) != TRUE)
	{
	    (void) fprintf (stderr, "Error resizing world canvas\n");
	}
    }
    if (viewimg_make_active (frames[0]) != TRUE)
    {
        (void) fprintf (stderr, "Error making ViewableImage active\n");
        return (FALSE);
    }
    ds_dealloc_multi (multi_desc);
    return (TRUE);
}   /*  End Function load  */


/*  Private routines follow  */

static void new_data_on_connection (first_time_data)
/*  This routine is called when new data arrives.
    If data is arriving the first time, then  first_time_data  will be TRUE.
    The routine returns nothing.
*/
flag first_time_data;
{
    load_and_setup ("connection");
}   /*  End Function new_data_on_connection  */

static void connection_closed (data_deallocated)
/*  This routine is called when the "multi_array" connection closes.
    If there was data on the connection, the value of  data_deallocated  will
    be TRUE.
    The routine returns nothing.
*/
flag data_deallocated;
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

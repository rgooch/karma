/*  kview_2d_generic.c

    Generic file for  kview_2d  (X11 image display tool for Karma).

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
    the general data structure format. The arrayfile must contain a 2
    dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   25-DEC-1994: Created ImageDisplay widget
  from old  kview_2d  code and rewrote  kview_2d

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
/*  File: kview_2d_main.c  */
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
iarray image_pseudo = NULL;


/*  Private data follows  */
static ViewableImage vimage = NULL;


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
    iarray image_red, image_green, image_blue;
    unsigned cmap_index;
    multi_array *multi_desc;
    extern ViewableImage vimage;
    extern iarray image_pseudo;
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
    if (image_pseudo != NULL)
    {
	iarray_dealloc (image_pseudo);
	image_pseudo = NULL;
    }
    if ( !iarray_get_image_from_multi (multi_desc, &image_pseudo,
				       &image_red, &image_green, &image_blue,
				       &cmap_index) )
    {
	(void) fprintf (stderr, "Error getting image\n");
	ds_dealloc_multi (multi_desc);
	return (FALSE);
    }
    if (vimage != NULL) viewimg_destroy (vimage);
    if (image_pseudo == NULL)
    {
	if ( ( vimage = viewimg_create_rgb (rgb_canvas, multi_desc,
					    (*image_red).arr_desc,
					    (*image_red).data, 1, 0,
					    (*image_red).elem_index,
					    (*image_green).elem_index,
					    (*image_blue).elem_index,
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
	iarray_dealloc (image_red);
	iarray_dealloc (image_green);
	iarray_dealloc (image_blue);
    }
    else
    {
	if ( ( vimage = viewimg_create_from_iarray (pseudo_canvas,image_pseudo,
						    FALSE) ) == NULL )
	{
	    (void) fprintf (stderr,
			    "Error getting ViewableImage from Iarray\n");
	    ds_dealloc_multi (multi_desc);
	    iarray_dealloc (image_pseudo);
	    image_pseudo = NULL;
	    return (FALSE);
	}
    }
    if (viewimg_make_active (vimage) != TRUE)
    {
        (void) fprintf (stderr, "Error making ViewableImage active\n");
        return (FALSE);
    }
    cmap = canvas_get_cmap (pseudo_canvas);
    if ( (cmap != NULL) && (cmap_index < (*multi_desc).num_arrays) )
    {
	if ( !kcmap_copy_from_struct (cmap, (*multi_desc).headers[cmap_index],
				      (*multi_desc).data[cmap_index]) )
	{
	    (void) fprintf (stderr, "Error changing colourmap\n");
	}
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
    extern ViewableImage vimage;

    if (vimage != NULL) viewimg_destroy (vimage);
    vimage = NULL;
}   /*  End Function connection_closed  */

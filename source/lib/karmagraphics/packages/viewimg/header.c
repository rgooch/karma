/*LINTLIBRARY*/
/*  header.c

    This code provides ViewableImage objects.

    Copyright (C) 1996  Richard Gooch

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Software Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*  This file contains convenience routines which implement a header display
  user interface.


    Written by      Richard Gooch   30-NOV-1996

    Last updated by Richard Gooch   30-NOV-1996


*/
#include <stdio.h>
#include <math.h>
#include <karma.h>
#define KWIN_GENERIC_ONLY
#define NEW_WIN_SCALE
#include <k_event_codes.h>
#include <karma_viewimg.h>
#include <karma_dmp.h>
#include <karma_ds.h>


/*  Private structures  */


/*  Private functions  */
STATIC_FUNCTION (void show_vm_structure, (CONST multi_array *multi_desc) );


/*  Public functions follow  */

/*EXPERIMENTAL_FUNCTION*/
flag viewimg_header_position_func (ViewableImage vimage,
				   double x, double y,
				   void *value, unsigned int event_code,
				   void *e_info, void **f_info,
				   double x_lin, double y_lin,
				   unsigned int value_type)
/*  [SUMMARY] Position event callback.
    [PURPOSE] This routine is a position event consumer for a world canvas
    which has a number of ViewableImage objects associated with it.
    <viewimg> The active viewable image.
    <x> The horizontal world co-ordinate of the event.
    <y> The vertical world co-ordinate of the event.
    These values will have been transformed by the registered transform
    function (see [<canvas_register_transform_func>]) for the associated
    world canvas.
    <value> A pointer to the data value in the viewable image corresponding
    to the event co-ordinates.
    <event_code> The arbitrary event code.
    <e_info> The arbitrary event information.
    <f_info> The arbitrary function information pointer.
    <x_lin> The linear horizontal world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <y_lin> The linear vertical world co-ordinate (the co-ordinate prior
    to the transform function being called).
    <value_type> The type of the data value. This may be K_DCOMPLEX or
    K_UB_RGB.
    [RETURNS] TRUE if the event was consumed, else FALSE indicating that
    the event is still to be processed.
*/
{
    multi_array *multi_desc;
    CONST char *string = e_info;
    /*static char function_name[] = "viewimg_header_position_func";*/

    if (event_code != K_CANVAS_EVENT_PLAIN_KEY_PRESS) return (FALSE);
    if (strcmp (string, "h") != 0) return (FALSE);
    viewimg_get_attributes (vimage,
			    VIEWIMG_VATT_MULTI_ARRAY, &multi_desc,
			    VIEWIMG_VATT_END);
    if (multi_desc == NULL)
    {
	fprintf (stderr, "No multi_array data structure available\n");
	return (TRUE);
    }
    show_vm_structure (multi_desc);
    return (TRUE);
}   /*  End Function viewimg_statistics_position_func  */

static void show_vm_structure (CONST multi_array *multi_desc)
/*  [SUMMARY] Show the header of a structure in virtual memory.
    <multi_desc> The structure.
    [RETURNS] Nothing.
*/
{
    unsigned int count, type;
    packet_desc *top_pack_desc;
    history *hist;
    char *top_packet;

    /* Assume all the interesting information is in the first data structure */
    top_pack_desc = multi_desc->headers[0];
    top_packet = multi_desc->data[0];
    for (count = 0; count < top_pack_desc->num_elements; ++count)
    {
	type = top_pack_desc->element_types[count];
	if (type == K_ARRAY)
	{
	    dmp_array_desc (stdout,
			    (array_desc *) top_pack_desc->element_desc[count],
			    FALSE);
	}
	else if (type == LISTP)
	{
	    puts ("Linked List header");
	}
	else if ( ds_element_is_named (type) )
	{
	    printf ("Element: \"%s\"\t\t",
		    top_pack_desc->element_desc[count]);
	    dmp_element (stdout, type, top_pack_desc->element_desc[count],
			 top_packet +
			 ds_get_element_offset (top_pack_desc, count),
			 FALSE);
	}
	else
	{
	    printf ("Unknown element type: %u\n", type);
	}
    }
    /*  Show history  */
    if (multi_desc->first_hist != NULL) puts ("");
    for (hist = multi_desc->first_hist; hist != NULL; hist = hist->next)
    {
	printf ("HISTORY: %s\n", hist->string);
    }
    fflush (stdout);
}   /*  End Function show_vm_structure  */

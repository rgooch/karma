/*LINTLIBRARY*/
/*  edit.c

    This code provides routines to draw editing instructions on world canvases.

    Copyright (C) 1993,1994  Richard Gooch

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

/*  This file contains all routines needed for processing lists of
    2-dimensional editing instructions and drawing onto a world canvas.


    Written by      Richard Gooch   26-APR-1993

    Updated by      Richard Gooch   27-APR-1993

    Updated by      Richard Gooch   3-MAY-1993: Implemented
  EDIT_INSTRUCTION_FPOLY.

    Updated by      Richard Gooch   27-AUG-1993: Took account of changes to
  iedit_  package.

    Last updated by Richard Gooch   26-NOV-1994: Moved to
  packages/canvas/edit.c


*/
#include <stdio.h>
#include <math.h>
#include <karma_ds.h>

#define KWIN_GENERIC_ONLY
#include <karma_canvas.h>

#include <karma_iedit.h>
#include <karma_a.h>


/*  Private functions  */
static flag draw_dab (/* canvas, coord_list_head, value */);
static flag draw_stroke (/* canvas, coord_list_head,
			    value */);
static flag draw_polygon (/* canvas, coord_list_head,
			     value */);


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag canvas_draw_edit_list (canvas, ilist)
/*  This routine will draw a list of edit objects to a world canvas.
    The canvas must be given by  canvas  .
    The instruction list must be given by  ilist  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KWorldCanvas canvas;
KImageEditList ilist;
{
    unsigned int count;
    unsigned int pack_size;
    char *data;
    packet_desc *edit_list_desc;
    list_header *list_head;
    list_entry *curr_entry;
    static char function_name[] = "canvas_draw_edit_list";

    if ( (canvas == NULL) || (ilist == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    edit_list_desc = iedit_get_instruction_desc ();
    list_head = iedit_get_list (ilist);
    if ( (*list_head).magic != MAGIC_LIST_HEADER )
    {
	(void) fprintf (stderr, "List header has bad magic number\n");
	a_prog_bug (function_name);
    }
    /*  Process instructions  */
    /*  Process contiguous section of list  */
    pack_size = ds_get_packet_size (edit_list_desc);
    for (count = 0, data = (*list_head).contiguous_data;
	 count < (*list_head).contiguous_length; ++count, data += pack_size)
    {
	if (canvas_draw_edit_object (canvas, data)
	    != TRUE)
	{
	    return (FALSE);
	}
    }
    /*  Process fragmented section of list  */
    for (curr_entry = (*list_head).first_frag_entry; curr_entry != NULL;
	 curr_entry = (*curr_entry).next)
    {
	if (canvas_draw_edit_object (canvas, (*curr_entry).data)
	    != TRUE)
	{
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function canvas_draw_edit_list  */

/*PUBLIC_FUNCTION*/
flag canvas_draw_edit_object (canvas, object)
/*  This routine will draw one edit object to a world canvas.
    The canvas must be given by  canvas  .
    The first element in the instruction entry which is a pointer to a linked
    list must be the linked list of co-ordinates.
    The routine will draw the edit object pointed to by  object  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KWorldCanvas canvas;
char *object;
{
    unsigned int instruction_code;
    unsigned int elem_count;
    unsigned int edit_coord_list_index;
    double value[2];
    list_header *coord_list_head;
    packet_desc *edit_list_desc;
    static char function_name[] = "canvas_draw_edit_object";

    if (canvas == NULL)
    {
	(void) fprintf (stderr, "NULL canvas passed\n");
	a_prog_bug (function_name);
    }
    if (object == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    edit_list_desc = iedit_get_instruction_desc ();
    /*  Find first linked list (should be the co-ordinate list)  */
    for (elem_count = 0,
	 edit_coord_list_index = (*edit_list_desc).num_elements;
	 elem_count < (*edit_list_desc).num_elements;
	 ++elem_count)
    {
	if (LISTP == (*edit_list_desc).element_types[elem_count])
	{
	    edit_coord_list_index = elem_count;
	}
    }
    if (edit_coord_list_index >= (*edit_list_desc).num_elements)
    {
	(void) fprintf (stderr, "No linked list found\n");
	a_prog_bug (function_name);
    }
    /*  Get instruction code  */
    if (ds_get_unique_named_value (edit_list_desc, object,
				   "Edit Instruction",
				   (unsigned int *) NULL, value) != TRUE)
    {
	(void) fprintf (stderr, "Error getting edit instruction code\n");
	return (FALSE);
    }
    instruction_code = (unsigned int) value[0];
    /*  Get data value and convert to pixel value  */
    if (ds_get_unique_named_value (edit_list_desc, object,
				   "Edit Object Value",
				   (unsigned int *) NULL, value) != TRUE)
    {
	(void) fprintf (stderr, "Error getting edit object value\n");
	return (FALSE);
    }
    coord_list_head = ( *(list_header **)
		       ( object +
			ds_get_element_offset (edit_list_desc,
					       edit_coord_list_index) ) );
    if ( (*coord_list_head).magic != MAGIC_LIST_HEADER )
    {
	(void) fprintf (stderr,
			"Co-ordinate list header has bad magic number\n");
	a_prog_bug (function_name);
    }
    switch (instruction_code)
    {
      case EDIT_INSTRUCTION_DAB:
	if (draw_dab (canvas, coord_list_head, value)
	    != TRUE)
	{
	    (void) fprintf (stderr, "Error drawing dab\n");
	    return (FALSE);
	}
	break;
      case EDIT_INSTRUCTION_STROKE:
	if (draw_stroke (canvas, coord_list_head, value)
	    != TRUE)
	{
	    (void) fprintf (stderr, "Error drawing stroke\n");
	    return (FALSE);
	}
	break;
      case EDIT_INSTRUCTION_FPOLY:
	if (draw_polygon (canvas, coord_list_head, value)
	    != TRUE)
	{
	    return (FALSE);
	}
	break;
      case EDIT_APPLY_INSTRUCTIONS:
	(void) fprintf (stderr,
			"2D edit instruction: Apply not drawable\n");
	a_prog_bug (function_name);
	break;
      case EDIT_UNDO_INSTRUCTIONS:
	(void) fprintf (stderr,
			"2D edit instruction: Undo not drawable\n");
	a_prog_bug (function_name);
	break;
      case EDIT_SET_POINT:
	(void) fprintf (stderr,
			"2D edit instruction: Pixel_edit not implemented yet\n");
	return (FALSE);
/*
	break;
*/
      default:
	(void) fprintf (stderr, "Illegal edit instruction: %u\n",
			instruction_code);
	return (FALSE);
/*
	break;
*/
    }
    return (TRUE);
}   /*  End Function canvas_draw_edit_object  */


/*  Private functions follow  */

static flag draw_dab (canvas, coord_list_head, value)
/*  This routine will draw a dab onto a world canvas.
    The canvas must be given by  canvas  .
    The co-ordinate list header must be pointed to by  coord_list_head  .
    The value to use when drawing must be given by  value  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KWorldCanvas canvas;
list_header *coord_list_head;
double *value;
{
    int cx;
    int cy;
    int rx;
    int ry;
    edit_coord *coords;
    static char function_name[] = "draw_dab";

    if ( (*coord_list_head).length != 2 )
    {
	(void) fprintf (stderr, "Dab requires 2 points, got: %u\n",
			(*coord_list_head).length);
	return (FALSE);
    }
    if (iedit_get_edit_coords (coord_list_head, &coords) != TRUE)
    {
	(void) fprintf (stderr, "Error getting co-ordinates\n");
	return (FALSE);
    }
    canvas_fill_ellipse (canvas,
			 coords[0].abscissa, coords[0].ordinate,
			 coords[1].abscissa, coords[1].ordinate, 
			 value);
    return (TRUE);
}   /*  End Function draw_dab  */

static flag draw_stroke (canvas, coord_list_head, value)
/*  This routine will draw a stroke onto a world canvas.
    The canvas must be given by  canvas  .
    The co-ordinate list header must be pointed to by  coord_list_head  .
    The value to use when drawing must be given by  value  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KWorldCanvas canvas;
list_header *coord_list_head;
double *value;
{
    unsigned int coord_count;
    edit_coord *coords;
    static char function_name[] = "draw_stroke";

    if ( (*coord_list_head).length != 4 )
    {
	(void) fprintf (stderr, "Stroke requires 4 points, got: %u\n",
			(*coord_list_head).length);
	return (FALSE);
    }
    if (iedit_get_edit_coords (coord_list_head, &coords) != TRUE)
    {
	(void) fprintf (stderr, "Error getting co-ordinates\n");
	return (FALSE);
    }
    return ( canvas_fill_polygon (canvas, coords, 4, value, TRUE) );
}   /*  End Function draw_stroke  */

static flag draw_polygon (canvas, coord_list_head, value)
/*  This routine will draw a polygon onto a world canvas.
    The canvas must be given by  canvas  .
    The co-ordinate list header must be pointed to by  coord_list_head  .
    The value to use when drawing must be given by  value  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KWorldCanvas canvas;
list_header *coord_list_head;
double *value;
{
    unsigned int coord_count;
    edit_coord *coords;
    static char function_name[] = "draw_polygon";

    if (iedit_get_edit_coords (coord_list_head, &coords) != TRUE)
    {
	(void) fprintf (stderr, "Error getting co-ordinates\n");
	return (FALSE);
    }
    return ( canvas_fill_polygon (canvas, coords, (*coord_list_head).length,
				  value, FALSE) );
}   /*  End Function draw_polygon  */

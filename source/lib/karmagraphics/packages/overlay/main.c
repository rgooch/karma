/*LINTLIBRARY*/
/*  main.c

    This code provides KOverlayList objects.

    Copyright (C) 1993-1996  Richard Gooch

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

/*  This file contains all routines needed for manipulating overlay lists.


    Written by      Richard Gooch   2-DEC-1993

    Updated by      Richard Gooch   6-DEC-1993: Added  overlay_empty_list  .

    Updated by      Richard Gooch   10-DEC-1993: Removed  overlay_empty_list
  and added  overlay_remove_objects  and  overlay_ellipses  .

    Updated by      Richard Gooch   12-DEC-1993: Converted to token passing
  system for communications.

    Updated by      Richard Gooch   19-MAY-1994: Switched to  canvas_get_colour

    Updated by      Richard Gooch   25-OCT-1994: Padded list of co-ordinates
  because of stupid Sparc processor problems with misaligned data accesses.
  ds_get_element  needs to be made tolerant. Incremented protocol number.

    Updated by      Richard Gooch   12-NOV-1994: Fixed bug in
  process_token_receive  where connection to slave with token was not
  reset, causing a master to fail if it has the token and a slave disconnects.

    Updated by      Richard Gooch   20-NOV-1994: Switched over to use of
  kwin_load_font  .

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/overlay/main.c

    Updated by      Richard Gooch   25-JUL-1995: Moved object packet elements
  around to fix bus error on r8000s.

    Updated by      Richard Gooch   21-SEP-1995: Fixed bugs in
  <overlay_ellipses> and <overlay_vectors>: error condition test inverted.

    Updated by      Richard Gooch   22-DEC-1995: Trap NULL co-ordinate arrays
  in <overlay_lines> routine.

    Updated by      Richard Gooch   28-FEB-1996: Added
  #include <karma_iarray.h>

    Last updated by Richard Gooch   14-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   19-MAY-1996: Changed from using window
  scale structure to using <canvas_get_attributes>.

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Last updated by Richard Gooch   4-JUN-1996: Switched to left-right
  bottom-top co-ordinate specification instead of min-max x and y.


*/
#include <stdio.h>
#include <math.h>
#include <string.h>
#define KWIN_GENERIC_ONLY
#define NEW_WIN_SCALE
#include <karma_overlay.h>
#include <karma_iarray.h>
#include <karma_dsrw.h>
#include <karma_dsra.h>
#include <karma_conn.h>
#include <karma_ch.h>
#include <karma_pio.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_a.h>
#include <karma_m.h>


#define MAGIC_NUMBER (unsigned int) 528762177
#define PROTOCOL_VERSION (unsigned int) 3

#define OBJECT_COORD_INDEX (unsigned int) 0
#define OBJECT_COLOURNAME_INDEX (unsigned int) 1
#define OBJECT_X_INDEX (unsigned int) 2
#define OBJECT_Y_INDEX (unsigned int) 3
#define OBJECT_RESTRICTION_INDEX (unsigned int) 4
#define OBJECT_TEXT_STRING_INDEX (unsigned int) 5
#define OBJECT_TEXT_FONTNAME_INDEX (unsigned int) 6
#define OBJECT_CODE_INDEX (unsigned int) 7
#define OBJECT_GP_UINT_INDEX (unsigned int) 8
#define OBJECT_LISTID_INDEX (unsigned int) 9
#define OBJECT_OBJECTID_INDEX (unsigned int) 10

#define OBJECT_TEXT_CLEAR_UNDER_INDEX OBJECT_GP_UINT_INDEX

#define COORD_TYPE_INDEX (unsigned int) 0
#define COORD_X_INDEX (unsigned int) 2
#define COORD_Y_INDEX (unsigned int) 3

#define RESTRICTION_NAME_INDEX (unsigned int) 0
#define RESTRICTION_VALUE_INDEX (unsigned int) 2

#define OBJECT_LINE (unsigned int) 0
#define OBJECT_LINES (unsigned int) 1
#define OBJECT_TEXT (unsigned int) 2
#define OBJECT_ELLIPSE (unsigned int) 3
#define OBJECT_FELLIPSE (unsigned int) 4
#define OBJECT_FPOLY (unsigned int) 5
#define OBJECT_VECTOR (unsigned int) 6
#define OBJECT_ELLIPSES (unsigned int) 7
#define OBJECT_FELLIPSES (unsigned int) 8
#define OBJECT_SEGMENTS (unsigned int) 9
#define OBJECT_VECTORS (unsigned int) 10
#define OBJECT_REMOVE_OBJECTS (unsigned int) 11
#define OBJECT_REQUEST_TOKEN (unsigned int) 12
#define OBJECT_GRANT_TOKEN (unsigned int) 13
#define OBJECT_REMOVE_OBJECT (unsigned int) 14
#define OBJECT_MOVE_OBJECT (unsigned int) 15

#define VERIFY_OVERLAYLIST(olist) if (olist == NULL) \
{(void) fprintf (stderr, "NULL overlay list passed\n"); \
 a_prog_bug (function_name); } \
if (olist->magic_number != MAGIC_NUMBER) \
{(void) fprintf (stderr, "Invalid overlay list object\n"); \
 a_prog_bug (function_name); }

/*  Some explanation of list IDs:
    The list master allocates slave IDs from a monotonically increasing
    counter. Slave IDs start at 2. ID 1 is the master. ID 0 is the list passed
    to the appropriate routines.
    The object IDs start at 1.
*/

struct overlay_list_type
{
    unsigned int magic_number;
    KWorldCanvas specification_canvas;
    list_header *list_head;
    list_header *buf_list; /*  Buffer for instructions until token received  */
    void *info;
    Connection master;
    unsigned int slave_count;
    Connection token_conn;                      /*  Slave who has the token  */
    flag have_token;                                   /*  I have the token  */
    flag requested_token;              /*  Have already requested the token  */
    struct token_request_type *first_token_request;     /*  Only for master  */
    struct token_request_type *last_token_request;      /*  Only for master  */
    unsigned int next_slave_id;                   /*  ID counter for slaves  */
    unsigned int my_id;                                /*  ListID for slave  */
    unsigned int last_object_id;                 /*  ID counter for objects  */
    char *xlabel;
    char *ylabel;
    unsigned int num_restrictions;
    char **restriction_names;
    double *restriction_values;
    struct refresh_canvas_type *refresh_canvases;
};

struct token_request_type
{
    Connection conn;
    struct token_request_type *next;
    struct token_request_type *prev;
};

struct refresh_canvas_type
{
    KWorldCanvas canvas;
    flag active;
    struct refresh_canvas_type *next;
};


/*  Private data  */
static packet_desc *object_desc = NULL;
static KOverlayList masterable_list = NULL;
static KOverlayList slaveable_list = NULL;
/*  NOTE: all pointer types (list, string) come first because int padding is
    not sufficient on some platforms (r8000).  */
static char *str_object_desc[] =
{
    "PACKET",
    "  11",
    "END",
    "  ELEMENT",
    "    LISTP",
    "  END",
    "    PACKET",
    "      4",
    "    END",
    "      ELEMENT",
    "        UINT",
    "        Overlay Coord Type",
    "      END",
    "      ELEMENT",
    "        UINT",
    "        Overlay Coord Pad",
    "      END",
    "      ELEMENT",
    "        DOUBLE",
    "        Overlay Coord Abscissa",
    "      END",
    "      ELEMENT",
    "        DOUBLE",
    "        Overlay Coord Ordinate",
    "      END",
    "  ELEMENT",
    "    VSTRING",
    "    Overlay Object Colourname",
    "  END",
    "  ELEMENT",
    "    VSTRING",
    "    Overlay Object Abscissa Label",
    "  END",
    "  ELEMENT",
    "    VSTRING",
    "    Overlay Object Ordinate Label",
    "  END",
    "  ELEMENT",
    "    LISTP",
    "  END",
    "    PACKET",
    "      3",
    "    END",
    "      ELEMENT",
    "        VSTRING",
    "        Overlay Restriction Name",
    "      END",
    "      ELEMENT",
    "        UINT",
    "        Overlay Restriction Pad",
    "      END",
    "      ELEMENT",
    "        DOUBLE",
    "        Overlay Restriction Value",
    "      END",
    "  ELEMENT",
    "    VSTRING",
    "    Overlay Text String",
    "  END",
    "  ELEMENT",
    "    VSTRING",
    "    Overlay Text Fontname",
    "  END",
    "  ELEMENT",
    "    UINT",
    "    Overlay Object Code",
    "  END",
    "  ELEMENT",
    "    UINT",
    "    Overlay GP UInteger",
    "  END",
    "  ELEMENT",
    "    UINT",
    "    Overlay ListID",
    "  END",
    "  ELEMENT",
    "    UINT",
    "    Overlay ObjectID",
    "  END",
    NULL
};


/*  Private functions  */
STATIC_FUNCTION (void initialise_overlay_package, () );
STATIC_FUNCTION (void worldcanvas_refresh_func,
		 (KWorldCanvas canvas, int width, int height,
		  struct win_scale_type *win_scale, Kcolourmap cmap,
		  flag cmap_resize, void **info) );
STATIC_FUNCTION (flag register_new_overlay_slave,
		 (Connection connection, void **info) );
STATIC_FUNCTION (flag read_instruction_from_slave,
		 (Connection connection, void **info) );
STATIC_FUNCTION (void register_slave_loss,
		 (Connection connection, void *info) );
STATIC_FUNCTION (flag verify_overlay_slave_connection, (void **info) );
STATIC_FUNCTION (flag register_overlay_slave_connection,
		 (Connection connection, void **info) );
STATIC_FUNCTION (flag read_instruction_from_master,
		 (Connection connection, void **info) );
STATIC_FUNCTION (void register_master_loss,
		 (Connection connection, void *info) );
STATIC_FUNCTION (flag transmit_to_slaves,
		 (KOverlayList olist, list_entry *object,
		  Connection except_conn) );
STATIC_FUNCTION (flag write_entries,
		 (Channel channel, packet_desc *list_desc,
		  list_entry *first_entry) );
STATIC_FUNCTION (flag process_instruction,
		 (KOverlayList olist, list_entry *instruction,
		  Connection conn) );
STATIC_FUNCTION (flag process_local_object,
		 (KOverlayList olist, list_entry *object, flag append) );
STATIC_FUNCTION (flag draw_object,
		 (KWorldCanvas canvas, char *object, char *xlabel,
		  char *ylabel, unsigned int num_restr, char **restr_names,
		  double *restr_values) );
STATIC_FUNCTION (void convert_to_pixcoords,
		 (KWorldCanvas canvas, unsigned int num_coords,
		  char *types, char *x_arr, char *y_arr,
		  unsigned int pack_size, int *px, int *py) );
STATIC_FUNCTION (flag send_token_request, (KOverlayList olist) );
STATIC_FUNCTION (flag send_token_grant, (KOverlayList olist,Connection conn) );
STATIC_FUNCTION (flag process_conn_token_request,
		 (KOverlayList olist, Connection conn) );
STATIC_FUNCTION (flag process_token_receive,
		 (KOverlayList olist, Connection conn) );
STATIC_FUNCTION (void remove_token_request,
		 (KOverlayList olist, Connection conn) );
STATIC_FUNCTION (flag process_app_instruction,
		 (KOverlayList olist, list_entry *instruction) );
STATIC_FUNCTION (list_entry *create_generic,
		 (KOverlayList olist, unsigned int object_code,
		  char *colourname, unsigned int num_coords,
		  packet_desc **coord_desc, char **coords,
		  unsigned int *object_id) );
STATIC_FUNCTION (flag remove_object,
		 (KOverlayList olist, unsigned int object_id,
		  unsigned int list_id) );
STATIC_FUNCTION (list_entry *find_object,
		 (KOverlayList olist, unsigned int object_id,
		  unsigned int list_id) );
STATIC_FUNCTION (flag move_object,
		 (KOverlayList olist, unsigned int object_id,
		  unsigned int list_id, double dx, double dy) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KOverlayList overlay_create_list (void *info)
/*  [SUMMARY] Create a managed overlay object list.
    <info> The arbitrary information pointer for the overlay list. This may be
    NULL.
    [RETURNS] A KOverlayList object on success, else NULL.
*/
{
    KOverlayList olist;
    extern KOverlayList masterable_list;
    extern KOverlayList slaveable_list;
    extern packet_desc *object_desc;
    static char function_name[] = "overlay_create_list";

    initialise_overlay_package ();
    if ( ( olist = (KOverlayList) m_alloc (sizeof *olist) ) == NULL )
    {
	m_error_notify (function_name, "KOverlayList object");
	return (NULL);
    }
    olist->magic_number = MAGIC_NUMBER;
    olist->specification_canvas = NULL;
    if ( ( olist->list_head = ds_alloc_list_head () ) == NULL )
    {
	m_error_notify (function_name, "list header");
	m_free ( (char *) olist );
	return (NULL);
    }
    if ( ( olist->buf_list = ds_alloc_list_head () ) == NULL )
    {
	m_error_notify (function_name, "buffer list header");
	m_free ( (char *) olist );
	ds_dealloc_list (object_desc, olist->buf_list);
	return (NULL);
    }
    olist->list_head->sort_type = SORT_RANDOM;
    olist->info = info;
    olist->master = NULL;
    olist->slave_count = 0;
    olist->token_conn = NULL;
    olist->have_token = TRUE;
    olist->requested_token = FALSE;
    olist->first_token_request = NULL;
    olist->last_token_request = NULL;
    olist->next_slave_id = 2;
    olist->my_id = 1;
    olist->last_object_id = 0;
    olist->xlabel = NULL;
    olist->ylabel = NULL;
    olist->num_restrictions = 0;
    olist->restriction_names = NULL;
    olist->restriction_values = NULL;
    olist->refresh_canvases = NULL;
    if (masterable_list == NULL) masterable_list = olist;
    if (slaveable_list == NULL) slaveable_list = olist;
    return (olist);
}   /*  End Function overlay_create_list  */


/*  Routines to set specification information follow  */

/*PUBLIC_FUNCTION*/
void overlay_specify_iarray_2d (KOverlayList olist, iarray array)
/*  [SUMMARY] Extract specifications from an Intelligent Array.
    [PURPOSE] This routine will specify horizontal and vertical label matching
    for an overlay list based on the dimension names of a 2-dimensional
    Intelligent Array. No further restrictions are imposed (any existing
    restrictions are removed) if the Intelligent Array is a pure 2-dimensional
    array. If the array is an alias of a plane of a 3-dimensional (or greater)
    array, then further restrictions are imposed.
    <olist> The overlay list object.
    <array> The Intelligent Array.
    [RETURNS] Nothing.
*/
{
    unsigned int xdim, ydim;
    unsigned int count;
    unsigned int num_restr;
    dim_desc **dimensions;
    static char function_name[] = "overlay_specify_iarray_2d";

    VERIFY_OVERLAYLIST (olist);
    olist->specification_canvas = NULL;
    /*  Free old information  */
    if (olist->xlabel != NULL) m_free (olist->xlabel);
    olist->xlabel = NULL;
    if (olist->ylabel != NULL) m_free (olist->ylabel);
    olist->ylabel = NULL;
    if (olist->restriction_names != NULL)
    {
	for (count = 0; count < olist->num_restrictions; ++count)
	{
	    if (olist->restriction_names[count] != NULL)
	    {
		m_free (olist->restriction_names[count]);
	    }
	}
	m_free ( (char *) olist->restriction_names );
    }
    if (olist->restriction_values != NULL)
    {
	m_free ( (char *) olist->restriction_values );
    }
    olist->restriction_values = NULL;
    /*  Copy labels  */
    dimensions = array->arr_desc->dimensions;
    xdim = array->orig_dim_indices[1];
    ydim = array->orig_dim_indices[0];
    if ( ( olist->xlabel = st_dup (dimensions[xdim]->name) ) == NULL )
    {
	m_abort (function_name, "x label");
    }
    if ( ( olist->ylabel = st_dup (dimensions[ydim]->name) ) == NULL )
    {
	m_abort (function_name, "y label");
    }
    /*  And now... for the restrictions  */
    num_restr = iarray_get_restrictions (array, &olist->restriction_names,
					 &olist->restriction_values);
    olist->num_restrictions = num_restr;
}   /*  End Function overlay_specify_iarray_2d  */

/*PUBLIC_FUNCTION*/
void overlay_specify_canvas (KOverlayList olist, KWorldCanvas canvas)
/*  [SUMMARY] Extract specification from a world canvas.
    [PURPOSE] This routine will register a world canvas to extract
    specification information from for all future overlay objects which are
    created with an overlay object list.
    <olist> The overlay list object.
    <canvas> The world canvas object.
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    static char function_name[] = "overlay_specify_canvas";

    VERIFY_OVERLAYLIST (olist);
    olist->specification_canvas = canvas;
    /*  Free old information  */
    if (olist->xlabel != NULL) m_free (olist->xlabel);
    olist->xlabel = NULL;
    if (olist->ylabel != NULL) m_free (olist->ylabel);
    olist->ylabel = NULL;
    if (olist->restriction_names != NULL)
    {
	for (count = 0; count < olist->num_restrictions; ++count)
	{
	    if (olist->restriction_names[count] != NULL)
	    {
		m_free (olist->restriction_names[count]);
	    }
	}
	m_free ( (char *) olist->restriction_names );
    }
    if (olist->restriction_values != NULL)
    {
	m_free ( (char *) olist->restriction_values );
    }
    olist->restriction_values = NULL;
    olist->num_restrictions = 0;
}   /*  End Function overlay_specify_canvas  */


/*  Routines for drawing on world canvases follow  */

/*PUBLIC_FUNCTION*/
flag overlay_associate_display_canvas (KOverlayList olist, KWorldCanvas canvas)
/*  [SUMMARY] Associate display canvas with overlay list.
    [PURPOSE] This routine will register a world canvas to display overlay
    objects on when overlay objects received (either by overlay_* function
    calls within the application or over a "2D_overlay" connection). Multiple
    canvases may  be associated with an overlay list.
    <olist> The overlay list object.
    <canvas> The world canvas object.
    [RETURNS] TRUE on success, else FALSE (indicating that an error occurred
    refreshing the canvas).
*/
{
    struct refresh_canvas_type *cnv;
    static char function_name[] = "overlay_associate_display_canvas";

    VERIFY_OVERLAYLIST (olist);
    /*  Search through for existing association  */
    for (cnv = olist->refresh_canvases; cnv != NULL; cnv = cnv->next)
    {
	if (canvas == cnv->canvas)
	{
	    /*  Canvas already associated: make it active if not already  */
	    if (cnv->active) return (TRUE);
	    cnv->active = TRUE;
	    if (olist->list_head->length < 1) return (TRUE);
	    /*  Display  */
	    if ( !canvas_resize (canvas, (struct win_scale_type *) NULL,
				 TRUE) )
	    {
		(void) fprintf (stderr, "Error refreshing canvas\n");
		return (FALSE);
	    }
	    return (TRUE);
	}
    }
    /*  Is not yet associated  */
    if ( ( cnv = (struct refresh_canvas_type *) m_alloc (sizeof *cnv) )
	== NULL )
    {
	m_abort (function_name, "refresh canvas structure");
    }
    cnv->canvas = canvas;
    cnv->next = olist->refresh_canvases;
    olist->refresh_canvases = cnv;
    cnv->active = TRUE;
    canvas_register_refresh_func (canvas,
				  ( void (*) () ) worldcanvas_refresh_func,
				  (void *) olist);
    if ( !canvas_resize (canvas, (struct win_scale_type *) NULL, TRUE) )
    {
	(void) fprintf (stderr, "Error refreshing canvas\n");
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function overlay_associate_display_canvas  */

/*PUBLIC_FUNCTION*/
flag overlay_unassociate_display_canvas (KOverlayList olist,
					 KWorldCanvas canvas)
/*  [SUMMARY] Dissassociate a world canvas to display overlay objects.
    <olist> The overlay list object.
    <canvas> The world canvas object.
    [RETURNS] TRUE if the canvas was associated, else FALSE.
*/
{
    struct refresh_canvas_type *cnv;
    static char function_name[] = "overlay_unassociate_display_canvas";

    VERIFY_OVERLAYLIST (olist);
    /*  Search through for association  */
    for (cnv = olist->refresh_canvases; cnv != NULL; cnv = cnv->next)
    {
	if (canvas == cnv->canvas)
	{
	    /*  Canvas associated: make it inactive if not already  */
	    if (cnv->active)
	    {
		cnv->active = FALSE;
		if (olist->list_head->length < 1) return (TRUE);
		/*  Refresh canvas  */
		(void) canvas_resize (canvas, (struct win_scale_type *) NULL,
				      TRUE);
		return (TRUE);
	    }
	    return (FALSE);
	}
    }
    /*  Is not associated: ignore  */
    return (FALSE);
}   /*  End Function overlay_unassociate_display_canvas  */

/*PUBLIC_FUNCTION*/
flag overlay_redraw_on_canvas (KOverlayList olist, KWorldCanvas canvas)
/*  [SUMMARY] Redraw an overlay list onto a world canvas.
    <olist> The overlay list object.
    <canvas> The world canvas object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int num_restr;
    char *xlabel;
    char *ylabel;
    char **restr_names;
    double *restr_values;
    list_header *list_head;
    list_entry *entry;
    static char function_name[] = "overlay_redraw_on_canvas";

    VERIFY_OVERLAYLIST (olist);
    list_head = olist->list_head;
    if (list_head->length < 1) return (TRUE);
    /*  Get canvas specification information  */
    canvas_get_specification (canvas, &xlabel, &ylabel,
			      &num_restr, &restr_names, &restr_values);
    if (list_head->contiguous_data)
    {
	(void) fprintf (stderr, "Overlay list has contiguous section!\n");
	a_prog_bug (function_name);
    }
    /*  Process fragmented section of list  */
    for (entry = list_head->first_frag_entry; entry != NULL;
	 entry = entry->next)
    {
	if ( !draw_object (canvas, entry->data, xlabel, ylabel,
			   num_restr, restr_names,
			   restr_values) ) return (FALSE);
    }
    return (TRUE);
}   /*  End Function overlay_redraw_on_canvas  */


/*  Drawing functions follow  */

/*PUBLIC_FUNCTION*/
unsigned int overlay_line (KOverlayList olist,
			   unsigned int type0, double x0,double y0,
			   unsigned int type1, double x1, double y1,
			   char *colourname)
/*  [SUMMARY] Add a line to an overlay object list. See also [<overlay_lines>].
    <olist> The overlay list object.
    <type0> The type of the first co-ordinate.
    <x0> The horizontal position of the first co-ordinate.
    <y0> The vertical position of the first co-ordinate.
    <type1> The type of the second co-ordinate.
    <x1> The horizontal position of the second co-ordinate.
    <y1> The vertical position of the second co-ordinate.
    <colourname> The colourname.
    [RETURNS] The objectID on success, else 0.
*/
{
    unsigned int pack_size;
    unsigned int object_id;
    double offset = 0.01;
    char *coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    static char function_name[] = "overlay_line";

    VERIFY_OVERLAYLIST (olist);
    if ( ( object = create_generic (olist, OBJECT_LINE, colourname,
				    2, &coord_desc, &coords, &object_id) )
	== NULL )
    {
	m_error_notify (function_name, "object");
	return (FALSE);
    }
    value[1] = 0.0;
    pack_size = ds_get_packet_size (coord_desc);
    value[0] = type0 + offset;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Type",
				value) ) return (FALSE);
    value[0] = x0;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Abscissa",
				value) ) return (FALSE);
    value[0] = y0;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Ordinate",
				value) ) return (FALSE);
    value[0] = type1 + offset;
    if ( !ds_put_named_element (coord_desc, coords + pack_size,
				"Overlay Coord Type", value) ) return (FALSE);
    value[0] = x1;
    if ( !ds_put_named_element (coord_desc, coords + pack_size,
				"Overlay Coord Abscissa",
				value) ) return (FALSE);
    value[0] = y1;
    if ( !ds_put_named_element (coord_desc, coords + pack_size,
				"Overlay Coord Ordinate",
				value) ) return (FALSE);
    /*  Now process object entry  */
    if ( !process_app_instruction (olist, object) ) return (0);
    return (object_id);
}   /*  End Function overlay_line  */

/*PUBLIC_FUNCTION*/
unsigned int overlay_lines (KOverlayList olist, unsigned int num_coords,
			    unsigned int *types, double *x_arr, double *y_arr,
			    char *colourname)
/*  [SUMMARY] Add many lines to an overlay list.
    [PURPOSE] This routine will add a number of connected lines to an overlay
    object list. These lines will form a single object. Using this routine is
    far more efficient than calling [<overlay_line>] repeatedly.
    <olist> The overlay list.
    <num_coords> The number of co-ordinates. The number of lines is one less
    than this value.
    <types> An array of co-ordinate types. If this is NULL, all co-ordinates
    are assumed to be world co-ordinates.
    <x_arr> The horizontal co-ordinate values.
    <y_arr> The vertical co-ordinate values.
    <colourname> The colourname.
    [RETURNS] The objectID on success, else 0.
*/
{
    unsigned int count;
    unsigned int pack_size;
    unsigned int object_id;
    double offset = 0.01;
    char *coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    static char function_name[] = "overlay_lines";

    VERIFY_OVERLAYLIST (olist);
    if (x_arr == NULL)
    {
	(void) fprintf (stderr, "NULL x_arr passed\n");
	a_prog_bug (function_name);
    }
    if (y_arr == NULL)
    {
	(void) fprintf (stderr, "NULL y_arr passed\n");
	a_prog_bug (function_name);
    }
    if ( ( object = create_generic (olist, OBJECT_LINES, colourname,
				    num_coords, &coord_desc, &coords,
				    &object_id) )
	== NULL )
    {
	m_error_notify (function_name, "object");
	return (FALSE);
    }
    value[1] = 0.0;
    pack_size = ds_get_packet_size (coord_desc);
    for (count = 0; count < num_coords; ++count, coords += pack_size)
    {
	if (types == NULL)
	{
	    value[0] = OVERLAY_COORD_WORLD;
	}
	else
	{
	    value[0] = types[count] + offset;
	}
	if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Type",
				    value) ) return (FALSE);
	value[0] = x_arr[count];
	if ( !ds_put_named_element (coord_desc, coords,
				    "Overlay Coord Abscissa",
				    value) ) return (FALSE);
	value[0] = y_arr[count];
	if ( !ds_put_named_element (coord_desc, coords,
				    "Overlay Coord Ordinate",
				    value) ) return (FALSE);
    }
    /*  Now process object entry  */
    if ( !process_app_instruction (olist, object) ) return (0);
    return (object_id);
}   /*  End Function overlay_lines  */

/*PUBLIC_FUNCTION*/
unsigned int overlay_text (KOverlayList olist, char *string, unsigned int type,
			   double x, double y, char *colourname,
			   char *fontname, flag clear_under)
/*  [SUMMARY] Add a text string to an overlay object list.
    <olist> The overlay list object.
    <string> The text string.
    <type> The type of the co-ordinate.
    <x> The horizontal position of the co-ordinate.
    <y> The vertical position of the co-ordinate.
    <colourname> The colourname.
    <fontname> The font name.
    <clear_under> If TRUE, then both the foreground and background of the
    characters will be drawn.
    [RETURNS] The objectID on success, else 0.
*/
{
    double offset = 0.01;
    unsigned int object_id;
    char *coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    extern packet_desc *object_desc;
    static char function_name[] = "overlay_text";

    VERIFY_OVERLAYLIST (olist);
    if ( ( object = create_generic (olist, OBJECT_TEXT, colourname,
				    1, &coord_desc, &coords, &object_id) )
	== NULL )
    {
	m_error_notify (function_name, "object");
	return (FALSE);
    }
    value[1] = 0.0;
    value[0] = type + offset;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Type",
				value) ) return (FALSE);
    value[0] = x;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Abscissa",
				value) ) return (FALSE);
    value[0] = y;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Ordinate",
				value) ) return (FALSE);
    if ( !ds_put_unique_named_string (object_desc, &object->data,
				      "Overlay Text String", string,
				      TRUE) ) return (FALSE);
    if ( !ds_put_unique_named_string (object_desc, &object->data,
				      "Overlay Text Fontname", fontname,
				      TRUE) ) return (FALSE);
    value[0] = clear_under;
    if ( !ds_put_named_element (object_desc, object->data,
				"Overlay GP UInteger", value) ) return (FALSE);
    /*  Now process object entry  */
    if ( !process_app_instruction (olist, object) ) return (0);
    return (object_id);
}   /*  End Function overlay_text  */

/*PUBLIC_FUNCTION*/
unsigned int overlay_ellipse (KOverlayList olist,
			      unsigned int ctype,double cx, double cy,
			      unsigned int rtype, double rx, double ry,
			      char *colourname, flag filled)
/*  [SUMMARY] Add an ellipse to an overlay list. See also [<overlay_eillipses>]
    <olist> The overlay list object.
    <ctype> The type of the centre co-ordinate.
    <cx> The horizontal position of the centre co-ordinate.
    <cy> The vertical position of the centre co-ordinate.
    <rtype> The type of the radius co-ordinate.
    <rx> The horizontal radius.
    <ry> The vertical radius.
    <colourname> The colourname.
    <filled> If TRUE the ellipse will be filled.
    [RETURNS] The objectID on success, else 0.
*/
{
    unsigned int pack_size;
    unsigned int code;
    unsigned int object_id;
    double offset = 0.01;
    char *coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    static char function_name[] = "overlay_ellipse";

    VERIFY_OVERLAYLIST (olist);
    code = filled ? OBJECT_FELLIPSE : OBJECT_ELLIPSE;
    if ( ( object = create_generic (olist, code, colourname,
				    2, &coord_desc, &coords, &object_id) )
	== NULL )
    {
	m_error_notify (function_name, "object");
	return (FALSE);
    }
    value[1] = 0.0;
    pack_size = ds_get_packet_size (coord_desc);
    value[0] = ctype + offset;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Type",
				value) ) return (FALSE);
    value[0] = cx;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Abscissa",
				value) ) return (FALSE);
    value[0] = cy;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Ordinate",
				value) ) return (FALSE);
    value[0] = rtype + offset;
    if ( !ds_put_named_element (coord_desc, coords + pack_size,
				"Overlay Coord Type", value) ) return (FALSE);
    value[0] = rx;
    if ( !ds_put_named_element (coord_desc, coords + pack_size,
				"Overlay Coord Abscissa",
				value) ) return (FALSE);
    value[0] = ry;
    if ( !ds_put_named_element (coord_desc, coords + pack_size,
				"Overlay Coord Ordinate",
				value) ) return (FALSE);
    /*  Now process object entry  */
    if ( !process_app_instruction (olist, object) ) return (0);
    return (object_id);
}   /*  End Function overlay_ellipse  */

/*PUBLIC_FUNCTION*/
unsigned int overlay_filled_polygon (KOverlayList olist,
				     unsigned int num_coords,
				     unsigned int *types,
				     double *x_arr, double *y_arr,
				     char *colourname)
/*  [SUMMARY] Add a filled polygon to an overlay object list.
    <olist> The overlay list object.
    <num_coords> The number of co-ordinates (vertices).
    <types> The array of co-ordinate type values. If this is NULL, all
    co-ordinates are assumed to be world co-ordinates.
    <x_arr> The array of horizontal co-ordinate values.
    <y_arr> The array of vertical co-ordinate values.
    <colourname> The colour name.
    [RETURNS] The objectID on success, else 0.
*/
{
    unsigned int count;
    unsigned int pack_size;
    unsigned int object_id;
    double offset = 0.01;
    char *coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    static char function_name[] = "overlay_filled_polygon";

    VERIFY_OVERLAYLIST (olist);
    if ( ( object = create_generic (olist, OBJECT_FPOLY, colourname,
				    num_coords, &coord_desc, &coords,
				    &object_id) )
	== NULL )
    {
	m_error_notify (function_name, "object");
	return (FALSE);
    }
    value[1] = 0.0;
    pack_size = ds_get_packet_size (coord_desc);
    for (count = 0; count < num_coords; ++count, coords += pack_size)
    {
	if (types == NULL)
	{
	    value[0] = OVERLAY_COORD_WORLD;
	}
	else
	{
	    value[0] = types[count] + offset;
	}
	if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Type",
				    value) ) return (FALSE);
	value[0] = x_arr[count];
	if ( !ds_put_named_element (coord_desc, coords,
				    "Overlay Coord Abscissa",
				    value) ) return (FALSE);
	value[0] = y_arr[count];
	if ( !ds_put_named_element (coord_desc, coords,
				    "Overlay Coord Ordinate",
				    value) ) return (FALSE);
    }
    /*  Now process object entry  */
    if ( !process_app_instruction (olist, object) ) return (0);
    return (object_id);
}   /*  End Function overlay_filled_polygon  */

/*PUBLIC_FUNCTION*/
unsigned int overlay_vector (KOverlayList olist,
			     unsigned int stype, double sx, double sy,
			     unsigned int dtype, double dx, double dy,
			     char *colourname)
/*  [SUMMARY] Add a vector to an overlay list.
    [PURPOSE] This routine will add a vector (directed line) to an overlay
    object list. See also [<overlay_vectors>].
    <olist> The overlay list object.
    <stype> The type of the start co-ordinate.
    <sx> The horizontal position of the start co-ordinate.
    <sy> The vertical position of the start co-ordinate.
    <dtype> The type of the vector direction.
    <dx> The horizontal vector direction.
    <dy> The vertical vector direction.
    <colourname> The colour name.
    [RETURNS] The objectID on success, else 0.
*/
{
    unsigned int pack_size;
    unsigned int object_id;
    double offset = 0.01;
    char *coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    static char function_name[] = "overlay_vector";

    VERIFY_OVERLAYLIST (olist);
    if ( ( object = create_generic (olist, OBJECT_VECTOR, colourname,
				    2, &coord_desc, &coords, &object_id) )
	== NULL )
    {
	m_error_notify (function_name, "object");
	return (FALSE);
    }
    value[1] = 0.0;
    pack_size = ds_get_packet_size (coord_desc);
    value[0] = stype + offset;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Type",
				value) ) return (FALSE);
    value[0] = sx;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Abscissa",
				value) ) return (FALSE);
    value[0] = sy;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Ordinate",
				value) ) return (FALSE);
    value[0] = dtype + offset;
    if ( !ds_put_named_element (coord_desc, coords + pack_size,
				"Overlay Coord Type", value) ) return (FALSE);
    value[0] = dx;
    if ( !ds_put_named_element (coord_desc, coords + pack_size,
				"Overlay Coord Abscissa",
				value) ) return (FALSE);
    value[0] = dy;
    if ( !ds_put_named_element (coord_desc, coords + pack_size,
				"Overlay Coord Ordinate",
				value) ) return (FALSE);
    /*  Now process object entry  */
    if ( !process_app_instruction (olist, object) ) return (0);
    return (object_id);
}   /*  End Function overlay_vector  */

/*PUBLIC_FUNCTION*/
unsigned int overlay_ellipses (KOverlayList olist, unsigned int num_ellipses,
			       unsigned int *ctypes, double *cx, double *cy,
			       unsigned int *rtypes, double *rx, double *ry,
			       char *colourname, flag filled)
/*  [SUMMARY] Add many ellipses to an overlay list.
    [PURPOSE] This routine will add a number of ellipses to an overlay object
    list. These ellipses will form a single object. Using this routine is far
    more efficient than calling [<overlay_ellipse>] repeatedly.
    <olist> The overlay list object.
    <num_ellipses> The number of ellipses.
    <ctypes> The types of the centre co-ordinates. If this is NULL, all
    co-ordinates are assumed to be world co-ordinates.
    <cx> The horizontal positions of the centre co-ordinates.
    <cy> The vertical positions of the centre co-ordinates.
    <rtypes> The types of the radii co-ordinates. If this is NULL, all
    co-ordinates are assumed to be world co-ordinates.
    <rx> The horizontal radii.
    <ry> The vertical radii.
    <colourname> The colour name.
    <filled> If TRUE the ellipses will be filled.
    [RETURNS] The objectID on success, else 0.
*/
{
    unsigned int count;
    unsigned int code;
    unsigned int pack_size;
    unsigned int object_id;
    double offset = 0.01;
    char *coords;
    char *centre_coords;
    char *radii_coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    static char function_name[] = "overlay_ellipses";

    VERIFY_OVERLAYLIST (olist);
    code = filled ? OBJECT_FELLIPSES : OBJECT_ELLIPSES;
    if ( ( object = create_generic (olist, code, colourname,
				    num_ellipses * 2, &coord_desc, &coords,
				    &object_id) )
	== NULL )
    {
	m_error_notify (function_name, "object");
	return (FALSE);
    }
    value[1] = 0.0;
    pack_size = ds_get_packet_size (coord_desc);
    /*  First a block of centres, then a block of radii  */
    centre_coords = coords;
    radii_coords = coords + num_ellipses * pack_size;
    for (count = 0; count < num_ellipses;
	 ++count, centre_coords += pack_size, radii_coords += pack_size)
    {
	if (ctypes == NULL)
	{
	    value[0] = OVERLAY_COORD_WORLD;
	}
	else
	{
	    value[0] = ctypes[count] + offset;
	}
	if ( !ds_put_named_element (coord_desc, centre_coords,
				    "Overlay Coord Type",
				    value) ) return (FALSE);
	value[0] = cx[count];
	if ( !ds_put_named_element (coord_desc, centre_coords,
				    "Overlay Coord Abscissa",
				    value) ) return (FALSE);
	value[0] = cy[count];
	if ( !ds_put_named_element (coord_desc, centre_coords,
				    "Overlay Coord Ordinate",
				    value) ) return (FALSE);
	if (rtypes == NULL)
	{
	    value[0] = OVERLAY_COORD_WORLD;
	}
	else
	{
	    value[0] = rtypes[count] + offset;
	}
	if ( !ds_put_named_element (coord_desc, radii_coords,
				    "Overlay Coord Type",
				    value) ) return (FALSE);
	value[0] = rx[count];
	if ( !ds_put_named_element (coord_desc, radii_coords,
				    "Overlay Coord Abscissa",
				    value) ) return (FALSE);
	value[0] = ry[count];
	if ( !ds_put_named_element (coord_desc, radii_coords,
				    "Overlay Coord Ordinate",
				    value) ) return (FALSE);
    }
    /*  Now process object entry  */
    if ( !process_app_instruction (olist, object) ) return (0);
    return (object_id);
}   /*  End Function overlay_ellipses  */

/*PUBLIC_FUNCTION*/
unsigned int overlay_segments (KOverlayList olist, unsigned int num_segments,
			       unsigned int *types0, double *x0, double *y0,
			       unsigned int *types1, double *x1, double *y1,
			       char *colourname)
/*  [SUMMARY] Add many segments to an overlay list.
    [PURPOSE] This routine will add a number of disjoint line segments to an
    overlay object list. These segments will form a single object. Using this
    routine is far more efficient than calling [<overlay_line>] repeatedly.
    <olist> The overlay list object.
    <num_segments> The number of segments.
    <types0> The types of the start co-ordinates. If this is NULL, all
    co-ordinates are assumed to be world co-ordinates.
    <x0> The horizontal positions of the start co-ordinates.
    <y0> The vertical positions of the start co-ordinates.
    <types1> The types of the stop co-ordinates. If this is NULL, all
    co-ordinates are assumed to be world co-ordinates.
    <x1> The horizontal positions of the stop co-ordinates.
    <y1> The vertical positions of the stop co-ordinates.
    <colourname> The colour name.
    [RETURNS] The objectID on success, else 0.
*/
{
    unsigned int count;
    unsigned int pack_size;
    unsigned int object_id;
    double offset = 0.01;
    char *coords;
    char *start_coords;
    char *stop_coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    static char function_name[] = "overlay_segments";

    VERIFY_OVERLAYLIST (olist);
    if ( ( object = create_generic (olist, OBJECT_SEGMENTS, colourname,
				    num_segments * 2, &coord_desc, &coords,
				    &object_id) )
	== NULL )
    {
	m_error_notify (function_name, "object");
	return (FALSE);
    }
    value[1] = 0.0;
    pack_size = ds_get_packet_size (coord_desc);
    /*  First a block of start co-ordinates, then a block of stop co-ords.  */
    start_coords = coords;
    stop_coords = coords + num_segments * pack_size;
    for (count = 0; count < num_segments;
	 ++count, start_coords += pack_size, stop_coords += pack_size)
    {
	if (types0 == NULL)
	{
	    value[0] = OVERLAY_COORD_WORLD;
	}
	else
	{
	    value[0] = types0[count] + offset;
	}
	if ( !ds_put_named_element (coord_desc, start_coords,
				    "Overlay Coord Type", value) )
	{
	    return (FALSE);
	}
	value[0] = x0[count];
	if ( !ds_put_named_element (coord_desc, start_coords,
				    "Overlay Coord Abscissa",
				    value) ) return (FALSE);
	value[0] = y0[count];
	if ( !ds_put_named_element (coord_desc, start_coords,
				    "Overlay Coord Ordinate",
				    value) ) return (FALSE);
	if (types1 == NULL)
	{
	    value[0] = OVERLAY_COORD_WORLD;
	}
	else
	{
	    value[0] = types1[count] + offset;
	}
	if ( !ds_put_named_element (coord_desc, stop_coords,
				    "Overlay Coord Type",
				    value) ) return (FALSE);
	value[0] = x1[count];
	if ( !ds_put_named_element (coord_desc, stop_coords,
				    "Overlay Coord Abscissa",
				    value) ) return (FALSE);
	value[0] = y1[count];
	if ( !ds_put_named_element (coord_desc, stop_coords,
				    "Overlay Coord Ordinate",
				    value) ) return (FALSE);
    }
    /*  Now process object entry  */
    if ( !process_app_instruction (olist, object) ) return (0);
    return (object_id);
}   /*  End Function overlay_segments  */

/*PUBLIC_FUNCTION*/
unsigned int overlay_vectors (KOverlayList olist, unsigned int num_vectors,
			      unsigned int *stypes, double *sx, double *sy,
			      unsigned int *dtypes, double *dx, double *dy,
			      char *colourname)
/*  [SUMMARY] Add many vectors to an overlay list.
    [PURPOSE] This routine will add a number of vectors (directed lines) to an
    overlay object list. These vectors will form a single object. Using this
    routine is far more efficient than calling [<overlay_vector>] repeatedly.
    <olist> The overlay list object.
    <num_vectors> The number of vectors.
    <stypes> The types of the start co-ordinates. If this is NULL, all
    co-ordinates are assumed to be world co-ordinates.
    <sx> The horizontal positions of the start co-ordinates.
    <sy> The vertical positions of the start co-ordinates.
    <dtypes> The types of the vector directions. If this is NULL, all
    directions are assumed to be in world co-ordinates.
    <dx> The horizontal vector directions.
    <dy> The vertical vector directions.
    <colourname> The colour name.
    [RETURNS] The objectID on success, else 0.
*/
{
    unsigned int count;
    unsigned int pack_size;
    unsigned int object_id;
    double offset = 0.01;
    char *coords;
    char *start_coords;
    char *direction_coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    static char function_name[] = "overlay_vectors";

    VERIFY_OVERLAYLIST (olist);
    if ( ( object = create_generic (olist, OBJECT_VECTORS, colourname,
				    num_vectors * 2, &coord_desc, &coords,
				    &object_id) )
	== NULL )
    {
	m_error_notify (function_name, "object");
	return (FALSE);
    }
    value[1] = 0.0;
    pack_size = ds_get_packet_size (coord_desc);
    /*  First a block of start co-ordinates, then a block of directions  */
    start_coords = coords;
    direction_coords = coords + num_vectors * pack_size;
    for (count = 0; count < num_vectors;
	 ++count, start_coords += pack_size, direction_coords += pack_size)
    {
	if (stypes == NULL)
	{
	    value[0] = OVERLAY_COORD_WORLD;
	}
	else
	{
	    value[0] = stypes[count] + offset;
	}
	if ( !ds_put_named_element (coord_desc, start_coords,
				    "Overlay Coord Type",
				    value) ) return (FALSE);
	value[0] = sx[count];
	if ( !ds_put_named_element (coord_desc, start_coords,
				    "Overlay Coord Abscissa",
				    value) ) return (FALSE);
	value[0] = sy[count];
	if ( !ds_put_named_element (coord_desc, start_coords,
				    "Overlay Coord Ordinate",
				    value) ) return (FALSE);
	if (dtypes == NULL)
	{
	    value[0] = OVERLAY_COORD_WORLD;
	}
	else
	{
	    value[0] = dtypes[count] + offset;
	}
	if ( !ds_put_named_element (coord_desc, direction_coords,
				    "Overlay Coord Type",
				    value) ) return (FALSE);
	value[0] = dx[count];
	if ( !ds_put_named_element (coord_desc, direction_coords,
				    "Overlay Coord Abscissa",
				    value) ) return (FALSE);
	value[0] = dy[count];
	if ( !ds_put_named_element (coord_desc, direction_coords,
				    "Overlay Coord Ordinate",
				    value) ) return (FALSE);
    }
    /*  Now process object entry  */
    if ( !process_app_instruction (olist, object) ) return (0);
    return (object_id);
}   /*  End Function overlay_vectors  */


/*  Overlay instruction routines follow  */

/*PUBLIC_FUNCTION*/
flag overlay_remove_objects (KOverlayList olist, unsigned int num_objects)
/*  [SUMMARY] Remove objects from the end of an overlay object list.
    <olist> The overlay list object.
    <num_objects> The number of objects to remove. If this is 0, then the list
    is emptied.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char *coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    extern packet_desc *object_desc;
    static char function_name[] = "overlay_remove_objects";

    VERIFY_OVERLAYLIST (olist);
    if ( ( object = create_generic (olist, OBJECT_REMOVE_OBJECTS, NULL,
				    0, &coord_desc, &coords, NULL) ) == NULL )
    {
	m_error_notify (function_name, "instruction");
	return (FALSE);
    }
    value[0] = num_objects;
    value[1] = 0.0;
    if ( !ds_put_named_element (object_desc, object->data,
				"Overlay GP UInteger", value) ) return (FALSE);
    return ( process_app_instruction (olist, object) );
}   /*  End Function overlay_remove_objects  */

/*PUBLIC_FUNCTION*/
flag overlay_remove_object (KOverlayList olist, unsigned int id_in_list,
			    unsigned int list_id)
/*  [SUMMARY] Remove one object from an overlay object list.
    <olist> The overlay list object.
    <id_in_list> The object ID. This ID refers to an object created by a
    particular list master or slave.
    <list_id> The ID of the list which created the object. If this is 0, the
    list given by <<olist>> is assumed.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char *coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    extern packet_desc *object_desc;
    static char function_name[] = "overlay_remove_object";

    VERIFY_OVERLAYLIST (olist);
    if ( ( object = create_generic (olist, OBJECT_REMOVE_OBJECT, NULL,
				    0, &coord_desc, &coords, NULL) ) == NULL )
    {
	m_error_notify (function_name, "instruction");
	return (FALSE);
    }
    value[0] = id_in_list;
    value[1] = 0.0;
    if ( !ds_put_named_element (object_desc, object->data,
				"Overlay ObjectID", value) ) return (FALSE);
    if (list_id == 0) list_id = olist->my_id;
    value[0] = list_id;
    value[1] = 0.0;
    if ( !ds_put_named_element (object_desc, object->data,
				"Overlay ListID", value) ) return (FALSE);
    return ( process_app_instruction (olist, object) );
}   /*  End Function overlay_remove_object  */

/*PUBLIC_FUNCTION*/
flag overlay_move_object (KOverlayList olist, unsigned int id_in_list,
			  unsigned int list_id, double dx, double dy)
/*  [SUMMARY] Move a object in an overlay object list.
    <olist> The overlay list object.
    <id_in_list> The object ID. This ID refers to an object created by a
    particular list master or slave.
    <list_id> The ID of the list which created the object. If this is 0, the
    list given by <<olist>> is assumed.
    <dx> The horizontal distance to move.
    <dy> The vertical distance to move.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    char *coords;
    packet_desc *coord_desc;
    list_entry *object;
    double value[2];
    extern packet_desc *object_desc;
    static char function_name[] = "overlay_move_object";

    VERIFY_OVERLAYLIST (olist);
    if ( ( object = create_generic (olist, OBJECT_MOVE_OBJECT, NULL,
				    1, &coord_desc, &coords, NULL) ) == NULL )
    {
	m_error_notify (function_name, "instruction");
	return (FALSE);
    }
    value[0] = id_in_list;
    value[1] = 0.0;
    if ( !ds_put_named_element (object_desc, object->data,
				"Overlay ObjectID", value) ) return (FALSE);
    if (list_id == 0) list_id = olist->my_id;
    value[0] = list_id;
    value[1] = 0.0;
    if ( !ds_put_named_element (object_desc, object->data,
				"Overlay ListID", value) ) return (FALSE);
    value[0] = dx;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Abscissa",
				value) ) return (FALSE);
    value[0] = dy;
    if ( !ds_put_named_element (coord_desc, coords, "Overlay Coord Ordinate",
				value) ) return (FALSE);
    return ( process_app_instruction (olist, object) );
}   /*  End Function overlay_move_object  */


/*  Private functions follow  */

static void initialise_overlay_package ()
/*  This routine will create the overlay object descriptor.
    [RETURNS] Nothing.
*/
{
    Channel channel;
    extern packet_desc *object_desc;
    extern char *str_object_desc[];
    static char function_name[] = "initialise_overlay_package";

    if (object_desc != NULL)
    {
	/*  Package is initialised  */
	return;
    }
    /*  Create descriptor  */
    if ( ( channel = ch_open_and_fill_memory (str_object_desc) ) == NULL )
    {
	m_abort (function_name, "memory channel");
    }
    if ( ( object_desc = dsra_packet_desc (channel) ) == NULL )
    {
	(void) ch_close (channel);
	m_abort (function_name, "overlay object list descriptor");
    }
    (void) ch_close (channel);
    /*  Register protocols  */
    conn_register_server_protocol ("2D_overlay", PROTOCOL_VERSION, 0,
				   ( flag (*) () ) register_new_overlay_slave,
				   ( flag (*) () ) read_instruction_from_slave,
				   ( void (*) () ) register_slave_loss);
    conn_register_client_protocol ("2D_overlay", PROTOCOL_VERSION, 1,
				   (flag(*)()) verify_overlay_slave_connection,
				   ( flag (*) () ) register_overlay_slave_connection,
				   ( flag (*) ()) read_instruction_from_master,
				   ( void (*) () ) register_master_loss);
}   /*  End Function initialise_overlay_package  */

static void worldcanvas_refresh_func (KWorldCanvas canvas, int width,
				      int height,
				      struct win_scale_type *win_scale,
				      Kcolourmap cmap, flag cmap_resize,
				      void **info)
/*  This routine registers a refresh event for a world canvas.
    The canvas is given by  canvas  .
    The width of the canvas in pixels is given by  width  .
    The height of the canvas in pixels is given by  height  .
    The window scaling information is pointed to by  win_scale  .
    The colourmap associated with the canvas is given by  cmap  .
    If the refresh function was called as a result of a colourmap resize the
    value of  cmap_resize  will be TRUE.
    The arbitrary canvas information pointer is pointed to by  info  .
    [RETURNS] Nothing.
*/
{
    KOverlayList olist;
    flag no_association;
    struct refresh_canvas_type *cnv;
    static char function_name[] = "__overlay_worldcanvas_refresh_func";

    olist = (KOverlayList) *info;
    VERIFY_OVERLAYLIST (olist);
    /*  Search through for association  */
    for (cnv = olist->refresh_canvases, no_association = TRUE;
	 (cnv != NULL) && no_association; cnv = cnv->next)
    {
	if ( (canvas == cnv->canvas) && cnv->active ) no_association = FALSE;
    }
    if (no_association) return;
    (void) overlay_redraw_on_canvas (olist, canvas);
}   /*  End Function worldcanvas_refresh_func  */


/*  Network functions follow  */


/*  Callbacks for the master (server)  */

static flag register_new_overlay_slave (Connection connection, void **info)
/*  This routine will register the opening of a connection from a 2D_overlay
    slave.
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on successful registration,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will not be called if this routine returns
    FALSE.
*/
{
    Channel channel;
    list_header *list_head;
    extern KOverlayList masterable_list;
    extern packet_desc *object_desc;
    /*static char function_name[] = "register_new_overlay_slave";*/

    channel = conn_get_channel (connection);
    if (masterable_list == NULL)
    {
	(void) fprintf (stderr,
			"Connection attempt by 2D_overlay client but we have no list!\n");
	dsrw_write_flag (channel, FALSE);
	(void) ch_flush (channel);
	return (FALSE);
    }
    if (masterable_list->master != NULL)
    {
	(void) fprintf (stderr, "Default masterable list is a slave!\n");
	dsrw_write_flag (channel, FALSE);
	(void) ch_flush (channel);
	return (FALSE);
    }
    if (masterable_list->next_slave_id == 0)
    {
	/*  Wow! There's been a lot of slave connections!  */
	(void) fprintf (stderr, "Slave ID counter has wrapped around!\n");
	dsrw_write_flag (channel, FALSE);
	(void) ch_flush (channel);
	return (FALSE);
    }
    masterable_list->my_id = 1;
    *info = (void *) masterable_list;
    dsrw_write_flag (channel, TRUE);
    if ( !pio_write32 (channel, masterable_list->next_slave_id) )
    {
	return (FALSE);
    }
    ++masterable_list->next_slave_id;
    /*  Write whatever is in list and flush  */
    list_head = masterable_list->list_head;
    if ( write_entries (channel, object_desc, list_head->first_frag_entry) )
    {
	++masterable_list->slave_count;
	return (TRUE);
    }
    return (FALSE);
}   /*  End Function register_new_overlay_slave  */

static flag read_instruction_from_slave (Connection connection, void **info)
/*  This routine will read in overlay objects from the connection (to a
    slave) given by  connection and will write any appropriate information to
    the pointer pointed to by  info  .
    The routine returns TRUE on successful reading,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will be called if this routine returns FALSE
*/
{
    Channel channel;
    KOverlayList olist;
    list_entry *entry;
    extern packet_desc *object_desc;
    static char function_name[] = "read_instruction_from_slave";

    olist = (KOverlayList) *info;
    VERIFY_OVERLAYLIST (olist);
    if ( ( entry = ds_alloc_list_entry (object_desc, TRUE) ) == NULL )
    {
	m_error_notify (function_name, "overlay object");
	return (FALSE);
    }
    channel = conn_get_channel (connection);
    if ( !dsrw_read_packet (channel, object_desc,
			    entry->data) ) return (FALSE);
    return ( process_instruction (olist, entry, connection) );
}   /*  End Function read_instruction_from_slave  */

static void register_slave_loss (Connection connection, void *info)
/*  This routine will register a closure of a connection to an overlay slave.
    When this routine is called, this is the last chance to read any
    buffered data from the channel associated with the connection object.
    The connection will be given by  connection  .
    The connection information pointer will be given by  info  .
    [RETURNS] Nothing.
*/
{
    KOverlayList olist;
    static char function_name[] = "register_slave_loss";

    olist = (KOverlayList) info;
    VERIFY_OVERLAYLIST (olist);
    if (connection == olist->token_conn)
    {
	/*  Take back the token  */
	(void) process_token_receive (olist, connection);
    }
    --olist->slave_count;
    remove_token_request (olist, connection);
}   /*  End Function register_slave_loss  */


/*  Callbacks for the slave (client)  */

static flag verify_overlay_slave_connection (void **info)
/*  This routine will validate whether it is appropriate to open a connection
    to an overlay master.
    The routine will write any appropriate information to the pointer
    pointed to by  info  .The pointer value written here will be passed
    to the other routines.
    The routine returns TRUE if the connection should be attempted,
    else it returns FALSE (indicating the connection should be aborted).
    NOTE: Even if this routine is called and returns TRUE, there is no
    guarantee that the connection will be subsequently opened.
*/
{
    list_header *list_head;
    extern KOverlayList slaveable_list;
    extern packet_desc *object_desc;
    /*static char function_name[] = "verify_overlay_slave_connection";*/

    if (slaveable_list == NULL)
    {
	(void) fprintf (stderr,
			"Connection attempt to 2D_overlay server but we have no list!\n");
	return (FALSE);
    }
    if (slaveable_list->master != NULL)
    {
	(void) fprintf (stderr,
			"Connection attempt to 2D_overlay server but already a slave!\n");
	return (FALSE);
    }
    list_head = slaveable_list->list_head;
    if (list_head->length > 0)
    {
	(void) fprintf (stderr,
			"Overlay list must be empty before becomming a slave\n");
	return (FALSE);
    }
    *info = (void *) slaveable_list;
    return (TRUE);
}   /*  End Function verify_overlay_slave_connection  */

static flag register_overlay_slave_connection (Connection connection,
					       void **info)
/*  This routine will register the opening of a connection to an overlay server
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on successful registration,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will not be called if this routine returns
    FALSE.
*/
{
    KOverlayList olist;
    Channel channel;
    flag success;
    unsigned long my_id;
    static char function_name[] = "register_overlay_slave_connection";

    olist = (KOverlayList) *info;
    VERIFY_OVERLAYLIST (olist);
    if (olist->master != NULL)
    {
	(void) fprintf (stderr,
			"Connection attempt to 2D_overlay server but suddenly a slave!\n");
	a_prog_bug (function_name);
    }
    channel = conn_get_channel (connection);
    if ( !dsrw_read_flag (channel, &success) ) return (FALSE);
    if (!success) return (FALSE);
    if ( !pio_read32 (channel, &my_id) ) return (FALSE);
    olist->my_id = my_id;
    olist->master = connection;
    olist->have_token = FALSE;
    olist->requested_token = FALSE;
    return (TRUE);
}   /*  End Function register_overlay_slave_connection  */

static flag read_instruction_from_master (Connection connection, void **info)
/*  This routine will read in data from the connection given by  connection
    and will write any appropriate information to the pointer pointed to by
    info  .
    The routine returns TRUE on successful reading,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will be called if this routine returns FALSE
*/
{
    Channel channel;
    KOverlayList olist;
    list_entry *entry;
    extern packet_desc *object_desc;
    static char function_name[] = "read_instruction_from_master";

    olist = (KOverlayList) *info;
    VERIFY_OVERLAYLIST (olist);
    if ( ( entry = ds_alloc_list_entry (object_desc, TRUE) ) == NULL )
    {
	m_error_notify (function_name, "overlay object");
	return (FALSE);
    }
    if (connection != olist->master)
    {
	(void) fprintf (stderr, "Connection missmatch\n");
	a_prog_bug (function_name);
    }
    channel = conn_get_channel (connection);
    if ( !dsrw_read_packet (channel, object_desc,
			    entry->data) ) return (FALSE);
    return ( process_instruction (olist, entry, connection) );
}   /*  End Function read_instruction_from_master  */

static void register_master_loss (Connection connection, void *info)
/*  This routine will register a closure of a connection to an overlay server.
    When this routine is called, this is the last chance to read any
    buffered data from the channel associated with the connection object.
    The connection will be given by  connection  .
    The connection information pointer will be given by  info  .
    [RETURNS] Nothing.
*/
{
    KOverlayList olist;
    static char function_name[] = "register_master_loss";

    olist = (KOverlayList) info;
    VERIFY_OVERLAYLIST (olist);
    if (!olist->have_token)
    {
	(void) process_token_receive (olist, olist->master);
    }
    olist->master = NULL;
}   /*  End Function register_master_loss  */


/*  Network support routines follow  */

static flag transmit_to_slaves (KOverlayList olist, list_entry *object,
				Connection except_conn)
/*  This routine will transmit an overlay object to all slaves of a managed
    overlay object list, except a specified slave.
    <olist> The overlay list object.
    The overlay object must be pointed to by  object  .
    The excepted slave connection must be given by  except_conn  .This may be
    NULL.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    Connection conn;
    unsigned int num_connections;
    unsigned int conn_count;
    extern packet_desc *object_desc;
    static char function_name[] = "transmit_to_slaves";

    /*  Write to any possible slaves  */
    num_connections = conn_get_num_serv_connections ("2D_overlay");
    for (conn_count = 0; conn_count < num_connections; ++conn_count)
    {
	if ( ( conn = conn_get_serv_connection ("2D_overlay", conn_count) )
	    == NULL )
	{
	    (void) fprintf (stderr, "2D_overlay connection: %u not found\n",
			    conn_count);
	    a_prog_bug (function_name);
	}
	if (conn == except_conn) continue;
	if (conn_get_connection_info (conn) != olist) continue;
	channel = conn_get_channel (conn);
	dsrw_write_packet (channel, object_desc, object->data);
	if ( !ch_flush (channel) ) return (FALSE);
    }
    return (TRUE);
}   /*  End Function transmit_to_slaves  */

static flag write_entries (Channel channel, packet_desc *list_desc,
			   list_entry *first_entry)
/*  This routine will write fragmented list entries in a linked list to a
    channel.
    The channel must be given by  channel  .
    The packet descriptor for the linked list must be pointed to by  list_desc
    The first entry must be pointed to by  first_entry  .
    [RETURNS] TRUE on success, else FALSE.
*/
{
    list_entry *entry;

    for (entry = first_entry; entry != NULL; entry = entry->next)
    {
	dsrw_write_packet (channel, list_desc, entry->data);
    }
    return ( ch_flush (channel) );
}   /*  End Function write_entries  */


/*  Processing routines follow  */

static flag process_instruction (KOverlayList olist, list_entry *instruction,
				 Connection conn)
/*  This routine will process a single overlay instruction (ie. a special
    instruction or an object) for a managed overlay object list.
    <olist> The overlay list object.
    The instruction must be given by  instruction  .
    The connection from where the instruction originated must be given by
    conn  .This may be NULL.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int instruction_code;
    unsigned int num_objects;
    unsigned int list_id, object_id;
    double dx, dy;
    double value[2];
    char *ptr, *coords;
    struct refresh_canvas_type *cnv;
    list_entry *curr_entry;
    list_entry *prev_entry = NULL;  /*  Initialised to keep compiler happy  */
    list_header *coord_list;
    packet_desc *coord_desc;
    extern packet_desc *object_desc;
    static char function_name[] = "process_instruction";

    VERIFY_OVERLAYLIST (olist);
    /*  Get object code  */
    if ( !ds_get_unique_named_value (object_desc, instruction->data,
				     "Overlay Object Code",
				     (unsigned int *) NULL, value) )
    {
	(void) fprintf (stderr, "Error getting overlay object code\n");
	return (FALSE);
    }
    instruction_code = (unsigned int) value[0];
    /*  Do it locally  */
    switch (instruction_code)
    {
      case OBJECT_REMOVE_OBJECTS:
	if (olist->master == NULL)
	{
	    if ( !transmit_to_slaves (olist, instruction,
				      conn) ) return (FALSE);
	}
	if (olist->list_head->length < 1) return (TRUE);
	if ( !ds_get_unique_named_value (object_desc, instruction->data,
					 "Overlay GP UInteger",
					 (unsigned int *) NULL, value) )
	{
	    (void) fprintf (stderr, "Error getting overlay object UINT\n");
	    ds_dealloc_data (object_desc, instruction->data);
	    m_free ( (char *) instruction );
	    return (FALSE);
	}
	/*  Deallocate instruction: can't use it later!  */
	ds_dealloc_data (object_desc, instruction->data);
	m_free ( (char *) instruction );
	num_objects = value[0];
	if ( (num_objects > olist->list_head->length) ||
	    (num_objects == 0) ) num_objects = olist->list_head->length;
	for (curr_entry = olist->list_head->last_frag_entry;
	     num_objects > 0; curr_entry = prev_entry, --num_objects)
	{
	    if (curr_entry == NULL)
	    {
		(void) fprintf (stderr,
				"Overran list! Possible protocol error\n");
		a_prog_bug (function_name);
	    }
	    ds_dealloc_data (object_desc, curr_entry->data);
	    prev_entry = curr_entry->prev;
	    m_free ( (char *) curr_entry );
	    --olist->list_head->length;
	}
	olist->list_head->last_frag_entry = prev_entry;
	if (prev_entry != NULL) prev_entry->next = NULL;
	if (olist->list_head->length < 1)
	{
	    olist->list_head->first_frag_entry = NULL;
	}
	/*  Search through for canvas associations  */
	for (cnv = olist->refresh_canvases; cnv != NULL; cnv = cnv->next)
	{
	    if (!cnv->active) continue;
	    /*  Refresh canvas  */
	    if ( !canvas_resize (cnv->canvas,
				 (struct win_scale_type *) NULL,
				 FALSE) ) return (FALSE);
	}
	return (TRUE);
/*
        break;
*/
      case OBJECT_REQUEST_TOKEN:
	/*  Deallocate instruction: can't use it later!  */
	ds_dealloc_data (object_desc, instruction->data);
	m_free ( (char *) instruction );
	return ( process_conn_token_request (olist, conn) );
/*
	break;
*/
      case OBJECT_GRANT_TOKEN:
	/*  Deallocate instruction: can't use it later!  */
	ds_dealloc_data (object_desc, instruction->data);
	m_free ( (char *) instruction );
	return ( process_token_receive (olist, conn) );
/*
	break;
*/
      case OBJECT_REMOVE_OBJECT:
	if (olist->master == NULL)
	{
	    if ( !transmit_to_slaves (olist, instruction,
				      conn) ) return (FALSE);
	}
	if (olist->list_head->length < 1) return (FALSE);
	if ( !ds_get_unique_named_value (object_desc, instruction->data,
					 "Overlay ListID",
					 (unsigned int *) NULL, value) )
	{
	    (void) fprintf (stderr, "Error getting overlay object ListID\n");
	    ds_dealloc_data (object_desc, instruction->data);
	    m_free ( (char *) instruction );
	    return (FALSE);
	}
	list_id = value[0];
	if ( !ds_get_unique_named_value (object_desc, instruction->data,
					 "Overlay ObjectID",
					 (unsigned int *) NULL, value) )
	{
	    (void) fprintf (stderr, "Error getting overlay object ObjectID\n");
	    ds_dealloc_data (object_desc, instruction->data);
	    m_free ( (char *) instruction );
	    return (FALSE);
	}
	object_id = value[0];
	/*  Deallocate instruction: can't use it later!  */
	ds_dealloc_data (object_desc, instruction->data);
	m_free ( (char *) instruction );
	if ( !remove_object (olist, object_id, list_id) ) return (TRUE);
	/*  Search through for canvas associations  */
	for (cnv = olist->refresh_canvases; cnv != NULL; cnv = cnv->next)
	{
	    if (!cnv->active) continue;
	    /*  Refresh canvas  */
	    if ( !canvas_resize (cnv->canvas,
				 (struct win_scale_type *) NULL,
				 FALSE) ) return (FALSE);
	}
	return (TRUE);
/*
	break;
*/
      case OBJECT_MOVE_OBJECT:
	if (olist->master == NULL)
	{
	    if ( !transmit_to_slaves (olist, instruction,
				      conn) ) return (FALSE);
	}
	if (olist->list_head->length < 1) return (FALSE);
	if ( !ds_get_unique_named_value (object_desc, instruction->data,
					 "Overlay ListID",
					 (unsigned int *) NULL, value) )
	{
	    (void) fprintf (stderr, "Error getting overlay object ListID\n");
	    ds_dealloc_data (object_desc, instruction->data);
	    m_free ( (char *) instruction );
	    return (FALSE);
	}
	list_id = value[0];
	if ( !ds_get_unique_named_value (object_desc, instruction->data,
					 "Overlay ObjectID",
					 (unsigned int *) NULL, value) )
	{
	    (void) fprintf (stderr, "Error getting overlay object ObjectID\n");
	    ds_dealloc_data (object_desc, instruction->data);
	    m_free ( (char *) instruction );
	    return (FALSE);
	}
	object_id = value[0];
	ptr = instruction->data + ds_get_element_offset (object_desc,
							   OBJECT_COORD_INDEX);
	coord_list = *(list_header **) ptr;
	if (coord_list->length != 1)
	{
	    ds_dealloc_data (object_desc, instruction->data);
	    m_free ( (char *) instruction );
	    return (FALSE);
	}
	coords = coord_list->contiguous_data;
	coord_desc= (packet_desc *)object_desc->element_desc[OBJECT_COORD_INDEX];
	dx = *(double *) ( coords + ds_get_element_offset (coord_desc,
							   COORD_X_INDEX) );
	dy = *(double *) ( coords + ds_get_element_offset (coord_desc,
							   COORD_Y_INDEX) );
	/*  Deallocate instruction: can't use it later!  */
	ds_dealloc_data (object_desc, instruction->data);
	m_free ( (char *) instruction );
	if ( !move_object (olist, object_id, list_id, dx, dy) ) return (TRUE);
	/*  Search through for canvas associations  */
	for (cnv = olist->refresh_canvases; cnv != NULL; cnv = cnv->next)
	{
	    if (!cnv->active) continue;
	    /*  Refresh canvas  */
	    if ( !canvas_resize (cnv->canvas,
				 (struct win_scale_type *) NULL,
				 FALSE) ) return (FALSE);
	}
	return (TRUE);
/*
	break;
*/
      default:
        break;
    }
    if (olist->master == NULL)
    {
	if ( !transmit_to_slaves (olist, instruction, conn) ) return (FALSE);
    }
    return ( process_local_object (olist, instruction, TRUE) );
}   /*  End Function process_instruction  */

static flag process_local_object (KOverlayList olist, list_entry *object,
				  flag append)
/*  This routine will add a single overlay object to a managed overlay
    object list.
    <olist> The overlay list object.
    The object must be given by  object  .
    If the value of  append  is TRUE, the object is appended to the list.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int num_restr;
    char *xlabel;
    char *ylabel;
    char **restr_names;
    double *restr_values;
    struct refresh_canvas_type *cnv;
    static char function_name[] = "process_local_object";

    VERIFY_OVERLAYLIST (olist);
    /*  Search through for canvas associations  */
    for (cnv = olist->refresh_canvases; cnv != NULL; cnv = cnv->next)
    {
	if (!cnv->active) continue;
	/*  Get canvas specification information  */
	canvas_get_specification (cnv->canvas, &xlabel, &ylabel,
				  &num_restr, &restr_names, &restr_values);
	/*  Draw it  */
	if ( !draw_object (cnv->canvas, object->data, xlabel, ylabel,
			   num_restr, restr_names,
			   restr_values) ) return (FALSE);
    }
    /*  Add it to the list  */
    if (append) ds_list_append (olist->list_head, object);
    return (TRUE);
}   /*  End Function process_local_object  */


/*  Drawing functions follow  */

static flag draw_object (KWorldCanvas canvas, char *object, char *xlabel,
			 char *ylabel, unsigned int num_restr,
			 char **restr_names, double *restr_values)
/*  This routine will draw an overlay object onto a world canvas.
    <canvas> The world canvas object.
    The object must be pointed to by  object  .
    The horizontal dimension label must be pointed to by  xlabel  .
    The vertical dimension label must be pointed to by  ylabel  .
    The number of restrictions must be given by  num_restr  .
    The array of restriction names must by pointed to by  restr_names  .
    The restriction values must be pointed to by  restr_values  .
    [RETURNS] TRUE on success, else FALSE.
*/
{
    KPixCanvas pixcanvas;
    KPixCanvasFont font;
    flag clear_under;
    int tmp_x, tmp_y;
    int x_offset, y_offset, width, height;
    unsigned int object_code;
    unsigned int coord_pack_size;
    unsigned int restr_pack_size;
    unsigned int num_coords;
    unsigned int count1, count2;
    unsigned long pixel_value;
    double value[2];
    char *ptr;
    char *colourname;
    char *x_label;
    char *y_label;
    char *coords;
    char *types, *x_arr, *y_arr;
    char *restriction_names;
    char *restriction_values;
    char *name;
    char *textstring, *fontname;
    list_header *coord_list;
    list_header *restriction_list;
    packet_desc *coord_desc;
    packet_desc *restriction_desc;
    extern packet_desc *object_desc;
    static unsigned int coord_array_length = 0;
    static int *px = NULL;
    static int *py = NULL;
    static char function_name[] = "__overlay_draw_object";

    ptr = object + ds_get_element_offset (object_desc, OBJECT_CODE_INDEX);
    object_code = *(unsigned int *) ptr;
    ptr = object + ds_get_element_offset (object_desc, OBJECT_COORD_INDEX);
    coord_list = *(list_header **) ptr;
    num_coords = coord_list->length;
    coords = coord_list->contiguous_data;
    coord_desc= (packet_desc *)object_desc->element_desc[OBJECT_COORD_INDEX];
    coord_pack_size = ds_get_packet_size (coord_desc);
    types = coords + ds_get_element_offset (coord_desc, COORD_TYPE_INDEX);
    x_arr = coords + ds_get_element_offset (coord_desc, COORD_X_INDEX);
    y_arr = coords + ds_get_element_offset (coord_desc, COORD_Y_INDEX);
    ptr = object + ds_get_element_offset (object_desc,OBJECT_COLOURNAME_INDEX);
    colourname = *(char **) ptr;
    ptr = object + ds_get_element_offset (object_desc, OBJECT_X_INDEX);
    x_label = *(char **) ptr;
    ptr = object + ds_get_element_offset (object_desc, OBJECT_Y_INDEX);
    y_label = *(char **) ptr;
    ptr = object + ds_get_element_offset(object_desc,OBJECT_RESTRICTION_INDEX);
    restriction_list = *(list_header **) ptr;
    ptr = object_desc->element_desc[OBJECT_RESTRICTION_INDEX];
    restriction_desc = (packet_desc *) ptr;
    ptr = object + ds_get_element_offset(object_desc,OBJECT_TEXT_STRING_INDEX);
    textstring = *(char **) ptr;
    ptr = object + ds_get_element_offset (object_desc,
					  OBJECT_TEXT_FONTNAME_INDEX);
    fontname = *(char **) ptr;
    ptr = object + ds_get_element_offset (object_desc,
					  OBJECT_TEXT_CLEAR_UNDER_INDEX);
    clear_under = *(unsigned int *) ptr;
    /*  Check if labels match  */
    if ( (xlabel != NULL) && (xlabel[0] != '\0') &&
	(x_label != NULL) && (x_label[0] != '\0') )
    {
	if (strcmp (xlabel, x_label) != 0) return (TRUE);
    }
    if ( (ylabel != NULL) && (ylabel[0] != '\0') &&
	(y_label != NULL) && (y_label[0] != '\0') )
    {
	if (strcmp (ylabel, y_label) != 0) return (TRUE);
    }
    /*  Process restrictions  */
    ptr = restriction_list->contiguous_data;
    restriction_names = ptr + ds_get_element_offset (restriction_desc,
						     RESTRICTION_NAME_INDEX);
    restriction_values = ptr + ds_get_element_offset (restriction_desc,
						      RESTRICTION_VALUE_INDEX);
    restr_pack_size = ds_get_packet_size (restriction_desc);
    for (count1 = 0; count1 < restriction_list->length;
	 ++count1, restriction_names += restr_pack_size,
	 restriction_values += restr_pack_size)
    {
	name = *(char **) restriction_names;
	(void) ds_get_element (restriction_values, K_DOUBLE, value,
			       (flag *) NULL);
	/*  Scan through canvas restrictions  */
	for (count2 = 0; count2 < num_restr; ++count2)
	{
	    if (strcmp (name, restr_names[count2]) == 0)
	    {
		/*  Name match! Check that value!! Git!!!
		    Now, back to work...  */
		if (value[0] != restr_values[count2])
		{
		    /*  NOOOO! Failed! I'm devastated!! Sob, snif...  */
		    return (TRUE);
		}
	    }
	}
    }
    if (num_coords > coord_array_length)
    {
	coord_array_length = 0;
	if (px != NULL) m_free ( (char *) px );
	if (py != NULL) m_free ( (char *) py );
	py = NULL;
	if ( ( px = (int *) m_alloc (sizeof *px * num_coords) ) == NULL )
	{
	    m_error_notify (function_name, "x co-ordinate array");
	    return (FALSE);
	}
	if ( ( py = (int *) m_alloc (sizeof *py * num_coords) ) == NULL )
	{
	    m_error_notify (function_name, "y co-ordinate array");
	    return (FALSE);
	}
	coord_array_length = num_coords;
    }
    pixcanvas = canvas_get_pixcanvas (canvas);
    if ( !canvas_get_colour (canvas, colourname, &pixel_value,
			     (unsigned short *) NULL, (unsigned short *) NULL,
			     (unsigned short *) NULL) ) return (FALSE);
    canvas_get_attributes (canvas,
			   CANVAS_ATT_X_OFFSET, &x_offset,
			   CANVAS_ATT_Y_OFFSET, &y_offset,
			   CANVAS_ATT_END);
    kwin_get_size (pixcanvas, &width, &height);
    switch (object_code)
    {
      case OBJECT_LINE:
	if (num_coords != 2)
	{
	    (void) fprintf (stderr,
			    "Co-ordinate list length: %u must be 2 for line\n",
			    num_coords);
	    return (FALSE);
	}
	convert_to_pixcoords (canvas, num_coords, types, x_arr, y_arr,
			      coord_pack_size, px, py);
	kwin_draw_line (pixcanvas, px[0], py[0], px[1], py[1], pixel_value);
	break;
      case OBJECT_LINES:
	convert_to_pixcoords (canvas, num_coords, types, x_arr, y_arr,
			      coord_pack_size, px, py);
	kwin_draw_lines (pixcanvas, px, py, num_coords, pixel_value);
	break;
      case OBJECT_TEXT:
	if (num_coords != 1)
	{
	    (void) fprintf (stderr,
			    "Co-ordinate list length: %u must be 1 for line\n",
			    num_coords);
	    return (FALSE);
	}
	convert_to_pixcoords (canvas, num_coords, types, x_arr, y_arr,
			      coord_pack_size, px, py);
	if ( ( font = kwin_load_font (pixcanvas, fontname) ) == NULL )
	{
	    return (FALSE);
	}
	kwin_set_attributes (pixcanvas,
			     KWIN_ATT_FONT, font,
			     KWIN_ATT_END);
	kwin_draw_string (pixcanvas, px[0], py[0], textstring,
			  pixel_value, clear_under);
	break;
      case OBJECT_ELLIPSE:
      case OBJECT_FELLIPSE:
	if (num_coords != 2)
	{
	    (void) fprintf (stderr,
			    "Co-ordinate list length: %u must be 2 for line\n",
			    num_coords);
	    return (FALSE);
	}
	convert_to_pixcoords (canvas, num_coords, types, x_arr, y_arr,
			      coord_pack_size, px, py);
	switch ( *(unsigned int *) (types + coord_pack_size) )
	{
	  case OVERLAY_COORD_PIXEL:
	  case OVERLAY_COORD_LAST:
	    break;
	  case OVERLAY_COORD_RELATIVE:
	    py[1] = height - 1 - py[1];
	    px[1] -= x_offset;
	    py[1] -= y_offset;
	    break;
	  case OVERLAY_COORD_WORLD:
	    (void) canvas_convert_from_canvas_coord (canvas,
						     0.0, 0.0, &tmp_x, &tmp_y);
	    px[1] -= tmp_x;
	    py[1] = tmp_y - py[1];
	    break;
	}
	if (object_code == OBJECT_ELLIPSE)
	{
	    kwin_draw_ellipse (pixcanvas, px[0], py[0], px[1], py[1],
			       pixel_value);
	}
	else
	{
	    kwin_fill_ellipse (pixcanvas, px[0], py[0], px[1], py[1],
			       pixel_value);
	}
	break;
      case OBJECT_FPOLY:
	convert_to_pixcoords (canvas, num_coords, types, x_arr, y_arr,
			      coord_pack_size, px, py);
	if ( !kwin_fill_polygon (pixcanvas, px, py, num_coords, pixel_value,
				 FALSE) ) return (FALSE);
	break;
      case OBJECT_VECTOR:
	if (num_coords != 2)
	{
	    (void) fprintf (stderr,
			    "Co-ordinate list length: %u must be 2 for line\n",
			    num_coords);
	    return (FALSE);
	}
	convert_to_pixcoords (canvas, num_coords, types, x_arr, y_arr,
			      coord_pack_size, px, py);
	switch ( *(unsigned int *) (types + coord_pack_size) )
	{
	  case OVERLAY_COORD_PIXEL:
	  case OVERLAY_COORD_LAST:
	    break;
	  case OVERLAY_COORD_RELATIVE:
	    py[1] = height - 1 - py[1];
	    px[1] -= x_offset;
	    py[1] -= y_offset;
	    break;
	  case OVERLAY_COORD_WORLD:
	    (void) canvas_convert_from_canvas_coord (canvas,
						     0.0, 0.0, &tmp_x, &tmp_y);
	    px[1] -= tmp_x;
	    py[1] = tmp_y - py[1];
	    break;
	}
	kwin_draw_line (pixcanvas,
			px[0], py[0], px[0] + px[1], py[0] + py[1],
			pixel_value);
	break;
      case OBJECT_ELLIPSES:
      case OBJECT_FELLIPSES:
	if (num_coords % 2 != 0)
	{
	    (void) fprintf (stderr,
			    "Co-ordinate list length: %u must be even for ellipses\n",
			    num_coords);
	    return (FALSE);
	}
	convert_to_pixcoords (canvas, num_coords, types, x_arr, y_arr,
			      coord_pack_size, px, py);
	num_coords /= 2;
	(void) canvas_convert_from_canvas_coord (canvas,
						 0.0, 0.0, &tmp_x, &tmp_y);
	ptr = types + num_coords * coord_pack_size;
	for (count1 = 0; count1 < num_coords; ++count1, ptr += coord_pack_size)
	{
	    switch (*(unsigned int *) ptr)
	    {
	      case OVERLAY_COORD_PIXEL:
	      case OVERLAY_COORD_LAST:
		break;
	      case OVERLAY_COORD_RELATIVE:
		py[num_coords + count1] = height - 1 - py[num_coords + count1];
		px[num_coords + count1] -= x_offset;
		py[num_coords + count1] -= y_offset;
		break;
	      case OVERLAY_COORD_WORLD:
		px[num_coords + count1] -= tmp_x;
		py[num_coords + count1] = tmp_y - py[num_coords + count1];
		break;
	    }
	}
	if (object_code == OBJECT_ELLIPSES)
	{
	    kwin_draw_ellipses (pixcanvas,
				px, py, px + num_coords, py + num_coords,
				num_coords, pixel_value);
	}
	else
	{
	    kwin_fill_ellipses (pixcanvas,
				px, py, px + num_coords, py + num_coords,
				num_coords, pixel_value);

	}
	break;
      case OBJECT_SEGMENTS:
	if (num_coords % 2 != 0)
	{
	    (void) fprintf (stderr,
			    "Co-ordinate list length: %u must be even for segments\n",
			    num_coords);
	    return (FALSE);
	}
	convert_to_pixcoords (canvas, num_coords, types, x_arr, y_arr,
			      coord_pack_size, px, py);
	num_coords /= 2;
	ptr = types + num_coords * coord_pack_size;
	kwin_draw_segments (pixcanvas,
			    px, py, px + num_coords, py + num_coords,
			    num_coords, pixel_value);
	break;
      case OBJECT_VECTORS:
	if (num_coords % 2 != 0)
	{
	    (void) fprintf (stderr,
			    "Co-ordinate list length: %u must be even for vectors\n",
			    num_coords);
	    return (FALSE);
	}
	convert_to_pixcoords (canvas, num_coords, types, x_arr, y_arr,
			      coord_pack_size, px, py);
	num_coords /= 2;
	(void) canvas_convert_from_canvas_coord (canvas,
						 0.0, 0.0, &tmp_x, &tmp_y);
	ptr = types + num_coords * coord_pack_size;

	for (count1 = 0; count1 < num_coords; ++count1, ptr += coord_pack_size)
	{
	    switch (*(unsigned int *) ptr)
	    {
	      case OVERLAY_COORD_PIXEL:
	      case OVERLAY_COORD_LAST:
		break;
	      case OVERLAY_COORD_RELATIVE:
		py[num_coords + count1] = height - 1 - py[num_coords + count1];
		px[num_coords + count1] -= x_offset;
		py[num_coords + count1] -= y_offset;
		break;
	      case OVERLAY_COORD_WORLD:
		px[num_coords + count1] -= tmp_x;
		py[num_coords + count1] = tmp_y - py[num_coords + count1];
		break;
	    }
	    px[num_coords + count1] += px[count1];
	    py[num_coords + count1] += py[count1];
	}
	kwin_draw_segments (pixcanvas,
			    px, py, px + num_coords, py + num_coords,
			    num_coords, pixel_value);

	break;
      default:
	(void) fprintf (stderr, "Illegal object code: %u\n", object_code);
	a_prog_bug (function_name);
	break;
    }
    return (TRUE);
}   /*  End Function draw_object  */

static void convert_to_pixcoords (KWorldCanvas canvas, unsigned int num_coords,
				  char *types, char *x_arr, char *y_arr,
				  unsigned int pack_size, int *px, int *py)
/*  This routine will convert a number of co-ordinates to pixel canvas
    co-ordinates.
    <canvas> The world canvas object.
    The number of co-ordinates to convert must be given by  num_coords  .
    The first co-ordinate type value must be pointed to by  types  .
    The first horizontal co-ordinate value must be pointed to by  x_arr  .
    The first vertical co-ordinate value must be pointed to by  y_arr  .
    The size of the co-ordinate packets must be given by  pack_size  .
    The horizontal pixel co-ordinates will be written to the array pointed to
    by  px  .
    The vertical pixel co-ordinates will be written to the array pointed to by
    py  .
    [RETURNS] Nothing.
*/
{
    unsigned int count;
    double x, y, left_x, right_x, bottom_y, top_y;
    double value[2];
    double offset = 0.01;
    static int last_x = 0;
    static int last_y = 0;
    static char function_name[] = "__overlay_convert_to_pixcoords";

    canvas_get_attributes (canvas,
			   CANVAS_ATT_LEFT_X, &left_x,
			   CANVAS_ATT_RIGHT_X, &right_x,
			   CANVAS_ATT_BOTTOM_Y, &bottom_y,
			   CANVAS_ATT_TOP_Y, &top_y,
			   CANVAS_ATT_END);
    for (count = 0; count < num_coords;
	 ++count, types += pack_size, x_arr += pack_size, y_arr += pack_size,
	 ++px, ++py)
    {
	/*  Need to use the generic routine because doubles will not be
	    aligned. Stupid Sparc processor doesn't like it.
	*/
	(void) ds_get_element (x_arr, K_DOUBLE, value, (flag *) NULL);
	x = value[0];
	(void) ds_get_element (y_arr, K_DOUBLE, value, (flag *) NULL);
	y = value[0];
	switch (*(unsigned int *) types)
	{
	  case OVERLAY_COORD_PIXEL:
	    *px = (x + offset);
	    *py = (y + offset);
	    break;
	  case OVERLAY_COORD_RELATIVE:
	    x = left_x + x * (right_x - left_x);
	    y = bottom_y + y * (top_y - bottom_y);
	  case OVERLAY_COORD_WORLD:
	    (void) canvas_convert_from_canvas_coord (canvas, x, y, px, py);
	    break;
	  case OVERLAY_COORD_LAST:
	    *px = last_x;
	    *py = last_y;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal co-ordinate type: %u\n",
			    *(unsigned int *) types);
	    a_prog_bug (function_name);
	    break;
	}
	last_x = *px;
	last_y = *py;
    }
}   /*  End Function convert_to_pixcoords  */


/*  Token handling routines  */

static flag send_token_request (KOverlayList olist)
/*  This routine will send a token request for an overlay list.
    <olist> The overlay list object.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    char *coords;
    packet_desc *coord_desc;
    list_entry *instruction;
    extern packet_desc *object_desc;
    static char function_name[] = "send_token_request";

    VERIFY_OVERLAYLIST (olist);
    if (olist->have_token)
    {
	(void) fprintf (stderr, "Already have token\n");
	a_prog_bug (function_name);
    }
    if (olist->requested_token) return (TRUE);
    if ( ( instruction = create_generic (olist, OBJECT_REQUEST_TOKEN, NULL,
					 0, &coord_desc, &coords, NULL) )
	== NULL )
    {
	m_error_notify (function_name, "token request");
	return (FALSE);
    }
    if (olist->master == NULL)
    {
	/*  This is master: look at slave connection with token  */
	if (olist->token_conn == NULL)
	{
	    (void) fprintf (stderr, "Nowhere to get token from\n");
	    a_prog_bug (function_name);
	}
	channel = conn_get_channel (olist->token_conn);
    }
    else
    {
	channel = conn_get_channel (olist->master);
    }
    dsrw_write_packet (channel, object_desc, instruction->data);
    ds_dealloc_data (object_desc, instruction->data);
    m_free ( (char *) instruction );
    if ( !ch_flush (channel) ) return (FALSE);
    olist->requested_token = TRUE;
    return (TRUE);
}   /*  End Function send_token_request  */

static flag send_token_grant (KOverlayList olist, Connection conn)
/*  This routine will send a token grant for an overlay list.
    <olist> The overlay list object.
    The connection to send the grant to must be given by  conn  .
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    char *coords;
    packet_desc *coord_desc;
    list_entry *instruction;
    extern packet_desc *object_desc;
    static char function_name[] = "send_token_grant";

    VERIFY_OVERLAYLIST (olist);
    if (!olist->have_token)
    {
	(void) fprintf (stderr, "Do not have token\n");
	a_prog_bug (function_name);
    }
    if ( (olist->master != NULL) && (conn != olist->master) )
    {
	(void) fprintf (stderr, "Slave not sending token to master\n");
	a_prog_bug (function_name);
    }
    if ( ( instruction = create_generic (olist, OBJECT_GRANT_TOKEN, NULL,
					 0, &coord_desc, &coords, NULL) )
	== NULL )
    {
	m_error_notify (function_name, "token grant");
	return (FALSE);
    }
    channel = conn_get_channel (conn);
    dsrw_write_packet (channel, object_desc, instruction->data);
    ds_dealloc_data (object_desc, instruction->data);
    m_free ( (char *) instruction );
    if ( !ch_flush (channel) ) return (FALSE);
    olist->have_token = FALSE;
    if (olist->master == NULL) olist->token_conn = conn;
    return (TRUE);
}   /*  End Function send_token_grant  */

static flag process_conn_token_request (KOverlayList olist, Connection conn)
/*  This routine will process a request for a token for an overlay list from a
    "2D_overlay" connection.
    <olist> The overlay list object.
    The connection from where the token request originated must be given by
    conn  .
    [RETURNS] TRUE on success, else FALSE.
*/
{
    struct token_request_type *entry;
    static char function_name[] = "process_conn_token_request";

    VERIFY_OVERLAYLIST (olist);
    if (conn == NULL)
    {
	(void) fprintf (stderr, "Who asked for token?\n");
	a_prog_bug (function_name);
    }
    if (olist->master == NULL)
    {
	/*  This is the master: find the token  */
	if (olist->have_token) return ( send_token_grant (olist, conn) );
	/*  Do not have the token: must be with a slave  */
	if (olist->token_conn == NULL)
	{
	    (void) fprintf (stderr, "MASTER: who has the token?\n");
	    a_prog_bug (function_name);
	}
	/*  Check for existing token request  */
	for (entry = olist->first_token_request; entry != NULL;
	     entry = entry->next)
	{
	    if (conn == entry->conn)
	    {
		(void) fprintf (stderr,
				"MASTER: slave has already asked for token\n");
		a_prog_bug (function_name);
	    }
	}
	if ( ( entry = (struct token_request_type *) m_alloc (sizeof *entry) )
	    == NULL )
	{
	    m_error_notify (function_name, "token request");
	    return (FALSE);
	}
	/*  Append to list  */
	entry->conn = conn;
	entry->prev = olist->last_token_request;
	entry->next = NULL;
	if (olist->last_token_request != NULL)
	{
	    olist->last_token_request->next = entry;
	}
	olist->last_token_request = entry;
	if (olist->first_token_request == NULL)
	{
	    olist->first_token_request = entry;
	}
	return ( send_token_request (olist) );
    }
    /*  This is a slave  */
    if (!olist->have_token)
    {
	(void) fprintf(stderr,
		       "SLAVE: request for token but I don't have it\n");
	a_prog_bug (function_name);
    }
    if (conn != olist->master)
    {
	(void) fprintf (stderr,
			"SLAVE: request for token not from master\n");
	a_prog_bug (function_name);
    }
    return ( send_token_grant (olist, olist->master) );
}   /*  End Function process_conn_token_request  */

static flag process_token_receive (KOverlayList olist, Connection conn)
/*  This routine will process the receipt of a token for an overlay list.
    <olist> The overlay list object.
    The connection from where the token originated must be given by   conn  .
    [RETURNS] TRUE on success, else FALSE.
*/
{
    Channel channel;
    struct token_request_type *entry;
    list_header *list_head;
    list_entry *instruction;
    list_entry *next_instruction;
    extern packet_desc *object_desc;
    static char function_name[] = "process_token_receive";

    VERIFY_OVERLAYLIST (olist);
    if (olist->have_token)
    {
	(void) fprintf (stderr, "Already have token\n");
	a_prog_bug (function_name);
    }
    list_head = olist->buf_list;
    if (olist->master == NULL)
    {
	/*  Master: must have come from slave  */
	if (conn != olist->token_conn)
	{
	    (void)fprintf(stderr,
			  "MASTER: token received from slave which doesn't have it\n");
	    a_prog_bug (function_name);
	}
	olist->token_conn = NULL;
	olist->have_token = TRUE;
	olist->requested_token = FALSE;
	for (instruction = list_head->first_frag_entry; instruction != NULL;
	     instruction = instruction->next)
	{
	    if ( !transmit_to_slaves (olist, instruction, NULL) )
	    {
		ds_dealloc_list_entries (object_desc, list_head);
		return (FALSE);
	    }
	    if ( !process_instruction (olist, instruction, NULL) )
	    {
		ds_dealloc_list_entries (object_desc, list_head);
		return (FALSE);
	    }
	    list_head->first_frag_entry = instruction->next;
	    --list_head->length;
	}
	list_head->last_frag_entry = NULL;
	if ( (entry = olist->first_token_request) == NULL ) return (TRUE);
	if ( !send_token_grant (olist, entry->conn) ) return (FALSE);
	remove_token_request (olist, entry->conn);
	/*  If no more token requests: leave token where it is  */
	if (olist->first_token_request == NULL) return (TRUE);
	return ( send_token_request (olist) );
    }
    /*  Slave: must have come from master  */
    if (conn != olist->master)
    {
	(void) fprintf (stderr, "SLAVE: token received but not from master\n");
	a_prog_bug (function_name);
    }
    olist->have_token = TRUE;
    olist->requested_token = FALSE;
    channel = conn_get_channel (olist->master);
    if ( !write_entries (channel, object_desc, list_head->first_frag_entry) )
    {
	ds_dealloc_list_entries (object_desc, list_head);
	return (FALSE);
    }
    for (instruction = list_head->first_frag_entry; instruction != NULL;
	 instruction = next_instruction)
    {
	next_instruction = instruction->next;
	if ( !process_instruction (olist, instruction, NULL) )
	{
	    (void) fprintf (stderr, "Error processing instruction\n");
	    ds_dealloc_list_entries (object_desc, list_head);
	    return (FALSE);
	}
	list_head->first_frag_entry = next_instruction;
	--list_head->length;
    }
    list_head->last_frag_entry = NULL;
    return (TRUE);
}   /*  End Function process_token_receive  */

static void remove_token_request (KOverlayList olist, Connection conn)
/*  This routine will remove a token request from the list.
    <olist> The overlay list object.
    The connection for the token request must be given by  conn  .
    [RETURNS] Nothing.
*/
{
    struct token_request_type *entry;
    struct token_request_type *next;
    static char function_name[] = "remove_token_request";

    VERIFY_OVERLAYLIST (olist);
    /*  Terminate any token requests from this slave  */
    for (entry = olist->first_token_request; entry != NULL; entry = next)
    {
	next = entry->next;
	if (conn == entry->conn)
	{
	    /*  Remove  */
	    if (next == NULL)
	    {
		olist->last_token_request = entry->prev;
	    }
	    else
	    {
		next->prev = entry->prev;
	    }
	    if (entry->prev == NULL)
	    {
		olist->first_token_request = next;
	    }
	    else
	    {
		entry->prev->next = next;
	    }
	    m_free ( (char *) entry );
	    return;
	}
    }
}   /*  End Function remove_token_request  */


/*  Miscellaneous routines follow  */

static flag process_app_instruction (KOverlayList olist,
				     list_entry *instruction)
/*  This routine will process an overlay instruction generated by the
    application (ie. not one that came over the network).
    The list must be given by  olist  .
    The instruction must be pointed to by  instruction  .
    The routine returns TRUE on succes, else it returns FALSE.
*/
{
    Channel channel;
    extern packet_desc *object_desc;
    static char function_name[] = "__overlay_process_app_instruction";

    VERIFY_OVERLAYLIST (olist);
    /*  Quick sanity check  */
    if ( !olist->have_token && (olist->master == NULL) &&
	(olist->slave_count < 1) )
    {
	(void) fprintf (stderr, "Lost token!\n");
	a_prog_bug (function_name);
    }
    if (olist->have_token)
    {
	/*  OK: first transmit if needed  */
	if (olist->master != NULL)
	{
	    /*  Send it to the master  */
	    channel = conn_get_channel (olist->master);
	    dsrw_write_packet (channel, object_desc, instruction->data);
	    if ( !ch_flush (channel) )
	    {
		ds_dealloc_data (object_desc, instruction->data);
		m_free ( (char *) instruction );
		return (FALSE);
	    }
	}
	(void) transmit_to_slaves (olist, instruction, NULL);
	return ( process_instruction (olist, instruction, NULL) );
    }
    /*  Do not have the token: store the instruction for later  */
    if ( !send_token_request (olist) )
    {
	ds_dealloc_data (object_desc, instruction->data);
	m_free ( (char *) instruction );
	return (FALSE);
    }
    ds_list_append (olist->buf_list, instruction);
    return (TRUE);
}   /*  End Function process_app_instruction  */

static list_entry *create_generic (KOverlayList olist,
				   unsigned int object_code, char *colourname,
				   unsigned int num_coords,
				   packet_desc **coord_desc, char **coords,
				   unsigned int *object_id)
/*  This routine will create a generic overlay object for a particular overlay
    list.
    <olist> The overlay list object.
    The object code must be given by  object_code  .
    <colourname> The colour name.
    The number of co-ordinates to allocate must be given by  num_coords  .
    The pointer to the co-ordinate packet descriptor will be written to the
    storage pointed to by  coord_desc  .
    The pointer to the allocated contiguous list of co-ordinate packets will be
    written to the storage pointed to by  coords  .
    The ID of the object will be written to the storage pointed to by
    object_id  .If this is NULL, the instruction is assumed to be a non-object
    and nothing is written here.
    The routine returns a pointer to the object on success,
    else it returns NULL.
*/
{
    unsigned int num_restr;
    unsigned int count;
    unsigned int restr_pack_size;
    unsigned int obj_id;
    char *names;
    char *values;
    char *xlabel;
    char *ylabel;
    char **restr_names;
    double *restr_values;
    char *ptr;
    list_entry *object;
    list_header *coord_list;
    list_header *restriction_list;
    packet_desc *restriction_desc;
    double value[2];
    extern packet_desc *object_desc;
    static char function_name[] = "__overlay_create_generic";

    /*  Get specification  */
    if (olist->specification_canvas == NULL)
    {
	/*  Internal to overlay list  */
	xlabel = olist->xlabel;
	ylabel = olist->ylabel;
	num_restr = olist->num_restrictions;
	restr_names = olist->restriction_names;
	restr_values = olist->restriction_values;
    }
    else
    {
	/*  Get it from the canvas  */
	canvas_get_specification (olist->specification_canvas,
				  &xlabel, &ylabel,
				  &num_restr, &restr_names, &restr_values);
    }
    if (object_id != NULL)
    {
	if (++olist->last_object_id == 0)
	{
	    (void) fprintf (stderr, "Object ID counter wrapped around!\n");
	    return (NULL);
	}
	*object_id = olist->last_object_id;
	obj_id = olist->last_object_id;
    }
    else
    {
	obj_id = 0;
    }
    if ( ( object = ds_alloc_list_entry (object_desc, TRUE) ) == NULL )
    {
	m_error_notify (function_name, "object entry");
	return (NULL);
    }
    ptr = object->data + ds_get_element_offset (object_desc,
						OBJECT_LISTID_INDEX);
    *(unsigned int *) ptr = olist->my_id;
    ptr = object->data + ds_get_element_offset (object_desc,
						OBJECT_CODE_INDEX);
    *(unsigned int *) ptr = object_code;
    ptr = object->data + ds_get_element_offset (object_desc,
						OBJECT_OBJECTID_INDEX);
    *(unsigned int *) ptr = obj_id;
    ptr = object->data + ds_get_element_offset (object_desc,
						OBJECT_COORD_INDEX);
    coord_list = *(list_header **) ptr;
    coord_list->sort_type = SORT_RANDOM;
    *coord_desc = (packet_desc *)object_desc->element_desc[OBJECT_COORD_INDEX];
    if ( !ds_alloc_contiguous_list (*coord_desc, coord_list, num_coords, TRUE,
				    TRUE) )
    {
	m_error_notify (function_name, "co-ordinate list");
	ds_dealloc_data (object_desc, object->data);
	m_free ( (char *) object );
	return (NULL);
    }
    *coords = coord_list->contiguous_data;
    ptr = object->data + ds_get_element_offset (object_desc,
						OBJECT_COLOURNAME_INDEX);
    if (colourname != NULL)
    {
	if ( ( *(char **) ptr = st_dup (colourname) ) == NULL )
	{
	    m_error_notify (function_name, "colour name");
	    ds_dealloc_data (object_desc, object->data);
	    m_free ( (char *) object );
	    return (NULL);
	}
    }
    /*  Add labels if needed  */
    if (xlabel != NULL)
    {
	ptr = object->data + ds_get_element_offset (object_desc,
						    OBJECT_X_INDEX);
	if ( ( *(char **) ptr = st_dup (xlabel) ) == NULL )
	{
	    m_error_notify (function_name, "x label");
	    ds_dealloc_data (object_desc, object->data);
	    m_free ( (char *) object );
	    return (NULL);
	}
    }
    if (ylabel != NULL)
    {
	ptr = object->data + ds_get_element_offset (object_desc,
						    OBJECT_Y_INDEX);
	if ( ( *(char **) ptr = st_dup (ylabel) ) == NULL )
	{
	    m_error_notify (function_name, "y label");
	    ds_dealloc_data (object_desc, object->data);
	    m_free ( (char *) object );
	    return (NULL);
	}
    }
    ptr = object->data + ds_get_element_offset (object_desc,
						OBJECT_RESTRICTION_INDEX);
    restriction_desc = ( (packet_desc *)
			object_desc->element_desc[OBJECT_RESTRICTION_INDEX]);
    restriction_list = *(list_header **) ptr;
    restriction_list->sort_type = SORT_RANDOM;
    if (num_restr < 1) return (object);
    /*  Add restrictions  */
    if ( !ds_alloc_contiguous_list (restriction_desc, restriction_list,
				    num_restr, TRUE, TRUE) )
    {
	m_error_notify (function_name, "restriction list");
	ds_dealloc_data (object_desc, object->data);
	m_free ( (char *) object );
	return (NULL);
    }
    restr_pack_size = ds_get_packet_size (restriction_desc);
    ptr = restriction_list->contiguous_data;
    names = ptr + ds_get_element_offset (restriction_desc,
					 RESTRICTION_NAME_INDEX);
    values = ptr + ds_get_element_offset (restriction_desc,
					  RESTRICTION_VALUE_INDEX);
    value[1] = 0.0;
    for (count = 0; count < num_restr;
	 ++count, names += restr_pack_size, values += restr_pack_size)
    {
	if ( ( *(char **) names = st_dup (restr_names[count]) ) == NULL )
	{
	    ds_dealloc_data (object_desc, object->data);
	    m_free ( (char *) object );
	    return (NULL);
	}
	value[0] = restr_values[count];
	if ( !ds_put_named_element (restriction_desc, values,
				    "Overlay Restriction Value", value) )
	{
	    ds_dealloc_data (object_desc, object->data);
	    m_free ( (char *) object );
	    return (NULL);
	}
    }
    return (object);
}   /*  End Function create_generic  */

static flag remove_object (KOverlayList olist, unsigned int object_id,
			   unsigned int list_id)
/*  This routine will remove an object from an overlay list.
    <olist> The overlay list object.
    The ID of the object must be given by  object_id  .
    The ID of the list that created the object must be given by  list_id  .
    The routine returns TRUE if the object ID was valid, else it returns FALSE.
*/
{
    list_header *list_head;
    list_entry *entry;
    extern packet_desc *object_desc;

    if ( ( entry = find_object (olist, object_id, list_id) )
	== NULL ) return (FALSE);
    ds_dealloc_data (object_desc, entry->data);
    list_head = olist->list_head;
    if (entry->prev == NULL)
    {
	list_head->first_frag_entry = entry->next;
    }
    else
    {
	entry->prev->next = entry->next;
    }
    if (entry->next == NULL)
    {
	list_head->last_frag_entry = entry->prev;
    }
    else
    {
	entry->next->prev = entry->prev;
    }
    m_free ( (char *) entry );
    --list_head->length;
    return (TRUE);
}   /*  End Function remove_object  */

static list_entry *find_object (KOverlayList olist, unsigned int object_id,
				unsigned int list_id)
/*  This routine will find an object in an overlay list.
    <olist> The overlay list object.
    The ID of the object must be given by  object_id  .
    The ID of the list that created the object must be given by  list_id  .
    The routine returns a pointer to the list entry on success, else it
    returns NULL.
*/
{
    uaddr object_id_offset, list_id_offset;
    list_header *list_head;
    list_entry *curr_entry;
    list_entry *found_entry = NULL;
    extern packet_desc *object_desc;

    object_id_offset = ds_get_element_offset (object_desc,
					      OBJECT_OBJECTID_INDEX);
    list_id_offset = ds_get_element_offset (object_desc, OBJECT_LISTID_INDEX);
    list_head = olist->list_head;
    for (curr_entry = list_head->first_frag_entry;
	 (curr_entry != NULL) && (found_entry == NULL);
	 curr_entry = curr_entry->next)
    {
	if (*(unsigned int *) (object_id_offset + curr_entry->data)
	    != object_id) continue;
	if (*(unsigned int *) (list_id_offset + curr_entry->data)
	    != list_id) continue;
	found_entry = curr_entry;
    }
    return (found_entry);
}   /*  End Function find_object  */

static flag move_object (KOverlayList olist, unsigned int object_id,
			 unsigned int list_id, double dx, double dy)
/*  This routine will move an object in an overlay list.
    <olist> The overlay list object.
    The ID of the object must be given by  object_id  .
    The ID of the list that created the object must be given by  list_id  .
    The horizontal distance to move must be given by  dx  .
    The vertical distance to move must be given by  dy  .
    The routine returns TRUE if the object ID was valid, else it returns FALSE.
*/
{
    unsigned int object_code;
    unsigned int num_coords, coord_pack_size, count;
    list_entry *entry;
    list_header *coord_list;
    packet_desc *coord_desc;
    char *coords, *ptr, *x_arr, *y_arr;
    extern packet_desc *object_desc;
    static char function_name[] = "move_object";

    if ( ( entry = find_object (olist, object_id, list_id) )
	== NULL ) return (FALSE);
    ptr = entry->data + ds_get_element_offset (object_desc,
					       OBJECT_CODE_INDEX);
    object_code = *(unsigned int *) ptr;
    ptr = entry->data + ds_get_element_offset (object_desc,
					       OBJECT_COORD_INDEX);
    coord_list = *(list_header **) ptr;
    num_coords = coord_list->length;
    coords = coord_list->contiguous_data;
    coord_desc= (packet_desc *)object_desc->element_desc[OBJECT_COORD_INDEX];
    coord_pack_size = ds_get_packet_size (coord_desc);
    x_arr = coords + ds_get_element_offset (coord_desc, COORD_X_INDEX);
    y_arr = coords + ds_get_element_offset (coord_desc, COORD_Y_INDEX);
    switch (object_code)
    {
      case OBJECT_LINE:
      case OBJECT_LINES:
      case OBJECT_TEXT:
	break;
      case OBJECT_ELLIPSE:
      case OBJECT_FELLIPSE:
	num_coords = 1;
	break;
      case OBJECT_FPOLY:
	break;
      case OBJECT_VECTOR:
	num_coords = 1;
	break;
      case OBJECT_ELLIPSES:
      case OBJECT_FELLIPSES:
	num_coords /= 2;
	break;
      case OBJECT_SEGMENTS:
	break;
      case OBJECT_VECTORS:
	num_coords /= 2;
	break;
      default:
	(void) fprintf (stderr, "Illegal object code: %u\n", object_code);
	a_prog_bug (function_name);
	break;
    }
    for (count = 0; count < num_coords;
	 ++count, x_arr += coord_pack_size, y_arr += coord_pack_size)
    {
	*(double *) x_arr += dx;
	*(double *) y_arr += dy;
    }
    return (TRUE);
}   /*  End Function move_object  */

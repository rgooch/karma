/*LINTLIBRARY*/
/*PREFIX:"iedit_"*/
/*  image_edit.c

    This code provides routines to manipulate image editing instructions.

    Copyright (C) 1992,1993  Richard Gooch

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

/*

    This file contains the various utility routines for creating and
    manipulating 2-dimensional (image) editing instructions.


    Written by      Richard Gooch   27-MAR-1993

    Updated by      Richard Gooch   28-MAR-1993

    Updated by      Richard Gooch   11-APR-1993: Removed need for passing of
  edit_coord_list_index  parameter.

    Updated by      Richard Gooch   25-APR-1993: Changed from structure for
  edit co-ordinates in  karma_iedit.h  to typedef in  karma_ds.h

    Updated by      Richard Gooch   7-MAY-1993: Cosmetic changes to quieten
  lint.

    Updated by      Richard Gooch   27-AUG-1993: Added some howto hints for
  iedit_create_desc  and created  KImageEditList  class.

    Updated by      Richard Gooch   31-AUG-1993: Declared some functions meant
  to be private as such.

    Last updated by Richard Gooch   6-OCT-1993: Fixed bug in
  iedit_add_instruction  when a slave adds an instruction.


*/

#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <karma.h>
#include <karma_dsra.h>
#include <karma_dsrw.h>
#include <karma_conn.h>
#include <karma_ds.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>

typedef struct instruction_list_type * KImageEditList;

#define KIMAGEEDITLIST_DEFINED
#include <karma_iedit.h>

#define MAGIC_NUMBER (unsigned int) 1472349087
#define PROTOCOL_VERSION (unsigned int) 1

#define VERIFY_ILIST(ilist) if (ilist == NULL) \
{(void) fprintf (stderr, "NULL KImageEditList passed\n"); \
 a_prog_bug (function_name); } \
if ( (*ilist).magic_number != MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid KImageEditList object\n"); \
 a_prog_bug (function_name); }

struct instruction_list_type
{
    unsigned int magic_number;
    list_header *list_head;
    void *info;
    void (*process_add) ();
    void (*process_loss) ();
    void (*process_apply) ();
    Channel master;
};

/*  Private data  */
static packet_desc *instruction_desc = NULL;
static packet_desc *coord_list_desc = NULL;
static unsigned int edit_coord_list_index = 0;
static KImageEditList masterable_list = NULL;
static KImageEditList slaveable_list = NULL;
static char *str_instruction_desc[] =
{
    "PACKET",
    "  3",
    "END",
    "  ELEMENT",
    "    UINT",
    "    Edit Instruction",
    "  END",
    "  ELEMENT",
    "    DCOMPLEX",
    "    Edit Object Value",
    "  END",
    "  ELEMENT",
    "    LISTP",
    "  END",
    "    PACKET",
    "      2",
    "    END",
    "      ELEMENT",
    "        DOUBLE",
    "        Edit Object Abscissa",
    "      END",
    "      ELEMENT",
    "        DOUBLE",
    "        Edit Object Ordinate",
    "      END",
    NULL
};


/*  Local functions  */
static void initialise_iedit_package ();
static flag add_instruction (/* ilist, instruction */);
static flag remove_instructions (/* ilist, num_to_remove */);
static flag apply_instructions (/* ilist */);
static flag register_new_edit_slave (/* connection, info */);
static flag read_edits_from_slave (/* connection, info */);
static flag verify_edit_slave_connection (/* info */);
static flag register_edit_slave_connection (/* connection, info */);
static flag read_edits_from_master (/* connection, info */);
static void register_master_loss (/* connection, info */);
static flag process_local_instruction (/* ilist, instruction */);
static flag transmit_to_slaves (/* ilist, instruction */);
static flag write_list (/* channel, list_desc, list_head */);


/*PUBLIC_FUNCTION*/
KImageEditList iedit_create_list (add_func, loss_func, apply_func, info)
/*  This routine will create a managed image edit instruction list.
    The function which will be called when a new instruction has been added to
    the list must be pointed to by  add_func  .
    The interface to this routine is as follows:

    void add_func (ilist, instruction, info)
    *   This routine will process a single edit instruction which has been
        added to  a managed image edit instruction list.
        The managed image edit instruction list will be given by  ilist  .
	The edit instruction must be pointed to by  instruction  .
	The arbitrary information pointer for the list will be pointed to by
	info  .
	The routine returns nothing.
    *
    KImageEditList ilist;
    list_entry *instruction;
    void **info;

    The function which will be called when an instruction has been removed from
    the list must be pointed to by  loss_func  .
    The interface to this routine is as follows:

    void loss_func (ilist, info)
    *   This routine will process the loss of an edit instruction from a
        managed image edit instruction list.
        The managed image edit instruction list will be given by  ilist  .
	The arbitrary information pointer for the list will be pointed to by
	info  .
	The routine returns nothing.
    *
    KImageEditList ilist;
    void **info;

    The function which will be called when the edit instructions are to be
    applied (prior to clearing of the list) must be pointed to by  apply_func
    The interface to this routine is as follows:

    void apply_func (ilist, info)
    *   This routine will apply (commit) an edit list prior to the list being
        cleared.
        The managed image edit instruction list will be given by  ilist  .
	The arbitrary information pointer for the list will be pointed to by
	info  .
	The routine returns nothing.
    *
    KImageEditList ilist;
    void **info;

    The arbitrary information pointer for the edit list must be pointed to by
    info  .This may be NULL.
    The routine returns a KImageEditList object on success,
    else it returns NULL.
*/
void (*add_func) ();
void (*loss_func) ();
void (*apply_func) ();
void *info;
{
    Channel channel;
    KImageEditList ilist;
    extern KImageEditList masterable_list;
    extern KImageEditList slaveable_list;
    extern packet_desc *instruction_desc;
    extern char *str_instruction_desc[];
    static char function_name[] = "iedit_create_list";

    initialise_iedit_package ();
    if ( ( ilist = (KImageEditList) m_alloc (sizeof *ilist) ) == NULL )
    {
	m_error_notify (function_name, "KImageEditList object");
	return (NULL);
    }
    (*ilist).magic_number = MAGIC_NUMBER;
    if ( ( (*ilist).list_head = ds_alloc_list_head () ) == NULL )
    {
	m_error_notify (function_name, "list header");
	m_free ( (char *) ilist );
	return (NULL);
    }
    (* (*ilist).list_head ).sort_type = SORT_RANDOM;
    (*ilist).info = info;
    (*ilist).process_add = add_func;
    (*ilist).process_loss = loss_func;
    (*ilist).process_apply = apply_func;
    (*ilist).master = NULL;
    if (masterable_list == NULL) masterable_list = ilist;
    if (slaveable_list == NULL) slaveable_list = ilist;
    return (ilist);
}   /*  End Function iedit_create_list  */

/*PUBLIC_FUNCTION*/
packet_desc *iedit_get_instruction_desc ()
/*  This routine will get the list descriptor for image edit instructions.
    The routine returns a pointer to the descriptor.
*/
{
    extern packet_desc *instruction_desc;

    initialise_iedit_package ();
    return (instruction_desc);
}   /*  End Function iedit_get_instruction_desc  */

/*PUBLIC_FUNCTION*/
edit_coord *iedit_alloc_edit_coords (num_coords)
/*  This routine will allocate an array of edit co-ordinates.
    The number of co-ordinates to allocate must be given by  num_coords  .
    The routine returns a pointer to a statically allocated array of
    co-ordinate structures on success, else it returns NULL.
*/
unsigned int num_coords;
{
    static unsigned int num_allocated_coords = 0;
    static edit_coord *coord_array = NULL;
    static char function_name[] = "iedit_alloc_edit_coords";

    /*  Check allocation  */
    if (num_coords > num_allocated_coords)
    {
	/*  Old co-ordinate array too small: reallocate  */
	if (coord_array != NULL)
	{
	    m_free ( (char *) coord_array );
	    coord_array = NULL;
	    num_allocated_coords = 0;
	}
	if ( ( coord_array = (edit_coord *)
	      m_alloc (sizeof *coord_array * num_coords) )
	    == NULL )
	{
	    m_error_notify (function_name, "array of edit co-ordinates");
	    return (NULL);
	}
	num_allocated_coords = num_coords;
    }
    return (coord_array);
}   /*  End Function iedit_alloc_edit_coords  */

/*PUBLIC_FUNCTION*/
flag iedit_get_edit_coords (list_head, coords)
/*  This routine will get a number of editing co-ordinates from a co-ordinate
    list.
    The routine will extract all co-ordinates in the list.
    The list header must be pointed to by  list_head  .
    The pointer to an internally allocated array of extracted co-ordinates is
    written to the storage pointed to by  coords  .
    The routine returns TRUE on success, else it returns FALSE.
*/
list_header *list_head;
edit_coord **coords;
{
    unsigned int abscissa_type;
    unsigned int ordinate_type;
    unsigned int abscissa_offset;
    unsigned int ordinate_offset;
    unsigned int elem_index;
    unsigned int coord_count;
    unsigned int pack_size;
    double value[2];
    char *data;
    list_entry *curr_entry;
    edit_coord *coord_array;
    extern packet_desc *coord_list_desc;
    static char function_name[] = "iedit_get_edit_coords";

    /*  Get abscissa info  */
    if ( ( elem_index = ds_f_elem_in_packet (coord_list_desc,
					     "Edit Object Abscissa") )
	>= (*coord_list_desc).num_elements )
    {
	(void) fprintf (stderr, "Error finding edit abscissa location\n");
	a_prog_bug (function_name);
    }
    abscissa_type = (*coord_list_desc).element_types[elem_index];
    abscissa_offset = ds_get_element_offset (coord_list_desc, elem_index);
    /*  Get ordinate info  */
    if ( ( elem_index = ds_f_elem_in_packet (coord_list_desc,
					     "Edit Object Ordinate") )
	>= (*coord_list_desc).num_elements )
    {
	(void) fprintf (stderr, "Error finding edit ordinate location\n");
	a_prog_bug (function_name);
    }
    ordinate_type = (*coord_list_desc).element_types[elem_index];
    ordinate_offset = ds_get_element_offset (coord_list_desc, elem_index);
    /*  Get co-ordinate array  */
    if ( ( coord_array =
	  iedit_alloc_edit_coords ( (unsigned int) (*list_head).length ) )
	== NULL )
    {
	m_error_notify (function_name, "array of edit co-ordinates");
	return (FALSE);
    }
    /*  Now extract co-ordinates  */
    pack_size = ds_get_packet_size (coord_list_desc);
    for (coord_count = 0, curr_entry = (*list_head).first_frag_entry,
	 data = (*list_head).contiguous_data;
	 coord_count < (*list_head).length;
	 ++coord_count)
    {
	if (coord_count >= (*list_head).contiguous_length)
	{
	    /*  Fragmented section of list  */
	    data = (*curr_entry).data;
	}
	if (ds_get_element (abscissa_offset + data,
			    abscissa_type, value, (flag *) NULL) != TRUE)
	{
	    (void) fprintf (stderr, "Error getting edit abscissa value\n");
	    return (FALSE);
	}
	coord_array[coord_count].abscissa = value[0];
	if (ds_get_element (ordinate_offset + data,
			    ordinate_type, value, (flag *) NULL) != TRUE)
	{
	    (void) fprintf (stderr, "Error getting edit ordinate value\n");
	    return (FALSE);
	}
	coord_array[coord_count].ordinate = value[0];
	if (coord_count < (*list_head).contiguous_length)
	{
	    /*  Contiguous section of list  */
	    data += pack_size;
	}
	else
	{
	    /*  Fragmented section of list  */
	    curr_entry = (*curr_entry).next;
	}
    }
    *coords = coord_array;
    return (TRUE);
}   /*  End Function iedit_get_edit_coords  */

/*PUBLIC_FUNCTION*/
flag iedit_add_instruction (ilist, instruction_code, coords, num_coords,
			    intensity)
/*  This routine will add a single edit instruction to a managed image edit
    instruction list. The  add_func  function registered for this instruction
    list will be called at a future time with a copy of the instruction to be
    added.
    The instruction list must be given by  ilist  .
    The instruction code must be given by  instruction_code  .
    The instruction co-ordinates must be pointed to by  coords  .
    The number of co-ordinates must be given by  num_coords  .
    The intensity value of the instruction must be pointed to by  intensity  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KImageEditList ilist;
unsigned int instruction_code;
edit_coord *coords;
unsigned int num_coords;
double intensity[2];
{
    unsigned int coord_count;
    unsigned int coord_pack_size;
    double value[2];
    char *coord_data;
    list_header *coord_list_head;
    list_entry *instruction;
    extern unsigned int edit_coord_list_index;
    extern packet_desc *instruction_desc;
    extern packet_desc *coord_list_desc;
    static char function_name[] = "iedit_add_instruction";

    VERIFY_ILIST (ilist);
    /*  Allocate instruction entry  */
    if ( ( instruction = ds_alloc_list_entry (instruction_desc, TRUE) )
	== NULL )
    {
	m_error_notify (function_name, "edit entry");
	return (FALSE);
    }
    /*  Set up co-ordinate list info  */
    coord_list_head = ( *(list_header **)
		       ( (*instruction).data +
			ds_get_element_offset (instruction_desc,
					       edit_coord_list_index) ) );
    (*coord_list_head).sort_type = SORT_RANDOM;
    /*  Allocate co-ordinate list  */
    if (ds_alloc_contiguous_list (coord_list_desc, coord_list_head,
				  num_coords, TRUE, TRUE)
	!= TRUE)
    {
	m_error_notify (function_name, "edit co-ordinate list");
	ds_dealloc_data (instruction_desc, (*instruction).data);
	m_free ( (char *) instruction );
	return (FALSE);
    }
    /*  Put instruction information into entry  */
    value[0] = instruction_code;
    if (ds_put_named_element (instruction_desc, (*instruction).data,
			      "Edit Instruction", value) != TRUE)
    {
	ds_dealloc_data (instruction_desc, (*instruction).data);
	m_free ( (char *) instruction );
	return (FALSE);
    }
    /*  Put intensity information into entry  */
    if (ds_put_named_element (instruction_desc, (*instruction).data,
			      "Edit Object Value", intensity) != TRUE)
    {
	ds_dealloc_data (instruction_desc, (*instruction).data);
	m_free ( (char *) instruction );
	return (FALSE);
    }
    /*  Put co-ordinates into list  */
    coord_pack_size = ds_get_packet_size (coord_list_desc);
    value[1] = 0.0;
    for (coord_count = 0, coord_data = (*coord_list_head).contiguous_data;
	 coord_count < num_coords;
	 ++coord_count, coord_data += coord_pack_size)
    {
	value[0] = coords[coord_count].abscissa;
	if (ds_put_named_element (coord_list_desc, coord_data,
				  "Edit Object Abscissa", value) != TRUE)
	{
	    ds_dealloc_data (instruction_desc, (*instruction).data);
	    m_free ( (char *) instruction );
	    return (FALSE);
	}
	value[0] = coords[coord_count].ordinate;
	if (ds_put_named_element (coord_list_desc, coord_data,
				  "Edit Object Ordinate", value) != TRUE)
	{
	    ds_dealloc_data (instruction_desc, (*instruction).data);
	    m_free ( (char *) instruction );
	    return (FALSE);
	}
    }
    /*  Now process instruction entry  */
    if ( (*ilist).master != NULL )
    {
	/*  Just send it to the master  */
	dsrw_write_packet ( (*ilist).master, instruction_desc,
			   (*instruction).data );
	if ( (instruction_code != EDIT_APPLY_INSTRUCTIONS) &&
	    (instruction_code != EDIT_UNDO_INSTRUCTIONS) &&
	    ( (*ilist).process_add != NULL ) )
	{
	    /*  Call registered callback  */
	    (* (*ilist).process_add ) (ilist, instruction, &(*ilist).info);
	}
	ds_dealloc_data (instruction_desc, (*instruction).data);
	m_free ( (char *) instruction );
	return ( ch_flush ( (*ilist).master ) );
    }
    /*  Master or standalone  */
    transmit_to_slaves (ilist, instruction);
    /*  Do it locally  */
    return ( process_local_instruction (ilist, instruction) );
}   /*  End Function iedit_add_instruction  */

/*PUBLIC_FUNCTION*/
flag iedit_remove_instructions (ilist, num_to_remove)
/*  This routine will remove a number of edit instructions from a managed
    image edit instruction list. The  loss_func  function registered for this
    instruction list will be called at a future time.
    The instruction list must be given by  ilist  .
    The number of instructions to remove must be given by  num_to_remove  .If
    this is 0, all instructions are removed.
    The routine returns TRUE on success, else it returns FALSE.
*/
KImageEditList ilist;
unsigned int num_to_remove;
{
    double value[2];
    static char function_name[] = "iedit_remove_instructions";

    VERIFY_ILIST (ilist);
    value[0] = num_to_remove;
    value[1] = 0.0;
    return ( iedit_add_instruction (ilist, EDIT_UNDO_INSTRUCTIONS,
				    (edit_coord *) NULL, 0, value) );
}   /*  End Function iedit_remove_instructions  */

/*PUBLIC_FUNCTION*/
flag iedit_apply_instructions (ilist)
/*  This routine will issue an apply (commit) request for a managed image edit
    instruction list. The  apply_func  function registered for this
    instruction list will be called at a future time. Some time after this,
    the  loss_func  registered will also be called.
    The instruction list must be given by  ilist  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KImageEditList ilist;
{
    double value[2];
    static char function_name[] = "iedit_apply_instructions";

    VERIFY_ILIST (ilist);
    value[0] = 0.0;
    value[1] = 0.0;
    return ( iedit_add_instruction (ilist, EDIT_APPLY_INSTRUCTIONS,
				    (edit_coord *) NULL, 0, value) );
}   /*  End Function iedit_apply_instructions  */

/*PUBLIC_FUNCTION*/
list_header *iedit_get_list (ilist)
/*  This routine will get the list of edit instructions associated with a
    managed image edit instruction list.
    The managed list must be given by  ilist  .
    The routine returns the list of edit instructions.
*/
KImageEditList ilist;
{
    static char function_name[] = "iedit_get_list";

    VERIFY_ILIST (ilist);
    return ( (*ilist).list_head );
}   /*  End Function iedit_get_list  */

/*PUBLIC_FUNCTION*/
void iedit_make_list_default_master (ilist)
/*  This routine will make a managed image edit instruction list the default
    list for accepting slave list connections.
    The managed list must be given by  ilist  .
    The routine returns nothing.
*/
KImageEditList ilist;
{
    extern KImageEditList masterable_list;
    static char function_name[] = "iedit_make_list_default_master";

    VERIFY_ILIST (ilist);
    if ( (*ilist).master != NULL )
    {
	(void) fprintf (stderr, "KImageEditList is a slave\n");
	a_prog_bug (function_name);
    }
    masterable_list = ilist;
}   /*  End Function iedit_make_list_default_master  */

/*PUBLIC_FUNCTION*/
void iedit_make_list_default_slave (ilist)
/*  This routine will make a managed image edit instruction list the default
    list for making slave list connections.
    The managed list must be given by  ilist  .
    The routine returns nothing.
*/
KImageEditList ilist;
{
    Connection conn;
    unsigned int num_connections;
    unsigned int conn_count;
    extern KImageEditList slaveable_list;
    static char function_name[] = "iedit_make_list_default_slave";

    VERIFY_ILIST (ilist);
    /*  Check for any possible slaves  */
    num_connections = conn_get_num_serv_connections ("2D_edit");
    for (conn_count = 0; conn_count < num_connections; ++conn_count)
    {
	if ( ( conn = conn_get_serv_connection ("2D_edit", conn_count) )
	    == NULL )
	{
	    (void) fprintf (stderr, "2D_edit connection: %u not found\n",
			    conn_count);
	    a_prog_bug (function_name);
	}
	if (conn_get_connection_info (conn) == ilist)
	{
	    (void) fprintf (stderr, "KImageEditList is a master\n");
	    a_prog_bug (function_name);
	}
    }
    slaveable_list = ilist;
}   /*  End Function iedit_make_list_default_slave  */


/*  Private functions follow  */

static void initialise_iedit_package ()
/*  This routine will create the edit instruction descriptor.
    The routine returns nothing.
*/
{
    Channel channel;
    unsigned int elem_count;
    extern unsigned int edit_coord_list_index;
    extern packet_desc *instruction_desc;
    extern packet_desc *coord_list_desc;
    extern char *str_instruction_desc[];
    static char function_name[] = "initialise_iedit_package";

    if (instruction_desc != NULL)
    {
	/*  Package is initialised  */
	return;
    }
    /*  Create descriptor  */
    if ( ( channel = ch_open_and_fill_memory (str_instruction_desc) ) == NULL )
    {
	m_abort (function_name, "memory channel");
    }
    if ( ( instruction_desc = dsra_packet_desc (channel) ) == NULL )
    {
	(void) ch_close (channel);
	m_abort (function_name, "edit instruction list descriptor");
    }
    (void) ch_close (channel);
    /*  Find first linked list (should be the co-ordinate list)  */
    for (elem_count = 0,
	 edit_coord_list_index = (*instruction_desc).num_elements;
	 elem_count < (*instruction_desc).num_elements;
	 ++elem_count)
    {
	if (LISTP == (*instruction_desc).element_types[elem_count])
	{
	    edit_coord_list_index = elem_count;
	}
    }
    if (edit_coord_list_index >= (*instruction_desc).num_elements)
    {
	(void) fprintf (stderr, "No linked list found\n");
	a_prog_bug (function_name);
    }
    coord_list_desc = (packet_desc *)
    (*instruction_desc).element_desc[edit_coord_list_index];
    /*  Register protocols  */
    conn_register_server_protocol ("2D_edit", PROTOCOL_VERSION, 0,
				   register_new_edit_slave,
				   read_edits_from_slave,
				   ( void (*) () ) NULL);
    conn_register_client_protocol ("2D_edit", PROTOCOL_VERSION, 1,
				   verify_edit_slave_connection,
				   register_edit_slave_connection,
				   read_edits_from_master,
				   register_master_loss);
}   /*  End Function initialise_iedit_package  */

static flag add_instruction (ilist, instruction)
/*  This routine will add a single edit instruction to a managed image edit
    instruction list. The  add_func  function registered for this instruction
    list will be called immediately. This function operates locally only.
    The instruction list must be given by  ilist  .
    The edit instruction must be pointed to by  instruction  .This instruction
    is subsequently deallocated by the routine.
    The routine returns TRUE on success, else it returns FALSE.
*/
KImageEditList ilist;
list_entry *instruction;
{
    extern packet_desc *instruction_desc;
    static char function_name[] = "add_instruction";

    VERIFY_ILIST (ilist);
    if (instruction == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    ds_list_append ( (*ilist).list_head, instruction );
    if ( (*ilist).process_add != NULL )
    {
	/*  Call registered callback  */
	(* (*ilist).process_add ) (ilist, instruction, &(*ilist).info);
    }
    return (TRUE);
}   /*  End Function add_instruction  */

static flag remove_instructions (ilist, num_to_remove)
/*  This routine will remove a number of edit instructions from a managed
    image edit instruction list. The  loss_func  function registered for this
    instruction list will be called immediately. This function operates locally
    only.
    The instruction list must be given by  ilist  .
    The number of instructions to remove must be given by  num_to_remove  .If
    this is 0, all instructions are removed.
    The routine returns TRUE on success, else it returns FALSE.
*/
KImageEditList ilist;
unsigned int num_to_remove;
{
    list_entry *prev_entry;
    list_entry *curr_entry;
    list_header *list_head;
    extern packet_desc *instruction_desc;
    static char function_name[] = "remove_instructions";

    VERIFY_ILIST (ilist);
    list_head = (*ilist).list_head;
    if (NULL == (*list_head).last_frag_entry)
    {
	return (TRUE);
    }
    if (num_to_remove == 0)
    {
	/*  Remove whole list  */
	ds_dealloc_list_entries (instruction_desc, list_head);
    }
    else
    {
	/*  Remove specified number of entries  */
	curr_entry = (*list_head).last_frag_entry;
	while ( (num_to_remove > 0) && (curr_entry != NULL) )
	{
	    --num_to_remove;
	    /*  Remove entry  */
	    prev_entry = (*curr_entry).prev;
	    (*list_head).last_frag_entry = prev_entry;
	    if (prev_entry != NULL)
	    {
		(*prev_entry).next = NULL;
	    }
	    ds_dealloc_data (instruction_desc, (*curr_entry).data);
	    m_free ( (char *) curr_entry );
	    curr_entry = prev_entry;
	    --(*list_head).length;
	}
	if ( (*list_head).length < 1 )
	{
	    (*list_head).first_frag_entry = NULL;
	}
    }
    if ( (*ilist).process_loss != NULL )
    {
	/*  Call registered callback  */
	(* (*ilist).process_loss ) (ilist, &(*ilist).info);
    }
    return (TRUE);
}   /*  End Function remove_instructions  */

static flag apply_instructions (ilist)
/*  This routine will issue an apply (commit) request for a managed image edit
    instruction list. The  apply_func  function registered for this
    instruction list will be called immediately, followed by the  loss_func
    registered. This function operates locally only.
    The instruction list must be given by  ilist  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KImageEditList ilist;
{
    Channel channel;
    Connection conn;
    unsigned int num_connections;
    unsigned int conn_count;
    extern packet_desc *instruction_desc;
    static char function_name[] = "apply_instructions";

    VERIFY_ILIST (ilist);
    /*  First apply  */
    if ( (*ilist).process_apply != NULL )
    {
	/*  Call registered callback  */
	(* (*ilist).process_apply ) (ilist, &(*ilist).info);
    }
    /*  Then remove  */
    ds_dealloc_list_entries (instruction_desc, (*ilist).list_head);
    if ( (*ilist).process_loss != NULL )
    {
	/*  Call registered callback  */
	(* (*ilist).process_loss ) (ilist, &(*ilist).info);
    }
    return (TRUE);
}   /*  End Function apply_instructions  */

static flag register_new_edit_slave (connection, info)
/*  This routine will register the opening of a connection from a 2D_edit slave
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on successful registration,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will not be called if this routine returns
    FALSE.
*/
Connection connection;
void **info;
{
    Channel channel;
    extern KImageEditList masterable_list;
    extern packet_desc *instruction_desc;
    static char function_name[] = "register_new_edit_slave";

    channel = conn_get_channel (connection);
    if (masterable_list == NULL)
    {
	(void) fprintf (stderr,
			"Connection attempt by 2D_edit client but we have no list!\n");
	dsrw_write_flag (channel, FALSE);
	(void) ch_flush (channel);
	return (FALSE);
    }
    if ( (*masterable_list).master != NULL )
    {
	(void) fprintf (stderr, "Default masterable list is a slave!\n");
	dsrw_write_flag (channel, FALSE);
	(void) ch_flush (channel);
	return (FALSE);
    }
    *info = (void *) masterable_list;
    dsrw_write_flag (channel, TRUE);
    /*  Write whatever is in list and flush  */
    return ( write_list (channel, instruction_desc,
			 (*masterable_list).list_head) );
}   /*  End Function register_new_edit_slave  */

static flag read_edits_from_slave (connection, info)
/*  This routine will read in edit instructions from the connection (to a
    slave) given by  connection and will write any appropriate information to
    the pointer pointed to by  info  .
    The routine returns TRUE on successful reading,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will be called if this routine returns FALSE
*/
Connection connection;
void **info;
{
    Channel channel;
    KImageEditList ilist;
    list_entry *entry;
    extern packet_desc *instruction_desc;
    static char function_name[] = "read_edits_from_slave";

    ilist = (KImageEditList) *info;
    VERIFY_ILIST (ilist);
    if ( ( entry = ds_alloc_list_entry (instruction_desc, TRUE) ) == NULL )
    {
	m_error_notify (function_name, "edit instruction");
	return (FALSE);
    }
    channel = conn_get_channel (connection);
    if (dsrw_read_packet (channel, instruction_desc, (*entry).data) != TRUE)
    {
	return (FALSE);
    }
    if (transmit_to_slaves (ilist, entry) != TRUE) return (FALSE);
    return ( process_local_instruction (ilist, entry) );
}   /*  End Function read_edits_from_slave  */

static flag verify_edit_slave_connection (info)
/*  This routine will validate whether it is appropriate to open a connection
    to an edit master.
    The routine will write any appropriate information to the pointer
    pointed to by  info  .The pointer value written here will be passed
    to the other routines.
    The routine returns TRUE if the connection should be attempted,
    else it returns FALSE (indicating the connection should be aborted).
    NOTE: Even if this routine is called and returns TRUE, there is no
    guarantee that the connection will be subsequently opened.
*/
void **info;
{
    list_header *list_head;
    extern KImageEditList slaveable_list;
    extern packet_desc *instruction_desc;
    static char function_name[] = "verify_edit_slave_connection";

    if (slaveable_list == NULL)
    {
	(void) fprintf (stderr,
			"Connection attempt to 2D_edit server but we have no list!\n");
	return (FALSE);
    }
    if ( (*slaveable_list).master != NULL )
    {
	(void) fprintf (stderr,
			"Connection attempt to 2D_edit server but already a slave!\n");
	return (FALSE);
    }
    list_head = (*slaveable_list).list_head;
    if ( (*list_head).length > 0 )
    {
	(void) fprintf (stderr,
			"Edit list must be empty before becomming a slave\n");
	return (FALSE);
    }
    *info = (void *) slaveable_list;
    return (TRUE);
}   /*  End Function verify_edit_slave_connection  */

static flag register_edit_slave_connection (connection, info)
/*  This routine will register the opening of a connection to an edit server.
    The connection will be given by  connection  .
    The routine will write any appropriate information to the pointer
    pointed to by  info  .
    The routine returns TRUE on successful registration,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will not be called if this routine returns
    FALSE.
*/
Connection connection;
void **info;
{
    KImageEditList ilist;
    Channel channel;
    flag success;
    static char function_name[] = "register_edit_slave_connection";

    ilist = (KImageEditList) *info;
    VERIFY_ILIST (ilist);
    if ( (*ilist).master != NULL )
    {
	(void) fprintf (stderr,
			"Connection attempt to 2D_edit server but suddenly a slave!\n");
	a_prog_bug (function_name);
    }
    channel = conn_get_channel (connection);
    if (dsrw_read_flag (channel, &success) != TRUE)
    {
	return (FALSE);
    }
    if (!success) return (FALSE);
    (*ilist).master = channel;
    return (TRUE);
}   /*  End Function register_edit_slave_connection  */

static flag read_edits_from_master (connection, info)
/*  This routine will read in data from the connection given by  connection
    and will write any appropriate information to the pointer pointed to by
    info  .
    The routine returns TRUE on successful reading,
    else it returns FALSE (indicating the connection should be closed).
    Note that the  close_func  will be called if this routine returns FALSE
*/
Connection connection;
void **info;
{
    Channel channel;
    KImageEditList ilist;
    list_entry *entry;
    extern packet_desc *instruction_desc;
    static char function_name[] = "read_edits_from_master";

    ilist = (KImageEditList) *info;
    VERIFY_ILIST (ilist);
    if ( ( entry = ds_alloc_list_entry (instruction_desc, TRUE) ) == NULL )
    {
	m_error_notify (function_name, "edit instruction");
	return (FALSE);
    }
    channel = conn_get_channel (connection);
    if (channel != (*ilist).master)
    {
	(void) fprintf (stderr, "Channel missmatch\n");
	a_prog_bug (function_name);
    }
    if (dsrw_read_packet (channel, instruction_desc, (*entry).data) != TRUE)
    {
	return (FALSE);
    }
    return ( process_local_instruction (ilist, entry) );
}   /*  End Function read_edits_from_master  */

static void register_master_loss (connection, info)
/*  This routine will register a closure of a connection.
    When this routine is called, this is the last chance to read any
    buffered data from the channel associated with the connection object.
    The connection will be given by  connection  .
    The connection information pointer will be given by  info  .
    The routine returns nothing.
*/
Connection connection;
void *info;
{
    KImageEditList ilist;
    static char function_name[] = "register_master_loss";

    ilist = (KImageEditList) info;
    VERIFY_ILIST (ilist);
    (*ilist).master = NULL;
}   /*  End Function register_master_loss  */

static flag process_local_instruction (ilist, instruction)
/*  This routine will add a single edit instruction to a managed image edit
    instruction list. The  add_func  function registered for this instruction
    list will be called at a future time with a copy of the instruction to be
    added.
    The instruction must be given by  instruction  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KImageEditList ilist;
list_entry *instruction;
{
    unsigned int instruction_code;
    double value[2];
    extern unsigned int edit_coord_list_index;
    extern packet_desc *instruction_desc;
    extern packet_desc *coord_list_desc;
    static char function_name[] = "process_local_instruction";

    VERIFY_ILIST (ilist);
    /*  Get instruction code  */
    if (ds_get_unique_named_value (instruction_desc, (*instruction).data,
				   "Edit Instruction",
				   (unsigned int *) NULL, value) != TRUE)
    {
	(void) fprintf (stderr, "Error getting edit instruction code\n");
	return (FALSE);
    }
    instruction_code = (unsigned int) value[0];
    /*  Get data value  */
    if (ds_get_unique_named_value (instruction_desc, (*instruction).data,
				   "Edit Object Value",
				   (unsigned int *) NULL, value) != TRUE)
    {
	(void) fprintf (stderr, "Error getting edit object value\n");
	return (FALSE);
    }
    /*  Do it locally  */
    switch (instruction_code)
    {
      case EDIT_UNDO_INSTRUCTIONS:
	return ( remove_instructions ( ilist,
				     (unsigned int) (value[0] + 0.01) ) );
/*
        break;
*/
      case EDIT_APPLY_INSTRUCTIONS:
	return ( apply_instructions (ilist) );
/*
        break;
*/
      default:
	return ( add_instruction (ilist, instruction) );
/*
        break;
	*/
    }
}   /*  End Function process_local_instruction  */

static flag transmit_to_slaves (ilist, instruction)
/*  This routine will transmit an edit instruction to all slaves of a managed
    edit instruction list.
    The edit list must be given by  ilist  .
    The edit instruction must be pointed to by  instruction  .
    The routine returns TRUE on success, else it returns FALSE.
*/
KImageEditList ilist;
list_entry *instruction;
{
    Channel channel;
    Connection conn;
    unsigned int num_connections;
    unsigned int conn_count;
    extern packet_desc *instruction_desc;
    static char function_name[] = "transmit_to_slaves";

    /*  Write to any possible slaves  */
    num_connections = conn_get_num_serv_connections ("2D_edit");
    for (conn_count = 0; conn_count < num_connections; ++conn_count)
    {
	if ( ( conn = conn_get_serv_connection ("2D_edit", conn_count) )
	    == NULL )
	{
	    (void) fprintf (stderr, "2D_edit connection: %u not found\n",
			    conn_count);
	    a_prog_bug (function_name);
	}
	if (conn_get_connection_info (conn) != ilist) continue;
	channel = conn_get_channel (conn);
	dsrw_write_packet (channel, instruction_desc, (*instruction).data);
	if (ch_flush (channel) != TRUE) return (FALSE);
    }
    return (TRUE);
}   /*  End Function transmit_to_slaves  */

static flag write_list (channel, list_desc, list_head)
/*  This routine will write all list entries in a linked list to a channel. The
    routine only recognises fragmented list entries.
    The channel must be given by  channel  .
    The packet descriptor for the linked list must be pointed to by  list_desc
    The linked list header must be pointed to by  list_head  .
    The routine returns TRUE on success, else it returns FALSE.
*/
Channel channel;
packet_desc *list_desc;
list_header *list_head;
{
    list_entry *entry;

    for (entry = (*list_head).first_frag_entry; entry != NULL;
	 entry = (*entry).next)
    {
	dsrw_write_packet (channel, list_desc, (*entry).data);
    }
    return ( ch_flush (channel) );
}   /*  End Function write_list  */

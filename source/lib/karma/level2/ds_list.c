/*LINTLIBRARY*/
/*PREFIX:"ds_"*/
/*  ds_list.c

    This code provides Karma data structure linked list manipulation routines.

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

    This file contains the various utility routines for manipulating the linked
    list component of the general data structure supported in Karma.


    Written by      Richard Gooch   5-APR-1993

    Last updated by Richard Gooch   7-APR-1993


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <os.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>


/*PUBLIC_FUNCTION*/
void ds_list_insert (list_head, new_entry, entry)
/*  This routine will insert an entry into the fragmented section of a linked
    list. 
    The list header must be pointed to by  list_head  .
    The entry to be inserted must be pointed to by  new_entry  .
    The list entry which is to be moved beyond the new entry must be pointed to
    by  entry  .If this is NULL, the new entry is inserted at the start of the
    fragmented section of the list.
    Inserting an entry into a list with no entries in the fragmented section is
    safe (however, in this case  entry  must be NULL).
    The routine returns nothing.
*/
list_header *list_head;
list_entry *new_entry;
list_entry *entry;
{
    static char function_name[] = "ds_list_insert";

    if (list_head == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list header pointer passed\n");
	a_prog_bug (function_name);
    }
    if (entry == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list entry pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (*list_head).magic != MAGIC_LIST_HEADER )
    {
	(void) fprintf (stderr, "List header has bad magic number\n");
	a_prog_bug (function_name);
    }
    if ( (entry != NULL) && ( (*list_head).first_frag_entry == NULL ) )
    {
	(void) fprintf (stderr,
			"No fragmented entries: reference entry must be bogus\n");
	a_prog_bug (function_name);
    }
    if ( (entry == NULL) || (entry == (*list_head).first_frag_entry) )
    {
	/*  Insert at beginning of fragmented section  */
	if ( (*list_head).first_frag_entry == NULL )
	{
	    /*  No other entries  */
	    (*list_head).first_frag_entry = new_entry;
	    (*list_head).last_frag_entry = new_entry;
	    (*new_entry).next = NULL;
	    (*new_entry).prev = NULL;
	    (*list_head).length++;
	    return;
	}
	/*  Displace first entry  */
	(*new_entry).next = (*list_head).first_frag_entry;
	(*list_head).first_frag_entry = new_entry;
	(*new_entry).prev = NULL;
	(*list_head).length++;
	return;
    }
    if ( (*entry).next == NULL )
    {
	/*  Insert at end of list (append)  */
        ds_list_append (list_head, new_entry);
        return;
    }
    /*  Insert somewhere in the middle  */
    (*new_entry).prev = (*entry).prev;
    (*new_entry).next = entry;
    (* (*entry).prev ).next = new_entry;
    (*entry).prev = new_entry;
    ++(*list_head).length;
}   /*  End Function ds_list_insert  */

/*PUBLIC_FUNCTION*/
void ds_list_append (list_head, entry)
/*  This routine will append an entry into a linked list.
    The list header must be pointed to by  list_head  .
    The entry must be pointed to by  entry  .
    The routine returns nothing.
*/
list_header *list_head;
list_entry *entry;
{
    static char function_name[] = "ds_list_append";

    if (list_head == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list header pointer passed\n");
	a_prog_bug (function_name);
    }
    if (entry == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list entry pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (*list_head).magic != MAGIC_LIST_HEADER )
    {
	(void) fprintf (stderr, "List header has bad magic number\n");
	a_prog_bug (function_name);
    }
    if ( (*list_head).first_frag_entry == NULL )
    {
	/*  Empty list  */
        (*list_head).first_frag_entry = entry;
        (*entry).prev = NULL;
    }
    else
    {
	/*  List not empty  */
        (* (*list_head).last_frag_entry ).next = entry;
        (*entry).prev = (*list_head).last_frag_entry;
    }
    (*list_head).last_frag_entry = entry;
    (*entry).next = NULL;
    (*list_head).length++;
}   /*  End Function ds_list_append  */

/*PUBLIC_FUNCTION*/
void ds_list_delete (list_desc, list_head, entry)
/*  This routine will delete an entry from the fragmented section of a linked
    list.
    The packet descriptor for the list entry data must be pointed to by
    list_desc  .If this is not NULL, the routine will recursively deallocate
    the packet data.
    The list header must be pointed to by  list_head  .
    The entry to be delete must be pointed to by  entry  .
    The routine will deallocate the list entry and it's data packet.
    The routine returns nothing.
*/
packet_desc *list_desc;
list_header *list_head;
list_entry *entry;
{
    static char function_name[] = "ds_list_delete";

    if (list_head == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list header pointer passed\n");
	a_prog_bug (function_name);
    }
    if (entry == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list entry pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (*list_head).magic != MAGIC_LIST_HEADER )
    {
	(void) fprintf (stderr, "List header has bad magic number\n");
	a_prog_bug (function_name);
    }
    if ( (entry != NULL) && ( (*list_head).first_frag_entry == NULL ) )
    {
	(void) fprintf (stderr,
			"No fragmented entries: entry must be bogus\n");
	a_prog_bug (function_name);
    }
    if ( (*entry).next == NULL )
    {
	/*  There is no next entry  */
	(*list_head).last_frag_entry = (*entry).prev;
    }
    else
    {
	/*  There is a next entry  */
	(* (*entry).next ).prev = (*entry).prev;
    }
    if ( (*entry).prev == NULL )
    {
	/*  There is no previous entry  */
	(*list_head).first_frag_entry = (*entry).next;
    }
    else
    {
	/*  There is a previous entry  */
	(* (*entry).prev ).next = (*entry).next;
    }
    /*  Free entry  */
    if (list_desc == NULL)
    {
	/*  No packet descriptor: deallocate blindly  */
	m_free ( (*entry).data );
    }
    else
    {
	ds_dealloc_data (list_desc, (*entry).data);
    }
    m_free ( (char *) entry );
    --(*list_head).length;
}   /*  End Function ds_list_delete  */

/*PUBLIC_FUNCTION*/
flag ds_list_unfragment (list_desc, list_head)
/*  This routine will unfragment a linked list (ie. all the entries and data
    packets in the linked list will be made contiguous in memory. This
    increases the storage efficiency (no  list_entry  structures are needed to
    link the data packets) as well as arbitrary indexing.
    The packet descriptor for the list must be pointed to by  list_desc  .
    The  contiguous_length  value in the linked list header pointed to by
    list_head  will be set to the length of the list.
    This routine, used with a sorting routine (in either order of execution),
    can be used to speed up searching algorithms, such as  ds_list_search  .
    If the routine is not successful in allocating the required memory, then
    no change is effected (and the  contiguous_length  value in the list header
    is not changed). Ie. data is not lost.
    The routine returns TRUE on success, else it returns FALSE.
*/
packet_desc *list_desc;
list_header *list_head;
{
    unsigned int pack_size;
    unsigned int length;
    char *data;
    char *ptr;
    list_entry *curr_entry;
    static char function_name[] = "ds_list_unfragment";

    if (list_desc == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if (list_head == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list header pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (*list_head).magic != MAGIC_LIST_HEADER )
    {
	(void) fprintf (stderr, "List header has bad magic number\n");
	a_prog_bug (function_name);
    }
    if ( (*list_head).contiguous_length == (*list_head).length )
    {
	/*  List is already contiguous  */
        return (TRUE);
    }
    if ( (*list_head).length < 1 )
    {
	/*  No need to make list contiguous  */
        return (TRUE);
    }
    length = (*list_head).length;
    pack_size = ds_get_packet_size (list_desc);
    /*  Allocate a contiguous block of list data packets  */
    if ( ( data = m_alloc (pack_size * (*list_head).length) )
	== NULL )
    {
	m_error_notify (function_name, "linked list data");
        return (FALSE);
    }
    /*  Copy contiguous data packets  */
    if ( (*list_head).contiguous_length > 0 )
    {
	m_copy (data, (*list_head).contiguous_data,
		pack_size * (*list_head).contiguous_length);
    }
    ptr = data + pack_size * (*list_head).contiguous_length;
    for (curr_entry = (*list_head).first_frag_entry; curr_entry != NULL;
	 curr_entry = (*curr_entry).next, ptr += pack_size)
    {
	/*  Another list entry to copy data for  */
	m_copy (ptr, (*curr_entry).data, pack_size);
    }
    /*  Deallocate old list  */
    ds_dealloc2_list (list_head);
    /*  Allocate new data  */
    (*list_head).length = length;
    (*list_head).contiguous_length = length;
    (*list_head).contiguous_data = data;
    return (TRUE);
}   /*  End Function ds_list_unfragment  */

/*PUBLIC_FUNCTION*/
flag ds_list_fragment (list_desc, list_head)
/*  This routine will fragment a linked list (ie. all the contiguous data
    packets in the linked list will be separately allocated and linked together
    by new  list_entry  structures. This decreases the storage efficiency (now
    list_entry  structures are needed to link the data packets) as well as
    arbitrary indexing. However, it does provide for easy insertion of new
    entries into any part of the list.
    The packet descriptor for the list must be pointed to by  list_desc  .
    The  contiguous_length  value in the linked list header pointed to by
    list_head  will be set to 0.
    If the routine is not successful in allocating the required memory, then
    no change is effected (and the  contiguous_length  value in the list header
    is not changed). Ie. data is not lost.
    The routine returns TRUE on success, else it returns FALSE.
*/
packet_desc *list_desc;
list_header *list_head;
{
    unsigned int pack_size;
    unsigned int count;
    char *data;
    list_entry *first_entry = NULL;
    list_entry *curr_entry;
    list_entry *new_entry;
    static char function_name[] = "ds_list_fragment";

    if (list_desc == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if (list_head == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list header pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (*list_head).magic != MAGIC_LIST_HEADER )
    {
	(void) fprintf (stderr, "List header has bad magic number\n");
	a_prog_bug (function_name);
    }
    if ( (*list_head).contiguous_length < 1 )
    {
	/*  List is already fragmented  */
        return (TRUE);
    }
    pack_size = ds_get_packet_size (list_desc);
    /*  Allocate  list_entry  structures and data packets  */
    for (count = 0, curr_entry = NULL, data = (*list_head).contiguous_data;
	 count < (*list_head).contiguous_length; ++count, data += pack_size)
    {
	/*  Allocate entry  */
	if ( ( new_entry = (list_entry *) m_alloc (sizeof *new_entry) )
	    == NULL )
	{
	    m_error_notify (function_name, "list entry structure");
	    /*  Free up entries already allocated  */
	    for (curr_entry = first_entry; curr_entry != NULL;
		 curr_entry = (*curr_entry).next)
	    {
		if ( (*curr_entry).data != NULL )
		{
		    m_free ( (*curr_entry).data );
		}
		m_free ( (char *) curr_entry );
	    }
	    return (FALSE);
	}
	/*  Set forward and backward pointers  */
	(*new_entry).prev = curr_entry;
	(*new_entry).next = NULL;
	/*  Allocate data packet  */
	if ( ( (*new_entry).data = m_alloc (pack_size) ) == NULL )
	{
	    m_error_notify (function_name, "list entry data packet");
	    /*  Free up entries and packets already allocated  */
	    for (curr_entry = first_entry; curr_entry != NULL;
		 curr_entry = (*curr_entry).next)
	    {
		if ( (*curr_entry).data != NULL )
		{
		    m_free ( (*curr_entry).data );
		}
		m_free ( (char *) curr_entry );
	    }
	    return (FALSE);
	}
	/*  Copy that data  */
	m_copy ( (*new_entry).data, data, pack_size );
	/*  Remember this entry for the next one that is allocated  */
	curr_entry = new_entry;
    }
    /*  Free up contiguous data packets  */
    m_free ( (*list_head).contiguous_data );
    (*list_head).contiguous_data = NULL;
    (*list_head).contiguous_length = 0;
    if ( (*list_head).first_frag_entry == NULL )
    {
	/*  No existing fragmented section of list: create  */
	(*list_head).last_frag_entry = curr_entry;
    }
    else
    {
	/*  Have existing fragmented section of list: insert  */
	(* (*list_head).first_frag_entry ).prev = first_entry;
    }
    (*first_entry).next = (*list_head).first_frag_entry;
    (*list_head).first_frag_entry = first_entry;
    return (TRUE);
}   /*  End Function ds_list_fragment  */

#ifdef dummy
/*PUBLIC _FUNCTION*/
void ds_list_sort (list_desc, list_head, elem_num, increasing)
/*  This routine will sort a linked list of data. The descriptor for the linked
    list must be pointed to by  list_desc  and the header to the linked
    list must be pointed to by  list_head  .
    The number of the data element (in the linked list data packets) to be
    sorted by must be in  elem_num  .
    The list will be sorted in increasing order if  increasing  is TRUE, else
    it will be sorted in decreasing order.
    Sorting of the following data element types is not permitted, and the
    routine exits gracefully:
        ARRAYP, LISTP.
    Sorting of complex data types is done by the absolute value of the data.
    The routine returns nothing.
*/
packet_desc *list_desc;
list_header *list_head;
unsigned int elem_num;
flag increasing;
{
    unsigned int byte_offset;
    unsigned int count = 0;
    unsigned int finish_elem;
    unsigned int elem_type;
    unsigned int pack_size;
    double value1;
    double value2;
    char *temp;
    flag sorted = FALSE;
    list_entry *entry1;
    list_entry *entry2;
    static char function_name[] = "ds_list_sort";

    if (list_desc == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if (list_head == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list header pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (*list_head).magic != MAGIC_LIST_HEADER )
    {
	(void) fprintf (stderr, "List header has bad magic number\n");
	a_prog_bug (function_name);
    }
    if (elem_num >= (*list_desc).num_elements)
    {
	/*  Specified element number does not exist  */
        return;
    }
    if ( (increasing != TRUE) && (increasing != FALSE) )
    {
	/*  Bad  increasing  parameter given  */
        return;
    }
    elem_type = (*list_desc).element_types[elem_num];
    if (ds_element_is_atomic (elem_type) != TRUE)
    {
	return;
    }
    if ( ( ( ( (*list_head).sort_type == SORT_INCREASING ) &&
            (increasing == TRUE) ) ||
	  ( ( (*list_head).sort_type == SORT_DECREASING ) &&
	   (increasing == FALSE) ) ) &&
        ( (*list_head).sort_elem_num == elem_num ) )
    {
	/*  The list is already sorted the way the caller wants  */
        return;
    }
    if ( (*list_head).length == 0 )
    {
	/*  No list to sort  */
        return;
    }
    /*  Set sort type and sort element number  */
    if (increasing == TRUE)
    {
	(*list_head).sort_type = SORT_INCREASING;
    }
    else
    {
	(*list_head).sort_type = SORT_DECREASING;
    }
    (*list_head).sort_elem_num = elem_num;
    if ( (*list_head).length == 1 )
    {
	/*  No need to sort list */
        return;
    }
    /*  List must be sorted  */
    /*  Allocate temporary copying memory  */
    pack_size = ds_get_packet_size (list_desc);
    if ( ( temp = m_alloc (pack_size) ) == NULL )
    {
	m_error_notify (function_name, "temporary copying memory");
	return;
    }
    /*  Find offset of element in data packet  */
    byte_offset = ds_get_element_offset (list_desc, elem_num);
    finish_elem = (*list_head).length;
    while ( (finish_elem > 0) && (sorted == FALSE) )
    {
	/*  List needs to be passed over again  */
        count = 0;
        entry1 = (*list_head).list_start;
        sorted = TRUE;
        while (count < finish_elem - 1)
        {
	    entry2 = (*entry1).next;
            value1 = convert_atomic ( (*entry1).data + byte_offset, elem_type);
            value2 = convert_atomic ( (*entry2).data + byte_offset, elem_type);
            if ( ( (increasing == TRUE) && (value1 > value2) ) ||
                ( (increasing == FALSE) && (value2 > value1) ) )
            {
		/*  Must swap  */
		m_copy (temp, (*entry1).data, pack_size);
		m_copy ( (*entry1).data, (*entry2).data, pack_size );
		m_copy ( (*entry2).data, temp, pack_size );
                sorted = FALSE;
            }
            ++count;
            entry1 = entry2;
        }
        --finish_elem;
    }
    /*  Deallocate temporary copying memory  */
    m_free (temp);
}   /*  End Function ds_list_sort  */

/*PUBLIC_ FUNCTION*/
unsigned int ds_list_search (list_desc, list_head, elem_num, compare_value)
/*  This routine will search a linked list for a value pointed to by
    compare_value  .The number of the element in the linked list data packets
    must be in  elem_num  .
    The linked list descriptor and header must be pointed to by
    list_desc  and  list_head  ,respectively.
    The following data element types cannot be searched for, and the routine
    exits gracefully:
        NONE, ARRAYP, LISTP, MULTI_ARRAY.
    The routine returns the entry number in the linked list where the value
    was found in the data packet. If a match was not found, the return value
    equals the length of the linked list.
*/
packet_desc *list_desc;
list_header *list_head;
unsigned int elem_num;
char *compare_value;
{
    unsigned int count = 0;
    unsigned int found_elem_num;
    unsigned int byte_offset;
    unsigned int datum_size;
    char *compare1;
    char *compare2;
    flag same;
    list_entry *entry;
    extern char host_type_sizes[NUMTYPES];
    static char function_name[] = "ds_list_search";

    found_elem_num = (*list_head).length;
    if (list_desc == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list descriptor pointer passed\n");
	a_prog_bug (function_name);
    }
    if (list_head == NULL)
    {
	/*  No list!  */
	(void) fprintf (stderr, "NULL list header pointer passed\n");
	a_prog_bug (function_name);
    }
    if ( (*list_desc).num_elements < 1 )
    {
	/*  Empty data packets  */
        return (found_elem_num);
    }
    if ( (*list_head).magic != MAGIC_LIST_HEADER )
    {
	(void) fprintf (stderr, "List header has bad magic number\n");
	a_prog_bug (function_name);
    }
    if ( (*list_head).length < 1 )
    {
	/*  Empty list  */
        return (found_elem_num);
    }
    if (elem_num >= (*list_desc).num_elements)
    {
	/*  Nonexistent element number  */
        return (found_elem_num);
    }
    if (ds_element_is_atomic ( (*list_desc).element_types[elem_num] )
	!= TRUE)
    {
	/*  Bad data type   */
	return (found_elem_num);
    }
    byte_offset = ds_get_element_offset (list_desc, elem_num);
    datum_size = host_type_sizes[ (*list_desc).element_types[elem_num] ];
    entry = (*list_head).list_start;
    while (entry != NULL)
    {
	count = 0;
        compare1 = compare_value;
        compare2 = (*entry).data + byte_offset;
        same = TRUE;
        while ( (same == TRUE) && (count < datum_size) )
        {
	    if (*compare1++ != *compare2++)
	    same = FALSE;
            ++count;
        }
        if (same == TRUE)
        {
	    /*  Found match */
            return (found_elem_num);
        }
        /*  Did not find match: go to the next entry    */
        entry = (*entry).next;
        ++found_elem_num;
    }
    return (found_elem_num);
}   /*  End Function ds_list_search    */
#endif

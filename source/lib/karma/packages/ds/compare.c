/*LINTLIBRARY*/
/*  find.c

    This code provides routines to compate data structures.

    Copyright (C) 1992-1996  Richard Gooch

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

    This file contains the various utility routines for comparing
    general data structure supported in Karma.


    Written by      Richard Gooch   1-NOV-1996: Extracted from get.c

    Last updated by Richard Gooch   1-NOV-1996


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>
#include <os.h>


/*PUBLIC_FUNCTION*/
flag ds_compare_packet_desc (CONST packet_desc *desc1,CONST packet_desc *desc2,
			     flag recursive)
/*  [SUMMARY] Recursively compare two packet descriptors.
    <desc1> One of the packet descriptors.
    <desc2> The other packet descriptor.
    <recursive> If TRUE the routine will perform a recursive comparison of
    sub-arrays and linked list descriptors.
    [RETURNS] TRUE if the two packet descriptors are equal, else FALSE.
*/
{
    unsigned int elem_count = 0;
    unsigned int elem_type1;
    unsigned int elem_type2;
    char *elem_name1;
    char *elem_name2;
    static char function_name[] = "ds_compare_packet_desc";

    if ( (desc1 == NULL) || (desc2 == NULL) )
    {
	a_func_abort (function_name, "NULL descriptor pointer(s)");
        return (FALSE);
    }
    if (desc1->num_elements != desc2->num_elements)
    {
	return (FALSE);
    }
    while (elem_count < desc1->num_elements)
    {
	elem_type1 = desc1->element_types[elem_count];
        elem_type2 = desc2->element_types[elem_count];
        elem_name1 = desc1->element_desc[elem_count];
        elem_name2 = desc2->element_desc[elem_count];
        if (elem_type1 == elem_type2)
        {
	    /*  Element types are the same  */
            if ( ds_element_is_named (elem_type1) )
            {
		if (strcmp (elem_name1, elem_name2) != 0)
                    return (FALSE);
            }
            else
            {
		if ( recursive && (elem_type1 == K_ARRAY) )
                {
		    if ( !ds_compare_array_desc ( (array_desc *)
						  elem_name1,
						  (array_desc *)
						  elem_name2,
						  recursive) )
                        return (FALSE);
                }
                if ( recursive &&(elem_type1 == LISTP) )
                {
		    if ( !ds_compare_packet_desc ( (packet_desc *)
						   elem_name1,
						   (packet_desc *)
						   elem_name2,
						   recursive) )
                        return (FALSE);             
                }
            }
        }
        else
        {
	    /*  Element types are not the same  */
            /*  Trap for bad element types  */
            if ( !ds_element_is_legal (elem_type1) )
            {
		(void) fprintf (stderr, "Element type: %u is not legal\n",
				elem_type1);
                a_prog_bug (function_name);
            }
            if ( !ds_element_is_legal (elem_type2) )
            {
		(void) fprintf (stderr, "Element type: %u is not legal\n",
				elem_type2);
                a_prog_bug (function_name);
            }
	    return (FALSE);
        }
        ++elem_count;
    }
    return (TRUE);
}   /*  End Function ds_compare_packet_desc  */

/*PUBLIC_FUNCTION*/
flag ds_compare_array_desc (CONST array_desc *desc1, CONST array_desc *desc2,
			    flag recursive)
/*  [SUMMARY] Recursively compare two array descriptors.
    <desc1> One of the array descriptors.
    <desc2> The other array descriptor.
    <recursive> If TRUE the routine will perform a recursive comparison of the
    array packet descriptors.
    [RETURNS] TRUE if the two array descriptors are equal, else FALSE.
*/
{
    unsigned int dim_count = 0;
    static char function_name[] = "ds_compare_array_desc";

    if ( (desc1 == NULL) || (desc2 == NULL) )
    {
	a_func_abort (function_name, "NULL descriptor pointer(s)");
        return (FALSE);
    }
    if (desc1->num_dimensions != desc2->num_dimensions)
    {
	return (FALSE);
    }
    while (dim_count < desc1->num_dimensions)
    {
	if ( !ds_compare_dim_desc (desc1->dimensions[dim_count],
				   desc2->dimensions[dim_count]) )
	return (FALSE);
        ++dim_count;
    }
    if (recursive)
    {
	if ( !ds_compare_packet_desc (desc1->packet, desc2->packet,recursive) )
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function ds_compare_array_desc  */

/*PUBLIC_FUNCTION*/
flag ds_compare_dim_desc (CONST dim_desc *desc1, CONST dim_desc *desc2)
/*  [SUMMARY] Compare two dimension descriptors.
    <desc1> One of the dimension descriptors.
    <desc2> The other dimension descriptor.
    [RETURNS] TRUE if the two dimension descriptors are equal, else FALSE.
*/
{
    unsigned int coord_count = 0;
    static char function_name[] = "ds_compare_dim_desc";

    if ( (desc1 == NULL) || (desc2 == NULL) )
    {
	a_func_abort (function_name, "NULL descriptor pointer(s)");
        return (FALSE);
    }
    if ( (desc1->name == NULL) || (desc2->name == NULL) )
    {
	(void) fprintf (stderr, "Dimension name is a NULL pointer\n");
        a_prog_bug (function_name);
    }
    if (strcmp (desc1->name, desc2->name) != 0)
    return (FALSE);
    if (desc1->length != desc2->length) return (FALSE);
    if (desc1->first_coord != desc2->first_coord) return (FALSE);
    if (desc1->last_coord != desc2->last_coord) return (FALSE);
    if (desc1->coordinates == NULL)
    {
	if (desc2->coordinates != NULL)
	return (FALSE);
    }
    else
    {
	if (desc2->coordinates == NULL)
	return (FALSE);
        while (coord_count < desc1->length)
        {
	    if (desc1->coordinates[coord_count] !=
                desc2->coordinates[coord_count])
	    return (FALSE);
            ++coord_count;
        }
    }
    return (TRUE);
}   /*  End Function ds_compare_dim_desc  */

/*LINTLIBRARY*/
/*  history.c

    This code provides routines to manipulate Karma data structurehistory info

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

/*

    This file contains the various utility routines for manipulating history
    information in the Karma general data structure.


    Written by      Richard Gooch   6-AUG-1996

    Last updated by Richard Gooch   28-SEP-1996: Created <ds_history_copy>.


*/

#include <stdio.h>
#include <math.h>
#include <karma.h>
#include <karma_ds.h>
#include <karma_a.h>
#include <karma_m.h>


/*PUBLIC_FUNCTION*/
flag ds_history_append_string (multi_array *multi_desc, CONST char *string,
			       flag new_alloc)
/*  [SUMMARY] Add a history string to a Karma data structure.
    <multi_desc> The multi_array descriptor.
    <string> The history string to add.
    <new_alloc> If TRUE, the routine will allocate a new copy of the history
    string, else it will copy the pointer (in which case the string must never
    be externally deallocated or changed).
    [RETURNS] TRUE on success, else FALSE.
*/
{
    history *new;
    static char function_name[] = "ds_history_append_string";

    if (multi_desc == NULL)
    {
	fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    if (string == NULL) return (TRUE);
    if ( ( new = (history *) m_alloc (sizeof *new) ) == NULL )
    {
	m_error_notify (function_name, "history entry");
	return (FALSE);
    }
    /*  Copy string information  */
    if (new_alloc)
    {
	if ( ( new->string = m_dup (string, strlen (string) + 1) ) == NULL )
	{
	    m_error_notify (function_name, "history string");
	    m_free ( (char *) new );
	    return (FALSE);
	}
    }
    else new->string = (char *) string;
    new->next = NULL;
    if (multi_desc->first_hist == NULL) multi_desc->first_hist = new;
    else multi_desc->last_hist->next = new;
    multi_desc->last_hist = new;
    return (TRUE);
}   /*  End Function ds_history_append_string  */

/*PUBLIC_FUNCTION*/
flag ds_history_copy (multi_array *out, CONST multi_array *in)
/*  [SUMMARY] Copy history information.
    <out> The output multi_array descriptor to copy the history to.
    <in> The input multi_array descriptor to copy the history from.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    history *curr;

    for (curr = in->first_hist; curr != NULL; curr = curr->next)
    {
	if ( !ds_history_append_string (out, curr->string,TRUE) ) return FALSE;
    }
    return (TRUE);
}   /*  End Function ds_history_copy  */

/*LINTLIBRARY*/
/*  misc.c

    This code provides a high level data structure processing routine.

    Copyright (C) 1992,1993,1994,1995  Richard Gooch

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

/*  This file contains all routines needed for the simple processing of data
  structures.


    Written by      Richard Gooch   16-OCT-1992

    Updated by      Richard Gooch   26-NOV-1992

    Updated by      Richard Gooch   2-JAN-1993: Took acccount of change to
  ds_alloc_data

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   27-MAR-1993: Added explicit control for
  memory mapping to  dsproc_object  by adding parameter.

    Updated by      Richard Gooch   17-MAY-1993: Took account of change to
  dsxfr_get_multi  .

    Updated by      Richard Gooch   4-AUG-1993: Changed call to  sprintf(3)  to
  calls to  strcpy  and  strcat  in order to avoid possible VX/MVX problems.

    Updated by      Richard Gooch   8-AUG-1993: Made explicit reference to
  dsxfr_get_multi  ,corrected documentation on the  post_process  routine and
  implemented its operation.

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/dsproc/main.c

    Updated by      Richard Gooch   19-APR-1995: Cleaned some code.

    Last updated by Richard Gooch   13-JUL-1995: Cleaned some more code.


*/
#include <stdio.h>
#include <string.h>
#include <karma.h>
#include <karma_dsproc.h>
#include <karma_dsxfr.h>
#include <karma_ds.h>
#include <karma_m.h>
#include <karma_a.h>


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void dsproc_object (char *object, char *array_names[], unsigned int num_arrays,
		    flag save_unproc_data, flag (*pre_process) (),
		    flag (*process_array) (), flag (*post_process) (),
		    unsigned int mmap_option)
/*  This routine will process the Karma array in the file which has a name
    pointed to by  object  .This parameter is passed directly to the
    dsxfr_get_multi  routine.
    If the arrayfile name is "connection" or is of the form: "connection[#]"
    then the routine will attempt to find data sent from a network connection
    using the  multi_array  protocol and will also send the resultant
    information down any network connections.
    The various arrays in the multi array general data structure are selected
    and processed by using various calls.
    The array names which are to be processed must be pointed to by
    array_names  and the number of these names must be in  num_arrays  .
    If  num_arrays  is zero, all array names are matched.
    If the flag  save_unproc_data  is TRUE, then arrays with names which
    do not match are copied, rather than ignored.
    All memory allocation and error handling required are performed.
    The output multi_array data structure produced is deallocated once the data
    is written.
    The specific processing functions required in the module must be passed to
    this routine. The interface to these functions are:

    flag pre_process (multi_desc)
    *	This routine will pre process the multi array general data
	structure pointed to by  multi_desc  .
	The routine returns TRUE if processing is to continue, else it
	returns FALSE.
    *
    multi_array *multi_desc;
    {}

    flag process_array (inp_desc, inp_data, out_desc, out_data)
    *	This routine will process the input data with descriptor and data
	pointed to by  inp_desc  and  inp_data  respectively.
	The output descriptor and data pointers are written to the storage
	pointed to by  out_desc  and  out_data  respectively.
	The routine returns TRUE on success, else it returns FALSE.
    *
    packet_desc *inp_desc;
    char *inp_data;
    packet_desc **out_desc;
    char **out_data;
    {}

    flag post_process (inp_desc, out_desc)
    *	This routine will perform post processing on the multi array
	pointed to by  out_desc  .This routine is only called for disc
	arrayfiles.
	The input multi array must be pointed to by  inp_desc  .
	This routine will usually add history information and copy over
	history information from the input array.
	The routine returns TRUE if the array is to be saved, else it
	returns FALSE.
    *
    multi_array *inp_desc;
    multi_array *out_desc;
    {}

    The  mmap_option  parameter is passed directly to the  dsxfr_get_multi
    routine. In addition, the  cache  parameter for  dsxfr_get_multi  is set to
    TRUE.
    If the input data structure is likely to be modified, the value of
    mmap_option must be K_CH_MAP_NEVER, otherwise the data may be read-only
    memory mapped and writing to it will cause a segmentation fault.
    The routine returns nothing.
*/
{
    unsigned int array_count = 0;
    unsigned int *index_list;
    multi_array *multi_desc;
    multi_array *out_multi_desc;
    char txt[STRING_LENGTH];
    extern char module_name[STRING_LENGTH + 1];
    static char function_name[] = "dsproc_object";

    if ( ( multi_desc = dsxfr_get_multi (object, TRUE, mmap_option, FALSE) )
	== NULL )
    {
	return;
    }
    /*  Determine suitability of input array  */
    if ( !(*pre_process) (multi_desc)  )
    {
	return;
    }
    /*  Create output object name. Don't use  sprintf(3)  because it's broken
	in the C library for the VX/MVX (although I don't recall having
	problems with this particular invocation), and besides, it results in
	a Host Procedure Call (ie. it's slow).
	*/
    (void) strcpy (txt, module_name);
    (void) strcat (txt, "_");
    (void) strcat (txt, object);
    if ( ( out_multi_desc = ds_select_arrays (array_names, num_arrays,
					      multi_desc,
					      save_unproc_data, &index_list) )
	== NULL )
    {
	(void) fprintf (stderr, "Error selecting arrays during function: %s\n",
			function_name);
	return;
    }
    /*  Process arrays  */
    while (array_count < out_multi_desc->num_arrays)
    {
	if (index_list[array_count] < multi_desc->num_arrays)
	{
	    /*  Name match: process array  */
	    if ( !(*process_array)
		( multi_desc->headers[index_list[array_count]],
		 multi_desc->data[ index_list[array_count] ],
		 &out_multi_desc->headers[array_count],
		 &out_multi_desc->data[array_count] ) )
	    {
		if ( out_multi_desc->num_arrays < 2 )
		{
		    (void) fprintf (stderr,
				    "Error processing array_file: %s\n",
				    object);
		}
		else
		{
		    (void) fprintf (stderr,
				    "Error processing array: %s of array_file: %s\n",
				    out_multi_desc->array_names[array_count],
				    object);
		}
		(void) fprintf (stderr, "Function: %s error\n", function_name);
		ds_dealloc_multi (out_multi_desc);
		m_free ( (char *) index_list );
		return;
	    }
	}
	else
	{   /* No name match: copy over input array */
	    if ( ( out_multi_desc->headers[array_count] =
		  ds_copy_desc_until ( multi_desc->headers[array_count],
				      (char *) NULL ) )
		== NULL )
	    {
		a_func_abort (function_name,
			      "Error copying packet descriptor");
		ds_dealloc_multi (out_multi_desc);
		m_free ( (char *) index_list );
		return;
	    }
	    /*  Allocate data space for array  */
	    if ( ( out_multi_desc->data[array_count] =
		  ds_alloc_data ( out_multi_desc->headers[array_count],
				 TRUE, TRUE ) )
		== NULL )
	    {
		m_error_notify (function_name, "unprocessed array");
		ds_dealloc_multi (out_multi_desc);
		m_free ( (char *) index_list );
		return;
	    }
	    /*  Copy data  */
	    if (ds_copy_data ( multi_desc->headers[array_count] ,
			      multi_desc->data[array_count],
			      out_multi_desc->headers[array_count],
			      out_multi_desc->data[array_count]) == FALSE)
	    {
		a_func_abort (function_name,
			      "Error copying over data for unprocessed array");
		ds_dealloc_multi (out_multi_desc);
		m_free ( (char *) index_list );
		return;
	    }
	}
	++array_count;
    }
    m_free ( (char *) index_list );
    /*  Finished processing  */
    /*  Decrement attachment count for input data structure  */
    ds_dealloc_multi (multi_desc);
    /*  Determine suitability of output array  */
    if ( !(*post_process) (multi_desc, out_multi_desc)  )
    {
	ds_dealloc_multi (out_multi_desc);
	return;
    }
    if ( !dsxfr_put_multi (txt, out_multi_desc) )
    {
	ds_dealloc_multi (out_multi_desc);
	return;
    }
    /*  Everything is fine: deallocate output now that it is saved  */
    ds_dealloc_multi (out_multi_desc);
}   /*  End Function dsproc_object  */

/*  karma_dsxfr.h

    Header for  dsxfr_  package.

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

    This include file contains all the definitions and function declarations
  needed to interface to the dsxfr_ routines in the Karma library.


    Written by      Richard Gooch   19-SEP-1992

    Last updated by Richard Gooch   28-MAY-1993

*/

#ifndef KARMA_DSXFR_H
#define KARMA_DSXFR_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#include <karma_ds_def.h>
#include <karma_ch.h>


/*  File:   dsxfr.c   */
EXTERN_FUNCTION (void dsxfr_register_connection_limits,
		 (int max_incoming, int max_outgoing) );
EXTERN_FUNCTION (flag dsxfr_put_multi, (char *object,
					multi_array *multi_desc) );
EXTERN_FUNCTION (multi_array *dsxfr_get_multi, (char *object, flag cache,
						unsigned int mmap_option,
						flag writeable) );
EXTERN_FUNCTION (void dsxfr_register_read_func,
		 ( void (*read_func) (flag first_time_data) ) );
EXTERN_FUNCTION (void dsxfr_register_close_func,
		 ( void (*close_func) (flag data_deallocated) ) );


#endif /*  KARMA_DSXFR_H  */

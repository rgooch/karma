/*  karma_vc.h

    Header for  vc_  package.

    Copyright (C) 1993  Richard Gooch

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
  needed to interface to the vc_ routines in the Karma library.


    Written by      Richard Gooch   5-MAR-1993

    Last updated by Richard Gooch   7-MAR-1993

*/

#ifndef KARMA_VC_H
#define KARMA_VC_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#ifndef KARMA_H
#  include <karma.h>
#endif

#ifndef KDISPLAY_DEFINED
#define KDISPLAY_DEFINED
typedef void * Kdisplay;
#endif


/*  File:   vc.c   */
EXTERN_FUNCTION (Kdisplay vc_get_dpy_handle, () );
EXTERN_FUNCTION (unsigned int vc_alloc_colours, (unsigned int num_cells,
						 unsigned long *pixel_values,
						 unsigned int min_cells,
						 Kdisplay dpy_handle) );
EXTERN_FUNCTION (void vc_free_colours, (unsigned int num_cells,
					unsigned long *pixel_values,
					Kdisplay dpy_handle) );
EXTERN_FUNCTION (void vc_store_colours, (unsigned int num_cells,
					 unsigned long *pixel_values,
					 unsigned short *reds,
					 unsigned short *greens,
					 unsigned short *blues,
					 unsigned int stride,
					 Kdisplay dpy_handle) );
EXTERN_FUNCTION (void vc_get_location, (Kdisplay dpy_handle,
					unsigned long *serv_hostaddr,
					unsigned long *serv_display_num) );
EXTERN_FUNCTION (void vc_set_visual, (Kdisplay dpy_handle,
				      flag pseudo_colour) );


#endif /*  KARMA_VC_H  */

/*  karma_c.h

    Header for  c_  package.

    Copyright (C) 1994  Richard Gooch

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
  needed to interface to the c_ routines in the Karma library.


    Written by      Richard Gooch   27-NOV-1994

    Last updated by Richard Gooch   28-NOV-1994

*/

#ifndef KARMA_C_H
#define KARMA_C_H


#ifndef KARMA_H
#  include <karma.h>
#endif

#ifndef KARMA_C_DEF_H
#  include <karma_c_def.h>
#endif


/*  File:  main.c  */
EXTERN_FUNCTION (KCallbackFunc c_register_callback,
		 (KCallbackList *list, flag (*callback) (),
		  void *object, void *client1_data, flag client1_indirect,
		  void *client2_data, flag client2_indirect,flag quenchable) );
EXTERN_FUNCTION (void c_unregister_callback, (KCallbackFunc callback) );
EXTERN_FUNCTION (flag c_call_callbacks, (KCallbackList list,void *call_data) );
EXTERN_FUNCTION (void c_destroy_list, (KCallbackList list) );


#endif /*  KARMA_C_H  */

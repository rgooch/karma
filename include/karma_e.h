/*  karma_e.h

    Header for  e_  package.

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

    This include file contains all the definitions and function declarations
  needed to interface to the e_ routines in the Karma library.


    Written by      Richard Gooch   30-NOV-1996

    Last updated by Richard Gooch   1-DEC-1996

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#if !defined(KARMA_E_DEF_H) || defined(MAKEDEPEND)
#  include <karma_e_def.h>
#endif

#ifndef KARMA_E_H
#define KARMA_E_H


#define DISPATCH_SYNCHRONOUS  0
#define DISPATCH_ASYNCHRONOUS 1


/*  File:  generic.c  */
EXTERN_FUNCTION (KPeriodicEventList e_create_list,
		 (flag (*start) (), void (*stop) (), void (*block) (),
		  void *timer_info, unsigned long interval_us,
		  unsigned long interval_s, void *list_info) );
EXTERN_FUNCTION (void e_dispatch_events,
		 (KPeriodicEventList list, unsigned int dispatch_level,
		  flag timeout) );
EXTERN_FUNCTION (KPeriodicEventFunc e_register_func,
		 (KPeriodicEventList list, flag (*func) (),
		  void *info, unsigned long interval,
		  unsigned int dispatch_level) );
EXTERN_FUNCTION (void e_unregister_func, (KPeriodicEventFunc func) );


/*  File: unix.c  */
EXTERN_FUNCTION (KPeriodicEventList e_unix_create_list,
		 (unsigned long interval_us, unsigned long interval_s,
		  void *list_info) );
EXTERN_FUNCTION (void e_unix_dispatch_events, (unsigned int dispatch_level) );


#endif /*  KARMA_E_H  */

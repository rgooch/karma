/*  karma_mt.h

    Header for  mt_  package.

    Copyright (C) 1995-1996  Richard Gooch

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
  needed to interface to the mt_ routines in the Karma library.


    Written by      Richard Gooch   12-JAN-1995

    Last updated by Richard Gooch   24-JAN-1996

*/

#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_MT_H
#define KARMA_MT_H


typedef struct threadpool_type * KThreadPool;


/*  File:  main.c  */
EXTERN_FUNCTION (KThreadPool mt_create_pool, (void *pool_info) );
EXTERN_FUNCTION (void mt_destroy_pool, (KThreadPool pool, flag interrupt) );
EXTERN_FUNCTION (void mt_destroy_all_pools, (flag interrupt) );
EXTERN_FUNCTION (unsigned int mt_num_threads, (KThreadPool pool) );
EXTERN_FUNCTION (void mt_launch_job,
		 (KThreadPool pool,
		  void (*func) (void *pool_info,
				void *call_info1, void *call_info2,
				void *call_info3, void *call_info4,
				void *thread_info),
		  void *call_info1, void *call_info2,
		  void *call_info3, void *call_info4) );
EXTERN_FUNCTION (void mt_wait_for_all_jobs, (KThreadPool pool) );
EXTERN_FUNCTION (void mt_setlock, (KThreadPool pool, flag lock) );
EXTERN_FUNCTION (void mt_new_thread_info,
		 (KThreadPool pool, void *info, uaddr size) );
EXTERN_FUNCTION (void *mt_get_thread_info, (KThreadPool pool) );


#endif /*  KARMA_MT_H  */

/*LINTLIBRARY*/
/*  event.c

    This code provides miscellaneous routines for the X Intrinsics toolkit

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

/*  This file contains miscellaneous routines needed for the using the Xt
  toolkit.


    Written by      Richard Gooch   1-DEC-1996

    Last updated by Richard Gooch   1-DEC-1996


*/
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
/*  Do not include  karma.h  first, since CONST may be screwed otherwise. This
    applies to broken versions of  X11/Intrinsic.h  */
#include <karma_xtmisc.h>
#include <karma_m.h>
#include <karma_e.h>

typedef struct
{
    XtAppContext app_context;
    XtIntervalId id;
    KPeriodicEventList list;
    unsigned long interval;
} xt_timer_info;


/*  Private functions  */
STATIC_FUNCTION (flag start_timer,
		 (KPeriodicEventList list, void **timer_info,
		  unsigned long interval_us, unsigned long interval_s) );
STATIC_FUNCTION (void stop_timer,
		 (KPeriodicEventList list, void **timer_info) );
STATIC_FUNCTION (void proc, (XtPointer client_data, XtIntervalId *id) );


/*  Private data  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KPeriodicEventList xtmisc_event_create_list (XtAppContext app_context,
					     unsigned long interval_us,
					     unsigned long interval_s,
					     void *list_info)
/*  [SUMMARY] Create a periodic event management list under Xt.
    <app_context> The application context to use.
    <interval_us> The microsecond component time between timeouts on this list.
    <interval_s> The second component time between timeouts on this list.
    <list_info> An arbitrary pointer passed to event functions on this list.
    [NOTE] The specified interval may be rounded up to the system timer
    resolution (10ms on many systems).
    [RETURNS] A KPeriodicEventList on success, else the process aborts.
*/
{
    xt_timer_info *info;
    static char function_name[] = "xtmisc_event_create_list";

    if ( ( info = (xt_timer_info *) m_alloc (sizeof *info) ) == NULL )
    {
	m_abort (function_name, "structure");
    }
    info->app_context = app_context;
    info->id = 0;
    info->list = e_create_list (start_timer, stop_timer, NULL, info,
				interval_us, interval_s, list_info);
    return (info->list);
}   /*  End Function xtmisc_event_create_list  */


/*  Private functions follow  */

static flag start_timer (KPeriodicEventList list, void **timer_info,
			 unsigned long interval_us, unsigned long interval_s)
/*  [SUMMARY] Start timer using Xt interval timer.
    [NOTE] This routine must not be called by the application, it is for
    internal use by the [<e>] package.
    <list> The event list.
    <timer_info> A pointer to an arbitrary information pointer.
    <interval_us> The microsecond component time between timeouts on this list.
    <interval_s> The second component time between timeouts on this list.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    xt_timer_info *info = *timer_info;
    /*static char function_name[] = "__xtmisc_event_start_timer";*/

    info->interval = interval_us / 1000 + interval_s * 1000;
    info->id = XtAppAddTimeOut (info->app_context, info->interval, proc, info);
    return (TRUE);
}   /*  End Function start_timer  */

static void stop_timer (KPeriodicEventList list, void **timer_info)
/*  [SUMMARY] Stop the Xt interval timer.
    [NOTE] This routine must not be called by the application, it is for
    internal use by the [<e>] package.
    <list> The event list.
    <timer_info> A pointer to an arbitrary information pointer.
    [RETURNS] Nothing.
*/
{
    xt_timer_info *info = *timer_info;
    /*static char function_name[] = "__xtmisc_event_stop_timer";*/

    if (info->id == 0) return;
    XtRemoveTimeOut (info->id);
    info->id = 0;
}   /*  End Function stop_timer  */

static void proc (XtPointer client_data, XtIntervalId *id)
/*  [SUMMARY] Xt interval procedure.
*/
{
    xt_timer_info *info = client_data;

    info->id = XtAppAddTimeOut (info->app_context, info->interval, proc, info);
    e_dispatch_events (info->list, DISPATCH_SYNCHRONOUS, TRUE);
}   /*  End Function proc  */

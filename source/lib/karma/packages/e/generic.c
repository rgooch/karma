/*LINTLIBRARY*/
/*  generic.c

    This code provides KPeriodicEvent* objects.

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

/*  This file contains all routines needed for the creation, manipulation and
  deletion of KPeriodicEvent* objects.


    Written by      Richard Gooch   30-NOV-1996

    Last updated by Richard Gooch   30-NOV-1996


*/

#include <stdio.h>
#include <stdlib.h>
#include <karma.h>
#include <karma_e.h>


#define LIST_MAGIC_NUMBER 908234290
#define FUNC_MAGIC_NUMBER 32908534

#define VERIFY_LIST(list) if (list == NULL) \
{fprintf (stderr, "NULL KPeriodicEventList passed\n"); \
 prog_bug (function_name); } \
if ((list)->magic_number != LIST_MAGIC_NUMBER ) \
{fprintf (stderr, "Invalid KPeriodicEventList object\n"); \
 prog_bug (function_name); }

#define VERIFY_FUNC(func) if (func == NULL) \
{fprintf (stderr, "NULL KPeriodicEventFunc passed\n"); \
 prog_bug (function_name); } \
if (func->magic_number != FUNC_MAGIC_NUMBER ) \
{fprintf (stderr, "Invalid KPeriodicEventFunc object\n"); \
 prog_bug (function_name); }

struct periodic_event_list_type
{
    unsigned int magic_number;
    flag (*start) (KPeriodicEventList list, void **timer_info,
		   unsigned long interval_us, unsigned long interval_s);
    void (*stop) (KPeriodicEventList list, void **timer_info);
    void (*block) (KPeriodicEventList list, void **timer_info, flag block);
    void *timer_info;
    unsigned long interval_us;
    unsigned long interval_s;
    void *list_info;
    flag running;
    KPeriodicEventFunc first;
    KPeriodicEventFunc last;
};

struct periodic_event_func_type
{
    unsigned int magic_number;
    KPeriodicEventList list;
    flag (*func) (KPeriodicEventFunc func, void *info);
    void *info;
    unsigned long interval;
    unsigned long counter;
    unsigned int dispatch_level;
    flag dispatch;
    KPeriodicEventFunc next;
    KPeriodicEventFunc prev;
};


/*  Private functions  */
STATIC_FUNCTION (void prog_bug, (char *function_name) );
STATIC_FUNCTION (void mem_abort, (char *name, char *reason) );


/*  Public functions follow  */

/*UNPUBLISHED_FUNCTION*/
KPeriodicEventList e_create_list (flag (*start) (), void (*stop) (),
				  void (*block) (), void *timer_info,
				  unsigned long interval_us,
				  unsigned long interval_s, void *list_info)
/*  [SUMMARY] Create a periodic event management list.
    <start> The function to be called when a periodic timer should start. The
    interface to this function is the same as [<e_unix_start_timer>].
    <stop> The function to be called when a periodic timer is no longer needed.
    The interface to this function is the same as [<e_unix_stop_timer>].
    <block> The function to be called when the periodic timer is to be blocked
    or unblocked. This may be NULL. The interface to this function is the same
    as [<e_unix_block_sigalrm>]
    <timer_info> An arbitrary pointer passed to <<start>> and <<stop>>.
    <interval_us> The microsecond component time between timeouts on this list.
    <interval_s> The second component time between timeouts on this list.
    <list_info> An arbitrary pointer passed to event functions on this list.
    [NOTE] The specified interval may be rounded up to the system timer
    resolution (10ms on many systems).
    [RETURNS] A KPeriodicEventList on success, else the process aborts.
*/
{
    KPeriodicEventList list;
    static char function_name[] = "e_create_list";

    if ( ( list = malloc (sizeof *list) ) == NULL )
    {
	mem_abort (function_name, "list");
    }
    list->magic_number = LIST_MAGIC_NUMBER;
    list->start = start;
    list->stop = stop;
    list->block = block;
    list->timer_info = timer_info;
    list->interval_us = interval_us;
    list->interval_s = interval_s;
    list->list_info = list_info;
    list->running = FALSE;
    list->first = NULL;
    list->last = NULL;
    return (list);
}   /*  End Function e_create_list  */

/*PUBLIC_FUNCTION*/
void e_dispatch_events (KPeriodicEventList list, unsigned int dispatch_level,
			flag timeout)
/*  [SUMMARY] Dispatch events for a list.
    <list> The event list.
    <dispatch_level> The level of events that may dispatched. Event functions
    at the same level or higher are dispatched. See [<E_DISPATCH_LEVELS>] for a
    list of legal values.
    <timeout> If TRUE, the perdiodic timer timed out.
    [RETURNS] Nothing.
*/
{
    KPeriodicEventFunc func;
    static char function_name[] = "e_dispatch_events";

    VERIFY_LIST (list);
    /*  Scan through list looking for events to dispatch  */
    for (func = list->first; func != NULL; func = func->next)
    {
	if (timeout)
	{
	    ++func->counter;
	    if (func->counter >= func->interval)
	    {
		func->dispatch = TRUE;
		func->counter = 0;
	    }
	}
	if ( func->dispatch && (func->dispatch_level >= dispatch_level) )
	{
	    func->dispatch = FALSE;
	    if ( !(*func->func) (func, func->info) ) e_unregister_func (func);
	}
    }
}   /*  End Function e_dispatch_events  */

/*PUBLIC_FUNCTION*/
KPeriodicEventFunc e_register_func (KPeriodicEventList list, flag (*func) (),
				    void *info, unsigned long interval,
				    unsigned int dispatch_level)
/*  [SUMMARY] Register a function to be called periodically.
    <list> The event list to manage the function.
    <func> The function to be called upon the specified timout. The prototype
    function is [<E_PROTO_event_func>].
    <info> An arbitrary pointer passed to <<func>>.
    <interval> The interval between successive calls to <<func>>. This is
    measured in units of the list timeout.
    <dispatch_level> The preferred method of dispatch for the function. See
    [<E_DISPATCH_LEVELS>] for a list of legal values. Not all implementations
    may support the higher levels, so events may dispatched at a lower level
    than desired.
    [RETURNS] A KPeriodicEventFunc on success, else the process aborts.
*/
{
    KPeriodicEventFunc new;
    static char function_name[] = "e_register_func";

    VERIFY_LIST (list);
    if ( ( new = malloc (sizeof *new) ) == NULL )
    {
	mem_abort (function_name, "function");
    }
    new->magic_number = FUNC_MAGIC_NUMBER;
    new->list = list;
    new->func = func;
    new->info = info;
    new->interval = interval;
    new->counter = 0;
    new->dispatch_level = dispatch_level;
    new->dispatch = FALSE;
    new->prev = list->last;
    new->next = NULL;
    if (list->block != NULL) (*list->block) (list, &list->timer_info, TRUE);
    /*  Append to end of list  */
    if (list->first == NULL) list->first = new;
    else list->last->next = new;
    list->last = new;
    if (list->block != NULL) (*list->block) (list, &list->timer_info, FALSE);
    if (list->running) return (new);
    /*  Need to start timer  */
    if ( !(*list->start) (list, &list->timer_info,
			  list->interval_us, list->interval_s) )
    {
	fprintf (stderr, "%s: could not start timer\n", function_name);
	return (new);
    }
    list->running = TRUE;
    return (new);
}   /*  End Function e_register_func  */

/*PUBLIC_FUNCTION*/
void e_unregister_func (KPeriodicEventFunc func)
/*  [SUMMARY] Unregister an event function.
    <func> The callback function.
    [RETURNS] Nothing.
*/
{
    KPeriodicEventList list;
    static char function_name[] = "e_unregister_func";

    VERIFY_FUNC (func);
    list = func->list;
    if (list->block != NULL) (*list->block) (list, &list->timer_info, TRUE);
    if (func->prev == NULL)
    {
	/*  First in list  */
	func->list->first = func->next;
    }
    else
    {
	func->prev->next = func->next;
    }
    if (func->next == NULL)
    {
	/*  Last in list  */
	func->list->last = func->prev;
    }
    else
    {
	func->next->prev = func->prev;
    }
    if (list->first == NULL) (*list->stop) (list, &list->timer_info);
    if (list->block != NULL) (*list->block) (list, &list->timer_info, FALSE);
    func->magic_number = 0;
    free ( (char *) func );
}   /*  End Function e_unregister_func  */


/*  Private functions follow  */

static void prog_bug (char *function_name)
{
    fprintf (stderr, "Program bug noted in function: %s\n", function_name);
    fprintf (stderr, "Aborting.%c\n", BEL);
    abort ();
}   /*  End Function prog_bug   */

static void mem_abort (char *name, char *reason)
{
    fprintf (stderr, "Error allocating memory for: %s  for function: %s%c\n",
	     reason, name, BEL);
    fprintf (stderr, "Aborting.\n");
    exit (RV_MEM_ERROR);
}   /*  End Function mem_abort  */  

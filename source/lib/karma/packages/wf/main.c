/*LINTLIBRARY*/
/*  main.c

    This code provides KWorkFunc objects.

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

/*  This file contains all routines needed for the creation, manipulation and
  deletion of KWorkFunc objects.


    Written by      Richard Gooch   30-DEC-1995

    Last updated by Richard Gooch   13-APR-1996: Changed to new documentation
  format.


*/

#include <stdio.h>
#include <stdlib.h>
#include <karma.h>
#include <karma_wf.h>
#include <karma_c.h>
#include <karma_m.h>
#include <karma_a.h>


#define FUNC_MAGIC_NUMBER 1987342876

#define VERIFY_FUNC(func) if (func == NULL) \
{fprintf (stderr, "NULL KWorkFunc passed\n"); \
 a_prog_bug (function_name); } \
if (func->magic_number != FUNC_MAGIC_NUMBER ) \
{fprintf (stderr, "Invalid KWorkFunc object\n"); \
 a_prog_bug (function_name); }

struct work_func_type
{
    unsigned int magic_number;
    flag (*func) (void **info);
    void *info;
    unsigned int priority;
    KWorkFunc next;
    KWorkFunc prev;
};


/*  Private data  */
static flag work_funcs_supported = FALSE;
static KWorkFunc first_func = NULL;
static KWorkFunc last_func = NULL;
static KWorkFunc executing_func = NULL;
static KCallbackList register_list = NULL;


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag wf_test_supported ()
/*  [SUMMARY] Test if application supports work functions.
    [PURPOSE] This routine will determine if work functions have been supported
    by the application.
    [RETURNS] TRUE if work functions are supported, else FALSE.
*/
{
    extern flag work_funcs_supported;

    return (work_funcs_supported);
}   /*  End Function wf_test_supported  */


/*PUBLIC_FUNCTION*/
void wf_register_support ()
/*  [SUMMARY] Register support for work functions.
    [PURPOSE] This routine will inform the package that the application will
    support work functions. This routine may only be called once.
    [RETURNS] Nothing.
*/
{
    extern flag work_funcs_supported;
    static flag first_time = TRUE;
    static char function_name[] = "wf_register_support";

    if (first_time)
    {
	work_funcs_supported = TRUE;
	first_time = FALSE;
	return;
    }
    fprintf (stderr, "Work functions already supported\n");
    a_prog_bug (function_name);
}   /*  End Function wf_register_support  */

/*PUBLIC_FUNCTION*/
KCallbackFunc wf_register_notify_func (void (*func) (void *info), void *info)
/*  [SUMMARY] Register new work function callback.
    [PURPOSE] This routine will register a routine which is called whenever a
    new work function is registered.
    <func> The function that is called when a new work function is registered.
    The prototype function is [<WF_PROTO_new_func>].
    <info> The initial arbitrary information pointer.
    [RETURNS] A KCallbackFunc object. On failure, the process aborts.
*/
{
    extern KCallbackList register_list;

    return ( c_register_callback (&register_list, ( flag (*) () ) func, info,
				  NULL, FALSE, NULL, FALSE, FALSE) );
}   /*  End Function wf_register_notify_func  */

/*PUBLIC_FUNCTION*/
flag wf_work_to_be_done ()
/*  [SUMMARY] Test for pending work functions waiting to be called.
    [PURPOSE] This routine will determine if there are any pending work
    functions waiting to be called.
    [RETURNS] TRUE if any work functions are pending, else FALSE.
*/
{
    extern KWorkFunc first_func;

    return ( (first_func == NULL) ? FALSE : TRUE );
}   /*  End Function wf_work_to_be_done  */

/*PUBLIC_FUNCTION*/
flag wf_do_work ()
/*  [SUMMARY] Do some work.
    [PURPOSE] This routine will call any pending work functions. The work
    function at the top of the list is called once only.
    [RETURNS] TRUE if more work is left to be done, else FALSE.
*/
{
    flag more_to_do;
    extern KWorkFunc first_func;
    extern KWorkFunc executing_func;
    static char function_name[] = "wf_do_work";

    if (first_func == NULL) return (FALSE);
    if (executing_func != NULL)
    {
	fprintf (stderr, "Not re-entrant!\n");
	a_prog_bug (function_name);
    }
    executing_func = first_func;
    more_to_do = (*executing_func->func) (&executing_func->info);
    executing_func = NULL;
    if (more_to_do) return (TRUE);
    wf_unregister_func (first_func);
    return ( (first_func == NULL) ? FALSE : TRUE );
}   /*  End Function wf_do_work  */

/*PUBLIC_FUNCTION*/
KWorkFunc wf_register_func (flag (*func) (void **info), void *info,
			    unsigned int priority)
/*  [SUMMARY] Register a work function.
    [PURPOSE] This routine will register a work function. The work function at
    the top of the list is the one that is always called. Only when the work
    function at the top of the list is unregistered will the next work function
    be called.
    <func> The routine to be called. The prototype function is
    [<WF_PROTO_work_func>].
    <info> The initial arbitrary work function information pointer.
    <priority> The priority level of the work function. See [<WF_PRIORITIES>]
    for a list of legal values.
    [RETURNS] A KWorkFunc object. On failure, the process aborts.
*/
{
    KWorkFunc new;
    extern KWorkFunc first_func;
    extern KWorkFunc last_func;
    extern KWorkFunc executing_func;
    extern KCallbackList register_list;
    extern flag work_funcs_supported;
    static char function_name[] = "wf_register_func";

    if (!work_funcs_supported)
    {
	fprintf (stderr, "Work functions not supported by application\n");
	a_prog_bug (function_name);
    }
    if ( ( new = (KWorkFunc) m_alloc (sizeof *new) ) == NULL )
    {
	m_abort (function_name, "KWorkFunc object");
    }
    new->func = func;
    new->info = info;
    new->priority = priority;
    new->magic_number = FUNC_MAGIC_NUMBER;
    if ( (executing_func == NULL) ||
	(executing_func->priority != KWF_PRIORITY_HIGHEST) )
    {
	/*  Add to start of list  */
	new->prev = NULL;
	new->next = first_func;
	first_func = new;
	if (last_func == NULL) last_func = new;
	c_call_callbacks (register_list, NULL);
	return (new);
    }
    /*  We have been called from inside a work function, which will not allow
	functions registered during its execution to be placed above it in the
	list. Hence place the new function second.  */
    new->prev = executing_func;
    new->next = executing_func->next;
    executing_func->next = new;
    c_call_callbacks (register_list, NULL);
    return (new);
}   /*  End Function wf_register_func  */

/*PUBLIC_FUNCTION*/
void wf_unregister_func (KWorkFunc wf)
/*  [SUMMARY] Unregister a work function.
    [PURPOSE] This routine will unregister a work function. The second function
    in the list will moved to the top of the list. A work function cannot
    unregister itself in this manner.
    <wf> The KWorkFunc object.
    [RETURNS] Nothing.
*/
{
    extern KWorkFunc first_func;
    extern KWorkFunc last_func;
    extern KWorkFunc executing_func;
    static char function_name[] = "wf_unregister_func";

    VERIFY_FUNC (wf);
    if (wf == executing_func)
    {
	fprintf (stderr, "Work procedure cannot unregister itself this way\n");
	a_prog_bug (function_name);
    }
    if (wf->prev == NULL) first_func = wf->next;
    else wf->prev->next = wf->next;
    if (wf->next == NULL) last_func = wf->prev;
    else wf->next->prev = wf->prev;
    m_clear ( (char *) wf, sizeof *wf );
    m_free ( (char *) wf );
}   /*  End Function wf_unregister_func  */

/*LINTLIBRARY*/
/*  main.c

    This code provides KCallback* objects.

    Copyright (C) 1994-1996  Richard Gooch

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
  deletion of KCallback* objects.


    Written by      Richard Gooch   27-NOV-1994

    Updated by      Richard Gooch   28-NOV-1994

    Updated by      Richard Gooch   25-JAN-1995: Cosmetic changes.

    Updated by      Richard Gooch   20-APR-1995: More cosmetic changes.

    Updated by      Richard Gooch   27-JUL-1995: More descriptive error when
  callback returns invalid flag.

    Updated by      Richard Gooch   8-OCT-1995: Call <abort> rather than <exit>
  in <prog_bug>.

    Updated by      Richard Gooch   24-JAN-1996: Fixed bug in
  <c_unregister_callback>.

    Last updated by Richard Gooch   31-MAR-1996: Changed documentation style.


*/

#include <stdio.h>
#include <stdlib.h>
#include <karma.h>
#include <karma_c.h>


#define LIST_MAGIC_NUMBER 190483290
#define FUNC_MAGIC_NUMBER 219873398

#define VERIFY_LIST(list) if (list == NULL) \
{(void) fprintf (stderr, "NULL KCallbackList passed\n"); \
 prog_bug (function_name); } \
if ((list)->magic_number != LIST_MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid KCallbackList object\n"); \
 prog_bug (function_name); }

#define VERIFY_FUNC(func) if (func == NULL) \
{(void) fprintf (stderr, "NULL KCallbackFunc passed\n"); \
 prog_bug (function_name); } \
if (func->magic_number != FUNC_MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid KCallbackFunc object\n"); \
 prog_bug (function_name); }

struct callback_list_type
{
    unsigned int magic_number;
    flag dispatching;
    KCallbackFunc first;
    KCallbackFunc last;
};

struct callback_func_type
{
    unsigned int magic_number;
    KCallbackList list;
    flag (*func) (void *object, void *client1_data, void *call_data,
		  void *client2_data);
    void *object;
    flag client1_indirect;
    void *client1_data;
    flag client2_indirect;
    void *client2_data;
    flag quenchable;
    KCallbackFunc next;
    KCallbackFunc prev;
};


/*  Private functions  */
STATIC_FUNCTION (void prog_bug, (char *function_name) );
STATIC_FUNCTION (void mem_abort, (char *name, char *reason) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KCallbackFunc c_register_callback (KCallbackList *list, flag (*callback) (),
				   void *object,
				   void *client1_data, flag client1_indirect,
				   void *client2_data, flag client2_indirect,
				   flag quenchable)
/*  [SUMMARY] Register a callback function with a list.
    [PURPOSE] This routine will register a function which should be called when
    the callbacks for an object should be called. When the object is destroyed
    a call should be made to [<c_destroy_list>]. The first callback registered
    is the first one called. Multiple callback functions may be registered per
    object.
    <list> A pointer to the callback list. This is changed. The initial value
    must be NULL.
    <callback> The function to be called when the object callbacks are called.
    The prototype function is [<C_PROTO_callback>].
    <object> The object pointer passed to the callback. This may be NULL.
    <client1_data> The first client information pointer passed to the callback.
    This may be NULL.
    <client1_indirect> If TRUE the callback is passed a pointer to the storage
    containing <<client1_data>>, else <<client1_data>> is passed directly to
    the callback.
    <client2_data> The second client information pointer passed to the callback
    This may be NULL.
    <client2_indirect> If TRUE the callback is passed a pointer to the storage
    containing <<client2_data>>, else <<client2_data>> is passed directly to
    the callback.
    <quenchable> If TRUE then the routine is permitted to quench calls to the
    following callbacks in the list.
    [RETURNS] A KCallbackFunc on success. On failure, the process aborts.
*/
{
    KCallbackFunc new_cbk;
    static char function_name[] = "c_register_callback";

    FLAG_VERIFY (client1_indirect);
    FLAG_VERIFY (client2_indirect);
    FLAG_VERIFY (quenchable);
    if (*list == NULL)
    {
	/*  Initialise  */
	if ( ( *list = (KCallbackList) malloc (sizeof **list) ) == NULL )
	{
	    mem_abort (function_name, "list");
	}
	(**list).magic_number = LIST_MAGIC_NUMBER;
	(**list).dispatching = FALSE;
	(**list).first = NULL;
	(**list).last = NULL;
    }
    else
    {
	VERIFY_LIST (*list);
    }
    if ( ( new_cbk = (KCallbackFunc) malloc (sizeof *new_cbk) ) ==NULL )
    {
	mem_abort (function_name, "func");
    }
    /*  Append to list  */
    new_cbk->magic_number = FUNC_MAGIC_NUMBER;
    new_cbk->list = *list;
    new_cbk->func = callback;
    new_cbk->object = object;
    new_cbk->client1_indirect = client1_indirect;
    new_cbk->client1_data = client1_data;
    new_cbk->client2_indirect = client2_indirect;
    new_cbk->client2_data = client2_data;
    new_cbk->quenchable = quenchable;
    new_cbk->prev = (**list).last;
    new_cbk->next = NULL;
    if ( (**list).first == NULL )
    {
	(**list).first = new_cbk;
    }
    else
    {
	(**list).last->next = new_cbk;
    }
    (**list).last = new_cbk;
    return (new_cbk);
}   /*  End Function c_register_callback  */

/*PUBLIC_FUNCTION*/
void c_unregister_callback (KCallbackFunc callback)
/*  [SUMMARY] Unregister a callback function.
    <callback> The callback function.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "c_unregister_callback";

    VERIFY_FUNC (callback);
    if (callback->prev == NULL)
    {
	/*  First in list  */
	callback->list->first = callback->next;
    }
    else
    {
	callback->prev->next = callback->next;
    }
    if (callback->next == NULL)
    {
	/*  Last in list  */
	callback->list->last = callback->prev;
    }
    else
    {
	callback->next->prev = callback->prev;
    }
    callback->magic_number = 0;
    free ( (char *) callback );
}   /*  End Function c_unregister_callback  */

/*PUBLIC_FUNCTION*/
flag c_call_callbacks (KCallbackList list, void *call_data)
/*  [SUMMARY] Call all registered callbacks for an object.
    <list> The callback list.
    <call_data> The arbitrary call information pointer.
    [RETURNS] TRUE if one of the callbacks quenched the further delivery of
    callbacks, else FALSE.
*/
{
    KCallbackFunc curr;
    flag quench;
    void *client1_data, *client2_data;
    static char function_name[] = "c_call_callbacks";

    if (list == NULL) return (FALSE);
    VERIFY_LIST (list);
    if (list->dispatching)
    {
	(void) fprintf (stderr, "Already dispatching callbacks for list!\n");
	prog_bug (function_name);
    }
    for (curr = list->first; curr != NULL; curr = curr->next)
    {
	list->dispatching = TRUE;
	if (curr->client1_indirect)
	{
	    client1_data = (void *) &curr->client1_data;
	}
	else
	{
	    client1_data = curr->client1_data;
	}
	if (curr->client2_indirect)
	{
	    client2_data = (void *) &curr->client2_data;
	}
	else
	{
	    client2_data = curr->client2_data;
	}
	quench = (*curr->func) (curr->object, client1_data,
				call_data, client2_data);
	list->dispatching = FALSE;
	if (curr->quenchable)
	{
	    if ( (quench != TRUE) && (quench != FALSE) )
	    {
		(void) fprintf (stderr, "Illegal return flag value: %d\n",
				quench);
		a_prog_bug (function_name);
	    }
	    if (quench) return (TRUE);
	}
    }
    return (FALSE);
}   /*  End Function c_call_callbacks  */

/*PUBLIC_FUNCTION*/
void c_destroy_list (KCallbackList list)
/*  [SUMMARY] Destroy a callback list.
    [PURPOSE] This routine will unregister all callbacks in a callback list and
    then destroys the list.
    <list> The callback list.
    [RETURNS] Nothing.
*/
{
    static char function_name[] = "c_destroy_list";

    VERIFY_LIST (list);
    while (list->first != NULL) c_unregister_callback (list->first);
    list->magic_number = 0;
    free ( (char *) list );
}   /*  End Function c_destroy_list  */


/*  Private functions follow  */

static void prog_bug (char *function_name)
{
    (void) fprintf (stderr, "Program bug noted in function: %s\n",
		    function_name);
    (void) fprintf (stderr, "Aborting.%c\n", BEL);
    abort ();
}   /*  End Function prog_bug   */

static void mem_abort (char *name, char *reason)
{
    (void) fprintf (stderr,
		    "Error allocating memory for: %s  for function: %s%c\n",
		    reason, name, BEL);
    (void) fprintf (stderr, "Aborting.\n");
    exit (RV_MEM_ERROR);
}   /*  End Function mem_abort  */  

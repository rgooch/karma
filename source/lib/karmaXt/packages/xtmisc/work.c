/*LINTLIBRARY*/
/*  work.c

    This code provides miscellaneous routines for the X Intrinsics toolkit

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

/*  This file contains miscellaneous routines needed for the using the Xt
  toolkit.


    Written by      Richard Gooch   30-DEC-1995

    Last updated by Richard Gooch   16-APR-1996: Changed to new documentation
  format.


*/
#include <stdio.h>
#include <ctype.h>
#include <sys/types.h>
#include <errno.h>
/*  Do not include  karma.h  first, since CONST may be screwed otherwise. This
    applies to broken versions of  X11/Intrinsic.h  */
#include <karma_xtmisc.h>
#include <karma_wf.h>
#include <karma_m.h>
#include <karma_a.h>

typedef struct
{
    XtAppContext app_context;
    flag xt_work_func_registered;
} xt_work_info;


/*  Private functions  */
STATIC_FUNCTION (Boolean xt_work_proc, (XtPointer client_data) );
STATIC_FUNCTION (void new_work_func, (void *info) );


/*  Private data  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
void xtmisc_support_work_funcs (XtAppContext app_context)
/*  [SUMMARY] Register support for work functions within an Xt application.
    [PURPOSE] This routine will register Xt support for work functions using
    the [<wf>] package.
    <app_context> The Xt application context.
    [RETURNS] Nothing. On failure the process aborts.
*/
{
    xt_work_info *info;
    static char function_name[] = "xtmisc_support_work_funcs";

    wf_register_support ();
    if ( wf_work_to_be_done () )
    {
	(void) fprintf (stderr, "No work functions may be registered yet\n");
	a_prog_bug (function_name);
    }
    if ( ( info = (xt_work_info *) m_alloc (sizeof *info) ) == NULL )
    {
	m_abort (function_name, "handle");
    }
    info->app_context = app_context;
    info->xt_work_func_registered = FALSE;
    (void) wf_register_notify_func (new_work_func, (void *) info);
}   /*  End Function xtmisc_support_work_funcs  */


/*  Private functions follow  */

static Boolean xt_work_proc (XtPointer client_data)
/*  [PURPOSE] This routine is an Xt work procedure.
    <client_data> The aribtrary information pointer.
    [RETURNS] True if the work procedure is finished and should be
    unregistered, else False indicating that there is more work to be done.
*/
{
    xt_work_info *info = (xt_work_info *) client_data;

    if ( wf_do_work () ) return (False);
    info->xt_work_func_registered = FALSE;
    return (True);
}   /*  End Function xt_work_proc  */

static void new_work_func (void *info)
/*  [PURPOSE] This routine is called whenever a new work function is
    registered.
    <info> An arbitrary information pointer.
    [RETURNS] Nothing.
*/
{
    xt_work_info *w_info = (xt_work_info *) info;

    if (w_info->xt_work_func_registered) return;
    w_info->xt_work_func_registered = TRUE;
    (void) XtAppAddWorkProc (w_info->app_context, xt_work_proc, w_info);
}   /*  End Function new_work_func  */

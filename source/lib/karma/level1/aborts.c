/*LINTLIBRARY*/
/*PREFIX:"a_"*/
/*  aborts.c
    This code provides some simple error printing routines.

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

    This file contains the various utility routines for printing error
    messages.


    Written by      Richard Gooch   14-AUG-1992

    Last updated by Richard Gooch   1-MAY-1993: Added GLPL.


*/

#include <stdio.h>
#include <karma.h>
#include <karma_a.h>

/*PUBLIC_FUNCTION*/
void a_print_abort ()
/*  This routine will print the message "Aborting.\n" to the standard output
    and will then abort the process.
    The routine does not return.
*/
{
    (void) fprintf (stderr, "Aborting.\n");
    abort ();
}   /*  End Function a_print_abort    */

/*PUBLIC_FUNCTION*/
void a_prog_bug (function_name)
/*  This routine will print an error message to the standard error indicating
    that a bug occurred.
    The name of the function in which the bug was noted must be pointed to by
    function_name  .
    The routine returns nothing. The process is aborted.
*/
char *function_name;
{
    (void) fprintf (stderr, "Program bug noted in function: %s\n",
		    function_name);
    (void) fprintf (stderr, "Aborting.%c\n", BEL);
    abort ();
}   /*  End Function a_prog_bug   */

/*PUBLIC_FUNCTION*/
void a_func_abort (function_name, reason)
/*  This routine will print an error message to the standard error indicating
    that a function is terminating abnormally.
    The name of the function must be pointed to by  function_name  .
    The reason for the function aborting must be pointed to by  reason  .
    The routine returns nothing.
*/
char *function_name;
char *reason;
{
    (void) fprintf (stderr, "Function: %s bad exit, reason: %s\n",
		    function_name, reason);
}   /*  End Function a_func_abort */

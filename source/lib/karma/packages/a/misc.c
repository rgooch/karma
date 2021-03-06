/*LINTLIBRARY*/
/*  misc.c
    This code provides some simple error printing routines.

    Copyright (C) 1992-1996  Richard Gooch

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

    Updated by      Richard Gooch   1-MAY-1993: Added GLPL.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/a/aborts.c

    Last updated by Richard Gooch   30-MAR-1996: Changed to new documentation
  format.


*/

#include <stdio.h>
#include <karma.h>
#include <karma_a.h>

/*PUBLIC_FUNCTION*/
void a_print_abort ()
/*  [SUMMARY] Print an abort message.
    [PURPOSE] This routine will print the message "Aborting.\n" to the standard
    error and will then abort the process.
    [RETURNS] The routine does not return. The process is aborted.
*/
{
    (void) fprintf (stderr, "Aborting.\n");
    abort ();
}   /*  End Function a_print_abort    */

/*PUBLIC_FUNCTION*/
void a_prog_bug (char *function_name)
/*  [SUMMARY] Print a bug message.
    [PURPOSE] This routine will print an error message to the standard error
    indicating that a bug occurred.
    <function_name> The name of the function in which the bug was noted.
    [RETURNS] The routine does not return. The process is aborted.
*/
{
    (void) fprintf (stderr, "Program bug noted in function: %s\n",
		    function_name);
    (void) fprintf (stderr, "Aborting.%c\n", BEL);
    abort ();
}   /*  End Function a_prog_bug   */

/*PUBLIC_FUNCTION*/
void a_func_abort (char *function_name, char *reason)
/*  [SUMMARY] Print a warning message.
    [PURPOSE] This routine will print an error message to the standard error
    indicating that a function is terminating abnormally.
    <function_name> The name of the function.
    <reason> The reason for the function aborting.
    [RETURNS] Nothing.
*/
{
    (void) fprintf (stderr, "Function: %s bad exit, reason: %s\n",
		    function_name, reason);
}   /*  End Function a_func_abort */

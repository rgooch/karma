/*LINTLIBRARY*/
/*  misc.c

    This code provides some sample (recommended) signal handlers.

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

    This file contains the various Karma utility routines which relate to
  handling signals.


    Written by      Richard Gooch   19-SEP-1992

    Updated by      Richard Gooch   14-NOV-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/s/misc.c

    Last updated by Richard Gooch   13-APR-1996: Changed to new documentation
  format.


*/
#include <stdio.h>
#include <karma.h>
#include <karma_s.h>

/*  Private data  */
static unsigned int c_control_c_limit = 5;
static unsigned int c_control_c = 0;

/*  Local functions  */
static void print_abort ();


/*PUBLIC_FUNCTION*/
void s_int_handler ()
/*  [SUMMARY] Recommended SIGINT handler.
    [PURPOSE] This routine will handle SIGINT signals (those generated by the
    C shell in response to a control C keypress).
    [RETURNS] Nothing.
*/
{
    extern unsigned int c_control_c;
    extern unsigned int c_control_c_limit;

    if (++c_control_c > c_control_c_limit)
    {
	(void) fprintf (stderr,
			"Number of control C's exceeded: %u\n\n",
			c_control_c_limit);
	print_abort ();
    }
}   /*  End Function s_int_handler  */

/*PUBLIC_FUNCTION*/
void s_term_handler ()
/*  [SUMMARY] Recommended SIGTERM handler.
    [PURPOSE] This routine will handle SIGTERM signals.
    [RETURNS] Nothing.
*/
{
    (void) fprintf (stderr, "Ignored sigTERM\n");
}   /*  End Function s_term_handler  */

/*PUBLIC_FUNCTION*/
flag s_check_for_int ()
/*  [SUMMARY] Check for queued SIGINTs.
    [PURPOSE] This routine will check if there is a queued interrupt (SIGINT)
    pending.
    [RETURNS] TRUE if there is a queued SIGINT, else FALSE.
*/
{
    extern unsigned int c_control_c;
    extern unsigned int c_control_c_limit;

    if (c_control_c > 0)
    {
	--c_control_c;
	return (TRUE);
    }
    return (FALSE);
}   /*  End Function s_check_for_int  */

static void print_abort ()
{
    (void) fprintf (stderr, "Aborting.\n");
    exit (RV_UNDEF_ERROR);
}   /*  End Function print_abort    */

/*LINTLIBRARY*/
/*  misc.c

    This code provides asynchronous command line reading routines.

    Copyright (C) 1992,1993,1994  Richard Gooch

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

    This file contains the various utility routines for reading command lines
  from the standard input in an asynchronous fashion.
  These routines depend on the chm_ and cm_ routines. Hence, they are not
  useable in a module which also uses the XView notifier.


    Written by      Richard Gooch   10-OCT-1992

    Updated by      Richard Gooch   13-DEC-1992

    Updated by      Richard Gooch   24-DEC-1992: Added buffering of unlimited
  numbers of lines on order to prevent loss of data.

    Updated by      Richard Gooch   30-DEC-1992: Added  arln_read_line  .

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   31-JAN-1993: Added check for interrupt
  (Control-C).

    Updated by      Richard Gooch   24-JUL-1993: Made use of new
  ch_test_for_io  routine.

    Updated by      Richard Gooch   1-SEP-1993: Added polling in case where
  ch_stdin  has closed.

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Updated by      Richard Gooch   6-AUG-1994: Moved to full ASCII function
  declaration syntax. Had to do this because of the confused Dec Alpha compiler

    Last updated by Richard Gooch   26-NOV-1994: Split and moved to
  packages/arln/misc.c


*/

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <karma.h>
#include <karma_arln.h>
#include <karma_chm.h>
#include <karma_ch.h>
#include <karma_cm.h>
#include <karma_ex.h>
#include <karma_a.h>
#include <karma_m.h>
#include <karma_s.h>


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
float arln_read_float (char *prompt, float default_value)
/*  This is an input routine that accepts input of one floating point 
    number, using a prompt. If no input is given, the routine returns a
    default value.
    The prompt must be pointed to by  prompt  .
    The default value must be given by  default_value  .
    The routine returns the value.
*/
{
    char string[STRING_LENGTH];
    char *n;
    static char function_name[] = "arln_read_float";

    if (arln_read_from_stdin (string, STRING_LENGTH, prompt) != TRUE)
    {
	(void) fprintf (stderr, "Error reading input\n");
	a_prog_bug (function_name);
    }
    if (string[0] == '\0')
    {
	return (default_value);
    }
    else
    {
	return ( ex_float (string, &n) );
    }
}   /*  End Function arln_read_float  */

/*PUBLIC_FUNCTION*/
int arln_read_int (char *prompt, int default_value)
/*  This is an input routine that accepts input of one integer number, using a
    prompt. If no input is given, the routine returns a default value.
    The prompt must be pointed to by  prompt  .
    The default value must be given by  default_value  .
    The routine returns the value.
*/
{
    char string[STRING_LENGTH];
    char *n;
    static char function_name[] = "arln_read_int";

    if (arln_read_from_stdin (string, STRING_LENGTH, prompt) != TRUE)
    {
	(void) fprintf (stderr, "Error reading input\n");
	a_prog_bug (function_name);
    }
    if (string[0] == '\0')
    {
	return (default_value);
    }
    else
    {
	return ( ex_int (string, &n) );
    }
}   /*  End Function arln_read_int  */

/*PUBLIC_FUNCTION*/
flag arln_read_flag (char *prompt, flag default_value)
/*  This is an input routine that prompts for a yes/no answer.
    If no input is given, the routine returns a default value.
    The prompt must be pointed to by  prompt  .
    The default value must be given by  default_value  .
    The routine returns TRUE if "yes" was typed in, else it returns FALSE.
*/
{
    char string[STRING_LENGTH];
    char *n;
    static char function_name[] = "arln_read_flag";

    if (arln_read_from_stdin (string, STRING_LENGTH, prompt) != TRUE)
    {
	(void) fprintf (stderr, "Error reading input\n");
	a_prog_bug (function_name);
    }
    if (string[0] == '\0')
    {
	return (default_value);
    }
    else
    {
	n = string;
	return ( ex_yes (&n, default_value) );
    }
}   /*  End Function arln_read_flag  */

/*PUBLIC_FUNCTION*/
flag arln_read_line (char *buffer, unsigned int length, char *prompt)
/*  This routine will read a line from a the standard input, stripping all
    comments, leading and trailing whitespace. The comment character is '#'.
    The NULL terminated string will be written to the storage pointed to by
    buffer  . The routine will NOT copy the '\n' newline character into the
    buffer.
    The length of the buffer must be given by  length  .
    The prompt which is to be displayed must be pointed to by  prompt  .Note
    that the prompt is only displayed if the standard input is NOT a disc file.
    The routine returns TRUE on successful reading, else it returns FALSE.
    The size of the buffer must be given by  length  .
    The routine returns TRUE on success, else it returns FALSE (indicating
    End-Of-File).
*/
{
    int len;
    char *ch;

    while ( arln_read_from_stdin (buffer, length, prompt) )
    {
	/*  Process line  */
	/*  Remove any comments  */
	if ( ( ch = strchr (buffer, '#') ) != NULL )
	{
	    *ch = '\0';
	}
	/*  Remove trailing spaces  */
	len = strlen (buffer) - 1;
	while ( (len >= 0) && isspace (buffer[len]) )
	{
	    /*  Whitespace  */
	    buffer[len--] = '\0';
	}
	if (buffer[0] == '\0')
	{
	    /*  Comment or blank line: skip  */
	    continue;
	}
	/*  Skip leading whitespace  */
	for (ch = buffer; isspace (*ch); ++ch);
	if (ch > buffer)
	{
	    /*  Get rid of whitespace  */
	    m_copy (buffer, ch, strlen (ch) + 1);
	}
	return (TRUE);
    }
    return (FALSE);
}   /*  End Function arln_read_line  */

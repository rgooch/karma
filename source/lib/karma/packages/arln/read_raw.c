/*LINTLIBRARY*/
/*  read_raw.c

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

    Updated by      Richard Gooch   26-NOV-1994: Split and moved to
  packages/arln/read_raw.c

    Last updated by Richard Gooch   7-DEC-1994: Stripped declaration of  errno


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

#define LINE_BUF_LENGTH 1024

/*  Structure declarations  */
typedef struct _linetype
{
    char buffer[LINE_BUF_LENGTH];
    unsigned int pos;
    struct _linetype *next;
} linetype;


/*  Private functions  */
STATIC_FUNCTION (flag stdin_input_func, (Channel channel, void **info) );
STATIC_FUNCTION (void stdin_close_func, (Channel channel, void *info) );


/*  Private data  */
static linetype *nextline = NULL;
static flag stdin_closed = FALSE;


/*  Private functions follow  */

static flag stdin_input_func (Channel channel, void **info)
/*  This routine will process input on the standard input channel.
    The channel object is given by  channel  .
    An arbitrary pointer may be written to the storage pointed to by  info
    The pointer written here will persist until the channel is unmanaged
    (or a subsequent callback routine changes it).
    The routine returns TRUE if the channel is to remain managed and
    open, else it returns FALSE (indicating that the channel is to be
    unmanaged and closed). This routine MUST NOT unmanage or close the
    channel given by  channel  .
*/
{
    int bytes_readable;
    extern linetype *nextline;
    static linetype *newline = NULL;
    static linetype *lastline = NULL;
    static char function_name[] = "stdin_input_func";

    if ( ( bytes_readable = ch_get_bytes_readable (channel) ) < 0 )
    {
	(void) fprintf (stderr,
			"Error getting bytes readable on standard input\n");
	return (FALSE);
    }
    if (newline == NULL)
    {
	/*  Allocate storage for another line  */
	if ( ( newline = (linetype *) m_alloc (sizeof *newline) ) == NULL )
	{
	    m_abort (function_name, "newline");
	}
	(*newline).pos = 0;
	(*newline).next = NULL;
    }
    while (bytes_readable > 0)
    {
	/*  Pull in another character  */
	if (ch_read (channel, (*newline).buffer + (*newline).pos, 1) < 1)
	{
	    (void) fprintf (stderr, "Error reading from standard input\n");
	    m_free ( (char *) newline );
	    newline = NULL;
	    return (FALSE);
	}
	if ( (*newline).buffer[(*newline).pos] == '\n' )
	{
	    /*  End of line reached  */
	    (*newline).buffer[(*newline).pos] = '\0';
	    if (nextline == NULL)
	    {
		/*  Create list  */
		nextline = newline;
		lastline = newline;
	    }
	    else
	    {
		/*  Append to list  */
		(*lastline).next = newline;
		lastline = newline;
	    }
	    newline = NULL;
	    return (TRUE);
	}
	if (++(*newline).pos > LINE_BUF_LENGTH)
	{
	    (void) fprintf (stderr, "Too many characters for line buffer\n");
	    (void) fprintf (stderr, "Discarding: %u characters\n",
			    LINE_BUF_LENGTH);
	    (*newline).pos = 0;
	}
	--bytes_readable;
    }
    /*  Not enough data yet  */
    return (TRUE);
}   /*  End Function stdin_input_func  */

static void stdin_close_func (Channel channel, void *info)
/*  This routine will process the closure of the standard input channel.
    The channel object is given by  channel  .
    The arbitrary pointer for the channel will be pointed to by  info  .
    This routine MUST NOT unmanage the channel pointed to by  channel  ,
    the channel will be automatically unmanaged and deleted upon closure
    (even if no close_func is specified).
    Any unread buffered data in the channel will be lost upon closure. The
    call to this function is the last chance to read this buffered data.
    The routine returns nothing.
*/
{
    extern flag stdin_closed;

    stdin_closed = TRUE;
}   /*  End Function stdin_close_func  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
flag arln_read_from_stdin (char *buffer, unsigned int length, char *prompt)
/*  This routine will read a line from the standard input without preventing
    event processing.
    Note that the standard input channel is internally allocated.
    The NULL terminated string will be written to the storage pointed to by
    buffer  . The routine will NOT copy the '\n' newline character into the
    buffer.
    The length of the buffer must be given by  length  .
    The prompt which is to be displayed must be pointed to by  prompt  .Note
    that the prompt is only displayed if the standard input is NOT a disc file.
    The routine returns TRUE on successful reading, else it returns FALSE.
*/
{
    unsigned int buf_pos;
    linetype *line;
    extern flag stdin_closed;
    extern linetype *nextline;
    static flag locked = FALSE;
    static flag to_be_managed = TRUE;
    static char function_name[] = "arln_read_from_stdin";

    /*  First check to see if routine is being called recursively  */
    if (locked)
    {
	(void) fprintf (stderr, "Cannot perform recursive calls\n");
	a_prog_bug (function_name);
    }
    locked = TRUE;
    /*  Open up  ch_stdin  if not already open  */
    if (ch_stdin == NULL)
    {
	ch_open_stdin ();
	if (ch_test_for_io (ch_stdin) != TRUE)
	{
	    (void) fprintf (stderr, "Standard input is not capable of IO\n");
	    a_prog_bug (function_name);
	}
    }
    if (to_be_managed)
    {
	if ( ch_test_for_asynchronous (ch_stdin) )
	{
	    /*  Manage channel  */
	    if (chm_manage (ch_stdin, NULL, stdin_input_func, stdin_close_func,
			    ( flag (*) () ) NULL, ( flag (*) () ) NULL)
		!= TRUE)
	    {
		(void) fprintf (stderr, "Error managing standard input\n");
		a_prog_bug (function_name);
	    }
	}
	to_be_managed = FALSE;
    }
    if (stdin_closed)
    {
	/*  Poll to ensure other connection events are processed  */
	cm_poll_silent (FALSE);
	chm_poll (10);
	locked = FALSE;
	return (FALSE);
    }
    /*  Try reading data  */
    if ( ch_test_for_asynchronous (ch_stdin) )
    {
	(void) fprintf (stderr, "\n%s", prompt);
	/*  Asynchronous channel: look at stored buffer  */
	while ( (nextline == NULL) && (stdin_closed == FALSE) )
	{
	    /*  Poll  */
	    cm_poll_silent (FALSE);
	    chm_poll (-1);
	    if ( s_check_for_int () )
	    {
		/*  User interrupt  */
		(void) fprintf (stderr, "\nUser interrupt. Quitting.\n");
		exit (RV_CONTROL_C);
	    }
	}
	if (stdin_closed)
	{
	    locked = FALSE;
	    return (FALSE);
	}
	/*  Have a line  */
	if ( (*nextline).pos < length )
	{
	    /*  Will fit into buffer  */
	    m_copy (buffer, (*nextline).buffer, (*nextline).pos + 1);
	    line = nextline;
	    nextline = (*nextline).next;
	    m_free ( (char *) line );
	    locked = FALSE;
	    return (TRUE);
	}
	/*  Will have to trim  */
	m_copy (buffer, (*nextline).buffer, length - 1);
	buffer[length] = '\0';
	line = nextline;
	nextline = (*nextline).next;
	m_free ( (char *) line );
	locked = FALSE;
	return (TRUE);
    }
    /*  Disc channel  */
    buf_pos = 0;
    while (ch_read (ch_stdin, buffer + buf_pos, 1) > 0)
    {
	/*  Have another character  */
	if (buffer[buf_pos] == '\n')
	{
	    /*  Have a line  */
	    buffer[buf_pos] = '\0';
	    locked = FALSE;
	    return (TRUE);
	}
	if (++buf_pos > length)
	{
	    (void) fprintf (stderr, "Too many characters for line buffer\n");
	    (void) fprintf (stderr, "Discarding: %u characters\n", length);
	    buf_pos = 0;
	}
    }
    /*  Error reading  */
    stdin_closed = TRUE;
    locked = FALSE;
    return (FALSE);
}   /*  End Function arln_read_stdin  */

/*LINTLIBRARY*/
/*  misc.c

    This code provides miscellaneous channel manipulation routines.

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

/*  This file contains miscellaneous routines for the manipulation of channel
  objects.


    Written by      Richard Gooch   26-MAR-1993

    Updated by      Richard Gooch   26-MAR-1993

    Updated by      Richard Gooch   20-AUG-1993: Moved  ch_gets  and  ch_puts
  from  channel.c  and added  ch_getl  .

    Updated by      Richard Gooch   20-SEP-1993: Corrected documentation for
  ch_gets  and  ch_getl  routines.

    Updated by      Richard Gooch   20-MAY-1994: Added  CONST  declaration
  where appropriate.

    Updated by      Richard Gooch   21-NOV-1994: Tidied up  VERIFY_CHANNEL
  macro.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/ch/misc.c

    Updated by      Richard Gooch   27-SEP-1995: Created <ch_drain> and
  <ch_printf>.

    Updated by      Richard Gooch   31-MAR-1996: Changed documentation style.

    Last updated by Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.


*/
#include <stdio.h>
#include <stdarg.h>
#include <karma.h>
#include <karma_ch.h>
#include <karma_m.h>
#include <karma_a.h>

#define BUF_LEN 4096

#define VERIFY_CHANNEL(ch) if (ch == NULL) \
{(void) fprintf (stderr, "NULL channel passed\n"); \
 a_prog_bug (function_name); }


/*PUBLIC_FUNCTION*/
Channel ch_open_and_fill_memory (char **strings)
/*  [SUMMARY] Create and fill memory channel.
    [PURPOSE] This routine will open a memory channel with sufficient space to
    contain a list of strings.
    <strings> The NULL terminated array of string pointers.
    The strings are written with a NEWLINE character to terminate the string.
    The NULL terminator character is not written.
    [RETURNS] A channel object on success, else NULL.
*/
{
    Channel channel;
    unsigned int buf_size;
    char **ptr;
    static char function_name[] = "ch_open_and_fill_memory";

    if (strings == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Determine size needed  */
    for (ptr = strings, buf_size = 0; *ptr != NULL; ++ptr)
    {
	buf_size += strlen (*ptr) + 1;
    }
    if ( ( channel = ch_open_memory (NULL, buf_size) ) == NULL )
    {
	m_abort (function_name, "memory channel");
    }
    /*  Fill channel  */
    for (ptr = strings; *ptr != NULL; ++ptr)
    {
	if (ch_puts (channel, *ptr, TRUE) != TRUE)
	{
	    (void) fprintf (stderr, "Error writing to memory channel\n");
	    a_prog_bug (function_name);
	}
    }
    return (channel);
}   /*  End Function ch_open_and_fill_memory  */

/*PUBLIC_FUNCTION*/
flag ch_gets (Channel channel, char *buffer, unsigned int length)
/*  [SUMMARY] Read a line from a channel.
    [PURPOSE] This routine will read a character string from a channel into a
    buffer.
    <channel> The channel object.
    <buffer> The buffer to write the data into.
    The routine will write a NULL terminator character at the end of the
    string.
    [NOTE] The newline chanacter '\n' is NOT copied into the buffer.
    <length> The length of the buffer. If the buffer is not large enough to
    contain the string, then the remainder of the string is NOT read. See also
    the [<ch_getl>] routine.
    [RETURNS] TRUE on success, else FALSE (indicating end-of-file was
    encountered).
*/
{
    flag return_value = TRUE;
    flag another = TRUE;
    static char function_name[] = "ch_gets";

    VERIFY_CHANNEL (channel);
    if (buffer == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while (another == TRUE)
    {
	if (ch_read (channel, buffer, 1) < 1)
	{
	    /*  Error occurred  */
	    another = FALSE;
	    return_value = FALSE;
	    continue;
	}
	if (*buffer == '\n')
	{
	    another = FALSE;
	    continue;
	}
	++buffer;
	--length;
	if (length < 2)
	{
	    another = FALSE;
	}
    }
    /*  Write NULL terminator  */
    *buffer = '\0';
    return (return_value);
}   /*  End Function ch_gets  */

/*PUBLIC_FUNCTION*/
flag ch_getl (Channel channel, char *buffer, unsigned int length)
/*  [SUMMARY] Read a line from a channel.
    [PURPOSE] This routine will read a character string from a channel into a
    buffer.
    <channel> The channel object.
    <buffer> The buffer to write the data into.
    The routine will write a NULL terminator character at the end of the
    string.
    [NOTE] The newline chanacter '\n' is NOT copied into the buffer.
    <length> The length of the buffer. If the buffer is not large enough to
    contain the string, then the remainder of the string (including the
    newline character) is read in and discarded and a warning message is
    displayed. See also the [<ch_gets>] routine.
    [RETURNS] TRUE on success, else FALSE (indicating end-of-file was
    encountered).
*/
{
    flag return_value = TRUE;
    flag another = TRUE;
    static char function_name[] = "ch_getl";

    VERIFY_CHANNEL (channel);
    if (buffer == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while (another)
    {
	if (ch_read (channel, buffer, 1) < 1)
	{
	    /*  Error occurred  */
	    another = FALSE;
	    return_value = FALSE;
	    continue;
	}
	if (*buffer == '\n')
	{
	    another = FALSE;
	    continue;
	}
	++buffer;
	--length;
	if (length < 2)
	{
	    another = FALSE;
	    /*  Consume rest of line  */
	    (void) fprintf (stderr, "WARNING: discarding characters: \"");
	    while (*buffer != '\n')
	    {
		if (ch_read (channel, buffer, 1) < 1)
		{
		    /*  Error occurred  */
		    return_value = FALSE;
		    continue;
		}
		else
		{
		    if (*buffer != '\n') (void) fputc (*buffer, stderr);
		}
	    }
	    (void) fprintf (stderr, "\"\n");
	}
    }
    /*  Write NULL terminator  */
    *buffer = '\0';
    return (return_value);
}   /*  End Function ch_getl  */

/*PUBLIC_FUNCTION*/
flag ch_puts (Channel channel, CONST char *string, flag newline)
/*  [SUMMARY] Write a character string to a channel.
    <channel> The channel object.
    <string> The string.
    <newline> If TRUE, the routine will write a NEWLINE character after writing
    the string.
    [NOTE] The routine will not write the NULL terminator character.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned int length;
    char newline_char = '\n';
    static char function_name[] = "ch_puts";

    VERIFY_CHANNEL (channel);
    if (string == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    length = strlen (string);
    if (ch_write (channel, string, length) < length)
    {
	return (FALSE);
    }
    if (newline)
    {
	if (ch_write (channel, &newline_char, 1) < 1)
	{
	    return (FALSE);
	}
    }
    return (TRUE);
}   /*  End Function ch_puts  */

/*PUBLIC_FUNCTION*/
unsigned int ch_drain (Channel channel, unsigned int length)
/*  [SUMMARY] Drain bytes from a channel.
    [PURPOSE] This routine will drain (read) a specified number of bytes from a
    channel, ignoring the data.
    <channel> The Channel object.
    <length> The number of bytes to drain.
    [RETURNS] The number of bytes drained.
*/
{
    unsigned int block, read;
    unsigned int num_drained = 0;
    char buffer[BUF_LEN];

    while (num_drained < length)
    {
	block = length - num_drained;
	if (block > BUF_LEN) block = BUF_LEN;
	read = ch_read (channel, buffer, block);
	num_drained += read;
	if (read < block) return (num_drained);
    }
    return (num_drained);
}   /*  End Function ch_drain  */

/*PUBLIC_FUNCTION*/
flag ch_printf (Channel channel, CONST char *format, ...)
/*  [SUMMARY] Write formatted output to a channel.
    <channel> The channel object.
    <format> The format string. See <<fprintf>>.
    [VARARGS] The optional parameters. See <<fprintf>>.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    va_list argp;
    char buffer[BUF_LEN];

    va_start (argp, format);
    (void) vsprintf (buffer, format, argp);
    va_end (argp);
    return ( ch_puts (channel, buffer, FALSE) );
}   /*  End Function ch_printf  */

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

    Updated by      Richard Gooch   26-MAY-1996: Cleaned code to keep
  gcc -Wall -pedantic-errors happy.

    Updated by      Richard Gooch   8-AUG-1996: Created <ch_fill>,
  <ch_drain_to_boundary> and <ch_fill_to_boundary> routines.

    Last updated by Richard Gooch   10-AUG-1996: Copied from main.c the
  <ch_read_and_swap_blocks> and <ch_swap_and_write_blocks> routines. Modified
  <ch_read_and_swap_blocks> to process in sections.


*/
#include <stdio.h>
#include <stdarg.h>
#include <errno.h>
#include <karma.h>
#include <karma_ch.h>
#include <karma_m.h>
#include <karma_a.h>

#define BUF_LEN 262144
#define SWAP_BUF_SIZE 65536
#define SWAP_READ_LENGTH 4194304  /*  4 MBytes  */

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
    unsigned int block, n_read;
    unsigned int num_drained = 0;
    char buffer[BUF_LEN];

    while (num_drained < length)
    {
	block = length - num_drained;
	if (block > BUF_LEN) block = BUF_LEN;
	n_read = ch_read (channel, buffer, block);
	num_drained += n_read;
	if (n_read < block) return (num_drained);
    }
    return (num_drained);
}   /*  End Function ch_drain  */

/*PUBLIC_FUNCTION*/
unsigned int ch_fill (Channel channel, unsigned int length, char fill_value)
/*  [SUMMARY] Fill a channel with bytes.
    [PURPOSE] This routine will write a specified byte to a channel a number of
    times.
    <channel> The Channel object.
    <length> The number of bytes to write.
    <fill_value> The fill value.
    [RETURNS] The number of bytes written.
*/
{
    unsigned int block, n_write;
    unsigned int num_written = 0;
    char buffer[BUF_LEN];

    /*  Fill buffer  */
    for (block = 0; block < BUF_LEN; ++block) buffer[block] = fill_value;
    while (num_written < length)
    {
	block = length - num_written;
	if (block > BUF_LEN) block = BUF_LEN;
	n_write = ch_write (channel, buffer, block);
	num_written += n_write;
	if (n_write < block) return (num_written);
    }
    return (num_written);
}   /*  End Function ch_fill  */

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

/*PUBLIC_FUNCTION*/
flag ch_drain_to_boundary (Channel channel, uaddr size)
/*  [SUMMARY] Drain bytes from a channel until a specified boundary.
    [PURPOSE] This routine will drain (read) from a channel until the current
    read position is aligned with a boundary.
    channel, ignoring the data.
    <channel> The Channel object.
    <size> The size to align to.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned long read_pos, write_pos;
    extern char *sys_errlist[];
    static char function_name[] = "ch_drain_to_boundary";

    if (size == 0)
    {
	fprintf (stderr, "zero size\n");
	a_prog_bug (function_name);
    }
    if ( !ch_tell (channel, &read_pos, &write_pos) )
    {
	fprintf (stderr, "Error getting position\t%s\n", sys_errlist[errno]);
	return (FALSE);
    }
    if (read_pos % size == 0) return (TRUE);
    size = size - read_pos % size;
    if (ch_drain (channel, size) < size)
    {
	if (errno == 0) return (FALSE);
	fprintf (stderr, "Error draining\t%s\n", sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function ch_drain_to_boundary  */

/*PUBLIC_FUNCTION*/
flag ch_fill_to_boundary (Channel channel, uaddr size, char fill_value)
/*  [SUMMARY] Write bytes to a channel until a specified boundary.
    [PURPOSE] This routine will write bytes to a channel until the current
    write position is aligned with a boundary.
    channel, ignoring the data.
    <channel> The Channel object.
    <size> The size to align to.
    <fill_value> The value to fill with.
    [RETURNS] TRUE on success, else FALSE.
*/
{
    unsigned long read_pos, write_pos;
    extern char *sys_errlist[];
    static char function_name[] = "ch_fill_to_boundary";

    if (size == 0)
    {
	fprintf (stderr, "zero size\n");
	a_prog_bug (function_name);
    }
    if ( !ch_tell (channel, &read_pos, &write_pos) )
    {
	fprintf (stderr, "Error getting position\t%s\n", sys_errlist[errno]);
	return (FALSE);
    }
    if (write_pos % size == 0) return (TRUE);
    size = size - write_pos % size;
    if (ch_fill (channel, size, fill_value) < size)
    {
	fprintf (stderr, "Error draining\t%s\n", sys_errlist[errno]);
	return (FALSE);
    }
    return (TRUE);
}   /*  End Function ch_fill_to_boundary  */

/*EXPERIMENTAL_FUNCTION*/
unsigned int ch_read_and_swap_blocks (Channel channel, char *buffer,
				      unsigned int num_blocks,
				      unsigned int block_size)
/*  [SUMMARY] Read blocks from a channel and swap bytes.
    [PURPOSE] This routine will read a number of blocks from a channel and
    places them into a buffer after swapping (reversing the order).
    <channel> The channel object.
    <buffer> The buffer to write the data into.
    <num_blocks> The number of blocks to read.
    <block_size> The size (in bytes) of each block.
    [NOTE] If the channel is a connection and the number of bytes readable from
    the connection is equal to or more than <<num_blocks * block_size>> the
    routine will NOT block.
    [RETURNS] The number of bytes read. Errors may cause partial blocks to be
    read.
*/
{
    unsigned int num_read;
    unsigned int tot_bytes_read = 0;
    unsigned int length;
    unsigned int blocks_to_read, max_blocks;
    static char function_name[] = "ch_read_and_swap_blocks";

    VERIFY_CHANNEL (channel);
    if (buffer == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Divide the transfer into multiple sections, which reduces the amount of
	disc activity due to swapping. If the sections are too large, the net
	effect is a massive disc-to-disc copy, since a write to virtual memory
	can result in a write to disc if there is insufficient RAM  */
    max_blocks = SWAP_READ_LENGTH / block_size;
    for (; num_blocks > 0; num_blocks -= blocks_to_read, buffer += length)
    {
	blocks_to_read = (num_blocks > max_blocks) ? max_blocks : num_blocks;
	length = blocks_to_read * block_size;
	if ( ( num_read = ch_read (channel, buffer, length) ) < length )
	{
	    return (tot_bytes_read);
	}
	tot_bytes_read += num_read;
	m_copy_and_swap_blocks (buffer, NULL, block_size, 0, block_size,
				blocks_to_read);
    }
    return (tot_bytes_read);
}   /*  End Function ch_read_and_swap_blocks  */

/*EXPERIMENTAL_FUNCTION*/
unsigned int ch_swap_and_write_blocks (Channel channel, CONST char *buffer,
				       unsigned int num_blocks,
				       unsigned int block_size)
/*  [SUMMARY] Write blocks to a channel after swapping bytes.
    [PURPOSE] This routine will write a number of blocks to a channel after
    swapping the bytes.
    <channel> The channel object.
    <buffer> The buffer to read the data from.
    <num_blocks> The number of blocks to write.
    <block_size> The size (in bytes) of each block.
    [RETURNS] The number of bytes written. Errors may cause partial blocks to
    be written.
*/
{
    unsigned int tot_bytes_written, bytes_written, bytes_to_write, nblocks;
    char swap_buffer[SWAP_BUF_SIZE];
    static char function_name[] = "ch_swap_and_write_blocks";

    VERIFY_CHANNEL (channel);
    if (buffer == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    for (tot_bytes_written = 0; num_blocks > 0;
	 num_blocks -= nblocks, buffer += bytes_written)
    {
	nblocks = num_blocks;
	if (nblocks * block_size > SWAP_BUF_SIZE)
	    nblocks = SWAP_BUF_SIZE / block_size;
	m_copy_and_swap_blocks (swap_buffer, buffer, block_size, block_size,
				block_size, nblocks);
	bytes_to_write = nblocks * block_size;
	bytes_written = ch_write (channel, swap_buffer, bytes_to_write);
	tot_bytes_written += bytes_written;
	if (bytes_written < bytes_to_write) return (tot_bytes_written);
    }
    return (tot_bytes_written);
}   /*  End Function ch_swap_and_write_blocks  */

/*LINTLIBRARY*/
/*  idea.c

    This code provides IDEA encryption/decryption for channel objects.

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

/*  This file contains all routines needed for encryption of data passed
  through channels using the IDEA cipher.


    Written by      Richard Gooch   9-APR-1994

    Updated by      Richard Gooch   13-APR-1994

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Updated by      Richard Gooch   26-AUG-1994: Moved  typedef  of
  idea_status  class to header.

    Updated by      Richard Gooch   4-SEP-1994: Rewrote CFB code to do IDEA
  encryption once per IDEA block size: speeds up encryption 8 times.

    Updated by      Richard Gooch   21-NOV-1994: Tidied up  VERIFY_STATUS
  macro.

    Updated by      Richard Gooch   25-NOV-1994: Stripped encryption code and
  renamed to  cen_  package.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/cen/idea.c

    Updated by      Richard Gooch   4-DEC-1994: Improved scrubbing of
  sensitive data.

    Last updated by Richard Gooch   31-MAR-1996: Changed documentation style.


*/
#include <stdio.h>
#include <ctype.h>
#include <karma.h>
#include <karma_cen.h>
#include <karma_ch.h>
#include <karma_m.h>
#include <karma_a.h>
#include <karma_p.h>
#include <os.h>


#define MAGIC_NUMBER 1987532984

#define VERIFY_INFO(st) if (st == NULL) \
{(void) fprintf (stderr, "NULL info passed\n"); \
 a_prog_bug (function_name); } \
if ( (*st).magic_number != MAGIC_NUMBER ) \
{(void) fprintf (stderr, "Invalid info object\n"); \
 a_prog_bug (function_name); }

struct converter_info_type
{
    unsigned int magic_number;
    idea_status read_status;
    idea_status write_status;
};

/*  Declarations of private functions follow  */
STATIC_FUNCTION (unsigned int size_func, (Channel channel, void **info) );
STATIC_FUNCTION (unsigned int read_func, (Channel channel, char *buffer,
					  unsigned int length, void **info) );
STATIC_FUNCTION (unsigned int write_func, (Channel channel, char *buffer,
					   unsigned int length, void **info) );
STATIC_FUNCTION (flag flush_func, (Channel channel, void **info) );
STATIC_FUNCTION (void close_func, (void *info) );


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
ChConverter cen_idea (Channel channel, char read_key[EN_IDEA_KEY_SIZE],
		      char read_init_vector[EN_IDEA_BLOCK_SIZE],
		      char write_key[EN_IDEA_KEY_SIZE],
		      char write_init_vector[EN_IDEA_BLOCK_SIZE], flag clear)
/*  [SUMMARY] Add IDEA encryption to a channel object.
    [PURPOSE] This routine will register converter functions for a channel so
    that all IO will be encrypted using the IDEA cipher in Cipher Feed Back
    mode.
    <read_key> The 16 byte IDEA key used when reading.
    <read_init_vector> The 8 byte initialisation vector used when reading.
    <write_key> The 16 byte IDEA key used when writing.
    <write_init_vector> The 8 byte initialisation vector used when writing.
    <clear> If TRUE the keys and initialisation vectors will be cleared after
    use (highly recommended if they will not be needed again), irrespective of
    whether the routine fails.
    [RETURNS] A ChConverter object on success (which may be used in a call to
    ch_unregister_converter), else NULL.
*/
{
    ChConverter converter;
    struct converter_info_type *converter_info;
    static char function_name[] = "cen_idea";

    FLAG_VERIFY (clear);
    if ( (read_key == NULL) || (read_init_vector == NULL) ||
	(write_key == NULL) || (write_init_vector == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	if (clear)
	{
	    m_clear (read_key, EN_IDEA_KEY_SIZE);
	    m_clear (read_init_vector, EN_IDEA_BLOCK_SIZE);
	    m_clear (write_key, EN_IDEA_KEY_SIZE);
	    m_clear (write_init_vector, EN_IDEA_BLOCK_SIZE);
	}
	a_prog_bug (function_name);
    }
    if ( ( converter_info = (struct converter_info_type *)
	  m_alloc (sizeof *converter_info) ) == NULL )
    {
	m_error_notify (function_name, "channel converter info");
	if (clear)
	{
	    m_clear (read_key, EN_IDEA_KEY_SIZE);
	    m_clear (read_init_vector, EN_IDEA_BLOCK_SIZE);
	    m_clear (write_key, EN_IDEA_KEY_SIZE);
	    m_clear (write_init_vector, EN_IDEA_BLOCK_SIZE);
	}
	return (NULL);
    }
    converter_info->magic_number = MAGIC_NUMBER;
    if ( ( converter_info->read_status = en_idea_init (read_key, TRUE,
							 read_init_vector,
							 FALSE) ) == NULL )
    {
	m_free ( ( char *) converter_info );
	if (clear)
	{
	    m_clear (read_key, EN_IDEA_KEY_SIZE);
	    m_clear (read_init_vector, EN_IDEA_BLOCK_SIZE);
	    m_clear (write_key, EN_IDEA_KEY_SIZE);
	    m_clear (write_init_vector, EN_IDEA_BLOCK_SIZE);
	}
	return (NULL);
    }
    if ( ( converter_info->write_status = en_idea_init (write_key, FALSE,
							  write_init_vector,
							  clear) ) == NULL )
    {
	en_idea_close (converter_info->read_status);
	m_free ( ( char *) converter_info );
	if (clear)
	{
	    m_clear (read_key, EN_IDEA_KEY_SIZE);
	    m_clear (read_init_vector, EN_IDEA_BLOCK_SIZE);
	    m_clear (write_key, EN_IDEA_KEY_SIZE);
	    m_clear (write_init_vector, EN_IDEA_BLOCK_SIZE);
	}
	return (NULL);
    }
    if (clear)
    {
	m_clear (read_key, EN_IDEA_KEY_SIZE);
	m_clear (read_init_vector, EN_IDEA_BLOCK_SIZE);
    }
    if ( ( converter = ch_register_converter (channel, size_func, read_func,
					      write_func, flush_func,
					      close_func, converter_info) )
	== NULL )
    {
	en_idea_close (converter_info->read_status);
	en_idea_close (converter_info->write_status);
	m_free ( ( char *) converter_info );
	return (NULL);
    }
    return (converter);
}   /*  End Function cen_idea  */


/*  Private functions follow  */

static unsigned int size_func (Channel channel, void **info)
/*  This routine will determine the approximate number of bytes that the
    read_func  will return.
    The channel object will be given by  channel  .
    The arbitrary information pointer will be pointed to by  info  .The
    information pointer may be modified.
    The routine returns the number of bytes the  read_func  will return.
*/
{
    struct converter_info_type *converter_info;
    static char function_name[] = "__cen_size_func";

    converter_info = (struct converter_info_type *) *info;
    VERIFY_INFO (converter_info);
    return (0);
}   /*  End Function size_func  */

static unsigned int read_func (Channel channel, char *buffer,
			       unsigned int length, void **info)
/*   This routine will convert bytes being read from a channel object.
     The channel object will be given by  channel  .It is permissable for
     the routine to call  ch_read  with this channel. If this is done,
     this  read_func  will be bypassed.
     The buffer to write the data into will be pointed to by  buffer  .
     The number of bytes to write into the buffer will be pointed to by
     length  .
     The arbitrary information pointer will be pointed to by  info  .The
     information pointer may be modified.
     The routine returns the number of bytes actually written to the buffer.
*/
{
    struct converter_info_type *converter_info;
    static char function_name[] = "__cen_read_func";

    converter_info = (struct converter_info_type *) *info;
    VERIFY_INFO (converter_info);
    if (ch_read (channel, buffer, length) < length) return (0);
    en_idea_cfb (converter_info->read_status, buffer, length);
    return (length);
}   /*  End Function read_func  */

static unsigned int write_func (Channel channel, char *buffer,
				unsigned int length, void **info)
/*  This routine will convert bytes being written to a channel object.
    The channel object will be given by  channel  .It is permissable for
    the routine to call  ch_write  with this channel. If this is done,
    this  write_func  will be bypassed.
    The buffer to read the data from will be pointed to by  buffer  .The
    contents of this buffer may be modified if needed.
    The number of bytes to read from the buffer will be given by  length
    The arbitrary information pointer will be pointed to by  info  .The
    information pointer may be modified.
    The routine returns the number of bytes read from the buffer.
*/
{
    struct converter_info_type *converter_info;
    static char function_name[] = "__cen_write_func";

    converter_info = (struct converter_info_type *) *info;
    VERIFY_INFO (converter_info);
    en_idea_cfb (converter_info->write_status, buffer, length);
    return (ch_write (channel, buffer, length) );
}   /*  End Function write_func  */

static flag flush_func (Channel channel, void **info)
/*  This routine will process a flush request for a channel object.
    The channel object will be given by  channel  .It is permissable for
    the routine to call  ch_write  with this channel. If this is done,
    this  write_func  will be bypassed.
    The arbitrary information pointer will be pointed to by  info  .The
    information pointer may be modified.
    The routine returns TRUE on success, else it returns FALSE.
*/
{
    struct converter_info_type *converter_info;
    static char function_name[] = "__cen_flush_func";

    converter_info = (struct converter_info_type *) *info;
    VERIFY_INFO (converter_info);
    return (TRUE);
}   /*  End Function flush_func  */

static void close_func (void *info)
/*  This routine is called when a channel is closed.
    The arbitrary information pointer will be given by  info  .
    The routine returns nothing.
*/
{
    struct converter_info_type *converter_info;
    static char function_name[] = "__cen_close_func";

    converter_info = (struct converter_info_type *) info;
    VERIFY_INFO (converter_info);
    en_idea_close (converter_info->read_status);
    en_idea_close (converter_info->write_status);
}   /*  End Function close_func  */

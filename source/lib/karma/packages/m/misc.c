/*LINTLIBRARY*/
/*  misc.c

    This code provides simple memory manipulation routines.

    Copyright (C) 1992,1993,1994,1995  Richard Gooch

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

/*  This file contains various memory operation routines for clearing, copying
    and comparing memory.


    Written by      Richard Gooch   12-AUG-1992

    Updated by      Richard Gooch   19-NOV-1992: Added  m_fill  routine.

    Updated by      Richard Gooch   2-JAN-1993: Fixed bug in  m_copy  routine.

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   25-MAY-1993: Added  m_cmp  .

    Updated by      Richard Gooch   19-SEP-1993: Fixed bug in  m_copy_blocks  .

    Updated by      Richard Gooch   20-MAY-1994: Added  CONST  declaration
  where appropriate.

    Updated by      Richard Gooch   15-OCT-1994: Fixed bug in  m_clear  which
  could result in misaligned accesses.

    Updated by      Richard Gooch   6-NOV-1994: Created  m_dup  .

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/m/misc.c

    Last updated by Richard Gooch   7-JAN-1995: Advertised <m_free_scratch>
  routine to documentation generator.


*/
#include <stdio.h>
#include <karma.h>
#include <karma_m.h>


/*  Private data  */
static flag scratch_block_reserved = FALSE;
static uaddr scratch_block_size = 0;
static char *scratch_block_ptr = NULL;


/*  Private functions  */
STATIC_FUNCTION (void prog_bug, (char *function_name) );


/*PUBLIC_FUNCTION*/
void m_clear (char *memory, uaddr length)
/*  [PURPOSE] This routine will clear a block of memory
    <memory> The memory block to clear.
    <length> The length of the block (in bytes) to clear.
    [NOTE] The memory is cleared in long integers and chars.
    [RETURNS] Nothing.
*/
{
    uaddr front_pad;
    uaddr long_count;
    uaddr char_count;
    uaddr num_long;
    uaddr num_char;
    unsigned long *long_ptr;
    unsigned char *char_ptr;
    static char function_name[] = "m_clear";

    if (memory == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	prog_bug (function_name);
    }
    if (length < 1)
    {
	return;
    }
    /*  First clear stray bytes at start of block  */
    front_pad = (uaddr) memory % sizeof (unsigned long);
    front_pad = (front_pad > 0) ? (sizeof (unsigned long) - front_pad) : 0;
    for (char_count = front_pad; (char_count > 0) && (length > 0);
	 --char_count, --length)
	*memory++ = (char) NULL;
    num_long = length / sizeof (unsigned long);
    num_char = length - num_long * sizeof (unsigned long);
    long_ptr = (unsigned long *) memory;
    for (long_count = 0; long_count < num_long; long_count++)
        *long_ptr++ = (unsigned long) NULL;
    char_ptr = (unsigned char *) long_ptr;
    for (char_count = 0; char_count < num_char; ++char_count)
        *char_ptr++ = (unsigned char) NULL;
}   /*  End Function m_clear  */

/*PUBLIC_FUNCTION*/
void m_copy (char *dest, CONST char *source, uaddr length)
/*  [PURPOSE] This routine will copy a block of memory.
    <dest> The destination block of memory.
    <source> The source block of memory.
    <length> The number of bytes to transfer. If this is zero, it is
    permissable for <<source>> and <<dest>> to be NULL.
    [NOTE] The routine copies long integers and chars.
    [RETURNS] Nothing.
*/
{
    uaddr long_count;
    unsigned int char_count;
    uaddr num_long;
    unsigned int num_char;
    unsigned long *source_long;
    unsigned long *dest_long;
    unsigned char *source_char;
    unsigned char *dest_char;
    static char function_name[] = "m_copy";

    if (length < 1)
    {
	return;
    }
    if (dest == NULL)
    {
	(void) fprintf (stderr, "NULL destination pointer passed\n");
	prog_bug (function_name);
    }
    if (source == NULL)
    {
	(void) fprintf (stderr, "NULL source pointer passed\n");
	prog_bug (function_name);
    }
    if ( (uaddr) dest % sizeof (unsigned long) !=
	(uaddr) source % sizeof (unsigned long) )
    {
	/*  Blocks have different misalignment: byte copy  */
	for (; length > 0; --length)
	    *dest++ = *source++;
	return;
    }
    /*  First copy stray bytes at start of blocks  */
    for (char_count = sizeof (unsigned long) -
	 (uaddr) dest % sizeof (unsigned long);
	 (char_count > 0) && (length > 0);
	 --char_count, --length)
	*dest++ = *source++;
    num_long = length / sizeof (unsigned long);
    num_char = length - num_long * sizeof (unsigned long);
    source_long = (unsigned long *) source;
    dest_long = (unsigned long *) dest;
    for (long_count = 0; long_count < num_long; ++long_count)
        *dest_long++ = *source_long++;
    source_char = (unsigned char *) source_long;
    dest_char = (unsigned char *) dest_long;
    for (char_count = 0; char_count < num_char; ++char_count)
        *dest_char++ = *source_char++;
}   /*  End Function m_copy   */

/*PUBLIC_FUNCTION*/
void m_copy_blocks (char *dest, CONST char *source, unsigned int dest_stride,
		    unsigned int source_stride, unsigned int block_size,
		    unsigned int num_blocks)
/*  [PURPOSE] This routine will copy blocks of data.
    <dest> The destination for the first block copy.
    <source> The source for the first block copy.
    <dest_stride> The spacing (in bytes) between destintion blocks.
    <source_stride> The spacing (in bytes) between source blocks.
    <block_size> The size of each block (in bytes).
    <num_blocks> The number of blocks to copy.
    [RETURNS] Nothing.
*/
{
    unsigned int block_count;
    unsigned int blk_size;
    unsigned int long_count;
    unsigned int char_count;
    unsigned int num_long;
    unsigned int num_char;
    CONST unsigned long *source_long;
    unsigned long *dest_long;
    CONST char *source_char;
    char *dest_char;
    static char function_name[] = "m_copy_blocks";

    if ( (source == NULL) || (dest == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	prog_bug (function_name);
    }
    if ( (dest_stride < 1) || (source_stride < 1) )
    {
	(void) fprintf (stderr, "Strides must be greater than zero\n");
	prog_bug (function_name);
    }
    /*  Iterate through the blocks to be copied  */
    for (block_count = 0; block_count < num_blocks;
	 ++block_count, dest += dest_stride, source += source_stride)
    {
	source_char = source;
	dest_char = dest;
	if ( (unsigned long) dest_char % sizeof (unsigned long) !=
	    (unsigned long) source_char % sizeof (unsigned long) )
	{
	    /*  Blocks have different misalignment: byte copy  */
	    for (char_count = 0; char_count < block_size; ++char_count)
	         *dest_char++ = *source_char++;
	    continue;
	}
	/*  First copy stray bytes at start of block  */
	blk_size = block_size;
	for (char_count = sizeof (unsigned long) -
	     (unsigned long) dest_char % sizeof (unsigned long);
	     (char_count > 0) && (blk_size > 0);
	     --char_count, --blk_size)
	    *dest_char++ = *source_char++;
	/*  Copy next section of block using long copies  */
	num_long = blk_size / sizeof (unsigned long);
	num_char = blk_size - num_long * sizeof (unsigned long);
	source_long = (unsigned long *) source_char;
	dest_long = (unsigned long *) dest_char;
	for (long_count = 0; long_count < num_long; long_count++)
	    *dest_long++ = *source_long++;
	source_char = (char *) source_long;
	dest_char = (char *) dest_long;
	/*  Copy last section of block using byte copies  */
	for (char_count = 0; char_count < num_char; char_count++)
	    *dest_char++ = *source_char++;
    }  
}   /*  End Function m_copy_blocks  */

/*PUBLIC_FUNCTION*/
void m_fill (char *dest, uaddr stride, CONST char *source,
	     uaddr size, unsigned int num)
/*  [PURPOSE] This routine will fill memory blocks with a specified value.
    <dest> The destination.
    <stride> The stride (in bytes) of destination blocks.
    <source> The fill block.
    <size> The size (in bytes) of the fill value block.
    <num> The number of destination blocks to fill.
    [RETURNS] Nothing.
*/
{
    uaddr byte_count;
    static char function_name[] = "m_fill";

    if ( (dest == NULL) || (source == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	prog_bug (function_name);
    }
    if (stride < size)
    {
	(void) fprintf (stderr, "stride: %u must not be less than size: %u\n",
			stride, size);
	prog_bug (function_name);
    }
    while (num-- > 0)
    {
	for (byte_count = 0; byte_count < size; ++byte_count)
	{
	    dest[byte_count] = source[byte_count];
	}
	dest += stride;
    }
}   /*  End Function m_fill  */

/*PUBLIC_FUNCTION*/
flag m_cmp (CONST char *block1, CONST char *block2, uaddr length)
/*  [PURPOSE] This routine will compare two blocks of memory.
    <block1> The first memory block.
    <block2> The second memory block.
    <length> The number of bytes to compare.
    [NOTE] The routine compares long integers and chars.
    [RETURNS] TRUE if the blocks are equal, else FALSE.
*/
{
    uaddr long_count;
    unsigned int char_count;
    uaddr num_long;
    unsigned int num_char;
    CONST unsigned long *block2_long;
    CONST unsigned long *block1_long;
    CONST unsigned char *block2_char;
    CONST unsigned char *block1_char;
    static char function_name[] = "m_cmp";

    if ( (block2 == NULL) || (block1 == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	prog_bug (function_name);
    }
    if (length < 1)
    {
	(void) fprintf (stderr, "length  must be greater than zero\n");
	prog_bug (function_name);
    }
    if ( (uaddr) block1 % sizeof (unsigned long) !=
	(uaddr) block2 % sizeof (unsigned long) )
    {
	/*  Blocks have different misalignment: byte compare  */
	for (; length > 0; --length)
	{
	    if (*block1++ != *block2++) return (FALSE);
	}
	return (TRUE);
    }
    /*  First compare stray bytes at start of blocks  */
    for (char_count = sizeof (unsigned long) -
	 (uaddr) block1 % sizeof (unsigned long);
	 (char_count > 0) && (length > 0);
	 --char_count, --length)
    {
	if (*block1++ != *block2++) return (FALSE);
    }
    num_long = length / sizeof (unsigned long);
    num_char = length - num_long * sizeof (unsigned long);
    block2_long = (unsigned long *) block2;
    block1_long = (unsigned long *) block1;
    for (long_count = 0; long_count < num_long; ++long_count)
    {
	if (*block1_long++ != *block2_long++) return (FALSE);
    }
    block2_char = (unsigned char *) block2_long;
    block1_char = (unsigned char *) block1_long;
    for (char_count = 0; char_count < num_char; ++char_count)
    {
	if (*block1_char++ != *block2_char++) return (FALSE);
    }
    return (TRUE);
}   /*  End Function m_cmp   */

/*PUBLIC_FUNCTION*/
char *m_dup (CONST char *original, uaddr size)
/*  [PURPOSE] This routine will duplicate a block of memory into a freshly
    allocated block.
    <original> The original block of memory.
    <size> The size in bytes of the block.
    [RETURNS] A pointer to a freshly allocated block which contains identical
    data as the orginal on success, else NULL.
*/
{
    char *duplicate;

    if ( ( duplicate = m_alloc (size) ) == NULL ) return (NULL);
    m_copy (duplicate, original, size);
    return (duplicate);
}   /*  End Function m_dup   */

/*PUBLIC_FUNCTION*/
char *m_alloc_scratch (uaddr size, char *function_name)
/*  [PURPOSE] This routine will allocate a scratch block of memory, which may
    be re-used by many different routines. The block is reserved until a call
    is made to <<m_free_scratch>>.
    <size> The minimum size in bytes of the scratch block.
    <function_name> If the memory block is already reserved and this is not
    NULL the string is printed and the process aborts.
    If the memory block is already reserved and this is NULL the routine fails
    normally.
    [RETURNS] A pointer on success to a dynamically allocated block of memory
    which is valid until the next call to this routine, else NULL.
*/
{
    extern flag scratch_block_reserved;
    extern uaddr scratch_block_size;
    extern char *scratch_block_ptr;

    if (scratch_block_reserved)
    {
	(void) fprintf (stderr, "Scratch block already reserved\n");
	if (function_name == NULL) return (NULL);
	prog_bug (function_name);
    }
    if (scratch_block_ptr == NULL)
    {
	/*  First time allocation  */
	if ( ( scratch_block_ptr = m_alloc (size) ) == NULL ) return (NULL);
	scratch_block_size = size;
	scratch_block_reserved = TRUE;
	return (scratch_block_ptr);
    }
    if (scratch_block_size >= size)
    {
	scratch_block_reserved = TRUE;
	return (scratch_block_ptr);
    }
    /*  Existing block is too small  */
    m_free (scratch_block_ptr);
    scratch_block_size = 0;
    if ( ( scratch_block_ptr = m_alloc (size) ) == NULL ) return (NULL);
    scratch_block_size = size;
    scratch_block_reserved = TRUE;
    return (scratch_block_ptr);
}   /*  End Function m_alloc_scratch  */

/*PUBLIC_FUNCTION*/
void m_free_scratch ()
/*  [PURPOSE] This routine will free the scratch memory.
    [RETURNS] Nothing.
*/
{
    extern flag scratch_block_reserved;
    static char function_name[] = "m_free_scratch";

    if (!scratch_block_reserved)
    {
	(void) fprintf (stderr, "Scratch block not reserved\n");
	prog_bug (function_name);
    }
    scratch_block_reserved = FALSE;
}   /*  End Function m_free_scratch  */


/*  Private routines follow  */

static void prog_bug (char *function_name)
{
    (void) fprintf (stderr, "Program bug noted in function: %s\n",
		    function_name);
    (void) fprintf (stderr, "Aborting.%c\n", BEL);
    exit (RV_PROGRAM_BUG);
}   /*  End Function prog_bug   */

/*LINTLIBRARY*/
/*PREFIX:"m_"*/
/*   memory.c

    This code provides simple memory manipulation routines.

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

/*  This file contains various memory operation routines for clearing, copying
    and comparing memory.


    Written by      Richard Gooch   12-AUG-1992

    Updated by      Richard Gooch   19-NOV-1992: Added  m_fill  routine.

    Updated by      Richard Gooch   2-JAN-1993: Fixed bug in  m_copy  routine.

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   25-MAY-1993: Added  m_cmp  .

    Last updated by Richard Gooch   19-SEP-1993: Fixed bug in  m_copy_blocks  .


*/
#include <stdio.h>
#include <karma.h>
#include <karma_m.h>

/*  Private functions  */
static void prog_bug ();

/*PUBLIC_FUNCTION*/
void m_clear (memory, length)
/*  This routine will clear a block of memory pointed to by  memory  .
    The length of the block (in chars) to clear must be given by  length  .
    The memory is cleared in long integers and chars.
    The routine returns nothing.
*/
char *memory;
unsigned int length;
{
    unsigned int long_count;
    unsigned int char_count;
    unsigned int num_long;
    unsigned int num_char;
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
    for (char_count = (unsigned int) memory % sizeof (unsigned long);
	 (char_count > 0) && (length > 0);
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
void m_copy (dest, source, length)
/*  This routine will copy a block of memory pointed to by  source  to the
    block of memory pointed to by  dest  .
    The number of chars to transfer must be given by  length  .If this is zero,
    it is permissable for  source  and  dest  to be NULL.
    The routine copies long integers and chars.
    The routine returns nothing.
*/
char *dest;
char *source;
unsigned int length;
{
    unsigned int long_count;
    unsigned int char_count;
    unsigned int num_long;
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
    if ( (unsigned int) dest % sizeof (unsigned long) !=
	(unsigned int) source % sizeof (unsigned long) )
    {
	/*  Blocks have different misalignment: byte copy  */
	for (; length > 0; --length)
	    *dest++ = *source++;
	return;
    }
    /*  First copy stray bytes at start of blocks  */
    for (char_count = sizeof (unsigned long) -
	 (unsigned int) dest % sizeof (unsigned long);
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
void m_copy_blocks (dest, source, dest_stride, source_stride, block_size,
		    num_blocks)
/*  This routine will copy blocks of data.
    The destination for the first block copy must be pointed to by  dest  .
    The source for the first block copy must be pointed to by  source  .
    The spacing (in bytes) between destintion blocks must be in  dest_stride  .
    The spacing (in bytes) between source blocks must be in  source_stride  .
    The size of each block (in bytes) must be given by  block_size  .
    The number of blocks to copy must be in  num_blocks  .
    The routine returns nothing.
*/
char *dest;
char *source;
unsigned int dest_stride;
unsigned int source_stride;
unsigned int block_size;
unsigned int num_blocks;
{
    unsigned int block_count;
    unsigned int blk_size;
    unsigned int long_count;
    unsigned int char_count;
    unsigned int num_long;
    unsigned int num_char;
    unsigned long *source_long;
    unsigned long *dest_long;
    char *source_char;
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
	if ( (unsigned int) dest_char % sizeof (unsigned long) !=
	    (unsigned int) source_char % sizeof (unsigned long) )
	{
	    /*  Blocks have different misalignment: byte copy  */
	    for (char_count = 0; char_count < block_size; ++char_count)
	         *dest_char++ = *source_char++;
	    continue;
	}
	/*  First copy stray bytes at start of block  */
	blk_size = block_size;
	for (char_count = sizeof (unsigned long) -
	     (unsigned int) dest_char % sizeof (unsigned long);
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
void m_fill (dest, stride, source, size, num)
/*  This routine will fill memory blocks with a specified value.
    The destination must be pointed to by  dest  .
    The stride (in bytes) of destination blocks must be given by  stride  .
    The fill value must be pointed to by  source  .
    The size (in bytes) of the fill value must be given by  size  .
    The number of destination blocks to fill must be given by  num  .
    The routine returns nothing.
*/
char *dest;
unsigned int stride;
char *source;
unsigned int size;
unsigned int num;
{
    unsigned int byte_count;
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
flag m_cmp (block1, block2, length)
/*  This routine will compare a block of memory pointed to by  block2  to the
    block of memory pointed to by  block1  .
    The number of chars to compare must be given by  length  .
    The routine copies long integers and chars.
    The routine returns TRUE if the blocks are equal, else it returns FALSE.
*/
char *block1;
char *block2;
unsigned int length;
{
    unsigned int long_count;
    unsigned int char_count;
    unsigned int num_long;
    unsigned int num_char;
    unsigned long *block2_long;
    unsigned long *block1_long;
    unsigned char *block2_char;
    unsigned char *block1_char;
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
    if ( (unsigned int) block1 % sizeof (unsigned long) !=
	(unsigned int) block2 % sizeof (unsigned long) )
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
	 (unsigned int) block1 % sizeof (unsigned long);
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

static void prog_bug (function_name)
char *function_name;
{
    (void) fprintf (stderr, "Program bug noted in function: %s\n",
		    function_name);
    (void) fprintf (stderr, "Aborting.%c\n", BEL);
    exit (RV_PROGRAM_BUG);
}   /*  End Function prog_bug   */

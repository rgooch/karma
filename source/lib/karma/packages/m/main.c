/*LINTLIBRARY*/
/*  main.c

    This code provides memory allocation routines which check for array overrun

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

/*


    Written by      Richard Gooch   16-JUL-1994: Scrapped old version.

    Updated by      Richard Gooch   21-JUL-1994: Fixed bug in  m_free  which
  used incorrect head and tail offsets.

    Updated by      Richard Gooch   1-AUG-1994: Check for NULL pointer passed
  to  m_free  .

    Updated by      Richard Gooch   10-AUG-1994: Added support for
  M_ALLOC_MAX_CHECK_INTERVAL  environment variable.

    Updated by      Richard Gooch   7-SEP-1994: Fixed bug in
  verify_memory_integrity  which reported all blocks in the list after the
  first corrupted block as being corrupted too.

    Updated by      Richard Gooch   5-OCT-1994: Added test in  m_alloc  to
  ensure C type  char  is indeed signed. Stupid AIX/rs6000 compiler requires a
  compile flag.

    Updated by      Richard Gooch   3-NOV-1994: Fixed  verify_memory_integrity
  to always return a value.

    Updated by      Richard Gooch   4-NOV-1994: Made  verify_memory_integrity
  public as  m_verify_memory_integrity  .

    Updated by      Richard Gooch   6-NOV-1994: Changed to ANSI function
  definitions.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/m/main.c

    Updated by      Richard Gooch   21-JAN-1995: Made routines MT-Safe.

    Updated by      Richard Gooch   7-APR-1995: Moved #include <sys/types.h>
  from header file.

    Updated by      Richard Gooch   7-MAY-1995: Placate gcc -Wall

    Updated by      Richard Gooch   9-JUN-1995: Attempt to fix for crayPVP.

    Updated by      Richard Gooch   1-JAN-1996: Documented M_ALLOC_FAST
  environment variable.

    Updated by      Richard Gooch   12-APR-1996: Changed to new documentation
  format.

    Updated by      Richard Gooch   5-MAY-1996: Added notification of
  allocation failure when M_ALLOC_DEBUG is TRUE.

    Last updated by Richard Gooch   2-AUG-1996: Documented
  M_ALLOC_MAX_CHECK_INTERVAL environment variable.


*/
#ifdef OS_Solaris
#  ifndef _REENTRANT
#    define _REENTRANT
#  endif
#  include <thread.h>
#endif
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>
#include <string.h>
#include <sys/types.h>
#include <os.h>
#include <karma.h>
#include <karma_m.h>

#define CHAR_MALLOC(s) (char *) malloc ( (s) )


#ifdef OS_Solaris
#  define LOCK mutex_lock (&global_lock)
#  define UNLOCK mutex_unlock (&global_lock)
#endif

#ifndef LOCK
#  define LOCK
#  define UNLOCK
#endif


#define INITIAL_CHECK_COUNT 10
#define MAX_CHECK_COUNT 1000
#define MIN_BOUNDARY_SIZE ( sizeof (double) )
#define HEAD_SIZE ( sizeof (unsigned int) )
#define TAIL_SIZE 4
#define MIN_HEAD_SIZE (sizeof (struct mem_header) + HEAD_SIZE)
#define HEAD_MAGIC_NUMBER (unsigned int) 1478328934
#define TAIL_MAGIC_NUMBER0 (unsigned char) 0xde
#define TAIL_MAGIC_NUMBER1 (unsigned char) 0x7d
#define TAIL_MAGIC_NUMBER2 (unsigned char) 0x03
#define TAIL_MAGIC_NUMBER3 (unsigned char) 0x98


/*  When a memory block is requested of  m_alloc  the actual memory requested
    of  malloc  contains (note: there is no padding unless explicitely stated):

      BEFORE the memory block
        a struct mem_header
	zero or more bytes of padding
	(unsigned int) head magic number
      the memory block aligned on the first MIN_BOUNDARY_SIZE boundary
      AFTER the memory block
        4 bytes tail magic number

  On a 32 bit machine, the total allocation overhead should be 20 bytes
  On a 64 bit machine, the total allocation overhead should be 36 bytes
*/

struct mem_header
{
    struct mem_header *next;
    struct mem_header *prev;
    uaddr size;              /*  Length of block requested by application  */
};


/*  Private data  */
static uaddr total_allocated = 0;
static struct mem_header *block_list = NULL;
#ifdef OS_Solaris
static mutex_t global_lock;
#endif

#ifdef HAS_ENVIRON
extern char *getenv ();
#else
static char *getenv ();
#endif


/*  Private functions  */
static void prog_bug ();
static int string_icmp ();
static char *string_upr ();
static flag debug_required ();
static flag fast_alloc_required ();

#if !defined(HAS_INTERNATIONALISATION) && !defined(toupper)
static char toupper (c)
char c;
{
    return (c - 'a' + 'A');
}
#endif


/*  Public routines follow  */

/*PUBLIC_FUNCTION*/
char *m_alloc (uaddr size)
/*  [SUMMARY] Allocate Virtual Memory.
    <size> The number of bytes to allocate.
    [MT-LEVEL] Safe under Solaris 2.
    [RETURNS] A pointer to the memory on success, else NULL.
    [NOTE] If the environment variable "M_ALLOC_DEBUG" is set to "TRUE" then
    the routine will print allocation debugging information.
    [NOTE] If the environment variable "M_ALLOC_FAST" is set to "TRUE" then NO
    periodic integrity check of memory is performed and no debugging
    [NOTE] The "M_ALLOC_MAX_CHECK_INTERVAL" environment variable controls the
    maximum interval between integrity checks.
    information will be printed.
*/
{
    unsigned char test_val = 0x81;
    int pad_bytes, tot_head_size;
    char *ptr;
    unsigned char *tail_ptr;
    struct mem_header *header;
    extern uaddr total_allocated;
    extern struct mem_header *block_list;
    static char first_time = TRUE;

#ifndef MACHINE_crayPVP
    LOCK;
    if (first_time)
    {
	first_time = FALSE;
	ptr = (char *) &test_val;
	if (*ptr != -127)
	{
	    (void) fprintf (stderr,
			    "Type:  char  is not a proper signed type\n");
	    (void) fprintf (stderr,
			    "Library should be recompiled with appropriate");
	    (void) fprintf (stderr, " flag. Exiting.\n");
	    exit (RV_UNDEF_ERROR);
	}
    }
    UNLOCK;
#endif
    /*  Check fast flag  */
    if ( fast_alloc_required () )
    {
	if ( ( ptr = CHAR_MALLOC (size) ) == NULL )
	{
	    if ( !debug_required () ) return (NULL);
	    (void) fprintf (stderr, "Allocation failure for: %lu bytes\n",
			    size);
	    return (NULL);
	}
	return (ptr);
    }
    (void) m_verify_memory_integrity (FALSE);
    pad_bytes = MIN_HEAD_SIZE % MIN_BOUNDARY_SIZE;
    if (pad_bytes > 0) pad_bytes = MIN_BOUNDARY_SIZE - pad_bytes;
    tot_head_size = MIN_HEAD_SIZE + pad_bytes;
    if ( ( ptr = CHAR_MALLOC (tot_head_size + size + TAIL_SIZE) ) == NULL )
    {
	(void) fprintf (stderr,
			"Unable to allocate memory, size = %lu bytes\n",
			size);
	return (NULL);
    }
    header = (struct mem_header *) ptr;
    /*  Add to list  */
    LOCK;
    if (block_list != NULL) (*block_list).prev = header;
    (*header).next = block_list;
    block_list = header;
    UNLOCK;
    (*header).prev = NULL;
    (*header).size = size;
    *(unsigned int *) (ptr + sizeof *header + pad_bytes) = HEAD_MAGIC_NUMBER;
    tail_ptr = (unsigned char *) ptr + tot_head_size + size;
    tail_ptr[0] = TAIL_MAGIC_NUMBER0;
    tail_ptr[1] = TAIL_MAGIC_NUMBER1;
    tail_ptr[2] = TAIL_MAGIC_NUMBER2;
    tail_ptr[3] = TAIL_MAGIC_NUMBER3;
    total_allocated += size;
    if ( debug_required () )
    {
	(void) fprintf (stderr, "Allocated: %-20lu total: %-20lu ptr: %p\n",
			size, total_allocated, ptr + tot_head_size);
    }
    return (ptr + tot_head_size);
}   /*  End Function m_alloc  */

/*PUBLIC_FUNCTION*/
void m_free (char *ptr)
/*  [SUMMARY] Free Virtual Memory.
    <ptr> The start of a previously allocated block of memory to be freed.
    [MT-LEVEL] Safe.
    [RETURNS] Nothing.
    [NOTE] If the environment variable "M_ALLOC_DEBUG" is set to "TRUE" then
    the routine will print deallocation debugging information.
*/
{
    flag trashed = FALSE;
    int pad_bytes, tot_head_size;
    unsigned char *tail_ptr;
    struct mem_header *header;
    extern uaddr total_allocated;
    extern struct mem_header *block_list;
    static char function_name[] = "m_free";

    /*  Check fast flag  */
    if (ptr == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	prog_bug (function_name);
    }
    if ( fast_alloc_required () )
    {
	(void) free (ptr);
	return;
    }
#ifndef MACHINE_crayPVP  /*  I might manage to get away with this  */
    if ( (unsigned long) ptr % MIN_BOUNDARY_SIZE != 0 )
    {
	(void) fprintf (stderr, "Non aligned block: %p\n", ptr);
	prog_bug (function_name);
    }
#endif
    pad_bytes = MIN_HEAD_SIZE % MIN_BOUNDARY_SIZE;
    if (pad_bytes > 0) pad_bytes = MIN_BOUNDARY_SIZE - pad_bytes;
    tot_head_size = MIN_HEAD_SIZE + pad_bytes;
    header = (struct mem_header *) (ptr - tot_head_size);
    if (*(unsigned int *) (ptr - HEAD_SIZE) != HEAD_MAGIC_NUMBER)
    {
	trashed = TRUE;
	(void) fprintf (stderr, "Invalid check field in front of block: %p\n",
			ptr);
    }
    tail_ptr = (unsigned char *) ptr + (*header).size;
    if ( (tail_ptr[0] != TAIL_MAGIC_NUMBER0) ||
	(tail_ptr[1] != TAIL_MAGIC_NUMBER1) ||
	(tail_ptr[2] != TAIL_MAGIC_NUMBER2) ||
	(tail_ptr[3] != TAIL_MAGIC_NUMBER3) )
    {
	trashed = TRUE;
	(void) fprintf (stderr, "Invalid check field after block: %p\n", ptr);
    }
    /*  The order of the test below is significant  */
    if ( (m_verify_memory_integrity (trashed) < 1) && trashed )
    {
	(void) fprintf (stderr, "Check of all known blocks is fine: maybe ");
	(void) fprintf (stderr, "block at: %p does not exist?\n", ptr);
    }
    if (trashed)
    {
	(void) fprintf (stderr,
			"Attempted free of block at: %p ignored for purposes",
			ptr);
	(void) fprintf (stderr, " of total allocation count\n");
    }
    else
    {
	total_allocated -= (*header).size;
	if ( debug_required () )
	{
	    (void) fprintf (stderr,
			    "Freed    : %-20lu total: %-20lu ptr: %p\n",
			    (*header).size, total_allocated, ptr);
	}
    }
    /*  Remove from list  */
    LOCK;
    if ( (*header).next != NULL ) (* (*header).next ).prev = (*header).prev;
    if ( (*header).prev == NULL )
    {
	/*  First entry in list  */
	block_list = (*header).next;
    }
    else
    {
	(* (*header).prev ).next = (*header).next;
    }
    UNLOCK;
    *(unsigned int *) (ptr - HEAD_SIZE) = 0;
    tail_ptr[0] = 0;
    tail_ptr[1] = 0;
    tail_ptr[2] = 0;
    tail_ptr[3] = 0;
    (*header).size = 0;
    (void) free ( (char *) header );
}   /*  End Function m_free  */

/*PUBLIC_FUNCTION*/
void m_error_notify (char *function_name, char *purpose)
/*  [SUMMARY] Print memory error notification message.
    <function_name> The name of the function requesting the memory.
    <purpose> The purpose for the memory allocation.
    [RETURNS] Nothing.
*/
{
    (void) fprintf (stderr,
		    "Error allocating memory for: %s in function: %s\n",
		    purpose, function_name);
}   /*  End Function m_error_notify   */

/*PUBLIC_FUNCTION*/
void m_abort (char *name, char *reason)
/*  [SUMMARY] Print memory error notification message and abort.
    <function_name> The name of the function requesting the memory.
    <reason> The reason for the memory allocation.
    [RETURNS] Nothing. The process aborts.
*/
{
    static char function_name[] = "m_abort";

    if (name == NULL)
    {
	(void) fprintf (stderr, "Parameter name is NULL\n");
	prog_bug (function_name);
    }
    if (reason == NULL)
    {
	(void) fprintf (stderr, "Parameter reason is NULL\n");
	prog_bug (function_name);
    }
    (void) fprintf (stderr,
		    "Error allocating memory for: %s  for function: %s%c\n",
		    reason, name, BEL);
    (void) fprintf (stderr, "Aborting.\n");
    exit (RV_MEM_ERROR);
}   /*  End Function m_abort  */  

/*PUBLIC_FUNCTION*/
unsigned int m_verify_memory_integrity (flag force)
/*  [SUMMARY] Periodically verify memory integrity.
    <force> If TRUE, the check is forced, else the check is made periodically.
    [MT-LEVEL] Safe.
    [RETURNS] The number of corrupted blocks.
*/
{
    flag front_trashed;
    flag back_trashed;
    flag printed_preamble = FALSE;
    int pad_bytes, tot_head_size;
    unsigned int num_bad_blocks = 0;
    char *ptr, *env;
    unsigned char *tail_ptr;
    struct mem_header *header;
    extern struct mem_header *block_list;
    static flag first_time = TRUE;
    static int check_count = 0;
    static int check_cycle = INITIAL_CHECK_COUNT;
    static unsigned int max_check_interval = MAX_CHECK_COUNT;

    LOCK;
    if (first_time)
    {
	first_time = FALSE;
	if ( ( env = getenv ("M_ALLOC_MAX_CHECK_INTERVAL") ) != NULL )
	{
	    max_check_interval = atoi (env);
	    check_cycle = max_check_interval;
	    (void) fprintf (stderr, "Memory check interval: %u\n",
			    max_check_interval);
	}
    }
    if (!force)
    {
	if (++check_count < check_cycle)
	{
	    UNLOCK;
	    return (0);
	}
	if (check_cycle * 2 < max_check_interval) check_cycle *= 2;
    }
    check_count = 0;
    pad_bytes = MIN_HEAD_SIZE % MIN_BOUNDARY_SIZE;
    if (pad_bytes > 0) pad_bytes = MIN_BOUNDARY_SIZE - pad_bytes;
    tot_head_size = MIN_HEAD_SIZE + pad_bytes;
    /*  Loop through all the blocks  */
    for (header = block_list; header != NULL; header = (*header).next)
    {
	front_trashed = FALSE;
	back_trashed = FALSE;
	ptr = (char *) header;
	if (*(unsigned int *) (ptr + sizeof *header + pad_bytes)
	    != HEAD_MAGIC_NUMBER)
	{
	    front_trashed = TRUE;
	}
	tail_ptr = (unsigned char *) ptr + tot_head_size + (*header).size;
	if ( (tail_ptr[0] != TAIL_MAGIC_NUMBER0) ||
	    (tail_ptr[1] != TAIL_MAGIC_NUMBER1) ||
	    (tail_ptr[2] != TAIL_MAGIC_NUMBER2) ||
	    (tail_ptr[3] != TAIL_MAGIC_NUMBER3) )
	{
	    back_trashed = TRUE;
	}
	if ( (front_trashed || back_trashed) && !printed_preamble )
	{
	    (void)fprintf(stderr,
			  "Overwriting past memory block bounds has occurred\n");
	    (void) fprintf (stderr,
			    "List of blocks with corrupted front and back guards follows:\n\n");
	    printed_preamble = TRUE;
	}
	if (front_trashed || back_trashed)
	{
	    (void) fprintf (stderr,
			    "Block at: %p size: %-20lufront %-12sback %s\n",
			    ptr + tot_head_size, (*header).size,
			    front_trashed ? "corrupted" : "untouched",
			    back_trashed ? "corrupted" : "untouched");
	    ++num_bad_blocks;
	}
    }
    if (num_bad_blocks > 0)
    {
	(void) fprintf (stderr, "Aborting.\n");
	abort ();
    }
    UNLOCK;
    return (num_bad_blocks);
}   /*  End Function m_verify_memory_integrity  */


/*  Private routines follow  */

static void prog_bug (char *function_name)
{
    (void) fprintf (stderr, "Program bug noted in function: %s\n",
		    function_name);
    (void) fprintf (stderr, "Aborting.%c\n", BEL);
    abort ();
}   /*  End Function prog_bug   */

static int string_icmp (char *string1, char *string2)
/*  This routine will compare two strings pointed to by  string1  and
    string2  .The operation of this routine is similar to that of  strcmp
    ,except that the comparison is case insensitive.
    The comparison value is returned.
*/
{
    int ret_value;
    char *str1 = NULL;
    char *str2 = NULL;
    static char function_name[] = "string_icmp";

    if (string1 == string2)
    {
	return (0);
    }
    if (string1 == NULL)
    {
	return (-1);
    }
    if (string2 == NULL)
    {
	return (1);
    }
    if ( ( str1 = CHAR_MALLOC (strlen (string1) + 1) ) == NULL )
    {
	m_abort (function_name, "copy of string1");
    }
    (void) strcpy (str1, string1);
    (void) string_upr (str1);

    if ( ( str2 = CHAR_MALLOC (strlen (string2) + 1) ) == NULL )
    {
	m_abort (function_name, "copy of string2");
    }
    (void) strcpy (str2, string2);
    (void) string_upr (str2);
    ret_value = strcmp (str1, str2);
    free (str1);
    free (str2);
    return (ret_value);
}   /*  End Function string_icmp   */

static char *string_upr (char *string)
/*  This routine will convert the string pointed to by  string  to uppercase.
    The routine returns the address of the string.
*/
{
    while (*string != '\0')
    {
	if (islower (*string) != 0)
	{
	    *string = toupper (*string);
	}
        ++string;
    }
    return (string);
}   /*  End Function string_upr */

static flag debug_required ()
/*  [PURPOSE] This routine will determine if debugging information is required.
    [MT-LEVEL] Safe.
    [RETURNS] TRUE if debugging information is required, else FALSE.
*/
{
    char *env;
    static flag checked = FALSE;
    static flag debug = FALSE;

    LOCK;
    if (checked)
    {
	UNLOCK;
	return (debug);
    }
    checked = TRUE;
    /*  Determine if debug needed  */
    if ( ( ( env = getenv ("M_ALLOC_DEBUG") ) != NULL ) &&
	(string_icmp (env, "TRUE") == 0) )
    {
	debug = TRUE;
	(void) fprintf (stderr,
			"Running m_alloc and m_free with debugging\n");
    }
    UNLOCK;
    return (debug);
}   /*  End Function debug_required  */

static flag fast_alloc_required ()
/*  [PURPOSE] This routine will determine if fast allocation is required.
    [MT-LEVEL] Safe.
    [RETURNS] TRUE if fast allocation is required, else FALSE.
*/
{
    char *env;
    static flag checked = FALSE;
    static flag fast_alloc = FALSE;

    LOCK;
    if (checked)
    {
	UNLOCK;
	return (fast_alloc);
    }
    checked = TRUE;
    /*  Determine if fast alloc needed  */
    if ( ( ( env = getenv ("M_ALLOC_FAST") ) != NULL ) &&
	(string_icmp (env, "TRUE") == 0) )
    {
	fast_alloc = TRUE;
	(void) fprintf (stderr,
			"Running m_alloc and m_free without checking\n");
    }
    UNLOCK;
    return (fast_alloc);
}   /*  End Function fast_alloc_required  */

#ifndef HAS_ENVIRON
static char *getenv (char *name)
/*  This routine will get the value of the environment variable with name
    pointed to by  name  .
    The routine returns a pointer to the value string if present,
    else it returns NULL.
*/
{
    int length;
    char **env;
    char cmp[STRING_LENGTH];
    extern char **environ;

    if (environ == NULL)
    {
	return (NULL);
    }
    (void) strcpy (cmp, name);
    length = strlen (cmp);
    if (cmp[length] != '=')
    {
	cmp[length] = '=';
	cmp[length + 1] = '\0';
	++length;
    }
    for (env = environ; *env != NULL; ++env)
    {
	if (strncmp (cmp, *env, length) == 0)
	{
	    return (*env + length);
	}
    }
    return (NULL);
}   /*  End Function getenv  */
#endif

/*LINTLIBRARY*/
/*PREFIX:"m_"*/
/*  mem_alloc.c

    This code provides memory allocation routines which check for array overrun

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

/*


    Updated by      Richard Gooch   12-AUG-1992

    Updated by      Richard Gooch   14-NOV-1992

    Updated by      Richard Gooch   22-DEC-1992: Added debugging option.

    Updated by      Richard Gooch   31-DEC-1992: Added local  getenv  .

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   6-JAN-1993: Improved portability of
  toupper  patch routine declaration.

    Updated by      Richard Gooch   28-JUL-1993: Made debugging information
  more verbose.

    Last updated by Richard Gooch   5-SEP-1993: Documented debugging control.


*/

/* DEC/CMS REPLACEMENT HISTORY, Element MEM_ALLOC.C*/
/*#include errno
 *#include ssdef
 *#include libdef
 */

#include <stdio.h>
#include <ctype.h>
#include <os.h>
#include <karma.h>
#include <karma_m.h>

#define INITIAL_CHECK_COUNT 10
#define MAX_CHECK_COUNT 1000000000
#define PAD_SIZE 16



static unsigned int check_count = INITIAL_CHECK_COUNT;
static unsigned int total_allocated = 0;

static struct alloc
{   struct alloc *next,*last ;
    int size ;
    char scrap[4] ;
}first_alloc={0,0,PAD_SIZE,"Don"},last_alloc={0,0,PAD_SIZE,"Don"},
*alloc_high , *alloc_low ;
static int alloc_count=0,alloc_biggest,alloc_smallest ;

char *malloc() ;
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
static check_alloc ();
static check_alloc_pt ();

#if !defined(HAS_INTERNATIONALISATION) && !defined(toupper)
static char toupper (c)
char c;
{
    return (c - 'a' + 'A');
}
#endif


/*PUBLIC_FUNCTION*/
char *m_alloc (size)
/*  Allocate  size  bytes of Virtual Memory.
    If the environment variable "M_ALLOC_DEBUG" is set to "TRUE" then the
    routine will print allocation debugging information.
    The routine returns a pointer to the memory on success,
    else it returns NULL.
*/
unsigned int size ;
{
    char *env;
    char *return_value;
    char           *chpt;
    struct alloc   *pt, *last;
    int             status;
    unsigned        sz, scrap;
    extern unsigned int check_count;
    extern unsigned int total_allocated;
    static int      count = 0;	/* used to control occasional full checks */
    static char function_name[] = "m_alloc";

    /*  Check fast flag  */
    if ( fast_alloc_required () )
    {
	return ( malloc (size) );
    }
    /* Check that list is intact (occasional full checks). */

    count = (count + 1) % check_count;
    if (!check_alloc(FALSE, count == 0))
    {
	/*  Reset check count  */
	check_count = INITIAL_CHECK_COUNT;
	(void) fprintf (stderr,
			"Memory allocation block has been overwritten\n");
	check_alloc(TRUE, TRUE);
	exit (RV_MEM_ERROR);
    }
    else
    {
	/*  Double check count  */
	if (check_count * 2 <= MAX_CHECK_COUNT)
	{
	    check_count *= 2;
	}
    }
    size += PAD_SIZE;	      /* make space for PAD_SIZE bytes identifier */
    sz = ((size + 7) / 8 * 8);
    scrap = sz - size;

    /*
     * status=lib$get_vm(&size,&pt) ; 
     */
    ;
    if ( ( pt = (struct alloc *) malloc (sz) ) != NULL )
    {
	total_allocated += (size - PAD_SIZE);
    }

    /* if(status==SS$_NORMAL) */
    if (pt != NULL)
    {
	if ( debug_required () )
	{
	    (void) fprintf (stderr, "Allocated: %u    total: %u    ptr: %p\n",
			    size - PAD_SIZE, total_allocated,
			    (char *) pt + PAD_SIZE);
	}
	if (alloc_count == 0)
	{
	    first_alloc.next = pt;
	    last_alloc.last = pt;
	    pt->last = &first_alloc;
	    pt->next = &last_alloc;
	    alloc_high = alloc_low = pt;
	    alloc_biggest = alloc_smallest = size;
	    strncpy(first_alloc.scrap, "Donald", 4);
	    strncpy(last_alloc.scrap, "Donald", 4);
	} else
	{
	    pt->last = last_alloc.last;
	    pt->last->next = pt;
	    pt->next = &last_alloc;
	    last_alloc.last = pt;
	    alloc_high = alloc_high > pt ? alloc_high : pt;
	    alloc_low = alloc_low < pt ? alloc_low : pt;
	    alloc_biggest = alloc_biggest > size ? alloc_biggest : size;
	    alloc_smallest = alloc_smallest < size ? alloc_smallest : size;
	}
	pt->size = size;
	alloc_count++;
	chpt = (char *) pt;
	if (scrap)
	    strncpy(chpt + size, "Donald", scrap);
	return (chpt + PAD_SIZE);
    } else
    {				/* errno=status ; ** save error status */
	(void) fprintf (stderr, "Unable to allocate memory,  size=%d bytes\n",
			size);
	/* print_message(errno) ; */
	return (0);
    }
}


static check_alloc(wantpr,complete)
int wantpr,complete ;
{
    struct alloc   *pt, *last;
    int             i, ok = TRUE;
    int             sz, scrap;
    char           *chpt;

    if (alloc_count < 1)
    {
	if (wantpr)
	    (void) fprintf (stderr, "No memory allocations\n");
	return (TRUE);
    }
    if (wantpr || complete)
    {
	if (wantpr) (void) fprintf (stderr,
				    "    Address        Next        Last        Size\n");

	for (pt = &first_alloc, last = 0, i = -1; pt;
	     i++, last = pt, pt = pt->next)
	{
	    if (i > alloc_count)
	    {
		(void) fprintf (stderr, "Chain too long\n");
		ok = FALSE;
		break;
	    }
	    if (wantpr)
		(void) fprintf (stderr, "%10x  %10x  %10x  %10d.\n",
				pt, pt->next, pt->last, pt->size);
	    if (!(ok = check_alloc_pt(pt)))
		break;
	    sz = ((pt->size + 7) / 8 * 8);
	    scrap = sz - pt->size;
	    if (scrap)
	    {
		chpt = (char *) pt;
		chpt += pt->size;
		if (strncmp(chpt, "Donald", scrap))
		{
		    if (wantpr)
			(void) fprintf (stderr, " -- overflowed");
		    else
			(void) fprintf (stderr,
					"Error at %d; data has overflowed allocation.\n",
				       pt);
		}
	    }
	}


	if (ok || !wantpr)
	    return (ok);

	/* hunt backwards as far as poss. */

	for (pt = &last_alloc, last = 0, i = -1; pt->last;
	     i++, last = pt, pt = pt->last)
	{
	    if (i > alloc_count)
	    {
		ok = FALSE;
		break;
	    }
	    if (!(ok = check_alloc_pt(pt)))
		break;
	}

	if (last)
	    for (pt = last, last = pt->last, i += 2; i > 0 && pt;
		i--, last = pt, pt = pt->next)

	    {
		if (wantpr)
		    (void) fprintf (stderr, "%10d  %10d  %10d  %10d\n",
				    pt, pt->next, pt->last, pt->size);
		if (!(ok = check_alloc_pt(pt)))
		    break;
	    }
	return (FALSE);
    }
    else
    {
	/* hunt backwards for last 10 entries */

	for (pt = &last_alloc, last = 0, i = -1;
	    i < 10 && pt->last; i++, last = pt, pt = pt->last)
	{
	    if (i > alloc_count)
	    {
		ok = FALSE;
		break;
	    }
	    if (!(ok = check_alloc_pt(pt)))
		break;
	}
	return (ok);
    }
}



static check_alloc_pt(pt)
struct alloc *pt ;

{   
    if(pt != &last_alloc && pt != &first_alloc &&
    (pt > alloc_high || pt < alloc_low))
    {
	(void) fprintf (stderr,
			"Erroneous pointer=%d, possible range=%d to %d\n",
			pt, alloc_low,alloc_high) ;
        return(FALSE) ; 
    }
    
    if(pt->next != &last_alloc && pt->next != 0 &&
    (pt->next > alloc_high || pt->next < alloc_low))
    {
	(void) fprintf (stderr,
			"Error at %d, pointer=%d, possible range=%d to %d\n",
			pt, pt->next,alloc_low, alloc_high) ;
        return(FALSE) ; 
    }
    
    if(pt->last != &first_alloc && pt->last != 0 &&
    (pt->last > alloc_high || pt->last < alloc_low))
    {
	(void) fprintf (stderr,
			"Error at %d, pointer=%d, possible range=%d to %d\n",
			pt, pt->last,alloc_low, alloc_high) ;
        return(FALSE) ; 
    }
    
    if(pt->size<PAD_SIZE || pt->size>alloc_biggest)
    {
	(void) fprintf (stderr,
			"Error at %d, size=%d, possible range=%d to %d\n",
			pt, pt->size, PAD_SIZE, alloc_biggest) ;
        return(FALSE) ;
    }
    
    
    if((pt->last && pt->last->next != pt) ||
       (pt->next && pt->next->last != pt))
    {
	(void) fprintf (stderr,
			"Error at %d, links to memory allocation chain broken\n",
			pt) ;
        return(FALSE) ;
    }
    
    return(TRUE) ;
}




/*PUBLIC_FUNCTION*/
void m_free (p)
/*  Free memory pointed to by  p  .
    If the environment variable "M_ALLOC_DEBUG" is set to "TRUE" then the
    routine will print deallocation debugging information.
    The routine returns nothing.
*/
char *p ;
{
    char *env;
    unsigned size,status ;
    struct alloc *pt ;
    int sz,scrap ; char *chpt ;
    static int count=0 ;     /* used to do a complete check every 10 calls */
    extern unsigned int m_alloc_flag;
    extern unsigned int check_count;
    extern unsigned int total_allocated;
    static char function_name[] = "m_free";
    
    /*  Check fast flag  */
    if ( fast_alloc_required () )
    {
	free (p);
	return;
    }
    p -= PAD_SIZE ; pt=(struct alloc *)p ;
    
    
    /* Check that list is intact */
    
    count = (count+1) % check_count ;
    if(!check_alloc(FALSE,count==0))
    {
	/*  Reset check count  */
	check_count = INITIAL_CHECK_COUNT;
	(void) fprintf (stderr,
			"Memory allocation block has been overwritten\n") ;
        check_alloc(TRUE,TRUE) ;
        /* lib$signal(SS$_ACCVIO) ; */
	exit (RV_MEM_ERROR);
    }
    else
    {
	/*  Double check count  */
	if (check_count * 2 <= MAX_CHECK_COUNT)
	{
	    check_count *= 2;
	}
    }
    
    
    /* Check pointers*/
    if(pt>=alloc_low && pt<=alloc_high && 
    ((pt->last >= alloc_low && pt->last <= alloc_high) ||
    pt->last==(&first_alloc)) && 
    ((pt->next >= alloc_low && pt->next <= alloc_high) ||
    pt->next==(&last_alloc)) && 
    pt->last->next==pt && pt->next->last==pt)
    {
	pt->last->next=pt->next ; pt->next->last=pt->last ;
        size= *((unsigned *)(p+8));
	total_allocated -= (size - PAD_SIZE);
	if ( debug_required () )
	{
	    (void) fprintf (stderr, "Freed: %d    total: %u    ptr: %p\n",
			    size - PAD_SIZE, total_allocated, p + PAD_SIZE);
	}
	alloc_count--;
        sz=((size+7)/8*8) ; scrap=sz-size ;
        if(scrap)
        {
	    chpt=(char *)pt ; chpt += pt->size ;
            if(strncmp(chpt,"Donald",scrap))
            {
		(void) fprintf(stderr,
			       "Error at %d; data has overflowed allocation\n",
			       pt) ;
                /* errno=SS$_ACCVIO ; return; */
            }
        }
        
	/*        if((status=lib$free_vm(&sz,&p))==SS$_NORMAL)return;
	 *        errno=status ; return;
	 */
	/*  Richard Gooch 30-MAY-1991: changed from  free (&p)  to free (p)  */
	free (p);
	return;
    }
    (void) fprintf (stderr, "Attempt to free unallocated memory, pointer=%d\n",
		    p+PAD_SIZE) ;
/*    errno=LIB$_BADBLOADR ; */
    return;
}


/*PUBLIC_FUNCTION*/
void m_error_notify (function_name, purpose)
char *function_name;
char *purpose;
{
    (void) fprintf (stderr,
		    "Error allocating memory for: %s in function: %s\n",
		    purpose, function_name);
}   /*  End Function m_error_notify   */

/*PUBLIC_FUNCTION*/
void m_abort (name, reason)
char *name;
char *reason;
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

static void prog_bug (function_name)
char *function_name;
{
    (void) fprintf (stderr, "Program bug noted in function: %s\n",
		    function_name);
    (void) fprintf (stderr, "Aborting.%c\n", BEL);
    exit (RV_PROGRAM_BUG);
}   /*  End Function prog_bug   */

static int string_icmp (string1, string2)
/*  This routine will compare two strings pointed to by  string1  and
    string2  .The operation of this routine is similar to that of  strcmp
    ,except that the comparison is case insensitive.
    The comparison value is returned.
*/
char *string1;
char *string2;
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
    if ( ( str1 = malloc (strlen (string1) + 1) ) == NULL )
    {
	m_abort (function_name, "copy of string1");
    }
    strcpy (str1, string1);
    string_upr (str1);

    if ( ( str2 = malloc (strlen (string2) + 1) ) == NULL )
    {
	m_abort (function_name, "copy of string2");
    }
    strcpy (str2, string2);
    string_upr (str2);
    ret_value = strcmp (str1, str2);
    free (str1);
    free (str2);
    return (ret_value);
}   /*  End Function string_icmp   */

static char *string_upr (string)
/*  This routine will convert the string pointed to by  string  to uppercase.
    The routine returns the address of the string.
*/
char *string;
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
/*  This routine will determine if debugging information is required.
    The routine returns TRUE if debugging information is required,
    else it returns FALSE.
*/
{
    char *env;
    static flag checked = FALSE;
    static flag debug = FALSE;

    if (checked) return (debug);
    /*  Determine if debug needed  */
    checked = TRUE;
    if ( ( ( env = getenv ("M_ALLOC_DEBUG") ) != NULL ) &&
	(string_icmp (env, "TRUE") == 0) )
    {
	debug = TRUE;
	(void) fprintf (stderr,
			"Running m_alloc and m_free with debugging\n");
    }
    return (debug);
}   /*  End Function debug_required  */

static flag fast_alloc_required ()
/*  This routine will determine if fast allocation is required.
    The routine returns TRUE if fast allocation is required,
    else it returns FALSE.
*/
{
    char *env;
    static flag checked = FALSE;
    static flag fast_alloc = FALSE;

    if (checked) return (fast_alloc);
    /*  Determine if fast alloc needed  */
    checked = TRUE;
    if ( ( ( env = getenv ("M_ALLOC_FAST") ) != NULL ) &&
	(string_icmp (env, "TRUE") == 0) )
    {
	fast_alloc = TRUE;
	(void) fprintf (stderr,
			"Running m_alloc and m_free without checking\n");
    }
    return (fast_alloc);
}   /*  End Function fast_alloc_required  */

#ifndef HAS_ENVIRON
static char *getenv (name)
/*  This routine will get the value of the environment variable with name
    pointed to by  name  .
    The routine returns a pointer to the value string if present,
    else it returns NULL.
*/
char *name;
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

/*LINTLIBRARY*/
/*  misc.c

    This code provides simple string manipulation routines.

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

/*  This file contains various string operation routines.


    Written by      Richard Gooch   12-SEP-1992

    Updated by      Richard Gooch   15-OCT-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   6-JAN-1993: Improved portability of
  toupper  and  tolower  patch routine declarations.

    Updated by      Richard Gooch   24-JAN-1993: Declared  swap  at front of
  file.

    Updated by      Richard Gooch   2-JUN-1993: Added improved error checking.

    Updated by      Richard Gooch   20-MAY-1994: Added  CONST  declaration
  where appropriate.

    Updated by      Richard Gooch   30-JUN-1994: Fixed bug in  st_nupr  where
  test for null character was performed prior to count test. This caused SEGV
  if  string[str_len]  was an invalid address.

    Updated by      Richard Gooch   6-AUG-1994: Moved to full ASCII function
  declaration syntax. Had to do this because of the confused Dec Alpha compiler

    Updated by      Richard Gooch   3-NOV-1994: Fixed declaration of  st_chr  .

    Updated by      Richard Gooch   6-NOV-1994: Optimised  st_dup  by calling
  m_dup  .

    Last updated by Richard Gooch   26-NOV-1994: Moved to  packages/st/misc.c


*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <karma.h>
#include <os.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_a.h>

/*  C library string functions  */
/*
extern char *strcat();
extern char *strncat();
extern int strcmp();
extern int strncmp();
extern char *strcpy();
extern char *strncpy();
extern int strlen();
extern char *index();
extern char *rindex();
*/

#if !defined(HAS_INTERNATIONALISATION) && !defined(toupper)
static char toupper (char c)
{
    return (c - 'a' + 'A');
}

static char tolower (char c)
{
    return (c - 'A' + 'a');
}
#endif


/*  Private functions  */
static void swap (/* v, i, j */);


/*PUBLIC_FUNCTION*/
unsigned int st_find ( CONST char **string_list, unsigned int list_length,
		      CONST char *string, int (*function) () )
/*  This routine will search the list of strings pointed to by  string_list
    for an occurrence of the string  string  .The length of the string list
    must be in  list_length  .
    The comparison routine uses the routine pointed to by  function  .
    The routine returns the index of the found string in the string list.
    If no match is found, the routine returns  list_length  .
    The routine returns on the first match.
*/
{
    unsigned int count = 0;
    static char function_name[] = "st_find";

    if ( (string_list == NULL) || (string == NULL) )
    {
	return (list_length);
    }
    while (count < list_length)
    {
	if (string_list[count] == NULL)
        {
	    fprintf (stderr, "Null string pointer in string list\n");
            a_prog_bug (function_name);
        }
        if ( (*function) (string_list[count], string) == 0)
        {
	    return (count);
        }
        ++count;
    }
    return (count);
}   /*  End Function st_find   */

/*PUBLIC_FUNCTION*/
CONST char *st_chr (CONST char *string, char c)
/*  @(#)strchr.c    1.2 */
/*
 * Return the ptr in string at which the character c appears;
 * NULL if not found
 */
{
    static char function_name[] = "st_chr";

    if (string == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    do
    {
        if (*string == c)
	return ( (char *) string );
    }
    while (*string++);
    return(NULL);
}

/*PUBLIC_FUNCTION*/
int st_cmp_wild (CONST char *a, CONST char *b)
/* compare strings a & b allowing for wild cards in a but not b */
{
    CONST char *a_star;
    int l,lb,diff ;
    
    if(!a || !*a)return(-1) ;
    if(!b || !*b)return( 1) ;
    
    for( ; *a && *b ; a++ , b++)
    {   if(*a=='%') ;
        else if(*a=='*')
        {   a++ ; if(*a=='\0')return(0) ; if(*a=='*' || *a=='%')continue ;
            if((a_star=st_chr(a,'*') ))l=a_star-a ;
            else l=strlen(a) ;
            lb=strlen(b) ;
            for( diff=TRUE ; lb>=l && (diff=strncmp(a,b,l)) ; b++ , lb--) ;
            if(diff)return(diff) ;
        }
	else
        {   if((diff= *a-*b))return(diff) ;
        }
    }
    if(!*b && *a=='*' && *(a+1)=='\0')return(0) ;
    return(*a-*b) ;
}

/*PUBLIC_FUNCTION*/
int st_cspn (CONST char *string, CONST char *charset)
/*  @(#)strcspn.c   1.1 */
/*
 * Return the number of characters in the maximum leading segment
 * of string which consists solely of characters NOT from charset.
 */
{
    register CONST char *p, *q;
    static char function_name[] = "st_cspn";

    if ( (string == NULL) || (charset == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    for(q=string; *q != '\0'; ++q)
    {
        for(p=charset; *p != '\0' && *p != *q; ++p)
            ;
        if(*p != '\0')
            break;
    }
    return(q-string);
}

/*PUBLIC_FUNCTION*/
int st_icmp (CONST char *string1, CONST char *string2)
/*  This routine will compare two strings pointed to by  string1  and
    string2  .The operation of this routine is similar to that of  strcmp
    ,except that the comparison is case insensitive.
    The comparison value is returned.
*/
{
    int ret_value;
    char *str1 = NULL;
    char *str2 = NULL;
    static char function_name[] = "st_icmp";

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

    if ( ( str1 = m_alloc (strlen (string1) + 1) ) == NULL )
    {
	m_abort (function_name, "copy of string1");
    }
    strcpy (str1, string1);
    st_upr (str1);

    if ( ( str2 = m_alloc (strlen (string2) + 1) ) == NULL )
    {
	m_abort (function_name, "copy of string2");
    }
    strcpy (str2, string2);
    st_upr (str2);
    ret_value = strcmp (str1, str2);
    m_free (str1);
    m_free (str2);
    return (ret_value);
}   /*  End Function st_icmp   */

/*PUBLIC_FUNCTION*/
char *st_lwr (char *string)
/*  This routine will convert the string pointed to by  string  to lowercase.
    The routine returns the address of the string.
*/
{
    static char function_name[] = "st_lwr";

    if (string == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while (*string != '\0')
    {
	if (isupper (*string) != 0)
	{
	    *string = tolower (*string);
	}
        ++string;
    }
    return (string);
}   /*  End Function st_lwr */

/*PUBLIC_FUNCTION*/
int st_nicmp (CONST char *string1, CONST char *string2, int str_len)
/*  This routine will compare two strings pointed to by  string1  and
    string2  .The string comparison is performed up to  str_len  characters
    long. The operation of this routine is similar to that of  strncmp  ,
    except that the comparison is case insensitive.
    The comparison value is returned.
*/
{
    int ret_value;
    char *str1 = NULL;
    char *str2 = NULL;
    static char function_name[] = "st_nicmp";

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

    if ( ( str1 = m_alloc (str_len) ) == NULL )
    {
	m_abort (function_name, "partial copy of string1");
    }
    strncpy (str1, string1, str_len);
    st_nupr (str1, str_len);

    if ( ( str2 = m_alloc (str_len) ) == NULL )
    {
	m_abort (function_name, "partial copy of string2");
    }
    strncpy (str2, string2, str_len);
    st_nupr (str2, str_len);
    ret_value = strncmp (str1, str2, str_len);
    m_free (str1);
    m_free (str2);
    return (ret_value);
}   /*  End Function st_nicmp   */

/*PUBLIC_FUNCTION*/
char *st_nupr (char *string, int str_len)
/*  This routine will convert the string pointed to by  string  to uppercase.
    The conversion stops at the first NULL terminator character or if  str_len
    characters have been converted.
    The routine returns the address of the string.
*/
{
    int count = 0;
    static char function_name[] = "st_nupr";

    if (string == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while ( (count < str_len) && (*string != '\0') )
    {
	if (islower (*string) != 0)
	{
	    *string = toupper (*string);
	}
        ++string;
        ++count;
    }
    return (string);
}   /*  End Function st_nupr */

/*PUBLIC_FUNCTION*/
char *st_nlwr (char *string, int str_len)
/*  This routine will convert the string pointed to by  string  to lowercase.
    The conversion stops at the first NULL terminator character or if  str_len
    characters have been converted.
    The routine returns the address of the string.
*/
{
    int count = 0;
    static char function_name[] = "st_nlwr";

    if (string == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while ( (count < str_len) && (*string != '\0') )
    {
	if (isupper (*string) != 0)
	{
	    *string = tolower (*string);
	}
        ++string;
        ++count;
    }
    return (string);
}   /*  End Function st_nlwr */

/*PUBLIC_FUNCTION*/
char *st_pbrk (CONST char *string, CONST char *brkset)
/*  @(#)strpbrk.c   1.1 */
/*
 * Return ptr to first occurance of any character from `brkset'
 * in the character string `string'; NULL if none exists.
 */
{
    CONST register char *p;
    static char function_name[] = "st_pbrk";

    if ( (string == NULL) || (brkset == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    do
    {
        for (p = brkset; *p != '\0' && *p != *string; ++p)
	;
        if (*p != '\0')
            return ( (char *) string );
    }
    while (*string++);
    return (NULL);
}

/*PUBLIC_FUNCTION*/
CONST char *st_rchr (CONST char *string, char c)
/*  @(#)strrchr.c   1.2 */
/*
 * Return the ptr in string at which the character c last
 * appears; NULL if not found
*/
{
    CONST register char *r;
    static char function_name[] = "st_rchr";

    if (string == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    r = NULL;
    do
    {
        if(*string == c)
            r = string;
    }
    while (*string++);
    return (r);
}

/*PUBLIC_FUNCTION*/
int st_spn (CONST char *string, CONST char *charset)
/*  @(#)strspn.c    1.1 */
/*
 * Return the number of characters in the maximum leading segment
 * of string which consists solely of characters from charset.
 */
{
    CONST register char *p, *q;
    static char function_name[] = "st_rchr";

    if ( (string == NULL) || (charset == NULL) )
    {
	(void) fprintf (stderr, "NULL pointer(s) passed\n");
	a_prog_bug (function_name);
    }
    for (q = string; *q != '\0'; ++q)
    {
        for (p = charset; *p != '\0' && *p != *q; ++p)
            ;
        if (*p == '\0')
            break;
    }
    return (q - string);
}

/*PUBLIC_FUNCTION*/
char *st_tok (char *string, CONST char *sepset)
/*  @(#)strtok.c    1.2 */
/*
 * uses strpbrk and strspn to break string into tokens on
 * sequentially subsequent calls.  returns NULL when no
 * non-separator characters remain.
 * `subsequent' calls are calls with first argument NULL.
 */
{
    register char   *p, *q, *r;
    static char *savept;

    /*first or subsequent call*/
    p = (string == NULL) ? savept: string;

    if (p == 0)      /* return if no tokens remaining */
        return (NULL);

    q = p + st_spn (p, sepset);  /* skip leading separators */

    if (*q == '\0')      /* return if no tokens remaining */
        return (NULL);

    if ( (r = (char *) st_pbrk (q, sepset) ) == NULL )    /* move past token */
        savept = 0; /* indicate this is last token */
    else
    {
        *r = '\0';
        savept = ++r;
    }
    return (q);
}

#define DIGIT(x) (isdigit(x)? ((x)-'0'): (10+tolower(x)-'a'))
#define MBASE 36

/*PUBLIC_FUNCTION*/
long st_tol (CONST char *str, char **ptr, int base)
{
    long val;
    int xx, sign;

    val = 0L;
    sign = 1;
    if (base < 0 || base > MBASE)
    goto OUT;
    while ( isspace (*str) )
    ++str;
    if (*str == '-')
    {
        ++str;
        sign = -1;
    }
    else if (*str == '+')
    ++str;
    if (base == 0)
    {
        if (*str == '0')
	{
            ++str;
            if (*str == 'x' || *str == 'X')
	    {
                ++str;
                base = 16;
            }
	    else
	    base = 8;
        }
	else
	base = 10;
    }
    else if (base == 16)
    if ( str[0] == '0' && (str[1] == 'x' || str[1] == 'X') )
    str += 2;
    /*
     * for any base > 10, the digits incrementally following
     *  9 are assumed to be "abc...z" or "ABC...Z"
     */
    while (isalnum (*str) && ( xx = DIGIT (*str) ) < base)
    {
        /* accumulate neg avoids surprises near maxint */
        val = base * val - xx;
        ++str;
    }
  OUT:
    if (ptr != (char**) 0)
    *ptr = (char *) str;
    return ( sign * (-val) );
}

/*PUBLIC_FUNCTION*/
char *st_upr (char *string)
/*  This routine will convert the string pointed to by  string  to uppercase.
    The routine returns the address of the string.
*/
{
    static char function_name[] = "st_upr";

    if (string == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    while (*string != '\0')
    {
	if (islower (*string) != 0)
	{
	    *string = toupper (*string);
	}
        ++string;
    }
    return (string);
}   /*  End Function st_upr */

/*PUBLIC_FUNCTION*/
char *st_dup (CONST char *input)
/*  This routine will make a duplicate copy of the string pointed to by  input
    The routine returns a pointer to a copy of the string (allocated using
    m_alloc) on success, else it returns NULL.
*/
{
    unsigned int new_length;
    char *copy;
    static char function_name[] = "st_dup";

    if (input == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    new_length = strlen (input) + 1;
    if ( ( copy = m_dup (input, new_length) ) == NULL )
    {
	m_error_notify (function_name, "duplicate string");
	return (NULL);
    }
    return (copy);
}   /*  End Function st_dup  */

/*PUBLIC_FUNCTION*/
void st_qsort (char **v, int left, int right)
/*  This routine will perform a quicksort on an array of strings.
    The array of strings must be pointed to by  v  .
    The left and right string indices must be given by  left  and  right  ,
    respectively.
    The routine returns nothing.
*/
{
    int i,last;
    extern void swap(/* char *v[],int i,int j */);

    if(left>=right) return;
    swap(v,left,(left+right)/2);
    last=left;
    for(i=left+1;i<=right;i++)
    if(strcmp(v[i],v[left])<0) swap(v,++last,i);
    swap(v,left,last);
    st_qsort(v,left,last-1);
    st_qsort(v,last+1,right);
}   /*  End Function st_qsort   */

static void swap (char **v, int i, int j)
{
    char *temp;
    temp=v[i];
    v[i]=v[j];
    v[j]=temp;
}   /*  End Function swap  */

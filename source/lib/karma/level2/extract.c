/*LINTLIBRARY*/
/*PREFIX:"ex_"*/
/*  extract.c

    This code provides routines to extract values and substrings from strings.

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

/*  Functions to decode (EXtract) floats and integers from strings.


    Written by      Richard Gooch    30-SEP-1992

    Updated by      Richard Gooch    2-DEC-1992

    Last updated by Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>


*/
#include <stdio.h>
#include <ctype.h>
#include <string.h>
#include <karma.h>
#include <karma_ex.h>
#include <karma_m.h>


static int string_cspn (string, charset)
/*  @(#)strcspn.c   1.1 */
/*
 * Return the number of characters in the maximum leading segment
 * of string which consists solely of characters NOT from charset.
 */
char *string;
char *charset;
{
    register char *p, *q;

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
int ex_int (str, rest)
char *str;
char **rest;
{   
    int i=0,base=10,maxdig=017777777777,neg=FALSE,dig  ;
    
    if(!str || !(*str))
    {
	*rest=0 ; return(0) ; 
    }
    
    for( ; !isdigit(*str) ; str++)
    {
	if(! *str)
	{
	    *rest=0 ; return(0) ; 
	}
	if(*str=='+')
	{
	    str++ ; break ; 
	}
	if(*str=='-')
	{
	    neg=TRUE ; str++ ; break ; 
	}
    }
    
    if(*str=='0')		/* leading "0" indicates octal */
    {
	base=8 ; str++ ;
	if(*str=='x' || *str=='X') /* leading "0x" indicates hex.*/
        {
	    base=16 ; str++ ;
            
	    for( ; isxdigit(*str) ; str++)
            {
		dig=isdigit(*str) ? (*str-'0') :
                isupper(*str) ? (*str-'A'+10) :
		(*str-'a'+10) ;
		i=i*base+dig ;
		if(i>maxdig)
		{
		    *rest=0 ; if(neg)i= -i ; return(i) ; 
		}
            }
        }
    }    
    
    for( ; isdigit(*str) ; str++)
    {
	i=i*base+(*str-'0') ; 
	if(i>maxdig)
	{
	    *rest=0 ; if(neg)i= -i ; return(i) ; 
	}
    }
    
    for( ; isspace(*str)  ; str++) ; /* skip trailing spaces */
    *rest= *str ? str : 0 ;     
    if(neg)i= -i ; return(i) ;
}




/*PUBLIC_FUNCTION*/
char *ex_word (str, rest)
/* Function decodes (eXtracts) a word from str, and returns a pointer
* to a copy of that word, with null terminator. It also 
* returns a pointer to the next character of the string in rest.
*
*  Author Don McLean 1984 Sept. 6.
*/
char *str;
char **rest;
{   
    int n=0 ; char *start,*copy ;
    
    if(!str || !(*str))return(0) ;
    for( ; isspace(*str)  ; str++) ; /* skip leading spaces */
    start=str ;
    /* find trailing blank or null */
    for( ; !isspace(*str) && *str ; str++ , n++) ; 
    for( ; isspace(*str) ; str++) ; /* skip trailing spaces */
    *rest= *str ? str : 0 ;
    copy=m_alloc((unsigned)n+1) ; strncpy(copy,start,n) ; copy[n]='\0' ;
    return(copy) ;
}

static char command_terminators[]=" \n\t/=?!()" ;

/*PUBLIC_FUNCTION*/
char *ex_command (str, rest)
/* Function decodes (eXtracts) a command from str, and returns a pointer
 * to a copy of that word, with null terminator. It also 
 * returns a pointer to the next character of the string in rest.
 * Identical to ex_word() except that any of " \t/=?!" terminate a command.
 *
 *  Author Don McLean 1986 February 13.
*/
char *str;
char **rest;
{   
    int n=0 ; char *start,*copy,*strpbrk() ;
    
    if(!str || !(*str))return(0) ;
    for( ; isspace(*str)  ; str++) ; /* skip leading spaces */
    start=str++ ; if(!*start)return(0) ;
    /* find trailing blank, null, '/' or '=' */
    if((str=strpbrk(str,command_terminators)))
    {
	n=str-start ; 
	for( ; isspace(*str) || *str=='=' ; str++) ; /* skip trailing spaces */
	*rest= *str ?  str : 0 ;
    }
    else
    {
	n=strlen(start) ; 
	*rest=0 ;
    }
    copy=m_alloc((unsigned)n+1) ; strncpy(copy,start,n) ; copy[n]='\0' ;
    return(copy) ;
}




/*PUBLIC_FUNCTION*/
char *ex_word_skip (str)
/* Skips a word in str, and returns a pointer
 * to the next character of the string.
 *
 *  Author Don McLean 1984 Sept. 6.
*/
char *str;
{   
    if(!str || !(*str))return(0) ;    
    for( ; isspace(*str)  ; str++) ; /* skip leading spaces */
    /* find trailing blank or null */
    for( ; !isspace(*str) && *str ; str++ ) ; 
    for( ; isspace(*str)  ; str++) ; /* skip trailing spaces */
    return(*str ? str : 0) ;
}




/*PUBLIC_FUNCTION*/
double ex_float (str, rest)
/* Function decodes (eXtracts) next floating point value from string and
 * returns value as function value, and pointer to next character of string
 * in rest.
 *
 * Author Don McLean 1985 January 8.
 *
 */
char *str;
char **rest;
{   
    int infract=FALSE,pos=TRUE,exponent=0 ;
    double base=10.0,f=1.0,val=0 ;
    
    
    /* check for null string */
    if(!str || !(*str)){   *rest=0 ; return(val) ; }
    
    /* skip leading rubbish */
    
    for( ; !isdigit(*str) ; str++)
    {
	if(! *str)
	{
	    *rest=0 ; return(0) ; 
	}
	if(*str=='+')
	{ 
	    str++ ; break ; 
	}
	if(*str=='-')
	{
	    str++ ; pos=FALSE ; break ; 
	}
	if(*str=='.') break ;
    }
    
    /* loop to pick up integer, fraction and exponent parts */
    
    for( ; *str ; str++)
    {
	if(*str=='.')
        {
	    if(infract)break ; infract=TRUE ;
        }
	else if(isdigit(*str))
        {
	    if(infract)
	    {
		f=f/base ; val=val+(*str-'0')*f ;
            }
	    else 
	    val=val*base+(*str-'0') ;
        }
	else if(*str=='e' || *str=='E' || *str=='d' || *str=='D')
        {
	    for( exponent=ex_int(str,&str) ; exponent>0 ; exponent--)
            {
		if(val>1.0e32)break ; val *= base ;
            }
	    for( ; exponent<0 ; exponent++)val /= base ; 
	    break ;
        }
	else 
	break ;
    }
    if(str)for( ; isspace(*str)  ; str++) ; /* skip trailing spaces */
    *rest= str && *str ? str : 0 ; if(!pos)val = -val ; return(val) ;
}



/*PUBLIC_FUNCTION*/
double ex_hour (p, nxt)
char *p;
char **nxt;
{   
    double h ;
    
    h=ex_float(p,&p) ;
    
    if(!p)
    {   
	*nxt=p ; return(h) ; 
    }
    while(*p==':' || isspace(*p))p++ ;
    if(!isdigit(*p))
    {
	*nxt=p ; return(h) ; 
    }
    h += ex_float(p,&p)/60.0 ;
    
    if(!p)
    {
	*nxt=p ; return(h) ; 
    }
    while(*p==':' || isspace(*p))p++ ;
    if(!isdigit(*p))
    {
	*nxt=p ; return(h) ; 
    }
    return(h + ex_float(p,nxt)/3600.0) ;
}




/*PUBLIC_FUNCTION*/
char *ex_command_skip (str)
/* Skips a command from str, and returns a pointer
 * to the next character of the string.
 * Identical to ex_word_skip() except that any of " \t/=?!" terminate a command.
 *
 *  Author Don McLean 1986 February 14.
*/
char *str;
{   
    char *strpbrk() ;
    
    if(!str || !(*str))return(0) ;
    for( ; isspace(*str)  ; str++) ; /* skip leading spaces */
    if(!*str++)return(0) ;
    /* find trailing blank, null, '/' or '=' */
    str=strpbrk(str,command_terminators) ; if(!str)return(0) ;
    while(isspace(*str) || *str=='=')str++ ;
    if(*str)return(str) ; else return(0) ;
}



/*PUBLIC_FUNCTION*/
int ex_on (ptr)
/* ptr points to a text string (normally control input)
 * On entry ptr is advanced over one (key-)word, then
 * if next word matches "on" or "off", ptr will be advanced
 * and function returns TRUE or FALSE. 
 * If neither "on" nor "off" is found, pointer
 * is not advanced, but function returns TRUE.
 * A word matches a target if all its letters match the letters
 * of the target: "y", "ye", "yes" all match "yes", but "yesterday"
 * does not.
 */
char **ptr;
{   
    char *ex_command_skip() ;
    int l;
      
    if(!(*ptr=ex_command_skip(*ptr)))return(TRUE) ;
    
    l=string_cspn(*ptr," \t/=!?\n") ;
    
    if(!strncmp(*ptr,"on",l))
    {
	*ptr = ex_command_skip(*ptr) ;
	return(TRUE) ;
    }
    else if(!strncmp(*ptr,"off",l))
    {
	*ptr = ex_command_skip(*ptr) ;
	return(FALSE) ;
    }
    else return(TRUE) ;
}


/*PUBLIC_FUNCTION*/
int ex_on_or_off (ptr)
/* ptr points to a text string (normally control input)
 * If next word matches "on" or "off", ptr will be advanced
 * and function returns TRUE or FALSE. 
 * If neither "on" nor "off" is found, pointer
 * is not advanced, but function returns TRUE.
 *
 * The only difference between 'ex_on' and 'ex_on_or_off' is that
 * 'ex_on_or_off' does not skip an initial key-word on entry.
 *
 * A word matches a target if all its letters match the letters
 * of the target: "y", "ye", "yes" all match "yes", but "yesterday"
 * does not.
 */
char **ptr;
{   
    char *ex_command_skip() ;
    int l;
    
    l=string_cspn(*ptr," \t/=!?") ;
    
    if(!strncmp(*ptr,"on",l))
    {
	*ptr = ex_command_skip(*ptr) ;
	return(TRUE) ;
    }
    else if(!strncmp(*ptr,"off",l))
    {
	*ptr = ex_command_skip(*ptr) ;
	return(FALSE) ;
    }
    else 
    return(TRUE) ;
}



/*PUBLIC_FUNCTION*/
int ex_yes (ptr, default_v)
/* ptr points to a text string (normally control input)
 * if next word matches "yes" or "no", ptr will be advanced
 * and function returns TRUE or FALSE. 
 * If neither "yes" nor "no" is found, pointer
 * is not advanced, but function returns 'default_v'.
 * A word matches a target if all its letters match the letters
 * of the target: "y", "ye", "yes" all match "yes", but "yesterday"
 * does not.
 */
char **ptr;
int default_v;
{   
    char *w,*ex_command_skip() ;
    int l ;
    
    if(!ptr || !*ptr || !**ptr)return(default_v) ;
    
    for(w = *ptr ; *w && isspace(*w) ; w++) ;
    for(l = 0 ; *w && !isspace(*w) ; l++ , w++) ;
    if(!l)return(default_v) ;
    
    if(!strncmp(*ptr,"yes",l))
    {
	*ptr = ex_command_skip(*ptr) ;
	return(TRUE) ;
    }
    else if(!strncmp(*ptr,"no",l))
    {
	*ptr = ex_command_skip(*ptr) ;
	return(FALSE) ;
    }
    else
    return(default_v) ;
}

/*PUBLIC_FUNCTION*/
char *ex_str (str, rest)
/*  This routine will extract a sub-string from a string. The sub-string may be
    delimited by any number of whitespace characters.
    The input string must be pointed to by  str. The routine will write a
    pointer to the next field in the input string to  rest  . If this is NULL,
    nothing is written here.
    The double quote character may appear anywhere in the sub-string, and will
    force all whitespace characters except '\n' into the output string. A
    second double quote character unquotes the previous quote. These double
    quote characters are not copied, unless they are consecutive.
    The routine returns a pointer to a copy of the sub-string.
*/
char *str;
char **rest;
{
    flag finished = FALSE;
    char quote = '\0';
    char *return_value;
    char *out_ptr;
    static char function_name[] = "ex_str";

    if ( (str == NULL) || (*str == '\0') )
    {
	if (rest != NULL)
	{
	    *rest = NULL;
	}
	return (NULL);
    }
    /*  Skip leading whitespace  */
    while ( isspace (*str) && (*str != '\0' ) )
    {
	++str;
    }
    if ( ( return_value = m_alloc (strlen (str) + 1) ) == NULL )
    {
	m_error_notify (function_name, "sub-string copy");
	return (NULL);
    }
    out_ptr = return_value;
    while (finished == FALSE)
    {
	switch (*str)
	{
	  case '\0':
	    *out_ptr = *str;
	    finished = TRUE;
	    break;
	  case ' ':
	  case '\t':
	    if (quote != '\0')
	    {
		*out_ptr++ = *str++;
	    }
	    else
	    {
		*out_ptr = '\0';
		finished = TRUE;
	    }
	    break;
	  case '"':
	  case '\'':
	    if ( (quote != '\0') && (*str != quote) )
	    {
		/*  Quote is already open and character is not the quote
		    character  */
		*out_ptr++ = *str++;
		continue;
	    }
	    if (str[1] == str[0])
	    {
		/*  Two quote characters: copy only one of them  */
		*out_ptr++ = *str++;
		++str;
		continue;
	    }
	    /*  Open quote if not open; close quote if already open  */
	    quote = (quote == '\0') ? *str : '\0';
	    ++str;
	    break;
	  default:
	    *out_ptr++ = *str++;
	    break;
	}
    }
    if (quote != '\0')
    {
	(void) fprintf (stderr, "\nWarning: closing quote not found");
    }
    if (rest != NULL)
    {
	/*  Skip trailing whitespace  */
	while ( isspace (*str) && (*str != '\0' ) )
	{
	    ++str;
	}
	*rest = (*str == '\0') ? '\0' : str;
    }
    return (return_value);
}   /*  End Function ex_str  */

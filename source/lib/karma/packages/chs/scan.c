/*LINTLIBRARY*/
/*  scan.c

    This code provides Channel scanning routines.

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

/*  This file contains all routines needed for the scanning of channel objects
  for integers and floats.


    Written by      Richard Gooch   3-DEC-1992

    Updated by      Richard Gooch   3-DEC-1992

    Updated by      Richard Gooch   30-DEC-1992: Added  chs_get_line  .

    Updated by      Richard Gooch   1-JAN-1993: Took account of change to
  ch_gets  .

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   20-AUG-1993: Changed from using  ch_gets
  to  ch_getl  in  chs_get_line  .

    Updated by      Richard Gooch   21-MAY-1994: Added  #include <karma_ch.h>

    Last updated by Richard Gooch   26-NOV-1994: Moved to  packages/chs/scan.c


*/
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <karma.h>
#include <karma_chs.h>
#include <karma_ch.h>
#include <karma_ex.h>
#include <karma_m.h>
#include <karma_a.h>

/*PUBLIC_FUNCTION*/
unsigned int chs_get_value (channel, string, length)
/*  This routine will scan a channel object for a whitespace separated value.
    The channel to read from must be given by  channel  .
    The routine will write the value into the buffer pointed to by  string  .
    The size of the buffer (in bytes) must be given by  length  .
    The routine returns the length of the string scanned on success,
    else it returns 0.
*/
Channel channel;
char *string;
unsigned int length;
{
    unsigned int char_pos;
    static char function_name[] = "chs_get_value";

    char_pos = 0;
    while (ch_read (channel, string + char_pos, 1) == 1)
    {
	/*  Have another character  */
	if ( isspace (string[char_pos]) )
	{
	    /*  Have a whitespace character  */
	    if (char_pos > 0)
	    {
		/*  Have read in some non-whitespace already  */
		return (char_pos);
	    }
	}
	else
	{
	    /*  Non-whitespace  */
	    if (++char_pos >= length)
	    {
		a_func_abort (function_name, "value too large for buffer");
		return (0);
	    }
	}
    }
    return (char_pos);
}   /*  End Function chs_get_value  */

/*PUBLIC_FUNCTION*/
double chs_get_float (channel)
/*  This routine will scan a channel object for the ASCII representaion of a
    floating point number. Any leading whitespace will be ignored.
    The channel to read from must be given by  channel  .
    The routine will return the value scanned.
*/
Channel channel;
{
    char *p;
    char string[STRING_LENGTH];
    static char function_name[] = "chs_get_float";

    if (chs_get_value (channel, string, STRING_LENGTH) == 0)
    {
	exit (RV_BAD_DATA);
    }
    return ( ex_float (string, &p) );
}   /*  End Function chs_get_float  */

/*PUBLIC_FUNCTION*/
int chs_get_int (channel)
/*  This routine will scan a channel object for the ASCII representaion of a
    integer number. Any leading whitespace will be ignored.
    The channel to read from must be given by  channel  .
    The routine will return the value scanned.
*/
Channel channel;
{
    char *p;
    char string[STRING_LENGTH];
    static char function_name[] = "chs_get_int";

    if (chs_get_value (channel, string, STRING_LENGTH) == 0)
    {
	exit (RV_BAD_DATA);
    }
    return ( ex_int (string, &p) );
}   /*  End Function chs_get_int  */

/*PUBLIC_FUNCTION*/
flag chs_get_line (channel, buffer, length)
/*  This routine will read a line from a channel, stripping all comments,
    leading and trailing whitespace. The comment character is '#'.
    The channel must be given by  channel  .
    The buffer to write the line into must be pointed to by  buffer  .
    The size of the buffer must be given by  length  .
    The routine returns TRUE on success, else it returns FALSE (indicating
    End-Of-File).
*/
Channel channel;
char *buffer;
unsigned int length;
{
    int len;
    char *ch;

    while ( ch_getl (channel, buffer, STRING_LENGTH) )
    {
	/*  Process line  */
	/*  Remove any comments  */
	if ( ( ch = strchr (buffer, '#') ) != NULL )
	{
	    *ch = '\0';
	}
	/*  Remove trailing spaces  */
	len = strlen (buffer) - 1;
	while ( (len >= 0) && isspace (buffer[len]) )
	{
	    /*  Whitespace  */
	    buffer[len--] = '\0';
	}
	if (buffer[0] == '\0')
	{
	    /*  Comment or blank line: skip  */
	    continue;
	}
	/*  Skip leading whitespace  */
	for (ch = buffer; isspace (*ch); ++ch);
	if (ch > buffer)
	{
	    /*  Get rid of whitespace  */
	    m_copy (buffer, ch, strlen (ch) + 1);
	}
	return (TRUE);
    }
    return (FALSE);
}   /*  End Function chs_get_line  */

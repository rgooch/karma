/*LINTLIBRARY*/
/*  port_number.c

    This code provides Karma port number allocation and calculation routines.

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

/*

    This file contains the various utility routines for manipulating Karma
    connections.


    Written by      Richard Gooch   5-OCT-1992

    Updated by      Richard Gooch   27-OCT-1992

    Updated by      Richard Gooch   19-DEC-1992: Added
  r_get_screen_num_from_display  routine.

    Updated by      Richard Gooch   25-DEC-1992: Changed to use of  r_getenv
  in  r_get_karmabase  .

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   31-JAN-1993: Improved diagnostic in
  search_file_for_named_value  .

    Updated by      Richard Gooch   19-JUL-1993: Patched  r_get_service_number
  to get around  sprintf  bug in VX/ MVX C library.

    Updated by      Richard Gooch   14-SEP-1993: Modified  r_get_service_number
  to use hashing function instead of services file.

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/r/port_number.c

    Updated by      Richard Gooch   13-APR-1996: Changed to new documentation
  format.

    Last updated by Richard Gooch   25-OCT-1996: Hash in UID when computing
  default port number.


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <karma.h>
#include <karma_r.h>

#define DISPLAYS_PER_MACHINE 5
#define SERVICE_TO_PORT_FACTOR 5
#define DISPLAY_TO_PORT_FACTOR (SERVICE_TO_PORT_FACTOR * DISPLAYS_PER_MACHINE)
#define MODULUS_VALUE 4003

/*  Private functions  */


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
char *r_get_karmabase ()
/*  [SUMMARY] Get the pathname of the installed runtime Karma.
    [RETURNS] The pathname.
*/
{
    char *karmabase;
    static char *def_karmabase = "/usr/local/karma";

    if ( ( karmabase = r_getenv ("KARMABASE") ) == NULL )
    {
	fprintf (stderr, "Environment variable KARMABASE not found\n");
	fprintf (stderr, "Defaulting to: %s\n", def_karmabase);
	karmabase = def_karmabase;
    }
    return (karmabase);
}   /*  End Function r_get_karmabase  */

/*PUBLIC_FUNCTION*/
int r_get_service_number (CONST char *module_name)
/*  [SUMMARY] Get service number for a module.
    [PURPOSE] This routine uses a hashing function to determine the service
    number of a module.
    <module_name> The name of the module.
    [RETURNS] The service number.
*/
{
    int num = 1;
    int count;
    int len;

    len = strlen (module_name);
    for (count = 0; count < len; ++count)
    {
	if ( (module_name[count] <= ' ') || (module_name[count] > '~') )
	{
	    fprintf (stderr, "Illegal character: value: %d\n",
		     module_name[count]);
	    return (-1);
	}
	num = num * (module_name[count] - ' ');
	num += count;
	num %= MODULUS_VALUE;
	if (num < 1) num = 1;
    }
    return (num);
}   /*  End Function r_get_service_number  */

/*PUBLIC_FUNCTION*/
char *r_get_host_from_display (CONST char *display)
/*  [SUMMARY] Get the hostname in a display string.
    [PURPOSE] This routine will get the hostname from a display string.
    The syntax for the display string is the same as for the X Windows system
    DISPLAY environmental variable.
    <display> The display string. If this is NULL, the host "unix" is returned.
    [RETURNS] A pointer to a statically allocated string which will contain the
    host name on success, else NULL.
*/
{
    char *char_ptr;
    static char host[STRING_LENGTH];

    if (display != NULL)
    {
	/*  display  exists  */
	if ( ( char_ptr = strchr (display, ':') ) == NULL )
	{
	    /*  Error in format  */
	    fprintf (stderr, "Error in display format: \"%s\"\n", display);
	    return (NULL);
	}
	if (char_ptr != display)
	{
	    /*  Not a ":d.s" string  */
	    strncpy ( host, display,
			    (unsigned int) (char_ptr - display) );
	    host[char_ptr - display] = '\0';
	}
	else
	{
	    strcpy (host, "unix");
	}
    }
    else
    {
	/*  display  does not exist  */
	strcpy (host, "unix");
    }
    return (host);
}   /*  End Function r_get_host_from_display  */

/*PUBLIC_FUNCTION*/
int r_get_display_num_from_display (CONST char *display)
/*  [SUMMARY] Get display number from a display string.
    [PURPOSE] This routine will get the display number from a display string
    (following the X Windows system syntax for the DISPLAY environmental
    variable).
    <display> The display string. If this is NULL, the display number defaults
    to 0.
    [RETURNS] The display number on success, else -1.
*/
{
    int display_num;
    char *char_ptr;

    if (display == NULL)
    {
	/*  display not passed  */
	return (0);
    }
    /*  display  passed  */
    if ( ( char_ptr = strchr (display, ':') ) == NULL )
    {
	fprintf (stderr, "Error in  display  format: \"%s\"\n", display);
	return (-1);
    }
    ++char_ptr;
    if ( ( display_num = atoi (char_ptr) ) < 1 )
    {
	/*  Display number of 0 or smaller: check for '0' character  */
	if (*char_ptr != '0')
	{
	    fprintf (stderr, "Error in  display  format: \"%s\"\n", display);
	    return (-1);
	}
    }
    return (display_num);
}   /*  End Function r_get_display_num_from_display  */

/*PUBLIC_FUNCTION*/
int r_get_screen_num_from_display (CONST char *display)
/*  [SUMMARY] Get the screen number in a display string.
    [PURPOSE] This routine will get the screen number from a display string
    (following the X Windows system syntax for the DISPLAY environmental
    variable).
    <display> The display string. If this is NULL, the screen number defaults
    to 0.
    [RETURNS] The display number on success, else -1.
*/
{
    int screen_num;
    char *char_ptr;

    if (display == NULL)
    {
	/*  display not passed  */
	return (0);
    }
    /*  display  passed  */
    if ( ( char_ptr = strchr (display, ':') ) == NULL )
    {
	fprintf (stderr, "Error in  display  format: \"%s\"\n", display);
	return (-1);
    }
    if ( ( char_ptr = strchr (char_ptr, '.') ) == NULL )
    {
	return (0);
    }
    ++char_ptr;
    if ( ( screen_num = atoi (char_ptr) ) < 1 )
    {
	/*  Screen number of 0 or smaller: check for '0' character  */
	if (*char_ptr != '0')
	{
	    fprintf (stderr, "Error in  display  format: \"%s\"\n", display);
	    return (-1);
	}
    }
    return (screen_num);
}   /*  End Function r_get_screen_num_from_display  */

/*PUBLIC_FUNCTION*/
int r_get_def_port (CONST char *module_name, CONST char *display)
/*  [SUMMARY] Get the default Karma port number for a module.
    <module_name> The module name.
    <display> The display string. The display number entry in the string is
    also used to compute the port number. The syntax for this string is the
    same as for the X Windows system DISPLAY environmental variable.
    [NOTE] This routine does not resolve multiple port numbers residing on the
    same machine.
    [RETURNS] The default port number on success, else -1.
*/
{
    int display_num;
    int service_num;

    /*  Get display number  */
    if ( ( display_num = r_get_display_num_from_display (display) ) < 0 )
    {
	return (-1);
    }
    /*  Get Karma service number  */
    if ( ( service_num = r_get_service_number (module_name) ) < 0 )
    {
	fprintf (stderr, "Service number not found for module: \"%s\"\n",
		 module_name);
	return (-1);
    }
    /*  Hash in UID in an attempt to partition users from each other. Add 1 to
	UID just in case root runs a module  */
    service_num = ( service_num * (getuid () + 1) ) % MODULUS_VALUE;
    return (SERVICE_TO_PORT_FACTOR * service_num +
	    display_num * DISPLAY_TO_PORT_FACTOR);
}   /*  End Function r_get_def_port  */


/*  Private functions follow  */

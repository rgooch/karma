/*LINTLIBRARY*/
/*PREFIX:"r_"*/
/*  port_number.c

    This code provides Karma port number allocation and calculation routines.

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

    Last updated by Richard Gooch   14-SEP-1993: Modified  r_get_service_number
  to use hashing function instead of services file.


*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/param.h>
#include <karma.h>
#include <karma_r.h>

#define DISPLAYS_PER_MACHINE 5
#define SERVICE_TO_PORT_FACTOR 5
#define DISPLAY_TO_PORT_FACTOR (SERVICE_TO_PORT_FACTOR * DISPLAYS_PER_MACHINE)

/*  Private functions  */
#ifdef obsolete
static int search_file_for_named_value (/* filename, value_name */);
#endif


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
char *r_get_karmabase ()
/*  This routine will get the pathname of the installed runtime Karma.
    The routine returns the pathname.
*/
{
    char *karmabase;
    static char *def_karmabase = "/usr/local/karma";

    if ( ( karmabase = r_getenv ("KARMABASE") ) == NULL )
    {
	(void) fprintf (stderr, "Environment variable KARMABASE not found\n");
	(void) fprintf (stderr, "Defaulting to: %s\n", def_karmabase);
	karmabase = def_karmabase;
    }
    return (karmabase);
}   /*  End Function r_get_karmabase  */

/*PUBLIC_FUNCTION*/
int r_get_service_number (module_name)
/*  This routine uses a hashing function to determine the service number of the
    module with name pointed to by  module_name  .
    The routine will return the service number.
*/
char *module_name;
{
    int num = 1;
    int count;
    int len;

    len = strlen (module_name);
    for (count = 0; count < len; ++count)
    {
	if ( (module_name[count] <= ' ') || (module_name[count] > '~') )
	{
	    (void) fprintf (stderr, "Illegal character: value: %d\n",
			    module_name[count]);
	    return (-1);
	}
	num = num * (module_name[count] - ' ');
	num += count;
	num %= 4003;
	if (num < 1) num = 1;
    }
    return (num);
}   /*  End Function r_get_service_number  */

/*PUBLIC_FUNCTION*/
char *r_get_host_from_display (display)
/*  This routine will get the host to connect to from the string pointed to by
    display  .If this is NULL, the host "unix" is returned.
    The syntax for  display  is the same as for the X Windows system  DISPLAY
    environmental variable.
    The routine returns a pointer to a statically allocated string which will
    contain the host name on success, else it returns NULL.

*/
char *display;
{
    char *char_ptr;
    static char host[STRING_LENGTH];

    if (display != NULL)
    {
	/*  display  exists  */
	if ( ( char_ptr = strchr (display, ':') ) == NULL )
	{
	    /*  Error in format  */
	    (void) fprintf (stderr,
			    "Error in display format: \"%s\"\n",
			    display);
	    return (NULL);
	}
	if (char_ptr != display)
	{
	    /*  Not a ":d.s" string  */
	    (void) strncpy ( host, display,
			    (unsigned int) (char_ptr - display) );
	    host[char_ptr - display] = '\0';
	}
	else
	{
	    (void) strcpy (host, "unix");
	}
    }
    else
    {
	/*  display  does not exist  */
	(void) strcpy (host, "unix");
    }
    return (host);
}   /*  End Function r_get_host_from_display  */

/*PUBLIC_FUNCTION*/
int r_get_display_num_from_display (display)
/*  This routine will get the display number from the string pointed to by
    display  (following the X Windows system syntax for the  DISPLAY
    environmental variable).
    If  display  is a NULL pointer, the display number defaults to 0
    The routine returns the display number on success, else it returns -1
*/
char *display;
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
	(void) fprintf (stderr,
			"Error in  display  format: \"%s\"\n",
			display);
	return (-1);
    }
    ++char_ptr;
    if ( ( display_num = atoi (char_ptr) ) < 1 )
    {
	/*  Display number of 0 or smaller: check for '0' character  */
	if (*char_ptr != '0')
	{
	    (void) fprintf (stderr,
			    "Error in  display  format: \"%s\"\n",
			    display);
	    return (-1);
	}
    }
    return (display_num);
}   /*  End Function r_get_display_num_from_display  */

/*PUBLIC_FUNCTION*/
int r_get_screen_num_from_display (display)
/*  This routine will get the screen number from the string pointed to by
    display  (following the X Windows system syntax for the  DISPLAY
    environmental variable).
    If  display  is a NULL pointer, the screen number defaults to 0
    The routine returns the display number on success, else it returns -1
*/
char *display;
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
	(void) fprintf (stderr,
			"Error in  display  format: \"%s\"\n",
			display);
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
	    (void) fprintf (stderr,
			    "Error in  display  format: \"%s\"\n",
			    display);
	    return (-1);
	}
    }
    return (screen_num);
}   /*  End Function r_get_screen_num_from_display  */

/*PUBLIC_FUNCTION*/
int r_get_def_port (module_name, display)
/*  This routine will get the default Karma port number for the module with
    name pointed to by  module_name  .
    If  display  is not a NULL pointer, the display number entry in the string
    it points to is also used to compute the port number. The syntax for this
    string is the same as for the X Windows system  DISPLAY environmental
    variable.
    This routine does not resolve multiple port numbers residing on the same
    machine.
    The routine returns the default port number on success, else it returns -1
*/
char *module_name;
char *display;
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
	(void) fprintf (stderr,
			"Service number not found for module: \"%s\"\n",
			module_name);
	return (-1);
    }
    return (SERVICE_TO_PORT_FACTOR * service_num +
	    display_num * DISPLAY_TO_PORT_FACTOR);
}   /*  End Function r_get_def_port  */


/*  Private functions follow  */
#ifdef obsolete
static int search_file_for_named_value (filename, value_name)
/*  This routine will search the file specified by  filename  for an occurrence
    of the string pointed to by  value_name  .
    If the string is found, the value specified after the value name on the
    same line is read in.
    The routine returns the value for the name on success.
    The routine returns -1 if the named value does not exist in the file.
    The routine ignores blank lines and lines commented with a '#' as the
    first character.
*/
char *filename;
char *value_name;
{
    FILE *fp;
    int value;
    char txt[STRING_LENGTH + 1];
    char file_value_name[STRING_LENGTH + 1];

    if ( ( fp = fopen (filename, "r") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening file: %s\n", filename);
	(void) exit (RV_CANNOT_OPEN);
    }
    /*  Now read lines from the file until either a match is found
	or the end of file is detected.
	*/
    while (fgets (txt, STRING_LENGTH, fp) != NULL)
    {
	/*  Not end of file  */
	if ( (txt[0] == '#') || (txt[0] == '\n') )
	{
	    /*  Comment or blank line: skip  */
	    continue;
	}
	switch ( sscanf (txt, " %s %d", file_value_name, &value) )
	{
	  case 2:
	    /*  Found a valid line  */
	    if (strcmp (file_value_name, value_name) == 0)
	    {
		(void) fclose (fp);
		return (value);
	    }
	    /*  Did not match: go on  */
	    break;
	  default:
	    /*  Error converting line  */
	    (void) fclose (fp);
	    (void) fprintf (stderr, "Error in file: %s\n", filename);
	    (void) fprintf (stderr, "Bad line: \"%s\"\n", txt);
	    (void) exit (RV_BAD_DATA);
	    break;
	}
    }
    /*  End of file  */
    (void) fclose (fp);
    return (-1);
}   /*  End Function search_file_for_named_value  */
#endif  /*  obsolete  */

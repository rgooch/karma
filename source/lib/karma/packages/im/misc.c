/*LINTLIBRARY*/
/*  misc.c

    This code provides simple module initialisation routines.

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

/*

    This file contains the various Karma utility routines which perform low
  level initialisation of a module.


    Written by      Richard Gooch   3-OCT-1992

    Updated by      Richard Gooch   12-DEC-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/im/misc.c

    Updated by      Richard Gooch   15-APR-1995: Created
  <im_register_lib_version>.

    Last updated by Richard Gooch   5-MAY-1995: Placate SGI compiler.


*/
#include <stdio.h>
#include <karma.h>
#include <karma_im.h>
#include <karma_a.h>

/*  Private data  */


/*PUBLIC_FUNCTION*/
void im_register_module_name (char *name_string)
/*  [PURPOSE] This routine will register a new name for the module.
    <name_string> The name string.
    [RETURNS] Nothing.
*/
{
    extern char module_name[STRING_LENGTH + 1];
    static char function_name[] = "im_register_module_name";

    if (strcmp (module_name, name_string) == 0)
    {
	return;
    }
    if (strcmp (module_name, "<<Unknown>>") != 0)
    {
	(void) fprintf (stderr,
			"Attempt to overwrite existing module name: \"%s\" with: \"%s\"\n",
			module_name, name_string);
	a_prog_bug (function_name);
    }
    if (strlen (name_string) > STRING_LENGTH)
    {
	(void) fprintf (stderr, "Name string: \"%s\" is too long\n",
			name_string);
	a_prog_bug (function_name);
    }
    (void) strcpy (module_name, name_string);
}   /*  End Function im_register_module_name  */

/*PUBLIC_FUNCTION*/
void im_register_module_version_date (char *date_string)
/*  [PURPOSE] This routine will register a new version date for the module.
    <date_string> The date string.
    [RETURNS] Nothing.
*/
{
    extern char module_version_date[STRING_LENGTH + 1];
    static char function_name[] = "im_register_module_version_date";

    if (strlen (date_string) > STRING_LENGTH)
    {
	(void) fprintf (stderr, "Date string: \"%s\" is too long\n",
			date_string);
	a_prog_bug (function_name);
    }
    (void) strcpy (module_version_date, date_string);
}   /*  End Function im_register_module_version_date  */

/*PUBLIC_FUNCTION*/
void im_register_lib_version (char *version_string)
/*  [PURPOSE] This routine will register the library version a module was
    compiled with.
    <version_string> The version string of the library.
    [RETURNS] Nothing.
*/
{
    extern char module_lib_version[STRING_LENGTH + 1];
    static char function_name[] = "im_register_lib_version";

    if (strlen (version_string) > STRING_LENGTH)
    {
	(void) fprintf (stderr, "Version string: \"%s\" is too long\n",
			version_string);
	a_prog_bug (function_name);
    }
    (void) strcpy (module_lib_version, version_string);
}   /*  End Function im_register_lib_version  */

/*LINTLIBRARY*/
/*  misc.c

    This code provides history save/ restore routines.

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

    This file contains the various Karma utility routines which relate to
  saving and restoring history (parameter) information.


    Written by      Richard Gooch   3-OCT-1992

    Updated by      Richard Gooch   13-DEC-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   4-OCT-1993: Changed from "?!" to "?#" for
  parameter display command (in accordance with new  param_  package).

    Updated by      Richard Gooch   22-NOV-1993: Removed trailing '\n'
  character when reading lines from file.

    Updated by      Richard Gooch   4-OCT-1994: Changed to  getcwd  routine in
  order to avoid having to link with buggy UCB compatibility library in
  Slowaris 2.3

    Updated by      Richard Gooch   5-OCT-1994: Worked around strange SEGV
  problem with rs6000/AIX.

    Updated by      Richard Gooch   3-NOV-1994: Switched to OS_ and MACHINE_
  macros for machine determination.

    Updated by      Richard Gooch   26-NOV-1994: Moved to  packages/hi/misc.c

    Updated by      Richard Gooch   7-DEC-1994: Stripped declaration of  errno
  and added #include <errno.h>

    Updated by      Richard Gooch   8-JUN-1995: Added #ifdef OS_UNICOS

    Updated by      Richard Gooch   20-FEB-1996: Increased size of temporary
  filename strings.

    Last updated by Richard Gooch   12-APR-1996: Changed to new documentation
  format.


*/
#include <stdio.h>
#include <stdlib.h>
#if defined(OS_MSDOS) && !defined(__GNUC__)
#  include <io.h>
#else
#  include <unistd.h>
#  include <sys/param.h>
#endif
#include <string.h>
#include <errno.h>
#include <karma.h>
#include <karma_hi.h>
#include <karma_r.h>

#if defined(OS_MSDOS) || defined(OS_HPUX)
extern char *getcwd ();
#endif

#ifdef OS_MSDOS
#  ifndef MAXPATHLEN
#    define MAXPATHLEN FILENAME_MAX
#  endif
#endif
#ifdef OS_UNICOS
#  ifndef MAXPATHLEN
#    define MAXPATHLEN 50
#  endif
#endif

STATIC_FUNCTION (char *find_filename_in_tree, (CONST char *filename) );


/*PUBLIC_FUNCTION*/
void hi_read (CONST char *module_name,
	      flag (*command_decode) (CONST char *command, FILE *fp) )
/*  [SUMMARY] Read history information for a module.
    [PURPOSE] This routine will read any available history information for a
    module and process accordingly.
    <module_name> The name of the module.
    <command_decode> The function used to decode history lines.
    [RETURNS] Nothing.
*/
{
    FILE *fp;
    char *home = NULL;
    char *pathname;
    char file_name[STRING_LENGTH];
    char line[STRING_LENGTH];

    (void) strcpy (file_name, ".");
    (void) strcat (file_name, module_name);
    (void) strcat (file_name, ".defaults");
    /*  Try to find file somewhere up there ...  */
    if ( ( pathname = find_filename_in_tree (file_name) ) == NULL )
    {
	/*  Not found: look in "$HOME/.KARMAdefaults"  */
	/*  Find where HOME is  */
	if ( ( home = r_getenv ("HOME") ) == NULL )
	{
	    fprintf (stderr, "Environmental variable HOME not found\n");
	    return;
	}
	(void) sprintf (line, "%s/.KARMAdefaults/%s", home, file_name);
	if ( ( fp = fopen (line, "r") ) == NULL )
	{
	    /*  Not found: look in "$KARMABASE/defaults"  */
	    (void) sprintf (line, "%s/defaults/%s", r_get_karmabase (),
			    file_name);
	    if ( ( fp = fopen (line, "r") ) == NULL )
	    {
		/*  Still not found: can't be found  */
		return;
	    }
	}
    }
    else
    {
	/*  Use default file in directory ...  */
	if ( ( fp = fopen (pathname, "r") ) == NULL )
	{
	    return;
	}
    }
    while (fgets (line, STRING_LENGTH, fp) != NULL)
    {
	if (line[strlen (line) - 1] == '\n') line[strlen (line) - 1] = '\0';
	if (line[0] && line[0] != '\n') (void) (*command_decode) (line,stderr);
    }
    (void) fclose (fp);
}   /*  End Function hi_read  */

/*PUBLIC_FUNCTION*/
void hi_write (CONST char *module_name,
	       flag (*command_decode) (CONST char *command, FILE *fp) )
/*  [SUMMARY] Write history information for a module.
    [PURPOSE] This routine will write history information for the module in the
    current working directory.
    <module_name> The name of the module.
    <command_decode> The function used to decode history lines.
    [RETURNS] Nothing.
*/
{
    FILE *fp;
    char file_name[STRING_LENGTH];
    extern char *sys_errlist[];

    (void) sprintf (file_name, ".%s.defaults", module_name);
#ifdef OS_VXMVX
    (void) fprintf (stderr, "Writing history for: \"%s\" to file: \"%s\"\n",
		    module_name, file_name);
#endif
    /*  Write new defaults file to current working directory  */
    if ( ( fp = fopen (file_name, "w") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening: \"%s\"\t%s\n",
			file_name, sys_errlist[errno]);
	exit (RV_CANNOT_OPEN);
    }
    if ( !(*command_decode) ("?#", fp) )
    {
	(void) fprintf (stderr, "Error in writing defaults\n");
    }
    (void) fprintf (fp, "\n");
    (void) fclose (fp);
}  /*  End Function hi_write  */

static char *find_filename_in_tree (CONST char *filename)
/*  This routine will search the current working directory for a defaults file
    with filename (stripped of any directory specifications) pointed to by
    filename  .
    The routine will search the current working directory and all parent
    directories for a file readable by the user until one is found.
    The routine will return a pointer to a statically allocated pathname if
    a readable filename exists, else it returns NULL.
    The routine will return the pointer  filename  if the file exists in the
    current working directory.
*/
{
    char *path_ptr, *pathname_ptr;
    char curr_dir[MAXPATHLEN + 1];
    static char pathname[MAXPATHLEN + STRING_LENGTH + 1];

    if (access (filename, R_OK) == 0)
    {
	/*  Found file  */
	return ( (char *) filename );
    }
    pathname_ptr = pathname;
    /*  Search up directory tree  */
    if (getcwd (curr_dir, MAXPATHLEN) != curr_dir)
    {
	(void) fprintf (stderr,
			"Error getting current working directory:\t%s\n",
			curr_dir);
	return (NULL);
    }
    curr_dir[MAXPATHLEN] = '\0';
    if (strcmp (curr_dir, "/") == 0)
    {
	/*  Already at root  */
	return (NULL);
    }
    path_ptr = curr_dir + strlen (curr_dir);
    while (path_ptr >= curr_dir)
    {
	if (*path_ptr == '/')
	{
	    /*  Next directory level up  */
	    (void) strncpy (pathname, curr_dir, path_ptr - curr_dir + 1);
	    /*  Must reference through  pathname_ptr  rather than  pathname
		because of bug in AIX/rs6000 compiler.  */
	    pathname_ptr[path_ptr - curr_dir + 1] = '\0';
	    (void) strcat (pathname, filename);
	    if (access (pathname, R_OK) == 0)
	    {
		/*  Found file  */
		return ( (char *) pathname );
	    }
	}
	--path_ptr;
    }
    /*  Not found anywhere in directories  */
    return (NULL);
}   /*  End Function find_filename_in_tree  */

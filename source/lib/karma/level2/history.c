/*LINTLIBRARY*/
/*PREFIX:"hi_"*/
/*  history.c

    This code provides history save/ restore routines.

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

    This file contains the various Karma utility routines which relate to
  saving and restoring history (parameter) information.


    Written by      Richard Gooch   3-OCT-1992

    Updated by      Richard Gooch   13-DEC-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   4-OCT-1993: Changed from "?!" to "?#" for
  parameter display command (in accordance with new  param_  package).

    Last updated by Richard Gooch   22-NOV-1993: Removed trailing '\n'
  character when reading lines from file.


*/
#include <stdio.h>
#include <stdlib.h>
#if defined(ARCH_MS_DOS_386) && !defined(__GNUC__)
#include <io.h>
#else
#include <unistd.h>
#include <sys/param.h>
#endif
#include <string.h>
#include <karma.h>
#include <karma_hi.h>
#include <karma_r.h>

#if defined(ARCH_MS_DOS_386) || defined(ARCH_hp9000)
extern char *getcwd ();
#else
extern char *getwd ();
#endif

#ifdef ARCH_MS_DOS_386
#  ifndef MAXPATHLEN
#  define MAXPATHLEN FILENAME_MAX
#  endif
#endif

static char *find_filename_in_tree ();


/*PUBLIC_FUNCTION*/
void hi_read (command_name,command_decode)
/* Read defaults file and process */
char *command_name;        /* name of command */
flag (*command_decode) ();  /* Function to decode commands in defaults file */
{
    FILE           *fp;		/* pointer to file */
    char *home = NULL;
    char *pathname;
    char            file_name[32];	/* name of defaults file */
    char            line[256];	/* buffer for data from file */

    (void) strcpy (file_name, ".");
    (void) strcat (file_name, command_name);
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
    for (;;)
    {
	if (!fgets(line, sizeof(line), fp))
	    break;
	if (line[strlen (line) - 1] == '\n') line[strlen (line) - 1] = '\0';
	if (line[0] && line[0] != '\n')
	(void) (*command_decode) (line, stderr);
    }
    fclose (fp);
}   /*  End Function hi_read  */

/*PUBLIC_FUNCTION*/
void hi_write (command_name,command_decode)
char *command_name;        /* name of command */
flag (*command_decode) ();  /* Function to decode commands in defaults file */
{
    FILE *fp;
    char file_name[32];	/* name of defaults file */
    ERRNO_TYPE errno;
    extern char *sys_errlist[];

    (void) sprintf (file_name, ".%s.defaults", command_name);
#ifdef ARCH_VXMVX
    (void) fprintf (stderr, "Writing history for: \"%s\" to file: \"%s\"\n",
		    command_name, file_name);
#endif
    /*  Write new defaults file to current working directory  */
    if ( ( fp = fopen (file_name, "w") ) == NULL )
    {
	(void) fprintf (stderr, "Error opening: \"%s\"\t%s\n",
			file_name, sys_errlist[errno]);
	exit (RV_CANNOT_OPEN);
    }
    if ( (*command_decode) ("?#", fp) != TRUE )
    {
	(void) fprintf (stderr, "Error in writing defaults\n");
    }
    (void) fprintf (fp, "\n");
    (void) fclose (fp);
}  /*  End Function hi_write  */

static char *find_filename_in_tree (filename)
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
char *filename;
{
    char *path_ptr;
    char curr_dir[MAXPATHLEN + 1];
    static char pathname[MAXPATHLEN + STRING_LENGTH + 1];

    if (access (filename, R_OK) == 0)
    {
	/*  Found file  */
	return (filename);
    }
    /*  Search up directory tree  */
#if defined(ARCH_MS_DOS_386) || defined(ARCH_hp9000)
    if (getcwd (curr_dir, MAXPATHLEN) != curr_dir)
#else
    if (getwd (curr_dir) != curr_dir)
#endif
    {
	(void) fprintf (stderr,
			"Error getting current working directory:\t%s\n",
			curr_dir);
	return (NULL);
    }
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
	    strncpy (pathname, curr_dir, path_ptr - curr_dir);
	    pathname[path_ptr - curr_dir] = '/';
	    pathname[path_ptr - curr_dir + 1] = '\0';
	    strcat (pathname, filename);
	    if (access (pathname, R_OK) == 0)
	    {
		/*  Found file  */
		return (pathname);
	    }
	}
	--path_ptr;
    }
    /*  Not found anywhere in directories  */
    return (NULL);
}   /*  End Function find_filename_in_tree  */

/*LINTLIBRARY*/
/*PREFIX:"dir_"*/
/*  file.c

    This code provides an enhanced directory scanning routines.

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

/*  This file contains all routines needed for the enhanced, portable scanning
  of directories..


    Written by      Richard Gooch   1-JUN-1993

    Last updated by Richard Gooch   2-JUN-1993


*/
#include <stdio.h>
#include <ctype.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <dirent.h>
#include <errno.h>
#include <karma.h>
#include <karma_st.h>
#include <karma_m.h>
#include <karma_p.h>
#include <karma_a.h>


/*  Internal definition of KDir object  */
typedef struct dir_type * KDir;

#define KDIR_DEFINED
#include <karma_dir.h>

#if __STDC__ == 1
#  define MAGIC_NUMBER 2942390429u
#else
#  define MAGIC_NUMBER (unsigned int) 2942390429
#endif

struct dir_type
{
    unsigned int magic_number;
    char *dirname;
    DIR *dirp;
    KFileInfo file;
    char sym_path[STRING_LENGTH];
};


/*  Public functions follow  */

/*PUBLIC_FUNCTION*/
KDir dir_open (dirname)
/*  This routine will open a directory for reading (scanning).
    The directory name must be pointed to by  dirname  .
    The routine routines returns a KDir object on success, else it returns NULL
*/
char *dirname;
{
    KDir dir;
    DIR *dirp;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "dir_open";

    if ( ( dirp = opendir (dirname) ) == NULL )
    {
	(void) fprintf (stderr, "Error opening directory: \"%s\"\t%s\n",
			dirname, sys_errlist[errno]);
	return (NULL);
    }
    if ( ( dir = (KDir) m_alloc (sizeof *dir) ) == NULL )
    {
	m_error_notify (function_name, "directory object");
	return (NULL);
    }
    if ( ( (*dir).dirname = st_dup (dirname) ) == NULL )
    {
	m_error_notify (function_name, "directory name");
	m_free ( (char *) dir );
	return (NULL);
    }
    (*dir).magic_number = MAGIC_NUMBER;
    (*dir).dirp = dirp;
    return (dir);
}   /*  End Function dir_open  */

/*PUBLIC_FUNCTION*/
KFileInfo *dir_read (dir, skip_control)
/*  This routine will read (scan) a directory for files.
    The directory object must be given by  dir  .
    The value of  skip_control  determines whether or not to skip the special
    files: "." and "..". Legal values are:
        KDIR_DOT                  Pass ".", skip ".."
	KDIR_DOTDOT               Pass "..", skip "."
	KDIR_DOT_AND_DOTDOT       Pass "." and ".."
	KDIR_NO_DOTS              Skip "." and ".."
    The routine routines returns a pointer to a  KFileInfo  structure on
    success, else it returns NULL. The data in this structure is valid until
    the next call to  dir_read  or  dir_close  with this directory object.
*/
KDir dir;
unsigned int skip_control;
{
    flag scan_another = TRUE;
    flag other = TRUE;
    int sym_length;
    long tmp_numgrp;
    long count;
    struct stat statbuf;
    KFileInfo *file;
    struct dirent *dp;
    char pathname[STRING_LENGTH];
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static uid_t euid = -1;
    static gid_t *groups = NULL;
    static long num_groups = -1;
    static char function_name[] = "dir_read";

    /*  Initialisation  */
    if (num_groups < 0)
    {
	euid = geteuid ();
	if ( ( tmp_numgrp = sysconf (_SC_NGROUPS_MAX) ) == -1 )
	{
	    (void) fprintf (stderr, "Error getting NGROUPS_MAX\t%s\n",
			    sys_errlist[errno]);
	    return (NULL);
	}
	if ( ( groups = (gid_t *) m_alloc (sizeof *groups * tmp_numgrp) )
	    == NULL )
	{
	    m_abort (function_name, "array of process group IDs");
	}
	if ( ( num_groups = getgroups (tmp_numgrp, groups) ) < 0 )
	{
	    (void) fprintf (stderr, "Error getting process group IDs\t%s\n",
			    sys_errlist[errno]);
	    return (NULL);
	}
    }
    if (dir == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if directory is valid  */
    if ( (*dir).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid directory object\n");
	a_prog_bug (function_name);
    }
    while (scan_another)
    {
	if ( ( dp = readdir ( (*dir).dirp ) ) == NULL )
	{
	    return (NULL);
	}
	switch (skip_control)
	{
	  case KDIR_DOT:
	    if (strcmp ("..", (*dp).d_name) == 0) continue;
	    break;
	  case KDIR_DOTDOT:
	    if (strcmp (".", (*dp).d_name) == 0) continue;
	    break;
	  case KDIR_DOT_AND_DOTDOT:
	    break;
	  case KDIR_NO_DOTS:
	    if (strcmp (".", (*dp).d_name) == 0) continue;
	    if (strcmp ("..", (*dp).d_name) == 0) continue;
	    break;
	  default:
	    (void) fprintf (stderr, "Illegal value of  skip_control: %u\n",
			    skip_control);
	    a_prog_bug (function_name);
	    break;
	}
	scan_another = FALSE;
    }
    file = &(*dir).file;
    (*file).filename = (*dp).d_name;
    /*  Construct full pathname  */
    (void) strcpy (pathname, (*dir).dirname);
    (void) strcat (pathname, "/");
    (void) strcat (pathname, (*file).filename);
    /*  Get stats on file  */
    if (lstat (pathname, &statbuf) != 0)
    {
	(void) fprintf (stderr, "Error getting stats on file: \"%s\"\t$s\n",
			pathname, sys_errlist[errno]);
	return (NULL);
    }
    (*file).type = KFILETYPE_REGULAR;
    if ( S_ISLNK (statbuf.st_mode) )
    {
	/*  Symbolic link  */
	if (stat (pathname, &statbuf) != 0)
	{
	    if (errno == ENOENT)
	    {
		/*  Symbolic link does not point to anything  */
		(*file).type = KFILETYPE_DANGLING_SYMLINK;
	    }
	    else
	    {
		(void) fprintf (stderr,
				"Error getting stats on symlink: \"%s\"\t%s\n",
				pathname, sys_errlist[errno]);
		return (NULL);
	    }
	}
	if ( ( sym_length = readlink (pathname, (*dir).sym_path,
				      STRING_LENGTH - 1) ) == -1 )
	{
	    (void) fprintf (stderr,
			    "Error reading symbolic link: \"%s\"\t%s\n",
			    pathname, sys_errlist[errno]);
	    return (NULL);
	}
	(*dir).sym_path[sym_length] = '\0';
	(*file).sym_path = (*dir).sym_path;
	(*file).is_symlink = TRUE;
	if ( (*file).type == KFILETYPE_DANGLING_SYMLINK )
	{
	    return (file);
	}
    }
    else
    {
	(*file).sym_path = NULL;
	(*file).is_symlink = FALSE;
    }
    if ( S_ISDIR (statbuf.st_mode) )
    {
	(*file).type = KFILETYPE_DIRECTORY;
    }
    else if ( S_ISCHR (statbuf.st_mode) )
    {
	(*file).type = KFILETYPE_CHAR;
    }
    else if ( S_ISBLK (statbuf.st_mode) )
    {
	(*file).type = KFILETYPE_BLOCK;
    }
    else if ( S_ISREG (statbuf.st_mode) )
    {
	(*file).type = KFILETYPE_REGULAR;
    }
    else if ( S_ISSOCK (statbuf.st_mode) )
    {
	(*file).type = KFILETYPE_SOCKET;
    }
    else if ( S_ISFIFO (statbuf.st_mode) )
    {
	(*file).type = KFILETYPE_FIFO;
    }
    else
    {
	(void) fprintf (stderr, "Unknown file mode: %d\n",
			(int) statbuf.st_mode);
	return (NULL);
    }
    (*file).uid = statbuf.st_uid;
    (*file).gid = statbuf.st_gid;
    (*file).mode = statbuf.st_mode;
    (*file).num_links = statbuf.st_nlink;
    (*file).dev_num = statbuf.st_dev;
    (*file).inode = statbuf.st_ino;
    (*file).size = statbuf.st_size;
    (*file).atime = statbuf.st_atime;
    (*file).mtime = statbuf.st_mtime;
    (*file).ctime = statbuf.st_ctime;
/*  NOT PORTABLE (not in POSIX)
    (*file).blocksize = statbuf.st_blksize;
*/
    /*  Determine file access permissions for this process  */
    (*file).can_read = FALSE;
    (*file).can_write = FALSE;
    (*file).can_execute = FALSE;
    if (euid == (*file).uid)
    {
	/*  Owner  */
	if (S_IRUSR & (*file).mode) (*file).can_read = TRUE;
	if (S_IWUSR & (*file).mode) (*file).can_write = TRUE;
	if (S_IXUSR & (*file).mode) (*file).can_execute = TRUE;
	other = FALSE;
    }
    else
    {
	/*  See if in group  */
	for (count = 0; count < num_groups; ++count)
	{
	    if (groups[count] == (*file).gid)
	    {
		/*  In the group  */
		if (S_IRGRP & (*file).mode) (*file).can_read = TRUE;
		if (S_IWGRP & (*file).mode) (*file).can_write = TRUE;
		if (S_IXGRP & (*file).mode) (*file).can_execute = TRUE;
		other = FALSE;
		count = num_groups;
	    }
	}
    }
    if (other)
    {
	if (S_IROTH & (*file).mode) (*file).can_read = TRUE;
	if (S_IWOTH & (*file).mode) (*file).can_write = TRUE;
	if (S_IXOTH & (*file).mode) (*file).can_execute = TRUE;
    }
    /*  Determine if local filesystem  */
    if ( (*file).dev_num < 0 )
    {
	(*file).local_fs = FALSE;
    }
    else
    {
	(*file).local_fs = TRUE;
    }
    return (file);
}   /*  End Function dir_read  */

/*PUBLIC_FUNCTION*/
void dir_close (dir)
/*  This routine will close a directory.
    The directory object must be given by  dir  .
*/
KDir dir;
{
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    static char function_name[] = "dir_close";

    if (dir == NULL)
    {
	(void) fprintf (stderr, "NULL pointer passed\n");
	a_prog_bug (function_name);
    }
    /*  Check if directory is valid  */
    if ( (*dir).magic_number != MAGIC_NUMBER )
    {
	(void) fprintf (stderr, "Invalid directory object\n");
	a_prog_bug (function_name);
    }
    if (closedir ( (*dir).dirp ) != 0)
    {
	(void) fprintf (stderr,
			"Error closing directory: \"%s\"\t%s\n",
			(*dir).dirname, sys_errlist[errno]);
    }
    m_free ( (char *) (*dir).dirname );
    m_free ( (char *) dir );
}   /*  End Function dir_close  */

/*  karma_dir.h

    Header for  dir_  package.

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

    This include file contains all the definitions and function declarations
  needed to interface to the dir_ routines in the Karma library.


    Written by      Richard Gooch   1-JUN-1993

    Last updated by Richard Gooch   24-JUN-1996

*/

#include <sys/types.h>
#if !defined(KARMA_H) || defined(MAKEDEPEND)
#  include <karma.h>
#endif

#ifndef KARMA_DIR_H
#define KARMA_DIR_H


typedef struct dir_type * KDir;


#define KDIR_DOT (unsigned int) 0
#define KDIR_DOTDOT (unsigned int) 1
#define KDIR_DOT_AND_DOTDOT (unsigned int) 2
#define KDIR_NO_DOTS (unsigned int) 3

#define KFILETYPE_DIRECTORY (unsigned int) 0
#define KFILETYPE_CHAR (unsigned int) 1
#define KFILETYPE_BLOCK (unsigned int) 2
#define KFILETYPE_REGULAR (unsigned int) 3
#define KFILETYPE_SOCKET (unsigned int) 4
#define KFILETYPE_FIFO (unsigned int) 5
#define KFILETYPE_DANGLING_SYMLINK (unsigned int) 6

typedef struct
{
    char *filename;         /*  Filename-only component of pathname          */
    char *sym_path;         /*  Translated symbolic link pathname            */
    unsigned int type;      /*  File type (eg. KFILETYPE_REGULAR)            */
    flag is_symlink;        /*  TRUE if file is a translated symlink         */
    uid_t uid;              /*  User ID of owner                             */
    gid_t gid;              /*  Group ID of owner                            */
    mode_t mode;            /*  Access mode- see stat(2)                     */
    nlink_t num_links;      /*  Number of hard links to file                 */
    dev_t dev_num;          /*  Device number                                */
    ino_t inode;            /*  Inode number                                 */
    off_t size;             /*  Size of file in bytes                        */
    time_t atime;           /*  Last access time                             */
    time_t mtime;           /*  Last modification time                       */
    time_t ctime;           /*  Last status change time                      */
#ifdef NOT_IMPLEMENTED
    long blocksize;         /*  Preferrred blocksize for filesystem IO       */
    long blocks_allocated;  /*  Number of blocks actually allocated          */
#endif
    flag can_read;          /*  TRUE if process can read file                */
    flag can_write;         /*  TRUE if process can write file               */
    flag can_execute;       /*  TRUE if process can execute file             */
#ifdef NOT_IMPLEMENTED
    char *fs_type;          /*  Filesystem type (eg. "4.2", "nfs", "minix")  */
#endif
    flag local_fs;          /*  TRUE if locally mounted filesystem           */
    CONST char *dirname;    /*  Directory component of pathname              */
    void *private_ptr;
} KFileInfo;

typedef struct
{ 
    unsigned int num_entries;
    KFileInfo *entries;
} KDirInfo;

/*  File:   dir_scan.c   */
EXTERN_FUNCTION (KDir dir_open, (CONST char *dirname) );
EXTERN_FUNCTION (KFileInfo *dir_read, (KDir dir, unsigned int skip_control) );
EXTERN_FUNCTION (void dir_rewind, (KDir dir) );
EXTERN_FUNCTION (void dir_close, (KDir dir) );
#ifdef NOT_IMPLEMENTED
EXTERN_FUNCTION (KDirInfo *dir_get_directory, (char *dirname,
					       unsigned int skip_control) );

EXTERN_FUNCTION (char *dir_get_uname, (KFileInfo file) );
EXTERN_FUNCTION (char *dir_get_gname, (KFileInfo file) );
EXTERN_FUNCTION (char *dir_get_other_hardlinks, (KFileInfo file) );
EXTERN_FUNCTION (KFileInfo *dir_stat_file, (char *filename) );
#endif


#endif  /*  KARMA_DIR_H  */

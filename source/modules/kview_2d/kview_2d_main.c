/*  kview_2d_main.c

    Main file for  kview_2d  (X11 image display tool for Karma).

    Copyright (C) 1993  Richard Gooch

    This program is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.

    Richard Gooch may be reached by email at  karma-request@atnf.csiro.au
    The postal address is:
      Richard Gooch, c/o ATNF, P. O. Box 76, Epping, N.S.W., 2121, Australia.
*/

/*
    This Karma module will enable on-screen imaging of a single arrayfile of
    the general data structure format. The arrayfile must contain a 2
    dimensional "Intelligent Array".
    This module runs on an X11 server.


    Written by      Richard Gooch   12-JUL-1993

    Last updated by Richard Gooch   28-SEP-1993


*/
#include <stdio.h>
#include "karma.h"
#include <karma_im.h>
#include <karma_hi.h>
#ifdef ARCH_SUNsparc
#  include <floatingpoint.h>
#endif

#define COMMAND_LINE_LENGTH 4096
#define VERSION "1.0"

main (argc, argv)
int argc;       /*  Count of parameters on command line */
char **argv;    /*  List of command line parameters     */
{
    int arg_count;
    int line_length;
    char line[COMMAND_LINE_LENGTH];
    void kview_2d_xview (), (* xview_function) () = kview_2d_xview;
    ERRNO_TYPE errno;
    extern char *sys_errlist[];
    extern char module_name[STRING_LENGTH + 1];

#ifdef SIGFPE_ABORT
	ieee_handler ("set", "division", SIGFPE_ABORT);
#endif
    /*  Initialise module  */
    im_register_module_name ("kview_2d");
    im_register_module_version_date (VERSION);

    /*  Call XView startup  */
    xview_function (argc, argv);
}   /*  End Function main   */

/*LINTLIBRARY*/
/*  globals.c

    This code provides global variables for the  im_  package.

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

    This file contains the definitions for various global variables which are
    used by the im_ routines in the Karma library.


    Written by      Richard Gooch   13-DEC-1992

    Updated by      Richard Gooch   13-DEC-1992

    Updated by      Richard Gooch   6-JAN-1993: Changed from  #include "*.h"
  to  #include <*.h>

    Updated by      Richard Gooch   26-NOV-1994: Moved to
  packages/im/globals.c

    Last updated by Richard Gooch   15-APR-1995: Added library version strings.


*/
#include <karma.h>
#include <karma_im.h>

char module_name[STRING_LENGTH + 1] = "<<Unknown>>";
char module_version_date[STRING_LENGTH + 1] = "Unknown";
char module_lib_version[STRING_LENGTH + 1] = "Unknown";

char karma_library_version[STRING_LENGTH + 1] = KARMA_VERSION;

/*  karma_module.h

    Header for  module_  package.

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

    This include file contains all the definitions and function declarations
  needed to interface to the module_ routines in the Karma library.


    Written by      Richard Gooch   4-OCT-1992

    Last updated by Richard Gooch   14-SEP-1993

*/

#ifndef KARMA_MODULE_H
#define KARMA_MODULE_H


#ifndef EXTERN_FUNCTION
#  include <c_varieties.h>
#endif
#include <stdio.h>
#include <karma_ds_def.h>


/*  File:   module.c  */
EXTERN_FUNCTION (void module_run, (int argc, char **argv, char *name_string,
				   char *version_string,
				   flag (*decode_func) (char *command,
							FILE *fp),
				   int max_incoming, int max_outgoing,
				   flag server) );


#endif /*  KARMA_MODULE_H  */

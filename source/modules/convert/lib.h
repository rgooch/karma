/*  lib.h

    Include file for various conversion modules.

    Copyright (C) 1996  Richard Gooch

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
    This file contains support includes for various conversion modules.


    Written by      Richard Gooch   16-AUG-1996

    Last updated by Richard Gooch   17-AUG-1996


*/

#include <stdio.h>
#include <karma.h>
#include <karma_ds_def.h>
#include <karma_ch_def.h>


EXTERN_FUNCTION (flag command_parse, (char *p, FILE *fp) );
EXTERN_FUNCTION (Channel open_karma,
		 (CONST char *filename, CONST multi_array *multi_desc) );
EXTERN_FUNCTION (flag setup_for_writing,
		 (multi_array *multi_desc, flag tile,
		  flag allow_truncation, uaddr **dim_lengths,
		  uaddr **coords, Channel *karma_ch, CONST char *arrayfile) );
EXTERN_FUNCTION (flag write_blocks,
		 (Channel channel, multi_array *multi_desc,
		  CONST uaddr *dim_lengths, uaddr *coords,
		  CONST char *data, unsigned int num_values) );
EXTERN_FUNCTION (flag write_tail,
		 (Channel channel, multi_array *multi_desc, CONST char *infile,
		  CONST char *arrayfile) );
EXTERN_FUNCTION (void cleanup,
		 (Channel karma_ch, uaddr *dim_lengths, uaddr *coords) );
